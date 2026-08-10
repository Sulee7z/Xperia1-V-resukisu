/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief Filtering
 *
 * Filtering of events, so that we know what we record.
 *
 * Notes about the implementation: The configuration is given to us
 * as a protocol buffer. We sort the contents in it and save it as
 * 'filters.txt', which we can load much faster (since it's already
 * sorted) compared to loading the protocol buffer every time.
 *
 * The protocol buffer also includes the filter type - include or
 * exclude. We store a special filter name, called __type::exclude
 * if it is of type 'exclude'. If that one is not present, the filter
 * is of type 'include'.
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include "protos/idd/iddd.pb-c.h"
#include "list.h"
#include "configuration.h"
#include "config.h"
#include "util.h"
#include "filter.h"

#define LOG_TAG "iddd-filter"
#include "logging.h"

#define DEFAULT_FILTER_TYPE IDDD__FILTER__TYPE__INCLUDE

/* Probe filter for IDD */
static idd_filter_t idd_filter[MAX_FILTERS] =
  {{NULL, DEFAULT_FILTER_TYPE, COUNTERS_FILTER_LIST_FNAME},
   {NULL, DEFAULT_FILTER_TYPE, STATICDATA_FILTER_LIST_FNAME},
   {NULL, DEFAULT_FILTER_TYPE, EVENTS_FILTER_LIST_FNAME}
  };
idd_filter_t* g_filter = &idd_filter[0];

/**
 * Compare two idd_filter_t elements on package and filter
 * specification (name). Supports simple wildcard matching.
 *
 * @param lhs_p pointer to first idd_filter_t element
 * @param rhs_p pointer to second idd_filter_t element
 *
 * @return 0 if equal, normal strcmp semantics
 */
static int compare_filter(const void* lhs_p, const void* rhs_p)
{
  int diff = 0;
  const idd_filter_key_t* key1_p = (const idd_filter_key_t*)lhs_p;
  const idd_filter_key_t* key2_p = (const idd_filter_key_t*)rhs_p;

  if (key1_p->package_p[0] != '*' && key2_p->package_p[0] != '*')
  {
    diff = strcmp(key1_p->package_p, key2_p->package_p);

    if (diff == 0)
    {
      if (key1_p->name_p[0] != '*' && key2_p->name_p[0] != '*')
      {
        diff = strcmp(key1_p->name_p, key2_p->name_p);
      }
    }
  }

  return diff;
}

/**
 * Create an idd_filter_t element.
 *
 * @param package_p name of the package
 * @param name_p name of the event(s) to be filtered
 *
 * @return a new idd_filter_item_t element or NULL if memory alloc failed
 */
static idd_filter_item_t* create_filter(const char* package_p, const char* name_p)
{
  size_t package_size = strlen(package_p);
  size_t name_size = strlen(name_p);
  idd_filter_item_t* filter_p = malloc(sizeof(*filter_p) + package_size + 1 + name_size + 1);

  if (filter_p != NULL)
  {
    char* filter_package_p = (char*)filter_p + sizeof(*filter_p);
    char* filter_name_p = filter_package_p + package_size + 1;
    filter_p->key.package_p = filter_package_p;
    filter_p->key.name_p = filter_name_p;
    memcpy(filter_package_p, package_p, package_size + 1);
    memcpy(filter_name_p, name_p, name_size + 1);
  }
  return filter_p;
}

/**
 * Implementation of the callback idd_list_load_t from list.h.
 *
 * @param this_record_p package name
 * @param next_record_p name of the event filter
 * @param userdata_p always NULL
 *
 * @return a pointer to a new idd_filter_item_t element
 */
static void* load_filter_list_cb(char* this_record_p, char* next_record_p, void* userdata_p)
{
  const char* package_p;
  const char* name_p = NULL;
  idd_filter_item_t* filter_p = NULL;

  (void)userdata_p;
  package_p = this_record_p;
  this_record_p += strlen(this_record_p) + 1;
  if (this_record_p < next_record_p)
  {
    name_p = this_record_p;
    this_record_p += strlen(this_record_p) + 1;
  }

  if (name_p != NULL)
  {
    filter_p = create_filter(package_p, name_p);
    if (filter_p != NULL)
    {
      IDD_LOGD("Loading filter: %s::%s", package_p, name_p);
    }
  }
  else
  {
    IDD_LOGE("Failed to parse config line.");
  }
  return filter_p;
}

/**
 * Callback implementation of the callback idd_list_save_t from list.h.
 *
 * @param fp file to write to
 * @param element_p pointer to the idd_filter_t element to serialize
 * @param userdata_p always NULL
 *
 * @return 1 if success, 0 if failure
 */
static int save_filter_list_cb(FILE* fp, const void* element_p, void* userdata_p)
{
  idd_filter_item_t* filter_p = (idd_filter_item_t*)element_p;

  (void)userdata_p;

  if(fprintf(fp, "%s%c%s%c%c%c", filter_p->key.package_p, 0, filter_p->key.name_p,
          0, 0, '\n') < 0)
  {
    IDD_LOGE("Failed to write to filter list file.");
    return 0;
  }

  return 1;
}

/**
 * Create a list using filter settings.
 */
static idd_list_t* create_filter_list(void)
{
  return list_create(FILTER_LIST_INITIAL_SIZE,
                     FILTER_LIST_GROW_BY,
                     compare_filter,
                     NULL);
}

/**
 * Check if a filter is present for a given probe in given list.
 *
 * @param list_p
 * @param package_p
 * @param name_p
 *
 * @return 1 if the filter is present in the list, otherwise 0
 */
static int filter_is_present(idd_list_t* list_p, const char* package_p, const char* name_p)
{
  idd_filter_key_t key;
  idd_filter_item_t* item_p;
  key.package_p = package_p;
  key.name_p = name_p;

  item_p = list_find(list_p, &key);
  return item_p != NULL;
}

/**
 */
int filter_is_probe_active(idd_filter_t* filter, const char* package_p, const char* name_p,
                           chk_idd_t check_idd_enabled)
{
  int is_active = 0;
  if ((check_idd_enabled == NO_CHECK_IDD_ENABLED) || is_data_collection_enabled())
  {
    int present = filter_is_present(filter->filter, package_p, name_p);
    Iddd__Filter__Type filter_type = filter->filter_type;
    is_active = (((filter_type == IDDD__FILTER__TYPE__INCLUDE) && present) ||
                 ((filter_type == IDDD__FILTER__TYPE__EXCLUDE) && !present));
  }

  return is_active;
}

/**
 * Reset filter definitions by destroying the old filter list and
 * creating a new empty list excluding nothing.
 *
 */
static void filter_reset_default(idd_filter_t* filter)
{
  idd_list_t* new_list_p = create_filter_list();

  if (new_list_p != NULL)
  {
    if (filter->file)
    {
      unlink(filter->file);
    }
    list_destroy(&filter->filter);
    filter->filter = new_list_p;
    filter->filter_type = DEFAULT_FILTER_TYPE;
  }
}

/**
* Create and load the given probe filter from the given file.
  *
 * @return 1 if filter is loaded from file successfully, otherwise 0
 */
static int filter_init_load(idd_filter_t* filter)
{
  filter->filter = create_filter_list();

  if (filter->filter == NULL)
  {
    die("OOM, while creating config_list_p");
  }

  if (filter->file && list_load(filter->filter, filter->file, load_filter_list_cb, NULL))
  {
    // Include or exclude? See implementation notes above.
    filter->filter_type = filter_is_present(filter->filter, "__type", "exclude")
      ? IDDD__FILTER__TYPE__EXCLUDE
      : IDDD__FILTER__TYPE__INCLUDE;
    IDD_LOGD("%s filter loaded from %s",
             filter->filter_type == IDDD__FILTER__TYPE__EXCLUDE ? "Exclude" : "Include",
             filter->file);
    return 1;
  }
  else
  {
    // No existing filter list file, default to disable all probes, i.e. include nothing.
    filter->filter_type = DEFAULT_FILTER_TYPE;
    if (filter->file)
    {
      IDD_LOGD("No filter loaded from %s, use default (empty Include filter)", filter->file);
    }
    return 0;
  }
}

/**
 */
int filter_init(idd_filter_t* filter)
{
  int i, res = 0;
  for (i = 0; i < MAX_FILTERS; i++)
  {
    /* the last filter has to be the event filter, see filter_t in filter.h */
    res = filter_init_load(filter + i);
  }
  return res;
}

/**
 * Create a list of idd_filter_t elements derived from
 * FilterItem protobufs in 'filter_p'. FilterItem is defined
 * in configuration.proto.
 *
 * @param list_p list to be populated. Out parameter.
 * @param filter_p Filter protobuf defined in configuration.proto.
 * In parameter.
 *
 * @return 1 on success, 0 on failure
 */
static int load_configuration_list(idd_list_t* list_p, Iddd__Filter* filter_p)
{
  size_t i;
  for (i = 0; i < filter_p->n_item; i++)
  {
    Iddd__FilterItem* pb_item_p = filter_p->item[i];
    if ((pb_item_p == NULL) || (pb_item_p->package == NULL) || (pb_item_p->name == NULL))
    {
      IDD_LOGE("Invalid probe filter, new probe filter not activated!");
      return 0;
    }
    else
    {
      idd_filter_item_t* item_p = create_filter(pb_item_p->package, pb_item_p->name);
      if (item_p == NULL)
      {
        IDD_LOGE("Out Of Memory creating probe filter, new probe filter not activated!");
        return 0;
      }
      if (!list_add(list_p, (void**)&item_p))
      {
        IDD_LOGE("Failed adding probe <%s><%s> to filter, new probe filter not activated!", pb_item_p->package, pb_item_p->name);
        return 0;
      }
    }
  }

  if (filter_p->type == IDDD__FILTER__TYPE__EXCLUDE)
  {
    idd_filter_item_t* item_p = create_filter("__type", "exclude");
    if (item_p == NULL)
    {
      IDD_LOGE("Out Of Memory creating exclude filter, new probe filter not activated!");
      return 0;
    }
    if (!list_add(list_p, (void**)&item_p))
    {
      IDD_LOGE("Failed adding exclude filter, new probe filter not activated!");
      return 0;
    }
  }

  return 1;
}

/**
 * Get a pointer to the filter in the FilterConfig protobuf corresponding to specified type.
 *
 * @param type The filter type address to return
 * @param fc_p FilterConfig protobuf
 */
static Iddd__Filter* get_filter(probe_type_t type, Iddd__FilterConfig* fc_p)
{
  if (fc_p != NULL)
  {
    switch (type)
    {
      case EVENT : return fc_p->event_filter;
      case COUNTER : return fc_p->counter_filter;
      case STATICDATA : return fc_p->staticdata_filter;
      case MAX_FILTERS : return NULL;
    }
  }
  return NULL;
}

/**
 * Loads specified filter from given unpacked FilterConfig protobuf.
 * It will also be stored in appropriate file.
 *
 * @param filter The filter to load
 * @param probe_type The probe type to load (and store)
 * @param fc_p FilterConfig protobuf
 */
void load_filter(idd_filter_t* filter, probe_type_t probe_type, Iddd__FilterConfig* fc_p)
{
  int filter_found = 0;
  Iddd__Filter* filter_p = get_filter(probe_type, fc_p);
  if (filter_p != NULL)
  {
    idd_list_t* new_list_p = create_filter_list();
    if (new_list_p != NULL)
    {
      IDD_LOGD("Loading %zu filters from configuration", filter_p->n_item);
      if (load_configuration_list(new_list_p, filter_p))
      {
        int ok = filter->file ?
          list_save(new_list_p, filter->file, save_filter_list_cb, NULL) : 1;
        if (ok)
        {
          // Hot-swap the lists - so that the old one will be released
          // when the function exits.
          idd_list_t* temp_p = filter->filter;
          filter->filter = new_list_p;
          new_list_p = temp_p;
          filter->filter_type = filter_p->type;
          filter_found = 1;
          if (filter->file)
          {
            IDD_LOGD("Filters saved persistently to %s", filter->file);
          }
        }
      }
      list_destroy(&new_list_p);
    }
  }
  if (filter_found == 0)
  {
    if (filter->file)
    {
      IDD_LOGD("Loading filter failed, set filter to default state, %s", filter->file);
    }
    filter_reset_default(filter);
  }
}

/**
 * Unpack the given buffer assuming it is a FilterConfig protobuf
 * (defined in configuration.proto). Assign global variables
 * holding probe filter if successful.
 *
 * @param filter The filter to load
 * @param buf_p FilterConfig protobuf
 * @param buf_size size of 'buf_p'
 */
static void load_configuration_buffer(idd_filter_t* filter, const unsigned char* buf_p,
                                      size_t buf_size)
{
  int i;
  Iddd__FilterConfig* fc_p = iddd__filter_config__unpack(NULL, buf_size, buf_p);
  IDD_LOGD("Loading probe filters from buffer, %zu bytes", buf_size);

  for (i = 0; i < MAX_FILTERS; i++)
  {
    load_filter(filter + i, i, fc_p);
  }

  if (fc_p != NULL)
  {
    iddd__filter_config__free_unpacked(fc_p, NULL);
  }
}

/**
 * Parses a new filter configuration, if one is present. If failure
 * then reset to default filter by calling filter_reset_default().
 */
void filter_load_configuration(idd_filter_t* filter)
{
  int i;
  FILE* fp = config_open_file("iddd", "FilterConfig");
  if (fp != NULL)
  {
    size_t file_size;
    unsigned char* buf_p = read_entire_file(fp, &file_size);
    if (buf_p != NULL)
    {
      IDD_LOGD("Loading filters from fd = %d", fileno(fp));
      load_configuration_buffer(filter, buf_p, file_size);
      free(buf_p);
      IDD_LOGD("Loading filters, end");
      return;
    }
    fclose(fp);
  }

  IDD_LOGD("No filter configuration section in the new configuration or "
           "reading filter configuration failed, set filter to default state.");
  for (i = 0; i < MAX_FILTERS; i++)
  {
    filter_reset_default(filter + i);
  }
}

/**
 * Destroy the global lists containing idd probe filters.
 */
void filter_shutdown(idd_filter_t* filter)
{
  int i;
  for (i = 0; i < MAX_FILTERS; i++)
  {
    if ((filter + i)->filter != NULL)
    {
      list_destroy(&(filter + i)->filter);
    }
  }
}

/**
 */
void filter_create(Iddd__FilterConfig* probe_filter, idd_filter_t filter[])
{
  int i;
  for (i = 0; i < MAX_FILTERS; i++)
  {
    load_filter(&filter[i], i, probe_filter);
  }
}
