/*
 * Copyright (C) 2014 Sony Mobile Communications AB.
 * All rights, including trade secret rights, reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <snappy-c.h>
#include "snappy_wrap.h"

static __thread char *snappy_buf;


enum archiver_result snappy_init()
{
	return ARCHIVER_RES_OK;
}

enum archiver_result snappy_compression(
	const char *inbuf, unsigned int inbuf_size,
	char **outbuf, unsigned int *outbuf_size)
{
	size_t max_outbuf_size = snappy_max_compressed_length(inbuf_size);
	if (snappy_buf) {
		free(snappy_buf);
		snappy_buf = NULL;
	}
	snappy_buf = (char *) malloc(max_outbuf_size);
	if (snappy_buf == NULL) {
		printf("Error: malloc(%d) %s\n", errno, strerror(errno));
		return ARCHIVER_RES_ERR;
	}
	int snappy_ret = snappy_compress(inbuf, (size_t) inbuf_size,
				snappy_buf, &max_outbuf_size);
	if (snappy_ret != SNAPPY_OK) {
		printf("Error: snappy_compress(%d)\n", snappy_ret);
		return ARCHIVER_RES_ERR;
	}
	*outbuf = snappy_buf;
	*outbuf_size = (unsigned int) max_outbuf_size;
	return ARCHIVER_RES_OK;
}

enum archiver_result snappy_uncompression(
	const char *inbuf, unsigned int inbuf_size,
	char **outbuf, unsigned int *outbuf_size)
{
	size_t max_outbuf_size;
	snappy_status snappy_ret = snappy_uncompressed_length(inbuf,
			inbuf_size, &max_outbuf_size);
	if (snappy_ret != SNAPPY_OK) {
		printf("Error: snappy_uncompressed_length(%d)\n", snappy_ret);
		return ARCHIVER_RES_ERR;
	}
	if (snappy_buf) {
		free(snappy_buf);
		snappy_buf = NULL;
	}
	snappy_buf = (char *) malloc(max_outbuf_size);
	if (snappy_buf == NULL) {
		printf("Error: malloc(%d) %s\n", errno, strerror(errno));
		return ARCHIVER_RES_ERR;
	}
	snappy_ret = snappy_uncompress(inbuf, (size_t) inbuf_size,
				snappy_buf, &max_outbuf_size);
	if (snappy_ret != SNAPPY_OK) {
		printf("Error: snappy_uncompress(%d)\n", snappy_ret);
		return ARCHIVER_RES_ERR;
	}
	*outbuf = snappy_buf;
	*outbuf_size = (unsigned int) max_outbuf_size;
	return ARCHIVER_RES_OK;
}

enum archiver_result snappy_end()
{
	if (snappy_buf) {
		free(snappy_buf);
		snappy_buf = NULL;
	}
	return ARCHIVER_RES_OK;
}
