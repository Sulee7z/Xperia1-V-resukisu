/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief In Device Diagnostics Daemon
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#include <sys/file.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include "config.h"
#include "events.h"
#include "counters.h"
#include "sdata.h"
#include "util.h"
#include "compactor.h"
#include "configuration.h"
#include "filter.h"
#include "crypto.h"
#include "settings.h"
#include "file_utils.h"
#include "protos/idd/iddd.pb-c.h"
#include "plugins.h"
#include "tokenize.h"

#define LOG_TAG "iddd"
#include "app_event_packages.h"
#include "logging.h"
#include "idd_config.h"

/* Link failure on getdtablesize() in volatile-pre-l-kitakami. DMS06082904 was created for this.
 * To enable correct functionallity in IDD, IDDD_MAX_OPEN_FD replaced the function call temporary.
 */
#define IDDD_MAX_OPEN_FD 1024
#define IDDD_MAX_SOCK_BUCKET_NUMBER 5
#define IDDD_SIZE_T_DIGIT_NUMBER 20

static const char* base_dir_p = NULL;
static int detach = 1;

static void receive_dgram(idd_sock_t* sock_p);
static void receive_seq_listen(idd_sock_t* sock_p);
static void receive_stream_listen(idd_sock_t* sock_p);

/* Incoming connection-less (SOCK_DGRAM) unix domain socket */
idd_sock_t sock_cl = {receive_dgram, -1, NULL};

/* Incoming connection-oriented (SOCK_SEQPACKET) unix domain socket */
idd_sock_t sock_co = {receive_seq_listen, -1, NULL};

/* Incoming connection-oriented (SOCK_STREAM) unix domain socket */
idd_sock_t sock_co_stream = {receive_stream_listen, -1, NULL};

/* SIGPIPE happens? 0/1 */
static volatile int sigpipe;

/* Socket backet */
typedef struct
{
  int fd;
  char *head;
  char *tail;
  char *limit;
} idd_sock_bucket_t;

static idd_sock_bucket_t g_sock_buckets[IDDD_MAX_SOCK_BUCKET_NUMBER];

/* epoll socket */
static int epoll_fd = -1;

/* connection less socket */
#define SOCK_CL  (-1)

//String returned to socket client in case of failure
#define DAEMON_ERROR_STR "FAIL"

// Repeat calling 'func' if we got an EINTR signal.
#define RETRY_IF_EINTR(func) ({int res; do res = (func); while (res == -1 && errno == EINTR); res;})

/*
 * Allocate socket bucket
 *
 * @param sock_fd Socket file descriptor
 * @param size Size to be allocated
 *
 * @return 0 on finding sock bucket or non-zero valued error message on respective errors
 */
static iddd_ret_val_t alloc_sock_bucket(int sock_fd, size_t size)
{
  int i;
  if (size == 0)
  {
     IDD_LOGE("Incorrect size data %ld from client %d, dropping.", size, sock_fd);
     return IDD_CO_SOCK_INVALID_HEADER_SIZE;
  }
  for (i = 0; i < IDDD_MAX_SOCK_BUCKET_NUMBER; i++)
  {
    if (g_sock_buckets[i].head == NULL)
    { // Available empty socket bucket was found.
      g_sock_buckets[i].head = malloc(size + 1);
      if (g_sock_buckets[i].head != NULL)
      {
        g_sock_buckets[i].fd = sock_fd;
        g_sock_buckets[i].tail = g_sock_buckets[i].head;
        g_sock_buckets[i].limit = g_sock_buckets[i].head + size;
        return IDD_CO_SOCK_SEND_SUCCESS;
      }
      IDD_LOGE("Socket bucket for client %d of %zu bytes, #%d is found, but no available memory.",
                sock_fd, size, i);
      return IDD_CO_SOCK_BUCKET_MALLOC_FAIL;
    }
  }
  IDD_LOGE("No empty socket bucket for client %d of %zu bytes.", sock_fd, size);
  return IDD_CO_SOCK_NO_EMPTY_BUCKET;
}

/**
 * Free socket bucket
 *
 * @param sock_fd Socket file descriptor
 *
 * @return void
 */
static void free_sock_bucket(idd_sock_bucket_t *sock_bucket_p)
{
  if (sock_bucket_p != NULL)
  {
    free(sock_bucket_p->head);
    memset(sock_bucket_p, 0, sizeof(idd_sock_bucket_t));
  }
}

/**
 * Find socket bucket
 *
 * @param sock_fd Socket file descriptor
 *
 * @return Pointer to the socket bucket if found. Otherwise NULL.
 */
static idd_sock_bucket_t* find_sock_bucket(int sock_fd)
{
  int i;
  for (i = 0; i < IDDD_MAX_SOCK_BUCKET_NUMBER; i++)
  {
    if (sock_fd == g_sock_buckets[i].fd)
    {
      // Socket bucket was found.
      return &g_sock_buckets[i];
    }
  }
  return NULL;
}

/**
 * Send file descriptor as response to client.
 *
 * @param sock connection-oriented (SOCK_SEQPACKET) unix domain socket
 * @param sendfd file descriptor in this process
 *
 * @return 1 on success, 0 on failure
 */
static int send_fd_msg(int sock, int sendfd)
{
  struct msghdr   msg;
  struct iovec    iov[1] = {};

  union {
    struct cmsghdr    cm;
    char              control[CMSG_SPACE(sizeof(int))];
  } control_un;
  struct cmsghdr  *cmptr;

  memset(&msg, 0, sizeof(msg));

  msg.msg_control = control_un.control;
  msg.msg_controllen = sizeof(control_un.control);

  cmptr = CMSG_FIRSTHDR(&msg);
  cmptr->cmsg_len = CMSG_LEN(sizeof(int));
  cmptr->cmsg_level = SOL_SOCKET;
  cmptr->cmsg_type = SCM_RIGHTS;
  *((int *) CMSG_DATA(cmptr)) = sendfd;

  msg.msg_name = NULL;
  msg.msg_namelen = 0;

  iov[0].iov_base = "OK";
  iov[0].iov_len = 2;
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  if (RETRY_IF_EINTR(sendmsg(sock, &msg, 0)) == -1)
  {
    if (sigpipe)
    {
      sigpipe = 0;
      IDD_LOGE("Fatal signal 13 (SIGPIPE), sending to disconnected client %d", sock);
    }
    else
    {
      IDD_LOGE("Failed to send fd-message to client %d", sock);
    }
    return 0;
  }
  return 1;
}

/**
 * Version of send() handling full buffer.
 *
 * @param sock connection-oriented (SOCK_SEQPACKET) unix domain socket
 * @param buffer send buffer
 * @param buflen send buffer length
 *
 * @return buflen on success, -1 on failure
 */
ssize_t socket_send(int sock, const char *buffer, size_t buflen)
{
  ssize_t ret = -1;
  size_t remaining_bytes = buflen;
  const char *buffer_p = buffer;
  int retry_cnt = 3;

  while(retry_cnt > 0)
  {
    ssize_t sent_bytes = RETRY_IF_EINTR(send(sock, buffer_p, remaining_bytes, 0));

    if (sent_bytes < 0)
    {
      // when send buffer is full, delay and re-try
      if (errno == EAGAIN)
      {
        const int SLEEP_USECS = 500;   // 0.5 ms
        IDD_LOGD("Got EAGAIN for client %d, sleeping for %d us before retrying", sock, SLEEP_USECS);
        usleep(SLEEP_USECS);
        --retry_cnt;
        continue;
      }

      if (sigpipe)
      {
        sigpipe = 0;
        IDD_LOGE("Fatal signal 13 (SIGPIPE), sending to disconnected client %d", sock);
      }
      // timeout and other error conditions
      break;
    }

    if ((size_t)sent_bytes == remaining_bytes)
    {
      ret = buflen;
      break;
    }
    else
    {
      retry_cnt = 3;
      remaining_bytes -= sent_bytes;
      buffer_p += sent_bytes;
    }
  }

  return ret;
}

/**
 * Verifying the package and name given as input .
 *
 * @return 1 if the input is ok, 0 if not
 */
static int probe_id_size_ok(const char* fn, const char* package, const char* name)
{
  if (package == NULL || name == NULL)
  {
    IDD_LOGE("[%s] Null in probe id - dropping. [%s:%s]",
              fn, package ? package : "null", name ? name : "null");
    return 0;
  }

  if ((strlen(package) + strlen(name)) > IDD_MAX_PROBE_ID_SIZE)
  {
    IDD_LOGE("[%s] To large probe id, %zu > %d - dropping. [%s:%s]",
              fn, strlen(package) + strlen(name), IDD_MAX_PROBE_ID_SIZE, package, name);
    return 0;
  }

  return 1;
}

/**
 * Verifying the package, name and value given as input .
 *
 * @return 1 if the input is ok, 0 if not
 */
static int probe_input_ok(const char* fn, const char* package, const char* name, const char* value)
{
  if (!probe_id_size_ok(fn, package, name))
  {
    return 0;
  }

  if ((value == NULL) || (strlen(value) == 0))
  {
    IDD_LOGE("[%s] Null in probe value - dropping. [%s:%s]", fn, package, name);
    return 0;
  }

  return 1;
}

/**
 * Handler for all incoming commands to do something.
 * Commands:
 * Events:       EV<space>package<space>name<space>binary
 * Plugin events:EP<space>plugin-id<space>package<space>name<space>binary
 * App events:   AV<space>app_package<space>app_version_name<space>app_version_code\
 *               <space>timestamp<space>payload_type<space>binary
 * Register App Data JSON: RA<space>app_package<space>class name<space>1/0
 * Get Registered packeges: RP -> registered_packages(str)
 * Counter inc:  CI<space>package<space>name<space>value(str)
 * Counter set:  CS<space>package<space>name<space>value(str)
 * Static data:  SD<space>package<space>name<space>value(str)
 * Flush:        FL
 * Set IMEI:     IM<space>IMEI(str)
 * Set ANDROID_ID of IDD agent:               AI<space>ANDROID_ID(str)
 * Set ANDROID_ID of Xperia services:         XA<space>ANDROID_ID(str)
 * Set GUID of Xperia services:               XG<space>GUID(str)
 * Set user consent:   UC<space>value(interger)
 * Notify Conf:  NC
 * Probe Active? PA<space>package<space>name -> "1" or "0"
 * Write Config: WC<space>binary
 * Get Config?   GC<space>package<space>name -> file descriptor (-1 on error)
 * IDD Active?   IA -> "1"
 * Force Save:   FS
 * Enable/disable data collection: DC<space>value(integer 1 or 0)
 * Delete report:      RM<space>value(integer, 0 is remove all, positive is specified report)
 * Get DeviceId?       GD -> device id(str)
 * Set GroupId:        SG<space>groupid(str)
 * Delete GroupId:     DG<space>groupid(str)
 * Replace GroupId:    RG<space>togroupid(str)<space>fromgroupid(str)
 * File iterator next: FN<space>client_id<space>filter_iterator
 * File get:           FG<space>client_id<space>filter_iterator
 * Config request Get: CR
 * Config notification:CN
 * Do Data Washing:    DW
 * Set timezone:       TZ
 *
 * @param sock
 * @param buf_p in: command received from the client, out: result value if returned value > 0
 * @param size
 * @param msg From recvmsg(.., msg) if SOCK_CL, NULL otherwise
 *
 * @return len of result (buf_p) or 0 if to be ignored by caller
 */
static size_t handle_data(int sock, char* buf_p, size_t size, struct msghdr* msg)
{
  static const char* delimiters = " \t\r\n";
  char* save_p = NULL;
  const char* command_p;

  command_p = tokenize_r(buf_p, delimiters, &save_p);

  if (command_p == NULL)
  {
    IDD_LOGE("Invalid command: none - dropping.");
    return 0;
  }

  if (command_p[0] == 'E' && (command_p[1] == 'V' || command_p[1] == 'P') && command_p[2] == '\0')
  {
    // EV idd_add_event, EP idd_add_plugin_event
    int plugin_id = ALL_PLUGINS;
    if (command_p[1] == 'P')
    {
      const char* plugin_p = tokenize_r(NULL, delimiters, &save_p);
      // If plugin_p == NULL there will be no package_p either and command will be rejected.
      plugin_id = (plugin_p ? atoi(plugin_p) : ALL_PLUGINS);
    }

    const char* package_p = tokenize_r(NULL, delimiters, &save_p);
    const char* name_p    = tokenize_r(NULL, delimiters, &save_p);
    /*
     * First check if it's worth while to continue.
     */
    if (!is_data_collection_enabled() && !plugins_filter_is_probe_active(package_p, name_p))
    {
      return 0;
    }

    if (probe_id_size_ok(command_p, package_p, name_p))
    {
      const char* payload_p = name_p + strlen(name_p) + 1;
      size_t payload_size = size - (payload_p - buf_p);
      if (payload_p > (buf_p + size))
      {
        payload_size = 0;
      }
      if ((plugin_id == ALL_PLUGINS) || (plugin_id == (int)IDDD__PLUGIN_ID__IDD))
      {
        handle_event(g_filter, package_p, name_p, payload_p, payload_size);
      }
      if (plugin_id != (int)IDDD__PLUGIN_ID__IDD)
      {
        plugins_handle_event(plugin_id, package_p, name_p, payload_size, payload_p);
      }

    }
  }
  else if (command_p[0] == 'A' && command_p[1] == 'V' && command_p[2] == '\0')
  {
    // AV idd_add_app_event
    const char* app_package_p = tokenize_r(NULL, delimiters, &save_p);
    const char* app_version_name_p = tokenize_r(NULL, delimiters, &save_p);
    const char* app_version_code_p = tokenize_r(NULL, delimiters, &save_p);
    const char* timestamp_p = tokenize_r(NULL, delimiters, &save_p);
    const char* payload_type_p = tokenize_r(NULL, delimiters, &save_p);
    if (app_package_p == NULL || app_version_name_p == NULL || app_version_code_p == NULL ||
        timestamp_p == NULL || payload_type_p == NULL)
    {
      IDD_LOGE("Invalid field(s) of app event - dropping.");
      return 0;
    }
    int app_version_code = atoi(app_version_code_p);
    int payload_type = atoi(payload_type_p);
    /*
     * First check if it's worth while to continue.
     */
    if (!is_data_collection_enabled())
    {
      return 0;
    }

    if (probe_id_size_ok(command_p, PACKAGE_PROBE_ANDROID_IDD_VIRTUAL_APP_EVENT, app_package_p))
    {
      const char* payload_p = payload_type_p + strlen(payload_type_p) + 1;
      size_t payload_size = size - (payload_p - buf_p);
      if (payload_p > (buf_p + size))
      {
        payload_size = 0;
      }

      handle_app_event(g_filter, app_package_p, app_version_name_p, app_version_code, timestamp_p,
                       payload_type, payload_p, payload_size);
    }
  }
  else if (command_p[0] == 'R' && command_p[1] == 'A' && command_p[2] == '\0')
  {
    // RA idd_register_to_use_add_app_event
    const char* package_p = tokenize_r(NULL, delimiters, &save_p);
    const char* class_p = tokenize_r(NULL, delimiters, &save_p);
    if (package_p != NULL && class_p != NULL)
    {
      int active = filter_is_probe_active(g_filter + EVENT,
          PACKAGE_PROBE_ANDROID_IDD_VIRTUAL_APP_EVENT, package_p, CHECK_IDD_ENABLED);
      int ret = app_event_packages_register(package_p, class_p, active);
      if (!ret)
      {
        IDD_LOGE("register_to_use_add_app_event (RA) failed");
        strncpy(buf_p, "-1", 2);
        return strlen(buf_p);
      }
      strncpy(buf_p, active ? "1" : "0", 2);
    }
    return strlen(buf_p);
  }
  else if (command_p[0] == 'R' && command_p[1] == 'P' && command_p[2] == '\0')
  {
    // RP idd_get_app_event_enabled_packages
    char updated[MAX_APP_EVENT_ENABLED_PACKAGES_SIZE] = {'\0'};
    app_event_packages_update(updated, sizeof(updated));
    strncpy(buf_p, updated, sizeof(updated));
    return strlen(buf_p);
  }
  else if ((command_p[0] == 'C' && command_p[1] == 'I' && command_p[2] == '\0') ||
           (command_p[0] == 'C' && command_p[1] == 'S' && command_p[2] == '\0'))
  {
    // CI idd_inc_counter, CS idd_set_counter
    const char* package_p = tokenize_r(NULL, delimiters, &save_p);
    const char* name_p    = tokenize_r(NULL, delimiters, &save_p);
    const char* value_p   = tokenize_r(NULL, delimiters, &save_p);
    if (probe_input_ok(command_p[1] == 'S' ? "CS" : "CI", package_p, name_p, value_p))
    {
      if (command_p[1] == 'S')
      {
        counters_handle(g_filter, package_p, name_p, atoi(value_p), 0);
      }
      else
      {
        counters_handle(g_filter, package_p, name_p, atoi(value_p), 1);
      }
    }
  }
  else if ((command_p[0] == 'S' && command_p[1] == 'D' && command_p[2] == '\0'))
  {
    // SD idd_set_sdata
    const char* package_p = tokenize_r(NULL, delimiters, &save_p);
    const char* name_p    = tokenize_r(NULL, delimiters, &save_p);
    const char* value_p   = save_p;
    if (probe_input_ok("SD", package_p, name_p, value_p))
    {
      sdata_handle(g_filter, package_p, name_p, value_p);
    }
  }
  else if ((command_p[0] == 'F') && (command_p[1] == 'L') && command_p[2] == '\0')
  {
    // FL idd_flush
    IDD_LOGD("Flush request");

    /*
     * Don't flush if we should not collection data.
     */
    if (is_data_collection_enabled() == 0)
    {
      return 0;
    }

    (void)generate_report();
    /* Mark the current time zone.*/
    create_report_header();
  }
  else if ((command_p[0] == 'N') && (command_p[1] == 'C') && command_p[2] == '\0')
  {
    // NC idd_notify_config
    strncpy(buf_p, load_new_configuration(0) ? "1" : "0", 2);
    IDD_LOGD("load new configuration => %s", buf_p);
    create_report_header();
    return strlen(buf_p);
  }
  else if ((command_p[0] == 'I') && (command_p[1] == 'M') && command_p[2] == '\0')
  {
    // IM idd_set_imei
    const char* imei_p = tokenize_r(NULL, delimiters, &save_p);
    if (imei_p == NULL)
    {
      IDD_LOGE("set_imei without value.");
    }
    else
    {
      IDD_LOGD("Set IMEI <%.*s>", IDD_MAX_DEVICE_IMEI_SIZE, imei_p);
      imei_handle(imei_p);
    }
  }
  else if ((command_p[0] == 'A') && (command_p[1] == 'I') && command_p[2] == '\0')
  {
    // AI idd_set_android_id
    const char* android_id_p = tokenize_r(NULL, delimiters, &save_p);
    if (android_id_p == NULL)
    {
      IDD_LOGE("set_android_id without value.");
    }
    else
    {
      IDD_LOGD("Set ANDROID_ID <%.*s>", IDD_MAX_ANDROID_ID_SIZE, android_id_p);
      android_id_handle(android_id_p);
    }
  }
  else if ((command_p[0] == 'X') && (command_p[1] == 'A') && command_p[2] == '\0')
  {
    // XA idd_set_xs_android_id
    const char* xs_android_id_p = tokenize_r(NULL, delimiters, &save_p);
    if (xs_android_id_p == NULL)
    {
      IDD_LOGE("set_xs_android_id without value.");
    }
    else
    {
      IDD_LOGD("Set Xperia services' ANDROID_ID <%.*s>", IDD_MAX_XS_ANDROID_ID_SIZE,
               xs_android_id_p);
      xs_android_id_handle(xs_android_id_p);
    }
  }
  else if ((command_p[0] == 'X') && (command_p[1] == 'G') && command_p[2] == '\0')
  {
    // XG idd_set_xs_guid
    const char* xs_guid_p = tokenize_r(NULL, delimiters, &save_p);
    if (xs_guid_p == NULL)
    {
      IDD_LOGE("set_xs_guid without value.");
    }
    else
    {
      IDD_LOGD("Set Xperia services' GUID <%.*s>", IDD_MAX_XS_GUID_SIZE, xs_guid_p);
      xs_guid_handle(xs_guid_p);
    }
  }
  else if ((command_p[0] == 'U') && (command_p[1] == 'C') && command_p[2] == '\0')
  {
    // UC idd_set_user_consent
    const char* value_p = tokenize_r(NULL, delimiters, &save_p);
    if (value_p != NULL)
    {
      int user_consent = atoi(value_p);
      user_consent_handle(user_consent);
    }
    else
    {
      IDD_LOGE("user_consent without value.");
    }
  }
  else if ((command_p[0] == 'I') && (command_p[1] == 'A') && command_p[2] == '\0')
  {
    // IA idd_is_active
    IDD_LOGD("Is IDD active? Yes, of course");
    strncpy(buf_p, "1", 2);
    return strlen(buf_p);
  }
  else if ((command_p[0] == 'P') && (command_p[1] == 'A') && command_p[2] == '\0')
  {
    // PA idd_is_probe_active
    const char* package_p = tokenize_r(NULL, delimiters, &save_p);
    const char* name_p    = tokenize_r(NULL, delimiters, &save_p);
    if (probe_id_size_ok("PA", package_p, name_p))
    {
      int active = filter_is_probe_active(g_filter + EVENT, package_p, name_p, CHECK_IDD_ENABLED) ||
                   plugins_filter_is_probe_active(package_p, name_p);
      strncpy(buf_p, active ? "1" : "0", 2);
      IDD_LOGD("Is filter active (%s, %s) => %s", package_p, name_p, buf_p);
      return strlen(buf_p);
    }
  }
  else if ((command_p[0] == 'W') && (command_p[1] == 'C') && command_p[2] == '\0')
  {
    // WC idd_write_config
    const char* config_p = buf_p + strlen(command_p) + 1;
    size_t config_size = size - (config_p - buf_p);
    write_config(config_p, config_size);
  }
  else if ((command_p[0] == 'G') && (command_p[1] == 'C') && command_p[2] == '\0')
  {
    // GC idd_get_config
    const char* package_p = tokenize_r(NULL, delimiters, &save_p);
    const char* name_p    = tokenize_r(NULL, delimiters, &save_p);
    if (probe_id_size_ok("GC", package_p, name_p))
    {
      if (sock != SOCK_CL)
      {
        FILE* fp = config_open_file(package_p, name_p);
        if (fp != NULL)
        {
          IDD_LOGD("Get config file '%s'::'%s' -> %d", package_p, name_p, fileno(fp));
          send_fd_msg(sock, fileno(fp));
          fclose(fp);
          return 0;
        }
        else
        {
          strncpy(buf_p, DAEMON_ERROR_STR, sizeof(DAEMON_ERROR_STR));
          return strlen(buf_p);
        }
      }
    }
  }
  else if ((command_p[0] == 'G') && (command_p[1] == 'C') && (command_p[2] == 'P') && command_p[3] == '\0')
  {
    // GCP idd_get_config_path
    const char* package_p = tokenize_r(NULL, delimiters, &save_p);
    const char* name_p    = tokenize_r(NULL, delimiters, &save_p);
    if (probe_id_size_ok("GCP", package_p, name_p))
    {
      char path[PATH_MAX] = {0};
      if (get_config_file_path(package_p, name_p, path, sizeof(path)) <= 0) {
        IDD_LOGD("Get config file path error '%s'::'%s' -> %s", package_p, name_p, path);
        strncpy(buf_p, "-1", 3);
        return strlen(buf_p);
      }
      IDD_LOGD("Get config file path'%s'::'%s' -> %s", package_p, name_p, path);
      strncpy(buf_p, path, sizeof(path));
      return strlen(buf_p);
    }
  }
  else if ((command_p[0] == 'G') && (command_p[1] == 'R') && (command_p[2] == 'S') && command_p[3] == '\0')
  {
    // GRS idd_get_report_stats
    int oldest_file_nr = 0;
    int newest_file_nr = 0;
    int total_file_nr = 0;
    size_t sum_size = 0;
    bool ret = get_idd_report_stats(&oldest_file_nr, &newest_file_nr, &total_file_nr, &sum_size);
    if (ret) {
      snprintf(buf_p, IDD_MAX_PACKET_SIZE, "%d,%d,%d,%d", oldest_file_nr, newest_file_nr, total_file_nr, sum_size);
      IDD_LOGD("%s", buf_p);
      return strlen(buf_p);
    } else {
      snprintf(buf_p, IDD_MAX_PACKET_SIZE, "-1");
      IDD_LOGD("%s", buf_p);
      return strlen(buf_p);
    }
  }
  else if ((command_p[0] == 'F') && (command_p[1] == 'S') && command_p[2] == '\0')
  {
    // FS idd_force_save
    counters_save();
    sdata_save();
    save_time_update();
  }
  else if ((command_p[0] == 'S') && (command_p[1] == 'G') && command_p[2] == '\0')
  {
    // SG idd_set_group_id
    const char* group_id_str_p = tokenize_r(NULL, delimiters, &save_p);
    if (group_id_str_p == NULL)
    {
      IDD_LOGE("set_group_id without value.");
    }
    else
    {
      group_id_set_handle(group_id_str_p, 1);
    }
  }
  else if ((command_p[0] == 'D') && (command_p[1] == 'G') && command_p[2] == '\0')
  {
    // DG idd_delete_group_id
    const char* del_group_id_str_p = tokenize_r(NULL, delimiters, &save_p);
    if (del_group_id_str_p == NULL)
    {
      IDD_LOGE("delete_group_id without value.");
    }
    else
    {
      group_id_delete_handle(del_group_id_str_p, 1);
    }
  }
  else if ((command_p[0] == 'R') && (command_p[1] == 'G') && command_p[2] == '\0')
  {
    // RG idd_replace_group_id
    const char* to_group_id_str_p = tokenize_r(NULL, delimiters, &save_p);
    const char* from_group_id_str_p = tokenize_r(NULL, delimiters, &save_p);
    if (to_group_id_str_p == NULL || from_group_id_str_p == NULL)
    {
      IDD_LOGE("replace_group_id without valid value.");
    }
    else
    {
      group_id_replace_handle(to_group_id_str_p, from_group_id_str_p, 1);
    }
  }
  else if ((command_p[0] == 'D') && (command_p[1] == 'C') && command_p[2] == '\0')
  {
    // DC idd_set_data_collection_enable
    const char* value_p   = tokenize_r(NULL, delimiters, &save_p);
    if (value_p != NULL)
    {
      int enabled = atoi(value_p);
      set_data_collection_enabled(enabled == 0 ? 0 : 1);
      state_save();
      IDD_LOGD("Change data collection to: %s", enabled == 0 ? "disabled" : "enabled");
    }
    else
    {
      IDD_LOGE("Enable/disable data collection without value.");
    }
  }
  else if ((command_p[0] == 'R') && (command_p[1] == 'M') && command_p[2] == '\0')
  {
    // RM idd_remove_report
    const char* value_p   = tokenize_r(NULL, delimiters, &save_p);
    if (value_p != NULL)
    {
      int report_num = atoi(value_p);
      remove_report(report_num);
    }
    else
    {
      IDD_LOGE("Remove report without number.");
    }
  }
  else if ((command_p[0] == 'G') && (command_p[1] == 'D') && command_p[2] == '\0')
  {
    // GD idd_get_device_id
    char device_id[IDD_MAX_DEVICE_ID_SIZE] = {0};
    get_device_id(device_id);
    IDD_LOGD("Get device id -> %s", device_id);
    strncpy(buf_p, device_id, IDD_MAX_DEVICE_ID_SIZE);
    return strlen(buf_p);
  }
  else if ((command_p[0] == 'C') && (command_p[1] == 'R') && command_p[2] == '\0')
  {
    // CR idd_config_request_get
    int size = CONFIG_REQ_ERROR;
    size = config_request_get(buf_p, IDD_MAX_EVENT_SIZE);
    IDD_LOGD("Get config request -> size = %d", size);
    if (size == CONFIG_REQ_ERROR)
    {
      size = sizeof(DAEMON_ERROR_STR);
      strncpy(buf_p, DAEMON_ERROR_STR, size);
      IDD_LOGE("getConfigRequest (CR) failed");
    }
    return size;
  }
  else if ((command_p[0] == 'C') && (command_p[1] == 'N') && command_p[2] == '\0')
  {
    // CN idd_get_config_notify
    int fd = config_change_notify_get();
    int size = 0;
    if (fd != -1)
    {
      send_fd_msg(sock, fd);
      close(fd);
    }
    else
    {
      IDD_LOGE("config_change_notify_get failed: %s", strerror(errno));
      size = sizeof(DAEMON_ERROR_STR);
      strncpy(buf_p, DAEMON_ERROR_STR, size);
    }
    return size;
  }
  else if ((command_p[0] == 'D') && (command_p[1] == 'W') && command_p[2] == '\0')
  {
    // DW idd_do_washing
    strncpy(buf_p, do_washing() ? "1" : "0", 2);
    IDD_LOGD("Do washing => %s", buf_p);
    return strlen(buf_p);
  }
  else if ((command_p[0] == 'T') && (command_p[1] == 'Z') && command_p[2] == '\0')
  {
    // TZ idd_set_timezone
    timezone_handle();
  }
  else
  {
    IDD_LOGE("Bad command: %d/%d - dropping", buf_p[0], buf_p[1]);
  }
  return 0;
}

/**
 * Set send and recieve timeouts on the given socket.
 *
 * @param sock Socket (fd)
 */
static void set_socket_timeouts(int sock)
{
  static struct timeval tv = {
    tv_sec: IDD_SOCKET_TIMEOUT,
    tv_usec: 0
  };
  // Log failure but do not propagate to caller
  if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) == -1 ||
      setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv)) == -1)
  {
    IDD_LOGE("%s: failed to set socket timeouts, errno=%d (%s)", __func__, errno, strerror(errno));
  }
}

/**
 * Socket callback for incoming connection-less (SOCK_DGRAM) unix
 * domain socket requests. Passes along the command to handle_data().
 *
 * @param sock_p
 */
static void receive_dgram(idd_sock_t* sock_p)
{
  char buffer[IDD_MAX_PACKET_SIZE + 1] = {0};
  char control[1024] = {0};
  struct msghdr msg;
  struct iovec iov;
  memset(&msg, 0, sizeof(msg));
  iov.iov_base = buffer;
  iov.iov_len = IDD_MAX_PACKET_SIZE;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = control;
  msg.msg_controllen = sizeof(control);

  // We have already set receive timeout. Ignore dgram if we timeout.
  ssize_t rlen = RETRY_IF_EINTR(recvmsg(sock_p->fd, &msg, 0));
  if (rlen > 0)
  {
    if (rlen <= IDD_MAX_PACKET_SIZE)
    {
      buffer[rlen] = 0;
      handle_data(SOCK_CL, buffer, (size_t)rlen, &msg);
    }
    else
    {
      IDD_LOGE("Too large packet (dgram) from client %d, dropping.", sock_p->fd);
    }
  }
}

/**
 * Handle incoming commands on a connection-oriented (SOCK_SEQPACKET)
 * unix domain socket. Passes along the command to handle_data().
 *
 * @param sock_p
 */
static void receive_seq_client(idd_sock_t* sock_p)
{
  char buffer[IDD_MAX_PACKET_SIZE + 1] = {0};
  struct sockaddr_un from;
  socklen_t fromlen = sizeof(from);
  int sock = sock_p->fd;

  // We have already set receive timeout. Ignore seqpacket if we timeout.
  ssize_t rlen = RETRY_IF_EINTR(recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&from, &fromlen));
  if (rlen < 0)
  {
    // Timeout, closed by peer, not connected etc errors.
    IDD_LOGE("recvfrom() error from client %d, %s", sock, strerror(errno));
  }
  else if (rlen == 0)
  {
    // No data from peer and orderly shutdown.
    IDD_LOGD("Client %d disconnected", sock);
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock, NULL);
    close(sock);
    free(sock_p);
  }
  else if (rlen <= IDD_MAX_PACKET_SIZE)
  {
    buffer[rlen] = 0;
    size_t res = handle_data(sock, buffer, (size_t)rlen, NULL);
    if (res > 0)
    {
      if (socket_send(sock, buffer, res) == -1)
      {
        IDD_LOGE("%s: failed to send to client %d, errno=%d (%s)", __func__, sock, errno, strerror(errno));
      }
    }
  }
  else
  {
    IDD_LOGE("Too large packet (seq) from client %d, dropping.", sock);
  }
}

/**
 * Socket callback for incoming connection-oriented (SOCK_SEQPACKET)
 * unix domain socket requests. Calls accept() and pass further
 * handling on to receive_seq_client().
 *
 * @param sock_p
 */
static void receive_seq_listen(idd_sock_t* sock_p)
{
  struct sockaddr_in clientaddr;
  socklen_t clientaddr_size = sizeof(clientaddr);

  int client_sock = accept(sock_p->fd, (struct sockaddr*)&clientaddr, &clientaddr_size);
  if (client_sock < 0)
  {
    IDD_LOGE("Accept failed: %s.", strerror(errno));
    return;
  }

  // Add this socket to the set.
  idd_sock_t* client_p = malloc(sizeof(*client_p));
  if (client_p == NULL)
  {
    close(client_sock);
  }
  else
  {
    struct epoll_event ev;
    ev.data.u64 = 0;  //To make valgrind happy (Syscall param epoll_ctl(event) points to uninitialised byte(s))
    set_socket_timeouts(client_sock);
    client_p->fd = client_sock;
    client_p->on_receive = receive_seq_client;
    ev.events = EPOLLIN;
    ev.data.ptr = client_p;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sock, &ev);
  }
}

/**
 * Handle incoming commands on a connection-oriented (SOCK_STREAM)
 * unix domain socket. Firstly parse header to mark the size.
 * Passes along the command to handle_data().
 *
 * @param sock_p
 */
static void receive_stream_client(idd_sock_t* sock_p)
{
  int sock = sock_p->fd;
  idd_sock_bucket_t *sock_bucket_p = find_sock_bucket(sock);
  ssize_t rlen;

  if (sock_bucket_p == NULL)
  {
    // The first packet came in, having size of stream data.
    char size_p[IDDD_SIZE_T_DIGIT_NUMBER + 1] = {0};
    rlen = RETRY_IF_EINTR(read(sock, size_p, sizeof(size_p)));
    if (rlen < 0)
    {
      // Timeout, closed by peer, not connected etc errors.
      IDD_LOGE("read() error (size of stream data) from client %d, %s", sock, strerror(errno));
    }
    else if (rlen == 0)
    {
      // No data from peer and orderly shutdown.
      IDD_LOGD("Client (stream) %d disconnected", sock);
      epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock, NULL);
      close(sock);
      free(sock_p);
    }
    else
    {
      size_p[rlen] = 0;
      size_t size = atoi(size_p);
      if (size > 0) {
        iddd_ret_val_t result = alloc_sock_bucket(sock,size);
        if (RETRY_IF_EINTR(write(sock, &result, sizeof(result))) == -1)
        {
          IDD_LOGE("%s: failed to reply to client %d, errno=%d (%s)", __func__, sock, errno,
                   strerror(errno));
        }
      }
      else
      {
        IDD_LOGE("error (size of stream data) from client %u", size);
      }
    }
  }
  else
  {
    // Data reception proceeds.
    rlen = RETRY_IF_EINTR(read(sock, sock_bucket_p->tail, IDD_MAX_PACKET_SIZE));
    if (rlen < 0)
    {
      // Timeout, closed by peer, not connected etc errors.
      IDD_LOGE("read() error (intermediate stream data) from client %d, %s", sock, strerror(errno));
    }
    else if (rlen == 0)
    {
      // No data from peer and orderly shutdown.
      IDD_LOGD("Client (stream) %d disconnected", sock);
      free_sock_bucket(sock_bucket_p);
      epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock, NULL);
      close(sock);
      free(sock_p);
    }
    else if ((sock_bucket_p->tail + rlen) < sock_bucket_p->limit)
    {
      // Intermidiate data from peer.
      sock_bucket_p->tail += rlen;
    }
    else if ((sock_bucket_p->tail + rlen) == sock_bucket_p->limit)
    {
      // All of data arrived then its data in bucket will be handled.
      sock_bucket_p->tail += rlen;
      *(sock_bucket_p->tail) = 0;
      size_t slen = handle_data(sock_bucket_p->fd, sock_bucket_p->head,
                                (size_t)(sock_bucket_p->tail - sock_bucket_p->head), NULL);
      if (slen > 0)
      {
        if (RETRY_IF_EINTR(write(sock, sock_bucket_p->head, slen)) == -1)
        {
          IDD_LOGE("%s: failed to send back data to client %d, errno=%d (%s)", __func__, sock,
                   errno, strerror(errno));
        }
      }
    }
    else
    {
      // Too large packet (stream) from peer.
      IDD_LOGE("Too large packet (stream) %zu from client %d, disconnected", rlen, sock);
    }
  }
}

/**
 * Socket callback for incoming connection-oriented (SOCK_STREAM)
 * unix domain socket requests. Calls accept() and pass further
 * handling on to receive_stream_client().
 *
 * @param sock_p
 */
static void receive_stream_listen(idd_sock_t* sock_p)
{
  struct sockaddr_in clientaddr;
  socklen_t clientaddr_size = sizeof(clientaddr);

  int client_sock = accept(sock_p->fd, (struct sockaddr*)&clientaddr, &clientaddr_size);
  if (client_sock < 0)
  {
    IDD_LOGE("Accept failed: %s.", strerror(errno));
    return;
  }

  // Add this socket to the set.
  idd_sock_t* client_p = malloc(sizeof(*client_p));
  if (client_p == NULL)
  {
    close(client_sock);
  }
  else
  {
    struct epoll_event ev;
    ev.data.u64 = 0;  //To make valgrind happy (Syscall param epoll_ctl(event) points to uninitialised byte(s))
    set_socket_timeouts(client_sock);
    client_p->fd = client_sock;
    client_p->on_receive = receive_stream_client;
    ev.events = EPOLLIN;
    ev.data.ptr = client_p;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sock, &ev);
  }
}

/**
 * Create all directories used by the daemon. Exit if failure.
 *
 */
void create_dirs(void)
{
  if (mkdir(PRIVATE_DIR_NAME, PRIVATE_DIR_MODE) == -1 &&
      errno != EEXIST)
  {
    IDD_LOGE("Failed to create private directory: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }
  if (mkdir(PRIVATE_CONFIGS_BACKUP_DIR_NAME, PRIVATE_DIR_MODE) == -1 &&
      errno != EEXIST)
  {
    IDD_LOGE("Failed to create private directory: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }
  if (mkdir(OUTPUT_DIR_NAME, OUTPUT_DIR_MODE) == -1 &&
      errno != EEXIST)
  {
    IDD_LOGE("Failed to create output directory: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }
  if (mkdir(INPUT_DIR_NAME, INPUT_DIR_MODE) == -1 &&
      errno != EEXIST)
  {
    IDD_LOGE("Failed to create input directory: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

/**
 * Signal handler for SIGINT and SIGTERM. Call set_should_exit() to do
 * a nice shutdown in main().
 *
 * @param nr
 */
static void sigh_exit(int nr)
{
  (void)nr;
  set_should_exit();
}

static void sigh_pipe()
{
  sigpipe = 1;
}

/**
 * Shutdown everything.
 *
 */
static void free_memory()
{
  app_event_packages_shutdown();
  counters_shutdown();
  sdata_shutdown();
  util_shutdown();
  filter_shutdown(g_filter);
  config_shutdown();
  free((void*)base_dir_p);
  base_dir_p = NULL;
}

/**
 * Parse command line arguments.
 *
 * @param argc
 * @param argv
 */
static void parse_args(int argc, char* argv[])
{
  static const struct option long_options[] =
    {
      {"verbose",        no_argument,       0, 'v'},
      {"help",           no_argument,       0, 'h'},
      {"no-detach",      no_argument,       0, 'D'},
      {0, 0, 0, 0}
    };
  int option_index = 0;
  int c;
  int help_mode = 0;

  // Set initial values for some options we might change below
  base_dir_p = strdup(IDD_DEFAULT_BASE_DIR);

  while ((c = getopt_long (argc, argv, "vhD", long_options, &option_index)) != -1)
  {
    switch(c)
    {
    case 'D':
      detach = 0;
      break;
    case 'v':
      set_verbose(1);
      break;
    case 'h':
      help_mode = 1;
      break;
    }
  }

  if (help_mode)
  {
    printf("Syntax: iddd [OPTIONS]\n"
           "\n"
           "  -h, --help             give this help list\n"
           "  -v, --verbose          emit debugging messages\n"
           "  -D, --no-detach        don't detach from tty\n"
           "\n");
    exit(2);
  }
}

/**
 * Create a file system socket of the given type. Exit if failure.
 *
 * @param type type of socket
 * @param path file system path
 *
 * @return socket
 */
static int create_uds(int type, const char* path)
{
  int sock;
  struct sockaddr_un servaddr;

  if ((sock = socket(AF_UNIX, type, 0)) < 0)
  {
    IDD_LOGE("Failed to create AF_UNIX socket: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sun_family = AF_UNIX;
  strncpy(servaddr.sun_path, path, sizeof(servaddr.sun_path));
  servaddr.sun_path[sizeof(servaddr.sun_path) - 1] = '\0';

  // If the socket already exists, we can't bind to it, so it has to be
  // removed. This is due to a non-clean previous shutdown
  if (unlink(servaddr.sun_path) != 0 && errno != ENOENT)
  {
    IDD_LOGE("Failed to unlink socket: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (bind(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
  {
    close(sock);
    IDD_LOGE("Failed to bind to socket: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  chmod(path, SOCKET_MODE);

  IDD_LOGD("Listening to socket %s", servaddr.sun_path);
  return sock;
}

/**
 * Create all sockets and set up socket callback functions. Exit the
 * daemon if failure.
 *
 */
static void sock_init(void)
{
  struct epoll_event ev;
  ev.data.u64 = 0; //To make valgrind happy (Syscall param epoll_ctl(event) points to uninitialised byte(s))
  int on = 1;

  epoll_fd = epoll_create(10);
  if (epoll_fd == -1)
  {
    IDD_LOGE("epoll_create failed: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  sock_cl.fd = create_uds(SOCK_DGRAM, IDD_CL_SOCK_FNAME);
  set_socket_timeouts(sock_cl.fd);
  ev.events = EPOLLIN;
  ev.data.ptr = &sock_cl;
  setsockopt(sock_cl.fd, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on));
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_cl.fd, &ev);

  sock_co.fd = create_uds(SOCK_SEQPACKET, IDD_CO_SOCK_FNAME);
  if (listen(sock_co.fd, 5) == -1)
  {
    IDD_LOGE("Failed to listen to uds-socket: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }
  ev.data.ptr = &sock_co;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_co.fd, &ev);

  sock_co_stream.fd = create_uds(SOCK_STREAM, IDD_CO_STREAM_SOCK_FNAME);
  if (listen(sock_co_stream.fd, 5) == -1)
  {
    IDD_LOGE("Failed to listen to uds-socket: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }
  ev.data.ptr = &sock_co_stream;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_co_stream.fd, &ev);
}

/**
 * If we should detach and are not already detached fork a child
 * process and exit the parent process. Create a new session setting
 * the child process as 'process group leader' with no
 * terminal. Verify that 'idd base directory' exists and lock the lock file
 * (iddd.pid). If this fails another instance of the daemon is
 * already executing, if so exit. Finally set up signal handlers.
 *
 */
static void daemonize(void)
{
  char buf[20] = {0};
  struct sigaction act;
  int pid_fd;

  if (detach && (getppid() != 1))
  {
    pid_t res;
    int fd, stdout_fd = -1, stderr_fd = -1, i;

    res = fork();
    if (res == -1)
    {
      exit(EXIT_FAILURE);
    }
    if (res > 0)
    {
      exit(EXIT_SUCCESS); // Parent process.
    }

    if (setsid() == -1)
    {
      exit(EXIT_FAILURE);
    }

    for (i = IDDD_MAX_OPEN_FD; i >= 0; --i)
    {
      close(i);
    }

    if ((fd = open("/dev/null",O_RDWR)) != STDIN_FILENO)
    {
      exit(EXIT_FAILURE);
    }

    stdout_fd = dup(fd);
    if (stdout_fd != STDOUT_FILENO)
    {
      close(fd);
      if (stdout_fd != -1)
      {
        close(stdout_fd);
      }
      exit(EXIT_FAILURE);
    }
    stderr_fd = dup(fd);
    if (stderr_fd != STDERR_FILENO)
    {
      close(fd);
      close(stdout_fd);
      if (stderr_fd != -1)
      {
        close(stderr_fd);
      }
      exit(EXIT_FAILURE);
    }
    close(fd);
    close(stdout_fd);
    close(stderr_fd);

  }
  else
  {
    puts("Will not detach (or already detached)");
  }

  if (chdir(base_dir_p) == -1)
  {
    IDD_LOGE("Could not chdir(%s), and errno is %s", base_dir_p, strerror(errno));
    exit(EXIT_FAILURE);
  }

  pid_fd = open(PID_FILE, O_RDWR | O_CREAT, 0644);
  if (pid_fd == -1)
  {
    IDD_LOGE("Failed to open lock file %s, and errno is %s", PID_FILE, strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (flock(pid_fd, LOCK_EX | LOCK_NB) == -1)
  {
    IDD_LOGE("Another iddd is already running, and errno is %s", strerror(errno));
    close(pid_fd);
    exit(EXIT_FAILURE);
  }

  snprintf(buf, sizeof(buf), "%d\n", getpid());
  if (write_all(pid_fd, buf, strlen(buf)) == -1)
  {
    close(pid_fd);
    exit(EXIT_FAILURE);
  }

  close(pid_fd);
  memset (&act, '\0', sizeof(act));

  act.sa_handler = sigh_exit;

  if (sigaction(SIGINT, &act, NULL) < 0)
  {
    IDD_LOGE("Failed to install SIGINT handler: %s", strerror(errno));
  }

  if (sigaction(SIGTERM, &act, NULL) < 0)
  {
    IDD_LOGE("Failed to install SIGTERM handler: %s", strerror(errno));
  }

  act.sa_handler = SIG_IGN;
  if (sigaction(SIGCHLD, &act, NULL) < 0)
  {
    IDD_LOGE("Failed to install SIGCHLD handler: %s", strerror(errno));
  }

  act.sa_handler = sigh_pipe;
  if (sigaction(SIGPIPE, &act, NULL) < 0)
  {
    IDD_LOGE("Failed to install SIGPIPE handler: %s", strerror(errno));
  }
}

static void handle_build_type_changed() {
  if (!is_build_type_changed()) return;
  load_default_iddd_config(1);
}

/**
 * Call all init related functions including setting up in memory
 * storage, daemonizing, applying the current configuration/settings
 * and start listening for incoming commands from clients. Regularily
 * save counters and static data to file every 10th minute (default
 * value, override with the -s option). Also responsible for all clean
 * up when the daemon is exiting.
 *
 * @param argc
 * @param argv
 *
 * @return
 */
int main(int argc, char* argv[])
{
  // NOTE: We are not allowed to log and then run "daemonize"
  //       since that will close the log socket.
  settings_init();
  crypto_init();
  set_initial_data_collection_enabled();
  parse_args(argc, argv);
  daemonize();
  // If the kill switch is enabled, this function will never return.
  run_kill_switch();
  IDD_LOGI("Starting up...");
  umask(0);
  create_dirs();
  umask(FILE_CREATION_MASK);
  rescue_report(PARTIAL_EVENTS_FNAME, EVENTS_FILE);
  handle_build_type_changed();
  counters_init();
  sdata_init();
  // Header should be initiated before any referred.
  report_header_init();
  //Config file has to be read before report header is created.
  state_load();
  config_init();
  settings_load_configuration();
  plugins_load_configuration();
  prune_data(0);
  generate_device_id(0);
  generate_rca_device_id();
  generate_session_id();
  state_save();

  sock_init();

  int config_loaded = load_new_configuration(0);
  app_event_packages_init();

  handle_prototype_setting();
  create_report_header();

  save_time_update();
  IDD_LOGD("Using base dir '%s'", base_dir_p);
  IDD_LOGD("Saving counters and static data with an interval of %d second(s)",
           IDD_DEFAULT_SAVE_INTERVAL);

  /* Assuming that Idd partition is mounted successfuly and
   * never be unmounted at all
   */
  run_configuration_actions();

  IDD_LOGI("Ready to serve the world.");

  /* Main loop */
  while (!should_exit())
  {
    int result;
    struct epoll_event events[10] = {};

    result = epoll_wait(epoll_fd, events, sizeof(events)/sizeof(events[0]), save_time_left()*1000);

    if (result < 0)
    {
      // Unhandled signals (USR1 for example) will terminate the program right off....
      IDD_LOGD("select() < 0, Probably a handled signal: %s\n", strerror(errno));
    }
    else if (result > 0)
    {
      int i;
      for (i = 0; i < result; i++)
      {
        if (events[i].events & EPOLLIN)
        {
          idd_sock_t* sock_p = events[i].data.ptr;
          sock_p->on_receive(sock_p);
        }
      }
    }
    else
    {
      // Timer expired.
      IDD_LOGD("Save start");
      counters_save();
      sdata_save();
      save_time_update();
      IDD_LOGD("Save end");
    }
  }

  IDD_LOGI("Shutting down...");

  /* Shutting down */
  counters_save();
  sdata_save();
  state_save();

  unlink(PID_FILE);
  unlink(IDD_CL_SOCK_FNAME);
  unlink(IDD_CO_SOCK_FNAME);
  unlink(IDD_CO_STREAM_SOCK_FNAME);
  close(sock_cl.fd);
  sock_cl.fd = -1;
  close(sock_co.fd);
  close(sock_co_stream.fd);

  free_memory();

  IDD_LOGI("Exiting.");
  return EXIT_SUCCESS;
}
