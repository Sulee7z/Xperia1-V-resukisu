/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief Parser for /system/etc/event-log-tags
 *
 * @author Carl-Emil Lagerstedt (carl-emil.lagerstedt@sonyericsson.com)
 */
#ifndef TAGMAP_H_INCLUSON_GUARD
#define TAGMAP_H_INCLUSON_GUARD

enum DataType {
  TYPE_NOT_SET = 0,
  TYPE_INT = 1,
  TYPE_LONG = 2,
  TYPE_STRING = 3,
  TYPE_LIST = 4
};

enum DataUnit {
  UNIT_NOT_SET = 0,
  UNIT_NUMBER_OF_OBJECTS = 1,
  UNIT_NUMBER_OF_BYTES = 2,
  UNIT_NUMBER_OF_MILLISECONDS = 3,
  UNIT_NUMBER_OF_ALLOCS = 4,
  UNIT_ID = 5,
  UNIT_PERCENT = 6
};

/**
 * Describes a field
 */
struct Field
{
  char* name_p;
  enum DataType type;
  enum DataUnit unit;
};

/**
 * Describes a row in the event tag map
 */
struct EventType {
  uint32_t code;
  char* tag_p;
  struct Field** fields_pp;
  size_t num_fields;
  struct EventType* next_event_p;
};

/**
 * Reads the event map file into internal representations to speed up log parsing/matching.
 */
void read_tag_map(char* pFile);

/**
 * Find the event tag description. This is a binary search and will be called each time a rule
 * have wildcard as tag.
 * Maybe there are quicker algorithms...
 */
struct EventType* search_eventtypes(const uint32_t value);

/**
 * Return the EventType that matches given tag, NULL if no match found
 */
struct EventType* find_event_type_from_tag(char* tag);

/**
 * Returns the name of a DataUnit
 */
char* dataunit_to_string(enum DataUnit unit);

#endif //TAGMAP_H_INCLUSON_GUARD
