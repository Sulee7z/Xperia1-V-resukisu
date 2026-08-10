/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief Counters module, for handling persistent counters.
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include "counters.h"
#include "util.h"
#include "list.h"
#include "config.h"
#include "idd_config.h"
#include "serializer.h"
#include "settings.h"
#include "filter.h"

#define LOG_TAG "iddd-counters"
#include "logging.h"

static int counters_modified_since_save = 0;
static idd_list_t* clist_p = NULL;

typedef struct {
  const char* package_p;
  const char* name_p;
} idd_counter_key_t;

typedef struct {
  idd_counter_key_t key;
  int64_t from_timestamp;
  int64_t to_timestamp;
  int64_t value;
} idd_counter_t;

// Structure passed as user data to list_save.
// Counters that are saved will have to have a timestamp that is
// larger (i.e. younger) than this limit to be saved.
typedef struct {
  int64_t age_limit;
} counters_save_t;

/**
 * Find a counter in 'clist_p'.
 *
 * @param package_p name of the originating package
 * @param name_p name of the counter
 *
 * @return pointer to the found idd_counter_t element or NULL if not found
 */
static idd_counter_t* find_counter(const char* package_p, const char* name_p)
{
  idd_counter_key_t key;
  key.package_p = package_p;
  key.name_p = name_p;

  return (idd_counter_t*)list_find(clist_p, &key);
}

/**
 * Create a counter in 'clist_p'.
 *
 * @param package_p name of the originating package
 * @param name_p name of the counter
 * @param value
 * @param from_timestamp
 * @param to_timestamp
 *
 * @return pointer to a new idd_counter_t element or NULL if not created
 */
static idd_counter_t* create_counter(const char* package_p, const char* name_p,
                                     int64_t value, int64_t from_timestamp,
                                     int64_t to_timestamp)
{
  size_t package_size = strlen(package_p);
  size_t name_size = strlen(name_p);
  idd_counter_t* counter_p = malloc(sizeof(*counter_p) + package_size + 1 + name_size + 1);

  if (counter_p != NULL)
  {
    char* counter_package_p = (char*)counter_p + sizeof(*counter_p);
    char* counter_name_p = counter_package_p + package_size + 1;
    counter_p->key.package_p = counter_package_p;
    counter_p->key.name_p = counter_name_p;
    memcpy(counter_package_p, package_p, package_size);
    memcpy(counter_name_p, name_p, name_size);
    counter_package_p[package_size] = '\0';
    counter_name_p[name_size] = '\0';
    counter_p->from_timestamp = from_timestamp;
    counter_p->to_timestamp = to_timestamp;
    counter_p->value = value;
  }
  return counter_p;
}

/**
 * Compare two counters. Two counters are considered equal if both the
 * package and name are equal. The value or timestamps are not
 * compared.
 *
 * @param lhs_p counter1
 * @param rhs_p counter2
 *
 * @return 0 if equal, normal strcmp semantics
 */
static int compare_counters(const void* lhs_p, const void* rhs_p)
{
  const idd_counter_key_t* key1_p = (const idd_counter_key_t*)lhs_p;
  const idd_counter_key_t* key2_p = (const idd_counter_key_t*)rhs_p;

  int diff = strcmp(key1_p->package_p, key2_p->package_p);
  if (diff == 0)
  {
    diff = strcmp(key1_p->name_p, key2_p->name_p);
  }

  return diff;
}

/**
 * Handle incoming command to set or increment a counter. Called from
 * handle_data() in main.c
 *
 * @param filter The probe filter to use
 * @param package_p name of the originating package
 * @param name_p name of the counter
 * @param value
 * @param is_delta 0 means 'set', 1 means 'add' to previous value (if any)
 */
void counters_handle(idd_filter_t* filter, const char* package_p, const char* name_p, int value,
                     int is_delta)
{
  if (!filter_is_probe_active(filter + COUNTER, package_p, name_p, CHECK_IDD_ENABLED))
  {
    IDD_LOGD("Counter %s::%s was discarded since it was not active.", package_p, name_p);
    return;
  }

  idd_counter_t* counter_p = find_counter(package_p, name_p);
  int64_t timestamp = get_timestamp();
  if (counter_p != NULL)
  {
    counter_p->value = is_delta ? counter_p->value + value : value;
    counter_p->to_timestamp = timestamp;
    counters_modified_since_save = 1;
    IDD_LOGD("Mod ::%s::%s::%" PRId64 ":: [%" PRId64 " -- %" PRId64 "]",
             counter_p->key.package_p, counter_p->key.name_p, counter_p->value,
             counter_p->from_timestamp, counter_p->to_timestamp);

  }
  else
  {
    counter_p = create_counter(package_p, name_p, value, timestamp, timestamp);
    if (counter_p != NULL)
    {
      if (list_add(clist_p, (void**)&counter_p))
      {
        counters_modified_since_save = 1;
        IDD_LOGD("New ::%s::%s::%" PRId64 ":: [%" PRId64 " -- %" PRId64 "]",
                 counter_p->key.package_p, counter_p->key.name_p, counter_p->value,
                 counter_p->from_timestamp, counter_p->to_timestamp);
      }
      else
      {
        IDD_LOGE("Failed to add new counter.");
      }
    }
  }
}

/**
 * Implementation of the callback idd_list_load_t from list.h
 *
 * Called when we restore the list from file. Every line in that file
 * represents a counter element, and we must interpret it here.
 * strlen(line) is crash safe since end_mark contains a NULL (in fact
 * two "\0\0\n").
 *
 * @param line line read from the file
 * @param end_line pointer to the end-marker in 'line'
 * @param userdata_p always NULL
 *
 * @return pointer to the new idd_counter_t element, NULL if no
 * counter value found in 'line'
 */
static void* deserialize_counter(char* line, char* end_line, void* userdata_p)
{
  const char* package_p;
  const char* name_p;
  const char* from_timestamp_p;
  const char* to_timestamp_p;
  const char* value_p = NULL;
  idd_counter_t* counter_p = NULL;

  (void)userdata_p;
  package_p = line;
  line += strlen(line) + 1;
  if (line < end_line)
  {
    name_p = line;
    line += strlen(line) + 1;
  }
  if (line < end_line)
  {
    from_timestamp_p = line;
    line += strlen(line) + 1;
  }
  if (line < end_line)
  {
    to_timestamp_p = line;
    line += strlen(line) + 1;
  }
  if (line < end_line)
  {
    value_p = line;
    line += strlen(line);
    if (line != end_line)
    {
      //The last token (value) is to long
      value_p = NULL;
    }
  }

  if (value_p)
  {
    int64_t from_timestamp = strtoint64(from_timestamp_p);
    int64_t to_timestamp   = strtoint64(to_timestamp_p);
    int64_t local_value    = strtoint64(value_p);

    counter_p = create_counter(package_p, name_p, local_value, from_timestamp, to_timestamp);
    IDD_LOGD("Loading ::%s::%s::%" PRId64 ":: [%" PRId64 " -- %" PRId64 "]",
             package_p, name_p, local_value, from_timestamp, to_timestamp);
  }
  else
  {
    IDD_LOGE("Failed to parse counter line.");
  }
  return counter_p;
}

/**
 * Implementation of the callback idd_list_save_t from list.h
 *
 * @param fp file to write to
 * @param element_p pointer to the idd_counter_t element to serialize
 * @param userdata_p always NULL
 *
 * @return 1 if success, 0 if failure
 */
static int serialize_counter(FILE* fp, const void* element_p, void* userdata_p)
{
  const counters_save_t* save_data_p = (const counters_save_t*)userdata_p;
  const idd_counter_t* counter_p = (const idd_counter_t*)element_p;
  char line[IDD_MAX_PROBE_ID_SIZE + 2 + (3 * (MAX_NUMBER + 1)) + IDD_RECORD_END_LEN] = {0};
  const char* format_p = "%s%c%s%c%" PRId64 "%c%" PRId64 "%c%" PRId64 "%c%c\n";

  if (counter_p->to_timestamp > save_data_p->age_limit)
  {
    int count = snprintf(line, sizeof(line), format_p,
                         counter_p->key.package_p, '\0',
                         counter_p->key.name_p, '\0',
                         counter_p->from_timestamp, '\0',
                         counter_p->to_timestamp, '\0',
                         counter_p->value, '\0', '\0');

    IDD_LOGD("Serializing ::%s::%s::%" PRId64 ":: [%" PRId64 " -- %" PRId64 "] (%d bytes)",
             counter_p->key.package_p, counter_p->key.name_p, counter_p->value,
             counter_p->from_timestamp, counter_p->to_timestamp, count);

    if (fwrite(line, 1, count, fp) != (size_t)count)
    {
      IDD_LOGE("Failed to write to counters file: %s", strerror(errno));
      return 0;
    }
  }

  return 1;
}

/**
 * Save all counters in 'clist_p' to file if modified since last save.
 *
 */
void counters_save(void)
{
  if (counters_modified_since_save)
  {
    counters_save_t save_data;
    /* Set a filter to expire old counters */
    save_data.age_limit = settings_expire_counters_get() == 0 ?
      0 : get_timestamp() - (int64_t)settings_expire_counters_get() * 1000;

    if (list_save(clist_p, COUNTERS_TXT_FNAME, serialize_counter, &save_data))
    {
      counters_modified_since_save = 0;
    }
  }
}

/**
 * Called from main() to create and load 'clist_p' from file
 * "counters.txt". 'clist_p' holds all current counters in memory.
 *
 */
void counters_init(void)
{
  clist_p = list_create(COUNTER_LIST_INITIAL_SIZE,
                        COUNTER_LIST_GROW_BY,
                        compare_counters,
                        NULL);
  if (clist_p == NULL)
  {
    die("OOM, while creating clist_p");
  }

  /* Load old counters from disk, if there are any. */
  list_load(clist_p, COUNTERS_TXT_FNAME, deserialize_counter, NULL);
}

/**
 * Serialize one counter as a protobuf. Callback function used by
 * list_iterate() (list.h) when flushing counters (see
 * counters_flush() below).
 *
 * @param element_p pointer to the idd_counter_t element to serialize
 * @param ud file descriptor to write the counter protobuf to
 */
static void write_pb(const void* element_p, void* ud)
{
  char buffer[IDD_MAX_PROBE_ID_SIZE + MAX_NUMBER * 3] = {0};
  const idd_counter_t* counter_p = (const idd_counter_t*)element_p;

  if (counter_p->to_timestamp >= iddd_flushtime_get())
  {
    size_t size = ser_counter(buffer, sizeof(buffer), counter_p->key.package_p,
                              counter_p->key.name_p, counter_p->from_timestamp,
                              counter_p->to_timestamp, counter_p->value);

    IDD_LOGD("Flushing ::%s::%s::%" PRId64 ":: [%" PRId64 " -- %" PRId64 "]",
             counter_p->key.package_p, counter_p->key.name_p, counter_p->value,
             counter_p->from_timestamp, counter_p->to_timestamp);

    if (fwrite(buffer, 1, size, (FILE*)ud) != size)
    {
      IDD_LOGE("Failed to write PB to file: %s", strerror(errno));
    }
  }
}

/**
 * Flush all counters in 'clist_p' by iterating over it and write each
 * counter to file as a protobuf.
 *
 * @param fp file descriptor to write the counter protobufs to
 */
void counters_flush(FILE* fp)
{
  list_iterate(clist_p, write_pb, (void*)fp);
}

/**
 * Destroy the global list containing all counters ('clist_p').
 *
 */
void counters_shutdown(void)
{
  if (clist_p != NULL)
  {
    list_destroy(&clist_p);
  }
}

/**
 * Check if there is counter data update since last report is generated.
 *
 */
int is_counter_update(void)
{
  int has_update = 0;
  if (clist_p == NULL)
  {
     return has_update;
  }

  size_t i;
  for (i = 0; i < clist_p->length; i++)
  {
    const idd_counter_t* counter_p = (const idd_counter_t*) clist_p->list_pp[i];
    if (counter_p->to_timestamp >= iddd_flushtime_get())
    {
      has_update = 1;
      break;
    }
  }
  return has_update;
}
