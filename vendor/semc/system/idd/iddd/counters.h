/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief Counters module, for handling persistent counters.
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#ifndef COUNTERS_H_INCLUSION_GUARD
#define COUNTERS_H_INCLUSION_GUARD

#include "filter.h"

void counters_handle(idd_filter_t* filter, const char* package_p, const char* name_p, int value,
                     int is_delta);
void counters_init(void);
void counters_flush(FILE* fp);
void counters_save(void);
void counters_shutdown(void);
int is_counter_update(void);

#endif
