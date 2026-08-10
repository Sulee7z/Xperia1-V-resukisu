/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief C API for accessing the IDD Daemon private parts
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#ifndef IDD_PRIVATE_API_H_INCLUSION_GUARD
#define IDD_PRIVATE_API_H_INCLUSION_GUARD

#include "idd_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IDD_NO_FILE (-99942)
#define IDD_NO_DATA (-99943)

#define IDD_SEC_LIVE    'L'
#define IDD_SEC_DEBUG   'D'
#define IDD_SEC_UNKNOWN 'U'

/*
 * Low-level event dispatching - for use by JNI.
 */
void* idd_ll_create_event(IDD_t* idd, const char* package_p, const char* name_p, size_t payload_size);
int idd_ll_dispatch_event(IDD_t* idd, size_t payload_size); // Returns 0 if successful, < 0 if not

/*
 * Flush all data and generate a report in the output directory.
 * Returns 0 if successful, < 0 if not.
 */
int idd_flush(IDD_t* idd);

/*
 * Send raw commands to the IDD Daemon. Returns 0 if successful, < 0
 * if not.
 */
int idd_send_raw(IDD_t* idd, const char* raw_p, size_t raw_size);

/*
 * Receive IMEI from outside.This is because we can not get it from C
 * code but from Java code. Returns 0 is successful, < 0 if not.
 */
int idd_set_imei(IDD_t* idd, const char* imei_p);

/*
 * Receive ANDROID_ID owned by IDD agent.
 * Returns 0 is successful, < 0 if not.
 */
int idd_set_android_id(IDD_t* idd, const char* android_id_p);

/*
 * Receive ANDROID_ID owned by Xperia services.
 * Returns 0 is successful, < 0 if not.
 */
int idd_set_xs_android_id(IDD_t* idd, const char* xs_android_id_p);

/*
 * Receive GUID owned by Xperia services.
 * Returns 0 is successful, < 0 if not.
 */
int idd_set_xs_guid(IDD_t* idd, const char* xs_guid_p);

/*
 * Receive user consent level.
 * Returns 0 if successful, < 0 if not.
 */
int idd_set_user_consent(IDD_t* idd, int user_consent);

/*
 * Notify IDD about a new configuration file.  The Daemon will then
 * make the new config packages available.
 *
 * The configuration must be located in the incoming directory,
 * e.g. idd base directory + "incoming/" and named "config". Returns 0 if
 * successful, < 0 if not.
 */
int idd_notify_config(IDD_t* idd);

/*
 * Force save the static data and counter.
 * Returns 0 if successful, < 0 if not.
 */
int idd_force_save(IDD_t* idd);

/*
 * Receive serial number from outside.We can not get it from C
 * code because only root can invoke libmiscta interface.
 * Returns 0 is successful, < 0 if not.
 */
int idd_set_serial_number(IDD_t* idd, const char* serial_number_p);

/*
 * Set data collection enable or disable.
 * @param enable: 1 is enable, 0 is disable
 * Returns 0 if successful, < 0 if not.
 */
int idd_set_data_collection_enable(IDD_t* idd, int enable);

/* Remove report.
 *
 * @param report_num: 0 is remove all reports, positive is specified report.
 * Returns 0 if successful, < 0 if not.
 */
int idd_remove_report(IDD_t* idd, int report_num);

/* Get device id
 * Returns 0 if successful, < 0 if not.
 */
int idd_get_device_id(IDD_t* idd, char* device_id);

/*
 * Send IDD configuration to the deamon.
 * Returns 0 if successful, < 0 if not.
 */
int idd_write_config(IDD_t* idd, const char* config_p, size_t config_size);

/* Get configuration request data
 *
 * The format of the data is in a ConfigRequest protobuf (See configuration.proto).
 *
 * @param cr: A pointer where the ConfigRequest will be put.
 * @param cr_size: The size of cr.
 * Returns the size of the ConfigRequest (cr), < 0 on errors.
 */
int idd_config_request_get(IDD_t* idd, char* cr, int cr_size);

/*
 * Set timezone.
 * Returns 0 if successful, < 0 if not.
 */
int idd_set_timezone(IDD_t* idd);

/* Get App Event Enabled Packages.
 * Returns 0 if successful, < 0 if not.
 */
int idd_get_app_event_enabled_packages(IDD_t* idd, char* registered_pkg);

int idd_get_config_path(IDD_t* idd, const char* package_p, const char* name_p, char* out_path);

int idd_get_report_stats(IDD_t* idd, int* oldest_file_nr, int* newest_file_nr, int* total_file_nr, size_t* sum_size);

#ifdef __cplusplus
}
#endif

#endif

