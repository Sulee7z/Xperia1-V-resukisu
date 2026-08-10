/*
 * Copyright 2020 Sony Corporation
 */

#ifndef SECURE_MODE_H_INCLUSION_GUARD
#define SECURE_MODE_H_INCLUSION_GUARD

#include "protos/common.pb-c.h"

/*
 * Return DEBUG or LIVE as secure mode.
 */
Idd__Header__SecureMode get_secure_mode(void);

/*
 * Return true if secure mode is live.
 */
bool is_secure_mode_live(void);

#endif
