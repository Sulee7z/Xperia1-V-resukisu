/*
 * Copyright 2013 Sony Corporation
 */

/**
 * @brief File utilities to IDD
 *
 * @author Leif Mårtensson (leif.martensson@sonymobile.com)
 */

#ifndef FILE_UTILS_H_INCLUSION_GUARD
#define FILE_UTILS_H_INCLUSION_GUARD

#include <zlib.h>

#define ADD_HEADER    1

/* Creates a IDD report.
 * @input_p File name of the input file, should be an uncompressed IDD reportr.
 * @output_p File name of the output file. Will be an unencrypted IDD report.
 * @param header If true, a IDD report header is added first if it is missing.
 * Returns 1 on success, 0 on error.
 */
int report_compact(const char* input_p, const char* output_p, int header);

#endif
