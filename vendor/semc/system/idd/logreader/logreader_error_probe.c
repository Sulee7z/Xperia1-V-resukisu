/*
 * Copyright 2017 Sony Corporation
 */

/**
 * @brief Idd logreader error probe
 *
 * This probe will send idd logreader errors
 * and logreader error count to idd daemon
 */

#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include <idd_api.h>

#include "config.h"
#include "logreader_error_probe.h"
#include "idd/logreader.pb-c.h"

#undef LOG_TAG
#define LOG_TAG "logreader-error"
#include "../iddd/logging.h"

#define LOG_BUF_SIZE 1024
#define FILE_BUF 256
#define DURATION_BUF 20

#define PROBE_LOGREADERERROR_PKG       "idd.logreader"
#define PROBE_LOGREADERERROR_NAME      "LogReaderError"
#define PROBE_LOGREADERERROR_CNT_NAME  "Cnt_LogReaderError"

/*
 * This is the time period in seconds used in tests to verify that a flooding time period
 * has ended. The value is given in seconds and has to be synchronized with the test.
 */
#define LOGREADER_ERROR_TEST_PERIOD 6

/* The read-protected directory, and its mode. */
#define PRIVATE_DIR_NAME "private/"

/* This file contains data that has to be persistent */
#define LOGREADER_STATE_FNAME PRIVATE_DIR_NAME "logreader.state"

/* logreader error number since timestamp */
#define LOGREADER_STATE_TAG_ERROR_NR "error_nr: "
#define LOGREADER_STATE_TAG_ERROR_TIMESTAMP "error_time: "

static IDD_t *iddp = NULL;

static time_t logreader_error_timestamp = 0;
static int logreader_error_period = 10 * 60 * 60; /* 10 hours */
static int nr_errors = 0;
static int nr_errors_max = 100; /* 100 error probes */

static char* idd_base_dir = NULL;

/*
 * Create an IDD connection
 *
 * @return 1 if IDD connection is successful otherwse 0
 */
static int idd_connect()
{
  if (!iddp)
  {
    iddp = idd_base_dir ? idd_create_from_path(idd_base_dir) : idd_create();
    if (!iddp)
    {
      /* Make sure that all logging in the logreader error probe client does not generate
       * a new error probe. */
      IDD_LOGE("Tried to generate error probe data, failed to create a IDD connection");
    }
  }
  return iddp ? 1 : 0;
}

/*
 * Initialize the logreader error probe
 *
 * @param base_dir path where IDD is running
 * @param setting_test_mode receive true if test mode is set otherwise false
 */
void idd_logreader_error_probe_init(char* base_dir, bool setting_test_mode)
{
  idd_base_dir = base_dir;
  if (setting_test_mode)
  {
    logreader_error_period = LOGREADER_ERROR_TEST_PERIOD;
  }
  state_load();
}

/*
 * Set the number of logreader error probes
 *
 * @param nr error number
 */
void logreader_error_number_set(int nr)
{
  nr_errors = nr;
}

/*
 * Returns number of logreader error probes
 *
 * @return nr_errors
 */
int logreader_error_number_get()
{
  return nr_errors;
}

/*
 * Set the start of the current period
 *
 * @param timestamp from the logreader.state
 */
void logreader_error_timestamp_set(time_t timestamp)
{
  logreader_error_timestamp = timestamp;
}

/*
 * Returns the error probe timestamp
 *
 * @return logreader_error_timestamp
 */
time_t logreader_error_timestamp_get()
{
  return logreader_error_timestamp;
}

/*
 * Compute the current time
 *
 * @param timestamp_in input time stamp to calulate the current time
 *
 * @return current time stamp if success otherwise 0
 */
static time_t get_time(time_t* timestamp_in)
{
  if (timestamp_in && (*timestamp_in != 0))
  {
    return *timestamp_in;
  }
  time_t timestamp = time(NULL);
  if (timestamp == ((time_t)-1))
  {
    IDD_LOGE("Failure to compute the current time: %s", strerror(errno));
    return (time_t)0;
  }

  return timestamp;
}

/*
 * Check whether logreader error probe can be send
 *
 * @param flooding set to 1 if maximum allowed error has been reached otherwise 0
 *
 * @return 1 if flooded otherwise 0
 */
static int logreader_error_probe_is_flooded(int* flooding)
{
  *flooding = 0;
  if ((logreader_error_timestamp = get_time(&logreader_error_timestamp)) == (time_t)0)
  {
    return 1;
  }
  if (++nr_errors <= nr_errors_max)
  {
    if (nr_errors == nr_errors_max)
    {
      *flooding = 1;
    }
    return 0;
  }

  /* Too many errors, check the time period */
  time_t now;
  if ((now = get_time(NULL)) == (time_t)0)
  {
    return 1;
  }
  if ((now - logreader_error_timestamp) > logreader_error_period)
  {
    /* The old error period, starting at logreader_error_timestamp has expired.
     * This is the first error in a new error period (starting at logreader_error_timestamp) */
    nr_errors = 1;
    logreader_error_timestamp = now;
    return 0;
  }

  /* More than nr_errors_max error probes has been generated within the current time period.
   * No more error probes will be generated within current time period.
   * The flooding set to 1 here is therefore redundant but causes no harm. */
  *flooding = 1;
  return 1;
}

/*
 * Send the logreader error probe to idd daemon
 *
 * @param payload error message
 */
static void send_probe(char *payload)
{
  int flooding;
  int ret = -1;
  static int error_logging_done = 0;

  if (logreader_error_probe_is_flooded(&flooding))
  {
    if (!error_logging_done)
    {
      char buff[DURATION_BUF] = {'\0'};
      time_t period_end = logreader_error_timestamp + logreader_error_period;
      strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", localtime(&period_end));
      IDD_LOGE("IDD logreader error event will not be created until %s to prevent flooding.", buff);
      error_logging_done = 1;
    }
    return;
  }
  error_logging_done = 0;

  /* Connect to idd socket if not already connected */
  if (!idd_connect())
  {
    return;
  }
  Idd__Logreader__LogReaderError lre = IDD__LOGREADER__LOG_READER_ERROR__INIT;
  lre.log = payload;
  if (flooding)
  {
    lre.has_flooding = 1;
    lre.flooding = 1;
  }

  /* ok, we have the data, turn it into a protocol buffer */
  size_t size = idd__logreader__log_reader_error__get_packed_size(&lre);
  uint8_t *pSerialized = malloc(size);
  if (pSerialized)
  {
    size = idd__logreader__log_reader_error__pack(&lre, pSerialized);
    ret = idd_add_event(iddp, PROBE_LOGREADERERROR_PKG, PROBE_LOGREADERERROR_NAME,
                        (char *)pSerialized, size);
    if (ret < 0)
    {
      IDD_LOGE("Event dispatching failed, Returned error = %d", ret);
    }
    free(pSerialized);
    state_save();
  }
  else
  {
    IDD_LOGE("IDD logreader error event not created. Failed to allocate %zu bytes", size);
  }

  ret = idd_inc_counter(iddp, PROBE_LOGREADERERROR_PKG, PROBE_LOGREADERERROR_CNT_NAME, 1);
  if(ret < 0)
  {
    IDD_LOGE("Counter increament failed, Returned error = %d", ret);
  }
}

/*
 * Create a logreader error probe
 *
 * @param fmt format of the variable arguments received in .three dots.
 */
void idd_logreader_error_probe(const char *fmt, ...)
{
  va_list ap;
  char payload[LOG_BUF_SIZE] = {'\0'};

  va_start(ap, fmt);
  vsnprintf(payload, LOG_BUF_SIZE, fmt, ap);
  va_end(ap);
  send_probe(payload);
}

/*
 * Load the time stamp and number of logreader errors.
 */
void state_load()
{
  FILE* file_p = fopen(LOGREADER_STATE_FNAME, "r");
  if (file_p == NULL)
  {
    IDD_LOGE("Failed to open the logreader.state file: %s", strerror(errno));
    return;
  }
  else
  {
    int flag = 0;
    char buffer[FILE_BUF] = {'\0'};
    long timestamp = 0;

    while (fgets(buffer, sizeof(buffer), file_p) != NULL)
    {
      if (sscanf(buffer, LOGREADER_STATE_TAG_ERROR_NR "%d", &flag))
      {
        logreader_error_number_set(flag);
      }
      else if (sscanf(buffer, LOGREADER_STATE_TAG_ERROR_TIMESTAMP "%ld", &timestamp))
      {
        logreader_error_timestamp_set((time_t)timestamp);
      }
    }
    fclose(file_p);
  }
}

/*
 * Save the timestamp and number of logreader errors in logreader.state
 */
void state_save()
{
  FILE* fp = fopen(LOGREADER_STATE_FNAME, "wb");
  if (fp == NULL)
  {
    IDD_LOGE("Failed to open the logreader.state file: %s", strerror(errno));
    return;
  }
  else
  {
    fprintf(fp, LOGREADER_STATE_TAG_ERROR_NR "%1d\n", logreader_error_number_get());
    fprintf(fp, LOGREADER_STATE_TAG_ERROR_TIMESTAMP "%ld\n", (long)logreader_error_timestamp_get());
    fclose(fp);
  }
}
