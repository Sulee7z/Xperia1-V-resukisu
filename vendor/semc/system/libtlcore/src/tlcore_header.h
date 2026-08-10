/*
* Copyright (C) 2014 Sony Mobile Communications AB.
* All rights, including trade secret rights, reserved.
*/

#ifndef _TLCORE_HEADER_H_
#define _TLCORE_HEADER_H_

#include <stdint.h>
#include <tlcore.h>

#define DUMP_RES_UNHANDLED 255
struct tlcore_header_v4 {
	uint32_t magic;
	uint32_t tlcore_version;
	char dumper_version[TLCORE_MAX_DUMPER_VERSION_SZ];
	char platform[TLCORE_MAX_PLATFORM_NAME_SZ];
	char product[TLCORE_MAX_PRODUCT_NAME_SZ];
	char build_id[TLCORE_MAX_BUILD_ID_SZ];
	char build_variant[TLCORE_MAX_BUILD_VARIANT_SZ];
	char imei[TLCORE_MAX_IMEI_SZ];
	char serial[TLCORE_MAX_SERIAL_NO_SZ];
	uint32_t result;
	uint32_t count;
	char sign[TLCORE_MAX_SIGNATURE_SZ];
} __attribute__((packed));

struct tlcore_header_v5 {
	uint32_t magic;
	uint16_t major;
	uint16_t minor;
	char dumper_version[TLCORE_MAX_DUMPER_VERSION_SZ];
	char platform[TLCORE_MAX_PLATFORM_NAME_SZ];
	char product[TLCORE_MAX_PRODUCT_NAME_SZ];
	char build_id[TLCORE_MAX_BUILD_ID_SZ];
	char build_variant[TLCORE_MAX_BUILD_VARIANT_SZ];
	char imei[TLCORE_MAX_IMEI_SZ];
	char serial[TLCORE_MAX_SERIAL_NO_SZ];
	uint32_t result;
	uint32_t count;
	char sign[TLCORE_MAX_SIGNATURE_SZ];
	char crash_time[TLCORE_MAX_CRASH_TIME_SZ];
	char dumper_name[TLCORE_MAX_DUMPER_NAME_SZ];
	uint16_t dump_type;
	uint16_t device_type;
	char reserved[168];
} __attribute__((packed));

struct tlcore_header_v6 {
	uint32_t magic;
	uint16_t major;
	uint16_t minor;
	char dumper_version[TLCORE_MAX_DUMPER_VERSION_SZ];
	char platform[TLCORE_MAX_PLATFORM_NAME_SZ];
	char product[TLCORE_MAX_PRODUCT_NAME_SZ];
	char build_id[TLCORE_MAX_BUILD_ID_SZ];
	char build_variant[TLCORE_MAX_BUILD_VARIANT_SZ];
	char imei[TLCORE_MAX_IMEI_SZ];
	char serial[TLCORE_MAX_SERIAL_NO_SZ];
	uint32_t result;
	uint32_t count;
	char sign[TLCORE_MAX_SIGNATURE_SZ];
	char crash_time[TLCORE_MAX_CRASH_TIME_SZ];
	char dumper_name[TLCORE_MAX_DUMPER_NAME_SZ];
	uint16_t dump_type;
	uint16_t device_type;
	uint16_t arch_type;
} __attribute__((packed));

struct tlcore_header *__init_tlhdr(FILE *fp, struct tlcore *tlcore,
		uint64_t offset);
int __tlhdr_inc_scn_count(void);
int __tlhdr_set_sha1(const unsigned char *sha1_str, uint32_t sha1_size);
int __finish_tlcore_header(void);
#endif
