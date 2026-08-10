/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief Filtering
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#ifndef FILTER_H_INCLUSION_GUARD
#define FILTER_H_INCLUSION_GUARD

#include "list.h"
#include "protos/idd/iddd.pb-c.h"

typedef enum {
  NO_CHECK_IDD_ENABLED = 0,
  CHECK_IDD_ENABLED = 1
} chk_idd_t;

typedef enum {
  COUNTER = 0,
  STATICDATA = 1,
  EVENT = 2,  /* This has to be the last filter due to dependency in filter_init() */
  MAX_FILTERS = 3
} probe_type_t;

typedef struct {
  const char* package_p;
  const char* name_p;
} idd_filter_key_t;

typedef struct {
  idd_filter_key_t key;
} idd_filter_item_t;

typedef struct {
  idd_list_t* filter;
  Iddd__Filter__Type filter_type;
  char* file; /* The file name where the filter is stored. If not given the filter module will not
                 persist/load the filter during restarts. */
} idd_filter_t;

/* The idd probe filter */
extern idd_filter_t* g_filter;

/* Initializes the given IDD filter struct and loads old filter from file
 *
 * Return 0 if the probe filter was not loaded from file.
 *        That means this is the first start. load builtin configuration.
 *        1 if the filter was loaded from file.
 */
int filter_init(idd_filter_t filter[]);

/**
 * Check if a the given probe (package + name) is active in the given IDD filter struct.
 *
 * @param filter A pointer to the IDD filter struct. The filter is either for COUNTER, STATICDATA
 *              or EVENT. It is of type INCLUDE or EXCLUDE.
 * @param package_p
 * @param name_p
 * @param check_idd_enabled If CHECK_IDD_ENABLED, IDD also has to be enabled
 *                          (data_collection_enabled).
 * @return 1 if the probe is active. 0 if it's not. The filter type, INCLUDE or EXCLUDE,
 *         determines whether the probe should be in the list.
 */
int filter_is_probe_active(idd_filter_t* filter, const char* package_p, const char* name_p,
                           chk_idd_t check_idd_enabled);

/* Loads new incoming filters from conf_X-file */
void filter_load_configuration(idd_filter_t* filter);

/**
 * Loads given IDD filter struct from given unpacked FilterConfig protobuf.
 * It will also be stored in appropriate file.
 *
 * @param filter The IDD filter struct to load
 * @param probe_type The probe type to load (and store)
 * @param fc_p FilterConfig protobuf
 */
void load_filter(idd_filter_t* filter, probe_type_t probe_type, Iddd__FilterConfig* fc_p);

/* Deinitializes given filter.
 */
void filter_shutdown(idd_filter_t* filter);

/**
 * Loads given IDD filter struct with probe filter from given unpacked FilterConfig protobuf.
 *
 * @param probe_filter The probe filter to load to the given IDD filter struct.
 * @param filter The IDD filter struct to load.
 */
void filter_create(Iddd__FilterConfig* probe_filter, idd_filter_t filter[]);


#endif
