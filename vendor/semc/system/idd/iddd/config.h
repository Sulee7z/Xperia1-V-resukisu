/*
 * Copyright 2012 Sony Corporation
 */

/**
 * @brief Static configuration for IDD Daemon
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#ifndef CONFIG_H_INCLUSION_GUARD
#define CONFIG_H_INCLUSION_GUARD

#include <sys/stat.h>
#include "idd_config.h"

/* The mode of the unix domain sockets */
#define SOCKET_MODE (S_IRUSR | S_IWUSR | S_IWGRP | S_IWOTH)

/* The file creation mask */
#define FILE_CREATION_MASK (S_IWGRP | S_IWOTH)

/* Debug Interface, incoming TCP port number */
#define DEBUG_INTERFACE_PORT (2305)

/* The read-protected directory, and its mode. */
#define PRIVATE_DIR_NAME "private/"
#define PRIVATE_DIR_MODE (S_IRWXU)

/* Event filename, and maximum size until a flush is forced */
#define PARTIAL_EVENTS_FNAME PRIVATE_DIR_NAME "events"

/* Default maximum size of a report
 * The size can be overridden by iddd configuration download */
#define REPORT_MAX_SIZE_DEFAULT (400*1024)

/* Counters filename */
#define COUNTERS_TXT_FNAME PRIVATE_DIR_NAME "counters.txt"

/* Static data filename */
#define SDATA_TXT_FNAME PRIVATE_DIR_NAME "sdata.txt"

/* Configuration List filename */
#define CONFIG_LIST_FNAME PRIVATE_DIR_NAME "config_list.txt"

/* Events filters list filename */
#define EVENTS_FILTER_LIST_FNAME PRIVATE_DIR_NAME "filters.txt"

/* Counters and staticdata filters list filename */
#define COUNTERS_FILTER_LIST_FNAME PRIVATE_DIR_NAME "filters_cnt.txt"

/* Counters and staticdata filters list filename */
#define STATICDATA_FILTER_LIST_FNAME PRIVATE_DIR_NAME "filters_sd.txt"

/* The different configuration parts */
#define CONFIG_PART_FNAME PRIVATE_DIR_NAME "conf_%d.bin"

/* The registered apps data list filename */
#define REGISTERED_APP_PACKAGES_FNAME PRIVATE_DIR_NAME "registered_app_packages_list.txt"

/* The filename in which events, counters and such will be placed before compression */
#define FLUSHED_DATA_FNAME PRIVATE_DIR_NAME "flushed_data"

/* The filename in which registered apps data list will be temporarily placed */
#define FLUSHED_PKG_DATA_FNAME PRIVATE_DIR_NAME "flushed_pkg_data"

/* The temporary filename used by the compactor as result */
#define COMPACTED_DATA_FNAME FLUSHED_DATA_FNAME ".compacted"

/* The temporary filename used by the compactor when encrypting */
#define ENCRYPTED_DATA_FNAME FLUSHED_DATA_FNAME ".encrypted"

/* The output directory and its mode. */
#define OUTPUT_DIR_NAME "output/"
#define OUTPUT_DIR_MODE (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)

/* The total file size in the iddd output dir must not exceed
 * OUTPUT_DIR_MAX_SIZE_DEFAULT - REPORT_MAX_SIZE_DEFAULT when a new report shall be created.
 * If so old reports will be deleted.
 * The size can be overriden by iddd configuration download */
#define OUTPUT_DIR_MAX_SIZE_DEFAULT (20*1024*1024)

/* The input directory and its mode. */
#define INPUT_DIR_NAME "input/"
#define INPUT_DIR_MODE (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)

/* Configuration file names */
#define PUBLIC_CONFIG_FNAME INPUT_DIR_NAME "config"
#define PRIVATE_CONFIG_FNAME PRIVATE_DIR_NAME "new_config.gz"
#define PRIVATE_CONFIG_SIG_FNAME PRIVATE_DIR_NAME "new_config.sig"
#define PRIVATE_CONFIGS_BACKUP_DIR_NAME PRIVATE_DIR_NAME "configs_backup/"
#define PRIVATE_CONFIG_ANONYMOUS_FNAME PRIVATE_CONFIGS_BACKUP_DIR_NAME "config_anonymous"
#define PRIVATE_CONFIG_RECOVERY_FNAME PRIVATE_CONFIGS_BACKUP_DIR_NAME "config_recovery"

/* The location of the default daemon configuration file */
#define DEFAULT_DAEMON_USER_CONFIG_FNAME "/vendor/etc/iddd_user.conf"
#define DEFAULT_DAEMON_DEBUG_CONFIG_FNAME "/vendor/etc/iddd_debug.conf"

/* The maximum size of an incoming configuration */
#define MAX_CONFIG_SIZE (200*1024)

/* Filename suffix for temporary files */
#define TEMPORARY_FILE_SUFFIX "_"

/* The lock file used by IDD Daemon */
#define PID_FILE "iddd.pid"

/* Max number of characters needed to represent a uint64 */
#define MAX_NUMBER 30

/* Maximum number of group ids a report header can be stored*/
#define MAX_GROUP_ID_NUMBER 6

/* Default interval in seconds between save to file of counters and staticdata */
#define IDD_DEFAULT_SAVE_INTERVAL (10*60)

/* Expire static data and counters that haven't been touched for XX seconds */
#define IDD_DEFAULT_EXPIRE_COUNTERS_AGE (30*24*3600)
#define IDD_DEFAULT_EXPIRE_SDATA_AGE (30*24*3600)

/* The settings tags for expire counters and static data */
#define IDD_STATE_TAG_EXPIRE_COUNTERS "expire-counters: "
#define IDD_STATE_TAG_EXPIRE_SDATA    "expire-sdata: "

/* This file contains data that has to be persistent */
#define IDD_STATE_FNAME PRIVATE_DIR_NAME "iddd.state"

/* The timestamp of the last flush. Used for only sending updated data */
#define IDD_STATE_TAG_LAST_FLUSH "last-flush: "

/* This is the largest allowed report size */
#define IDD_STATE_TAG_REPORT_MAX_SIZE "report-max-size: "

/* This is the maximum size of the output directory. */
#define IDD_STATE_TAG_OUTPUT_DIR_MAX_SIZE "output-dir-max-size: "

/* This is the time when we shut down iddd last time. It's used for
 * restoring the time when running in test-mode where we have a time
 * source that we control completely - and used for producing
 * deterministic results.
 */
#define IDD_STATE_TAG_SHUTDOWN_TIME "shutdown-time: "

/* The SHA1 of the current downloaded configuration file */
#define IDD_STATE_TAG_CONFIG_CHECKSUM "config-checksum: "

/* The device id*/
#define IDD_STATE_TAG_DEVICE_ID "device_id: "
#define IDD_STATE_TAG_DEVICE_ID_CR "device_id_cr: "

/* The rca device id */
#define IDD_STATE_TAG_RCA_DEVICE_ID "rca_device_id: "

/* The session id */
#define IDD_STATE_TAG_SESSION_ID "session_id: "

/* The flag of data collection is enabled or disabled */
#define IDD_STATE_TAG_DATA_COLLECTION "data-collection: "

/* The header group id */
#define IDD_STATE_TAG_GROUP_ID "group_id: "

/* Build information */
#define IDD_STATE_TAG_PRODUCT "product: "
#define IDD_STATE_TAG_SW_VERSION "sw_version: "
#define IDD_STATE_TAG_VARIANT "variant: "
#define IDD_STATE_TAG_BUILD_TYPE "build_type: "
#define IDD_STATE_TAG_CDF_ID "cdf_id: "
#define IDD_STATE_TAG_SERIAL_NUMBER "serial_num: "
#define IDD_STATE_TAG_SECURE_MODE "secure_mode: "

/* Imei */
#define IDD_STATE_TAG_USE_IMEI "use_imei: "
#define IDD_STATE_TAG_DEVICE_IMEI "device_imei: "

/* Timezone */
#define IDD_STATE_TAG_TIMEZONE "timezone: "

/* Daemon error number since timestamp */
#define IDD_STATE_TAG_ERROR_NR "error_nr: "
#define IDD_STATE_TAG_ERROR_TIMESTAMP "error_time: "

/* The wash personal data tag */
#define IDD_STATE_TAG_WASH_DATA_ID "wash_data: "

/* ANDROID_ID owned by IDD agent */
#define IDD_STATE_TAG_USE_ANDROID_ID "use_android_id: "
#define IDD_STATE_TAG_ANDROID_ID "android_id: "

/* ANDROID_ID owned by Xperia services*/
#define IDD_STATE_TAG_USE_XS_ANDROID_ID "use_xs_android_id: "
#define IDD_STATE_TAG_XS_ANDROID_ID "xs_android_id: "

/* GUID (user ID) owned by Xperia services */
#define IDD_STATE_TAG_USE_XS_GUID "use_xs_guid: "
#define IDD_STATE_TAG_XS_GUID "xs_guid: "

/* The internal product name */
#define IDD_STATE_TAG_INTERNAL_PRODUCT_NAME "internal_product_name: "

/* The internal product name */
#define IDD_STATE_TAG_MARKET_MODEL_NAME "market_model_name: "

/* The internal product name */
#define IDD_STATE_TAG_MARKET_PRODUCT_NAME "market_product_name: "

/* The internal product name */
#define IDD_STATE_TAG_OS_VERSION "os_version: "

/* The internal product name */
#define IDD_STATE_TAG_KERNEL_VERSION "kernel_version: "

/* The sony product code */
#define IDD_STATE_TAG_SONY_PRODUCT_CODE "sony_product_code: "

/* The customer id */
#define IDD_STATE_TAG_CUSTOMER_ID "customer_id: "

/* The subsystem id */
#define IDD_STATE_TAG_SUBSYSTEM_VERSION "subsystem_version: "

/* The user consent level */
#define IDD_STATE_TAG_USER_CONSENT "user_consent: "
#define IDD_STATE_TAG_REQUIRED_USER_CONSENT "required_user_consent: "

/* How the list implementation for counters behaves */
#define COUNTER_LIST_INITIAL_SIZE 256
#define COUNTER_LIST_GROW_BY 128

/* How the list implementation for static data behaves */
#define SDATA_LIST_INITIAL_SIZE 256
#define SDATA_LIST_GROW_BY 128

/* How the list implementation for configurations behaves */
#define CONFIG_LIST_INITIAL_SIZE 32
#define CONFIG_LIST_GROW_BY 16

/* How the list implementation for filters behaves */
#define FILTER_LIST_INITIAL_SIZE 256
#define FILTER_LIST_GROW_BY 128

/* The intent we send to the Java clients on configuration updates. */
#define CONFIGURATION_UPDATED_INTENT "com.sonyericsson.idd.CONFIG_UPDATED"

/* Base path to public keys */
#define PUBLIC_KEY_DIR "/vendor/etc/security/"

/* Public verification key file name */
#define CONF_PUBLIC_KEY_FNAME "idd_config.pem"

/* Public encryption key file name */
#define REPORT_PUBLIC_KEY_FNAME "idd_report.pem"

/* Path the kill switch file, that is only present when IDD should be disabled */
#define KILL_SWITCH_FNAME OUTPUT_DIR_NAME "idd_disabled"

/* Argument to turn off encryption and verification */
#define NO_ENCRYPTION_ARGUMENT "NO_ENCRYPTION"

/* uuid file name*/
#define UUID_FNAME "/proc/sys/kernel/random/uuid"

/* Java package name for android probes that have 'package idd' statement in its proto files */
#define PACKAGE_PROBE_ANDROID_IDD "com.sonyericsson.idd.probe.android.idd"

/* AppEvent probe name */
#define PROBE_APP_EVENT "AppEvent"

/* Android permissions for calling IDD API */
#define PERMISSION_IDD_PRIVATE "com.sonyericsson.permission.IDD_PRIVATE"

#endif
