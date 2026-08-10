/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief Misc utilities
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#ifndef UTIL_H_INCLUSION_GUARD
#define UTIL_H_INCLUSION_GUARD
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>

#define IDD_DBG_CMD(s) if (is_debug()) {s;}

#ifndef MAX
# define MAX( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef MIN
# define MIN( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif

//quote (stringify) the argument, i.e. QUOTE_MACRO(3+5) -> "3+5"
#define QUOTE(x) #x
//quote (stringify) the "value" of the argument, i.e. QUOTE_MACRO(PROPERTY_VALUE_MAX) -> "92"
#define QUOTE_MACRO(x) QUOTE(x)

typedef enum
{
  NO_FORCE_WRITE = 0, // Do not force write of report header
  FORCE_WRITE    = 1  // Force write of report header
} Idd_Write_Report_Header_t;

typedef struct idd_sock_t_
{
  void(*on_receive)(struct idd_sock_t_* sock_p);
  int fd;
  void* ud;
} idd_sock_t;

typedef enum {
  REPORT_WITH_HEADER = 0,
  REPORT_WITHOUT_HEADER = 1,
  EVENTS_FILE = 2
} Idd_Report_t;

/* Returns the UNIX timestamp, in milliseconds since the epoch */
int64_t get_timestamp(void);

/* Returns the seconds east of UTC. */
int32_t get_timezone(void);

/* Handles setting and retrieving debug mode (which will enable printouts) */
int is_verbose(void);
void set_verbose(int val);

/* A working strtoll for architectures that don't produce valid results. */
int64_t strtoint64(const char* s);

/*
 * Create the report header.
 * Generate new report if the header has changed.
 */
void create_report_header(void);

/*
 * Write serialized report header to given file descriptor.
 * If force_write is false and the report header is not valid, the report header will not be
 * written to the file descriptor.
 *
 */
void write_report_header(int fd, Idd_Write_Report_Header_t force_write);

/*
 * Write serialized rca report header to the given file descriptor.
 */
void write_rca_report_header(int fd);

size_t get_report_header(const char** buffer_pp);

/*
 * Get empty report header.
 * Shouldn't store the unencrypted header info
 * into the encrypted report. Header is
 * the required field in the report.
 */
size_t get_empty_report_header(const char** buffer_pp);

/* Frees all memory used by the util module */
void util_shutdown(void);

/* Exits execution and prints out the error message. */
void die(const char* str);

/*
 * Set new wait time for save to file of counters and staticdata
 */
void save_time_update();

/*
 * Returns number of seconds to next save to file of counters and staticdata
 */
int save_time_left();

/* The length in bytes of the checksum of the
 * downloaded configuration. SHA1 = 160 bits = 20 bytes.
 */
#define CONFIG_CHECKSUM_LEN (20)

/* Sets the (binary, raw) SHA1 checksum of the current
 * configuration file.
 *
 * checksum_p must point at a buffer, CONFIG_CHECKSUM_LEN
 * bytes large.
 *
 * Returns true  If the configuration has changed.
 *         false If this is the device first configuration or this configuration already active in
 *               device.
 */
int set_config_checksum(char* checksum_p);

/* Retreives the (binary, raw) SHA1 checksum of the
 * current configuration file.
 *
 * checksum_p must point at a buffer, CONFIG_CHECKSUM_LEN
 * bytes large.
 */
void get_config_checksum(char* checksum_p);

/*
 * Initialize iddd by reading persisted data from the iddd state file
 */
void state_load();

/*
 * Update iddd state file with persistent data
 */
void state_save();

/*
 * Set new flush time
 */
void iddd_flushtime_set(int64_t new_flushtime);

/*
 * Return the time when the last flush to output file was done
 */
int64_t iddd_flushtime_get();

/*
 * Get the file size of the open file descriptor.
 * The file position will be set to the start of the file.
 * On error file size is set to 0.
 *
 * Returns 0 on success, -1 on error.
 */
int get_file_size(FILE* in_fp, size_t* size_p);

/* Handle an incoming IMEI.
 */
void imei_handle(const char* imei);

/* Reads the entire file contents of the file descriptor given
 * and puts the file size in size_p.
 *
 * On error, the function will return NULL and errno is set.
 */
unsigned char* read_entire_file(FILE* fp, size_t* size_p);

/* Indicates if IDDD should exit.
 *
 * Returns 0 if it should continue running and
 * 1 if it should exit as soon as possible.
 */
int should_exit(void);

/* Schedules a termination of IDDD
 *
 * This will make should_exit() return 1
 */
void set_should_exit(void);

/* Runs the kill switch, if present.
 *
 * Which means that we will never return from
 * this function if it's active.
 */
void run_kill_switch(void);

/*
 * Wrapper of the write(...) function.
 * Guarantees that the requested size is written,
 * if not so returns -1 with errno set.
 *
 * Parameters same as write(), unistd.h.
 * Returns same as write(), unistd.h.
 */
ssize_t write_all(int fd, const void *buf, size_t size);

/*
 * Try to rescue the report if it's corrupt because
 * last exit of process erroneous data writing.
 */
void rescue_report(char* filename, Idd_Report_t type);

/*
 * When idd daemon starts, should invoke this function to
 * initiate the report header.
 */
void report_header_init();

/*
 * Check the header is valid or not.
 * Imei and serial number must exist if the device is a prototype.
 */
int is_valid_header(void);

/* Check whether data collection is enabled or not.
 *
 * Returns 1 if enabled, 0 if disabled.
 */
int is_data_collection_enabled(void);

/* Enable/disable data collection.
 *
 * @param enable: 1 is enable, 0 is disable.
 */
void set_data_collection_enabled(int enable);

/*
 * Handle incoming header group id -- set a new group id.
 */
void group_id_set_handle(const char* new_group_id, int create_report);

/*
 * Handle incoming header group id -- delete an existed group id.
 */
void group_id_delete_handle(const char* del_group_id, int create_report);

/*
 * Handle incoming header group id -- replace an existed group id to a specified new group id.
 */
void group_id_replace_handle(const char* to_group_id, const char* from_group_id, int create_report);

/*
 * Return the group_id starting with group_id_start_with.
 *        NULL if not found.
 */
char* group_id_starting_with_exist(const char* group_id_start_with);

/* Remove report.
 *
 * @param report_num: 0 is remove all reports, positive is specified report.
 */
void remove_report(int report_num);

/*
 * Generate device id for IDD.
 * The first device id will be used for all configuration request.
 * A forced device id will only be used in IDD reports.
 *
 * @param force: if 1 then generate new device id.
 */
void generate_device_id(int force);

/*
 * Generate device id for RCA.
 */
void generate_rca_device_id();

/* Retreives the device id generated by iddd
 *
 * device_id_p must point at a buffer, IDD_MAX_DEVICE_ID_SIZE
 * bytes large.
 */
void get_device_id(char* device_id_p);

/*
 * Return the device id used in the configuration request.
 */
char* get_device_id_cr();

/*
 * Generate session id.
 */
void generate_session_id(void);

/*
 * Remove the content of given directory and if remove_dir is true, the directory too.
 * On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
 */
int remove_directory(const char *path, int remove_dir);

/* Shall data washing of personal data be done?.
 *
 * Non prototypes shall wash personal data and prototypes shall not. However the IDD configuration
 * flag iddd.Settings.wash_data can be used to override this.
 *
 * Returns true if data washing shall be done, false if no washing shall be done.
 */
bool do_washing(void);

/*
 * Handle an incoming ANDROID_ID owned by IDD agent.
 */
void android_id_handle(const char* new_android_id);

/*
 * Handle an incoming ANDROID_ID owned by Xperia services.
 */
void xs_android_id_handle(const char* new_xs_android_id);

/*
 * Handle an incoming GUID owned by Xperia services.
 */
void xs_guid_handle(const char* new_xs_guid);

/*
 * Handle an incoming user consent.
 */
void user_consent_handle(int user_consent);

/*
 * Return true if user consent is not all and configuration is not only for user consent with all.
 */
bool needs_save_anonymous_config(void);

/*
 * Return true if user consent is matched with the one required by configuration.
 */
bool is_required_user_consent_matched(void);

/*
 * Return true if user variant.
 */
bool is_user_variant(void);

/*
 * Return true if build type is changed.
 */
bool is_build_type_changed(void);

/*
 * Set initial data collection enabled.
 */
void set_initial_data_collection_enabled(void);

/*
 * Set initial setting for prototype.
 */
void handle_prototype_setting(void);

/*
 * Handle timezone.
 */
void timezone_handle(void);

/*
 * Get idd report directory stats.
 */
bool get_idd_report_stats(int* oldest_file_nr, int* newest_file_nr, int* total_file_nr, size_t* sum_size);

#endif
