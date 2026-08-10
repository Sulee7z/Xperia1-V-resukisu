/*
 * Copyright (C) 2014 Sony Mobile Communications AB.
 * All rights, including trade secret rights, reserved.
 */
#ifndef _ZLIB_WRAP_H
#define _ZLIB_WRAP_H


#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

int decompress(FILE *source, FILE *dest);
void zerr(int ret);

#endif
