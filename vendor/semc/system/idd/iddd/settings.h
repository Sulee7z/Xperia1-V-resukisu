/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief iddd settings
 *
 * @author Leif Mårtensson (leif.martensson@sonyericsson.com)
 */
#ifndef IDDD_SETTINGS_H_INCLUSION_GUARD
#define IDDD_SETTINGS_H_INCLUSION_GUARD

#include <stdbool.h>
#include "protos/idd/iddd.pb-c.h"

/*
 * The package and name for iddd settings as specified in the probe definition file
 */
#define SETTINGS_PROBE_PACKAGE "iddd"
#define SETTINGS_PROBE_NAME    "Settings"

typedef enum {
  WASH_CONFIG_FALSE = 0,
  WASH_CONFIG_TRUE = 1,
  WASH_CONFIG_UNDEFINED = 2
} wash_config_t;
/*
 * Initialize settings. All settings are set to default values.
 */
void settings_init();

/*
 * Set new max size of a report file.
 */
void settings_report_max_size_set(size_t new_report_max_size);

/*
 * Return the max size of a report file.
 */
size_t settings_report_max_size_get();

/*
 * Set new max size of the output directory.
 */
void settings_output_dir_max_size_set(size_t new_output_dir_max_size);

/*
 * Return the max size of the output directory.
 */
size_t settings_output_dir_max_size_get();

/* Parses new iddd settings from configuration file, if one is present.
 */
void settings_load_configuration(void);

/*
 * Number of seconds when untouched counters shall be removed.
 * 0 to indicate that they never expire.
 */
void settings_expire_counters_set(size_t new_time);

/*
 * Returns when untouched counters should expire
 */
size_t settings_expire_counters_get(void);

/*
 * Number of seconds when untouched static data shall be removed.
 * 0 to indicate that they never expire.
 */
void settings_expire_sdata_set(size_t new_time);

/*
 * Returns when untouched sdata should expire
 */
size_t settings_expire_sdata_get(void);

/*
 * Sets the default value for counters expire age. This value will be
 * used if no counters expire age has been specified by the latest
 * configuration or calls to settings_expire_counters_set(size_t new_time).
 */
void settings_default_expire_counters_set(size_t time);

/*
 * Sets the default value for static data expire age. This value will be
 * used if no static data expire age has been specified by the latest
 * configuration or calls to settings_expire_sdata_set(size_t new_time).
 */
void settings_default_expire_sdata_set(size_t time);

/*
 * Set wash personal data status.
 */
void settings_wash_data_set(wash_config_t new_wash_data);

/*
 * Returns the value of the IDD configuration field iddd.Settings.wash_data.
 * If missing in configuration, WASH_CONFIG_UNDEFINED is returned.
 */
wash_config_t settings_wash_data_get(void);

/*
 * Set use imei status.
 */
void settings_use_imei_set(bool use_imei);

/*
 * Returns the value of the IDD configuration field iddd.Settings.use_imei.
 */
bool settings_use_imei_get(void);

/*
 * Set use android id status.
 */
void settings_use_android_id_set(bool use_android_id);

/*
 * Returns the value of the IDD configuration field iddd.Settings.use_android_id.
 */
bool settings_use_android_id_get(void);

/*
 * Set use xs android id status.
 */
void settings_use_xs_android_id_set(bool use_xs_android_id);

/*
 * Returns the value of the IDD configuration field iddd.Settings.use_xs_android_id.
 */
bool settings_use_xs_android_id_get(void);

/*
 * Set use xs guid status.
 */
void settings_use_xs_guid_set(bool use_xs_guid);

/*
 * Returns the value of the IDD configuration field iddd.Settings.use_xs_guid.
 */
bool settings_use_xs_guid_get(void);

/*
 * Set required user consent status.
 */
void settings_required_user_consent_set(Iddd__UserConsent__Type new_wash_data);

/*
 * Returns the value of the IDD configuration field idd.Settings.required_user_consent.
 */
Iddd__UserConsent__Type settings_required_user_consent_get(void);

#endif
