/*
 * Copyright 2012 Sony Corporation
 *
 * Author: Victor Boivie <victor.boivie@sonymobile.com>
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include <idd_api.h>
#include "memory/memory.pb-c.h"

#include <log/log.h>
#undef LOG_TAG
#define LOG_TAG "startup-prober"

#define EMMC_HOST "/sys/class/mmc_host/mmc0/mmc0:0001"
#define EMMC_CSD_SUFFIX "/csd"

/* See JEDEC STANDARD Embedded Multi-Media Card (e•MMC) Electrical Standard (5.01) JESD84-B50.1 */
#define EMMC_EXT_CSD             "/sys/kernel/debug/mmc0/mmc0:0001/ext_csd"
#define EMMC_TOTAL_LEN           (512 * 2)
#define EMMC_CSD_REV_START       (192 * 2)
#define EMMC_CSD_REV_LEN         (1 * 2)
#define EMMC_FIRMWARE_START      (254 * 2)
#define EMMC_FIRMWARE_LEN        (8 * 2)
#define EMMC_EST_A_START         (268 * 2)
#define EMMC_EST_A_LEN           (1 * 2)
#define EMMC_EST_B_START         (269 * 2)
#define EMMC_EST_B_LEN           (1 * 2)
#define EMMC_HEALTH_RPT_START    (270 * 2)
#define EMMC_HEALTH_RPT_LEN      (32 * 2)
#define PATH_MAX_LEN 128

bool read_string(const char *fname, char *buf, size_t size)
{
  FILE* fh = fopen(fname, "r");
  size_t read;

  if (!fh) {
    printf("Failed to open %s: %s\n", fname, strerror(errno));
    return false;
  }

  read = fread(buf, 1, size - 1, fh);
  *(buf + read) = '\0';
  buf[strcspn(buf, "\n")] = '\0';

  fclose(fh);
  return true;
}

/*
 * Reverse the order of bytes in a ascci hex string from src to dest.
 * Ex src = "0123456789ab" and len = 12 -> dest = "ab8967452301".
 * dest will be NULL terminated and must be at least of size len + 1.
 */
void reverse_string(char dest[], char src[], int len)
{
  int i;
  for (i = 0; i < len; i += 2) {
    dest[i] = src[len - 2 - i];
    dest[i + 1] = src[len - 1 - i];
  }
  dest[len] = '\0';
}

/*
 * Open a file safely.
 * pathname : Pathname of a file to open safely.
 * Returns file descriptor for success, -1 otherwise.
 */
int open_safely(const char *pathname)
{
  int fd;
  struct stat lstat_result, fstat_result;

  if (lstat(pathname, &lstat_result) != 0)
  {
    printf("Failed to lstat %s\n", pathname);
    return -1;
  }

  if (!S_ISREG(lstat_result.st_mode))
  {
    printf("Failed to S_ISREG %s\n", pathname);
    return -1;
  }

  fd = open(pathname, O_RDONLY, 0);
  if (fd < 0)
  {
    printf("Failed to open %s\n", pathname);
    return -1;
  }

  if (fstat(fd, &fstat_result) != 0)
  {
    printf("Failed to fstat %s\n", pathname);
    close(fd);
    return -1;
  }

  if (lstat_result.st_ino != fstat_result.st_ino ||
      lstat_result.st_dev != fstat_result.st_dev)
  {
    printf("Failed to compare lstat with fstat %s\n", pathname);
    close(fd);
    return -1;
  }

  return fd;
}

bool isDirExists(const char* dir_name) {
  DIR* dir = opendir(dir_name);
  if (!dir) return false;
  closedir(dir);
  return true;
}

void get_emmc_data(IDD_t *idd_p)
{
  const char emmc_host_path[PATH_MAX_LEN] = EMMC_HOST;
  if (!isDirExists(emmc_host_path)) return; // not emmc device

  char emmc_csd_path[PATH_MAX_LEN] = {0};
  strncpy(emmc_csd_path, emmc_host_path, strlen(emmc_host_path) + 1);
  strncat(emmc_csd_path, EMMC_CSD_SUFFIX, strlen(EMMC_CSD_SUFFIX) + 1);
  int fd = open_safely(emmc_csd_path);
  if (fd < 0)
  {
    return;
  }
  close(fd);

  char buf[EMMC_TOTAL_LEN + 1] = {0};
  if (read_string(emmc_csd_path, buf, sizeof(buf))) {
    idd_set_sdata(idd_p, "memory", "EmmcCSD", buf);
  }

  buf[0] = '\0';
  if (read_string(EMMC_EXT_CSD, buf, sizeof(buf)) && (strlen(buf) == EMMC_TOTAL_LEN)) {
    Memory__EmmcExtCSD ext_csd = MEMORY__EMMC_EXT_CSD__INIT;

    char csd_rev[EMMC_CSD_REV_LEN + 1] = {0};
    reverse_string(csd_rev, &buf[EMMC_CSD_REV_START], EMMC_CSD_REV_LEN);
    ext_csd.csd_revision = &csd_rev[0];

    char firmware_version[EMMC_FIRMWARE_LEN + 1] = {0};
    reverse_string(firmware_version, &buf[EMMC_FIRMWARE_START], EMMC_FIRMWARE_LEN);
    ext_csd.firmware_version = &firmware_version[0];

    char est_type_a[EMMC_EST_A_LEN + 1] = {0};
    reverse_string(est_type_a, &buf[EMMC_EST_A_START], EMMC_EST_A_LEN);
    ext_csd.device_life_time_est_type_a = &est_type_a[0];

    char est_type_b[EMMC_EST_B_LEN + 1] = {0};
    reverse_string(est_type_b, &buf[EMMC_EST_B_START], EMMC_EST_B_LEN);
    ext_csd.device_life_time_est_type_b = &est_type_b[0];

    char health_rpt[EMMC_HEALTH_RPT_LEN + 1] = {0};
    reverse_string(health_rpt, &buf[EMMC_HEALTH_RPT_START], EMMC_HEALTH_RPT_LEN);
    ext_csd.vendor_proprietary_health_report = &health_rpt[0];

    /* ok, we have the data, turn it into a protocol buffer */
    size_t size = memory__emmc_ext_csd__get_packed_size(&ext_csd);
    uint8_t* serialized = malloc(size);
    if (serialized)
    {
      size = memory__emmc_ext_csd__pack(&ext_csd, serialized);
      idd_add_event(idd_p, memory__emmc_ext_csd__descriptor.package_name,
                    memory__emmc_ext_csd__descriptor.short_name, (char *)serialized, size);
      free(serialized);
    }
    else
    {
      printf("No IDD event created. Failed to allocate %zu bytes\n", size);
    }
  }
  else {
    printf("Couldn't read expected nr of bytes (%d) from ext_csd=<%zu>\n",
            EMMC_TOTAL_LEN, strlen(buf));
  }
}

int main(int argc, char **argv)
{
  IDD_t* idd_p = idd_create();

  if (!idd_p) {
    ALOGE("Failed to open IDD");
    exit(EXIT_FAILURE);
  }

  (void)get_emmc_data(idd_p);

  idd_destroy(&idd_p);

  (void)argc;
  (void)argv;
  return 0;
}
