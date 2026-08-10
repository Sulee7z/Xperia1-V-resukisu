/*
 * Copyright (C) 2014 Sony Mobile Communications AB.
 * All rights, including trade secret rights, reserved.
 */

#ifndef _SECTION_DATA_H_
#define _SECTION_DATA_H_

struct tlcore_section_data *__init_section_data(struct tlcore *tlcore_obj,
		FILE *core, uint64_t offset);
int __reset_current_position(struct tlcore_section *scn);
int __scndata_block_finish(struct tlcore_section *scn);
int __reserve_section_data(struct tlcore_section *scn, uint64_t reserve_size);
void __finish_section_data(struct tlcore_section_data *section_data);
#endif
