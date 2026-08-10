/*
 * Copyright 2014 Sony Corporation
 */

/**
 * @brief Generic functions for different idd-logreader modules.
 *
 * @author Leif Martensson (leif.martensson@sonyericsson.com)
 */

#include <malloc.h>
#include <stdio.h>
#include <unistd.h>

void free_wrapper(void ** pointer_pp) {
  free(*pointer_pp);
  *pointer_pp = NULL;
}

/*
 * Returns the file size of the open file descriptor.
 * The file position will be set to the start of the file.
 *
 * Returns 0 on success, -1 on error.
 */
static int get_file_size(FILE* in_fp, size_t* size_p)
{
  *size_p = 0;
  if (fseek(in_fp, 0, SEEK_END) == 0)
  {
    long size = ftell(in_fp);
    if (fseek(in_fp, 0, SEEK_SET) == 0)
    {
      *size_p = (size_t)size;
      return 0;
    }
  }
  return -1;
}

unsigned char* read_entire_file(int fd, size_t* size_p)
{
  unsigned char* buf_p = NULL;
  FILE* fp = fdopen(fd, "rb");

  if (fp == NULL)
  {
    close(fd);
  }
  else
  {
    if (get_file_size(fp, size_p) == 0)
    {
      buf_p = malloc(*size_p);
      if (buf_p != NULL)
      {
        if (fread((char*)buf_p, 1, *size_p, fp) != *size_p)
        {
          free(buf_p);
          buf_p = NULL;
        }
      }
    }
    fclose(fp);
  }
  return buf_p;
}
