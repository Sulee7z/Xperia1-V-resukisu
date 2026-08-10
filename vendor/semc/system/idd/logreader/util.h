/*
 * Copyright 2010 Sony Corporation
 */

/**
 * Utility functions for logreader
 *
 * @author Carl-Emil Lagerstedt (carl-emil.lagerstedt@sonyericsson.com)
 */
#ifndef UTIL_H_INCLUSION_GUARD
#define UTIL_H_INCLUSION_GUARD

#include <log/log_id.h>
#include "rules.h"

/** Using this will make the pointer point to NULL */
#define FREE(X) do {free_wrapper((void**) &X);}while(0);
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define KiB 1024

/**
 * Simple wrapper around free
 */
void free_wrapper(void ** pointer_pp);

/*
 * Returns a pointer to the complete content of the open file descriptor fd.
 * The caller shall free the returned buffer.
 * The file is closed in this function.
 * The size of returned buffer is returned in the parameter size_p.
 */
unsigned char* read_entire_file(int fd, size_t* size_p);

#endif //UTIL_H_INCLUSION_GUARD
