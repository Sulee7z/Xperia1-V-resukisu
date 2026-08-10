/*
 * Copyright 2018 Sony Corporation
 */
#include "app_event_packages.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "filter.h"
#include "logging.h"
#include "plugins.h"

#include "idd_config.h"

// Structure - Linked list of packages
struct CELL {
  struct CELL *next;
  char package[MAX_APP_EVENT_PACKAGE_NAME_SIZE + 1];
  char class[MAX_APP_EVENT_ENABLED_INTENT_RECEIVER_NAME_SIZE + 1];
  int status;
};

static struct CELL header;

/**
 * Create the distinct packages registered list structure.
 *
 * return 0 on failure and 1 on success.
 */
static int app_event_packages_add_head(const char *package_p, const char *class_p, int status)
{
  int ret = 0;
  struct CELL *p, *q, *new = NULL;
  p = header.next;
  q = &header;
  new = calloc(1, sizeof(struct CELL));
  if (new != NULL)
  {
    new->next = p;
    strncpy(new->package, package_p, MAX_APP_EVENT_PACKAGE_NAME_SIZE);
    strncpy(new->class, class_p, MAX_APP_EVENT_ENABLED_INTENT_RECEIVER_NAME_SIZE);
    new->status = status;
    q->next = new;
    ret = 1;
  }
  return ret;
}

/**
 *
 */
static struct CELL *app_event_packages_search(const char *package_p, const char *class_p)
{
  struct CELL *p, *q;
  p = header.next;
  q = &header;
  while (p != NULL)
  {
    if (!strncmp(p->package, package_p, strlen(package_p)))
    {
      if (!strncmp(p->class, class_p, strlen(class_p)))
      {
        return p;
      }
    }
    q = p;
    p = p->next;
  }
  return NULL;
}

/**
 * Load the registered packages info line by line from REGISTERED_APP_PACKAGES_FNAME file.
 */
static void app_event_packages_load(void)
{
  FILE* fp = fopen(REGISTERED_APP_PACKAGES_FNAME, "rb");
  if (fp != NULL)
  {
    char buf[MAX_APP_EVENT_PACKAGE_LIST_LINE_LENGTH + 1] = {'\0'};
    char package_p[MAX_APP_EVENT_PACKAGE_NAME_SIZE + 1] = {'\0'};
    char class_p[MAX_APP_EVENT_ENABLED_INTENT_RECEIVER_NAME_SIZE + 1] = {'\0'};
    int status = 0;
    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
      sscanf(buf, "%s %s %d", package_p, class_p, &status);
      if (!app_event_packages_add_head(package_p, class_p, status))
      {
        IDD_LOGE("Out of memory when adding package to list");
      }
    }
  }
}

/**
 * Save the latest packages registered list data into the REGISTERED_APP_PACKAGES_FNAME file.
 *
 * return 0 on failure and 1 on success.
 */
static int app_event_packages_save(void)
{
  int ret = 0;

  FILE* tmpfile_fp = fopen(FLUSHED_PKG_DATA_FNAME, "wb");
  if (tmpfile_fp != NULL)
  {
    struct CELL *p, *q;
    p = header.next;
    q = &header;
    while (p != NULL)
    {
      ret = fprintf(tmpfile_fp, "%s %s %d\n", p->package, p->class, p->status);
      if (ret < 0)
      {
        break;
      }
      q = p;
      p = p->next;
    }
    fclose(tmpfile_fp);
  }

  if (ret < 0)
  {
    IDD_LOGE("%s: Failed to save tmp file, err=%d(%s)", __func__, errno, strerror(errno));
    return 0;
  }
  else
  {
    /* Now we have latest packages info in FLUSHED_PKG_DATA_FNAME
     * so renaming to REGISTERED_APP_PACKAGES_FNAME.
     */
    if (rename(FLUSHED_PKG_DATA_FNAME, REGISTERED_APP_PACKAGES_FNAME) == -1)
    {
      IDD_LOGE("%s: Failed to rename output file, err=%d(%s)", __func__, errno, strerror(errno));
      return 0;
    }
    else
    {
      /* All done. We can remove flushed_data_1 to save space. */
      unlink(FLUSHED_PKG_DATA_FNAME);
    }
    return 1;
  }
}

/**
 * Freeing existing registered packages list..
 */
static void app_event_packages_destroy(void)
{
  struct CELL *p, *q;
  p = header.next;
  while (p != NULL) {
    q = p;
    p = q->next;
    free(q);
  }
}

/**
 * Initializing the linked list structure by reading REGISTERED_APP_PACKAGES_FNAME file.
 * If REGISTERED_APP_PACKAGES_FNAME file is not present then linked list will be NULL.
 */
void app_event_packages_init(void)
{
  app_event_packages_load();
}

/**
 * Destroy the global list CELL.
 */
void app_event_packages_shutdown(void)
{
  app_event_packages_destroy();
}
/**
 * Request sent from apps to register their package to use app event.
 *
 * @param package_p the package name of the app which wants to submit app event.
 * @param class_p the class name of the receiver to receive broadcast to notify that
 *                respective app event get collectable.
 * @param status the status is the value whether respective package name is enabled to
 *               start collecting app event data.
 *               Where "0" - Disabled, "1" - Enabled.
 *
 * return 0 on failure and 1 on success.
 */
int app_event_packages_register(const char *package_p, const char *class_p, int status)
{
  if (package_p != NULL && class_p != NULL)
  {
    if (!app_event_packages_search(package_p, class_p))
    {
      if (!app_event_packages_add_head(package_p, class_p, status))
      {
        IDD_LOGE("%s: Failed to add an app event package", __func__);
        return 0;
      }
      if (!app_event_packages_save())
      {
        IDD_LOGE("%s: Failed to save app event packages", __func__);
        return 0;
      }
    }
    IDD_LOGD("%s: Succeeded", __func__);
    return 1;
  }
  else
  {
    IDD_LOGE("%s: Arguments are invalid", __func__);
    return 0;
  }
}

/**
 * Fetches all the registered packages from REGISTERED_APP_PACKAGES_FNAME file.
 * Check the status of the respective package and if status is changed from
 * Disabled -> Enabled OR Enabled -> Disabled update the REGISTERED_APP_PACKAGES_FNAME file
 * accordingly.
 *
 * Where the status of packages are changed from Disabled -> Enabled those are added to
 * "updated" array.
 */
void app_event_packages_update(char* updated, size_t size)
{
  if (updated != NULL)
  {
    int count = 0;
    struct CELL *p, *q;
    p = header.next;
    q = &header;
    while (p != NULL)
    {
      int active = filter_is_probe_active(g_filter + EVENT,
          PACKAGE_PROBE_ANDROID_IDD_VIRTUAL_APP_EVENT, p->package, CHECK_IDD_ENABLED);
      if (active && !p->status)
      {
        if (count != 0)
        {
          count += snprintf(&updated[count], size - count, "\n");
        }
        count += snprintf(&updated[count], size - count, "%s %s", p->package, p->class);
        p->status = 1;
      }
      else if (!active && p->status)
      {
        p->status = 0;
      }
      q = p;
      p = p->next;
    }
    if (!app_event_packages_save()) {
      IDD_LOGE("%s: Failed to save app event packages", __func__);
    }
  }
}
