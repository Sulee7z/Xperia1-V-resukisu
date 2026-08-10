/*
 * Copyright (C) 2014 Sony Mobile Communications Inc.
 * All rights, including trade secret rights, reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "tlcore.h"
#include "tlcore_internal.h"
#include "tlcore_header.h"
#include "section_header.h"
#include "section_data.h"

#define TAG SECTION

static uint16_t tlcore_mode;

static struct tlcore_section *section_get_next(struct tlcore_section *scn)
{
	return scn->next;
}

static int section_reserve(struct tlcore_section *scn, uint64_t reserve_size)
{
	int ret;

	VALIDATE_PTR(scn, -EINVAL);
	if (__is_section_reserved(scn)) {
		PRINT_ERROR("Section already reserved.\n");
		return -EINVAL;
	}

	if (__is_section_committed(scn)) {
		PRINT_ERROR("Section already committed.\n");
		return -EINVAL;
	}

	PRINT_DEBUG(TAG, "reserve size %"PRIu64"\n", reserve_size);

	ret = __reserve_section_data(scn, reserve_size);
	if (ret < 0) {
		PRINT_ERROR("Failed to reserve section data.\n");
		return ret;
	}

	ret = __tlhdr_inc_scn_count();
	if (ret < 0) {
		PRINT_ERROR("Failed to increase section count.\n");
		return ret;
	}

	ret = scn->header->set_data_type(scn, TLCORE_DATA_TYPE_RESERVED);
	if (ret < 0) {
		PRINT_ERROR("Failed to set section type to Reserved\n");
		return ret;
	}

	ret = __scnhdr_update_data_type(scn);
	if (ret < 0) {
		PRINT_ERROR("Failed to update the data type to Reserved\n");
		return ret;
	}

	return 0;
}

static int section_commit(struct tlcore_section *scn)
{
	int ret;
	uint16_t data_fmt;
	uint64_t scn_written_size = 0;

	VALIDATE_PTR(scn, -EINVAL);
	if (tlcore_mode == TLCORE_READONLY) {
		PRINT_ERROR("Commit is invalid operation in READONLY mode\n");
		return -EINVAL;
	}

	if (__is_section_committed(scn)) {
		PRINT_ERROR("Section is already committed.\n");
		return 0;
	}

	if (!__is_commit_ok(scn)) {
		PRINT_ERROR("Check for commit Failed.\n");
		return -EINVAL;
	}

	ret = scn->header->get_data_format(scn, &data_fmt);
	if (ret < 0)
		goto error;

	ret = scn->header->get_size(scn, &scn_written_size);
	if (ret < 0)
		goto error;

	if (data_fmt == TLCORE_DATA_FORMAT_SEGMENT && scn_written_size) {
		ret = __scndata_block_finish(scn);
		if (ret < 0)
			goto error;
	}

	ret = __reset_current_position(scn);
	if (ret < 0)
		goto error;

	if (!__is_section_reserved(scn)) {
		ret = __tlhdr_inc_scn_count();
		if (ret < 0) {
			PRINT_ERROR("Failed to update number section count\n");
			goto error;
		}
	}

	ret = __scnhdr_update_data_type(scn);
	if (ret < 0)
		goto error;

	return 0;
error:
	PRINT_ERROR("Failed in section commit\n");
	return ret;
}

struct tlcore_section *__init_section(struct tlcore *tlcore_obj, FILE *fp_core,
		uint16_t scn_type, uint64_t scnhdr_offset,
		uint64_t scndata_offset)
{

	struct tlcore_section *scn = NULL;
	struct tlcore_section_header *scn_header = NULL;
	struct tlcore_section_data *scn_data = NULL;

	VALIDATE_PTR(tlcore_obj, NULL);
	VALIDATE_PTR(fp_core, NULL);
	scn = calloc(1, sizeof(struct tlcore_section));
	if (!scn) {
		PRINT_ERROR("Fail to allocate memory for section: %s(%d)\n",
				strerror(errno), errno);
		return NULL;
	}

	tlcore_mode = tlcore_obj->get_mode();
	scn->get_next = section_get_next;
	scn->reserve = section_reserve;
	scn->commit = section_commit;
	scn_header = __init_section_header(tlcore_obj, fp_core, scn_type,
			scnhdr_offset);
	if (!scn_header) {
		PRINT_ERROR("Failed to initialize section header for" \
				"offset@%"PRIu64"", scnhdr_offset);
		goto error;
	}

	PRINT_DEBUG(TAG, "Init_section_header done @ offset %"PRIu64"\n",
			scnhdr_offset);
	scn->header = scn_header;
	scn_data = __init_section_data(tlcore_obj, fp_core, scndata_offset);
	if (!scn_data) {
		PRINT_ERROR("Failed to initialize section data for " \
				"offset@%"PRIu64"", scndata_offset);
		goto error;
	}
	PRINT_DEBUG(TAG, "Init_section_data done @ offset %"PRIu64"\n",
			scndata_offset);
	scn->data = scn_data;
	scn->next = NULL;
	return scn;
error:
	free(scn);
	scn = NULL;
	return NULL;
}

int __finish_section(struct tlcore_section *scn)
{
	int ret = 0;

	VALIDATE_PTR(scn, -EINVAL);
	if (!__is_section_reserved(scn) && !__is_section_committed(scn)) {
		PRINT_ERROR("Section Not committed.\n");
		ret = -EINVAL;
	}

	if (scn) {
		__finish_section_header(scn->header);
		__finish_section_data(scn->data);
		free(scn);
		scn = NULL;
	}

	return ret;
}
