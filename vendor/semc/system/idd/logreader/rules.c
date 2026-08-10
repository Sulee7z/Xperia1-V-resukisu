/*
 * Copyright 2010 Sony Corporation
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include "rules.h"
#include "util.h"
#include  "../api/idd_api.h"
#include "idd/iddd.pb-c.h"
#include "common/tokenize.h"

// Reuse logging macros from iddd. LOG_TAG must come before the include.
#undef LOG_TAG
#define LOG_TAG "idd-logreader"
#include "../iddd/logging.h"

#define LINE_LENGTH 1024

static const char* delimiters = " \t\n\"";

/**
 * Error codes for parsing
 * Must match error_strings
 */
enum RuleStatus {
  Valid = 0,
  WarningKeyWord,
  InvalidOperator,
  InvalidLog,
  InvalidLevel,
  IncorrectNumberOfTokens
};

/**
 * Error codes for parsing
 * Must match RuleStatus
 */
static const char* ERROR_STRINGS[] =
  {
    "Valid",
    "Keyword Warning",
    "Invalid Operator",
    "Invalid log specified",
    "Invalid log prio specified",
    "Incorrect number of tokens in line"
  };


/**
 * String names of the differents logs
 * Must match enum Log
 */
static const char* LOG_NAMES[] =
  {
    "illegal",
    "radio",
    "main",
    "events",
    "system",
    "kernel",
    "*"
  };

/**
 * Supported operators
 */
enum RuleOperator {
  IllegalOperator = 0,
  Minus  = 1,
  Plus
};

/**
 * String names of the operators
 * Must match enum RuleOperator
 */
static const char RULEOPERATOR_NAMES[] =
{
  'i',
  '-',
  '+'
};

/**
 * The different log Levels
 * Must match levels[]
 */
enum LogLevel {
  Level_Illegal = 0,
  Level_Unknown = 1,
  Level_Default,
  Level_Vebose,
  Level_Debug,
  Level_Info,
  Level_Warn,
  Level_Error,
  Level_Fatal,
  Level_Silent,
  Level_Any
};

/**
 * Strings for the different log levels
 * Must Match LogLevel
 */
static const char* LEVELS[] =
  {
    "illegal",
    "unknown",
    "default",
    "verbose",
    "debug",
    "info",
    "warning",
    "error",
    "fatal",
    "silent",
    "*",
  };

/**
 * Structure representing a Rule
 * Rules are stored as a singly linked list.
 * Rules are evaluated backwards, last rule in the rule file wins
 */
struct Rule {
  enum RuleOperator type;
  Log log;
  enum LogLevel level;
  char* tag_p;
  char* key_word_p;
  struct EventType* eventtype_p;
  struct Rule* prev_p;
};

/**
 * Utility method, finds the index of a string in a array
 * @return a integer >= 1 if found, otherwise zero
 */
static int find_in_array(const char* str_p, const char** array_pp, int last)
{
  int i;

  for (i = 1; i <= last; ++i)
  {
    if (strncmp(array_pp[i], str_p, strlen(array_pp[i])) == 0)
    {
      return i;
    }
  }

  return 0;
}

/**
 * Utility method, maps a string to LogLevel
 */
static enum LogLevel get_loglevel(char* str_p)
{
  return (enum LogLevel)find_in_array(str_p, LEVELS, Level_Any);
}

/**
 * Utility method, maps a string to a Log
 */
static Log get_log(char* str_p)
{
  Log log = find_in_array(str_p, LOG_NAMES, Log_Any);
  return log;
}

static enum RuleOperator get_log_type(char *str_p)
{
  if (strcmp(str_p, "+") == 0)
  {
    return Plus;
  }
  else if (strcmp(str_p, "-") == 0)
  {
    return Minus;
  }
  else
  {
    return IllegalOperator;
  }
}

static enum RuleStatus valid_rule(struct Rule* rule_p)
{

  enum RuleStatus status = Valid;

  if (rule_p->type == IllegalOperator)
  {
    status = InvalidOperator;
  }
  else if (rule_p->log == Log_Illegal)
  {
    status = InvalidLog;
  }
  else if (rule_p->level == Level_Illegal)
  {
    status = InvalidLevel;
  }
  else if (rule_p->key_word_p != NULL && strcmp(rule_p->key_word_p, "*") == 0)
  {
    status = WarningKeyWord;
  }

  return status;
}

static void free_rule(struct Rule* rule_p)
{
  free(rule_p->key_word_p);
  free(rule_p->tag_p);
  free(rule_p);
}

static void free_rules(struct Rule* rules_p)
{
  struct Rule* rule_p = rules_p;

  while(rule_p != NULL)
  {
    struct Rule* prev_rule_p = rule_p->prev_p;
    free_rule(rule_p);
    rule_p = prev_rule_p;
  }
}

void free_probes(struct Probe* probe)
{
  while (probe != NULL)
  {
    struct Probe* probe_next = probe->next;
    free_rules(probe->rule);
    free(probe);
    probe = probe_next;
  }
}

/* Trims a string in-place
 *
 * It may modify the string provided and
 * return a pointer inside it.
 */
static char* trim(char* s)
{
  size_t size;
  char* end_p;

  size = strlen(s);

  if (size > 0)
  {
    end_p = s + size - 1;
    while (end_p != s && isspace(*end_p))
    {
      end_p--;
    }

    *(end_p + 1) = '\0';

    while (*s != '\0' && isspace(*s))
    {
      s++;
    }
  }
  return s;
}

static enum RuleStatus parse_rule(char* line_p, struct Rule** rule_pp)
{
  char tmp_line[LINE_LENGTH + 1] = {0};
  char* save_p = NULL;
  char* tag_p = NULL;
  enum RuleStatus status = Valid;
  struct Rule* rule_p = NULL;
  strncpy(tmp_line, line_p, LINE_LENGTH);
  tmp_line[LINE_LENGTH] = '\0';

  char* type_p = tokenize_r(tmp_line, delimiters, &save_p);
  char* log_p = tokenize_r(NULL, delimiters, &save_p);
  char* level_p = tokenize_r(NULL, delimiters, &save_p);
  if (*save_p == '"')
  {
    tokenize_r(NULL, "\"", &save_p);
    tag_p = tokenize_r(NULL, "\"", &save_p);
    tokenize_r(NULL, delimiters, &save_p);
  }
  else
  {
    tag_p = tokenize_r(NULL, delimiters, &save_p);
  }
  char *keyword_p = tokenize_r(NULL, "\n", &save_p);

  if (type_p != NULL && log_p != NULL && level_p != NULL && tag_p != NULL)
  {
    rule_p = malloc(sizeof(struct Rule));
    rule_p->key_word_p = NULL;
    rule_p->prev_p = NULL;
    rule_p->eventtype_p = NULL;

    rule_p->type   = get_log_type(type_p);
    rule_p->log    = get_log(log_p);
    rule_p->level  = get_loglevel(level_p);
    rule_p->tag_p   = strndup(tag_p, KiB);

    if (keyword_p != NULL)
    {
      keyword_p = trim(keyword_p);
      if (*keyword_p != '\0')
      {
        rule_p->key_word_p = strndup(keyword_p, KiB);
      }
    }
    status = valid_rule(rule_p);
    *rule_pp = rule_p;
  }
  else
  {
    status = IncorrectNumberOfTokens;
  }

  return status;
}

static bool skip_line(char* line_p)
{
  while(*line_p != '\0')
  {
    if (*line_p == '#')
    {
      return true;
    }
    else if (!isspace(*line_p))
    {
      return false;
    }

    line_p++;
  }

  return true;
}

static bool matches(struct Rule* rule_p, Log log, struct AndroidLogEntry_t* entry_p)
{
  if (rule_p->log == Log_Any ||
      rule_p->log == log)
  {
    if  (rule_p->level == Level_Any ||
         (rule_p->type == Plus &&
          rule_p->level <= (entry_p->priority + 1)) ||
         (rule_p->type == Minus &&
          rule_p->level >= (entry_p->priority + 1)))
    {
      if (rule_p->tag_p[0] == '*' ||
          strcmp(rule_p->tag_p, entry_p->tag) == 0)
      {
        if (rule_p->key_word_p == NULL ||
            strstr(entry_p->message, rule_p->key_word_p) != NULL)
        {
          return true;
        }
      }
    }
  }

  return false;
}

static bool matches_binary(struct Rule* rule_p, Log log, uint32_t tag_code,
                           char* eventdata, int eventlen)
{
  if (rule_p->log == Log_Any ||
      rule_p->log == log)
  {
    if ((rule_p->tag_p[0] == '*') ||
        (rule_p->eventtype_p && (rule_p->eventtype_p->code == tag_code)))
    {
      if ((rule_p->key_word_p == NULL) ||
          (memmem(eventdata, eventlen, rule_p->key_word_p, strlen(rule_p->key_word_p)) != NULL))
      {
        return true;
      }
    }
  }

  return false;
}

bool rule_exclude(struct Rule* rules_p, Log log, struct AndroidLogEntry_t* entry_p)
{
  struct Rule* rule_p;

  for (rule_p = rules_p; rule_p != NULL; rule_p = rule_p->prev_p)
  {
    if (matches(rule_p, log, entry_p))
    {
      break;
    }
  }

  return (rule_p == NULL || rule_p->type == Minus);
}

struct EventType* rule_include_binary(struct Rule* rules_p, Log log, uint32_t tag_code,
                                      char* eventdata, int eventlen)
{
  struct Rule* rule_p;
  struct EventType* eventtype_p = NULL;

  for (rule_p = rules_p; rule_p != NULL; rule_p = rule_p->prev_p)
  {
    if (matches_binary(rule_p, log, tag_code, eventdata, eventlen))
    {
      // If the tag in the rule is a wildcard, the tag description is not prefetched.
      eventtype_p = ((rule_p->tag_p[0] == '*') ? search_eventtypes(tag_code) : rule_p->eventtype_p);
      break;
    }
  }

  return ((rule_p && (rule_p->type == Plus)) ? eventtype_p : NULL);
}

static void parse_line(char* line, int* line_nr, struct Rule** rule_tail_p, int* error_count)
{
  struct Rule* rule_p = NULL;
  enum RuleStatus status = Valid;

  (*line_nr)++;
  if (skip_line(line))
  {
    return;
  }

  status = parse_rule(line, &rule_p);

  if (status == Valid && rule_p != NULL)
  {
    rule_p->prev_p = *rule_tail_p;
    *rule_tail_p = rule_p;
  }

  if (status != Valid)
  {
    (*error_count)++;

    if (*error_count == 1)
    {
      fprintf(stderr, "When parsing rules file\n");
    }

    if (status == WarningKeyWord)
    {
      fprintf(stderr, "%d: warning: using * for keyword, this is probably not desired\n", *line_nr);
    }
    else
    {
      fprintf(stderr, "%d: error: %s\n", *line_nr, ERROR_STRINGS[status]);
    }

    if (rule_p != NULL)
    {
      free_rule(rule_p);
    }
  }
}

static struct Rule* parse_rules_from_fp(FILE* in)
{
  char line[LINE_LENGTH + 1] = {0};
  struct Rule* rule_tail_p = NULL;
  int line_nr = 0;
  int error_count = 0;

  while (fgets(line, sizeof(line), in) != NULL)
  {
    parse_line(line, &line_nr, &rule_tail_p, &error_count);
  }

  return rule_tail_p;
}

struct Rule* parse_rules_from_str(char* rules)
{
  char* line;
  char* save_p;
  struct Rule* rule_tail_p = NULL;
  int line_nr = 0;
  int errorCount = 0;

  for (line = strtok_r(rules, "\n", &save_p); line; line = strtok_r(NULL, "\n", &save_p))
  {
    parse_line(line, &line_nr, &rule_tail_p, &errorCount);
  }

  return rule_tail_p;
}

struct Probe* create_probe(struct Rule* rule, int plugin_id)
{
  struct Probe* probe = NULL;
  if (rule)
  {
    probe = malloc(sizeof(struct Probe));
    probe->rule = rule;
    probe->plugin_id = plugin_id;
    probe->next = NULL;
  }
  return probe;
}

void probe_add(struct Probe** probe, struct Probe* new_probe)
{
  if (new_probe)
  {
    new_probe->next = *probe;
    *probe = new_probe;
  }
}

struct Probe* parse_rules(char* path_p)
{
  if (!path_p)
  {
    return NULL;
  }
  FILE* fp = fopen(path_p, "r");
  struct Probe* probe = NULL;
  if (fp != NULL)
  {
    probe = create_probe(parse_rules_from_fp(fp), (int)IDDD__PLUGIN_ID__IDD);
    fclose(fp);
  }
  return probe;
}

struct Probe* get_probes_from_config(IDD_t* idd_p)
{
  struct Probe* probe = NULL;
  int fd = idd_get_config(idd_p, "idd", "LogReader2");
  if (fd < 0)
  {
    fd = idd_get_config(idd_p, "idd", "LogReader");
  }
  if (fd >= 0)
  {
    FILE* fp = fdopen(fd, "r");

    if (fp == NULL)
    {
      IDD_LOGI("Failed to read config");
      close(fd);
    }
    else
    {
      probe = create_probe(parse_rules_from_fp(fp), (int)IDDD__PLUGIN_ID__IDD);
      fclose(fp);
    }
  }
  return probe;
}


bool log_enabled(struct Probe* probe, Log log)
{
  bool enabled = false;

  while ((probe != NULL) && !enabled)
  {
    struct Rule* rule = probe->rule;
    while ((rule != NULL) && !enabled)
    {
      enabled = (rule->log == log || rule->log == Log_Any) && rule->type == Plus;
      rule = rule->prev_p;
    }
    probe = probe->next;
  }

  return enabled;
}

 static void get_included_binary_tags_helper(struct Probe* probe,
                                             Log log)
{
  struct Rule* rules_p = probe->rule;

  // Traverse the rules for the IDD plugin
  while (rules_p != NULL)
  {
    // Fínd the rules for given binary log
    if (rules_p->log == log)
    {
      if (rules_p->tag_p[0] != '*')
      {
        rules_p->eventtype_p = find_event_type_from_tag(rules_p->tag_p);
      }
    }
    rules_p = rules_p->prev_p;
  }
}

 void get_included_binary_tags(struct Probe* probe, Log log)
 {
   // Traverse the IDD plugins.
   for (; probe; probe = probe->next)
   {
     get_included_binary_tags_helper(probe, log);
   }
 }

void print_probes(struct Probe* probe)
{
  for (; probe; probe = probe->next)
  {
    struct Rule* rule;
    IDD_LOGI("Rules for plugin_id %d", probe->plugin_id);
    for (rule = probe->rule; rule; rule = rule->prev_p)
    {
        IDD_LOGI("  <%s><%c><%s><%s><%s> (%d)",
                 LOG_NAMES[rule->log], RULEOPERATOR_NAMES[rule->type], LEVELS[rule->level],
                 rule->tag_p, rule->key_word_p, rule->eventtype_p ? rule->eventtype_p->code : 0);
    }
  }
}
