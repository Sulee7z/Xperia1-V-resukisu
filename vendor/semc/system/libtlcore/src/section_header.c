/*
 * Copyright (C) 2014 Sony Mobile Communications Inc.
 * All rights, including trade secret rights, reserved.
 */

#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "tlcore.h"
#include "tlcore_internal.h"
#include "section_header.h"
#include "section_data.h"

#define TAG SCN_HEADER

struct scnhdr_generic {
	uint16_t *data_type;
	uint16_t *data_format;
	uint16_t *scn_type;
	uint32_t *scn_type_32;
	uint32_t *scn_attr;
	uint32_t *written_size_32;
	uint64_t *written_size_64;
	uint64_t *section_size;
	char *somc_name;
	char *core_name;
	char *plat_name;
	char *log_name;
	char *regs_name;
	char *name;
	uint32_t *phys_addr_32;
	uint32_t *dump_addr_32;
	uint32_t *raw_size_32;
	uint64_t *phys_addr_64;
	uint64_t *dump_addr_64;
	uint64_t *raw_size_64;
};

struct private {
	void *scnhdr;
	uint64_t scn_hdr_offset;
	uint16_t data_type;
};

static FILE *fp_core;
static struct scnhdr_generic scnhdr_generic;
static int32_t version;
static int32_t mode;

int16_t __is_commit_ok(struct tlcore_section *scn)
{

	struct section_header_v6 *scnhdrv6_ll;

	VALIDATE_PTR(scn, 0);
	struct private *scn_privdata =
			(struct private *)scn->header->private_data;
	scnhdrv6_ll = (struct section_header_v6 *)scn_privdata->scnhdr;
	if (!scn_privdata->data_type) {
		PRINT_ERROR("Data type in section header not initialized.\n");
		return 0;
	}

	if (scnhdrv6_ll->data_fmt == DATA_FORMAT_UNHANDLED) {
		PRINT_ERROR("Data Format not set.\n");
		return 0;
	}

	if (!strnlen(scnhdrv6_ll->name, TLCORE_MAX_NAME_SZ)) {
		PRINT_ERROR("Name is not set in section header.\n");
		return 0;
	}

	if (scnhdrv6_ll->written_size && !scnhdrv6_ll->raw_size )
			PRINT_WARN("raw size in section header is not set.\n");

	return 1;
}

static uint16_t __is_write_started(struct tlcore_section *scn)
{
	struct section_header_v6 *scnhdrv6_ll;

	VALIDATE_PTR(scn, 0);
	struct private *scn_privdata =
			(struct private *)scn->header->private_data;
	scnhdrv6_ll = (struct section_header_v6 *)scn_privdata->scnhdr;
	if (scnhdrv6_ll->written_size) {
		PRINT_DEBUG(TAG, "Write started in the section\n");
		return 1;
	}
	return 0;
}

static int write_scnhdr_to_file(void *data)
{
	struct section_header_v6 *scnhdrv6_ll = NULL;

	VALIDATE_PTR(data, 0);
	struct private *scn_privdata = (struct private *)data;
	if (fseek(fp_core, scn_privdata->scn_hdr_offset, SEEK_SET) < 0) {
		PRINT_ERROR("fseek failed: %s(%d)\n", strerror(errno), errno);
		return -errno;
	}

	scnhdrv6_ll = (struct section_header_v6 *)scn_privdata->scnhdr;
	if (fwrite(scnhdrv6_ll, sizeof(struct section_header_v6), 1,
				fp_core) != 1) {
		PRINT_ERROR("fwrite failed: %s(%d)\n", strerror(errno), errno);
		return -errno;
	}

	return 0;
}

static int populate_scnhdr(void *data)
{
	struct private *scn_privdata = (struct private *)data;
	struct section_header_v4 *scnhdrv4_ll = NULL;
	struct section_header_v5 *scnhdrv5_ll = NULL;
	struct section_header_v6 *scnhdrv6_ll = NULL;

	memset(&scnhdr_generic, 0, sizeof(struct scnhdr_generic));
	VALIDATE_PTR(data, 0);
	if (version == VERSION_4) {
		scnhdrv4_ll = (struct section_header_v4 *)scn_privdata->scnhdr;
		scnhdr_generic.data_type = &scnhdrv4_ll->data_type;
		scnhdr_generic.data_format = &scnhdrv4_ll->data_fmt;
		scnhdr_generic.scn_type = &scnhdrv4_ll->scn_type;
		scnhdr_generic.written_size_32 = &scnhdrv4_ll->written_size;

		switch (*scnhdr_generic.scn_type) {
		case TYPE_DUMP:
			scnhdr_generic.somc_name =
				scnhdrv4_ll->scn.dump.somc_name;
			scnhdr_generic.plat_name =
				scnhdrv4_ll->scn.dump.plat_name;
			scnhdr_generic.phys_addr_32 =
				&scnhdrv4_ll->scn.dump.phys_addr;
			scnhdr_generic.dump_addr_32 =
				&scnhdrv4_ll->scn.dump.dump_addr;
			scnhdr_generic.raw_size_32 =
				&scnhdrv4_ll->scn.dump.raw_size;
			break;

		case TYPE_BINARY:
			scnhdr_generic.regs_name =
				scnhdrv4_ll->scn.binary.regs_name;
			scnhdr_generic.phys_addr_32 =
				&scnhdrv4_ll->scn.binary.phys_addr;
			scnhdr_generic.dump_addr_32 =
				&scnhdrv4_ll->scn.binary.dump_addr;
			scnhdr_generic.raw_size_32 =
				&scnhdrv4_ll->scn.binary.raw_size;
			break;

		case TYPE_TEXT:
			scnhdr_generic.log_name =
				scnhdrv4_ll->scn.text.log_name;
			scnhdr_generic.raw_size_32 =
				&scnhdrv4_ll->scn.text.raw_size;
			break;

		default:
			PRINT_ERROR("Invalid secion type: %d\n",
					*scnhdr_generic.scn_type);
			return -EINVAL;
		}
	} else if (version == VERSION_5) {
		scnhdrv5_ll = (struct section_header_v5 *) scn_privdata->scnhdr;
		scnhdr_generic.data_type = &scnhdrv5_ll->data_type;
		scnhdr_generic.data_format = &scnhdrv5_ll->data_fmt;
		scnhdr_generic.scn_type = &scnhdrv5_ll->scn_type;
		scnhdr_generic.written_size_64 = &scnhdrv5_ll->written_size;

		switch (*scnhdr_generic.scn_type) {
		case TYPE_DUMP:
			scnhdr_generic.somc_name =
				scnhdrv5_ll->scn.dump.somc_name;
			scnhdr_generic.plat_name =
				scnhdrv5_ll->scn.dump.plat_name;
			scnhdr_generic.phys_addr_32 =
				&scnhdrv5_ll->scn.dump.phys_addr;
			scnhdr_generic.dump_addr_32 =
				&scnhdrv5_ll->scn.dump.dump_addr;
			scnhdr_generic.raw_size_32 =
				&scnhdrv5_ll->scn.dump.raw_size;
			break;

		case TYPE_BINARY:
			scnhdr_generic.regs_name =
				scnhdrv5_ll->scn.binary.regs_name;
			scnhdr_generic.phys_addr_32 =
				&scnhdrv5_ll->scn.binary.phys_addr;
			scnhdr_generic.dump_addr_32 =
				&scnhdrv5_ll->scn.binary.dump_addr;
			scnhdr_generic.raw_size_32 =
				&scnhdrv5_ll->scn.binary.raw_size;
			break;

		case TYPE_TEXT:
			scnhdr_generic.log_name =
				scnhdrv5_ll->scn.text.log_name;
			scnhdr_generic.raw_size_32 =
				&scnhdrv5_ll->scn.text.raw_size;
			break;

		case TYPE_DUMP_64:
			scnhdr_generic.somc_name =
				scnhdrv5_ll->scn.dump64.somc_name;
			scnhdr_generic.plat_name =
				scnhdrv5_ll->scn.dump64.plat_name;
			scnhdr_generic.phys_addr_64 =
				&scnhdrv5_ll->scn.dump64.phys_addr;
			scnhdr_generic.dump_addr_64 =
				&scnhdrv5_ll->scn.dump64.dump_addr;
			scnhdr_generic.raw_size_64 =
				&scnhdrv5_ll->scn.dump64.raw_size;
			break;

		case TYPE_BINARY_64:
			scnhdr_generic.regs_name =
				scnhdrv5_ll->scn.binary64.regs_name;
			scnhdr_generic.phys_addr_64 =
				&scnhdrv5_ll->scn.binary64.phys_addr;
			scnhdr_generic.dump_addr_64 =
				&scnhdrv5_ll->scn.binary64.dump_addr;
			scnhdr_generic.raw_size_64 =
				&scnhdrv5_ll->scn.binary64.raw_size;
			break;

		default:
			PRINT_ERROR("Invalid secion type: %d\n",
					*scnhdr_generic.scn_type);
			return -EINVAL;
		}
	} else if (version == VERSION_6) {
		scnhdrv6_ll = (struct section_header_v6 *) scn_privdata->scnhdr;
		scnhdr_generic.data_type = &scnhdrv6_ll->data_type;
		scnhdr_generic.data_format = &scnhdrv6_ll->data_fmt;
		scnhdr_generic.scn_type_32 = &scnhdrv6_ll->scn_type;
		scnhdr_generic.scn_attr = &scnhdrv6_ll->scn_attr;
		scnhdr_generic.written_size_64 = &scnhdrv6_ll->written_size;
		scnhdr_generic.section_size = &scnhdrv6_ll->section_size;
		scnhdr_generic.raw_size_64 = &scnhdrv6_ll->raw_size;
		scnhdr_generic.name = scnhdrv6_ll->name;
		scnhdr_generic.core_name = scnhdrv6_ll->core_name;
		scnhdr_generic.phys_addr_64 = &scnhdrv6_ll->phys_addr;
		scnhdr_generic.dump_addr_64 = &scnhdrv6_ll->dump_addr;
	} else {
		return -EINVAL;
	}

	return 0;
}

int16_t __is_section_reserved(struct tlcore_section *scn)
{
	int ret;

	VALIDATE_PTR(scn, -EINVAL);
	ret = populate_scnhdr(scn->header->private_data);
	if (ret < 0) {
		PRINT_ERROR("Populate section header failed.\n");
		return ret;
	}

	if (!scnhdr_generic.data_type)
		goto error;

	if (*scnhdr_generic.data_type == TLCORE_DATA_TYPE_RESERVED)
		return 1;
	else
		return 0;
error:
	PRINT_ERROR("Operation not supported\n");
	return 0;
}

static int scnhdr_get_scn_attribute(struct tlcore_section *scn,
		uint32_t *scn_attr)
{
	int ret;

	VALIDATE_PTR(scn, -EINVAL);
	VALIDATE_PTR(scn_attr, -EINVAL);
	ret = populate_scnhdr(scn->header->private_data);
	if (ret < 0) {
		PRINT_ERROR("Populate section header failed.\n");
		return ret;
	}

	if (!scnhdr_generic.scn_attr)
		goto error;

	*scn_attr = *scnhdr_generic.scn_attr;
	return 0;
error:
	PRINT_ERROR("Operation not supported\n");
	return -ENOTSUP;
}

static int scnhdr_set_scn_attribute(struct tlcore_section *scn,
		uint32_t scn_attr)
{
	int ret;

	VALIDATE_PTR(scn, -EINVAL);
	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Read only Mode.\n");
		return -ENOTSUP;
	}

	if (__is_section_committed(scn)) {
		PRINT_ERROR("Section already Committed.\n");
		return -ENOTSUP;
	}

	ret = populate_scnhdr(scn->header->private_data);
	if (ret < 0) {
		PRINT_ERROR("Populate section header failed.\n");
		return ret;
	}

	if (scnhdr_generic.scn_attr)
		*scnhdr_generic.scn_attr = scn_attr;
	else
		goto error;

	return write_scnhdr_to_file(scn->header->private_data);
error:
	PRINT_ERROR("Operation not supported\n");
	return -ENOTSUP;
}

static int scnhdr_get_scn_type(struct tlcore_section *scn, uint32_t *scn_type)
{
	int ret;

	VALIDATE_PTR(scn_type, -EINVAL);
	VALIDATE_PTR(scn, -EINVAL);
	ret = populate_scnhdr(scn->header->private_data);
	if (ret < 0) {
		PRINT_ERROR("Populate section header failed.\n");
		return ret;
	}

	if ((version >=  VERSION_6) && scnhdr_generic.scn_type_32) {
		*scn_type = *scnhdr_generic.scn_type_32;
		return 0;
	} else if (scnhdr_generic.scn_type) {
		*scn_type = *scnhdr_generic.scn_type;
		return 0;
	}

	PRINT_ERROR("Operation not supported for scn_type\n");
	return -ENOTSUP;
}

static int scnhdr_get_data_format(struct tlcore_section *scn,
				uint16_t *data_format)
{
	int ret;

	VALIDATE_PTR(scn, -EINVAL);
	VALIDATE_PTR(data_format, -EINVAL);
	ret = populate_scnhdr(scn->header->private_data);
	if (ret < 0) {
		PRINT_ERROR("Populate section header failed.\n");
		return ret;
	}

	if (!scnhdr_generic.data_format) {
		PRINT_ERROR("Operation not supported\n");
		return -ENOTSUP;
	}

	*data_format = *scnhdr_generic.data_format;
	return 0;
}

static int scnhdr_set_data_format(struct tlcore_section *scn,
		uint16_t data_format)
{
	int ret;

	VALIDATE_PTR(scn, -EINVAL);
	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Read only Mode.\n");
		return -ENOTSUP;
	}

	if (data_format >= TLCORE_DATA_FORMAT_MAX) {
		PRINT_ERROR("Invalid data format passed.\n");
		return -EINVAL;
	}

	if (__is_section_committed(scn)) {
		PRINT_ERROR("Section already committed.\n");
		return -ENOTSUP;
	}

	if (__is_write_started(scn)) {
		PRINT_ERROR("Cannot change data format after write started.\n");
		return -ENOTSUP;
	}


	ret = populate_scnhdr(scn->header->private_data);
	if (ret < 0) {
		PRINT_ERROR("Populate section header failed.\n");
		return ret;
	}

	if (!scnhdr_generic.data_format) {
		PRINT_ERROR("Operation not supported\n");
		return -ENOTSUP;
	}

	*scnhdr_generic.data_format = data_format;
	return write_scnhdr_to_file(scn->header->private_data);
}

int16_t __is_section_committed(struct tlcore_section *scn)
{
	int ret;

	VALIDATE_PTR(scn, -EINVAL);
	ret = populate_scnhdr(scn->header->private_data);
	if (ret < 0) {
		PRINT_ERROR("Populate section header failed.\n");
		return ret;
	}

	if (!scnhdr_generic.data_type) {
		PRINT_ERROR("Operation not supported\n");
		return -ENOTSUP;
	}

	return !(*scnhdr_generic.data_type == TLCORE_DATA_TYPE_INCOMPLETE ||
		*scnhdr_generic.data_type == TLCORE_DATA_TYPE_RESERVED);
}

static int scnhdr_get_data_type(struct tlcore_section *scn, uint16_t *data_type)
{
	int ret;

	VALIDATE_PTR(scn, -EINVAL);
	VALIDATE_PTR(data_type, -EINVAL);
	ret = populate_scnhdr(scn->header->private_data);
	if (ret < 0) {
		PRINT_ERROR("Populate section header failed.\n");
		return ret;
	}

	if (!scnhdr_generic.data_type) {
		PRINT_ERROR("Operation not supported\n");
		return -ENOTSUP;
	}

	if (!__is_section_committed(scn)) {
		struct private *scnhdr_priv;
		scnhdr_priv = (struct private *)scn->header->private_data;
		*data_type = scnhdr_priv->data_type;
	} else {
		*data_type = *scnhdr_generic.data_type;
	}

	return 0;
}

static int scnhdr_set_data_type(struct tlcore_section *scn, uint16_t data_type)
{
	struct private *scnhdr_priv;

	VALIDATE_PTR(scn, -EINVAL);
	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Read only Mode.\n");
		return -ENOTSUP;
	}

	if (data_type >= TLCORE_DATA_TYPE_MAX) {
		PRINT_ERROR("Invalid data type being set.");
		return -EINVAL;
	}

	if (__is_section_committed(scn)) {
		PRINT_ERROR("Section already Committed.\n");
		return -ENOTSUP;
	}

	if (__is_write_started(scn)) {
		PRINT_ERROR("Can't change data type after write started.\n");
		return -ENOTSUP;
	}

	scnhdr_priv = scn->header->private_data;
	scnhdr_priv->data_type = data_type;
	return 0;
}

int __scnhdr_update_data_type(struct tlcore_section *scn)
{
	struct private *scnhdr_priv = NULL;
	int ret;

	VALIDATE_PTR(scn, -EINVAL);
	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Read only Mode.\n");
		return -ENOTSUP;
	}

	if (__is_section_committed(scn)) {
		PRINT_ERROR("Section already Committed.\n");
		return -ENOTSUP;
	}

	scnhdr_priv = scn->header->private_data;

	ret = populate_scnhdr(scnhdr_priv);
	if (ret < 0) {
		PRINT_ERROR("Unable to populate section header information\n");
		return ret;
	}

	*scnhdr_generic.data_type = scnhdr_priv->data_type;
	return write_scnhdr_to_file(scn->header->private_data);
}

static int scnhdr_get_size(struct tlcore_section *scn, uint64_t *size)
{
	int ret;

	VALIDATE_PTR(scn, -EINVAL);
	VALIDATE_PTR(size, -EINVAL);
	ret = populate_scnhdr(scn->header->private_data);
	if (ret < 0) {
		PRINT_ERROR("Populate section header failed.\n");
		return ret;
	}

	if (version == VERSION_4) {
		if (!scnhdr_generic.written_size_32)
			goto error;
		*size = *scnhdr_generic.written_size_32;
	} else {
		if (!scnhdr_generic.written_size_64)
			goto error;
		*size = *scnhdr_generic.written_size_64;
	}

	return 0;
error:
	PRINT_ERROR("Operation not supported\n");
	return -ENOTSUP;

}

static int scnhdr_get_raw_size(struct tlcore_section *scn, uint64_t *raw_size)
{
	int ret;

	VALIDATE_PTR(scn, -EINVAL);
	VALIDATE_PTR(raw_size, -EINVAL);
	ret = populate_scnhdr(scn->header->private_data);
	if (ret < 0) {
		PRINT_ERROR("Populate section header failed.\n");
		return ret;
	}


	if (version == VERSION_4) {
		if (!scnhdr_generic.raw_size_32)
			goto error;
		*raw_size = *scnhdr_generic.raw_size_32;
	} else {
		if (!scnhdr_generic.raw_size_64)
			goto error;
		*raw_size = *scnhdr_generic.raw_size_64;
	}

	return 0;
error:
	PRINT_ERROR("Operation not supported\n");
	return -ENOTSUP;
}

static int scnhdr_set_raw_size(struct tlcore_section *scn, uint64_t raw_size)
{
	int ret;

	VALIDATE_PTR(scn, -EINVAL);
	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Read only Mode.\n");
		return -ENOTSUP;
	}

	if (__is_section_committed(scn)) {
		PRINT_ERROR("Section already Committed.\n");
		return -ENOTSUP;
	}

	ret = populate_scnhdr(scn->header->private_data);
	if (ret < 0) {
		PRINT_ERROR("Populate section header failed.\n");
		return ret;
	}

	if (scnhdr_generic.raw_size_64)
		*scnhdr_generic.raw_size_64 = raw_size;
	else if (scnhdr_generic.raw_size_32)
		*scnhdr_generic.raw_size_32 = raw_size;
	else
		goto error;

	return write_scnhdr_to_file(scn->header->private_data);
error:
	PRINT_ERROR("Operation not supported\n");
	return -ENOTSUP;
}

static int scnhdr_get_phys_addr(struct tlcore_section *scn, uint64_t *phys_addr)
{
	int ret;

	VALIDATE_PTR(scn, -EINVAL);
	VALIDATE_PTR(phys_addr, -EINVAL);
	ret = populate_scnhdr(scn->header->private_data);
	if (ret < 0) {
		PRINT_ERROR("Populate section header failed.\n");
		return ret;
	}

	if (version == VERSION_4) {
		if (!scnhdr_generic.phys_addr_32)
			goto error;
		*phys_addr = *scnhdr_generic.phys_addr_32;
	} else {
		if (!scnhdr_generic.phys_addr_64)
			goto error;
		*phys_addr = *scnhdr_generic.phys_addr_64;
	}

	return 0;
error:
	PRINT_ERROR("Operation not supported\n");
	return -ENOTSUP;
}

static int scnhdr_set_phys_addr(struct tlcore_section *scn, uint64_t phys_addr)
{
	int ret;

	VALIDATE_PTR(scn, -EINVAL);
	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Read only Mode.\n");
		return -ENOTSUP;
	}

	if (__is_section_committed(scn)) {
		PRINT_ERROR("Section already Committed.\n");
		return -ENOTSUP;
	}

	ret = populate_scnhdr(scn->header->private_data);
	if (ret < 0) {
		PRINT_ERROR("Populate section header failed.\n");
		return ret;
	}

	if (scnhdr_generic.phys_addr_64)
		*scnhdr_generic.phys_addr_64 = phys_addr;
	else if (scnhdr_generic.phys_addr_32)
		*scnhdr_generic.phys_addr_32 = phys_addr;
	else
		goto error;

	return write_scnhdr_to_file(scn->header->private_data);
error:
	PRINT_ERROR("Operation not supported\n");
	return -ENOTSUP;
}

static int scnhdr_get_dump_addr(struct tlcore_section *scn, uint64_t *dump_addr)
{
	int ret;

	VALIDATE_PTR(scn, -EINVAL);
	VALIDATE_PTR(dump_addr, -EINVAL);
	ret = populate_scnhdr(scn->header->private_data);
	if (ret < 0) {
		PRINT_ERROR("Populate section header failed.\n");
		return ret;
	}

	if (version == VERSION_4) {
		if (!scnhdr_generic.dump_addr_32)
			goto error;
		*dump_addr = *scnhdr_generic.dump_addr_32;
	} else {
		if (!scnhdr_generic.dump_addr_64)
			goto error;
		*dump_addr = *scnhdr_generic.dump_addr_64;
	}

	return 0;
error:
	PRINT_ERROR("Operation not supported\n");
	return -ENOTSUP;
}

static int scnhdr_set_dump_addr(struct tlcore_section *scn, uint64_t dump_addr)
{
	int ret;

	VALIDATE_PTR(scn, -EINVAL);
	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Read only Mode.\n");
		return -ENOTSUP;
	}

	if (__is_section_committed(scn)) {
		PRINT_ERROR("Section already Committed.\n");
		return -ENOTSUP;
	}

	ret = populate_scnhdr(scn->header->private_data);
	if (ret < 0) {
		PRINT_ERROR("Populate section header failed.\n");
		return ret;
	}

	if (scnhdr_generic.dump_addr_32) {
		*scnhdr_generic.dump_addr_32 = dump_addr;
	} else if (scnhdr_generic.dump_addr_64) {
		*scnhdr_generic.dump_addr_64 = dump_addr;
	} else {
		PRINT_ERROR("Invalid arguments.\n");
		return -EINVAL;
	}

	return write_scnhdr_to_file(scn->header->private_data);
}

static int scnhdr_get_name(struct tlcore_section *scn, char *name,
							uint32_t size)
{
	int ret;

	VALIDATE_PTR(scn, -EINVAL);
	VALIDATE_PTR(name, -EINVAL);
	ret = populate_scnhdr(scn->header->private_data);
	if (ret < 0) {
		PRINT_ERROR("Populate section header failed.\n");
		return ret;
	}

	if (size > TLCORE_MAX_NAME_SZ)
		size = TLCORE_MAX_NAME_SZ;

	if (scnhdr_generic.regs_name &&
			strnlen(scnhdr_generic.regs_name,
				TLCORE_MAX_NAME_SZ))
		snprintf(name, size, "%s", scnhdr_generic.regs_name);
	else if (scnhdr_generic.log_name &&
			strnlen(scnhdr_generic.log_name,
				TLCORE_MAX_NAME_SZ))
		snprintf(name, size, "%s", scnhdr_generic.log_name);
	else if (scnhdr_generic.plat_name &&
			strnlen(scnhdr_generic.plat_name,
				TLCORE_MAX_PLATFORM_NAME_SZ))
		snprintf(name, size, "%s", scnhdr_generic.plat_name);
	else if (scnhdr_generic.name &&
			strnlen(scnhdr_generic.name, TLCORE_MAX_NAME_SZ))
		snprintf(name, size, "%s", scnhdr_generic.name);
	else
		goto error;

	return 0;
error:
	PRINT_ERROR("Name not set for this section.\n");
	return -EINVAL;
}

static int scnhdr_set_name(struct tlcore_section *scn, const char *name)
{
	int ret;

	VALIDATE_PTR(scn, -EINVAL);
	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Read only Mode.\n");
		return -ENOTSUP;
	}

	if (__is_section_committed(scn)) {
		PRINT_ERROR("Section already Committed.\n");
		return -ENOTSUP;
	}

	if (!name) {
		PRINT_ERROR("Pointer to name is NULL.\n");
		return -ENOTSUP;
	}

	ret = populate_scnhdr(scn->header->private_data);
	if (ret < 0) {
		PRINT_ERROR("Populate section header failed.\n");
		return ret;
	}

	snprintf(scnhdr_generic.name, TLCORE_MAX_NAME_SZ_V6, "%s", name);
	return write_scnhdr_to_file(scn->header->private_data);
}

static int scnhdr_get_core_name(struct tlcore_section *scn, char *name,
						uint32_t size)
{
	int ret;

	VALIDATE_PTR(scn, -EINVAL);
	VALIDATE_PTR(name, -EINVAL);
	ret = populate_scnhdr(scn->header->private_data);
	if (ret < 0) {
		PRINT_ERROR("Populate section header failed.\n");
		return ret;
	}

	if (size > TLCORE_MAX_CORE_NAME_SZ)
		size = TLCORE_MAX_CORE_NAME_SZ;

	if ((version < VERSION_6) && scnhdr_generic.somc_name &&
			strnlen(scnhdr_generic.somc_name,
				TLCORE_MAX_CORE_NAME_SZ))
		snprintf(name, size, "%s", scnhdr_generic.somc_name);
	else if ((version >= VERSION_6) && scnhdr_generic.core_name &&
			strnlen(scnhdr_generic.core_name,
				TLCORE_MAX_CORE_NAME_SZ))
		snprintf(name, size, "%s", scnhdr_generic.core_name);
	else
		goto error;

	return 0;
error:
	PRINT_INFO("No core name set for this section\n");
	return -EINVAL;
}

static int scnhdr_set_core_name(struct tlcore_section *scn, const char *name)
{
	int ret;

	VALIDATE_PTR(scn, -EINVAL);
	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Read only Mode.\n");
		return -ENOTSUP;
	}

	if (__is_section_committed(scn)) {
		PRINT_ERROR("Section already Committed.\n");
		return -ENOTSUP;
	}

	if (!name) {
		PRINT_ERROR("Pointer to name is NULL.\n");
		return -ENOTSUP;
	}

	ret = populate_scnhdr(scn->header->private_data);
	if (ret < 0) {
		PRINT_ERROR("Populate section header failed.\n");
		return ret;
	}

	snprintf(scnhdr_generic.core_name, TLCORE_MAX_CORE_NAME_SZ, "%s", name);
	return write_scnhdr_to_file(scn->header->private_data);
}

struct tlcore_section_header *__init_section_header(struct tlcore *tlcore_obj,
		FILE *core, uint16_t scn_type, uint64_t offset)
{
	struct section_header_v4 *scnhdrv4_ll = NULL;
	struct section_header_v5 *scnhdrv5_ll = NULL;
	struct section_header_v6 *scnhdrv6_ll = NULL;
	uint16_t major_version, minor_version;
	struct tlcore_section_header *section_header = NULL;
	struct private *scn_privdata = NULL;

	VALIDATE_PTR(tlcore_obj, NULL);
	VALIDATE_PTR(core, NULL);
	fp_core = core;
	section_header = calloc(1, sizeof(struct tlcore_section_header));
	if (!section_header) {
		PRINT_ERROR("Failed to allocate section header: %s(%d)\n",
					strerror(errno), errno);
		return NULL;
	}

	section_header->get_scn_type = scnhdr_get_scn_type;
	section_header->get_data_format = scnhdr_get_data_format;
	section_header->set_data_format = scnhdr_set_data_format;
	section_header->get_data_type = scnhdr_get_data_type;
	section_header->set_data_type = scnhdr_set_data_type;
	section_header->get_size = scnhdr_get_size;
	section_header->get_raw_size = scnhdr_get_raw_size;
	section_header->set_raw_size = scnhdr_set_raw_size;
	section_header->get_phys_addr = scnhdr_get_phys_addr;
	section_header->set_phys_addr = scnhdr_set_phys_addr;
	section_header->get_dump_addr = scnhdr_get_dump_addr;
	section_header->set_dump_addr = scnhdr_set_dump_addr;
	section_header->get_name = scnhdr_get_name;
	section_header->set_name = scnhdr_set_name;
	section_header->set_scn_attribute = scnhdr_set_scn_attribute;
	section_header->get_scn_attribute = scnhdr_get_scn_attribute;
	section_header->get_core_name = scnhdr_get_core_name;
	section_header->set_core_name = scnhdr_set_core_name;

	tlcore_obj->tlhdr->get_version(&major_version, &minor_version);
	if (fseek(fp_core, offset, SEEK_SET) < 0) {
		PRINT_ERROR("fseek failed: %s(%d)\n", strerror(errno), errno);
		goto early_out;
	}

	scn_privdata = calloc(1, sizeof(struct private));
	if (!scn_privdata) {
		PRINT_ERROR("Failed to allocate section private data: %s(%d)\n",
				strerror(errno), errno);
		goto early_out;
	}

	version = major_version;
	mode = tlcore_obj->get_mode();
	switch (mode) {
	case TLCORE_READONLY:
		if (version == VERSION_4) {
			scnhdrv4_ll = calloc(1,
				sizeof(struct section_header_v4));
			if (!scnhdrv4_ll) {
				PRINT_ERROR("Failed to allocate section " \
				"header v4: %s(%d)\n", strerror(errno), errno);
				goto out;
			}
			PRINT_DEBUG(TAG, "size of scnhdr %zd\n",
					sizeof(struct section_header_v4));
			if (fread(scnhdrv4_ll, sizeof(struct section_header_v4)
					, 1, fp_core) != 1) {
				if (ferror(fp_core))
					PRINT_ERROR("fread @-%"PRIu64" failed" \
						"with error %s(%d)\n", offset,
						strerror(errno), errno);
				free(scnhdrv4_ll);
				scnhdrv4_ll = NULL;
				goto out;
			}
			scn_privdata->scnhdr = scnhdrv4_ll;
		} else if (version == VERSION_5) {
			scnhdrv5_ll = calloc(1,
				sizeof(struct section_header_v5));
			if (!scnhdrv5_ll) {
				PRINT_ERROR("Failed to allocate section " \
				"header v5: %s(%d)\n", strerror(errno), errno);
				goto out;
			}

			PRINT_DEBUG(TAG, "size of scnhdr %zd\n",
					sizeof(struct section_header_v5));
			if (fread(scnhdrv5_ll, sizeof(struct section_header_v5),
					1, fp_core) != 1) {
				if (ferror(fp_core))
					PRINT_ERROR("fread @-%"PRIu64" failed" \
						"with error %s(%d)\n", offset,
						strerror(errno), errno);
				free(scnhdrv5_ll);
				scnhdrv5_ll = NULL;
				goto out;
			}
			scn_privdata->scnhdr = scnhdrv5_ll;
		} else if (version == VERSION_6) {
			scnhdrv6_ll = calloc(1,
				sizeof(struct section_header_v6));
			if (!scnhdrv6_ll) {
				PRINT_ERROR("Failed to allocate section" \
				"header V6 %s(%d)\n", strerror(errno), errno);
				goto out;
			}
			PRINT_DEBUG(TAG, "size of scnhdr %zd\n",
					sizeof(struct section_header_v6));
			if (fread(scnhdrv6_ll, sizeof(struct section_header_v6),
					1, fp_core) != 1) {
				if (ferror(fp_core))
					PRINT_ERROR("fread @-%"PRIu64" failed" \
						"with error %s(%d)\n", offset,
						strerror(errno), errno);
				free(scnhdrv6_ll);
				scnhdrv6_ll = NULL;
				goto out;
			}

			scn_privdata->scnhdr = scnhdrv6_ll;
			if (scnhdrv6_ll->data_type == TLCORE_DATA_TYPE_RESERVED)
				scn_privdata->data_type =
					scnhdrv6_ll->data_type;
		} else {
			goto out;
		}
		break;

	case TLCORE_READWRITE:
		if (version != LATEST_VERSION - 1) {
			PRINT_ERROR("Write supported only in latest version\n");
			goto out;
		}

		if (scn_type != TLCORE_SECTION_TYPE_TEXT &&
				scn_type != TLCORE_SECTION_TYPE_BIN) {
			PRINT_ERROR("Invalid section type %d\n", scn_type);
			goto out;
		}

		scnhdrv6_ll = calloc(1, sizeof(struct section_header_v6));
		if (!scnhdrv6_ll) {
			PRINT_ERROR("Failed to allocate section" \
			"header V6 %s(%d)\n", strerror(errno), errno);
			goto out;
		}

		scnhdrv6_ll->scn_type = scn_type;
		scnhdrv6_ll->data_fmt = DATA_FORMAT_UNHANDLED;
		if (fwrite(scnhdrv6_ll, sizeof(struct section_header_v6),
			1, fp_core) != 1) {
			PRINT_ERROR("fwrite @-%"PRIu64" failed with error" \
					"%s(%d)\n", offset,
					strerror(errno), errno);
			free(scnhdrv6_ll);
			scnhdrv6_ll = NULL;
			goto out;
		}

		scn_privdata->scnhdr = scnhdrv6_ll;
		break;

	case TLCORE_UPDATE:
		if (version != LATEST_VERSION - 1) {
			PRINT_ERROR("Write supported only in latest version\n");
			goto out;
		}

		scnhdrv6_ll = calloc(1, sizeof(struct section_header_v6));
		if (!scnhdrv6_ll) {
			PRINT_ERROR("Failed to allocate section" \
			"header V6 %s(%d)\n", strerror(errno), errno);
			goto out;
		}

		if (scn_type == SECTION_TYPE_UNHANDLED) {
			if (fread(scnhdrv6_ll, sizeof(struct section_header_v6),
					1, fp_core) != 1) {
				if (ferror(fp_core))
					PRINT_ERROR("fread @-%"PRIu64" failed" \
					" with error %s(%d)\n", offset,
					strerror(errno), errno);
				free(scnhdrv6_ll);
				scnhdrv6_ll = NULL;
				goto out;
			}

			scn_privdata->scnhdr = scnhdrv6_ll;
			if (scnhdrv6_ll->data_type == TLCORE_DATA_TYPE_RESERVED)
				scn_privdata->data_type =
					scnhdrv6_ll->data_type;
			break;
		}

		if (scn_type != TLCORE_SECTION_TYPE_TEXT &&
				scn_type != TLCORE_SECTION_TYPE_BIN) {
			PRINT_ERROR("Invalid section type %d\n", scn_type);
			free(scnhdrv6_ll);
			goto out;
		}

		scnhdrv6_ll->scn_type = scn_type;
		scnhdrv6_ll->data_fmt = DATA_FORMAT_UNHANDLED;
		if (fwrite(scnhdrv6_ll, sizeof(struct section_header_v6),
			1, fp_core) != 1) {
			PRINT_ERROR("fwrite @-%"PRIu64" failed with error" \
					"%s(%d)\n", offset,
					strerror(errno), errno);
			free(scnhdrv6_ll);
			scnhdrv6_ll = NULL;
			goto out;
		}

		scn_privdata->scnhdr = scnhdrv6_ll;
		break;

	default:
		PRINT_ERROR("Invalid Mode. %d\n", mode);
		goto out;
	}

	scn_privdata->scn_hdr_offset = offset;
	section_header->private_data = scn_privdata;

	return section_header;

out:
	free(scn_privdata);
	scn_privdata = NULL;

early_out:
	free(section_header);
	section_header = NULL;
	return NULL;
}

int __scnhdr_get_section_size(struct tlcore_section *scn,
		uint64_t *section_size)
{
	int ret = 0;

	VALIDATE_PTR(scn, -EINVAL);
	VALIDATE_PTR(section_size, -EINVAL);
	ret = populate_scnhdr(scn->header->private_data);
	if (ret < 0) {
		PRINT_ERROR("Populate section header failed.\n");
		return ret;
	}

	if (!scnhdr_generic.section_size)
		goto error;

	*section_size = *scnhdr_generic.section_size;
	return 0;
error:
	PRINT_ERROR("Operation not supported\n");
	return -ENOTSUP;

}

int __scnhdr_set_section_size(struct tlcore_section *scn, uint64_t section_size)
{
	int ret;

	VALIDATE_PTR(scn, -EINVAL);
	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Read only Mode.\n");
		return -ENOTSUP;
	}

	if (__is_section_committed(scn)) {
		PRINT_ERROR("Section already Committed.\n");
		return -ENOTSUP;
	}

	ret = populate_scnhdr(scn->header->private_data);
	if (ret < 0) {
		PRINT_ERROR("Populate section header failed.\n");
		return ret;
	}

	if (!scnhdr_generic.section_size) {
		PRINT_ERROR("Invalid section size\n");
		return -EINVAL;
	}

	*scnhdr_generic.section_size = section_size;
	return write_scnhdr_to_file(scn->header->private_data);
}

int __scnhdr_set_written_size(struct tlcore_section *scn, uint64_t written_size)
{
	int ret;

	VALIDATE_PTR(scn, -EINVAL);
	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Read only Mode.\n");
		return -ENOTSUP;
	}

	if (__is_section_committed(scn)) {
		PRINT_ERROR("Section already Committed.\n");
		return -ENOTSUP;
	}

	ret = populate_scnhdr(scn->header->private_data);
	if (ret < 0) {
		PRINT_ERROR("Populate section header failed.\n");
		return ret;
	}

	if (!scnhdr_generic.written_size_64) {
		PRINT_ERROR("Invalid size to be written\n");
		return -EINVAL;
	}

	*scnhdr_generic.written_size_64 = written_size;
	if (!__is_section_reserved(scn)) {
		if (!scnhdr_generic.section_size) {
			PRINT_ERROR("Operation not supported\n");
			return -ENOTSUP;
		}
		*scnhdr_generic.section_size = written_size;
	}
	return write_scnhdr_to_file(scn->header->private_data);
}

void __finish_section_header(struct tlcore_section_header *scn_hdr)
{
	if (scn_hdr) {
		struct private *priv = (struct private *)scn_hdr->private_data;
		free(priv->scnhdr);
		priv->scnhdr = NULL;
		free(priv);
		priv = NULL;
		free(scn_hdr);
		scn_hdr = NULL;
	}
}
