/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief Static Data Module, for handling Static Data
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include "sdata.h"
#include "list.h"
#include "util.h"
#include "config.h"
#include "serializer.h"
#include "settings.h"
#include "idd_config.h"
#include "filter.h"

#define LOG_TAG "iddd-sdata"
#include "logging.h"

static int sdata_modified_since_save = 0;
static idd_list_t* sdlist_p = NULL;

typedef struct
{
  const char* package_p;
  const char* name_p;
} idd_sdata_key_t;

typedef struct
{
  idd_sdata_key_t key;
  /* The timestamp when the value was last modified */
  int64_t modified_timestamp;
  /* The timestamp when the static data was last touched, which
     is updated when the same value is set as before. */
  int64_t touched_timestamp;
  char* value_p;
} idd_sdata_t;

// Structure passed as user data to list_save.
// Static data that are saved will have to have a timestamp that is
// larger (i.e. younger) than this limit to be saved.
typedef struct {
  int64_t age_limit;
} sdata_save_t;

/**
 * Finds the specified static data in the list of all static data in
 * 'sdlist_p'.
 *
 * @param package_p name of the originating package
 * @param name_p name of the static data
 *
 * @return pointer to the found idd_sdata_t element or NULL if not found
 */
static idd_sdata_t* find_sdata(const char* package_p, const char* name_p)
{
  idd_sdata_key_t key;
  key.package_p = package_p;
  key.name_p = name_p;

  return (idd_sdata_t*)list_find(sdlist_p, &key);
}

/**
 * Create a static data in 'sdlist_p':
 *
 * @param package_p name of the originating package
 * @param name_p name of the static data
 * @param value_p
 * @param mod_timestamp
 * @param touch_timestamp
 *
 * @return pointer to a new idd_sdata_t element or NULL if not created
 */
static idd_sdata_t* create_sdata(const char* package_p, const char* name_p, const char* value_p,
                                 int64_t mod_timestamp, int64_t touch_timestamp)
{
  size_t package_size = strlen(package_p);
  size_t name_size = strlen(name_p);
  idd_sdata_t* sdata_p = malloc(sizeof(*sdata_p) + package_size + 1 + name_size + 1);

  if (sdata_p != NULL)
  {
    char* sdata_package_p = (char*)sdata_p + sizeof(*sdata_p);
    char* sdata_name_p = sdata_package_p + package_size + 1;
    sdata_p->key.package_p = sdata_package_p;
    sdata_p->key.name_p = sdata_name_p;
    memcpy(sdata_package_p, package_p, package_size);
    memcpy(sdata_name_p, name_p, name_size);
    sdata_package_p[package_size] = '\0';
    sdata_name_p[name_size] = '\0';
    sdata_p->modified_timestamp = mod_timestamp;
    sdata_p->touched_timestamp = touch_timestamp;
    sdata_p->value_p = strdup(value_p);

    if (sdata_p->value_p == NULL)
    {
      free(sdata_p);
      sdata_p = NULL;
    }
  }
  return sdata_p;
}

/**
 * Compare two static data elements. Two static datas are considered
 * equal if both the package and name are equal. The value or
 * timestamps are not compared.
 *
 * @param lhs_p static data 1 (idd_sdata_t*)
 * @param rhs_p static data 2 (idd_sdata_t*)
 *
 * @return 0 if equal, normal strcmp semantics
 */
static int compare_sdata(const void* lhs_p, const void* rhs_p)
{
  const idd_sdata_key_t* key1_p = (const idd_sdata_key_t*)lhs_p;
  const idd_sdata_key_t* key2_p = (const idd_sdata_key_t*)rhs_p;

  int diff = strcmp(key1_p->package_p, key2_p->package_p);
  if (diff == 0)
  {
    diff = strcmp(key1_p->name_p, key2_p->name_p);
  }

  return diff;
}

/**
 * Handle an incoming RPC to set a static data value. Called from
 * handle_data() in main.c
 *
 * @param filter The filter to use
 * @param package_p name of the originating package
 * @param name_p name of the static data
 * @param value_p
 */
void sdata_handle(idd_filter_t* filter, const char* package_p, const char* name_p,
                  const char* value_p)
{
  if (!filter_is_probe_active(filter + STATICDATA, package_p, name_p, CHECK_IDD_ENABLED))
  {
    IDD_LOGD("StaticData %s::%s was discarded since it was not active.", package_p, name_p);
    return;
  }

  idd_sdata_t* sdata_p = find_sdata(package_p, name_p);
  int64_t timestamp = get_timestamp();

  if (sdata_p != NULL)
  {
    // Always update the 'touched' time.
    sdata_p->touched_timestamp = timestamp;

    if (strcmp(sdata_p->value_p, value_p) != 0)
    {
      char* new_value_p = strdup(value_p);
      if (new_value_p != NULL)
      {
        free(sdata_p->value_p);
        sdata_p->value_p = new_value_p;
        sdata_p->modified_timestamp = timestamp;
        sdata_modified_since_save = 1;
        IDD_LOGD("Mod ::%s::%s::%s:: [%" PRId64 "]",
                 sdata_p->key.package_p, sdata_p->key.name_p, sdata_p->value_p,
                 sdata_p->modified_timestamp);
      }
    }
  }
  else
  {
    sdata_p = create_sdata(package_p, name_p, value_p, timestamp, timestamp);
    if (sdata_p != NULL)
    {
      if (list_add(sdlist_p, (void**)&sdata_p))
      {
        sdata_modified_since_save = 1;
        IDD_LOGD("New ::%s::%s::%s:: [%" PRId64 "]",
                 sdata_p->key.package_p, sdata_p->key.name_p, sdata_p->value_p,
                 sdata_p->modified_timestamp);
      }
      else
      {
        IDD_LOGE("Failed to add new static data.");
      }
    }
  }
}

/**
 * Implementation of the callback idd_list_save_t from list.h.
 * Serializes the list element to the file descriptor given.  Every
 * list element should be one line.
 *
 * @param fp file to write to
 * @param element_p pointer to the idd_sdata_t element to serialize
 * @param userdata_p pointer to sdata_save_t
 *
 * @return 1 if success, 0 if failure
 */
static int serialize_sdata(FILE* fp, const void* element_p, void* userdata_p)
{
  const sdata_save_t* save_data_p = (const sdata_save_t*)userdata_p;
  const idd_sdata_t* sdata_p = (const idd_sdata_t*)element_p;
  char line[IDD_MAX_PROBE_ID_SIZE + 2 + MAX_NUMBER + 1 + MAX_NUMBER + 1 + IDD_MAX_SDATA_SIZE +
            IDD_RECORD_END_LEN] = {0};
  const char* format_p = "%s%c%s%c%" PRId64 "%c%" PRId64 "%c%." IDD_MAX_SDATA_SIZE_STR "s%c%c\n";

  if (sdata_p->touched_timestamp > save_data_p->age_limit)
  {
    int count = snprintf(line, sizeof(line), format_p,
                         sdata_p->key.package_p, '\0',
                         sdata_p->key.name_p, '\0',
                         sdata_p->modified_timestamp, '\0',
                         sdata_p->touched_timestamp, '\0',
                         sdata_p->value_p, '\0', '\0');

    IDD_LOGD("Serializing ::%s::%s::%s:: [%" PRId64 "] (%d bytes)",
             sdata_p->key.package_p, sdata_p->key.name_p, sdata_p->value_p,
             sdata_p->modified_timestamp, count);

    if (fwrite(line, 1, count, fp) != (size_t)count)
    {
      IDD_LOGE("Failed to write to sdata file: %s", strerror(errno));
      return 0;
    }
  }

  return 1;
}

/**
 * Writes down static data (in memory in 'sdlist_p') to persistent storage.
 *
 */
void sdata_save(void)
{
  if (sdata_modified_since_save)
  {
    sdata_save_t save_data;
    /* Set a filter to expire old sdata */
    save_data.age_limit = settings_expire_sdata_get() == 0 ?
      0 : get_timestamp() - (int64_t)settings_expire_sdata_get() * 1000;

    if (list_save(sdlist_p, SDATA_TXT_FNAME, serialize_sdata, &save_data))
    {
      sdata_modified_since_save = 0;
    }
  }
}

/**
 * Callback function called when an element is removed from the list 'sdlist_p'.
 *
 * @param element_p pointer to idd_sdata_t element to be freed
 */
static void destruct_sdata(void* element_p)
{
  idd_sdata_t* sdata_p = (idd_sdata_t*)element_p;
  free(sdata_p->value_p);
}

/**
 * Implementation of the callback idd_list_load_t from list.h
 * Called when we restore the list from file. Every line in that file represents
 * a static data element, and we must interpret it here.
 * strlen(line) is crash safe since end_mark contains a NULL (in fact two "\0\0\n").
 *
 * @param line line read from the file
 * @param end_mark pointer to the end-marker in 'line'
 * @param userdata_p unused
 *
 * @return pointer to the new idd_sdata_t element, NULL if no
 * static data value found in 'line'
 */
static void* deserialize_sdata(char* line, char* end_mark, void* userdata_p)
{
  const char* package_p;
  const char* name_p = NULL;
  const char* mod_timestamp_p;
  const char* touch_timestamp_p;
  const char* value_p = NULL;
  idd_sdata_t* sdata_p = NULL;

  (void)userdata_p;

  package_p = line;
  line += strlen(line) + 1;
  if (line < end_mark)
  {
    name_p = line;
    line += strlen(line) + 1;
  }
  if (line < end_mark)
  {
    mod_timestamp_p = line;
    line += strlen(line) + 1;
  }
  if (line < end_mark)
  {
    touch_timestamp_p = line;
    line += strlen(line) + 1;
  }
  if (line < end_mark)
  {
    value_p = line;
    line += strlen(line);
    if (line != end_mark)
    {
      //The last token (value) is too long
      value_p = NULL;
    }
  }

  if (value_p)
  {
    int64_t mod_timestamp = strtoint64(mod_timestamp_p);
    int64_t touch_timestamp = strtoint64(touch_timestamp_p);

    sdata_p = create_sdata(package_p, name_p, value_p, mod_timestamp, touch_timestamp);
    IDD_LOGD("Loading ::%s::%s::%s:: [%" PRId64 "]",
             package_p, name_p, value_p, mod_timestamp);
  }
  else
  {
    IDD_LOGE("Failed to parse sdata line <%s::%s>", package_p ? package_p : "NULL", name_p ? name_p : "NULL");
  }

  return sdata_p;
}

/**
 * Called from main() to create and load 'sdlist_p' from file
 * "sdata.txt". 'sdlist_p' holds all current static datas in memory.
 *
 */
void sdata_init(void)
{
  sdlist_p = list_create(SDATA_LIST_INITIAL_SIZE,
                         SDATA_LIST_GROW_BY,
                         compare_sdata,
                         destruct_sdata);
  if (sdlist_p == NULL)
  {
    die("OOM, while creating sdlist_p");
  }

  /* Load old sdata from disk, if there are any. */
  list_load(sdlist_p, SDATA_TXT_FNAME, deserialize_sdata, NULL);
}

/**
 * Writes the static data as a protocol buffer in the Report message
 * type. Callback function used by list_iterate() (list.h) when
 * flushing static datas (see sdata_flush() below).
 *
 * @param element_p pointer to the idd_sdata_t element to serialize
 * @param ud file descriptor to write the static data protobuf to
 */
static void write_pb(const void* element_p, void* ud)
{
  char buffer[512] = {0};
  const idd_sdata_t* sdata_p = (const idd_sdata_t*)element_p;

  size_t size = ser_sdata(buffer, sizeof(buffer), sdata_p->key.package_p,
                          sdata_p->key.name_p, sdata_p->modified_timestamp,
                          sdata_p->value_p);

  IDD_LOGD("Flushing ::%s::%s::%s:: [%" PRId64 "]",
           sdata_p->key.package_p, sdata_p->key.name_p, sdata_p->value_p,
           sdata_p->modified_timestamp);

  if (fwrite(buffer, 1, size, (FILE*)ud) != size)
  {
    IDD_LOGE("Failed to write PB to file: %s", strerror(errno));
  }
}

/**
 * Writes down all static data in 'sdlist_p' as protocol buffers to
 * the file descriptor given.
 *
 * @param fp file descriptor to write the static data protobufs to
 */
void sdata_flush(FILE* fp)
{
  list_iterate(sdlist_p, write_pb, (void*)fp);
}

/**
 * Destroy the global list containing all static datas ('sdlist_p').
 *
 */
void sdata_shutdown(void)
{
  if (sdlist_p != NULL)
  {
    list_destroy(&sdlist_p);
  }
}

/**
 * Check if there is static data update since last report is generated.
 *
 */
int is_sdata_update(void)
{
  int has_update = 0;
  if (sdlist_p == NULL)
  {
     return has_update;
  }

  size_t i;
  for (i = 0; i < sdlist_p->length; i++)
  {
    const idd_sdata_t* sdata_p = (const idd_sdata_t*) sdlist_p->list_pp[i];
    if (sdata_p->modified_timestamp >= iddd_flushtime_get())
    {
      has_update = 1;
      break;
    }
  }
  return has_update;
}
