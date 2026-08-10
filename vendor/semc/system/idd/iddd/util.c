/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief Misc utilities
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>
#include <dirent.h>
#ifndef PC_BUILD
# include <cutils/properties.h>
#endif
#include <inttypes.h>
#include <miscta.h>
#include <miscta_units.h>
#include "protos/common.pb-c.h"
#include "compactor.h"
#include "config.h"
#include "configuration.h"
#include "crypto.h"
#include "events.h"
#include "iddd_miscta.h"
#include "idd_private_api.h"
#include "protos/idd/iddd.pb-c.h"
#include "plugins.h"
#include "secure_mode.h"
#include "serializer.h"
#include "settings.h"
#include "util.h"

#define LOG_TAG "iddd-util"
#include "logging.h"

#ifndef PROPERTY_VALUE_MAX
#define PROPERTY_VALUE_MAX 92
#endif
#define HEADER_STRING_MAX PROPERTY_VALUE_MAX

#define HEADER_STRING_UNKNOWN "unknown"
#define IMEI_UNKNOWN ""

#define GROUP_ID_NOT_EXIST MAX_GROUP_ID_NUMBER

#define ANDROID_ID_UNKNOWN ""
#define XS_ANDROID_ID_UNKNOWN ""
#define XS_GUID_UNKNOWN ""

/*
  Protobuf tag is calculated as ((field_id << 3) | field_type)
  Both header and events has field_type length_delimited (2)
*/
#define REPORT_HEADER_TAG (char)((1 << 3) | 2)
#define REPORT_EVENT_TAG (char)((2 << 3) | 2)

/* The minimum accepted length for an IMEI.
 * Smaller imei's will be dismissed.
 * IMEI is 15 digits. MEID or Windy's is 14 characters.
 */
#define IDD_MIN_DEVICE_IMEI_LEN 14

#define TA_S1_BOOT_COUNTER (2053) //!< Boot counter
#define TA_MSN 4900

static int should_exit_flag = 0;

static const int32_t TIMEZONE_UNKNOWN = -1;
static int32_t g_timezone = TIMEZONE_UNKNOWN;
static Idd__Header header = IDD__HEADER__INIT;

static int verbose = 0;

static const char* header_buf_p = NULL;
static const char* rca_header_buf_p = NULL;
static size_t header_buf_size = 0;
static size_t rca_header_buf_size = 0;

/* A monotonically increasing counter, in test mode
 * for reproducability in testing.
 */
static struct timeval test_mode_tv = {1262325600, 0};  // 2010-01-01 07:00:00 UTC

static time_t last_save_time = 0;                      // The time of the last save to file of counters and staticdata

static int64_t flushtime_last = 0;                     // The timestamp when the last flush to output file was done

// The checksum of the currently used downloaded configuration.
static char config_checksum[CONFIG_CHECKSUM_LEN] = {0};

//Device id for report header
static char g_device_id[IDD_MAX_DEVICE_ID_SIZE + 1] = HEADER_STRING_UNKNOWN;

//Device id for rca report header
static char g_rca_device_id[IDD_MAX_DEVICE_ID_SIZE + 1] = HEADER_STRING_UNKNOWN;

//Device id for configuration request
static char g_device_id_cr[IDD_MAX_DEVICE_ID_SIZE + 1] = {0};

/* The flag which should accept the data from idd client or not.
 * Default is that shouldn't accept if user variant and that should accept if debug variant.
*/
static int data_collection_enabled = 0;

static uint64_t g_session_id = 0;

static char g_device_imei[PROPERTY_VALUE_MAX] = IMEI_UNKNOWN;

static char g_android_id[PROPERTY_VALUE_MAX] = ANDROID_ID_UNKNOWN;

static char g_xs_android_id[PROPERTY_VALUE_MAX] = XS_ANDROID_ID_UNKNOWN;

static char g_xs_guid[PROPERTY_VALUE_MAX] = XS_GUID_UNKNOWN;

static uint32_t g_user_consent = IDD__USER_CONSENT__NONE;

static const size_t FILE_BUFF_SIZE             = 6;
static const size_t PROTO_BUF_TAG_SIZE         = 1;

typedef enum _Idd_Report_Status {
  REPORT_STATUS_OK      = 0,
  REPORT_HEADER_CORRUPT = 1,
  REPORT_EVENT_CORRUPT  = 2,
} Idd_Report_Status;

static void base16_encode(char* dest_p, const char* src_p, size_t src_length);
static void base16_decode(char* dest_p, const char* src_p, size_t src_length);
static int read_miscta_string(uint32_t id, void *buf, uint32_t *size);
static void serial_number_handle(Idd__Header*);

static const uint64_t SESSION_ID_UNKNOWN = 0;

/*
 * Allocate memory of string fields in header and init
 * if they are not allocated yet.
 */
static void init_header(Idd__Header* header_p)
{
  /*
   * Use header_p->product_id to identify whether header_p is initiated or not.
  */
  if (header_p->product_id == NULL)
  {
    idd__header__init(header_p);

    header_p->product_id = malloc(HEADER_STRING_MAX + 1);
    header_p->software_version = malloc(HEADER_STRING_MAX + 1);
    header_p->software_variant = malloc(HEADER_STRING_MAX + 1);
    header_p->device_id = malloc(IDD_MAX_DEVICE_ID_SIZE + 1);
    header_p->build_type = malloc(HEADER_STRING_MAX + 1);
    header_p->config_checksum = calloc(1, HEADER_STRING_MAX + 1);
    header_p->report_uuid = calloc(1, HEADER_STRING_MAX + 1);
    header_p->cdf_id = malloc(HEADER_STRING_MAX + 1);
    header_p->serial_number = malloc(IDD_MAX_SERIAL_NUMBER_SIZE + 1);
    header_p->device_imei = malloc(IDD_MAX_DEVICE_IMEI_SIZE + 1);
    header_p->android_id = malloc(IDD_MAX_ANDROID_ID_SIZE + 1);
    header_p->xs_android_id = malloc(IDD_MAX_XS_ANDROID_ID_SIZE + 1);
    header_p->xs_guid = malloc(IDD_MAX_XS_GUID_SIZE + 1);
    header_p->internal_product_name = malloc(HEADER_STRING_MAX + 1);

    header_p->market_model_name = malloc(HEADER_STRING_MAX + 1);
    header_p->market_product_name = malloc(HEADER_STRING_MAX + 1);
    header_p->os_version = malloc(HEADER_STRING_MAX + 1);
    header_p->kernel_version = malloc(HEADER_STRING_MAX + 1);

    header_p->sony_product_code = malloc(HEADER_STRING_MAX + 1);
    header_p->customer_id = malloc(HEADER_STRING_MAX + 1);

    header_p->subsystem_version = malloc(HEADER_STRING_MAX + 1);

    header_p->has_family = true;
    header_p->family = IDD__FAMILY__ANDROID;
    header_p->has_timezone = true;
    header_p->timezone = TIMEZONE_UNKNOWN;
    header_p->has_session_id = true;
    header_p->session_id = SESSION_ID_UNKNOWN;
    header_p->has_user_consent = true;
    header_p->user_consent = IDD__USER_CONSENT__NONE;

    if (header_p->device_imei == NULL)
    {
      die("OOM: header_p->device_imei");
    }

    /*
     * Init these strings with "unknown".
    */
    strncpy(header_p->product_id, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX);
    strncpy(header_p->software_version, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX);
    strncpy(header_p->software_variant, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX);
    strncpy(header_p->device_id, HEADER_STRING_UNKNOWN, IDD_MAX_DEVICE_ID_SIZE);
    strncpy(header_p->build_type, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX);
    strncpy(header_p->cdf_id, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX);
    strncpy(header_p->serial_number, HEADER_STRING_UNKNOWN, IDD_MAX_SERIAL_NUMBER_SIZE);
    strncpy(header_p->device_imei, IMEI_UNKNOWN, IDD_MAX_DEVICE_IMEI_SIZE);
    strncpy(header_p->android_id, ANDROID_ID_UNKNOWN, IDD_MAX_ANDROID_ID_SIZE);
    strncpy(header_p->xs_android_id, XS_ANDROID_ID_UNKNOWN, IDD_MAX_XS_ANDROID_ID_SIZE);
    strncpy(header_p->xs_guid, XS_GUID_UNKNOWN, IDD_MAX_XS_GUID_SIZE);
    strncpy(header_p->internal_product_name, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX);

    strncpy(header_p->market_model_name, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX);
    strncpy(header_p->market_product_name, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX);
    strncpy(header_p->os_version, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX);
    strncpy(header_p->kernel_version, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX);

    strncpy(header_p->sony_product_code, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX);
    strncpy(header_p->customer_id, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX);

    strncpy(header_p->subsystem_version, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX);

    header_p->has_secure_mode = true;
    header_p->secure_mode = get_secure_mode();
  }
}

/*
 * Free the memory of group id fields in header.
 */
static void free_header_group_id(Idd__Header* header_p)
{
  if (header_p->group_id != NULL)
  {
    size_t i;
    for (i = 0; i < header_p->n_group_id; i++)
    {
      free(header_p->group_id[i]);
    }
    free(header_p->group_id);
    header_p->group_id = NULL;
    header_p->n_group_id = 0;
  }
}

/*
 * Free the memory of string fields in the header.
 */
static void free_header(Idd__Header* header_p)
{
  free(header_p->product_id);
  free(header_p->software_version);
  free(header_p->software_variant);
  free(header_p->device_id);
  free(header_p->build_type);
  free(header_p->config_checksum);
  free(header_p->report_uuid);
  free(header_p->cdf_id);
  free(header_p->serial_number);
  free(header_p->device_imei);
  free(header_p->android_id);
  free(header_p->xs_android_id);
  free(header_p->xs_guid);
  free(header_p->internal_product_name);
  free(header_p->market_model_name);
  free(header_p->market_product_name);
  free(header_p->os_version);
  free(header_p->kernel_version);
  free(header_p->sony_product_code);
  free(header_p->customer_id);
  free(header_p->subsystem_version);
  free_header_group_id(header_p);
  /*
   * Set header_p->product_id to NULL, then we can
   * use header_p->product_id to identify whether header_p
   * is initiated or not when init header_p.
  */
  header_p->product_id = NULL;
}

/**
 * Check whether the group id is already exist.
 *
 * Returns on the index number of group id in the list if already exist, otherwise GROUP_ID_NOT_EXIST.
 */
static size_t is_group_id_exist(Idd__Header* header_p, const char *new_group_id)
{
  size_t i;
  if (new_group_id != NULL)
  {
    for (i = 0; i < header_p->n_group_id; i++)
    {
      if (strncmp(header_p->group_id[i], new_group_id, IDD_MAX_GROUP_ID_SIZE) == 0)
      {
        IDD_LOGD("Group id is already exist.");
        return i;
      }
    }
  }
  return GROUP_ID_NOT_EXIST;
}

/**
 *
 */
char *group_id_starting_with_exist(const char* group_id_start_with)
{
  if (group_id_start_with)
  {
    Idd__Header* header_p = &header;
    size_t i;
    int chk_len = strnlen(group_id_start_with, IDD_MAX_GROUP_ID_SIZE);
    for (i = 0; i < header_p->n_group_id; i++)
    {
      if (strncmp(header_p->group_id[i], group_id_start_with, chk_len) == 0)
      {
        IDD_LOGD("Group <%s> starting with <%s> found!",
                 header_p->group_id[i], group_id_start_with);
        return header_p->group_id[i];
      }
    }
  }
  return NULL;
}

/**
 * Add a group id into header.
 *
 * Returns 1 on success add group id, otherwise 0.
 */
static int add_group_id(Idd__Header* header_p, const char *new_group_id)
{
  int success_add = 0;
  char *group_data = strndup(new_group_id, IDD_MAX_GROUP_ID_SIZE + 1);
  if (group_data != NULL)
  {
    if (header_p->n_group_id < MAX_GROUP_ID_NUMBER)
    {
      size_t new_n_group_id = header_p->n_group_id + 1;
      // max group ids not reached, grow the list
      char** new_group_id = (char**)realloc(header_p->group_id,
                                            new_n_group_id * sizeof(char *));
      if (new_group_id != NULL)
      {
        header_p->group_id = new_group_id;
        header_p->n_group_id = new_n_group_id;
        header_p->group_id[header_p->n_group_id-1] = group_data;
        success_add = 1;
      }
      else
      {
        IDD_LOGW("Failed to grow group id list to %zu elements", new_n_group_id);
      }
    }
    else
    {
      char** group_table = (char**)malloc(header_p->n_group_id * sizeof(char *));
      if (group_table != NULL)
      {
        size_t i;
        for (i = 0; i < header_p->n_group_id - 1; i++)
        {
          // max group ids reached, remove the oldest group id
          group_table[i] = header_p->group_id[i+1];
        }
        // Save the new entry last in table
        group_table[i] = group_data;
        // Free the oldest
        free(header_p->group_id[0]);
        // Delete old table
        free(header_p->group_id);
        // Start to use the new table
        header_p->group_id = group_table;
        success_add = 1;
      }
    }
  }
  if (!success_add)
  {
    free(group_data);
  }
  return success_add;
}

/**
 * Check and set an non-existed group id into header.
 *
 * Returns 1 on success set group id, otherwise 0.
 */
static int check_and_set_group_id(Idd__Header* header_p, const char *new_group_id)
{
  int success_check_and_set = 0;
  size_t exist = is_group_id_exist(header_p, new_group_id);
  if (exist == GROUP_ID_NOT_EXIST)
  {
    success_check_and_set = add_group_id(header_p, new_group_id);
  }
  return success_check_and_set;
}

/**
 * Delete an existed group id into header.
 *
 * Returns 1 on success delete group id, otherwise 0.
 */
static int delete_group_id(Idd__Header* header_p, const char *delete_group_id)
{
  size_t exist = is_group_id_exist(header_p, delete_group_id);
  if (exist != GROUP_ID_NOT_EXIST)
  {
    header_p->n_group_id--;
    char** group_table = (char**)malloc(header_p->n_group_id * sizeof(char *));
    if (group_table != NULL)
    {
      size_t i;
      // Copy the old table except the existed group id
      for (i = 0; i < exist ; i++)
      {
        group_table[i] = header_p->group_id[i];
      }
      for (i = exist; i < header_p->n_group_id; i++)
      {
        group_table[i] = header_p->group_id[i+1];
      }
      // free the space for the existed group id
      free(header_p->group_id[exist]);
      // Delete old table
      free(header_p->group_id);
      // Start to use the new table
      header_p->group_id = group_table;
      return 1;
    }
  }
  return 0;
}

/**
 * Replace an existed group id to a specified new group id.
 *
 * Returns 1 on success replace the group id, otherwise 0.
 */
static int replace_group_id(Idd__Header* header_p, const char *to_group_id,
                            const char *from_group_id)
{
  size_t exist = is_group_id_exist(header_p, from_group_id);
  if (exist != GROUP_ID_NOT_EXIST)
  {
    char *group_data = strndup(to_group_id, IDD_MAX_GROUP_ID_SIZE + 1);
    if (group_data != NULL)
    {
      // free the space for the existed group id
      free(header_p->group_id[exist]);
      // Start to use the new entry
      header_p->group_id[exist] = group_data;
      return 1;
    }
  }
  return 0;
}

static int debug_mode(char *build_type)
{
  return (!is_secure_mode_live() ||
          (build_type && (!strncmp(build_type, "userdebug", strlen("userdebug")) ||
          !strncmp(build_type, "eng", strlen("eng")))));
}

static void set_market_model_name(Idd__Header* header_p) {
  // 'XQ-AT51', 'J8210'
  property_get("ro.product.vendor.model", header_p->market_model_name, HEADER_STRING_UNKNOWN);
}

static void set_market_product_name(Idd__Header* header_p) {
  // 'Xperia 1 II', 'Xperia 5'
  property_get("ro.vendor.semc.product.name", header_p->market_product_name, HEADER_STRING_UNKNOWN);
}

static void set_os_version(Idd__Header* header_p) {
  property_get("ro.build.version.release", header_p->os_version, HEADER_STRING_UNKNOWN);
}

static void set_kernel_version(Idd__Header* header_p) {
  struct utsname buf;
  int ret = uname(&buf);
  if (ret) return;
  strncpy(header_p->kernel_version, buf.release, HEADER_STRING_MAX);
}

static void set_sony_product_code(Idd__Header* header_p) {
  const int TA_SONY_PRODUCT_CODE_SIZE_MAX = 8;
  const int spc_max = TA_SONY_PRODUCT_CODE_SIZE_MAX + 1; // The prototype device reports 9 length spc.
  property_get("ro.vendor.somc.sony_product_code", header_p->sony_product_code, HEADER_STRING_UNKNOWN);
  memset(&header_p->sony_product_code[spc_max], '\0', (HEADER_STRING_MAX + 1) - spc_max);
}

static void set_customer_id(Idd__Header* header_p) {
  property_get("ro.vendor.somc.customerid2", header_p->customer_id, HEADER_STRING_UNKNOWN);
}

static void set_subsystem_version(Idd__Header* header_p) {
  const char *file_list[] = {
    "/vendor/firmware_mnt/verinfo/build_id.txt",
    "/vendor/firmware_mnt/verinfo/mcfg_pkg.txt",
    NULL};
  char buf[HEADER_STRING_MAX] = {0};
  int current_index = 0;

  while (file_list[current_index] != NULL)
  {
    FILE* file_p = fopen(file_list[current_index], "r");
    if (file_p != NULL)
    {
      char buf_file[HEADER_STRING_MAX] = {0};
      if (fgets(buf_file, sizeof(buf_file), file_p) != NULL)
      {
        buf_file[strcspn(buf_file, "\n")] = '\0';
        if (current_index > 0 && buf[0] != '\0')
        {
          strlcat(buf, ",", HEADER_STRING_MAX);
        }
        strlcat(buf, buf_file, HEADER_STRING_MAX);
      }
      fclose(file_p);
    }
    else
    {
      IDD_LOGE("Failed to open file %s(%s)", file_list[current_index], strerror(errno));
    }
    current_index++;
  }
  strncpy(header_p->subsystem_version, buf, HEADER_STRING_MAX);
}

/*
 * Load header information from system property or
 * from header_info.conf if it's existed.
 */
static void load_header_info(Idd__Header* header_p)
{
#ifndef PC_BUILD
  char *ta_sw_version = calloc(1, HEADER_STRING_MAX + 1);
  char *ta_sw_variant = calloc(1, HEADER_STRING_MAX + 1);
  uint32_t size = HEADER_STRING_MAX;

  if (!read_miscta_string(TA_SW_VERSION, ta_sw_version, &size))
  {
    IDD_LOGE("Error reading TA_SW_VERSION miscta unit.");
  }
  else
  {
    char *product_id = strtok(ta_sw_version, "_");
    strncpy(header_p->product_id, product_id, strlen(product_id) + 1);
  }
  property_get("ro.vendor.build.id", header_p->software_version, HEADER_STRING_UNKNOWN);

  size = HEADER_STRING_MAX;
  if (!read_miscta_string(TA_SW_VARIANT, ta_sw_variant, &size))
  {
    IDD_LOGE("Error reading TA_SW_VARIANT miscta unit.");
  }
  else
  {
    strncpy(header_p->software_variant, ta_sw_variant, strlen(ta_sw_variant) + 1);
  }

  property_get("ro.vendor.build.type", header_p->build_type, HEADER_STRING_UNKNOWN);
  if (!strncmp(header_p->build_type, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX))
  {
    property_get("ro.build.type", header_p->build_type, HEADER_STRING_UNKNOWN);
  }

  // Load privacy header information if the device is a prototype.
  int dbg_mode = debug_mode(header_p->build_type);
  if (dbg_mode || settings_use_imei_get())
  {
    if (!strncmp(g_device_imei, IMEI_UNKNOWN, IDD_MAX_DEVICE_IMEI_SIZE)) {
      IDD_LOGW("IMEI is UNKNOWN");
    }
    strncpy(header_p->device_imei, g_device_imei, IDD_MAX_DEVICE_IMEI_SIZE);
  }
  if (dbg_mode || settings_use_android_id_get())
  {
    strncpy(header_p->android_id, g_android_id, IDD_MAX_ANDROID_ID_SIZE);
  }
  if (dbg_mode || settings_use_xs_android_id_get())
  {
    strncpy(header_p->xs_android_id, g_xs_android_id, IDD_MAX_XS_ANDROID_ID_SIZE);
  }
  if (dbg_mode || settings_use_xs_guid_get())
  {
    strncpy(header_p->xs_guid, g_xs_guid, IDD_MAX_XS_GUID_SIZE);
  }
  if (dbg_mode)
  {
    serial_number_handle(header_p);

  }

  if (!strncmp(header_p->internal_product_name, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX))
  {
    property_get("vendor.idd.product.name", header_p->internal_product_name, HEADER_STRING_UNKNOWN);
  }

  set_market_model_name(header_p);
  set_market_product_name(header_p);
  set_os_version(header_p);
  set_kernel_version(header_p);

  set_sony_product_code(header_p);
  set_customer_id(header_p);

  set_subsystem_version(header_p);

  // On pure make-builds, the above fetched values will not be set, but the
  // build system defines a few other ones that we can use as "backup".
  if (!strncmp(header_p->product_id, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX))
  {
    IDD_LOGW("product_id is UNKNOWN");
  }
  if (!strncmp(header_p->software_version, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX))
  {
    // TODO: We can indicate build inconsistency if ro.semc.sw_revision was
    // set, but it's different from ro.build.id
    property_get("ro.build.id", header_p->software_version, HEADER_STRING_UNKNOWN);
  }
  // NOTE: "variant" doesn't have a "backup" property.

  // Get the CDF information. For example if CDF id is "1234-5678" and
  // CDF revision is "R6D", then cdf_id is "1234-5678_R6D".
  size = HEADER_STRING_MAX;
  if (!read_miscta_string(TA_CUST_VERSION, header_p->cdf_id, &size))
  {
    IDD_LOGE("Error reading TA_CUST_VERSION miscta unit.");
  }
  free(ta_sw_version);
  free(ta_sw_variant);
#endif
  header_p->timezone = g_timezone;
}

/*
 * Append the header group id information.
 */
static void header_group_id_append(Idd__Header* to_header_p, const Idd__Header* from_header_p)
{
  if (from_header_p->n_group_id != 0)
  {
    size_t i;
    for (i = 0; i < from_header_p->n_group_id; i++)
    {
      // If the group id is not existed, append it to the rear.
      check_and_set_group_id(to_header_p, from_header_p->group_id[i]);
    }
  }
}

/*
 * Load header group id.
 */
static void load_header_group_id(Idd__Header* header_p)
{
  header_group_id_append(header_p, &header);
  char prop_buff[HEADER_STRING_MAX] = HEADER_STRING_UNKNOWN;
  if (strncmp(prop_buff, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX))
  {
    check_and_set_group_id(header_p, prop_buff);
  }
}

/*
 * Check the header information is changed or not.
 * Family (never changed) and uuid (set at server) are not included in the check list.
 * return non 0 if changed.
 */
static int is_header_changed(const Idd__Header* new_header_p, const Idd__Header* old_header_p,
                             int* force_flush)
{
  int serial_number_changed =
    strncmp(new_header_p->serial_number, old_header_p->serial_number, IDD_MAX_SERIAL_NUMBER_SIZE);

  int imei_changed =
    strncmp(new_header_p->device_imei, old_header_p->device_imei, IDD_MAX_DEVICE_IMEI_SIZE);

  int android_id_changed =
    strncmp(new_header_p->android_id, old_header_p->android_id, IDD_MAX_ANDROID_ID_SIZE);

  int xs_android_id_changed =
    strncmp(new_header_p->xs_android_id, old_header_p->xs_android_id, IDD_MAX_XS_ANDROID_ID_SIZE);

  int xs_guid_changed =
    strncmp(new_header_p->xs_guid, old_header_p->xs_guid, IDD_MAX_XS_GUID_SIZE);

  int ret = strncmp(new_header_p->product_id, old_header_p->product_id, HEADER_STRING_MAX) ||
    strncmp(new_header_p->software_version, old_header_p->software_version, HEADER_STRING_MAX) ||
    strncmp(new_header_p->software_variant, old_header_p->software_variant, HEADER_STRING_MAX) ||
    strncmp(new_header_p->build_type, old_header_p->build_type, HEADER_STRING_MAX) ||
    strncmp(new_header_p->config_checksum, old_header_p->config_checksum, HEADER_STRING_MAX) ||
    strncmp(new_header_p->cdf_id, old_header_p->cdf_id, HEADER_STRING_MAX) ||
    strncmp(new_header_p->device_id, old_header_p->device_id, IDD_MAX_DEVICE_ID_SIZE) ||
    (new_header_p->n_group_id == old_header_p->n_group_id ? 0 : 1) ||
    /* An IMEI set for the first time will not trigger a new report */
    (imei_changed && strncmp(old_header_p->device_imei, IMEI_UNKNOWN, IDD_MAX_DEVICE_IMEI_SIZE)) ||
    /* An ANDROID_ID set for the first time will not trigger a new report */
    (android_id_changed &&
     strncmp(old_header_p->android_id, ANDROID_ID_UNKNOWN, IDD_MAX_ANDROID_ID_SIZE)) ||
    (xs_android_id_changed &&
     strncmp(old_header_p->xs_android_id, XS_ANDROID_ID_UNKNOWN, IDD_MAX_XS_ANDROID_ID_SIZE)) ||
    (xs_guid_changed &&
     strncmp(old_header_p->xs_guid, XS_GUID_UNKNOWN, IDD_MAX_XS_GUID_SIZE)) ||
    (new_header_p->user_consent != old_header_p->user_consent) ||
    (new_header_p->session_id == old_header_p->session_id ? 0 : 1);
  if (ret == 0 && new_header_p->n_group_id != 0)
  {
    size_t i;
    for (i = 0; (ret == 0) && (i < new_header_p->n_group_id); i++)
    {
      ret = strncmp(new_header_p->group_id[i], old_header_p->group_id[i], IDD_MAX_GROUP_ID_SIZE);
    }
  }

  /* No serialized header? Then this is the first boot of the device. Don't flush! */
  *force_flush = (header_buf_size > 0) && ret;

  /* Now check if any member has changed that will not force a report flush */
  return (ret ||
          serial_number_changed || imei_changed || android_id_changed || xs_android_id_changed ||
          xs_guid_changed || (new_header_p->timezone != old_header_p->timezone));
}

/*
 * Using protobuf api to pack the header.
 */
static void pack_header()
{
  free((void*)header_buf_p);
  header_buf_p = NULL;
  Idd__Report report = IDD__REPORT__INIT;
  report.header = &header;
  header_buf_size = idd__report__get_packed_size(&report);
  if (header_buf_size > 0)
  {
    header_buf_p = malloc(header_buf_size);
    if (header_buf_p != NULL)
    {
      if (idd__report__pack(&report, (uint8_t*)header_buf_p) != header_buf_size)
      {
        IDD_LOGE("Failed to pack report header.");
        free((void*)header_buf_p);
        header_buf_p = NULL;
      }
    }
  }
  else
  {
    IDD_LOGE("Failed to get packed size of report header.");
  }
}

/*
 * Using protobuf api to pack the rca header
 */
static void pack_rca_header()
{
  free((void*)rca_header_buf_p);
  rca_header_buf_p = NULL;
  Idd__Report report = IDD__REPORT__INIT;
  report.header = &header;

  rca_header_buf_size = idd__report__get_packed_size(&report);
  if (rca_header_buf_size > 0)
  {
    rca_header_buf_p = malloc(rca_header_buf_size);
    if (rca_header_buf_p != NULL)
    {
      strncpy(report.header->device_id, g_rca_device_id, IDD_MAX_DEVICE_ID_SIZE);
      if (idd__report__pack(&report, (uint8_t*)rca_header_buf_p) != rca_header_buf_size)
      {
        IDD_LOGE("Failed to pack rca report header");
        free((void*)rca_header_buf_p);
        rca_header_buf_p = NULL;
      }
      // Reverting the device_id to g_device_id to avoid wrong reporting
      // and to maintain the original device_id in report header.
      strncpy(report.header->device_id, g_device_id, IDD_MAX_DEVICE_ID_SIZE);
    }
  }
  else
  {
    IDD_LOGE("Failed to get packed size of rca report header");
  }
}

/*
 * Copy the header information.
 * The family, uuid are not included.
 */
static void header_copy(Idd__Header* to_header_p, const Idd__Header* from_header_p)
{
  if (strncmp(from_header_p->product_id, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX))
  {
    strncpy(to_header_p->product_id, from_header_p->product_id, HEADER_STRING_MAX);
  }
  if (strncmp(from_header_p->software_version, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX))
  {
    strncpy(to_header_p->software_version, from_header_p->software_version, HEADER_STRING_MAX);
  }
  if (strncmp(from_header_p->software_variant, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX))
  {
    strncpy(to_header_p->software_variant, from_header_p->software_variant, HEADER_STRING_MAX);
  }
  if (strncmp(from_header_p->build_type, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX))
  {
    strncpy(to_header_p->build_type, from_header_p->build_type, HEADER_STRING_MAX);
  }
  strncpy(to_header_p->config_checksum, from_header_p->config_checksum, HEADER_STRING_MAX);
  if (strncmp(from_header_p->cdf_id, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX))
  {
    strncpy(to_header_p->cdf_id, from_header_p->cdf_id, HEADER_STRING_MAX);
  }
  if (strncmp(from_header_p->serial_number, HEADER_STRING_UNKNOWN, IDD_MAX_SERIAL_NUMBER_SIZE))
  {
    strncpy(to_header_p->serial_number, from_header_p->serial_number, IDD_MAX_SERIAL_NUMBER_SIZE);
  }
  strncpy(to_header_p->device_imei, from_header_p->device_imei, IDD_MAX_DEVICE_IMEI_SIZE);
  if (strncmp(from_header_p->device_id, HEADER_STRING_UNKNOWN, IDD_MAX_DEVICE_ID_SIZE))
  {
    strncpy(to_header_p->device_id, from_header_p->device_id, IDD_MAX_DEVICE_ID_SIZE);
  }
  to_header_p->timezone = from_header_p->timezone;
  if (from_header_p->session_id != SESSION_ID_UNKNOWN)
  {
    to_header_p->session_id = from_header_p->session_id;
  }
  strncpy(to_header_p->android_id, from_header_p->android_id, IDD_MAX_ANDROID_ID_SIZE);
  strncpy(to_header_p->xs_android_id, from_header_p->xs_android_id, IDD_MAX_XS_ANDROID_ID_SIZE);
  strncpy(to_header_p->xs_guid, from_header_p->xs_guid, IDD_MAX_XS_GUID_SIZE);
  if (strncmp(from_header_p->internal_product_name, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX))
  {
    strncpy(to_header_p->internal_product_name, from_header_p->internal_product_name, HEADER_STRING_MAX);
  }
  if (strncmp(from_header_p->market_model_name, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX))
  {
    strncpy(to_header_p->market_model_name, from_header_p->market_model_name, HEADER_STRING_MAX);
  }
  if (strncmp(from_header_p->market_product_name, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX))
  {
    strncpy(to_header_p->market_product_name, from_header_p->market_product_name, HEADER_STRING_MAX);
  }
  if (strncmp(from_header_p->os_version, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX))
  {
    strncpy(to_header_p->os_version, from_header_p->os_version, HEADER_STRING_MAX);
  }
  if (strncmp(from_header_p->kernel_version, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX))
  {
    strncpy(to_header_p->kernel_version, from_header_p->kernel_version, HEADER_STRING_MAX);
  }
  if (strncmp(from_header_p->sony_product_code, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX))
  {
    strncpy(to_header_p->sony_product_code, from_header_p->sony_product_code, HEADER_STRING_MAX);
  }
  if (strncmp(from_header_p->customer_id, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX))
  {
    strncpy(to_header_p->customer_id, from_header_p->customer_id, HEADER_STRING_MAX);
  }
  if (strncmp(from_header_p->subsystem_version, HEADER_STRING_UNKNOWN, HEADER_STRING_MAX))
  {
    strncpy(to_header_p->subsystem_version, from_header_p->subsystem_version, HEADER_STRING_MAX);
  }

  to_header_p->user_consent = from_header_p->user_consent;
}

void report_header_init()
{
  init_header(&header);
}

/*
 * Check the header is valid or not.
 * Imei, ANDROID_ID and serial number must exist if the device is a prototype.
 */
int is_valid_header(void)
{
  Idd__Header* header_p = &header;
  return (!debug_mode(header_p->build_type) ||
          (strncmp(header_p->serial_number, HEADER_STRING_UNKNOWN, IDD_MAX_SERIAL_NUMBER_SIZE) &&
           header_p->timezone != TIMEZONE_UNKNOWN &&
           strncmp(header_p->device_imei, IMEI_UNKNOWN, IDD_MAX_DEVICE_IMEI_SIZE) &&
           strncmp(header_p->android_id, ANDROID_ID_UNKNOWN, IDD_MAX_ANDROID_ID_SIZE) &&
           strncmp(header_p->xs_android_id, XS_ANDROID_ID_UNKNOWN, IDD_MAX_XS_ANDROID_ID_SIZE)));
}

static void print_header(char* title, Idd__Header* header_p)
{
  IDD_LOGD("--- [%s] ---\nproduct = %s\nsw_version = %s\n"
      "variant = %s\nbuild_type = %s\ntimezone = %d\ndevice_id = %s\n"
      "serial_num = %s\ndevice_imei = %s\ncdf_id = %s\nconfig_checksum = %s\n"
      "secure_mode = %d\nsession_id = %"PRIu64"\nandroid_id = %s\nxs_android_id = %s\n"
      "xs_guid = %s\nsony_product_code = %s\ncustomer_id = %s\nsubsystem_version = %s\n"
      "internal_product_name = %s\nmarket_model_name = %s\nmarket_product_name = %s\n"
      "os_version = %s\nkernel_version = %s\nuser_consent = %d",
      title, header_p->product_id, header_p->software_version, header_p->software_variant,
      header_p->build_type, header_p->timezone, header_p->device_id,
      header_p->serial_number, header_p->device_imei, header_p->cdf_id, header_p->config_checksum,
      header_p->secure_mode, header_p->session_id, header_p->android_id, header_p->xs_android_id,
      header_p->xs_guid, header_p->sony_product_code, header_p->customer_id, header_p->subsystem_version,
      header_p->internal_product_name, header_p->market_model_name, header_p->market_product_name,
      header_p->os_version, header_p->kernel_version, header_p->user_consent);
  if (header_p->n_group_id != 0)
  {
    size_t i;
    IDD_LOGD("#group_ids = %zu", header_p->n_group_id);
    for (i = 0; i < header_p->n_group_id; i++)
    {
      IDD_LOGD("group_id[%zu] = %s", i, header_p->group_id[i]);
    }
  }
}

/*
 * This will generate a report flush if header is changed.
 */
void create_report_header()
{
  int force_new;
  Idd__Header new_header = IDD__HEADER__INIT;
  init_header(&new_header);
  load_header_info(&new_header);
  load_header_group_id(&new_header); // BUG! Group changing will not lead to new report.

  /*
   * Device Id, Serial Number and Secure Mode are fixed values.
   * So if new value is unknown and old value is not unknown,
   * should assign valid old value to new.
   */
  if (!strncmp(new_header.device_id, HEADER_STRING_UNKNOWN, IDD_MAX_DEVICE_ID_SIZE) &&
      strncmp(g_device_id, HEADER_STRING_UNKNOWN, IDD_MAX_DEVICE_ID_SIZE))
  {
    strncpy(new_header.device_id, g_device_id, IDD_MAX_DEVICE_ID_SIZE);
  }

  if (!strncmp(new_header.serial_number, HEADER_STRING_UNKNOWN, IDD_MAX_SERIAL_NUMBER_SIZE) &&
      strncmp(header.serial_number, HEADER_STRING_UNKNOWN, IDD_MAX_SERIAL_NUMBER_SIZE))
  {
    strncpy(new_header.serial_number, header.serial_number, IDD_MAX_SERIAL_NUMBER_SIZE);
  }

  new_header.session_id = g_session_id;

  new_header.user_consent = g_user_consent;

  /* HEADER_STRING_MAX > CONFIG_CHECKSUM_LEN * 2 + 1 (63 > 41): */
  base16_encode(new_header.config_checksum, config_checksum, CONFIG_CHECKSUM_LEN);

  Idd__Header old_header = IDD__HEADER__INIT;
  init_header(&old_header);

  header_copy(&old_header, &header);
  /*
   * Header_copy doesn't deal with group id,
   * for group id, only it is a new that will be added into header.
   */
  header_group_id_append(&old_header, &header);
  header_copy(&header, &new_header);
  header_group_id_append(&header, &new_header);
  print_header("New header", &header);

  if (is_header_changed(&new_header, &old_header, &force_new))
  {
    print_header("Old header", &old_header);

    if (force_new)
    {
      /* The header has changed in a way that require a report flush. That will be done in a
       * forked process. If the currents events file don't have a header, typically at first boot
       * of a prototype or only new staticdata or counters, the header will be written in the
       * forked process with the header serialized by pack_header(). Thats why pack_header() has
       * to be called after generate_report().
       */
      (void) generate_report();
      plugins_flush();
    }

    /* If the report header is missing the old header shall be used. That's why pack_header()
     * must be called after generate_report().
     */
    pack_header();
  }
  else
  {
    IDD_LOGD("Report header unchanged");
  }

  state_save();

  free_header(&old_header);
  free_header(&new_header);

  if (header_buf_p == NULL)
  {
    die("OOM: header_buf_p");
  }
}

void util_shutdown(void)
{
  free((void*)header_buf_p);
  free((void*)rca_header_buf_p);
  free_header(&header);
}

void write_report_header(int fd, Idd_Write_Report_Header_t force)
{
  if ((force == FORCE_WRITE) || is_valid_header())
  {
    IDD_LOGD("Writing report header (%zd bytes)", header_buf_size);
    if (write_all(fd, header_buf_p, header_buf_size) < 0)
    {
      IDD_LOGE("Failed to write report header: %s", strerror(errno));
    }
  }
  else
  {
    IDD_LOGD("Report header not written! force=%d && valid=%d", force, is_valid_header());
  }
}

void write_rca_report_header(int fd)
{
    pack_rca_header();
    if (rca_header_buf_p != NULL) {
      if (write_all(fd, rca_header_buf_p, rca_header_buf_size) < 0)
      {
        IDD_LOGE("Failed to write report header: %s", strerror(errno));
      }
    }
}

size_t get_report_header(const char** buffer_pp)
{
  *buffer_pp = header_buf_p;
  return header_buf_size;
}

size_t get_empty_report_header(const char** buffer_pp)
{
  static const char* empty_header_buf_p = NULL;
  static size_t empty_header_buf_size = 0;

  if (empty_header_buf_p == NULL)
  {
    Idd__Report empty_report = IDD__REPORT__INIT;
    Idd__Header empty_header = IDD__HEADER__INIT;
    empty_report.header = &empty_header;
    empty_header_buf_size = idd__report__get_packed_size(&empty_report);
    if (empty_header_buf_size > 0)
    {
      empty_header_buf_p = malloc(empty_header_buf_size);
      if (empty_header_buf_p != NULL)
      {
        if (idd__report__pack(&empty_report, (uint8_t*)empty_header_buf_p) != empty_header_buf_size)
        {
          IDD_LOGE("Failed to pack empty report header.");
          free((void*)empty_header_buf_p);
          empty_header_buf_p = NULL;
          empty_header_buf_size = 0;
        }
      }
      else
      {
        empty_header_buf_size = 0;
        IDD_LOGE("OOM for empty report header.");
      }
    }
    else
    {
      IDD_LOGE("Failed to get packed size of empty report header.");
    }
  }

  *buffer_pp = empty_header_buf_p;
  return empty_header_buf_size;
}

int is_verbose(void)
{
  return verbose;
}

void set_verbose(int val)
{
  verbose = val;
}

static void remove_all_reports()
{
  struct dirent *de;
  DIR *dir;
  char name[PATH_MAX] = {0};

  dir = opendir(OUTPUT_DIR_NAME);
  if (dir == NULL)
  {
    return ;
  }

  IDD_LOGD("Delete all reports!");

  rewinddir(dir);

  while ((de = readdir(dir)) != NULL)
  {
    if (strcmp(de->d_name, ".") == 0 ||
        strcmp(de->d_name, "..") == 0)
    {
      continue;
    }

    snprintf(name, sizeof(name), "%s%s", OUTPUT_DIR_NAME, de->d_name);

    if (unlink(name) == 0)
    {
      IDD_LOGI("Deleting %s from output directory", name);
    }
  }

  closedir(dir);
}

void remove_report(int report_num)
{
  pid_t compactor_pid = get_compactor_pid();
  if (compactor_pid != -1)
  {
    /* Move on to next step after confirming that compactor is not running */
    waitpid(compactor_pid, NULL, 0);
  }

  if (report_num == 0)
  {
    remove_all_reports();
    remove_events();
  }
  else
  {
    char name[PATH_MAX] = {0};
    snprintf(name, sizeof(name), "%sidd_%d", OUTPUT_DIR_NAME, report_num);
    if (unlink(name) == 0)
    {
      IDD_LOGI("Deleting %s from output directory", name);
    }
    else
    {
      IDD_LOGW("Failed to delete %s : %s", name, strerror(errno));
    }
  }
  /* Secure that IDD report is surely removed */
  sync();
}

/* When strtoll is limited to 32 bits */
int64_t strtoint64(const char* s)
{
  int64_t result = 0;
  int negative = 0;

  if (*s == '-')
  {
    negative = 1;
    s++;
  }

  for ( ; *s != 0; s++)
  {
    int val = (*s - '0');
    result = 10 * result + val;
  }

  return negative ? -result : result;
}

int64_t get_timestamp(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void timezone_handle(void)
{
  int32_t tm_gmtoff = TIMEZONE_UNKNOWN;
  struct tm now;
  time_t t;
  time(&t);
  tzset();
  localtime_r(&t, &now);
  tm_gmtoff = now.tm_gmtoff;
  g_timezone = tm_gmtoff;
  IDD_LOGD("Timezone: %d", g_timezone);
  create_report_header();
}

void die(const char* str)
{
  (void)str;
  IDD_LOGE("%s", str);
  exit(EXIT_FAILURE);
}

/*
 * Set new wait time for save to file of counters and staticdata
 */
void save_time_update()
{
  last_save_time = time(NULL);
}

/*
 * Returns number of seconds to next save to file of counters and staticdata
 */
int save_time_left()
{
  time_t now = time(NULL);
  return MAX(IDD_DEFAULT_SAVE_INTERVAL - (now - last_save_time), 0);
}

/* base16 - encode and decode. Very simple, and is
 * only compatible with itself and other lower-case
 * variants.
 */
static int b16char(char c)
{
  switch (c)
  {
  case '0': case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    return c - '0';
  case 'a': case 'b': case 'c':
  case 'd': case 'e': case 'f':
    return c - 'a' + 10;
  default:
    return 0;
  }
}

static void base16_decode(char* dest_p, const char* src_p, size_t src_length)
{
  if ((src_length & 1) == 0)
  {
    size_t i;
    for (i = 0; i < src_length / 2; i++)
    {
      int c0 = b16char(*(src_p++));
      int c1 = b16char(*(src_p++));
      *dest_p++ = (c0 << 4) | c1;
    }
  }
}

static void base16_encode(char* dest_p, const char* src_p, size_t src_length)
{
  static const char hex_table[] = "0123456789abcdef";
  size_t i;

  for (i = 0; i < src_length; i++)
  {
    dest_p[i * 2 + 0] = hex_table[(src_p[i] & 0xF0) >> 4];
    dest_p[i * 2 + 1] = hex_table[(src_p[i] & 0x0F)];
  }
  dest_p[i * 2] = 0;
}

/*
 *
 */
void state_load()
{
  size_t new_size;
  int flag = 0;
  FILE* file_p = fopen(IDD_STATE_FNAME, "r");
  if (file_p != NULL)
  {
    int64_t shutdown_time = 0;
    char buffer[256] = {0};
    long timestamp;

    while (fgets(buffer, sizeof(buffer), file_p) != NULL)
    {
      char convertbuffer[60] = {0};
      if (sscanf(buffer, IDD_STATE_TAG_CONFIG_CHECKSUM "%59s", convertbuffer) &&
          (strlen(convertbuffer) == (CONFIG_CHECKSUM_LEN * 2)))
      {
        base16_decode(config_checksum, convertbuffer, CONFIG_CHECKSUM_LEN * 2);
        strncpy(header.config_checksum, convertbuffer, HEADER_STRING_MAX);
      }
      else if (sscanf(buffer, IDD_STATE_TAG_SHUTDOWN_TIME "%" PRId64, &shutdown_time))
      {
        // Test mode, set the internal test clock to what it was when we last stopped.
        test_mode_tv.tv_sec = (shutdown_time / 1000);
      }
      else if (sscanf(buffer, IDD_STATE_TAG_LAST_FLUSH "%" PRId64, &flushtime_last))
      {
        /* Test mode and no test time set yet. Let us assume that no more than
         * 1000 events events has arrived since last flush.
         */
        test_mode_tv.tv_sec = (flushtime_last / 1000) + 1000;
      }
      else if (sscanf(buffer, IDD_STATE_TAG_DEVICE_ID "%" IDD_MAX_DEVICE_ID_SIZE_STR "s", g_device_id))
      {
        strncpy(header.device_id, g_device_id, IDD_MAX_DEVICE_ID_SIZE);
      }
      else if (sscanf(buffer, IDD_STATE_TAG_DEVICE_ID_CR "%" IDD_MAX_DEVICE_ID_SIZE_STR "s", g_device_id_cr))
      {
        //Do nothing
      }
      else if (sscanf(buffer, IDD_STATE_TAG_RCA_DEVICE_ID "%" IDD_MAX_DEVICE_ID_SIZE_STR "s", g_rca_device_id))
      {
        //Do nothing
      }
      else if (sscanf(buffer, IDD_STATE_TAG_REPORT_MAX_SIZE "%zd", &new_size))
      {
        settings_report_max_size_set(new_size);
      }
      else if (sscanf(buffer, IDD_STATE_TAG_OUTPUT_DIR_MAX_SIZE "%zd", &new_size))
      {
        settings_output_dir_max_size_set(new_size);
      }
      else if (sscanf(buffer, IDD_STATE_TAG_PRODUCT "%s", header.product_id))
      {
        //Do nothing
      }
      else if (sscanf(buffer, IDD_STATE_TAG_SW_VERSION "%s", header.software_version))
      {
        //Do nothing
      }
      else if (sscanf(buffer, IDD_STATE_TAG_VARIANT "%s", header.software_variant))
      {
        //Do nothing
      }
      else if (sscanf(buffer, IDD_STATE_TAG_BUILD_TYPE "%s", header.build_type))
      {
        //Do nothing
      }
      else if (sscanf(buffer, IDD_STATE_TAG_CDF_ID "%s", header.cdf_id))
      {
        //Do nothing
      }
      else if (sscanf(buffer, IDD_STATE_TAG_SERIAL_NUMBER "%s", header.serial_number))
      {
        //Do nothing
      }
      else if (sscanf(buffer, IDD_STATE_TAG_USE_IMEI "%d", &flag))
      {
        settings_use_imei_set((bool)flag);
      }
      else if (sscanf(buffer, IDD_STATE_TAG_DEVICE_IMEI "%s", g_device_imei))
      {
        //Do nothing
      }
      else if (sscanf(buffer, IDD_STATE_TAG_GROUP_ID "%s", convertbuffer))
      {
        check_and_set_group_id(&header, convertbuffer);
      }
      else if (sscanf(buffer, IDD_STATE_TAG_EXPIRE_COUNTERS "%zd", &new_size))
      {
        settings_expire_counters_set(new_size);
      }
      else if (sscanf(buffer, IDD_STATE_TAG_EXPIRE_SDATA "%zd", &new_size))
      {
        settings_expire_sdata_set(new_size);
      }
      else if (sscanf(buffer, IDD_STATE_TAG_DATA_COLLECTION "%1d", &flag))
      {
        set_data_collection_enabled(flag);
      }
      else if (sscanf(buffer, IDD_STATE_TAG_SESSION_ID "%"PRIu64"", &header.session_id))
      {
        //Do nothing
      }
      else if (sscanf(buffer, IDD_STATE_TAG_WASH_DATA_ID "%d", &flag))
      {
        settings_wash_data_set((wash_config_t)flag);
      }
      else if (sscanf(buffer, IDD_STATE_TAG_USE_ANDROID_ID "%d", &flag))
      {
        settings_use_android_id_set((bool)flag);
      }
      else if (sscanf(buffer, IDD_STATE_TAG_USE_XS_ANDROID_ID "%d", &flag))
      {
        settings_use_xs_android_id_set((bool)flag);
      }
      else if (sscanf(buffer, IDD_STATE_TAG_USE_XS_GUID "%d", &flag))
      {
        settings_use_xs_guid_set((bool)flag);
      }
      else if (sscanf(buffer, IDD_STATE_TAG_ANDROID_ID "%s", g_android_id))
      {
        //Do nothing
      }
      else if (sscanf(buffer, IDD_STATE_TAG_XS_ANDROID_ID "%s", g_xs_android_id))
      {
        //Do nothing
      }
      else if (sscanf(buffer, IDD_STATE_TAG_XS_GUID "%s", g_xs_guid))
      {
        //Do nothing
      }
      else if (sscanf(buffer, IDD_STATE_TAG_INTERNAL_PRODUCT_NAME "%s", header.internal_product_name))
      {
        //Do nothing
      }
      else if (sscanf(buffer, IDD_STATE_TAG_MARKET_MODEL_NAME "%s", header.market_model_name))
      {
        //Do nothing
      }
      else if (sscanf(buffer, IDD_STATE_TAG_MARKET_PRODUCT_NAME "%s", header.market_product_name))
      {
        //Do nothing
      }
      else if (sscanf(buffer, IDD_STATE_TAG_OS_VERSION "%s", header.os_version))
      {
        //Do nothing
      }
      else if (sscanf(buffer, IDD_STATE_TAG_KERNEL_VERSION "%s", header.kernel_version))
      {
        //Do nothing
      }
      else if (sscanf(buffer, IDD_STATE_TAG_SONY_PRODUCT_CODE "%s", header.sony_product_code))
      {
        //Do nothing
      }
      else if (sscanf(buffer, IDD_STATE_TAG_CUSTOMER_ID "%s", header.customer_id))
      {
        //Do nothing
      }
#if 0 // Don't load subsystem version here.
      // Use /vendor/firmware_mnt/verinfo/build_id.txt for subsystem version.
      else if (sscanf(buffer, IDD_STATE_TAG_SUBSYSTEM_VERSION "%s", header.subsystem_version))
      {
        //Do nothing
      }
#endif
      else if (sscanf(buffer, IDD_STATE_TAG_USER_CONSENT "%d", &g_user_consent))
      {
        header.user_consent = g_user_consent;
      }
      else if (sscanf(buffer, IDD_STATE_TAG_REQUIRED_USER_CONSENT "%d", &flag))
      {
        settings_required_user_consent_set((Iddd__UserConsent__Type)flag);
      }
      else if (sscanf(buffer, IDD_STATE_TAG_TIMEZONE "%d", &g_timezone))
      {
        header.timezone = g_timezone;
      }
      else
      {
        IDD_LOGD("Unknown line read from " IDD_STATE_FNAME ": %s", buffer);
      }
    }
    fclose(file_p);

    if (debug_mode(header.build_type) || settings_use_imei_get())
    {
      // The device is a prototype! Add imei to header.
      if (strncmp(g_device_imei, IMEI_UNKNOWN, IDD_MAX_DEVICE_IMEI_SIZE))
      {
        strncpy(header.device_imei, g_device_imei, IDD_MAX_DEVICE_IMEI_SIZE);
      }
    }
    if (debug_mode(header.build_type) || settings_use_android_id_get())
    {
      // The device is allowed to add android_id to header.
      if (strncmp(g_android_id, ANDROID_ID_UNKNOWN, IDD_MAX_ANDROID_ID_SIZE))
      {
        strncpy(header.android_id, g_android_id, IDD_MAX_ANDROID_ID_SIZE);
      }
    }
    if (debug_mode(header.build_type) || settings_use_xs_android_id_get())
    {
      // The device is allowed to add xs_android_id to header.
      if (strncmp(g_xs_android_id, XS_ANDROID_ID_UNKNOWN, IDD_MAX_XS_ANDROID_ID_SIZE))
      {
        strncpy(header.xs_android_id, g_xs_android_id, IDD_MAX_XS_ANDROID_ID_SIZE);
      }
    }
    if (debug_mode(header.build_type) || settings_use_xs_guid_get())
    {
      // The device is allowed to add xs_guid to header.
      if (strncmp(g_xs_guid, XS_GUID_UNKNOWN, IDD_MAX_XS_GUID_SIZE))
      {
        strncpy(header.xs_guid, g_xs_guid, IDD_MAX_XS_GUID_SIZE);
      }
    }

    pack_header(); // Create a serialized header to use in reports.
  }
  else if (errno != ENOENT)
  {
    IDD_LOGE("Failed to open iddd state file " IDD_STATE_FNAME ": %s", strerror(errno));
  }
}

/*
 *
 */
void state_save()
{
  size_t i;
  FILE* fp = fopen(IDD_STATE_FNAME "_tmp", "wb");
  if (fp != NULL)
  {
    char checksum_base16[CONFIG_CHECKSUM_LEN * 2 + 1] = {0};
    base16_encode(checksum_base16, config_checksum, CONFIG_CHECKSUM_LEN);
    fprintf(fp, IDD_STATE_TAG_CONFIG_CHECKSUM "%s\n", checksum_base16);
    fprintf(fp, IDD_STATE_TAG_LAST_FLUSH "%" PRId64 "\n", flushtime_last);
    fprintf(fp, IDD_STATE_TAG_SHUTDOWN_TIME "%" PRId64 "\n", get_timestamp());
    fprintf(fp, IDD_STATE_TAG_DEVICE_ID"%s\n", g_device_id);
    fprintf(fp, IDD_STATE_TAG_DEVICE_ID_CR"%s\n", g_device_id_cr);
    fprintf(fp, IDD_STATE_TAG_RCA_DEVICE_ID"%s\n", g_rca_device_id);
    fprintf(fp, IDD_STATE_TAG_REPORT_MAX_SIZE "%zd\n", settings_report_max_size_get());
    fprintf(fp, IDD_STATE_TAG_OUTPUT_DIR_MAX_SIZE "%zd\n", settings_output_dir_max_size_get());
    fprintf(fp, IDD_STATE_TAG_PRODUCT "%s\n", header.product_id);
    fprintf(fp, IDD_STATE_TAG_SW_VERSION "%s\n", header.software_version);
    fprintf(fp, IDD_STATE_TAG_VARIANT "%s\n", header.software_variant);
    fprintf(fp, IDD_STATE_TAG_BUILD_TYPE "%s\n", header.build_type);
    fprintf(fp, IDD_STATE_TAG_CDF_ID "%s\n", header.cdf_id);
    fprintf(fp, IDD_STATE_TAG_SECURE_MODE"%d\n", header.secure_mode);
    fprintf(fp, IDD_STATE_TAG_USE_IMEI"%d\n", settings_use_imei_get());
    fprintf(fp, IDD_STATE_TAG_TIMEZONE"%d\n", g_timezone);
    fprintf(fp, IDD_STATE_TAG_SERIAL_NUMBER "%s\n", header.serial_number);
    fprintf(fp, IDD_STATE_TAG_DEVICE_IMEI"%s\n", g_device_imei);
    fprintf(fp, IDD_STATE_TAG_EXPIRE_COUNTERS "%zd\n", settings_expire_counters_get());
    fprintf(fp, IDD_STATE_TAG_EXPIRE_SDATA "%zd\n", settings_expire_sdata_get());
    fprintf(fp, IDD_STATE_TAG_DATA_COLLECTION "%1d\n", is_data_collection_enabled());
    fprintf(fp, IDD_STATE_TAG_SESSION_ID"%"PRIu64"\n", header.session_id);
    fprintf(fp, IDD_STATE_TAG_WASH_DATA_ID"%d\n", settings_wash_data_get());
    fprintf(fp, IDD_STATE_TAG_ANDROID_ID"%s\n", g_android_id);
    fprintf(fp, IDD_STATE_TAG_USE_ANDROID_ID"%d\n", settings_use_android_id_get());
    fprintf(fp, IDD_STATE_TAG_XS_ANDROID_ID"%s\n", g_xs_android_id);
    fprintf(fp, IDD_STATE_TAG_USE_XS_ANDROID_ID"%d\n", settings_use_xs_android_id_get());
    fprintf(fp, IDD_STATE_TAG_XS_GUID"%s\n", g_xs_guid);
    fprintf(fp, IDD_STATE_TAG_USE_XS_GUID"%d\n", settings_use_xs_guid_get());
    fprintf(fp, IDD_STATE_TAG_INTERNAL_PRODUCT_NAME "%s\n", header.internal_product_name);

    fprintf(fp, IDD_STATE_TAG_MARKET_MODEL_NAME "%s\n", header.market_model_name);
    fprintf(fp, IDD_STATE_TAG_MARKET_PRODUCT_NAME "%s\n", header.market_product_name);
    fprintf(fp, IDD_STATE_TAG_OS_VERSION "%s\n", header.os_version);
    fprintf(fp, IDD_STATE_TAG_KERNEL_VERSION "%s\n", header.kernel_version);

    fprintf(fp, IDD_STATE_TAG_SONY_PRODUCT_CODE "%s\n", header.sony_product_code);
    fprintf(fp, IDD_STATE_TAG_CUSTOMER_ID "%s\n", header.customer_id);
    fprintf(fp, IDD_STATE_TAG_SUBSYSTEM_VERSION "%s\n", header.subsystem_version);
    fprintf(fp, IDD_STATE_TAG_USER_CONSENT"%d\n", g_user_consent);
    fprintf(fp, IDD_STATE_TAG_REQUIRED_USER_CONSENT"%d\n", settings_required_user_consent_get());

    for (i = 0; i < header.n_group_id; i++)
    {
      if (header.group_id[i] != NULL)
      {
        fprintf(fp, IDD_STATE_TAG_GROUP_ID "%s\n", header.group_id[i]);
      }
    }

    fclose(fp);

    if (rename(IDD_STATE_FNAME "_tmp", IDD_STATE_FNAME) == -1)
    {
      IDD_LOGE("Failed to rename iddd state file: %s", strerror(errno));
    }
  }
  else
  {
    IDD_LOGE("Failed to create new iddd state file: %s", strerror(errno));
  }

  IDD_LOGD("Renamed iddd state file " IDD_STATE_FNAME
           "_tmp to " IDD_STATE_FNAME);
}

/*
 * Set new flush time
 */
void iddd_flushtime_set(int64_t new_flushtime)
{
  flushtime_last = new_flushtime;
}

/*
 * Return the time when the last flush to output file was done
 */
int64_t iddd_flushtime_get()
{
  return flushtime_last;
}

/*
 *
 */
int set_config_checksum(char* checksum_p)
{
  int changed = strnlen(config_checksum, CONFIG_CHECKSUM_LEN) && // Not changed if first IDD config
                strncmp(config_checksum, checksum_p, CONFIG_CHECKSUM_LEN);
  memcpy(config_checksum, checksum_p, CONFIG_CHECKSUM_LEN);
  return changed;
}

void get_config_checksum(char* checksum_p)
{
  memcpy(checksum_p, config_checksum, CONFIG_CHECKSUM_LEN);
}

/* Check whether data collection is enabled or not.
 *
 * Returns 1 if enabled, 0 if disabled.
 */
int is_data_collection_enabled(void)
{
  return data_collection_enabled;
}

/* Enable/disable data collection.
 *
 * @param enable: 1 is enable, 0 is disable.
 */
void set_data_collection_enabled(int enable)
{
  data_collection_enabled = enable;
  plugins_data_collection_enabled(enable);
}

/*
 * Rollback config along an incoming user consent.
 */
static void rollback_config(int user_consent)
{
  switch (user_consent)
  {
  case IDD__USER_CONSENT__ANONYMOUS:
    load_anonymous_config() ? : load_default_iddd_config(1);
    break;
  case IDD__USER_CONSENT__NONE:
    load_default_iddd_config(1);
    break;
  }
  load_new_configuration(1);
  return;
}

/*
 * Set user consent.
 */
static void set_user_consent(int user_consent)
{
  if (g_user_consent != (uint32_t)user_consent)
  {
    IDD_LOGI("New user consent received: <%d>", user_consent);
    if (g_user_consent > (uint32_t)user_consent)
    {
      rollback_config(user_consent);
    }
    g_user_consent = user_consent;
    generate_device_id(1);
    create_report_header();
  }
}

/*
 * Handle an incoming user consent.
 */
void user_consent_handle(int user_consent)
{
  if (debug_mode(header.build_type))
  {
    IDD_LOGI("This device is prototype keeping user consent ALL, received user consent <%d> was "
             "ignored", user_consent);
  }
  else
  {
    if (g_user_consent != (uint32_t)user_consent)
    {
      remove_report(0);
      set_user_consent(user_consent);
    }
  }
}

/*
 * Returns the file size of the open file descriptor.
 * The file position will be set to the start of the file.
 *
 * Returns 0 on success, -1 on error.
 */
int get_file_size(FILE* in_fp, size_t* size_p)
{
  if (size_p == NULL)
  {
    return -1;
  }

  *size_p = 0;
  if (fseek(in_fp, 0, SEEK_END) == 0)
  {
    long size = ftell(in_fp);
    if (fseek(in_fp, 0, SEEK_SET) == 0)
    {
      *size_p = (size_t)size;
      return 0;
    }
  }
  return -1;
}

/*
 * Read serial number from MiscTa.
 */
static void serial_number_handle(Idd__Header* header_p)
{
  char *ta_serial_number = calloc(1, HEADER_STRING_MAX + 1);
  uint32_t size = HEADER_STRING_MAX;

  if (!read_miscta_string(TA_MSN, ta_serial_number, &size))
  {
    IDD_LOGE("Error reading SERIAL_NUMBER %d miscta unit.", TA_MSN);
  }
  else
  {
    IDD_LOGD("Serial number miscta unit %d: %s", TA_MSN, ta_serial_number);
    strncpy(header_p->serial_number, ta_serial_number, IDD_MAX_SERIAL_NUMBER_SIZE);
  }
  free(ta_serial_number);
}

/*
 * Handle incoming header group id -- set a new group id.
 */
void group_id_set_handle(const char* new_group_id, int create_report)
{
  if (new_group_id != NULL)
  {
    if (check_and_set_group_id(&header, new_group_id) && create_report)
    {
      create_report_header();
    }
  }
}

/*
 * Handle incoming header group id -- delete an existed group id.
 */
void group_id_delete_handle(const char* del_group_id, int create_report)
{
  if (del_group_id != NULL)
  {
    if (delete_group_id(&header, del_group_id) && create_report)
    {
      create_report_header();
    }
  }
}

/*
 * Handle incoming header group id -- replace an existed group id to a specified new group id.
 */
void group_id_replace_handle(const char* to_group_id, const char* from_group_id, int create_report)
{
  if (to_group_id != NULL && from_group_id != NULL)
  {
    if (replace_group_id(&header, to_group_id, from_group_id) && create_report)
    {
      create_report_header();
    }
  }
}

/*
 * Handle an incoming IMEI.
 */
void imei_handle(const char* new_imei)
{
  if (strnlen(new_imei, IDD_MAX_DEVICE_IMEI_SIZE) < IDD_MIN_DEVICE_IMEI_LEN)
  {
    IDD_LOGE("To short IMEI received : <%.*s>", (int)strlen(new_imei), new_imei);
    return;
  }

  if (strncmp(g_device_imei, new_imei, IDD_MAX_DEVICE_IMEI_SIZE))
  {
    if (strncmp(g_device_imei, IMEI_UNKNOWN, IDD_MAX_DEVICE_IMEI_SIZE))
    {
      IDD_LOGE("IMEI not supposed to change: old  <%.*s>, new  <%.*s>",
          IDD_MAX_DEVICE_IMEI_SIZE, g_device_imei, IDD_MAX_DEVICE_IMEI_SIZE, new_imei);
    }
    strncpy(g_device_imei, new_imei, IDD_MAX_DEVICE_IMEI_SIZE);
    create_report_header();
  }
}

/*
 * Handle an incoming ANDROID_ID owned by IDD agent.
 */
void android_id_handle(const char* new_android_id)
{
  if (strncmp(g_android_id, new_android_id, IDD_MAX_ANDROID_ID_SIZE))
  {
    if (strncmp(g_android_id, ANDROID_ID_UNKNOWN, IDD_MAX_ANDROID_ID_SIZE))
    {
      IDD_LOGI("IDD agent's ANDROID_ID changed: old  <%.*s>, new  <%.*s>",
          IDD_MAX_ANDROID_ID_SIZE, g_android_id, IDD_MAX_ANDROID_ID_SIZE, new_android_id);
    }
    strncpy(g_android_id, new_android_id, IDD_MAX_ANDROID_ID_SIZE);
    create_report_header();
  }
}

/*
 * Handle an incoming ANDROID_ID owned by Xperia services.
 */
void xs_android_id_handle(const char* new_xs_android_id)
{
  if (strncmp(g_xs_android_id, new_xs_android_id, IDD_MAX_XS_ANDROID_ID_SIZE))
  {
    if (strncmp(g_xs_android_id, XS_ANDROID_ID_UNKNOWN, IDD_MAX_XS_ANDROID_ID_SIZE))
    {
      IDD_LOGI("Xperia services' ANDROID_ID changed: old  <%.*s>, new  <%.*s>",
          IDD_MAX_XS_ANDROID_ID_SIZE, g_xs_android_id, IDD_MAX_XS_ANDROID_ID_SIZE,
          new_xs_android_id);
    }
    strncpy(g_xs_android_id, new_xs_android_id, IDD_MAX_XS_ANDROID_ID_SIZE);
    create_report_header();
  }
}

/*
 * Handle an incoming GUID owned by Xperia services.
 */
void xs_guid_handle(const char* new_xs_guid)
{
  if (strncmp(g_xs_guid, new_xs_guid, IDD_MAX_XS_GUID_SIZE))
  {
    if (strncmp(g_xs_guid, XS_GUID_UNKNOWN, IDD_MAX_XS_GUID_SIZE))
    {
      IDD_LOGI("Xperia services' GUID changed: old  <%.*s>, new  <%.*s>",
          IDD_MAX_XS_GUID_SIZE, g_xs_guid, IDD_MAX_XS_GUID_SIZE,
          new_xs_guid);
    }
    strncpy(g_xs_guid, new_xs_guid, IDD_MAX_XS_GUID_SIZE);
    create_report_header();
  }
}

unsigned char* read_entire_file(FILE* fp, size_t* size_p)
{
  unsigned char* buf_p = NULL;

  if (fp != NULL)
  {
    if (get_file_size(fp, size_p) == 0)
    {
      buf_p = malloc(*size_p);
      if (buf_p != NULL)
      {
        if (fread((char*)buf_p, 1, *size_p, fp) != *size_p)
        {
          free(buf_p);
          buf_p = NULL;
        }
      }
    }
  }
  return buf_p;
}

int should_exit(void)
{
  return should_exit_flag;
}

void set_should_exit(void)
{
  // NOTE: This may execute from within a signal handler, so we
  // don't want to log, or do any other syscall.
  should_exit_flag = 1;
}

void run_kill_switch(void)
{
  if (access(KILL_SWITCH_FNAME, F_OK) == 0)
  {
    IDD_LOGE("IDD will disable itself.");
    // Note: We may still get SIGTERM which should make us
    // leave this loop
    while (!should_exit())
    {
      struct timeval timeout;
      // Delay for (at most) a day. Then do it again.
      timeout.tv_sec = 24 * 3600;
      timeout.tv_usec = 0;
      select(0, NULL, NULL, NULL, &timeout);
    }
  }
}

/*
 * See header file for documentation.
 */
ssize_t write_all(int fd, const void* buffer_p, size_t size)
{
  const char* buf_p = buffer_p;
  ssize_t total_written = 0;

  while (size > 0)
  {
    ssize_t written = write(fd, buf_p, size);
    if (written < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }
      return -1;
    }
    if (written == 0)
    {
      // This is not meant to happen, prevents eternal loop.
      errno = ENOSPC;
      return -1;
    }
    size -= written;
    buf_p += written;
    total_written += written;
  }

  return total_written;
}

/*
 * Parse proto-buf data to int.
*/
static int32_t unpack_int32 (const char *data, int32_t* value)
{
  int32_t length = 0;
  for (; length < 5; length++)
  {
    if ((data[length] & 0x80) == 0)
    {
      break;
    }
  }

  length += 1;
  int32_t v = data[0] & 0x7f;
  if (length > 1)
  {
    v |= ((data[1] & 0x7f) << 7);
    if (length > 2)
    {
      v |= ((data[2] & 0x7f) << 14);
      if (length > 3)
      {
        v |= ((data[3] & 0x7f) << 21);
        if (length > 4)
          v |= (data[4] << 28);
      }
    }
  }
  *value = v;
  return length;
}

/*
 * Check the beginning from cur_pos section data.
 *
 * @return 0 if the section is good.
 */
static int check_protobuf_section(FILE* fp,
                           size_t total_size,
                           size_t* cur_pos,
                           char expected_tag,
                           Idd_Report_t type)
{
  size_t current_pos  = *cur_pos;
  int32_t data_size = 0;
  int32_t section_size = 0;
  int field_len = 0;

  char buffer[FILE_BUFF_SIZE] = {0};
  size_t bytes_read   = 0;

  if ((bytes_read = fread((char*)buffer, 1, sizeof(buffer), fp)) <= 0)
  {
    IDD_LOGE("Failed to read file: %s", strerror(errno));
    return -1;
  }

  if (bytes_read < sizeof(buffer))
  {
    return -1;
  }

  if (buffer[0] != expected_tag)
  {
    if ((type == EVENTS_FILE) &&
        (expected_tag == REPORT_HEADER_TAG) &&
        (buffer[0] == REPORT_EVENT_TAG))
    {
      // Events files can miss header on prototypes who don't get imei
      // Rewind file and look for events instead
      if (fseek(fp, 0, SEEK_SET) == 0)
      {
        return 0;
      }
      IDD_LOGE("Failed to rewind events file <%s>", strerror(errno));
    }

    IDD_LOGE("Expected tag %d, found %d", expected_tag, buffer[0]);
    return -1;
  }

  /*
   * The section size is stored after tag(1 byte).
   */
  field_len = unpack_int32(buffer + PROTO_BUF_TAG_SIZE, &data_size);

  section_size = PROTO_BUF_TAG_SIZE + field_len + data_size;
  current_pos += section_size;
  if (data_size < 0 || current_pos > total_size)
  {
    return -1;
  }

  /*
   * Goto the beginning of next section.
   */
  if (fseek(fp, section_size - bytes_read, SEEK_CUR) != 0)
  {
    IDD_LOGE("Failed to fseek: %s", strerror(errno));
    return -1;
  }

  *cur_pos = current_pos;

  return 0;
}

/*
 * Check the report header.
 */
static int check_report_header(FILE* fp, size_t file_size, size_t* cur_pos, Idd_Report_t type)
{
  return check_protobuf_section(fp, file_size, cur_pos, REPORT_HEADER_TAG, type);
}

/*
 * Check the event.
 */
static int check_event(FILE* fp, size_t file_size, size_t* cur_pos, Idd_Report_t type)
{
  return check_protobuf_section(fp, file_size, cur_pos, REPORT_EVENT_TAG, type);
}

/*
 *
 */
void rescue_report(char* filename, Idd_Report_t type)
{
  FILE* fp = fopen(filename, "rb");
  if (fp == NULL)
  {
    if (errno != ENOENT)
    {
      IDD_LOGE("Failed to open event file <%s> to rescue: %s", filename, strerror(errno));
    }
    return;
  }

  size_t file_size = 0;
  size_t cur_pos  = 0;
  Idd_Report_Status status = REPORT_STATUS_OK;

  /*
   * If report header is corrupt, then delete the event file.
   * If some event is corrupt, then remove this event, and this
   * event must be the last.
   */

  if (get_file_size(fp, &file_size) == 0)
  {
    if ((type != REPORT_WITHOUT_HEADER) &&
        (check_report_header(fp, file_size, &cur_pos, type) == -1))
    {
      IDD_LOGE("File deleted (%s). Header is corrupt", filename);
      status = REPORT_HEADER_CORRUPT;
    }
    else
    {
      while (cur_pos < file_size)
      {
        if (check_event(fp, file_size, &cur_pos, type) == -1)
        {
          status = REPORT_EVENT_CORRUPT;
          break;
        }
      }
    }
  }
  else
  {
    IDD_LOGE("File deleted (%s). Failed to get event file size", filename);
    status = REPORT_HEADER_CORRUPT;
  }

  fclose(fp);

  switch (status)
  {
  case REPORT_HEADER_CORRUPT:
    unlink(filename);
    break;

  case REPORT_EVENT_CORRUPT:
    if (truncate(filename, cur_pos))
    {
      IDD_LOGE("File deleted (%s). Failed to truncate event at %zu :%s",
                filename, cur_pos, strerror(errno));
      unlink(filename);
    }
    else
    {
      IDD_LOGE("File truncated (%s). Corrupt event at %zu", filename, cur_pos);
    }
    break;

  default:
    IDD_LOGD("Event file <%s> is OK", filename);
  }
}

/*
 * Generate device id.
 */
void generate_device_id(int force)
{
  if (force || !strncmp(g_device_id, HEADER_STRING_UNKNOWN, IDD_MAX_DEVICE_ID_SIZE))
  {
    FILE* fp = fopen(UUID_FNAME, "r");
    if (fp != NULL)
    {
      char buffer[IDD_MAX_DEVICE_ID_SIZE] = {0};
      if (fgets(buffer, sizeof(buffer), fp) != NULL)
      {
        // remove newline added by fgets
        buffer[strnlen(buffer, IDD_MAX_DEVICE_ID_SIZE) - 1] = '\0';
        strncpy(g_device_id, buffer, IDD_MAX_DEVICE_ID_SIZE);
        IDD_LOGD("New device_id <%s>.", g_device_id);
      }
      fclose(fp);
    }
    else
    {
      IDD_LOGE("Failed to open uuid file: %s", strerror(errno));
    }
  }

  if ((g_device_id_cr[0] == '\0') &&
      strncmp(g_device_id, HEADER_STRING_UNKNOWN, IDD_MAX_DEVICE_ID_SIZE))
  {
    strncpy(g_device_id_cr, g_device_id, IDD_MAX_DEVICE_ID_SIZE);
    IDD_LOGD("Setting device_id for request <%s>.", g_device_id_cr);
  }
}

void generate_rca_device_id()
{
  if (!strncmp(g_rca_device_id, HEADER_STRING_UNKNOWN, IDD_MAX_DEVICE_ID_SIZE))
  {
    FILE *fp = fopen(UUID_FNAME, "r");
    if (fp != NULL)
    {
      char buffer[IDD_MAX_DEVICE_ID_SIZE] = {};
      if (fgets(buffer, sizeof(buffer), fp) != NULL)
      {
        // remove newline added by fgets
        buffer[strnlen(buffer, IDD_MAX_DEVICE_ID_SIZE) - 1] = '\0';
        strncpy(g_rca_device_id, buffer, IDD_MAX_DEVICE_ID_SIZE);
        IDD_LOGD("New rca_device_id <%s>.", g_rca_device_id);
      }
      fclose(fp);
    }
    else
    {
      IDD_LOGE("Failed to open uuid file for generating rca_device_id: %s", strerror(errno));
    }
  }
}

void get_device_id(char* device_id_p)
{
  if (device_id_p != NULL)
  {
      memset(device_id_p, '\0', IDD_MAX_DEVICE_ID_SIZE);
      strncpy(device_id_p, g_device_id, IDD_MAX_DEVICE_ID_SIZE);
  }
}

char* get_device_id_cr()
{
  if (g_device_id_cr[0] == '\0')
  {
    generate_device_id(0);
  }
  return &g_device_id_cr[0];
}

/*
 * Generate session id.
 */
void generate_session_id(void)
{
  uint32_t s1boot_counter;
  uint32_t size = sizeof(s1boot_counter);
  miscta_status_t ta_result;

  ta_result = miscta_read_unit(TA_S1_BOOT_COUNTER, &s1boot_counter, &size);
  if (ta_result == MT_SUCCESS && (size != 0))
  {
    g_session_id = (uint64_t)s1boot_counter;
    IDD_LOGD("Session ID has been generated with s1 boot counter.");
  }
  else
  {
    int fd = open("/dev/urandom", O_RDONLY);
    size_t result = 0;

    if (fd != -1)
    { // read on special devices like /dev/urandom can be interrupted. Hence, a while loop.
      do
      {
        result = read(fd, &header.session_id, sizeof(int));
      } while(errno == EINTR || result < sizeof(int));

      close(fd);

      IDD_LOGE("Could not read s1 boot counter. Code: %u.\n"
               "Session ID has been generated with random.", ta_result);
    }
    else
    {
      IDD_LOGE("Could not open /dev/urandom. Session id generation failed");
    }
  }

  IDD_LOGD("session_id = %"PRIu64"", header.session_id);
}

/*
 *
 */
int remove_directory(const char *path, int remove_dir)
{
  DIR *d = opendir(path);
  size_t path_len = strlen(path);
  int r = -1;

  if (d)
  {
    struct dirent *p;
    r = 0;
    while (!r && (p = readdir(d)) != NULL)
    {
      int r2 = -1;
      /* Skip the names "." and ".." as we don't want to recurse on them. */
      if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
      {
        continue;
      }

      size_t len = path_len + strlen(p->d_name) + 2;
      char *buf = malloc(len);
      if (buf)
      {
        struct stat statbuf;
        snprintf(buf, len, "%s/%s", path, p->d_name);
        if (!stat(buf, &statbuf))
        {
          if (S_ISDIR(statbuf.st_mode))
          {
            r2 = remove_directory(buf, 1);
          }
          else
          {
            r2 = unlink(buf);
          }
        }
        free(buf);
      }
      r = r2;
    }
    closedir(d);
  }

  if (!r && remove_dir)
  {
    r = rmdir(path);
  }
  return r;
}

/*
 *
 */
bool do_washing(void)
{
  bool wash;
  wash_config_t wash_settings = settings_wash_data_get();
  if (wash_settings != WASH_CONFIG_UNDEFINED)
  {
    wash = (wash_settings == WASH_CONFIG_TRUE ? true : false);
  }
  else
  {
    wash = (debug_mode(header.build_type) ? false : true);
  }
  return wash;
}

/*
 * Return true if user consent is not all and configuration is not for user consent with all.
 */
bool needs_save_anonymous_config(void)
{
  Iddd__UserConsent__Type required_user_consent = settings_required_user_consent_get();
  return (g_user_consent < IDD__USER_CONSENT__ALL &&
          required_user_consent < IDDD__USER_CONSENT__TYPE__ALL);
}
/*
 * Return true if user consent is matched with the one required by configuration.
 */
bool is_required_user_consent_matched(void)
{
  bool matched = false;
  if (debug_mode(header.build_type))
  {
    matched = true;
  }
  else
  {
    Iddd__UserConsent__Type required_user_consent = settings_required_user_consent_get();
    IDD_LOGD("iddd.Settings.required_consent = %d", required_user_consent);
    matched = (g_user_consent == required_user_consent ? true : false);
  }
  IDD_LOGD("Required user consent is = %s", matched == true ? "matched" : "mismatched");
  return matched;
}

/*
 * Read out string from miscTA with making null-terminated.
 * Returns 1 on success, otherwise 0.
 */
static int read_miscta_string(uint32_t id, void *buf, uint32_t *size)
{
  miscta_status_t rval = -1;

  if (buf != NULL)
  {
    memset(buf, '\0', *size);
    rval = miscta_read_unit(id, buf, size);
  }

  return rval == MT_SUCCESS ? 1 : 0;
}

static void get_build_type(char* build_type)
{
  property_get("ro.vendor.build.type", build_type, HEADER_STRING_UNKNOWN);
}

/*
 * Return true if build is user variant.
 */
bool is_user_variant(void)
{
  char build_type[HEADER_STRING_MAX] = HEADER_STRING_UNKNOWN;
  get_build_type(build_type);

  return strncmp(build_type, "user", HEADER_STRING_MAX) == 0;
}

/*
 * Return true if iddd.state has build type info.
 */
bool get_last_build_type(char* build_type)
{
  FILE* file_p = fopen(IDD_STATE_FNAME, "r");
  if (file_p == NULL) return false;

  char buffer[256] = {0};
  bool ret = false;
  while (fgets(buffer, sizeof(buffer), file_p) != NULL)
  {
    if (sscanf(buffer, IDD_STATE_TAG_BUILD_TYPE "%s", build_type))
    {
      ret = true;
      break;
    }
  }
  fclose(file_p);
  return ret;
}

/*
 * Return true if build type is changed.
 */
bool is_build_type_changed()
{
  char last_build_type[HEADER_STRING_MAX] = HEADER_STRING_UNKNOWN;
  char build_type[HEADER_STRING_MAX] = HEADER_STRING_UNKNOWN;

  if (!get_last_build_type(last_build_type)) return false;
  get_build_type(build_type);
  return strncmp(build_type, last_build_type, HEADER_STRING_MAX) != 0;
}

/*
 * Set device initial data collection value.
 * It will be overwritten as user setting value by state_load()
 */
void set_initial_data_collection_enabled(void)
{
  if (is_user_variant())
  {
    set_data_collection_enabled(0); // for user build
  }
  else
  {
    set_data_collection_enabled(1); // for userdebug build
  }
}

/*
 */
void handle_prototype_setting(void)
{
  // if market device, return immediately
  if (is_user_variant() && is_secure_mode_live()) return;
  set_data_collection_enabled(1);
  set_user_consent(IDD__USER_CONSENT__ALL);
}

bool get_idd_report_stats(int* oldest_file_nr, int* newest_file_nr, int* total_file_nr, size_t* sum_size)
{
  statdir output_statdir = { 0, INT_MAX, 0, 0 };
  if (get_statdir(OUTPUT_DIR_NAME, &output_statdir) == -1) return false;
  *oldest_file_nr = output_statdir.oldest_file_nr;
  *newest_file_nr = output_statdir.newest_file_nr;
  *total_file_nr = output_statdir.total_file_nr;
  *sum_size = output_statdir.sum_size;
  return true;
}
