/*
* Copyright (C) 2014 Sony Mobile Communications AB.
* All rights, including trade secret rights, reserved.
*/

#ifndef __SECTION_HEADER__
#define __SECTION_HEADER__

#include <stdint.h>
#include <tlcore.h>

#define DATA_FORMAT_UNHANDLED 255
#define SECTION_TYPE_UNHANDLED 255
struct scn_type_dump {
	char somc_name[TLCORE_MAX_NAME_SZ];
	char plat_name[TLCORE_MAX_NAME_SZ];
	uint32_t phys_addr;
	uint32_t dump_addr;
	uint32_t raw_size;
};

struct scn_type_binary {
	char regs_name[TLCORE_MAX_NAME_SZ];
	uint32_t phys_addr;
	uint32_t dump_addr;
	uint32_t raw_size;
};

struct scn_type_text {
	char log_name[TLCORE_MAX_NAME_SZ];
	uint32_t raw_size;
};

struct scn_type_dump64 {
	char somc_name[TLCORE_MAX_NAME_SZ];
	char plat_name[TLCORE_MAX_NAME_SZ];
	uint64_t phys_addr;
	uint64_t dump_addr;
	uint64_t raw_size;
};

struct scn_type_binary64 {
	char regs_name[TLCORE_MAX_NAME_SZ];
	uint64_t phys_addr;
	uint64_t dump_addr;
	uint64_t raw_size;
};

struct dummy_v4 {
	char reserved[116];
};

union section_v4 {
	struct scn_type_dump dump;
	struct scn_type_binary binary;
	struct scn_type_text text;
	struct dummy_v4 dummy;
};

struct section_header_v4 {
	uint16_t data_type;
	uint16_t data_fmt;
	uint16_t scn_type;
	uint16_t reserved;
	uint32_t written_size;
	union section_v4 scn;
} __attribute__((packed));

struct dummy_v5 {
	char reserved[112];
};

union section_v5 {
	struct scn_type_dump dump;
	struct scn_type_binary binary;
	struct scn_type_text text;
	struct scn_type_dump64 dump64;
	struct scn_type_binary64 binary64;
	struct dummy_v5 dummy;
};

struct section_header_v5 {
	uint16_t data_type;
	uint16_t data_fmt;
	uint16_t scn_type;
	uint16_t reserved;
	uint64_t written_size;
	union section_v5 scn;
} __attribute__((packed));

struct section_header_v6 {
	uint32_t scn_type;
	uint32_t scn_attr;
	uint64_t section_size;
	uint16_t data_type;
	uint16_t data_fmt;
	char name[TLCORE_MAX_NAME_SZ_V6];
	char core_name[TLCORE_MAX_CORE_NAME_SZ];
	uint64_t phys_addr;
	uint64_t dump_addr;
	uint64_t written_size;
	uint64_t raw_size;
} __attribute__((packed));

struct tlcore_section_header *__init_section_header(struct tlcore *tlcore_obj,
		FILE *fp_core, uint16_t scn_type, uint64_t offset);
int __scnhdr_set_written_size(struct tlcore_section *scn,
		uint64_t written_size);
int __scnhdr_set_section_size(struct tlcore_section *scn,
		uint64_t section_size);
int __scnhdr_get_section_size(struct tlcore_section *scn,
		uint64_t *section_size);
void __finish_section_header(struct tlcore_section_header *section_header);
int __scnhdr_update_data_type(struct tlcore_section *scn);
int16_t __is_commit_ok(struct tlcore_section *scn);
int16_t __is_section_committed(struct tlcore_section *scn);
int16_t __is_section_reserved(struct tlcore_section *scn);
#endif
