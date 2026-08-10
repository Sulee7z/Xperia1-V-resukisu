/*
 * Copyright 2013 Sony Corporation
 */

/**
 * @brief File utilities to IDD
 *
 * @author Leif Mårtensson (leif.martensson@sonymobile.com)
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <zlib.h>
#include <errno.h>
#include <string.h>

#include "file_utils.h"
#include "plugins.h"
#include "util.h"

#define LOG_TAG "iddd-fu"
#include "logging.h"

#define CHUNK_SIZE 2048

/* Each protobuf message starts with 3 bit message type. */
#define LEN_PROTO_TYPE             3
/* The code representing the Header message in the Report message,
 * as given in the common.proto file. */
#define REPORT_HEADER_ID           1
#define REPORT_HAS_HEADER(report) ((*(char*)(report) >> LEN_PROTO_TYPE) == REPORT_HEADER_ID)

/*
 *
 */
static int report_gzcompress(FILE* input_fp, gzFile output_gz, int header)
{
  char buffer[CHUNK_SIZE] = {0};

  for (;;)
  {
    ssize_t bytes_read = fread(buffer, 1, sizeof(buffer), input_fp);
    if (header)
    {
      header = 0; /* Make sure header is true only on first chunk. */
      if ((bytes_read == 0) || !REPORT_HAS_HEADER(buffer))
      {
        const char* header_p;
        /* get_report_header is not thread safe but as long as we fork the compactor we are good. */
        int header_size = (int)get_report_header(&header_p);
        if (gzwrite(output_gz, header_p, header_size) != header_size)
        {
          IDD_LOGE("Write report header %d byte failed <%s>.", header_size, strerror(errno));
          return 0;
        }
      }
    }
    if (bytes_read == 0)
    {
      return feof(input_fp) ? 1 : 0;
    }
    if (gzwrite(output_gz, buffer, bytes_read) != bytes_read)
    {
      return 0;
    }
  }
}

/**
 *
 */
int report_compact(const char* input_p, const char* output_p, int header)
{
  FILE* input_fp = fopen(input_p, "rb");
  int ret;
  gzFile output_gz;

  if (input_fp == NULL)
  {
    IDD_LOGE("Failed to open input file: %s", strerror(errno));
    return 0;
  }

  output_gz = gzopen(output_p, "wb9");

  if (output_gz == NULL)
  {
    IDD_LOGE("Failed to gzopen output file");
    fclose(input_fp);
    return 0;
  }
  ret = report_gzcompress(input_fp, output_gz, header);
  gzclose(output_gz);
  fclose(input_fp);

  return ret;
}

