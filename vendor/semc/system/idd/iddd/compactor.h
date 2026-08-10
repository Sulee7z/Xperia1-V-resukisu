/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief Report compactor (part of flusher) for IDD daemon
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#ifndef COMPACTOR_H_INCLUSION_GUARD
#define COMPACTOR_H_INCLUSION_GUARD

#include <sys/types.h>

typedef struct {
  int newest_file_nr;
  int oldest_file_nr;
  int total_file_nr;
  size_t sum_size;
} statdir;

/* Encrypt report given in output_p.
 * Uses tmpfile_p as temporary file. This is deleted before return.
 *
 * Returns 1 on success, 0 on error.
 */
int encrypt_handler(const char* output_p, const char* tmpfile_p);

/* Make sure we have free space and inodes in the IDD partition.
 *
 * Files will be deleted from the output folder until the free space on partition is at least
 * 2*settings_report_max_size_get() + 2*MAX_CONFIG_SIZE + 100kb + extra_size
 *
 * Returns -1 on failure
 */
int prune_data(size_t extra_size);

/**
 * Check if the available space of partition is at least avail_size bytes free large.
 * Also make sure we have free inodes.
 *
 * 1 is large enough , otherwise return 0
 */
int is_free_space_enough(const char* output_dir_p, fsblkcnt_t avail_size);

/**
 * Returns PID of compactor.
 */
pid_t get_compactor_pid();

/* Generates a new Report, in the output directory, with data
 * from all modules (events, counters, static data)
 *
 * Returns -1 on failure.
 */
int generate_report(void);

/**
 * Get output directory statistics
 *
 * Returns 0 on success, -1 on error
 */
int get_statdir(const char* path, statdir* statdir_p);

#endif
