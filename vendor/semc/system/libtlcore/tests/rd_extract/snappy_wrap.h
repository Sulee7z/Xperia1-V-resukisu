/*
 * Copyright (C) 2014 Sony Mobile Communications AB.
 * All rights, including trade secret rights, reserved.
 */
#ifndef _SNAPPY_WRAP_H
#define _SNAPPY_WRAP_H


#include <stdio.h>
#include <stdlib.h>

enum archiver_result {
	ARCHIVER_RES_OK = 0,
	ARCHIVER_RES_ERR,
};

enum archiver_result snappy_init();
enum archiver_result snappy_compression(const char *inbuf,
	unsigned int inbuf_size, char **outbuf, unsigned int *outbuf_size);
enum archiver_result snappy_uncompression(const char *inbuf,
	unsigned int inbuf_size, char **outbuf, unsigned int *outbuf_size);
enum archiver_result snappy_end();

#endif
