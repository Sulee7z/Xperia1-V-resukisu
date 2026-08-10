/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief Reads /dev/log/ and writes all log messages to iddd
 *
 * @author Carl-Emil Lagerstedt (carl-emil.lagerstedt@sonyericsson.com)
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "idd_config.h"
#include <unistd.h>

#include <errno.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>

#ifdef DEBUG_BUILD
#include <cutils/properties.h>
#endif

#include <log/event_tag_map.h>
#include "logreader_error_probe.h"
#include "util.h"
#include  "../api/idd_api.h"
// Reuse logging macros from iddd. LOG_TAG must come before the include.
#undef LOG_TAG
#define LOG_TAG "idd-logreader"
#include "../iddd/logging.h"
#include "rules.h"
#include "config.h"

#define LOGMESSAGE_PKG "idd.system_logging"
#define LOGMESSAGE_NAME "LogMessage"

#ifdef DEBUG_BUILD
#define PROPERTY_ENABLE "0"
#endif
#define UNUSED_PARAM(x) ((void)(x))

static const char* idd_path;

IDD_t* idd_p = NULL;

bool setting_verbose = false;
static bool setting_test_mode = false;
static char* test_file_p = NULL;

#ifdef DEBUG_BUILD
enum {
  ENABLE_LOGREADER,
  DISABLE_LOGREADER,
} property_state;

#endif

/**
 *  Map Android log id (log_id_t) and corresponding IDD value.
 */
struct log_filter
{
  Log id;
  bool enabled;
};
static struct log_filter log_ids[] =
{
  {Log_Main,    false},  // LOG_ID_MAIN = 0
  {Log_Radio,   false},  // LOG_ID_RADIO = 1
  {Log_Events,  false},  // LOG_ID_EVENTS = 2
  {Log_System,  false},  // LOG_ID_SYSTEM = 3
  {Log_Illegal, false},  // LOG_ID_CRASH = 4
  {Log_Illegal, false},  // LOG_ID_STATS = 5
  {Log_Illegal, false},  // LOG_ID_SECURITY = 6
  {Log_Kernel,  false}   // LOG_ID_KERNEL = 7
};

#define NR_LOG_IDS ((int)(sizeof(log_ids) / sizeof(log_ids[0])))

static bool new_config = true;    // When true, the log reader thread should read the IDD
                                  // configuration.
static pthread_mutex_t new_config_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t new_config_cond = PTHREAD_COND_INITIALIZER;

/**
 * The list of probes with its rules
 */
static struct Probe* g_probes;

/**
 * Default rules path
 */
static char* DEFAULT_RULES_PATH = IDD_DEFAULT_BASE_DIR "logreader.rules";

static char* event_map_file_p = NULL;

static struct pollfd config_pollfd = {0};

static struct logger_list* logger_list = NULL;

/**
 * Releases a field
 */
void free_field(Idd__SystemLogging__Field* field_p);

/*
 * Used when parsing binary events. Extracts a 4-byte value from a byte stream.
 */
static inline uint32_t read_4le(const uint8_t* src)
{
    return src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);
}

/**
 * Used when parsing binary events. Extracts an 8-byte value from a byte strem.
 */
static inline uint64_t get8LE(const uint8_t* src)
{
    uint32_t low, high;

    low = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);
    high = src[4] | (src[5] << 8) | (src[6] << 16) | (src[7] << 24);
    return ((uint64_t) high << 32) | (uint64_t) low;
}


void print_field(Idd__SystemLogging__Field* pField);

/**
 * Prints a list of fields
 */
void print_list (Idd__SystemLogging__List* list_p) {
  if (list_p != NULL) {
    size_t j;
    for (j =0; j < list_p->n_fields; ++j) {
      print_field(list_p->fields[j]);


      if (j < (list_p->n_fields - 1)){
        printf(", ");
      }
    }
  }
}

/**
 * Prints the content of a field in a binary LogMessage
 */
void print_field(Idd__SystemLogging__Field* field_p) {
  if (field_p != NULL) {
    if (field_p->list != NULL) {
      printf("[LIST ");
      if (field_p->name != NULL) {
        printf("%s", field_p->name);
      }

      if (field_p->has_unit) {
        printf (" %s", dataunit_to_string(field_p->unit));
      }

      print_list(field_p->list);

      printf("]");
    }
    else
    {
      if (field_p->name != NULL) {
        printf(" %s", field_p->name);
      }

      if (field_p->has_unit) {
        printf (" %s", dataunit_to_string(field_p->unit));
      }

      if (field_p->has_int_value) {
        printf ("(%ld) " , (long) field_p->int_value);
      }

      if (field_p->string_value != NULL) {
        printf ("(%s) ", field_p->string_value);
      }

      if (field_p->has_float_value) {
        printf ("(%f) ", field_p->float_value);
      }

    }
  }
}

/**
 * Prints the structure and contents of a binary log message
 */
void print_binarymsg(Idd__SystemLogging__LogMessage* msg_p) {
  size_t i;
  if (msg_p == NULL) {
    return;
  }

  printf("%s ", msg_p->tag);

  if (msg_p->list != NULL) {
    for (i = 0; i < msg_p->list->n_fields; i++) {
      print_field(msg_p->list->fields[i]);
    }
  }

  printf("\n");
}

#ifdef DEBUG_BUILD

/**
 * Conversion of property type.
 */
char* convert_prop_to_char(int property_state, char *property_value)
{
    if (snprintf(property_value, PROPERTY_VALUE_MAX, "%d", property_state) == -1) {
    //This means snprintf failed for some reason, so lets return enabled property to be safe.
        property_value = PROPERTY_ENABLE;
    }
    return property_value;
}

/**
 * Getting logreader property.
 */
int get_logreader_property() {
  char value[PROPERTY_VALUE_MAX] = {0}, prop_enable[PROPERTY_VALUE_MAX] = {0};
  property_get("persist.vendor.idd.logreader", value, convert_prop_to_char(ENABLE_LOGREADER, prop_enable));
  return atoi(value);
}

/**
 * Check property to run idd-logreader.
 */
bool is_logreader_enabled() {
  int property = get_logreader_property();
  if (property != DISABLE_LOGREADER) {
    return true;
  } else {
    return false;
  }
}

#endif

/**
 * Use for pretty printing
 */
char prio_to_char (android_LogPriority pri)
{
  switch (pri)
  {
  case ANDROID_LOG_VERBOSE:       return 'V';
  case ANDROID_LOG_DEBUG:         return 'D';
  case ANDROID_LOG_INFO:          return 'I';
  case ANDROID_LOG_WARN:          return 'W';
  case ANDROID_LOG_ERROR:         return 'E';
  case ANDROID_LOG_FATAL:         return 'F';
  case ANDROID_LOG_SILENT:        return 'S';
  case ANDROID_LOG_DEFAULT:
  case ANDROID_LOG_UNKNOWN:
  default:                        return '?';
  }
}

/**
 * Splits a wire-format buffer into an AndroidLogEntry
 * entry allocated by caller. Pointers will point directly into buf
 *
 * Returns 0 on success and -1 on invalid wire format (entry will be
 * in unspecified state)
 */
static int logreader_android_log_processLogBuffer(struct logger_entry* buf,
                                 AndroidLogEntry *entry)
{
  size_t tag_len;
  char *msg = (char *)buf + buf->hdr_size;

  if (msg == NULL)
  {
    IDD_LOGREADER_PE("%s: message field from logger_entry is null.", __func__);
    return -1;
  }

  entry->tv_sec = buf->sec;
  if (entry->tv_sec < 0)
  {
    IDD_LOGREADER_PE("%s: second(%ld) is invalid.", __func__, entry->tv_sec);
    return -1;
  }
  entry->tv_nsec = buf->nsec;
  if (entry->tv_nsec < 0)
  {
    IDD_LOGREADER_PE("%s: nano second(%ld) is invalid.", __func__, entry->tv_nsec);
    return -1;
  }
  entry->priority = msg[0];
  if ((int)entry->priority < (int)ANDROID_LOG_UNKNOWN)
  {
    IDD_LOGREADER_PE("%s: priority(%d) is invalid.", __func__, (int)entry->priority);
    return -1;
  }
  entry->pid = buf->pid;
  if (entry->pid < 0)
  {
    IDD_LOGREADER_PE("%s: pid(%d) is invalid.", __func__, entry->pid);
    return -1;
  }
  entry->tid = buf->tid;
  if (entry->tid < 0)
  {
    IDD_LOGREADER_PE("%s: tid(%d) is invalid.", __func__, entry->tid);
    return -1;
  }
  entry->tag = msg + 1;
  if (entry->tag == NULL)
  {
    IDD_LOGREADER_PE("%s: tag is null.", __func__);
    return -1;
  }
  tag_len = strlen(entry->tag);
  entry->message = entry->tag + tag_len + 1;
  if (entry->message == NULL)
  {
    IDD_LOGREADER_PE("%s: message(%s) is null.", __func__, entry->tag);
    return -1;
  }
  int messageLen = buf->len - tag_len - 3;
  if (messageLen < 0)
  {
    IDD_LOGREADER_PE("%s: message length(%s, %d) is invalid.", __func__, entry->tag, messageLen);
    return -1;
  }
  entry->messageLen = (size_t)messageLen;

  return 0;
}

/**
 * Frees a list of fields
 */
void free_field_list(Idd__SystemLogging__List* list_p) {
  if (list_p != NULL) {
    for (size_t i  = 0; i < list_p->n_fields; ++i) {
      free_field(list_p->fields[i]);
    }

    FREE(list_p->fields);
    FREE(list_p);
  }
}

void free_field(Idd__SystemLogging__Field* field_p)
{
  if (field_p != NULL) {
    free_field_list(field_p->list);
    FREE(field_p->name);
    FREE(field_p->string_value);
    FREE(field_p);
  }
}

/**
 * Frees the protocol buffer and all its children
 */
void free_log_message(Idd__SystemLogging__LogMessage* msg_p)
{
  if (msg_p != NULL) {
    free_field_list(msg_p->list);
    FREE(msg_p->tag);
    FREE(msg_p->msg);
    FREE(msg_p);
  }
}

uint32_t read_tag_code(struct logger_entry* buf)
{

  const unsigned char *eventData = (const unsigned char *)buf + buf->hdr_size;
  size_t in_count = buf->len;

  if (in_count < 4) {
    return 0;
  }

  uint32_t tag_code = read_4le(eventData);

  return tag_code;
}

static Idd__SystemLogging__List* create_list(int length) {
  Idd__SystemLogging__List* list_p = malloc(sizeof(*list_p));
  idd__system_logging__list__init(list_p);
  list_p->fields = malloc(length * sizeof(Idd__SystemLogging__Field *));
  list_p->n_fields = length;

  return list_p;
}

/**
 * Converts the fields of  binary events to its corresponding protobuf
 */
Idd__SystemLogging__Field* convert_binary_event_helper(const unsigned char** event_data_p,
                                                       size_t* event_data_len_p,
                                                       size_t* index,
                                                       struct EventType* event_type)
{
  const unsigned char* eventData = *event_data_p;
  size_t eventDataLen = *event_data_len_p;
  unsigned char type;
  struct Field* event_descr_p;
  Idd__SystemLogging__Field* field_p = NULL;
  size_t num_fields = event_type->num_fields;
  struct Field** event_descr_pp = event_type->fields_pp;

  event_descr_p = event_descr_pp != NULL &&
    *index < num_fields ?  event_descr_pp[*index] :  NULL;

  if (eventDataLen < 1) {
    goto ERROR;
  }

  type = *eventData++;
  eventDataLen--;

  field_p = calloc(1, sizeof(*field_p));

  if (field_p == NULL) {
    goto ERROR;
  }

  idd__system_logging__field__init(field_p);

  if (event_descr_p != NULL && type != EVENT_TYPE_LIST) {
    if (event_descr_p->name_p != NULL) {
      field_p->name = strndup(event_descr_p->name_p, KiB);
    }
    if (event_descr_p->unit != UNIT_NOT_SET) {
      field_p->has_unit = 1;
      field_p->unit = event_descr_p->unit;
    }
  }

  switch (type) {
  case EVENT_TYPE_INT:
    /* 32-bit signed int */
    {
      int32_t ival;

      if (eventDataLen < 4) {
        goto ERROR;
      }

        ival = (int32_t)read_4le(eventData);
        eventData += 4;
        eventDataLen -= 4;

        field_p->has_int_value = 1;
        field_p->int_value = ival;

        (*index)++;
    }
    break;
  case EVENT_TYPE_LONG:
    /* 64-bit signed long */
    {
      int64_t lval;

      if (eventDataLen < 8) {
        goto ERROR;
      }

      lval = get8LE(eventData);
      eventData += 8;
      eventDataLen -= 8;

      field_p->has_int_value = 1;
      field_p->int_value = lval;

      (*index)++;
    }
    break;
  case EVENT_TYPE_FLOAT:
    /* float */
    {
      uint32_t ival;
      float fval;

      if (eventDataLen < 4) {
        goto ERROR;
      }
      ival = read_4le(eventData);
      fval = *(float*)&ival;
      eventData += 4;
      eventDataLen -= 4;

      field_p->has_float_value = 1;
      field_p->float_value = fval;

      (*index)++;
      break;
    }
  case EVENT_TYPE_STRING:
    /* UTF-8 chars, not NULL-terminated */
    {
      uint32_t strLen;

      if (eventDataLen < 4) {
        goto ERROR;
      }

      strLen = read_4le(eventData);
      eventData += 4;
      eventDataLen -= 4;

      if (eventDataLen < strLen) {
        goto ERROR;
      }

      field_p->string_value = malloc(strLen + 1);

      if (field_p->string_value == NULL) {
        goto ERROR;
      }

      memcpy(field_p->string_value, eventData, strLen);
      field_p->string_value[strLen] = '\0';

      eventData += strLen;
      eventDataLen -= strLen;

      (*index)++;
      break;
    }
  case EVENT_TYPE_LIST:
    /* N items, all different types */
    {
      Idd__SystemLogging__List* list_p;
      unsigned char count;
      bool parsed_ok = true;

      if (eventDataLen < 1) {
        goto ERROR;
      }

      count = *eventData++;
      eventDataLen--;

      list_p = create_list(count);

      for (int i = 0; i < count; i++) {
        Idd__SystemLogging__Field* field_p2 = NULL;
        if (parsed_ok == true)
        {
          field_p2 =
            convert_binary_event_helper(&eventData,
                                        &eventDataLen,
                                        index,
                                        event_type);
          if (!field_p2) {
            /* Unknown event type! We are lost and must cancel the parsing! */
            parsed_ok = false;
          }
        }
        if (parsed_ok)
        {
          list_p->fields[i] = field_p2;
        }
        else
        {
          /* Field needed to avoid crash in idd__system_logging__log_message__get_packed_size() */
          list_p->fields[i] = malloc(sizeof(Idd__SystemLogging__Field));
          idd__system_logging__field__init(list_p->fields[i]);
        }
      }

      list_p->n_fields = count;
      field_p->list = list_p;
    }
    break;
  default:
    IDD_LOGREADER_PE("Uknown binary event type %d! Failed to parse tag %d <%s>.",
                     type, event_type->code, event_type->tag_p);
    goto ERROR;
  }

  goto NO_ERROR;

 ERROR:
  free_field(field_p);
  field_p = NULL;

 NO_ERROR:
    *event_data_p = eventData;
    *event_data_len_p = eventDataLen;


  return field_p;
}

Idd__SystemLogging__LogMessage* convert_binary_event_to_protobuf(struct logger_entry* buf_p,
                                                            struct EventType* event_descr_p)
{
    /*
     * Pull the tag out.
     */
    const unsigned char *eventData = (const unsigned char *)buf_p + buf_p->hdr_size;
    size_t in_count = buf_p->len;

    uint32_t tag_code = read_tag_code(buf_p);
    eventData += 4;
    in_count -= 4;

    Idd__SystemLogging__LogMessage* binary_msg_p = calloc(1, sizeof(*binary_msg_p));

    if (binary_msg_p == NULL) {
      return NULL;
    }

    idd__system_logging__log_message__init(binary_msg_p);

    binary_msg_p->has_priority = true;
    binary_msg_p->priority = ANDROID_LOG_INFO;

    if (event_descr_p != NULL) {
      size_t index = 0;
      Idd__SystemLogging__Field* field_p =
        convert_binary_event_helper(&eventData,
                                    &in_count,
                                    &index,
                                    event_descr_p);

      if (field_p) {
        binary_msg_p->list = create_list(1);
        if (binary_msg_p->list != NULL) {
          binary_msg_p->list->fields[0] = field_p;
        }
        else {
          free_field(field_p);
        }
      }

      if (event_descr_p->tag_p != NULL) {
        binary_msg_p->tag = strndup(event_descr_p->tag_p, KiB);
      }
    }

    /*
     * If we dont have a name just generate one
     */
    if (binary_msg_p->tag == NULL) {
      binary_msg_p->tag = malloc(20);
      if (binary_msg_p->tag != NULL) {
        snprintf(binary_msg_p->tag, 20, "[%d]", tag_code);
      }
    }

    /* eat the silly terminating '\n' */
    if (in_count == 1 && *eventData == '\n') {
        eventData++;
        in_count--;
    }

    if (in_count != 0 && setting_verbose) {
      IDD_LOGW("Leftover binary log data (%zu bytes)\n", in_count);
    }

    return binary_msg_p;
}

static Idd__SystemLogging__LogMessage* process_binary_message(struct Rule* rule,
                                                              Log log,
                                                              struct logger_entry* entry_p)
{
  const unsigned char *msg = (const unsigned char *)entry_p + entry_p->hdr_size;
  uint32_t tag_code = read_tag_code(entry_p);
  struct EventType* eventtype_p = NULL;

  if ((tag_code == 0) ||
      ((eventtype_p = rule_include_binary(
              rule, log, tag_code, msg, entry_p->len)) == NULL))
  {
    /* tag code is not included */
    return NULL;
  }

  Idd__SystemLogging__LogMessage* msg_p =
    convert_binary_event_to_protobuf(entry_p, eventtype_p);

  if (msg_p != NULL) {
    msg_p->log = (Idd__SystemLogging__Log)log;

    if (setting_verbose) {
      printf("+");
      print_binarymsg(msg_p);
    }
  }

  return msg_p;
}

static Idd__SystemLogging__LogMessage* process_message(struct Rule* rule, Log log,
                                                       AndroidLogEntry* log_entry_p)
{
  Idd__SystemLogging__LogMessage* log_message_p = NULL;

  /* Don't log our own log messages (recursion if we did)
     This is potentially really really dangerous. If some part of idd
     logs something that we dont filter out here we might end up in an
     infinite loop logging our own messages over and over.
     Therefore: For production use we should never use * in rules for any tag
  */
  if (strncmp(log_entry_p->tag, LOG_TAG, strlen(LOG_TAG)) == 0 ||
        strncmp(log_entry_p->tag, "iddd-events", strlen("iddd-events")) == 0) {
    return NULL;
  }

  bool skip = rule_exclude(rule, log, log_entry_p);

  if (setting_verbose)
  {
    IDD_LOGI("%c %d %c tag=(%s) message=(%s)\n",
             skip ? 'X' : 'I',
             log,
             prio_to_char(log_entry_p->priority),
             log_entry_p->tag,
             log_entry_p->message);
  }

  if (!skip)
  {
    log_message_p = calloc(1, sizeof(*log_message_p));

    if (log_message_p != NULL) {
      idd__system_logging__log_message__init(log_message_p);
      log_message_p->log = log;

      log_message_p->has_priority = log_entry_p->priority;
      log_message_p->priority = log_entry_p->priority;

      log_message_p->has_tid = true;
      log_message_p->tid = log_entry_p->tid;

      log_message_p->tag = strndup(log_entry_p->tag, KiB);
      log_message_p->msg = strndup(log_entry_p->message, MIN(log_entry_p->messageLen, LOGGER_ENTRY_MAX_LEN));
    }

  }

  return log_message_p;
}

/**
 *
 */
static Log get_log(log_id_t log_id)
{
  return (log_id >= 0 && log_id < NR_LOG_IDS) ? log_ids[log_id].id : Log_Illegal;
}

/**
 *
 */
static void handle_androidlog(struct log_msg* log_msg)
{
  AndroidLogEntry log_entry;
  memset(&log_entry, 0, sizeof(log_entry));
  Idd__SystemLogging__LogMessage* msg_p = NULL;
  struct Probe* probe;
  Log log;

  if (logreader_android_log_processLogBuffer(&log_msg->entry, &log_entry) == -1)
  {
    IDD_LOGREADER_PE("%s: Incoming entry was invalid, skips.", __func__);
    return;
  }
  if ((log = get_log(log_msg->entry.lid)) == Log_Illegal)
  {
    IDD_LOGREADER_PE("%s: Unexpected log id %d.", __func__, log_msg->entry.lid);
    return;
  }

  for (probe = g_probes; probe; probe = probe->next)
  {
    if (log == Log_Events) {
      msg_p = process_binary_message(probe->rule, log, &log_msg->entry);
    } else {
      msg_p = process_message(probe->rule, log, &log_entry);
    }

    if (msg_p != NULL) {
      msg_p->has_tv_sec = true;
      msg_p->tv_sec = log_entry.tv_sec;

      msg_p->has_tv_nsec = true;
      msg_p->tv_nsec = log_entry.tv_nsec;

      msg_p->has_pid = log_entry.pid;
      msg_p->pid = log_entry.pid;

      msg_p->has_tid = log_entry.tid;
      msg_p->tid = log_entry.tid;

      /* ok, we have the data, turn it into a protocol buffer */
      size_t size = idd__system_logging__log_message__get_packed_size(msg_p);
      unsigned char pb_buffer[LOGGER_ENTRY_MAX_LEN + 1] = {0};

      if (size < sizeof(pb_buffer)) {
        size = idd__system_logging__log_message__pack(msg_p, pb_buffer);
        idd_add_plugin_event(idd_p, probe->plugin_id, LOGMESSAGE_PKG, LOGMESSAGE_NAME,
                             (char*)pb_buffer, size);
      }

      free_log_message(msg_p);
    }
  }
}

/**
 * Return true if the new IDD configuration have different Android logs enabled than the current.
 */
static bool logging_changed()
{
  bool changed = false;

  for (log_id_t i = 0; i < NR_LOG_IDS; i++)
  {
    if (log_ids[i].id != Log_Illegal) {
      bool enabled_in_new = log_enabled(g_probes, log_ids[i].id);
      if (enabled_in_new != log_ids[i].enabled)
      {
        log_ids[i].enabled = enabled_in_new;
        changed = true;
      }
    }
  }
  if (!changed)
  {
    IDD_LOGI("Same logs enabled as before!");
  }
  return changed;
}

static void update_log_events_binary_tags()
{
  for (log_id_t i = 0; i < NR_LOG_IDS; i++)
  {
    if (log_ids[i].enabled && log_ids[i].id == Log_Events)
    {
      get_included_binary_tags(g_probes, Log_Events);
    }
  }
}

/**
 * Update our Android logger to correspond to the latest IDD configuration for Android logs.
 */
static int open_androidlogger()
{
  static bool first_start = true;
  int nr_enabled = 0;
  log_time log_start_time = {0};

  android_logger_list_free(logger_list);
  // If first start, get all available logs (start time = 0).
  // Otherwise, get only logs from now to minimize the number of duplicates.
  if (!first_start)
  {
    log_start_time.tv_sec = time(NULL);
  }
  else
  {
    first_start = false;
  }

  errno = 0;
  logger_list = android_logger_list_alloc_time(O_RDONLY, log_start_time, 0);
  if (!logger_list)
  {
    IDD_LOGREADER_PE("Failed to allocate logger, error = %s", strerror(errno));
    return 0;
  }

  for (log_id_t i = 0; i < NR_LOG_IDS; i++)
  {
    if (log_ids[i].enabled)
    {
      struct logger* logger = android_logger_open(logger_list, i);
      if (logger == NULL)
      {
        IDD_LOGREADER_PE("Failed to create logger for %d, error = %s", i, strerror(errno));
      }
      else
      {
        nr_enabled++;
      }
    }
  }
  return nr_enabled;
}

/**
 * Read and handle the logreader part of IDD configuration.
 */
static void handle_new_config(int *nr_enabled)
{
  /* Read the android log rules */
  free_probes(g_probes);
  g_probes = get_probes_from_config(idd_p);
  if (g_probes == NULL)
  {
    // No rules from IDDD - use default ones, if we have any.
    IDD_LOGI("No rules from daemon - use default");
    g_probes = parse_rules(DEFAULT_RULES_PATH);
  }
  if (g_probes == NULL)
  {
    IDD_LOGREADER_PE("No rules");
  }

  config_read(idd_p);
  config_probes_add(&g_probes);

  if (logging_changed())
  {
    /* Reopen android logger */
    *nr_enabled = open_androidlogger();
  }
  update_log_events_binary_tags();
  IDD_LOGI("Configuration contained %d logs", *nr_enabled);
  if (setting_verbose)
  {
    print_probes(g_probes);
  }
}

/**
 *
 */
static void set_config_pollfd()
{
  config_pollfd.fd = idd_get_config_notify(idd_p);
  if (config_pollfd.fd == -1)
  {
    IDD_LOGREADER_PE("Failed to open open IDD config change handler, errno %d <%s>",
                     errno, strerror(errno));
  }
  else
  {
    config_pollfd.events = POLLIN;
    config_pollfd.revents = 0;
  }
}

/**
 * The log reader thread.
 *  - When data is available in any of the logs, run it through logreader filter.
 *    If filter match, forward it to iddd.
 *  - Handles new configuration.
 */
static void *read_log()
{
  struct log_msg log_msg;
  int nr_enabled_log_ids = 0;     // Number of android logs to watch
  int ret;

  while (true)
  {
    // Handle Android log
    if (nr_enabled_log_ids > 0)
    {
      ret = android_logger_list_read(logger_list, &log_msg);
      if (ret > 0)
      {
        handle_androidlog(&log_msg);
      }
      else
      {
        IDD_LOGREADER_PE("android_logger_list_read(%p) returned %d. nr_enabled_log_ids=%d, "
                         "Reopen Android logger.",
                         logger_list, ret, nr_enabled_log_ids);
        nr_enabled_log_ids = open_androidlogger();
        IDD_LOGI("Reopened %d logs", nr_enabled_log_ids);
        continue;
      }
    }

    // Handle new IDD configuration
    pthread_mutex_lock(&new_config_mutex);
    if (nr_enabled_log_ids == 0)
    {
      while (new_config == false)
      {
        // No enabled logs! Wait here until a new IDD configuration is available.
        ret = pthread_cond_wait(&new_config_cond, &new_config_mutex);
      }
    }
    if (new_config)
    {
      new_config = false;
      handle_new_config(&nr_enabled_log_ids);
      IDD_LOGI("New configuration handled!");
    }
    pthread_mutex_unlock(&new_config_mutex);
  }
  return NULL;
}

/**
 * The main loop
 * - starts a thread that read and handle Android logs.
 * - listen for new configuration. If available, signal to 'log handler thread'
 *   via 'new_config = true'.
 */
static void process_log()
{
  bool loop_on = true;
  pthread_t log_handler_tid;
  int ret = pthread_create(&log_handler_tid, NULL, &read_log, NULL);
  if (ret != 0)
  {
    IDD_LOGREADER_PE("Failed to create log handler thread, exit logreader. errno %d <%s>",
                     ret, strerror(ret));
    loop_on = false;
  }

  while (loop_on)
  {
    #ifdef DEBUG_BUILD
    if(!is_logreader_enabled()) {
      return;
    }
    #endif

    ret = poll(&config_pollfd, 1, -1);
    if (ret > 0)
    {
      if (config_pollfd.revents & config_pollfd.events)
      {
        errno = 0;
        ret = idd_config_wait(idd_p, config_pollfd.fd);
        if (ret > 0)
        {
          // The 'new IDD configuration available' signal has been caught.
          pthread_mutex_lock(&new_config_mutex);
          // Inform the android log reader thread.
          new_config = true;
          pthread_cond_signal(&new_config_cond);
          pthread_mutex_unlock(&new_config_mutex);
          IDD_LOGI("New configuration available!");
        }
        else
        {
          IDD_LOGW("New configuration returned %d", ret);
          if (errno)
          {
            IDD_LOGREADER_PE("New configuration returned %d %s. revents 0x%0x,"
                             "Reopen. errno %d <%s>",
                             ret, ret == 0 ? " (Non-blocking?)" : "",
                             config_pollfd.revents, errno, strerror(errno));
            close(config_pollfd.fd);
            config_pollfd.fd = -1;
            set_config_pollfd();
          }
        }
      }
    }
    else if (ret < 0)
    {
      /* poll() returned a non positive int. */
      IDD_LOGREADER_PE("ret=%d, poll failed, errno = %d <%s>", ret, errno, strerror(errno));
      loop_on = false;
    }
  }

  return;
}

static void print_usage(FILE* stream_fp)
{
  fputs("usage idd-logreader [options]\n"
        "Options: \n"
        "-v, --verbose    Verbose mode\n"
        "-t, --test       Read data from a test file\n"
        "-h, --help       Prints this message\n",
        stream_fp);
}

static void parse_args(int argc, char* argv[])
{
  struct option long_options[] =
    {
      {"verbose",   no_argument,       0, 'v'},
      {"help",      no_argument,       0, 'h'},
      {"test-file", required_argument, 0, 't'},
      {"event-map", required_argument, 0, 'x'},
      {"sock",      required_argument, 0, 's'},
      {0,0,0,0}
    };

  int optionIndex = 0;
  int  c;

  while ((c = getopt_long (argc, argv,"t:e:E:vhs:", long_options, &optionIndex)) != -1)
  {
    if (c == 'v')
    {
      setting_verbose = true;
    }
    else if (c == 's')
    {
      FREE(idd_path);
      idd_path = strndup(optarg, KiB);
    }
    else if (c == 't')
    {
      setting_test_mode = true;
      IDD_LOGI("Testmode!");
      if (test_file_p != NULL) {
        FREE(test_file_p);
      }

      test_file_p = strndup(optarg, KiB);
    }
    else if (c == 'h')
    {
      print_usage(stdout);
      exit(2);
    }
    else if (c == 'x')
    {
      if (event_map_file_p != NULL) {
        FREE(event_map_file_p);
      }

      event_map_file_p = strndup(optarg, KiB);
    }
  }
}

int main(int argc, char** argv)
{
  IDD_LOGI("Starting");

  idd_path = strndup(IDD_DEFAULT_BASE_DIR, KiB);
  parse_args(argc, argv);
  idd_logreader_error_probe_init((char *)idd_path, setting_test_mode);
  idd_p = idd_create_from_path(idd_path);

  if (idd_p == NULL)
  {
    IDD_LOGREADER_PE("Failed to initialize IDD");
    exit(EXIT_FAILURE);
  }

  read_tag_map(event_map_file_p);

 // Initiate the 'New configuration' file descriptor
  config_pollfd.fd = -1;
  set_config_pollfd();

  IDD_LOGI("Started");

  process_log();

  IDD_LOGREADER_PE("Shutting down");

  return (EXIT_FAILURE);
}
