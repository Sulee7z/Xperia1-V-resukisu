/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief Rule based filtering for logreader
 *
 * @author Carl-Emil Lagerstedt (carl-emil.lagerstedt@sonyericsson.com)
 */
#ifndef RULES_H_INCLUSION_GUARD
#define RULES_H_INCLUSION_GUARD

#include  "../api/idd_api.h"

#include <stdbool.h>
#include <log/log_read.h>
#include <log/logprint.h>
#include "tagmap.h"
#include "idd/idd_system_logging.pb-c.h"

/**
 * The different log that are supported
 * Must match logs[]
 */
typedef enum {
  Log_Illegal = 0,
  Log_Radio = 1,
  Log_Main = 2,
  Log_Events = 3,
  Log_System = 4,
  Log_Kernel = 5,
  Log_Any
} Log;

struct Rule;

/*
 * This list holds all logreader probes.
 * These are IDD plugins with their own Android log filter.
 */
struct Probe {
  int plugin_id;                       //The IDD plugin id to use when create the LogMessage event.
  struct Rule* rule;                   //The Android log filter for the IDD plugin.
  struct Probe* next;
};

/**
 * Free ruleset for all probes
 */
void free_probes(struct Probe* probe_p);

/**
 * Parses a ruleset into a Rule struct
 */
struct Rule* parse_rules_from_str(char* rules);

/**
 * Create a log-reader probe with given plugin id and rules
 */
struct Probe* create_probe(struct Rule* rule, int plugin_id);

/**
 * Add new_probe to probe list
 */
void probe_add(struct Probe** probe, struct Probe* new_probe);

/**
 *
 * @return a ruleset if successful, NULL if parsing failed
 */
struct Probe* parse_rules(char* path_p);
struct Probe* get_probes_from_config(IDD_t* idd_p);

/**
 *
 * @return true if entry_p should be filtered out, false otherwise
 */
bool rule_exclude(struct Rule* rules_p, Log log, struct AndroidLogEntry_t* entry_p);

/**
 *
 * @return If the log is enabled and matched, a pointer to the log description, NULL otherwise.
 */
struct EventType* rule_include_binary(struct Rule* rules_p, Log log, uint32_t tag_code,
                                      char* eventdata, int eventlen);

/**
 * If there are no rules enabling that includes data on a log there is
 * no reason to read from it.
 *
 * This is not fool proof. if you write a rules file like this
 *  + * * *
 *  - main * *
 *
 * it will still consider main to be enabled.
 *
 * @return true if there are any rules that will (likely) include data
 */
bool log_enabled(struct Probe* probe, Log log);

/**
 * For binary logs we speed up the log parsing with a pointer to the tag descriptor.
 * For wildcard tags, the matching has to be done in realtime.
 * Realtime matching also has to be done if there are string matching in the rule.
 */
void get_included_binary_tags(struct Probe* probe, Log log);

/**
 * Print the current rules for all probes
 */
void print_probes(struct Probe* probe);

#endif
