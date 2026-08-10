/*
 * Copyright 2014 Sony Corporation
 */

/**
 * @brief IDD plugins
 *
 * @author Leif Mårtensson (leif.martensson@sonymobile.com)
 */

#ifndef IDDD_PLUGINS_H_INCLUSION_GUARD
#define IDDD_PLUGINS_H_INCLUSION_GUARD

#define ALL_PLUGINS (-1)
/**
 * Create/removes plugin output directories based on plugin configuration.
 */
void plugins_load_configuration();

/**
 * @return 1 if the given probe is enabled in any IDD plugin, otherwise 0.
 */
int plugins_filter_is_probe_active(const char* package_p, const char* name_p);


/**
 * Handles incoming IDD events for all IDD plugins.
 * If plugin_id == ALL_PLUGINS, the event is matched against all plugins probe filter.
 * Otherwise only the probe filter from the given plugin_id is matched.
 */
void plugins_handle_event(int plugin_id, const char* package_p, const char* name_p,
                          size_t payload_size, const char* payload_p);

/**
 * Returns next file iterator to the given client_id report.
 * @param report_iterator: Next report will be returned. If 0 the first report will be returned.
 * @return -1 on failure or if no next report exists.
 */
int plugins_report_iterator_next(int client_id, int report_iterator);

/**
 * Open and return a file descriptor to the given report.
 * @param report_iterator Report to open and return.
 * @return -1 on failure or if the given report is missing.
 */
int plugins_report_get(int client_id, int report_iterator);

/**
 * Return max number of bytes needed for IDD plugins on the regular report storage.
 * RCA plugins are excluded because they store reports in another partition.
 */
int plugins_max_size_get();

/**
 * Callback function when the collection enabled status changes.
 */
void plugins_data_collection_enabled(int enabled);

/**
 * Force a flush on all plugins. Typically due to a report header change.
 */
void plugins_flush();

#endif
