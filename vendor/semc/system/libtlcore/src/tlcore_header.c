/*
 * Copyright (C) 2014 Sony Mobile Communications Inc.
 * All rights, including trade secret rights, reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <tlcore.h>
#include <tlcore_internal.h>
#include <tlcore_header.h>
#include <tlcore_utils.h>

#define TAG TL_HEADER

struct tlcore_header_generic {
	uint32_t *magic;
	uint16_t *major;
	uint16_t *minor;
	char *dumper_version;
	char *platform_name;
	char *product_name;
	char *build_id;
	char *build_variant;
	char *imei;
	char *serial;
	uint32_t *result;
	uint32_t *count;
	char *signature;
	char *crash_time;
	char *dumper_name;
	uint16_t *dump_type;
	uint16_t *device_type;
	uint16_t *arch_type;
};

struct tlhdr_magic {
	uint32_t magic;
	uint16_t major;
	uint16_t minor;
};

static struct tlcore_header_generic tlhdr_generic;
static uint16_t mode, major_version, minor_version;
static FILE *fp_core;
static uint64_t tlhdr_offset;
static struct tlcore_header_v4 *tlhdrv4_ll;
static struct tlcore_header_v5 *tlhdrv5_ll;
static struct tlcore_header_v6 *tlhdrv6_ll;

static int write_tlhdr_in_tlcore(void)
{
	VALIDATE_PTR(fp_core, -EINVAL);
	if (fseek(fp_core, tlhdr_offset, SEEK_SET) < 0) {
		PRINT_ERROR("fseek failed: %s(%d)\n", strerror(errno), errno);
		return -errno;
	}

	if (fwrite(tlhdrv6_ll, sizeof(*tlhdrv6_ll), 1, fp_core) != 1) {
		PRINT_ERROR("fwrite failed: %s(%d)\n", strerror(errno), errno);
		return -errno;
	}
	return 0;
}

static int tlhdr_set_magic(void)
{
	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Invalid operation in TLCORE_READONLY mode.\n");
		return -EINVAL;
	}

	if (!tlhdr_generic.magic) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	*(tlhdr_generic.magic) = 0xDEADBEEF;
	return write_tlhdr_in_tlcore();
}

static int tlhdr_get_version(uint16_t *major, uint16_t *minor)
{
	VALIDATE_PTR(major, -EINVAL);
	VALIDATE_PTR(minor, -EINVAL);
	if (!tlhdr_generic.minor || !tlhdr_generic.major) {
		PRINT_ERROR("Operation not supported.\n");
		return -ENOTSUP;
	}

	*major = *tlhdr_generic.major;
	*minor = *tlhdr_generic.minor;
	return 0;
}

static int tlhdr_set_version(uint16_t major, uint16_t minor)
{

	if (!tlhdr_generic.major || !tlhdr_generic.minor) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Invalid operation in read only mode.");
		return -EINVAL;
	}

	*tlhdr_generic.major = major;
	*tlhdr_generic.minor = minor;
	return write_tlhdr_in_tlcore();
}

static int tlhdr_get_platform_name(char *platform_name, uint32_t size)
{
	VALIDATE_PTR(platform_name, -EINVAL);
	if (!tlhdr_generic.platform_name) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (size > TLCORE_MAX_PLATFORM_NAME_SZ)
		size = TLCORE_MAX_PLATFORM_NAME_SZ;

	snprintf(platform_name, size, "%s", tlhdr_generic.platform_name);
	return 0;
}

static int tlhdr_set_platform_name(const char *platform_name)
{
	VALIDATE_PTR(platform_name, -EINVAL);
	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Invalid operation in read only mode.");
		return -EINVAL;
	}

	if (!tlhdr_generic.platform_name) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	snprintf(tlhdr_generic.platform_name, TLCORE_MAX_PLATFORM_NAME_SZ,
			"%s", platform_name);
	return write_tlhdr_in_tlcore();
}

static int tlhdr_get_product_name(char *product_name, uint32_t size)
{
	VALIDATE_PTR(product_name, -EINVAL);
	if (!tlhdr_generic.product_name) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (size > TLCORE_MAX_PRODUCT_NAME_SZ)
		size = TLCORE_MAX_PRODUCT_NAME_SZ;

	snprintf(product_name, size, "%s", tlhdr_generic.product_name);
	return 0;
}

static int tlhdr_set_product_name(const char *product_name)
{
	VALIDATE_PTR(product_name, -EINVAL);
	if (!tlhdr_generic.product_name) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Invalid operation in read only mode.");
		return -EINVAL;
	}

	snprintf(tlhdr_generic.product_name,
			TLCORE_MAX_PRODUCT_NAME_SZ, "%s", product_name);
	return write_tlhdr_in_tlcore();
}

static int tlhdr_get_build_id(char *build_id, uint32_t size)
{
	VALIDATE_PTR(build_id, -EINVAL);
	if (!tlhdr_generic.build_id) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (size > TLCORE_MAX_BUILD_ID_SZ)
		size = TLCORE_MAX_BUILD_ID_SZ;

	snprintf(build_id, size, "%s", tlhdr_generic.build_id);
	return 0;
}

static int tlhdr_set_build_id(const char *build_id)
{
	VALIDATE_PTR(build_id, -EINVAL);
	if (!tlhdr_generic.build_id) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Invalid operation in read only mode.");
		return -EINVAL;
	}

	snprintf(tlhdr_generic.build_id,
			TLCORE_MAX_BUILD_ID_SZ, "%s", build_id);
	return write_tlhdr_in_tlcore();
}

static int tlhdr_get_build_variant(char *build_variant, uint32_t size)
{
	VALIDATE_PTR(build_variant, -EINVAL);
	if (!tlhdr_generic.build_variant) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (size > TLCORE_MAX_BUILD_VARIANT_SZ)
		size = TLCORE_MAX_BUILD_VARIANT_SZ;

	snprintf(build_variant, size, "%s", tlhdr_generic.build_variant);
	return 0;
}

static int tlhdr_set_build_variant(const char *build_variant)
{
	VALIDATE_PTR(build_variant, -EINVAL);
	if (!tlhdr_generic.build_variant) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Invalid operation in read only mode.");
		return -EINVAL;
	}

	snprintf(tlhdr_generic.build_variant,
			TLCORE_MAX_BUILD_VARIANT_SZ, "%s", build_variant);
	return write_tlhdr_in_tlcore();
}

static int tlhdr_get_imei(char *imei, uint32_t size)
{
	VALIDATE_PTR(imei, -EINVAL);
	if (!tlhdr_generic.imei) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (size > TLCORE_MAX_IMEI_SZ)
		size = TLCORE_MAX_IMEI_SZ;

	snprintf(imei, size, "%s", tlhdr_generic.imei);
	return 0;
}

static int tlhdr_set_imei(const char *imei)
{
	VALIDATE_PTR(imei, -EINVAL);
	if (!tlhdr_generic.imei) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Invalid operation in read only mode.");
		return -EINVAL;
	}

	snprintf(tlhdr_generic.imei, TLCORE_MAX_IMEI_SZ, "%s", imei);
	return write_tlhdr_in_tlcore();
}

static int tlhdr_get_serial_number(char *serial_no, uint32_t size)
{
	VALIDATE_PTR(serial_no, -EINVAL);
	if (!tlhdr_generic.serial) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (size > TLCORE_MAX_SERIAL_NO_SZ)
		size = TLCORE_MAX_SERIAL_NO_SZ;

	snprintf(serial_no, size, "%s", tlhdr_generic.serial);
	return 0;
}

static int tlhdr_set_serial_number(const char *serial_no)
{
	VALIDATE_PTR(serial_no, -EINVAL);
	if (!tlhdr_generic.serial) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Invalid operation in read only mode.");
		return -EINVAL;
	}

	snprintf(tlhdr_generic.serial,
			TLCORE_MAX_SERIAL_NO_SZ, "%s", serial_no);
	return write_tlhdr_in_tlcore();
}

static int tlhdr_get_dump_result(uint32_t *dump_result)
{
	VALIDATE_PTR(dump_result, -EINVAL);
	if (!tlhdr_generic.result) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	*dump_result = *tlhdr_generic.result;
	return 0;
}

static int tlhdr_set_dump_result(uint32_t dump_result)
{
	if (!tlhdr_generic.result) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Invalid operation in read only mode.");
		return -EINVAL;
	}

	*tlhdr_generic.result = dump_result;
	return write_tlhdr_in_tlcore();
}

static int tlhdr_get_scn_count(uint32_t *scn_count)
{
	VALIDATE_PTR(scn_count, -EINVAL);
	if (!tlhdr_generic.count) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (!scn_count) {
		PRINT_ERROR("Cant get section count\n");
		return -EINVAL;
	}

	*scn_count = *tlhdr_generic.count;
	return 0;
}

int __tlhdr_inc_scn_count(void)
{
	if (!tlhdr_generic.count) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Invalid operation in read only mode.");
		return -EINVAL;
	}

	*tlhdr_generic.count += 1;
	PRINT_DEBUG(TAG, "Section count incremented to - %d\n",
			*tlhdr_generic.count);
	return write_tlhdr_in_tlcore();
}

static int tlhdr_get_signature(char *signature, uint32_t size)
{
	VALIDATE_PTR(signature, -EINVAL);
	if (!tlhdr_generic.signature) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (size > TLCORE_MAX_SIGNATURE_SZ)
		size = TLCORE_MAX_SIGNATURE_SZ;

	snprintf(signature, size, "%s", tlhdr_generic.signature);
	return 0;
}

static int tlhdr_get_crash_time(char *crash_time, uint32_t size)
{
	VALIDATE_PTR(crash_time, -EINVAL);
	if (!tlhdr_generic.crash_time) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (size > TLCORE_MAX_CRASH_TIME_SZ)
		size = TLCORE_MAX_CRASH_TIME_SZ;

	snprintf(crash_time, size, "%s", tlhdr_generic.crash_time);
	return 0;
}

static int tlhdr_set_crash_time(const char *crash_time)
{
	VALIDATE_PTR(crash_time, -EINVAL);
	if (!tlhdr_generic.crash_time) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Invalid operation in read only mode.");
		return -EINVAL;
	}

	snprintf(tlhdr_generic.crash_time,
			TLCORE_MAX_CRASH_TIME_SZ, "%s", crash_time);
	return write_tlhdr_in_tlcore();
}

static int tlhdr_get_dumper_name(char *dumper_name, uint32_t size)
{
	VALIDATE_PTR(dumper_name, -EINVAL);
	if (!tlhdr_generic.dumper_name) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (size > TLCORE_MAX_DUMPER_NAME_SZ)
		size = TLCORE_MAX_DUMPER_NAME_SZ;

	snprintf(dumper_name, size, "%s", tlhdr_generic.dumper_name);
	return 0;
}

static int tlhdr_set_dumper_name(const char *dumper_name)
{
	VALIDATE_PTR(dumper_name, -EINVAL);
	if (!tlhdr_generic.dumper_name) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Invalid operation in read only mode.");
		return -EINVAL;
	}

	snprintf(tlhdr_generic.dumper_name,
			TLCORE_MAX_DUMPER_NAME_SZ, "%s", dumper_name);
	return write_tlhdr_in_tlcore();
}

static int tlhdr_get_dumper_version(char *dumper_version, uint32_t size)
{
	VALIDATE_PTR(dumper_version, -EINVAL);
	if (!tlhdr_generic.dumper_version) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (size > TLCORE_MAX_DUMPER_VERSION_SZ)
		size = TLCORE_MAX_DUMPER_VERSION_SZ;

	snprintf(dumper_version, size, "%s", tlhdr_generic.dumper_version);
	return 0;
}

static int tlhdr_set_dumper_version(const char *dumper_version)
{
	VALIDATE_PTR(dumper_version, -EINVAL);
	if (!tlhdr_generic.dumper_name) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Invalid operation in read only mode.");
		return -EINVAL;
	}

	snprintf(tlhdr_generic.dumper_version,
			TLCORE_MAX_DUMPER_VERSION_SZ, "%s", dumper_version);
	PRINT_DEBUG(TAG, "Dumper version - %s\n", tlhdr_generic.dumper_version);
	return write_tlhdr_in_tlcore();
}

static int tlhdr_get_dump_type(uint16_t *dump_type)
{
	VALIDATE_PTR(dump_type, -EINVAL);
	if (!tlhdr_generic.dump_type) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	*dump_type = *tlhdr_generic.dump_type;
	return 0;
}

static int tlhdr_set_dump_type(uint16_t dump_type)
{
	if (dump_type >= TLCORE_DUMP_TYPE_MAX) {
		PRINT_ERROR("Invalid Arguments passed.\n");
		return -EINVAL;
	}

	if (!tlhdr_generic.dump_type) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Invalid operation in read only mode.");
		return -EINVAL;
	}

	*tlhdr_generic.dump_type = dump_type;
	return write_tlhdr_in_tlcore();
}

static int tlhdr_get_device_type(uint16_t *device_type)
{
	VALIDATE_PTR(device_type, -EINVAL);
	if (!tlhdr_generic.device_type) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	*device_type = *tlhdr_generic.device_type;
	return 0;
}

static int tlhdr_set_device_type(uint16_t device_type)
{
	if (device_type >= TLCORE_DEVICE_TYPE_MAX) {
		PRINT_ERROR("Invalid Arguments passed.\n");
		return -EINVAL;
	}

	if (!tlhdr_generic.device_type) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Invalid operation in read only mode.");
		return -EINVAL;
	}

	*tlhdr_generic.device_type = device_type;

	return write_tlhdr_in_tlcore();
}

static int tlhdr_get_arch_type(uint16_t *arch_type)
{
	VALIDATE_PTR(arch_type, -EINVAL);
	if (!tlhdr_generic.arch_type) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (!arch_type) {
		PRINT_ERROR("Cant get Device Type\n");
		return -EINVAL;
	}

	*arch_type = *tlhdr_generic.arch_type;
	return 0;
}

static int tlhdr_set_arch_type(uint16_t arch_type)
{
	if (arch_type >= TLCORE_ARCH_TYPE_MAX) {
		PRINT_ERROR("Invalid Arguments passed.\n");
		return -EINVAL;
	}

	if (!tlhdr_generic.arch_type) {
		PRINT_ERROR("Operation Not Supported\n");
		return -ENOTSUP;
	}

	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Invalid operation in read only mode.");
		return -EINVAL;
	}

	*tlhdr_generic.arch_type = arch_type;

	return write_tlhdr_in_tlcore();
}

int __tlhdr_set_sha1(const unsigned char *sha1_str, uint32_t sha1_size)
{
	VALIDATE_PTR(sha1_str, -EINVAL);
	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Invalid operation in read only mode.");
		return -EINVAL;
	}

	if (sha1_size != TLCORE_MAX_SIGNATURE_SZ) {
		PRINT_ERROR("Invalid sha1 size. SHA1 "\
			"should be of %d bytes\n", TLCORE_MAX_SIGNATURE_SZ);
		return -EINVAL;
	}

	memcpy(tlhdr_generic.signature, sha1_str, sha1_size);

	return write_tlhdr_in_tlcore();
}

static struct tlcore_header tlcore_hdr = {
	.get_version = tlhdr_get_version,
	.get_platform_name = tlhdr_get_platform_name,
	.set_platform_name = tlhdr_set_platform_name,
	.get_product_name = tlhdr_get_product_name,
	.set_product_name = tlhdr_set_product_name,
	.get_build_id	= tlhdr_get_build_id,
	.set_build_id = tlhdr_set_build_id,
	.get_build_variant = tlhdr_get_build_variant,
	.set_build_variant = tlhdr_set_build_variant,
	.get_imei = tlhdr_get_imei,
	.set_imei = tlhdr_set_imei,
	.get_serial_number = tlhdr_get_serial_number,
	.set_serial_number = tlhdr_set_serial_number,
	.get_dump_result = tlhdr_get_dump_result,
	.set_dump_result = tlhdr_set_dump_result,
	.get_scn_count = tlhdr_get_scn_count,
	.get_signature = tlhdr_get_signature,
	.get_crash_time = tlhdr_get_crash_time,
	.set_crash_time = tlhdr_set_crash_time,
	.get_dumper_name = tlhdr_get_dumper_name,
	.set_dumper_name = tlhdr_set_dumper_name,
	.get_dumper_version = tlhdr_get_dumper_version,
	.set_dumper_version = tlhdr_set_dumper_version,
	.get_dump_type = tlhdr_get_dump_type,
	.set_dump_type = tlhdr_set_dump_type,
	.get_device_type = tlhdr_get_device_type,
	.set_device_type = tlhdr_set_device_type,
	.get_arch_type = tlhdr_get_arch_type,
	.set_arch_type = tlhdr_set_arch_type,
};

static int populate_tlhdr_generic(void)
{
	PRINT_DEBUG(TAG, "TLCORE Version - %d\n", major_version);
	switch (major_version) {
	case VERSION_4:
		VALIDATE_PTR(tlhdrv4_ll, -EINVAL);
		tlhdr_generic.magic = &tlhdrv4_ll->magic;
		tlhdr_generic.major = (uint16_t *)&tlhdrv4_ll->tlcore_version;
		tlhdr_generic.minor =
			(uint16_t *)(((char *)&tlhdrv4_ll->tlcore_version) + 2);
		tlhdr_generic.dumper_version = tlhdrv4_ll->dumper_version;
		tlhdr_generic.platform_name = tlhdrv4_ll->platform;
		tlhdr_generic.product_name = tlhdrv4_ll->product;
		tlhdr_generic.build_id = tlhdrv4_ll->build_id;
		tlhdr_generic.build_variant = tlhdrv4_ll->build_variant;
		tlhdr_generic.imei = tlhdrv4_ll->imei;
		tlhdr_generic.serial = tlhdrv4_ll->serial;
		tlhdr_generic.result = &tlhdrv4_ll->result;
		tlhdr_generic.count = &tlhdrv4_ll->count;
		tlhdr_generic.signature = tlhdrv4_ll->sign;
		break;

	case VERSION_5:
		VALIDATE_PTR(tlhdrv5_ll, -EINVAL);
		tlhdr_generic.magic = &tlhdrv5_ll->magic;
		tlhdr_generic.major = &tlhdrv5_ll->major;
		tlhdr_generic.minor = &tlhdrv5_ll->minor;
		tlhdr_generic.dumper_version = tlhdrv5_ll->dumper_version;
		tlhdr_generic.platform_name = tlhdrv5_ll->platform;
		tlhdr_generic.product_name = tlhdrv5_ll->product;
		tlhdr_generic.build_id = tlhdrv5_ll->build_id;
		tlhdr_generic.build_variant = tlhdrv5_ll->build_variant;
		tlhdr_generic.imei = tlhdrv5_ll->imei;
		tlhdr_generic.serial = tlhdrv5_ll->serial;
		tlhdr_generic.result = &tlhdrv5_ll->result;
		tlhdr_generic.count = &tlhdrv5_ll->count;
		tlhdr_generic.signature = tlhdrv5_ll->sign;
		tlhdr_generic.crash_time = tlhdrv5_ll->crash_time;
		tlhdr_generic.dumper_name = tlhdrv5_ll->dumper_name;
		tlhdr_generic.dump_type = &tlhdrv5_ll->dump_type;
		tlhdr_generic.device_type = &tlhdrv5_ll->device_type;
		break;

	case VERSION_6:
		VALIDATE_PTR(tlhdrv6_ll, -EINVAL);
		tlhdr_generic.magic = &tlhdrv6_ll->magic;
		tlhdr_generic.major = &tlhdrv6_ll->major;
		tlhdr_generic.minor = &tlhdrv6_ll->minor;
		tlhdr_generic.dumper_version = tlhdrv6_ll->dumper_version;
		tlhdr_generic.platform_name = tlhdrv6_ll->platform;
		tlhdr_generic.product_name = tlhdrv6_ll->product;
		tlhdr_generic.build_id = tlhdrv6_ll->build_id;
		tlhdr_generic.build_variant = tlhdrv6_ll->build_variant;
		tlhdr_generic.imei = tlhdrv6_ll->imei;
		tlhdr_generic.serial = tlhdrv6_ll->serial;
		tlhdr_generic.result = &tlhdrv6_ll->result;
		tlhdr_generic.count = &tlhdrv6_ll->count;
		tlhdr_generic.signature = tlhdrv6_ll->sign;
		tlhdr_generic.crash_time = tlhdrv6_ll->crash_time;
		tlhdr_generic.dumper_name = tlhdrv6_ll->dumper_name;
		tlhdr_generic.dump_type = &tlhdrv6_ll->dump_type;
		tlhdr_generic.device_type = &tlhdrv6_ll->device_type;
		tlhdr_generic.arch_type = &tlhdrv6_ll->arch_type;
		break;

	default:
		PRINT_ERROR("Unknown Version - %d\n", major_version);
		return -ENOTSUP;
	}

	return 0;
}

static int get_temp_sha1(unsigned char *temp_sha1)
{
	int ret;
	uint64_t seed_val;
	uint64_t random_val;
	char seed_buf[64];
	struct timeval tv;

	VALIDATE_PTR(temp_sha1, -EINVAL);
	gettimeofday(&tv, NULL);
	srand(tv.tv_usec);
	random_val = (uint64_t)rand();
	seed_val = random_val + (tv.tv_sec * 1000000LL) + tv.tv_usec;
	snprintf(seed_buf, sizeof(seed_buf), "%"PRIu64"", seed_val);

	ret = gen_sha1_from_buf(seed_buf, strnlen(seed_buf, sizeof(seed_buf)),
								temp_sha1);
	if (ret < 0) {
		PRINT_DEBUG(TAG, "Fail to get sha1 for buffer %d\n", ret);
		return ret;
	}

	return 0;
}

struct tlcore_header *__init_tlhdr(FILE *fp, struct tlcore *tlcore,
		uint64_t offset)
{
	struct tlhdr_magic tl_magic;
	unsigned char sha1_buf[SHA_DIGEST_LENGTH];

	VALIDATE_PTR(fp, NULL);
	VALIDATE_PTR(tlcore, NULL);
	fp_core = fp;
	mode = tlcore->get_mode();
	tlhdr_offset = offset;
	switch (mode) {
	case TLCORE_READONLY:
		if (fseek(fp_core, offset, SEEK_SET) < 0) {
			PRINT_ERROR("fseek failed: %s(%d)\n",
					strerror(errno), errno);
			return NULL;
		}

		memset((void *)&tl_magic, 0, sizeof(tl_magic));
		if (fread(&tl_magic, sizeof(tl_magic), 1, fp_core) != 1) {
			if (ferror(fp))
				PRINT_ERROR("fread failed: %s(%d)\n",
						strerror(errno), errno);

			return NULL;

		}

		if (tl_magic.magic != 0xDEADBEEF) {
			PRINT_ERROR("Invalid Tlcore. Wrong magic number - %x\n",
								tl_magic.magic);
			return NULL;
		}

		major_version = tl_magic.major;
		minor_version = tl_magic.minor;
		if (fseek(fp_core, offset, SEEK_SET) < 0) {
			PRINT_ERROR("fseek failed: %s(%d)\n",
					strerror(errno), errno);
			return NULL;
		}

		if (major_version == VERSION_4) {
			tlhdrv4_ll = calloc(1, sizeof(*tlhdrv4_ll));
			if (!tlhdrv4_ll) {
				PRINT_ERROR("Failed to allocate memory:%s(%d)",
						strerror(errno), errno);
				return NULL;
			}

			if (fread(tlhdrv4_ll, sizeof(*tlhdrv4_ll), 1,
						fp_core) != 1) {
				if (ferror(fp))
					PRINT_ERROR("fread failed: %s(%d)\n",
							strerror(errno), errno);

				goto error;
			}

		} else if (major_version == VERSION_5) {
			tlhdrv5_ll = calloc(1, sizeof(*tlhdrv5_ll));
			if (!tlhdrv5_ll) {
				PRINT_ERROR("Failed to allocate memory - %s",
						strerror(errno));
				return NULL;
			}

			if (fread(tlhdrv5_ll, sizeof(*tlhdrv5_ll), 1,
						fp_core) != 1) {
				if (ferror(fp))
					PRINT_ERROR("fread failed: %s(%d)\n",
							strerror(errno), errno);

				goto error;
			}

		} else if (major_version == VERSION_6) {
			tlhdrv6_ll = calloc(1, sizeof(*tlhdrv6_ll));
			if (!tlhdrv6_ll) {
				PRINT_ERROR("Failed to allocate memory:%s(%d)",
						strerror(errno), errno);
				return NULL;
			}

			if (fread(tlhdrv6_ll, sizeof(*tlhdrv6_ll), 1,
						fp_core) != 1) {
				if (ferror(fp))
					PRINT_ERROR("fread failed: %s(%d)\n",
							strerror(errno), errno);

				goto error;
			}

		} else {
			PRINT_ERROR("Invalid/Not Supported Tlcore ver:%d.%d\n",
				major_version, minor_version);
			return NULL;
		}

		if (populate_tlhdr_generic())
			goto error;

		break;

	case TLCORE_READWRITE:
		tlhdrv6_ll = calloc(1, sizeof(*tlhdrv6_ll));
		if (!tlhdrv6_ll) {
			PRINT_ERROR("Failed to allocate memory:%s(%d)\n",
					strerror(errno), errno);
			return NULL;
		}

		major_version = LATEST_VERSION - 1;
		minor_version = 0;
		if (populate_tlhdr_generic())
			goto error;

		if (tlhdr_set_magic())
			goto error;

		if (tlhdr_set_version(major_version, minor_version))
			goto error;

		if (tlhdr_set_dump_result(DUMP_RES_UNHANDLED))
			goto error;

		if (get_temp_sha1(sha1_buf) < 0) {
			PRINT_WARN("Fail to get temporary sha1\n");
			break;
		}

		if (__tlhdr_set_sha1(sha1_buf, sizeof(sha1_buf)) < 0)
			PRINT_WARN("Fail to write sha1 into tlcore\n");

		break;

	case TLCORE_UPDATE:
		tlhdrv6_ll = calloc(1, sizeof(*tlhdrv6_ll));
		if (!tlhdrv6_ll) {
			PRINT_ERROR("Failed to allocate memory:%s(%d)",
						strerror(errno), errno);
				return NULL;
		}

		if (fread(tlhdrv6_ll, sizeof(*tlhdrv6_ll), 1,
					fp_core) != 1) {
			if (ferror(fp))
				PRINT_ERROR("fread failed: %s(%d)\n",
						strerror(errno), errno);

			goto error;
		}

		major_version = LATEST_VERSION - 1;
		minor_version = 0;

		if (populate_tlhdr_generic())
			goto error;

		if (tlhdr_set_dump_result(DUMP_RES_UNHANDLED))
			goto error;

		break;

	default:
		PRINT_ERROR("Invalid mode %d ! Mode Supported: " \
				"READ/TLCORE_READWRITE\n", mode);
		goto error;
	}
	return &tlcore_hdr;
error:
	PRINT_ERROR("Init tlhdr failed.\n");
	if (tlhdrv4_ll) {
		free(tlhdrv4_ll);
		tlhdrv4_ll = NULL;
	}

	if (tlhdrv5_ll) {
		free(tlhdrv5_ll);
		tlhdrv5_ll = NULL;
	}

	if (tlhdrv6_ll) {
		free(tlhdrv6_ll);
		tlhdrv6_ll = NULL;
	}

	return NULL;
}

static int16_t is_tlhdr_complete(void)
{
	switch (major_version) {
	case VERSION_4:
		if (!tlhdrv4_ll)
			return 0;

		if (!strnlen(tlhdrv4_ll->build_variant,
				TLCORE_MAX_BUILD_VARIANT_SZ) ||
			!strnlen(tlhdrv4_ll->build_id,
				TLCORE_MAX_BUILD_ID_SZ) ||
			!strnlen(tlhdrv4_ll->product,
				TLCORE_MAX_PRODUCT_NAME_SZ) ||
			!strnlen(tlhdrv4_ll->platform,
				TLCORE_MAX_PLATFORM_NAME_SZ))
			return 0;
		break;

	case VERSION_5:
		if (!tlhdrv5_ll)
			return 0;

		if (!strnlen(tlhdrv5_ll->build_variant,
				TLCORE_MAX_BUILD_VARIANT_SZ) ||
			!strnlen(tlhdrv5_ll->build_id,
				TLCORE_MAX_BUILD_ID_SZ) ||
			!strnlen(tlhdrv5_ll->product,
				TLCORE_MAX_PRODUCT_NAME_SZ) ||
			!strnlen(tlhdrv5_ll->platform,
				TLCORE_MAX_PLATFORM_NAME_SZ))
			return 0;
		break;

	case VERSION_6:
		if (!tlhdrv6_ll)
			return 0;

		if (!tlhdrv6_ll->arch_type ||
			!strnlen(tlhdrv6_ll->build_variant,
				TLCORE_MAX_BUILD_VARIANT_SZ) ||
			!strnlen(tlhdrv6_ll->build_id,
				TLCORE_MAX_BUILD_ID_SZ) ||
			!strnlen(tlhdrv6_ll->product,
				TLCORE_MAX_PRODUCT_NAME_SZ) ||
			!strnlen(tlhdrv6_ll->platform,
				TLCORE_MAX_PLATFORM_NAME_SZ))
			return 0;
		break;

	default:
		PRINT_ERROR("Invalid version\n");
		return 0;
	}

	return 1;
}

int __finish_tlcore_header(void)
{
	int ret = 0;
	if (!is_tlhdr_complete()) {
		PRINT_ERROR("Tlcore header is incomplete\n");
		ret = -EINVAL;
	}

	if (tlhdrv4_ll) {
		free(tlhdrv4_ll);
		tlhdrv4_ll = NULL;
	}

	if (tlhdrv5_ll) {
		free(tlhdrv5_ll);
		tlhdrv5_ll = NULL;
	}

	if (tlhdrv6_ll) {
		free(tlhdrv6_ll);
		tlhdrv6_ll = NULL;
	}

	return ret;
}
