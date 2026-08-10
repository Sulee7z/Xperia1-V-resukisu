/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief Report compactor (part of flusher) for IDD daemon
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <selinux/android.h>
#include <sys/statfs.h>
#include "compactor.h"
#include "util.h"
#include "config.h"
#include "events.h"
#include "counters.h"
#include "sdata.h"
#include "settings.h"
#include "crypto.h"
#include "serializer.h"
#include "file_utils.h"
#include "plugins.h"

#define LOG_TAG "iddd-compactor"
#include "logging.h"

pid_t compactor_pid = -1;

#define MIN_FREE_INODES 8

static int encrypt_protobuf(FILE* input_fp, FILE* output_fp, idd_encryption_t* enc_p)
{
  int ret = 0;
  char crypto_header[IDD_MAX_PUBLIC_KEY_SIZE/8 + IDD_SYMMETRIC_KEY_SIZE/8 + 100] = {0};
  const char* header_p;
  size_t file_size;
  (void)get_file_size(input_fp, &file_size);
  size_t encrypted_size = get_encrypted_size(file_size);
  size_t header_bytes = get_empty_report_header(&header_p);
  if (header_bytes == 0)
  {
    IDD_LOGE("Failed to get empty report header.");
    return 0;
  }
  size_t crypto_bytes = serialize_encrypted_data(crypto_header, (char*)enc_p->key, enc_p->key_size,
                                                 (char*)enc_p->iv, IDD_SYMMETRIC_KEY_SIZE/8,
                                                 encrypted_size);

  if ((fwrite(header_p, 1, header_bytes, output_fp) == header_bytes) &&
      (fwrite(crypto_header, 1, crypto_bytes, output_fp) == crypto_bytes))
  {
    ret = encrypt_file(input_fp, output_fp, enc_p);
  }
  else
  {
    IDD_LOGE("Failed to write crypto data, %s", strerror(errno));
  }
  return ret;
}

static int encrypt_report(const char* input_p, const char* output_p)
{
  int ret;
  idd_encryption_t enc;

  ret = encrypt_init(&enc);
  if (ret)
  {
    FILE* input_fp = fopen(input_p, "rb");
    if (input_fp != NULL)
    {
      FILE* output_fp = fopen(output_p, "wb");
      if (output_fp != NULL)
      {
        ret = encrypt_protobuf(input_fp, output_fp, &enc);
        fclose(output_fp);
      }
      else
      {
        ret = 0;
        IDD_LOGE("Failed to open output encryption file: %s", strerror(errno));
      }
      fclose(input_fp);
    }
    else
    {
      ret = 0;
      IDD_LOGE("Failed to open input encryption file: %s", strerror(errno));
    }
    encrypt_destroy(&enc);
  }
  else
  {
    IDD_LOGE("Failed to initialize encryption.");
  }
  return ret;
}

/* Handles encryption for a report.
 *
 * Returns 1 on success, 0 on error.
 */
int encrypt_handler(const char* output_p, const char* tmpfile_p)
{
  int ret = 1;
  if (is_encryption_active())
  {
    IDD_LOGD("Encrypt(%s -> %s)", output_p, tmpfile_p);
    ret = encrypt_report(output_p, tmpfile_p);
    IDD_LOGD("Encryption done, ret = %d", ret);
    if (ret)
    {
      if (rename(tmpfile_p, output_p) != 0)
      {
        IDD_LOGE("Failed to rename encrypted file: %s", strerror(errno));
        unlink(tmpfile_p);
        ret = 0;
      }
      else
      {
        IDD_LOGD("Rename(%s -> %s) ok", tmpfile_p, output_p);
      }
    }
  }

  return ret;
}

/**
 *
 */
int is_free_space_enough(const char* output_dir_p, fsblkcnt_t avail_size)
{
  struct statfs idd_statfs = {};
  fsblkcnt_t act_avail_size = 0;
  long free_inodes = 0;

  if (statfs(output_dir_p, &idd_statfs) == -1)
  {
    IDD_LOGE("is_free_space_enough(): statfs() failed: %s", strerror(errno));
    return 0;
  }

  act_avail_size = idd_statfs.f_bavail * idd_statfs.f_bsize;
  /* if f_files is 0 filesystem is not inode based */
  free_inodes = idd_statfs.f_files == 0 ? MIN_FREE_INODES : idd_statfs.f_ffree;
  IDD_LOGD("Free space is %ld kb, reserved is %ld kb-> left for reports is %ld kb",
           act_avail_size/1024, avail_size/1024,
           (act_avail_size > avail_size) ? ((act_avail_size - avail_size) / 1024) : 0);
  IDD_LOGD("Free inodes is %ld , minimum accepted is %d", free_inodes, MIN_FREE_INODES);
  return act_avail_size >= avail_size && free_inodes >= MIN_FREE_INODES;
}

/**
 * Get output directory statistics
 *
 * Returns 0 on success, -1 on error
 */
int get_statdir(const char* path, statdir* statdir_p)
{
  struct dirent *de;
  struct stat sb;
  DIR *dir;
  char name[PATH_MAX] = {0};
  int oldest_file_nr = INT_MAX;
  int newest_file_nr = 0;
  int total_file_nr = 0;
  size_t sum_size = 0;

  dir = opendir(path);
  if (dir == NULL)
  {
    return -1;
  }

  rewinddir(dir);

  while ((de = readdir(dir)) != NULL)
  {
    int file_nr;
    if (strcmp(de->d_name, ".") == 0 ||
        strcmp(de->d_name, "..") == 0)
    {
      continue;
    }

    snprintf(name, sizeof(name), "%s%s", path, de->d_name);

    if (lstat(name, &sb) != 0)
    {
      IDD_LOGW("Failed to stat %s: %s", name, strerror(errno));
      continue;
    }

    total_file_nr++;
    sum_size += sb.st_size;

    /* Find out which number this is - we remove the lowest one */
    if (sscanf(de->d_name, "idd_%d", &file_nr) != 1)
    {
      IDD_LOGW("Bad file in our %s directory: %s", path, de->d_name);
      continue;
    }

    if (file_nr < oldest_file_nr)
    {
      oldest_file_nr = file_nr;
    }

    if (file_nr > newest_file_nr)
    {
      newest_file_nr = file_nr;
    }
  }

  statdir_p->newest_file_nr = newest_file_nr;
  statdir_p->oldest_file_nr = oldest_file_nr;
  statdir_p->total_file_nr = total_file_nr;
  statdir_p->sum_size = sum_size;

  closedir(dir);
  return 0;
}

/* Removes the oldest files until the directory has
 * avail_size enough free space and
 * is at maximum max_size bytes large.
 *
 * The newest file number found will be returned, or -1 on failure.
 */
static int prune_dir(const char* output_dir_p, size_t extra_size)
{
  /* The amount of needed free space. If free space is less than this, pruning is needed.
   * 2*settings_report_max_size_get() spared for the newest report and next events file
   * 2*MAX_CONFIG_SIZE spared for the newest configuration file and intermediate file,
   * e.g. new_config.gz & new_config.sig
   * 100kb for the remaining files in IDD folder
   * extra_size is typically size needed to store a very large event, for example a 2MB crash
   */
  fsblkcnt_t avail_size = 2*settings_report_max_size_get() +
                          2*MAX_CONFIG_SIZE +
                          100*1024 +
                          extra_size;
  avail_size += (fsblkcnt_t)plugins_max_size_get();

  /* We make sure that we have about the amount of space necessary for the events.
   * Since we will compress the output, we will (most certainly) end up with much less
   * than the max when we're finished, but on the other hand, we haven't considered the static
   * data and counters, but that's not much data so we should be on the safe side.
   */
  size_t max_size = settings_output_dir_max_size_get() - settings_report_max_size_get();

  char name[PATH_MAX] = {0};
  statdir output_statdir = { 0, INT_MAX, 0, 0 };

  IDD_LOGD("free space %s to %lu kb", output_dir_p, avail_size/1024);
  IDD_LOGD("pruning %s to %zu kb", output_dir_p, max_size/1024);

  for (;;)
  {
    if (get_statdir(output_dir_p, &output_statdir) == -1)
    {
      IDD_LOGE("prune_dir(): get_statdir() failed: %s", strerror(errno));
      return -1;
    }

    /* We make sure that we have about the amount of space necessary
     * for the events and all data write operation in IDD folder.
     */
    if (is_free_space_enough(output_dir_p, avail_size) && (output_statdir.sum_size < max_size))
    {
      IDD_LOGD("free space enough and pruning directory success");
      break;
    }

    if (output_statdir.oldest_file_nr == INT_MAX)
    {
      // Not enough free space or Max size already reached, but we found no file to remove?
      IDD_LOGE("Failed to prune files from output directory");
      output_statdir.newest_file_nr = -1;
      break;
    }

    snprintf(name, sizeof(name), "%sidd_%d", output_dir_p, output_statdir.oldest_file_nr);
    if (unlink(name) == -1)
    {
      output_statdir.newest_file_nr = -1;
      break;
    }
    IDD_LOGI("Deleting %s from output directory", name);
  }

  return output_statdir.newest_file_nr;
}

/* The IDD daemon will fork itself and run this function whenever
 * a file needs to be flushed. It will use FLUSHED_DATA_FNAME
 * as input, and put the output in the OUTPUT_DIR_NAME directory.
 */
static void compactor(void)
{
  int newest_file_nr = prune_dir(OUTPUT_DIR_NAME, 0);

  if (newest_file_nr == -1)
  {
    IDD_LOGE("Corrupt output directory - will not do anything.");
    exit(EXIT_FAILURE);
  }

  if (!report_compact(FLUSHED_DATA_FNAME, COMPACTED_DATA_FNAME, ADD_HEADER))
  {
    IDD_LOGE("Failed to compact file");
    exit(EXIT_FAILURE);
  }
  if (!encrypt_handler(COMPACTED_DATA_FNAME, ENCRYPTED_DATA_FNAME))
  {
    exit(EXIT_FAILURE);
  }

  char real_output[PATH_MAX] = {0};
  snprintf(real_output, sizeof(real_output), "%sidd_%d", OUTPUT_DIR_NAME, newest_file_nr + 1);
  if (rename(COMPACTED_DATA_FNAME, real_output) == -1)
  {
    IDD_LOGE("Failed to rename output file: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (setfilecon(real_output, "u:object_r:idd_output_file:s0") != 0)
  {
    IDD_LOGE("Failed to set file context for %s: %s", real_output, strerror(errno));
  }

  IDD_LOGD("Output saved as %s", real_output);
  /* All done. We can remove flushed_data to save space. */
  unlink(FLUSHED_DATA_FNAME);
}

/**
 *
 */
int prune_data(size_t extra_size)
{
  return prune_dir(OUTPUT_DIR_NAME, extra_size);
}

/**
 * Check for new IDD data since the last IDD report was generated.
 *
 * Returns 1 if non reported IDD data exists, otherwise 0.
 */
int has_new_data(void)
{
  int has_new_data = 0;
  if (is_counter_update()
      || is_sdata_update()
      || has_new_event())
  {
    has_new_data = 1;
  }
  return has_new_data;
}

/**
 * Returns PID of compactor.
 */
pid_t get_compactor_pid()
{
  return compactor_pid;
}

/* We want to generate a Report, that can be sent over the wire.
 * In other words: Empty events, serialize counters and static data
 * and put them all in the same file. This file will then be compressed
 * (and possibly encrypted) and put in the /output directory
 *
 * That will be done by the compactor, which is our iddd forked.
 */
int generate_report(void)
{
  if (!is_data_collection_enabled())
  {
    IDD_LOGD("Data collection is disabled, won't generate new report.");
    return -1;
  }

  if (!has_new_data())
  {
    IDD_LOGD("No any data update(events&counter&static data) since last report generation, won't generate new one");
    return -1;
  }

  FILE* fp;

  IDD_LOGI("Generating Report...");

  if (compactor_pid != -1)
  {
    /* The compactor has been running. In worst case, it's
     * still running, so we then wait for it here until it finishes.
     */
    waitpid(compactor_pid, NULL, 0);
    compactor_pid = -1;
  }

  /* This call will create FLUSHED_DATA_FNAME, which is the input
   * to the compactor.
   */
  if (flush_events() < 0)
  {
    IDD_LOGE("Failed to flush events");
    return -1;
  }

  fp = fopen(FLUSHED_DATA_FNAME, "ab");
  if (fp == NULL)
  {
    IDD_LOGE("Failed to open flushed data for writing: %s", strerror(errno));
    return -1;
  }
  else
  {
    int64_t timestamp_flush_started = get_timestamp();

    /* Avoid flushing before system time is updated */
    if (timestamp_flush_started > iddd_flushtime_get()) {
      /* Save time from before iterations of counters and static data secure that
       * we don't miss a record modified after we iterate past it.
       */
      counters_flush(fp);
      sdata_flush(fp);

      iddd_flushtime_set(timestamp_flush_started);
      state_save();
    }


    fclose(fp);
    /* Now fork a child which will compress the output and put it in the correct directory */
    compactor_pid = fork();
    if (compactor_pid == 0)
    {
      /* I'm the child. */
      compactor();
      IDD_LOGI("Compactor all done.");
      exit(EXIT_SUCCESS);
    }
    else if (compactor_pid == -1)
    {
      IDD_LOGE("Failed to fork compactor thread.");
    }
  }
  return 0;
}
