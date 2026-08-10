/*
 * Copyright (C) 2014 Sony Mobile Communications Inc.
 * All rights, including trade secret rights, reserved.
 */

#ifndef _TLCORE_INTERNAL_H_
#define _TLCORE_INTERNAL_H_

#include <stdint.h>
#include <inttypes.h>
#include <tlcore_utils.h>

enum debug_level {
	TLCORE = 1,
	TL_HEADER = 2,
	SECTION = 4,
	SCN_HEADER = 8,
	SCN_DATA = 16,
	TL_UTILS = 32,
};

#define fseek fseeko
#define ftell ftello

#define __FILE_NAME__ get_file_name(__FILE__)
#ifdef ANDROID
#include <android/log.h>
#define PRINT_ERROR(fmt, ...) \
do { \
	__android_log_print(ANDROID_LOG_ERROR, "libtlcore", \
		"%s(%d): " fmt, __FILE_NAME__, __LINE__, ##__VA_ARGS__); \
	fprintf(stderr, "ERROR: %s(%d): " fmt, __FILE_NAME__, __LINE__, \
					##__VA_ARGS__); \
} while (0)
#else
#define PRINT_ERROR(fmt, ...) \
fprintf(stderr, "ERROR: %s(%d): " fmt, __FILE_NAME__, __LINE__, ##__VA_ARGS__);
#endif

#define PRINT_INFO(fmt, ...) \
fprintf(stdout, "INFO: %s(%d): " fmt, __FILE_NAME__, __LINE__, ##__VA_ARGS__)

#define PRINT_WARN(fmt, ...) \
fprintf(stdout, "WARN: %s(%d): " fmt, __FILE_NAME__, __LINE__, ##__VA_ARGS__)

#define PRINT_DEBUG(flag, fmt, ...) \
do { \
	if (flag & DEBUG_LEVEL) \
		fprintf(stdout, "DEBUG: %s(%d): " fmt, __FILE_NAME__, \
					__LINE__, ##__VA_ARGS__); \
} while (0)

#define VALIDATE_PTR(ptr, ret) \
do { \
	if (!ptr) { \
		PRINT_ERROR(#ptr" is NULL\n"); \
		return ret; \
	} \
} while (0)

enum int_scn_type {
	TYPE_DUMP = 0,
	TYPE_BINARY,
	TYPE_TEXT,
	TYPE_DUMP_64,
	TYPE_BINARY_64
};

enum dump_fmt_version {
	VERSION_4 = 4,
	VERSION_5 = 5,
	VERSION_6 = 6,
	LATEST_VERSION
};

#endif
