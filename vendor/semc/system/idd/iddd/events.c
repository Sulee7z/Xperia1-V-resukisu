/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief Event module, for event reporting
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "events.h"
#include "config.h"
#include "serializer.h"
#include "errno.h"
#include "util.h"
#include "compactor.h"
#include "util.h"
#include "filter.h"
#include "plugins.h"
#include "settings.h"

#define LOG_TAG "iddd-events"
#include "logging.h"
#include "protos/idd/app_usage_data.pb-c.h"

static int events_fd = -1;

/**
 * Opens the event file 'events_fd', if it hasn't been opened already.
 *
 */
void open_file(void)
{
  if (events_fd == -1)
  {
    events_fd = open(PARTIAL_EVENTS_FNAME,
                     O_CREAT | O_WRONLY | O_APPEND,
                     PRIVATE_DIR_MODE);
    if (events_fd == -1)
    {
      IDD_LOGE("Failed to open partial event file: %s", strerror(errno));
    }
    else if (lseek(events_fd, 0, SEEK_END) == 0)
    {
      write_report_header(events_fd, NO_FORCE_WRITE);
    }
  }
}

/**
 * Remove partial events file.
 *
 */
void remove_events(void)
{
  struct stat lstat_result;

  if (events_fd == -1)
  {
    if (lstat(PARTIAL_EVENTS_FNAME, &lstat_result) != 0)
    {
      IDD_LOGI("%s: No partial events file: %s", __func__, strerror(errno));
    }
    else if (unlink(PARTIAL_EVENTS_FNAME) == 0)
    {
      IDD_LOGI("%s: Deleting partial events file", __func__);
    }
  }
  else
  {
    if (close(events_fd) != 0)
    {
      IDD_LOGE("%s: Failed to close partial events file: %s", __func__, strerror(errno));
    }
    events_fd = -1;
    if (unlink(PARTIAL_EVENTS_FNAME) == 0)
    {
      IDD_LOGI("%s: Deleting partial events file", __func__);
    }
  }
}

/**
 * Checks if the file is too large, and has to be flushed.
 * This call will then trigger the report generation.
 *
 */
static void check_file_size(void)
{
  if (events_fd != -1)
  {
    off_t position = lseek(events_fd, 0, SEEK_END);
    size_t max_size = is_valid_header() ?
                      settings_report_max_size_get() :
                      MIN(settings_report_max_size_get() * 5, settings_output_dir_max_size_get() / 2);

    if (position >= (off_t)max_size)
    {
      (void)generate_report();
    }
  }
}

/**
 * Flushes all events, which almost only moves it into the new name.
 *
 * @return 0 if the file is successfully renamed to FLUSHED_DATA_FNAME
 */
int flush_events(void)
{
  /* If we have just closed the file - probably because we have just flushed. Force a new file
   * so that we have a proper header.
   */
  open_file();

  /* close it - so that we can rename it. */
  if (events_fd != -1)
  {
    close(events_fd);
    events_fd = -1;
  }

  return rename(PARTIAL_EVENTS_FNAME, FLUSHED_DATA_FNAME);
}

/**
 * Writes one event to the global file descriptor 'events_fd'.
 *
 * @param package_p name of the package
 * @param name_p name of the event
 * @param payload_p incoming payload from client
 * @param payload_size size of the payload
 */
static void write_event(const char* package_p, const char* name_p, const char* payload_p, size_t payload_size)
{
  /* The header buffer must be large enough to hold the event's header protocol buffer.
   * And the largest dynamic parts are the package and name size - the 100 is just a "very large
   * approximation of the rest" (which is rather around 20 bytes)
   */
  static char headerbuf[IDD_MAX_PROBE_ID_SIZE + 100] = {0};
  int64_t timestamp = get_timestamp();
  size_t header_size = ser_event_header(headerbuf, sizeof(headerbuf), package_p, name_p, timestamp, payload_size);

  if (header_size > 0)
  {
    int try = 0;
    off_t position = lseek(events_fd, 0, SEEK_END);
    if (position == -1)
    {
      IDD_LOGE("Failed to set end of file offset(lseek), errno %d <%s>", errno, strerror(errno));
      return;
    }
    while (try++ < 2)
    {
      errno = 0;
      int written = write_all(events_fd, headerbuf, header_size);
      if (written >= 0 && payload_size > 0)
      {
        written = write_all(events_fd, payload_p, payload_size);
      }
      if (written < 0)
      {
        int errno_write = errno;
        errno = 0;
        if (errno_write == ENOSPC)
        {
          if (ftruncate(events_fd, position) == 0) /* Reset end of file */
          {
            if (lseek(events_fd, position, SEEK_SET) < 0) /* Reset file pointer */
            {
              IDD_LOGE("Failed to set file position to %ld after file truncate. errno %d <%s>",
                       position, errno, strerror(errno));
            }
            if (try == 1)
            {
              IDD_LOGW("Failed to write event %s::%s with %zu bytes payload (ENOSPC). "
                       "Deleting old reports",
                       package_p, name_p, payload_size);
              (void)prune_data(header_size + payload_size);
              continue; // Try to store the event again after deleting files.
            }
          }
          else
          {
            IDD_LOGE("ftruncate failed, errno %d <%s>", errno, strerror(errno));
          }
        }
        IDD_LOGE("Failed to write event %s::%s with %zu bytes header and %zu bytes payload."
                  " Error: %s", package_p, name_p, header_size, payload_size, strerror(errno_write));
      }
      break; /* Success or failure doesn't matter. Don't write the event again */
    }
  }
  else
  {
    IDD_LOGE("Failed to serialize header %s::%s", package_p, name_p);
  }
}

/**
 * Handle incoming event RPCs. Called from handle_data() in iddd.c in
 * respons to an incoming 'EV' command. The event is only saved if it
 * is in a filter.
 *
 * @param filter The probe filter to use
 * @param package_p name of the package
 * @param name_p name of the event
 * @param payload_p incoming payload from client
 * @param payload_size size of the payload
 */
void handle_event(idd_filter_t* filter, const char* package_p, const char* name_p,
                  const char* payload_p, size_t payload_size)
{
  if (!filter_is_probe_active(filter + EVENT, package_p, name_p, CHECK_IDD_ENABLED))
  {
    IDD_LOGD("Event %s::%s was discarded since it was not active.", package_p, name_p);
  }
  else
  {
    open_file();

    if (events_fd != -1)
    {
      write_event(package_p, name_p, payload_p, payload_size);
    }

    check_file_size();
  }
}

/**
 * Handle incoming app event RPCs. Called from handle_app_event() in iddd.c in
 * respons to an incoming 'AV' or 'AZ' command. The app data is only saved if it
 * is in a filter.
 *
 * @param filter The probe filter to use
 * @param app_package_p name of the app package
 * @param app_version_name_p name of the app version
 * @param app_version_code code of the app version
 * @param timestamp Time stamp when the event occurred
 * @param payload_type Format specifier of the payload
 * @param payload_p incoming payload from client
 * @param payload_size size of the payload
 */
void handle_app_event(idd_filter_t* filter, const char* app_package_p,
                      const char* app_version_name_p, int app_version_code, const char* timestamp_p,
                      int payload_type, const char* payload_p, size_t payload_size)
{
  int is_app_event_active = filter_is_probe_active(
      filter + EVENT, PACKAGE_PROBE_ANDROID_IDD_VIRTUAL_APP_EVENT, app_package_p,
      CHECK_IDD_ENABLED);
  int plugins_is_app_event_active = plugins_filter_is_probe_active(
      PACKAGE_PROBE_ANDROID_IDD_VIRTUAL_APP_EVENT, app_package_p);
  if (!is_app_event_active && !plugins_is_app_event_active)
  {
    IDD_LOGD("AppEvent %s was discarded since it was not active.", app_package_p);
  }
  else
  {
    char* app_event_pkg = (char *)idd__app_event__descriptor.package_name;
    char* app_event_name = (char *)idd__app_event__descriptor.short_name;
    Idd__AppEvent app_event = IDD__APP_EVENT__INIT;
    app_event.app_package = (char *)app_package_p;
    app_event.app_version_name = (char *)app_version_name_p;
    app_event.app_version_code = app_version_code;
    app_event.has_app_version_code = true;
    app_event.timestamp = (char *)timestamp_p;
    app_event.payload_type = payload_type;
    app_event.has_payload_type = true;
    app_event.payload = (char *)payload_p;

    size_t size = idd__app_event__get_packed_size(&app_event);
    uint8_t* serialized = malloc(size);
    if (serialized)
    {
      size = idd__app_event__pack(&app_event, serialized);

      open_file();

      if (events_fd != -1)
      {
        write_event(app_event_pkg, app_event_name, (const char*)serialized, size);
      }

      int plugin_id = ALL_PLUGINS;
      plugins_handle_event(plugin_id, PACKAGE_PROBE_ANDROID_IDD_VIRTUAL_APP_EVENT, app_package_p,
          size, (const char*)serialized);

      free(serialized);
      check_file_size();
    }
    else
    {
      ALOGE("No AppEvent created. Failed to allocate %zu bytes", size);
    }
  }
}
/**
 * Check if there is new events incoming since last report is generated.
 *
 */
int has_new_event(void)
{
  int has_new_event = 0;
  if (access(PARTIAL_EVENTS_FNAME, F_OK) == 0)
  {
    has_new_event = 1;
  }
  return has_new_event;
}

/**
 * Check if the event is generated by AppEvent probe
 */
int is_app_event(const char* package_p, const char* name_p)
{
  if (strncmp(package_p, PACKAGE_PROBE_ANDROID_IDD, sizeof(PACKAGE_PROBE_ANDROID_IDD)) == 0 &&
      strncmp(name_p, PROBE_APP_EVENT, sizeof(PROBE_APP_EVENT)) == 0)
  {
    return 1;
  }
  return 0;
}
