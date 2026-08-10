/*
 * Copyright (C) 2014 Sony Mobile Communications Inc.
 * All rights, including trade secret rights, reserved.
 */

/**
 * @brief Tlcore library utility functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <tlcore_internal.h>
#include <sha1.h>

#define BUFSIZE 65536
#define TAG TL_UTILS

/*
 * Generate SHA1 for a given file from the mentioned file offset to
 * the given size
 *
 * fp		- File pointer of the file whose SHA1 is to be generated
 * offset       - Offset from where the SHA1 is to be generated
 * size		- Size of data for which SHA1 is to be calculated
 * sha1str      - The SHA1 generated string.Size of the string
 *		  should be SHA_DIGEST_LENGTH
 * Return	- 0 on Success
 *               -ve error codes on Failure
 *
 */
int gen_sha1_from_file(FILE *fp, uint32_t offset, uint32_t size,
					unsigned char *sha1str)
{
	unsigned char *buffer = NULL;
	unsigned char message[SHA_DIGEST_LENGTH];
	int64_t pos;
	int32_t n;
	SHA1_CTX ctx;
	int ret, retval = 0;
	int32_t i, nr, rem;

	VALIDATE_PTR(fp, -EINVAL);
	VALIDATE_PTR(sha1str, -EINVAL);
	if (size == 0) {
		PRINT_ERROR("Size cannot be zero to get SHA1.\n");
		return -EINVAL;
	}

	buffer = (unsigned char *)calloc(1, BUFSIZE * sizeof(unsigned char));
	if (!buffer) {
		PRINT_ERROR("Memory allocation failed for" \
		"  buffer. Cannot get SHA1: %s(%d)\n", strerror(errno), errno);
		return -errno;
	}

	/* Store the current position of file pointer */
	pos = ftell(fp);
	if (pos < 0) {
		PRINT_ERROR("ftell failed.%s(%d)\n", strerror(errno), errno);
		free(buffer);
		return -errno;
	}

	/* Seek to the offset */
	ret = fseek(fp, offset, SEEK_SET);
	if (ret < 0) {
		PRINT_ERROR("error seeking to given offset : %d : %s(%d)\n",
				offset, strerror(errno), errno);
		retval = -errno;
		goto error;
	}

	/* Initialize the SHA_CTX structure */
	SHA1Init(&ctx);

	/* From mentioned offset till the mentioned size read the file
	 * and repeatedly hash the chunks of message (64K) */
	nr = size / BUFSIZE;
	rem = size % BUFSIZE;
	for (i = 1; i <= nr; i++) {
		n = fread(buffer, 1, BUFSIZE, fp);
		SHA1Update(&ctx, buffer, n);
	}

	if (ferror(fp)) {
		PRINT_ERROR("error reading : %s\n", strerror(errno));
		retval = -errno;
		goto error;
	}

	if (rem != 0) {
		n = fread(buffer, 1, rem, fp);
		if (ferror(fp)) {
			PRINT_ERROR("error reading for remaining buffer :" \
			" %s(%d)\n", strerror(errno), errno);
			retval = -errno;
			goto error;
		}
		SHA1Update(&ctx, buffer, n);
	}

	/* Place the final message digest in message */
	SHA1Final(message, &ctx);

	/* Copy the SHA1 string to the output buffer 'sha1str' */
	PRINT_DEBUG(TAG, "SHA1 generated is:\n");
	memcpy(sha1str, message, SHA_DIGEST_LENGTH);
error:
	if (buffer) {
		free(buffer);
		buffer = NULL;
	}
	/* Retain back the file pointer position */
	ret = fseek(fp, pos, SEEK_SET);
	if (ret < 0) {
		PRINT_ERROR("fseek fail back to original position:%"PRId64 \
				":%s(%d)\n", pos, strerror(errno), errno);
		retval = -errno;
	}
	return retval;
}

/*
 * Generate SHA1 for a given buffer for size len
 *
 * buff		- buffer for which sha1 needs to be generated
 * size		- Size of buffer for which SHA1 is to be calculated
 * sha1str      - The SHA1 generated string. Minimum Size of the string
 *		should be SHA_DIGEST_LENGTH
 *
 * Return	 0 on Success
 *               -ve error codes on Failure
 *
 */
int gen_sha1_from_buf(const void *buf, uint32_t len, unsigned char *sha1str)
{
	SHA1_CTX ctx;
	unsigned char message[SHA_DIGEST_LENGTH];

	VALIDATE_PTR(buf, -EINVAL);
	VALIDATE_PTR(sha1str, -EINVAL);

	if (len <= 0) {
		PRINT_INFO("Invalid Buffer Length to get sha1\n");
		return -EINVAL;
	}

	SHA1Init(&ctx);
	SHA1Update(&ctx, (uint8_t *)buf, len);
	SHA1Final(message, &ctx);

	memcpy(sha1str, message, SHA_DIGEST_LENGTH);

	return 0;
}

/*
 * Get only the name of the file from the complete path.
 *
 * path		- The comeplete path for the file.
 *
 * Return	- Returns the pointer to the file name from the given path
 */
char *get_file_name(const char *path)
{
	const char *tmp = NULL, *file_name = NULL;

	if (!path)
		return "(null)";

	file_name = tmp = path;
	while (*tmp) {
		if (*tmp++ == '/')
			file_name = tmp;
	}

	return (char *)file_name;
}
