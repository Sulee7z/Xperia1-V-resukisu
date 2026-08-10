/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief Static Data Module, for handling Static Data
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#ifndef SDATA_H_INCLUSION_GUARD
#define SDATA_H_INCLUSION_GUARD

#include "filter.h"

void sdata_handle(idd_filter_t* filter, const char* package_p, const char* name_p,
                  const char* value_p);
void sdata_init(void);
void sdata_flush(FILE* fp);
void sdata_save(void);
void sdata_shutdown(void);
int is_sdata_update(void);

#endif
