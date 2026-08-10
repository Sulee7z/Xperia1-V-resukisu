/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief iddd settings
 *
 * @author Leif Mårtensson (leif.martensson@sonyericsson.com)
 */
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <fcntl.h>
#include "idd_config.h"
#include "config.h"
#include "util.h"
#include "settings.h"
#include "protos/idd/iddd.pb-c.h"
#include "configuration.h"
#define LOG_TAG "iddd-settings"
#include "logging.h"

/* The maximum size of each generated report file */
static size_t report_max_size;

/* The maximum size of the output directory, where the report files are placed */
static size_t output_dir_max_size;

/* Counters and static data that haven't been touched for these many seconds
 * will be expired.
 */
static size_t expire_counters_age;
static size_t expire_sdata_age;

/*
 * The default values of expire_counters_age and expire_sdata_age can be manipulated
 * by a switch to iddd
 */
static size_t default_expire_counters_age;
static size_t default_expire_sdata_age;

static wash_config_t wash_data;
static bool use_imei;
static bool use_android_id;
static bool use_xs_android_id;
static bool use_xs_guid;
static Iddd__UserConsent__Type required_user_consent;

static int get_disk_size(const char *path, uint64_t *size)
{
  if (!path || !size) return -EINVAL;

  struct statvfs sv;
  int ret = statvfs(path, &sv);
  if (ret < 0) {
    IDD_LOGE("Error in getting statvfs:(%d) %s\n",
              errno, strerror(errno));
    return ret;
  }

  *size = (uint64_t)sv.f_bsize * (uint64_t)sv.f_blocks;
  return ret;
}

/**
 * Fetches default output directory max size from platform.
 */
static int settings_get_output_dir_max_size_default()
{
  uint64_t total_disk_size;
  int ret = get_disk_size(IDD_DEFAULT_BASE_DIR, &total_disk_size);
  if (ret < 0) return OUTPUT_DIR_MAX_SIZE_DEFAULT;

  /* Required margin for 2 idd report size and 2 xd report size
   *  & idd max config size
   */
  int required_free_size = MAX_CONFIG_SIZE + (4 * REPORT_MAX_SIZE_DEFAULT);
  int max_output_dir_size = total_disk_size - required_free_size;
  if (max_output_dir_size < 0) return OUTPUT_DIR_MAX_SIZE_DEFAULT;
  return max_output_dir_size;
}

/**
 * Set default settings.
 *
 */
static void settings_reset_default()
{
  report_max_size = REPORT_MAX_SIZE_DEFAULT;
  output_dir_max_size = settings_get_output_dir_max_size_default();
  expire_counters_age = default_expire_counters_age;
  expire_sdata_age = default_expire_sdata_age;
  wash_data = WASH_CONFIG_UNDEFINED;
  use_imei = false;
  use_android_id = false;
  use_xs_android_id = false;
  use_xs_guid = false;
  required_user_consent = IDDD__USER_CONSENT__TYPE__NONE;
}

/**
 * Called from main() to set up internal iddd settings.
 *
 */
void settings_init()
{
  default_expire_counters_age = IDD_DEFAULT_EXPIRE_COUNTERS_AGE;
  default_expire_sdata_age = IDD_DEFAULT_EXPIRE_SDATA_AGE;
  settings_reset_default();
}

void settings_report_max_size_set(size_t new_report_max_size)
{
  report_max_size = new_report_max_size;
}

size_t settings_report_max_size_get()
{
  return report_max_size;
}

void settings_output_dir_max_size_set(size_t new_output_dir_max_size)
{
  output_dir_max_size = new_output_dir_max_size;
}

size_t settings_output_dir_max_size_get()
{
  return output_dir_max_size;
}

void settings_expire_counters_set(size_t new_time)
{
  expire_counters_age = new_time;
}

size_t settings_expire_counters_get(void)
{
  return expire_counters_age;
}

void settings_expire_sdata_set(size_t new_time)
{
  expire_sdata_age = new_time;
}

size_t settings_expire_sdata_get(void)
{
  return expire_sdata_age;
}

void settings_default_expire_counters_set(size_t time)
{
  default_expire_counters_age = time;
}

void settings_default_expire_sdata_set(size_t time)
{
  default_expire_sdata_age = time;
}

void settings_wash_data_set(wash_config_t new_wash_data)
{
  wash_data = new_wash_data;
}

wash_config_t settings_wash_data_get(void)
{
  return wash_data;
}

void settings_use_imei_set(bool new_use_imei)
{
  use_imei = new_use_imei;
}

bool settings_use_imei_get(void)
{
  return use_imei;
}

void settings_use_android_id_set(bool new_use_android_id)
{
  use_android_id = new_use_android_id;
}

bool settings_use_android_id_get(void)
{
  return use_android_id;
}

void settings_use_xs_android_id_set(bool new_use_xs_android_id)
{
  use_xs_android_id = new_use_xs_android_id;
}

bool settings_use_xs_android_id_get(void)
{
  return use_xs_android_id;
}

void settings_use_xs_guid_set(bool new_use_xs_guid)
{
  use_xs_guid = new_use_xs_guid;
}

bool settings_use_xs_guid_get(void)
{
  return use_xs_guid;
}

void settings_required_user_consent_set(Iddd__UserConsent__Type new_required_user_consent)
{
  required_user_consent = new_required_user_consent;
}

Iddd__UserConsent__Type settings_required_user_consent_get(void)
{
  return required_user_consent;
}

/**
 * Sanity check max sizes.
 *
 */
static void settings_validate()
{
  if (report_max_size > output_dir_max_size)
  {
    IDD_LOGE(
        "Bad settings found in configuration file, report_max_size (%zu) > output_dir_max_size (%zu)."
          " Default values (%d)(%d) will be used", report_max_size,
        output_dir_max_size, REPORT_MAX_SIZE_DEFAULT,
        settings_get_output_dir_max_size_default());

    report_max_size = REPORT_MAX_SIZE_DEFAULT;
    output_dir_max_size = settings_get_output_dir_max_size_default();
  }
}

/**
 * Handles the study parameter in the configuration request.
 */
static void settings_handle_study(Iddd__Settings* settings_p)
{
  int new_device_id = 0;
  char *cur_study = group_id_starting_with_exist("study:");
  if (cur_study)
  {
    if (!settings_p->study)
    {
      IDD_LOGD("Leaving study <%s>.", cur_study);
      group_id_delete_handle(cur_study, 0);
      new_device_id = 1;
      goto out;
    }
    char new_study[IDD_MAX_GROUP_ID_SIZE] = {0};
    snprintf(new_study, IDD_MAX_GROUP_ID_SIZE, "study:%s", settings_p->study);
    if (strncmp(new_study, cur_study, IDD_MAX_GROUP_ID_SIZE) == 0)
    {
      IDD_LOGD("Still in study <%s>.", cur_study);
      goto out;
    }
    IDD_LOGD("Leaving study <%s>, entering study <%s>", cur_study, new_study);
    group_id_replace_handle(new_study, cur_study, 0);
    new_device_id = 1;
    goto out;
  }

  if (settings_p->study)
  {
    char new_study[IDD_MAX_GROUP_ID_SIZE] = {0};
    snprintf(new_study, IDD_MAX_GROUP_ID_SIZE, "study:%s", settings_p->study);
    IDD_LOGD("Entering study <%s>", new_study);
    group_id_set_handle(new_study, 0);
    new_device_id = 1;
  }

out:
  if (new_device_id)
  {
    generate_device_id(1);
  }
}

/**
 * Unpack the given buffer assuming it is a Settings protobuf (defined
 * in iddd.proto). Set various global variables.
 *
 * @param buf_p Settings protobuf
 * @param buf_size size of 'buf_p'
 */
static void settings_load_configuration_buffer(const unsigned char* buf_p,
    size_t buf_size)
{
  Iddd__Settings* settings_p = iddd__settings__unpack(NULL, buf_size, buf_p);
  if (settings_p != NULL)
  {
    if (settings_p->has_permanently_disable &&
        settings_p->permanently_disable)
    {
      // Activate the kill switch.
      IDD_LOGI("Kill switch enabled");
      int fd = open(KILL_SWITCH_FNAME, O_CREAT | O_TRUNC, OUTPUT_DIR_MODE);
      if (fd != -1)
      {
        close(fd);
      }
      // Schedule a termination of IDDD
      set_should_exit();
    }

    if (settings_p->has_max_output_dir_size)
    {
      settings_output_dir_max_size_set((size_t) settings_p->max_output_dir_size);
      IDD_LOGD("Loading max_dir_size (%u) from configuration file.",
          settings_p->max_output_dir_size);
    }

    if (settings_p->has_max_report_size)
    {
      settings_report_max_size_set((size_t) settings_p->max_report_size);
      IDD_LOGD("Loading max_report_size (%u) from configuration file.",
          settings_p->max_report_size);
    }

    if (settings_p->has_expire_counters_age)
    {
      settings_expire_counters_set((size_t) settings_p->expire_counters_age);
      IDD_LOGD("Loading expire_counters_age (%u) from configuration file.",
          settings_p->expire_counters_age);
    }

    if (settings_p->has_expire_sdata_age)
    {
      settings_expire_sdata_set((size_t) settings_p->expire_sdata_age);
      IDD_LOGD("Loading expire_sdata_age (%u) from configuration file.",
          settings_p->expire_sdata_age);
    }

    if (settings_p->has_wash_data)
    {
      settings_wash_data_set(settings_p->wash_data ? WASH_CONFIG_TRUE : WASH_CONFIG_FALSE);
      IDD_LOGD("Loading wash_data (%s) from configuration file.",
          settings_p->wash_data ? "true" : "false");
    }

    if (settings_p->has_use_imei)
    {
      settings_use_imei_set(settings_p->use_imei);
      IDD_LOGD("Loading use_imei (%d) from configuration file.",
          (int)settings_p->use_imei);
    }

    if (settings_p->has_use_android_id)
    {
      settings_use_android_id_set(settings_p->use_android_id);
      IDD_LOGD("Loading use_android_id (%d) from configuration file.",
          (int)settings_p->use_android_id);
    }

    if (settings_p->has_use_xs_android_id)
    {
      settings_use_xs_android_id_set(settings_p->use_xs_android_id);
      IDD_LOGD("Loading use_xs_android_id (%d) from configuration file.",
          (int)settings_p->use_xs_android_id);
    }

    if (settings_p->has_use_xs_guid)
    {
      settings_use_xs_guid_set(settings_p->use_xs_guid);
      IDD_LOGD("Loading use_xs_guid (%d) from configuration file.",
          (int)settings_p->use_xs_guid);
    }

    if (settings_p->required_user_consent != NULL)
    {
      settings_required_user_consent_set(settings_p->required_user_consent->type);
      IDD_LOGD("Loading required_user_consent (%d) from configuration file.",
          settings_p->required_user_consent->type);
    }

    settings_handle_study(settings_p);

    iddd__settings__free_unpacked(settings_p, NULL);
  }
}

/**
 * Parses new iddd settings from configuration file, if one is present.
 *
 */
void settings_load_configuration(void)
{

  // Parameters not included in the new configuration shall be reset to default values.
  settings_reset_default();

  FILE* fp = config_open_file(SETTINGS_PROBE_PACKAGE, SETTINGS_PROBE_NAME);
  if (fp != NULL)
  {
    size_t file_size;
    unsigned char* buf_p = read_entire_file(fp, &file_size);
    fclose(fp);
    if (buf_p != NULL)
    {
      settings_load_configuration_buffer(buf_p, file_size);
      settings_validate();
      free(buf_p);
    }
  }
}
