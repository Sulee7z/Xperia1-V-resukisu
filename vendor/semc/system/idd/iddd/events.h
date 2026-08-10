/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief Event module, for event reporting
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#ifndef EVENTS_H_INCLUSION_GUARD
#define EVENTS_H_INCLUSION_GUARD

#include <string.h>

#include "filter.h"

void remove_events(void);
void handle_event(idd_filter_t* filter, const char* package_p, const char* name_p,
                  const char* payload_p, size_t payload_size);
void handle_app_event(idd_filter_t* filter, const char* app_package_p,
                      const char* app_version_name_p, int app_version_code, const char* timestamp_p,
                      int payload_type, const char* payload_p, size_t payload_size);
void events_init(const char* base_dir_p);
int flush_events(void);
void events_shutdown(void);
int has_new_event(void);
int is_app_event(const char* package_p, const char* name_p);

#endif
