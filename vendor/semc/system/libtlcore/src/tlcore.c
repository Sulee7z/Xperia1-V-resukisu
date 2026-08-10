/*
 * Copyright (C) 2014 Sony Mobile Communications Inc.
 * All rights, including trade secret rights, reserved.
 */

#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "tlcore.h"
#include "tlcore_header.h"
#include "section_header.h"
#include "tlcore_utils.h"
#include "section.h"
#include "tlcore_internal.h"

#define TAG TLCORE

static struct tlcore_section *tlcore_append_section(uint16_t section_type);
static uint16_t tlcore_get_mode(void);
static int16_t tlcore_open;
static uint16_t major_version, minor_version, tlcore_mode;
static FILE *fp_core;

static struct tlcore tlcore_obj = {
	.append_section = tlcore_append_section,
	.get_mode = tlcore_get_mode
};

static int get_section_offsets(uint32_t cur_scn, uint64_t *scnhdr_offset,
		uint64_t *scndata_offset)
{
	int ret;
	uint32_t scn;
	uint32_t scn_count;
	int64_t core_off;
	uint64_t size = 0;
	struct section_header_v4 *scnhdrv4_ll = NULL;
	struct section_header_v5 *scnhdrv5_ll = NULL;
	struct section_header_v6 *scnhdrv6_ll = NULL;

	VALIDATE_PTR(scnhdr_offset, -EINVAL);
	VALIDATE_PTR(scndata_offset, -EINVAL);
	core_off = ftell(fp_core);
	if (core_off < 0) {
		PRINT_ERROR("ftell failed.%s(%d)\n", strerror(errno), errno);
		return -errno;
	}

	ret = tlcore_obj.tlhdr->get_scn_count(&scn_count);
	if (ret < 0) {
		PRINT_ERROR("Get section count Failed\n");
		return ret;
	}

	switch (major_version) {
	case VERSION_4:
		scnhdrv4_ll = calloc(1, sizeof(struct section_header_v4));
		if (!scnhdrv4_ll) {
			PRINT_ERROR("Failed to Allocate memory: %s(%d)\n",
					strerror(errno), errno);
			return -errno;
		}

		if (fseek(fp_core,
			sizeof(struct tlcore_header_v4), SEEK_SET) < 0) {
			PRINT_ERROR("fseek failed.%s(%d)\n",
					strerror(errno), errno);
			ret = -errno;
			goto err_v4;
		}

		for (scn = 0; scn <= cur_scn; scn++) {
			*scnhdr_offset = ((scn * sizeof(struct
					section_header_v4)) +
					(sizeof(struct tlcore_header_v4)));
			if (scn) {
				if (fread(scnhdrv4_ll,
					sizeof(struct section_header_v4),
					1, fp_core) != 1) {
					if (ferror(fp_core)) {
						PRINT_ERROR("fread failed." \
							"%s(%d)\n",
							strerror(errno), errno);
						ret = -errno;
					} else {
						ret = -EINVAL;
					}

					goto err_v4;
				}
				size = scnhdrv4_ll->written_size;
				*scndata_offset += size;
			} else {
				*scndata_offset = ((scn_count * sizeof(struct
					section_header_v4)) +
					(sizeof(struct tlcore_header_v4)));
			}

			PRINT_DEBUG(TAG, "hdr %"PRIu64" data %"PRIu64"\n",
					*scnhdr_offset,
					*scndata_offset);
		}

		free(scnhdrv4_ll);
		scnhdrv4_ll = NULL;
		break;

	case VERSION_5:
		scnhdrv5_ll = calloc(1, sizeof(struct section_header_v5));
		if (!scnhdrv5_ll) {
			PRINT_ERROR("Failed to Allocate memory: %s(%d)\n",
					strerror(errno), errno);
			return -errno;
		}

		if (fseek(fp_core,
			sizeof(struct tlcore_header_v5), SEEK_SET) < 0) {
			PRINT_ERROR("fseek failed.%s(%d)\n",
					strerror(errno), errno);
			ret = -errno;
			goto err_v5;
		}

		for (scn = 0; scn <= cur_scn; scn++) {
			*scnhdr_offset = ((scn * sizeof(struct
					section_header_v5)) +
					(sizeof(struct tlcore_header_v5)));
			if (scn) {
				if (fread(scnhdrv5_ll,
					sizeof(struct section_header_v5),
					1, fp_core) != 1) {
					if (ferror(fp_core)) {
						PRINT_ERROR("fread failed." \
							"%s(%d)\n",
							strerror(errno), errno);
						ret = -errno;
					} else {
						ret = -EINVAL;
					}

					goto err_v5;
				}
				size = scnhdrv5_ll->written_size;
				*scndata_offset += size;
			} else {
				*scndata_offset = ((scn_count * sizeof(struct
					section_header_v5)) +
					(sizeof(struct tlcore_header_v5)));
			}

			PRINT_DEBUG(TAG, "hdr %"PRIu64" data %"PRIu64"\n",
					*scnhdr_offset,
					*scndata_offset);
		}

		free(scnhdrv5_ll);
		scnhdrv5_ll = NULL;
		break;

	case VERSION_6:
		scnhdrv6_ll = calloc(1, sizeof(struct section_header_v6));
		if (!scnhdrv6_ll) {
			PRINT_ERROR("Failed to Allocate memory: %s(%d)\n",
					strerror(errno), errno);
			return -errno;
		}

		*scnhdr_offset = sizeof(struct tlcore_header_v6);
		*scndata_offset = *scnhdr_offset +
					sizeof(struct section_header_v6);

		for (scn = 0; scn < cur_scn; scn++) {
			if (fseek(fp_core, *scnhdr_offset, SEEK_SET) < 0) {
				PRINT_ERROR("fseek Failed. %s(%d)\n",
						strerror(errno), errno);
				ret = -errno;
				goto err_v6;
			}

			if (fread(scnhdrv6_ll, sizeof(struct section_header_v6),
						1, fp_core) != 1) {
					if (ferror(fp_core)) {
						PRINT_ERROR("fread failed." \
							"%s(%d)\n",
							strerror(errno), errno);
						ret = -errno;
					} else {
						ret = -EINVAL;
					}

				goto err_v6;
			}

			*scnhdr_offset = *scndata_offset +
						scnhdrv6_ll->section_size;
			*scndata_offset = *scnhdr_offset +
					sizeof(struct section_header_v6);
		}
		free(scnhdrv6_ll);
		scnhdrv6_ll = NULL;
		break;

	default:
		PRINT_ERROR("Invalid version\n");
		return -EINVAL;
	}

	if (fseek(fp_core, core_off, SEEK_SET) < 0) {
		PRINT_ERROR("fseek failed.%s(%d)\n", strerror(errno), errno);
		return -errno;
	}

	return 0;
err_v4:
	free(scnhdrv4_ll);
	scnhdrv4_ll = NULL;
	return ret;
err_v5:
	free(scnhdrv5_ll);
	scnhdrv5_ll = NULL;
	return ret;
err_v6:
	free(scnhdrv6_ll);
	scnhdrv6_ll = NULL;
	return ret;
}

static struct tlcore_section *tlcore_append_section(uint16_t section_type)
{
	int ret;
	uint32_t scn_num = 0;
	uint64_t scnhdr_offset = 0, scndata_offset = 0;
	struct tlcore_section *temp_section = NULL, *curr_section = NULL;
	struct tlcore_section *new_section = NULL;

	PRINT_DEBUG(TAG, "append start\n");
	temp_section = tlcore_obj.section_head;
	while (temp_section) {
		scn_num++;
		curr_section = temp_section;
		temp_section = temp_section->next;
	}

	if (tlcore_mode == TLCORE_READWRITE &&
			scn_num &&
			!__is_section_committed(curr_section) &&
			!__is_section_reserved(curr_section)) {
			PRINT_ERROR("commit the section before" \
					" appending new one.\n");
			return NULL;
	}

	PRINT_DEBUG(TAG, "Current section %d\n", scn_num);
	ret = get_section_offsets(scn_num, &scnhdr_offset, &scndata_offset);
	if (ret < 0) {
		PRINT_ERROR("Failed to read section-%d header and data" \
				" offsets\n", scn_num);
		return NULL;
	}

	new_section = __init_section(&tlcore_obj, fp_core,
			section_type, scnhdr_offset, scndata_offset);
	if (!new_section) {
		PRINT_ERROR("Failed to initialize section\n");
		return NULL;
	}

	if (!tlcore_obj.section_head) {
		tlcore_obj.section_head = new_section;
	} else {
		struct tlcore_section *temp = tlcore_obj.section_head;
		while (temp->next != NULL)
			temp = temp->next;

		temp->next = new_section;
	}

	PRINT_DEBUG(TAG, "append done\n");
	return new_section;
}

static uint16_t tlcore_get_mode()
{
	return tlcore_mode;
}

static int32_t rebuild_tlcore(void)
{
	uint32_t scn_cnt, scn;
	struct tlcore_header *tlcore_header = NULL;

	if (tlcore_mode == TLCORE_READWRITE)
		return -ENOTSUP;

	tlcore_header = __init_tlhdr(fp_core, &tlcore_obj, 0);
	if (!tlcore_header) {
		PRINT_ERROR("Create tlcore header object failed\n");
		return -EINVAL;
	}

	PRINT_DEBUG(TAG, "File offset after tlheader read: %"PRId64"\n",
				(int64_t)ftell(fp_core));

	tlcore_obj.tlhdr = tlcore_header;
	if (tlcore_header->get_scn_count(&scn_cnt))
		PRINT_ERROR("Failed to get section count\n");

	if (tlcore_header->get_version(&major_version, &minor_version))
		PRINT_ERROR("Failed to get version\n");

	PRINT_DEBUG(TAG, "TLCORE Version: %u.%u\n" \
			"Dump Section Count: %d\n",
			major_version, minor_version, scn_cnt);

	for (scn = 0; scn < scn_cnt; scn++) {
		PRINT_DEBUG(TAG, "section count - %d\n", scn_cnt);
		if (!tlcore_append_section(SECTION_TYPE_UNHANDLED)) {
			PRINT_ERROR("tlcore append section Failed\n");
			return -EINVAL;
		}
	}

	return 0;
}

struct tlcore *init_tlcore(const char *core_path, uint32_t mode)
{
	struct tlcore_header *tlcore_header = NULL;

	if (tlcore_open) {
		PRINT_ERROR("init_tlcore already called\n");
		return NULL;
	}

	VALIDATE_PTR(core_path, NULL);
	PRINT_DEBUG(TAG, "init start\n");
	switch (mode) {
	case TLCORE_READONLY:
		fp_core = fopen(core_path, "r");
		if (!fp_core) {
			PRINT_ERROR("Failed to open file %s\n", core_path);
			return NULL;
		}

		tlcore_mode = mode;
		if (rebuild_tlcore())
			goto out;

		break;

	case TLCORE_READWRITE:
		fp_core = fopen(core_path, "w+");
		if (!fp_core) {
			PRINT_ERROR("fopen %s failed (%s)\n", core_path,
							strerror(errno));
			return NULL;
		}

		tlcore_mode = mode;
		tlcore_header = __init_tlhdr(fp_core, &tlcore_obj, 0);
		if (!tlcore_header) {
			PRINT_ERROR("Create tlcore header object failed\n");
			goto early_out;
		}

		major_version = LATEST_VERSION - 1;
		tlcore_obj.tlhdr = tlcore_header;
		break;

	case TLCORE_UPDATE:
		fp_core = fopen(core_path, "r+");
		if (!fp_core) {
			PRINT_ERROR("fopen %s failed (%s)\n", core_path,
							strerror(errno));
			return NULL;
		}

		tlcore_mode = mode;
		if (rebuild_tlcore())
			goto out;

		break;

	default:
		PRINT_ERROR("Invalid mode %d ! Mode Supported: " \
				"READ/READWRITE\n", mode);
		return NULL;
	}

	PRINT_DEBUG(TAG, "init done\n");
	tlcore_open = 1;
	return &tlcore_obj;
out:
	__finish_tlcore_header();
early_out:
	fclose(fp_core);
	return NULL;
}

static uint16_t verify_tlcore(struct tlcore *tlcore)
{
	struct tlcore_section *scn;
	uint64_t scn_size;
	uint64_t tot_size = 0;

	VALIDATE_PTR(tlcore, 0);
	scn = tlcore->section_head;
	switch (major_version) {
	case VERSION_4:
		tot_size = sizeof(struct tlcore_header_v4);
		while (scn) {
			if (scn->header->get_size(scn, &scn_size) < 0)
				goto error;

			tot_size += sizeof(struct section_header_v4) + scn_size;
			scn = scn->next;
		}
		break;

	case VERSION_5:
		tot_size = sizeof(struct tlcore_header_v5);
		while (scn) {
			if (scn->header->get_size(scn, &scn_size) < 0)
				goto error;

			tot_size += sizeof(struct section_header_v5) + scn_size;
			scn = scn->next;
		}
		break;

	case VERSION_6:
		tot_size = sizeof(struct tlcore_header_v6);
		while (scn) {
			if (__scnhdr_get_section_size(scn, &scn_size) < 0)
				goto error;

			tot_size += sizeof(struct section_header_v6) + scn_size;
			scn = scn->next;
		}
		break;

	default:
		PRINT_ERROR("Invalid version\n");
		goto error;
	}

	if (fseek(fp_core, 0, SEEK_END)) {
		PRINT_ERROR("fseek Failed. %s\n", strerror(errno));
		goto error;
	}

	if (tot_size != (uint64_t)ftell(fp_core)) {
		PRINT_ERROR("ftell - %"PRId64" totsize - %"PRId64"\n",
				(uint64_t)ftell(fp_core), tot_size);
		goto error;
	} else {
		PRINT_DEBUG(TAG, "Tlcore is Proper.\n");
	}

	return 1;
error:
	PRINT_ERROR("Tlcore is not proper\n");
	return 0;

}

int finish_tlcore(struct tlcore *tlcore)
{
	struct tlcore_section *scn_next = NULL, *scn = NULL;
	int ret1 = 0, ret2 = 0;
	struct stat f_stat;

	VALIDATE_PTR(tlcore, -EINVAL);
	if (tlcore_mode == TLCORE_READWRITE && major_version == VERSION_6) {
		if (fstat(fileno(fp_core), &f_stat) == -1)
			PRINT_WARN("fstat Failed. %s\n", strerror(errno));
		else {
			unsigned char sha1_buf[SHA_DIGEST_LENGTH];
			int ret = 0;
			ret = gen_sha1_from_file(fp_core, 0, f_stat.st_size,
						sha1_buf);
			if (ret < 0) {
				PRINT_WARN("Fail to generate sha1\n");
				goto __finish;
			}

			ret = __tlhdr_set_sha1(sha1_buf, sizeof(sha1_buf));
			if (ret < 0)
				PRINT_WARN("Fail to write sha1 in tlcore\n");
		}
	}

__finish:
	scn = tlcore->section_head;
	verify_tlcore(tlcore);
	ret1 = __finish_tlcore_header();
	while (scn) {
		scn_next = scn->next;
		ret2 = __finish_section(scn);
		scn = scn_next;
	}

	if (fp_core)
		fclose(fp_core);

	tlcore->section_head = NULL;
	tlcore->tlhdr = NULL;
	tlcore = NULL;
	tlcore_open = 0;
	if (ret1 < 0)
		return ret1;
	else
		return ret2;
}
