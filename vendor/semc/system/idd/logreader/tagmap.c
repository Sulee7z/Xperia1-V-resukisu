/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @author Carl-Emil Lagerstedt (carl-emil.lagerstedt@sonyericsson.com)
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <stdint.h>
#include <stdbool.h>
#include "idd_config.h"
#include "tagmap.h"
#include "util.h"

#define EVENT_TAG_MAP_FILE "/system/etc/event-log-tags"

extern bool setting_verbose;

static char* units[] = {"NOT SET", "NUMBER OF OBJECTS", "NUMBER OF BYTES",
                        "NUMBER OF MILLISECONDS", "NUMBER OF ALLOCS","ID",
                        "PERCENT"};

static char* data_types[] = {"NOT_SET", "INT", "LONG", "STRING", "LIST"};

static int g_tagmap_size = 0;

static struct EventType* g_eventtype_list = NULL;
static struct EventType** g_eventtype_sorted = NULL;

/**
 * Frees a field descriptor
 */
static void tagmap_free_field(struct Field* field_p)
{
  if (field_p != NULL) {
    FREE(field_p->name_p);
    FREE(field_p);
  }
}

/**
 * Frees an EventType struct
 */
static void free_eventtype(struct EventType* eventtype_p)
{
  for (size_t i=0; i < eventtype_p->num_fields; i++) {
    tagmap_free_field(eventtype_p->fields_pp[i]);
  }

  FREE(eventtype_p->fields_pp);
  FREE(eventtype_p->tag_p);
  FREE(eventtype_p);
}

/**
 * Returns true if type is a legal type
 */
static bool legal_type(int type)
{
  return type >= TYPE_NOT_SET && type <= TYPE_LIST;
}

/**
 * maps a type to a string
 */
static char* data_type_to_string(enum DataType type)
{
  return legal_type(type) ? data_types[type] : "UNKNOWN TYPE";
}

/**
 * returns true if unit is legal unit
 */
static bool legal_unit(int unit){
  return unit >= UNIT_NOT_SET  && unit <= UNIT_PERCENT;
}

/**
 * maps unit to a string
 */
char* dataunit_to_string(enum DataUnit unit)
{
  return legal_unit(unit) ? units[unit] : "UNKNOW UNIT";
}

/**
 * Allocates a zero filled field descriptor
 */
static struct Field* create_field()
{
  return  calloc(1, sizeof(struct Field));
}

/**
 * Tokenizes a string. Will modify the string in input_p.
 * Caller must free the returned string.
 */
static char* tokenize(char* input_p, char* delimiters_p)
{
  char* retval_p = NULL;
  char* token_p = strtok(input_p, delimiters_p);

  if (token_p != NULL) {
    while (*token_p == ' ') {
      token_p++;
    }
    retval_p = strndup(token_p, KiB);
  }

  return retval_p;
}

/**
 * Parses a field description in the tag map file
 */
static struct Field* parse_field(char* field_str_p)
{
  char* token1_p = tokenize(field_str_p, "()|");
  char* token2_p = tokenize(NULL, "()|");
  char* token3_p = tokenize(NULL, "()|");

  struct Field* field_p = NULL;

  if (token1_p && token2_p && token3_p) {
    int type, unit;

    type = atoi(token2_p);
    unit = atoi(token3_p);

    if (legal_type(type) && legal_unit(unit))
    {
      field_p = create_field();
      if (field_p) {
        field_p->name_p = strndup(token1_p, KiB);
        field_p->type = type;
        field_p->unit = unit;
      }
    }
  } else if (token1_p && token2_p) {
    int type = atoi(token2_p);

    if (legal_type(type)) {
      field_p = create_field();
      if (field_p != NULL) {
        field_p->name_p = strndup(token1_p, KiB);
        field_p->type = type;
      }
    }
  } else {
    if (setting_verbose) {
      printf ("Failed to parse fields, tokens are not proper");
    }
  }

  FREE(token1_p);
  FREE(token2_p);
  FREE(token3_p);


  return field_p;
}

/**
 * Parses a list of fields in the tag map file
 */
static bool parse_fields (char* fields_p, struct EventType* eventtype_p)
{
  char* field_p;
  size_t len = strlen(fields_p);
  size_t count = 0;
  char** field_strings_pp;
  size_t i;
  bool success = true;

  if (len > 0) {
    count++;
  }

  for (i=0; i < len; ++i) {
    if (fields_p[i] == ',') {
      count++;
    }
  }

  field_strings_pp = malloc(sizeof(char*) * count);

  eventtype_p->num_fields = count;
  eventtype_p->fields_pp = calloc(count, sizeof(struct Field*));

  if (field_strings_pp != NULL && eventtype_p->fields_pp != NULL) {
    if (count > 0) {
      field_strings_pp[0] = tokenize(fields_p, ",\n");
    }

    i = 1;
    while ((field_p = tokenize(NULL, ",\n")) != NULL) {
      field_strings_pp[i++] = field_p;
    }

    for (i = 0; i < count; i++){
      eventtype_p->fields_pp[i] = parse_field(field_strings_pp[i]);

      if (eventtype_p->fields_pp[i] == NULL) {
        if (setting_verbose){ printf("Failed to parse field %s\n", field_strings_pp[i]);}
        success = false;
        break;
      }
    }

    for (i = 0; i < count; i++) {
      FREE(field_strings_pp[i]);
    }

    FREE(field_strings_pp);

    if (!success){
      for (i = 0; i < count; i++){
        FREE(eventtype_p->fields_pp[i]);
        eventtype_p->num_fields = 0;
      }
      FREE(eventtype_p->fields_pp);
    }

  }
  else {
    FREE(field_strings_pp);
    FREE(eventtype_p->fields_pp);
  }
  return success;
}

/**
 * Parses the event tag map file
 */
static struct EventType* parse(FILE* f) {
  char* buffer = malloc(1024);
  struct EventType* pHead = NULL;
  struct EventType* event_p = NULL;

  g_tagmap_size = 0;
  while (buffer != NULL && fgets(buffer, 1024, f) != NULL) {

    struct EventType* next_event_p = calloc(1, sizeof(*event_p));

    if (next_event_p != NULL) {
      char* tagcode_p;
      char* tag_p;
      char* fields_p;
      int tag_code = -1;

      tagcode_p = tokenize(buffer, " \n");
      tag_p = tokenize(NULL, " \n");
      fields_p = tokenize(NULL, "\n");

      if (tagcode_p != NULL && tag_p != NULL) {
        tag_code = atoi(tagcode_p);
      }

      if (tag_code > 0 && tag_p != NULL) {
        next_event_p->code = tag_code;
        next_event_p->tag_p = strndup(tag_p, KiB);

        if ((fields_p != NULL && parse_fields(fields_p, next_event_p)) ||
            fields_p == NULL) {
          if(event_p != NULL) {
            event_p->next_event_p = next_event_p;
          }

          event_p = next_event_p;

          if (pHead == NULL) {
            pHead = event_p;
          }
          g_tagmap_size++;
        }
        else  {
          if (setting_verbose) {
            printf("Failed to parse %s\n", buffer);
            printf("code=%s tag=%s fields=%s\n", tagcode_p, tag_p, fields_p);
          }
          free_eventtype(next_event_p);
        }
      }

      FREE(tagcode_p);
      FREE(fields_p);
      FREE(tag_p);
    }
  }

  FREE(buffer);

  return pHead;
}

/**
 * Prints the structure of a parsed event type
 */
static void print_events(struct EventType* event_p)
{
  while(event_p != NULL){
    printf ("EVENT TYPE %d %s ", event_p->code, event_p->tag_p );
    for (size_t i=0; i < event_p->num_fields; ++i){
      printf("%s|", event_p->fields_pp[i]->name_p);
      printf("%s(%d)|",
             data_type_to_string(event_p->fields_pp[i]->type),
             event_p->fields_pp[i]->type);
      printf("%s (%d)",
             dataunit_to_string(event_p->fields_pp[i]->unit),
             event_p->fields_pp[i]->unit);
    }

    printf("\n");
    event_p = event_p->next_event_p;
  }
}

/**
 * Find the event tag description. This is a binary search and will be called each time a rule
 * have wildcard as tag.
 * Maybe there are quicker algorithms...
 */
struct EventType* search_eventtypes(const uint32_t value)
{
  int low = 0;
  int high = g_tagmap_size - 1;

  while (low <= high) {
    uint32_t mid = (low + high) >> 1;
    uint32_t mid_val = g_eventtype_sorted[mid]->code;

    if (mid_val < value) {
      low = mid + 1;
    }
    else if (mid_val > value) {
       high = mid - 1;
    }
    else {
      return g_eventtype_sorted[mid];
    }
  }
  return NULL;
}

/**
 * Sort an array of EventType according to tag code.
 */
 static void sort_eventtypes(struct EventType* event_types_p[], size_t len)
{
  bool done;

  do {
    done = true;
    for (size_t i = 0 ; i < len -1; ++i) {
      if (event_types_p[i]->code > event_types_p[i+1]->code ) {
        struct EventType* tmp = event_types_p[i];
        event_types_p[i] = event_types_p[i+1];
        event_types_p[i+1] = tmp;
        done = false;
      }
    }
  } while (!done);
}

/**
 * Reads the event map file into internal representations to speed up log parsing/matching.
 */
void read_tag_map(char* path_p)
{
  FILE* f;

  if (path_p == NULL) {
    path_p = EVENT_TAG_MAP_FILE;
  }

  f = fopen(path_p,  "r");

  if (f != NULL ) {
    free(g_eventtype_list);
    g_eventtype_list = parse(f);

    if (setting_verbose) {
      print_events(g_eventtype_list);
    }

    // Create an array, sorted by the tag number.
    free(g_eventtype_sorted);
    g_eventtype_sorted = malloc(sizeof(struct EventType*) * g_tagmap_size);
    struct EventType* event_p = g_eventtype_list;
    for (int i = 0; i < g_tagmap_size && event_p; event_p = event_p->next_event_p, i++) {
      g_eventtype_sorted[i] = event_p;
    }
    sort_eventtypes(g_eventtype_sorted, g_tagmap_size);

    fclose(f);
  }
}

/**
 * Return the EventType that matches given tag, NULL if no match found
 */
struct EventType* find_event_type_from_tag(char* tag)
{
  struct EventType* event_list_p = g_eventtype_list;
  for (; (event_list_p != NULL) && strncmp(event_list_p->tag_p, tag, KiB);
       event_list_p = event_list_p->next_event_p);

  return event_list_p;
}
