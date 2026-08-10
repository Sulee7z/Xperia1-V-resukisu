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
#include "section_header.h"

#define TAG SCN_DATA
#define OFFSET_FOR_BLKSIZE(total_bsize, count)	\
	(4 * (count + 1) + total_bsize)

struct private {
	uint64_t scn_data_offset;
	uint64_t curr_pos;
};

static uint16_t major_version, minor_version, mode;
static FILE *fp_core;

int16_t __reset_current_position(struct tlcore_section *scn)
{
	struct private *private_data = NULL;

	VALIDATE_PTR(scn, 0);
	private_data = (struct private *) scn->data->private_data;
	private_data->curr_pos = 0;
	PRINT_DEBUG(TAG, "Section current position set to 0 !!\n");
	return 0;
}

static int16_t is_write_ok(struct tlcore_section *scn, uint64_t in_buf_size)
{
	int ret;
	uint64_t scn_written_size, scn_section_size;

	VALIDATE_PTR(scn, 0);
	if (__is_section_reserved(scn)) {
		ret = scn->header->get_size(scn, &scn_written_size);
		if (ret)
			return ret;

		ret = __scnhdr_get_section_size(scn, &scn_section_size);
		if (ret)
			return ret;

		if (scn_written_size + in_buf_size > scn_section_size) {
			PRINT_ERROR("Writing more than reserved.\n");
			return 0;
		}
	} else if (scn->next) {
		PRINT_ERROR("Section should already be committed.\n");
		return 0;
	}

	return 1;
}

static int scndata_write(struct tlcore_section *scn, void *in_buf, uint64_t
		in_buf_size, uint64_t *written_size)
{
	struct private *private_data = NULL;
	int ret;
	int64_t curr_pos;
	uint16_t data_fmt;

	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("write not supported in read only mode.\n");
		return -ENOTSUP;
	}

	VALIDATE_PTR(scn, -EINVAL);
	VALIDATE_PTR(in_buf, -EINVAL);
	VALIDATE_PTR(written_size, -EINVAL);
	if (!in_buf_size) {
		PRINT_WARN("Request for writting 0 bytes to tlcore\n");
		return 0;
	}

	if (__is_section_committed(scn)) {
		PRINT_ERROR("Writing in committed section not supported\n");
		return -ENOTSUP;
	}

	ret = scn->header->get_data_format(scn, &data_fmt);
	if (ret < 0) {
		PRINT_ERROR("reading data format failed.\n");
		return ret;
	}

	if (data_fmt == DATA_FORMAT_UNHANDLED) {
		PRINT_ERROR("Data format not set.\n");
		return -EINVAL;
	}

	if (data_fmt == TLCORE_DATA_FORMAT_SEGMENT) {
		PRINT_ERROR("For writing into segmented sections,"\
			" use block_write() instead.\n");
		return -EINVAL;
	}

	if (!is_write_ok(scn, in_buf_size))
		return -EINVAL;

	private_data = (struct private *)scn->data->private_data;
	if (fseek(fp_core,
			private_data->scn_data_offset + private_data->curr_pos,
			SEEK_SET) < 0) {
		PRINT_ERROR("fseek failed : %s(%d)\n", strerror(errno), errno);
		return -errno;
	}

	if (fwrite(in_buf, in_buf_size, 1, fp_core) != 1) {
		PRINT_ERROR("fwrite failed: %s(%d)\n", strerror(errno), errno);
		curr_pos = ftell(fp_core);
		if (curr_pos < 0) {
			PRINT_ERROR("ftell failed:%s(%d)\n",
					strerror(errno), errno);
			return -errno;
		}

		*written_size = curr_pos - (private_data->scn_data_offset
						+ private_data->curr_pos);
		return -errno;
	}

	curr_pos = ftell(fp_core);
	if (curr_pos < 0) {
		PRINT_ERROR("ftell failed:%s(%d)\n", strerror(errno), errno);
		return -errno;
	}

	*written_size = curr_pos - (private_data->scn_data_offset +
			private_data->curr_pos);
	private_data->curr_pos += *written_size;
	ret = __scnhdr_set_written_size(scn, private_data->curr_pos);
	if (ret < 0) {
		PRINT_ERROR("Failed to update written size\n");
		return ret;
	}

	return 0;
}

static int scndata_read(struct tlcore_section *scn, void *out_buf, uint64_t
		out_buf_size, uint64_t *read_size)
{
	struct private *private_data = NULL;
	uint64_t scn_size, read = out_buf_size;
	int64_t curr_pos;
	int ret;
	uint16_t data_fmt;

	VALIDATE_PTR(scn, -EINVAL);
	VALIDATE_PTR(out_buf, -EINVAL);
	VALIDATE_PTR(read_size, -EINVAL);
	ret = scn->header->get_data_format(scn, &data_fmt);
	if (ret < 0) {
		PRINT_ERROR("reading data format failed.\n");
		return ret;
	}

	if (data_fmt == DATA_FORMAT_UNHANDLED) {
		PRINT_ERROR("Data format not set.\n");
		return -EINVAL;
	}

	if (data_fmt == TLCORE_DATA_FORMAT_SEGMENT) {
		PRINT_ERROR("Use block read for segmented sections.\n");
		return -EINVAL;
	}

	if (!__is_section_committed(scn)) {
		PRINT_ERROR("Reading non committed section not supported\n");
		return -ENOTSUP;
	}

	if (scn->header->get_size(scn, &scn_size)) {
		PRINT_ERROR("Failed to read section size\n");
		return -EINVAL;
	}

	private_data = (struct private *)scn->data->private_data;
	if ((private_data->curr_pos + read) > scn_size)
		read = scn_size - private_data->curr_pos;

	if (!read) {
		*read_size = read;
		private_data->curr_pos = read;
		PRINT_DEBUG(TAG, "No more data left to read.\n");
		return -ENODATA;
	}

	if (fseek(fp_core, private_data->scn_data_offset +
				private_data->curr_pos, SEEK_SET) < 0) {
		PRINT_ERROR("fseek failed: %s(%d)\n", strerror(errno), errno);
		return -errno;
	}

	if (fread(out_buf, read, 1, fp_core) != 1) {
		if (ferror(fp_core)) {
			PRINT_ERROR("fread failed: %s(%d)\n",
				strerror(errno), errno);
			return -errno;
		}

		return -EINVAL;
	}

	curr_pos = ftell(fp_core);
	if (curr_pos < 0) {
		PRINT_ERROR("ftell failed: %s(%d)\n", strerror(errno), errno);
		return -errno;
	}
	*read_size = curr_pos - (private_data->scn_data_offset +
					private_data->curr_pos);
	private_data->curr_pos += *read_size;

	return 0;
}

static int scndata_set_pos(struct tlcore_section *scn, uint64_t pos)
{
	struct private *private_data = NULL;
	uint64_t scn_written_size = 0, scn_size = 0;
	int ret;
	uint16_t data_fmt;

	VALIDATE_PTR(scn, -EINVAL);
	ret = scn->header->get_data_format(scn, &data_fmt);
	if (ret < 0) {
		PRINT_ERROR("reading data format failed.\n");
		return ret;
	}

	if (data_fmt == DATA_FORMAT_UNHANDLED) {
		PRINT_ERROR("Data format not set.\n");
		return -EINVAL;
	}

	if (data_fmt == TLCORE_DATA_FORMAT_SEGMENT) {
		PRINT_ERROR("Not a valid operation in segmented Sections.\n");
		return -EINVAL;
	}

	private_data = (struct private *)scn->data->private_data;
	if (mode == TLCORE_READONLY) {
		if (scn->header->get_size(scn, &scn_written_size)) {
			PRINT_ERROR("Failed to read section size\n");
			return -EINVAL;
		}

		if (pos < scn_written_size)
			goto setpos;
		else
			PRINT_ERROR("Failed to set poistion %"PRIu64 \
					"with scn size" \
					"%"PRIu64"\n", pos, scn_written_size);
	} else if (mode == TLCORE_READWRITE) {
		ret = __scnhdr_get_section_size(scn, &scn_size);
		if (ret < 0) {
			PRINT_ERROR("Failed to read section size\n");
			return -EINVAL;
		}

		if (pos < scn_size)
			goto setpos;
		else
			PRINT_ERROR("Failed to set poistion %"PRIu64 \
					" next scn offset is %"PRIu64 \
					"\n", pos, scn_size);
	}

	return -EINVAL;

setpos:
	private_data->curr_pos = pos;
	PRINT_DEBUG(TAG, "Position set to %"PRIu64"\n", pos);
	return 0;
}

static int scndata_get_pos(struct tlcore_section *scn, uint64_t *pos)
{
	struct private *private_data = NULL;
	int ret;
	uint16_t data_fmt;

	VALIDATE_PTR(scn, -EINVAL);
	VALIDATE_PTR(pos, -EINVAL);
	ret = scn->header->get_data_format(scn, &data_fmt);
	if (ret < 0) {
		PRINT_ERROR("reading data format failed.\n");
		return ret;
	}

	if (data_fmt == DATA_FORMAT_UNHANDLED) {
		PRINT_ERROR("Data format not set.\n");
		return -EINVAL;
	}

	if (data_fmt == TLCORE_DATA_FORMAT_SEGMENT) {
		PRINT_ERROR("Not a valid operation in segmented Sections.\n");
		return -EINVAL;
	}

	private_data = (struct private *)scn->data->private_data;
	*pos = private_data->curr_pos;
	return 0;
}

static int scndata_block_init(struct tlcore_section *scn,
		uint32_t raw_block_size)
{
	struct private *private_data = NULL;
	int ret;
	uint16_t data_fmt;

	VALIDATE_PTR(scn, -EINVAL);
	ret = scn->header->get_data_format(scn, &data_fmt);
	if (ret < 0) {
		PRINT_ERROR("reading data format failed.\n");
		return ret;
	}

	if (data_fmt == DATA_FORMAT_UNHANDLED) {
		PRINT_ERROR("Data format not set.\n");
		return -EINVAL;
	}

	if (data_fmt != TLCORE_DATA_FORMAT_SEGMENT) {
		PRINT_ERROR("This call is Valid only for segmented sections\n");
		return -EINVAL;
	}

	private_data = (struct private *)scn->data->private_data;
	if (fseek(fp_core, private_data->scn_data_offset, SEEK_SET) < 0) {
		PRINT_ERROR("fseek failed: %s(%d)\n", strerror(errno), errno);
		return -errno;
	}

	if (fwrite(&raw_block_size, sizeof(uint32_t), 1, fp_core) != 1) {
		PRINT_ERROR("fwrite failed : %s(%d)\n", strerror(errno), errno);
		return -errno;
	}

	private_data->curr_pos = sizeof(uint32_t);

	ret = __scnhdr_set_written_size(scn, private_data->curr_pos);
	if (ret < 0) {
		PRINT_ERROR("Failed to update written size\n");
		return ret;
	}

	PRINT_DEBUG(TAG, "Block init done !!!\n");
	return 0;
}

int __scndata_block_finish(struct tlcore_section *scn)
{
	struct private *private_data = NULL;
	uint32_t end = 0x0;
	int ret;
	uint16_t data_fmt;

	VALIDATE_PTR(scn, -EINVAL);
	ret = scn->header->get_data_format(scn, &data_fmt);
	if (ret < 0) {
		PRINT_ERROR("reading data format failed.\n");
		return ret;
	}

	if (data_fmt == DATA_FORMAT_UNHANDLED) {
		PRINT_ERROR("Data format not set.\n");
		return -EINVAL;
	}

	if (data_fmt != TLCORE_DATA_FORMAT_SEGMENT) {
		PRINT_ERROR("This call is Valid only for segmented sections\n");
		return -EINVAL;
	}

	private_data = (struct private *)scn->data->private_data;
	if (fseek(fp_core,
			private_data->scn_data_offset + private_data->curr_pos,
				SEEK_SET) < 0) {
		PRINT_ERROR("fseek failed: %s(%d)\n", strerror(errno), errno);
		return -errno;
	}

	if (fwrite(&end, sizeof(uint32_t), 1, fp_core) != 1) {
		PRINT_ERROR("fwrite failed: %s(%d)\n", strerror(errno), errno);
		return -errno;
	}

	private_data->curr_pos += sizeof(uint32_t);

	ret = __scnhdr_set_written_size(scn, private_data->curr_pos);
	if (ret < 0) {
		PRINT_ERROR("Failed to update written size\n");
		return ret;
	}

	PRINT_DEBUG(TAG, "Block end bytes written!!!\n");
	return 0;
}

static int scndata_get_raw_block_size(struct tlcore_section *scn, uint32_t
		*raw_block_size)
{
	struct private *private_data = NULL;
	int ret;
	uint16_t data_fmt;

	VALIDATE_PTR(scn, -EINVAL);
	ret = scn->header->get_data_format(scn, &data_fmt);
	if (ret < 0) {
		PRINT_ERROR("reading data format failed.\n");
		return ret;
	}

	if (data_fmt == DATA_FORMAT_UNHANDLED) {
		PRINT_ERROR("Data format not set.\n");
		return -EINVAL;
	}

	if (data_fmt != TLCORE_DATA_FORMAT_SEGMENT) {
		PRINT_ERROR("This call is Valid only for segmented sections\n");
		return -EINVAL;
	}

	private_data = (struct private *)scn->data->private_data;
	if (fseek(fp_core, private_data->scn_data_offset, SEEK_SET) < 0) {
		PRINT_ERROR("fseek failed: %s(%d)\n", strerror(errno), errno);
		return -errno;
	}

	if (fread(raw_block_size, sizeof(uint32_t), 1, fp_core) != 1) {
		if (ferror(fp_core)) {
			PRINT_ERROR("fread failed: %s(%d)\n",
				strerror(errno), errno);
			return -errno;
		}

		return -EINVAL;
	}

	PRINT_DEBUG(TAG, "Raw block size %u!!!\n", *raw_block_size);
	return 0;
}

static int scndata_get_block_count(struct tlcore_section *scn, uint32_t
		*block_count)
{
	struct private *private_data = NULL;
	uint64_t scn_size, total_bsize = 0;
	uint32_t count = 0, blocksize = 0;
	int ret;
	uint16_t data_fmt;

	VALIDATE_PTR(scn, -EINVAL);
	VALIDATE_PTR(block_count, -EINVAL);
	if (!__is_section_committed(scn)) {
		PRINT_ERROR("Section not committed yet.\n");
		return -EINVAL;
	}

	ret = scn->header->get_data_format(scn, &data_fmt);
	if (ret < 0) {
		PRINT_ERROR("reading data format failed.\n");
		return ret;
	}

	if (data_fmt == DATA_FORMAT_UNHANDLED) {
		PRINT_ERROR("Data format not set.\n");
		return -EINVAL;
	}

	if (data_fmt != TLCORE_DATA_FORMAT_SEGMENT) {
		PRINT_ERROR("This call is Valid only for segmented sections\n");
		return -EINVAL;
	}

	private_data = (struct private *)scn->data->private_data;
	if (scn->header->get_size(scn, &scn_size)) {
		PRINT_ERROR("Failed to read section size\n");
		return -EINVAL;
	}

	do {
		if (OFFSET_FOR_BLKSIZE(total_bsize, count) >= scn_size) {
			PRINT_ERROR("Invalid block size %"PRIu64 \
					" which is greater" \
					"than section size %"PRIu64"\n",
					total_bsize, scn_size);
			return -EINVAL;
		}

		if (fseek(fp_core, private_data->scn_data_offset +
				OFFSET_FOR_BLKSIZE(total_bsize, count),
				SEEK_SET) < 0) {
			PRINT_ERROR("fseek failed: %s(%d)\n",
						strerror(errno), errno);
			return -errno;
		}

		if (fread(&blocksize, sizeof(uint32_t), 1, fp_core) != 1) {
			if (ferror(fp_core)) {
				PRINT_ERROR("fread failed: %s(%d)\n",
						strerror(errno), errno);
				return -errno;
			}

			return -EINVAL;
		}

		PRINT_DEBUG(TAG, "blocksize %x offset %"PRIu64" count %d\n",
				blocksize, OFFSET_FOR_BLKSIZE(total_bsize,
					count), count);
		if (!blocksize) {
			PRINT_DEBUG(TAG, "Encountered NULL which should be" \
					" end of the section\n");
			break;
		}

		total_bsize += blocksize;
		count++;

	} while (blocksize != 0);

	PRINT_DEBUG(TAG, "Block count %d\n", count);
	*block_count = count;
	return 0;
}

static int scndata_max_compressed_block_size(struct tlcore_section *scn,
			uint32_t *max_block_size)
{
	struct private *private_data = NULL;
	uint64_t scn_size, total_bsize = 0;
	uint32_t count = 0, blocksize = 0;
	uint32_t max_size = 0;
	int ret;
	uint16_t data_fmt;

	VALIDATE_PTR(scn, -EINVAL);
	VALIDATE_PTR(max_block_size, -EINVAL);
	if (!__is_section_committed(scn)) {
		PRINT_ERROR("Section not committed yet.\n");
		return -EINVAL;
	}

	ret = scn->header->get_data_format(scn, &data_fmt);
	if (ret < 0) {
		PRINT_ERROR("reading data format failed.\n");
		return ret;
	}

	if (data_fmt == DATA_FORMAT_UNHANDLED) {
		PRINT_ERROR("Data format not set.\n");
		return -EINVAL;
	}

	if (data_fmt != TLCORE_DATA_FORMAT_SEGMENT) {
		PRINT_ERROR("This call is Valid only for segmented sections\n");
		return -EINVAL;
	}

	private_data = (struct private *)scn->data->private_data;
	if (scn->header->get_size(scn, &scn_size)) {
		PRINT_ERROR("Failed to read section size\n");
		return -EINVAL;
	}

	do {
		if (OFFSET_FOR_BLKSIZE(total_bsize, count) >= scn_size) {
			PRINT_ERROR("Invalid block size %"PRIu64 \
					" which is greater" \
					"than section size %"PRIu64"\n",
					total_bsize, scn_size);
			return -EINVAL;
		}

		if (fseek(fp_core, private_data->scn_data_offset +
				OFFSET_FOR_BLKSIZE(total_bsize, count),
				SEEK_SET) < 0) {
			PRINT_ERROR("fseek failed: %s(%d)\n",
						strerror(errno), errno);
			return -errno;
		}

		if (fread(&blocksize, sizeof(uint32_t), 1, fp_core) != 1) {
			if (ferror(fp_core)) {
				PRINT_ERROR("fread failed: %s(%d)\n",
						strerror(errno), errno);
				return -errno;
			}

			return -EINVAL;
		}

		PRINT_DEBUG(TAG, "blocksize %x offset %"PRIu64" count %d\n",
				blocksize, OFFSET_FOR_BLKSIZE(total_bsize,
					count), count);

		if (blocksize > max_size)
			max_size = blocksize;

		if (!blocksize) {
			PRINT_DEBUG(TAG, "Encountered NULL which should be" \
					" end of the section\n");
			break;
		}

		total_bsize += blocksize;
		count++;

	} while (blocksize != 0);
	*max_block_size = max_size;
	PRINT_DEBUG(TAG, "Maximum size - %d\n", *max_block_size);
	return 0;
}

static int scndata_block_read(struct tlcore_section *scn, void *block, uint32_t
		*block_size)
{
	uint64_t scn_size;
	uint32_t head = 4, blk_size = 0;
	struct private *private_data = NULL;
	int ret;
	uint16_t data_fmt;

	VALIDATE_PTR(scn, -EINVAL);
	VALIDATE_PTR(block, -EINVAL);
	VALIDATE_PTR(block_size, -EINVAL);
	if (!__is_section_committed(scn)) {
		PRINT_ERROR("Section not committed yet.\n");
		return -EINVAL;
	}

	ret = scn->header->get_data_format(scn, &data_fmt);
	if (ret < 0) {
		PRINT_ERROR("reading data format failed.\n");
		return ret;
	}

	if (data_fmt == DATA_FORMAT_UNHANDLED) {
		PRINT_ERROR("Data format not set.\n");
		return -EINVAL;
	}

	if (data_fmt != TLCORE_DATA_FORMAT_SEGMENT) {
		PRINT_ERROR("This call is Valid only for segmented sections\n");
		return -EINVAL;
	}

	private_data = (struct private *)scn->data->private_data;
	if (scn->header->get_size(scn, &scn_size)) {
		PRINT_ERROR("Failed to read section size\n");
		return -EINVAL;
	}

	if (fseek(fp_core, private_data->scn_data_offset + head +
			private_data->curr_pos, SEEK_SET) < 0) {
		PRINT_ERROR("fseek failed: %s(%d)\n",
					strerror(errno), errno);
		return -errno;
	}

	if (fread(&blk_size, sizeof(uint32_t), 1, fp_core) != 1) {
		if (ferror(fp_core)) {
			PRINT_ERROR("fread failed: %s(%d)\n",
					strerror(errno), errno);
			return -errno;
		}

		return -EINVAL;
	}

	*block_size = blk_size;
	private_data->curr_pos += sizeof(uint32_t);
	PRINT_DEBUG(TAG, "block size %x\n", *block_size);
	if (!*block_size) {
		private_data->curr_pos = *block_size;
		PRINT_DEBUG(TAG, "End of the section\n");
		return 0;
	}

	if (private_data->curr_pos + *block_size > scn_size) {
		PRINT_ERROR("Invalid block size %"PRIu64"\n",
				private_data->curr_pos + *block_size);
		return -EINVAL;
	}

	if (fread(block, *block_size, 1, fp_core) != 1) {
		if (ferror(fp_core)) {
			PRINT_ERROR("fread failed: %s(%d)\n",
					strerror(errno), errno);
			return -errno;
		}

		return -EINVAL;

	}

	private_data->curr_pos += *block_size;

	return 0;
}

static int scndata_block_write(struct tlcore_section *scn, void *block,
		uint32_t block_size)
{
	uint32_t blocksize;
	struct private *private_data = NULL;
	int ret;
	uint16_t data_fmt;

	VALIDATE_PTR(scn, -EINVAL);
	VALIDATE_PTR(block, -EINVAL);
	if (!block_size) {
		PRINT_WARN("Request for writting 0 bytes to tlcore\n");
		return 0;
	}

	if (__is_section_committed(scn)) {
		PRINT_ERROR("Writing into committed sections not supported.\n");
		return -ENOTSUP;
	}

	ret = scn->header->get_data_format(scn, &data_fmt);
	if (ret < 0) {
		PRINT_ERROR("reading data format failed.\n");
		return ret;
	}

	if (data_fmt == DATA_FORMAT_UNHANDLED) {
		PRINT_ERROR("Data format not set.\n");
		return -EINVAL;
	}

	if (data_fmt != TLCORE_DATA_FORMAT_SEGMENT) {
		PRINT_ERROR("This call is Valid only for segmented sections\n");
		return -EINVAL;
	}

	private_data = (struct private *)scn->data->private_data;
	if (!private_data->curr_pos) {
		PRINT_ERROR("Block section is not initilized with" \
				"raw block size\n");
		return -ENOTSUP;
	}

	if (fseek(fp_core, private_data->scn_data_offset +
			private_data->curr_pos, SEEK_SET) < 0) {
		PRINT_ERROR("fseek failed: %s(%d)\n", strerror(errno), errno);
		return -errno;
	}

	blocksize = block_size;
	if (fwrite(&blocksize, sizeof(uint32_t), 1, fp_core) != 1) {
		PRINT_ERROR("fwrite failed: %s(%d)\n", strerror(errno),	errno);
		return -errno;
	}

	private_data->curr_pos += sizeof(uint32_t);
	if (!is_write_ok(scn, blocksize)) {
		PRINT_ERROR("Invalid block size %"PRIu64"\n",
				private_data->curr_pos + blocksize);
		return -EINVAL;
	}

	if (fwrite(block, blocksize, 1, fp_core) != 1) {
		PRINT_ERROR("fwrite failed: %s(%d)\n", strerror(errno),	errno);
		return -errno;
	}

	private_data->curr_pos += blocksize;
	ret = __scnhdr_set_written_size(scn, private_data->curr_pos);
	if (ret < 0) {
		PRINT_ERROR("Failed to update written size\n");
		return ret;
	}

	return 0;
}

struct tlcore_section_data *__init_section_data(struct tlcore *tlcore_obj,
		FILE *core, uint64_t offset)
{
	struct tlcore_section_data *section_data = NULL;
	struct private *scn_privdata = NULL;

	VALIDATE_PTR(tlcore_obj, NULL);
	VALIDATE_PTR(core, NULL);
	fp_core = core;
	mode = tlcore_obj->get_mode();
	tlcore_obj->tlhdr->get_version(&major_version, &minor_version);
	section_data = calloc(1, sizeof(struct tlcore_section_data));
	if (!section_data) {
		PRINT_ERROR("Failed to allocate memory: %s(%d)\n",
				strerror(errno), errno);
		return NULL;
	}

	section_data->write = scndata_write;
	section_data->read = scndata_read;
	section_data->set_pos = scndata_set_pos;
	section_data->get_pos = scndata_get_pos;
	section_data->block_init = scndata_block_init;
	section_data->block_read = scndata_block_read;
	section_data->block_write = scndata_block_write;
	section_data->get_block_count = scndata_get_block_count;
	section_data->get_raw_block_size = scndata_get_raw_block_size;
	section_data->get_max_compressed_block_size =
			scndata_max_compressed_block_size;
	scn_privdata = calloc(1, sizeof(struct private));
	if (!scn_privdata) {
		PRINT_ERROR("Failed to allocate memory: %s(%d)\n",
				strerror(errno), errno);
		goto error;
	}

	scn_privdata->scn_data_offset = offset;
	section_data->private_data = scn_privdata;
	return section_data;

error:
	free(section_data);
	section_data = NULL;
	return NULL;
}

int __reserve_section_data(struct tlcore_section *scn, uint64_t reserve_size)
{
	char buf[4096];
	int ret;
	uint64_t size;

	VALIDATE_PTR(scn, -EINVAL);
	struct private *scn_privdata =
		(struct private *)scn->data->private_data;

	if (mode == TLCORE_READONLY) {
		PRINT_ERROR("Reserve not supported in read mode.\n");
		return -ENOTSUP;
	}

	if (__is_section_committed(scn)) {
		PRINT_ERROR("Section is already committed. Cant reserve now\n");
		return -ENOTSUP;
	}

	if (__is_section_reserved(scn)) {
		PRINT_ERROR("Section is already reserved.\n");
		return -ENOTSUP;
	}

	memset(buf, 0, sizeof(buf));
	if (fseek(fp_core,
			scn_privdata->scn_data_offset + scn_privdata->curr_pos,
				SEEK_SET) < 0) {
		PRINT_ERROR("fseek failed: %s(%d)\n", strerror(errno), errno);
		return -errno;
	}

	size = reserve_size;
	while (size > sizeof(buf)) {
		if (fwrite(buf, sizeof(buf), 1, fp_core) != 1) {
			PRINT_ERROR("fwrite failed: %s(%d)\n", strerror(errno),
						errno);
			return -errno;
		}

		size -= sizeof(buf);
	}

	if (fwrite(buf, size, 1, fp_core) != 1) {
		PRINT_ERROR("fwrite failed: %s(%d)\n", strerror(errno),	errno);
		return -errno;
	}

	ret = __scnhdr_set_section_size(scn, reserve_size);
	if (ret < 0) {
		PRINT_ERROR("Failed to update Section size\n");
		return ret;
	}

	return 0;
}

void __finish_section_data(struct tlcore_section_data *scn_data)
{
	if (scn_data) {
		free(scn_data->private_data);
		scn_data->private_data = NULL;
		free(scn_data);
		scn_data = NULL;
	}
}
