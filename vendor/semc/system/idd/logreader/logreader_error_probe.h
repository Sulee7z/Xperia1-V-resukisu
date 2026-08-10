/*
 * Copyright 2017 Sony Corporation
 */

#ifndef IDD_LOGREADER_ERROR_H_INCLUSION_GUARD
#define IDD_LOGREADER_ERROR_H_INCLUSION_GUARD

#include <stdbool.h>
#include <time.h>

/*
 * Load the time stamp and number of logreader errors.
 */
void state_load();

/*
 * Save the timestamp and number of logreader errors in logreader.state
 */
void state_save();

/*
 * Set the number of logreader error probes
 *
 * @param nr error number
 */
void logreader_error_number_set(int nr);

/*
 * Returns number of logreader error probes
 *
 * @return nr_errors
 */
int logreader_error_number_get();

/*
 * Set the start of the current period
 *
 * @param timestamp from the logreader.state
 */
void logreader_error_timestamp_set(time_t timestamp);

/*
 * Returns the error probe timestamp
 *
 * @return logreader_error_timestamp
 */
time_t logreader_error_timestamp_get();

/*
 * Create a logreader error probe
 *
 * @param fmt format of the variable arguments received in .three dots.
 */
void idd_logreader_error_probe(const char *fmt, ...);

/*
 * Initialize the logreader error probe
 *
 * @param base_dir path where IDD is running
 * @param setting_test_mode receive true if test mode is set otherwise false
 */
void idd_logreader_error_probe_init(char* base_dir, bool setting_test_mode);

#define IDD_LOGREADER_PE(...) \
  do { \
    IDD_LOGE(__VA_ARGS__); \
    idd_logreader_error_probe(LOG_TAG ": " __VA_ARGS__); \
  } while (0)

#endif

