/*
 * Copyright (C) 2014 Sony Mobile Communications Inc.
 * All rights, including trade secret rights, reserved.
 */

#ifndef _TLCORE_UTILS_H_
#define _TLCORE_UTILS_H_
#include <sha1.h>

int gen_sha1_from_file(FILE *fp, uint32_t offset, uint32_t size,
			unsigned char *sha1str);
int gen_sha1_from_buf(const void *buf, uint32_t len, unsigned char *sha1str);
char *get_file_name(const char *path);
#endif /* _TLCORE_UTILS_H_ */
