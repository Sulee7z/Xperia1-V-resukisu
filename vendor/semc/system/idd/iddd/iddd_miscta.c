/*
 * Copyright 2014 Sony Corporation
 */

/**
 * @brief The MiscTA interface for IDD.
 *
 * @author Leif Mårtensson (leif.martensson@sonymobile.com)
 */

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>

#define LOG_TAG "iddd-miscta"
#include "logging.h"

// MiscTA includes
#include "miscta_units.h"
#include "miscta.h"

enum {MISCTA_NOT_SET = 0, MISCTA_SET = 1};
static uint8_t g_report_data_flag = MISCTA_NOT_SET;
static miscta_status_t g_mt_status = MT_SUCCESS;
static pthread_mutex_t g_mtx = PTHREAD_MUTEX_INITIALIZER;

static const char* miscta_status2str(miscta_status_t status)
{
  switch (status) {
    case MT_SUCCESS:    return "MT_SUCCESS";
    case MT_ERROR:      return "MT_ERROR";
    case MT_NOMEM:      return "MT_NOMEM";
    case MT_TAERR:      return "MT_TAERR";
    case MT_TOOBIG:     return "MT_TOOBIG";
    case MT_NOTFOUND:   return "MT_NOTFOUND";
    case MT_INVALPARAM: return "MT_INVALPARAM";
    case MT_SAMEVAL:    return "MT_SAMEVAL";
  }
  return "no such MiscTA error code";
}

static void *miscta_set_thread()
{
  uint32_t size = sizeof(g_report_data_flag);
  int s;

  s = pthread_mutex_trylock(&g_mtx);
  if (s != 0)
  {
    IDD_LOGE("Failed to trylock mutex before miscta_read_unit, errno=%d (%s)", s, strerror(s));
    return NULL;
  }
  g_mt_status = miscta_read_unit(TA_IDD_REPORT_DATA, (void *)&g_report_data_flag, &size);
  s = pthread_mutex_unlock(&g_mtx);
  if (s != 0)
  {
    IDD_LOGE("Failed to unlock mutex after miscta_read_unit, errno=%d (%s)", s, strerror(s));
    return NULL;
  }
  if (g_mt_status != MT_SUCCESS)
  {
    IDD_LOGE("Error <%s> reading MiscTA TA_IDD_REPORT_DATA! Read %d bytes!",
              miscta_status2str(g_mt_status), size);
  }

  if (g_report_data_flag != (uint8_t)MISCTA_SET)
  {
    g_report_data_flag = MISCTA_SET;
    size = sizeof(g_report_data_flag);
    s = pthread_mutex_trylock(&g_mtx);
    if (s != 0)
    {
      IDD_LOGE("Failed to trylock mutex before miscta_write_unit, errno=%d (%s)", s, strerror(s));
      return NULL;
    }
    g_mt_status = miscta_write_unit(TA_IDD_REPORT_DATA, (void *)&g_report_data_flag, size);
    s = pthread_mutex_unlock(&g_mtx);
    if (s != 0)
    {
      IDD_LOGE("Failed to unlock mutex after miscta_write_unit, errno=%d (%s)", s, strerror(s));
      return NULL;
    }
    if (g_mt_status != MT_SUCCESS)
    {
      g_report_data_flag = MISCTA_NOT_SET;
      IDD_LOGE("Error <%s> writing MiscTA TA_IDD_REPORT_DATA!", miscta_status2str(g_mt_status));
    }
  }
  return NULL;
}

void miscta_set_report_data()
{
  if (g_report_data_flag == MISCTA_NOT_SET && g_mt_status == MT_SUCCESS)
  {
    pthread_t thr;
    pthread_attr_t attr;
    int s;
    s = pthread_attr_init(&attr);
    if (s != 0)
    {
      IDD_LOGE("pthread_attr_init failed, errno=%d (%s)", s, strerror(s));
      return;
    }
    s = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (s != 0)
    {
      IDD_LOGE("pthread_attr_setdetachstate failed, errno=%d (%s)", s, strerror(s));
      return;
    }
    s = pthread_create(&thr, &attr, miscta_set_thread, NULL);
    if (s != 0)
    {
      IDD_LOGE("pthread_create failed, errno=%d (%s)", s, strerror(s));
      return;
    }
    s = pthread_attr_destroy(&attr);
    if (s != 0)
    {
      IDD_LOGE("pthread_attr_destroy failed, errno=%d (%s)", s, strerror(s));
      return;
    }
  }
}
