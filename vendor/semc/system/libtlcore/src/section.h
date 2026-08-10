/*
 * Copyright (C) 2014 Sony Mobile Communications AB.
 * All rights, including trade secret rights, reserved.
 */

#ifndef _SECTION_H_
#define _SECTION_H_

struct tlcore_section *__init_section(struct tlcore *tlcore_obj, FILE *fp_core,
		uint16_t scn_type, uint64_t scnhdr_offset,
		uint64_t scndata_offset);
int __finish_section(struct tlcore_section *scn);
#endif
