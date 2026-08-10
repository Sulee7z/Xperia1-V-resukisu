/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief C API for accessing the IDD Daemon
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <inttypes.h>
#include <math.h>
#include <zlib.h>
#include "idd_api.h"
#include "idd_private_api.h"
#include "idd_config.h"

// Reuse logging macros from iddd. LOG_TAG must come before the include.
#define LOG_TAG "idd-api"
#include "../../iddd/logging.h"

//The system will use 32 bytes of the message to headers.
#define MESSAGE_HEADER_SIZE 32

#define STR_ERROR_BUF_SIZE 100
//String returned to socket by the IDD daemon (see iddd.c) in case of failure
#define DAEMON_ERROR_STR "FAIL"
#define DAEMON_ERROR(str, str_len) ((str_len == sizeof(DAEMON_ERROR_STR)) && \
                                    !strncmp(str, DAEMON_ERROR_STR, sizeof(DAEMON_ERROR_STR)))

struct idd_instance_t {
  /* Contains the path to the connection-less socket */
  struct sockaddr_un server_addr;
  int server_sock;
};

typedef struct {
    int reply;
    const char* msg;
} idd_sock_ret_t;

static const idd_sock_ret_t idd_sock_ret[] = {
    { IDD_CO_SOCK_SEND_SUCCESS, "SUCCESS"},
    { IDD_CO_SOCK_BUCKET_MALLOC_FAIL, "MALLOC_FAIL"},
    { IDD_CO_SOCK_NO_EMPTY_BUCKET, "NO_EMPTY_BUCKET"},
    { IDD_CO_SOCK_INVALID_HEADER_SIZE, "INVALID_HEADER_SIZE"},
    { IDD_CO_SOCK_UNKNOWN_RET_VAL, "UNKNOWN"}
};

// Timeout in seconds for all socket related functions.
// Set below 10s since we need some margin to prevent ANR from kicking in.
// Test cases relies on this value too.
#define SOCKET_TIMEOUT 3

// Repeat calling 'func' if we got an EINTR signal.
#define RETRY_IF_EINTR(func) ({int res; do res = (func); while (res == -1 && errno == EINTR); res;})


// Max string length for 'int to string'
#define INT_MAX_LEN 21

static int connect_idd(struct idd_instance_t *this_p, const char *sock_fname, const char *func);

/*
 * IDD api helper functions
 */

/**
 * Change the blocking mode for the given socket (fd).
 *
 * @param sock Socket (fd)
 * @param block If 0 set socket to non-blocking mode, else to blocking mode.
 * @return 0 if successful, -1 if not.
 */
static int set_blocking_mode(int sock, int block)
{
  int sock_flags = fcntl(sock, F_GETFL, 0);
  if (sock_flags != -1)
  {
    if (block)
      sock_flags &= ~O_NONBLOCK;
    else
      sock_flags |= O_NONBLOCK;
    sock_flags = fcntl(sock, F_SETFL, sock_flags);
  }
  if (sock_flags == -1)
  {
    IDD_LOGE("%s: failed changing socket blocking mode to %sblocking",
             __func__, block ? "" : "non-");
    return -1;
  }
  return 0;
}

/**
 * Block until output is possible on the given socket (fd), or
 * until the timeout period has expired.
 *
 * @param sock Socket (fd)
 * @return 1 if output is possible, < 0 if timeout or error
 */
static int wait_ready(int sock)
{
  int retry_cnt = 0;
  const int FD_COUNT = 1;
  const int SOCKET_TIMEOUT_MS = SOCKET_TIMEOUT * 1000;
  struct pollfd fds[FD_COUNT] = {};

  fds[0].fd = sock;
  fds[0].events = POLLOUT;
  fds[0].revents = 0;

  for (; retry_cnt < 3; ++retry_cnt)
  {
    // poll() returns 1 if output is possible, 0 on timeout, -1 on error
    int ret = poll(fds, FD_COUNT, SOCKET_TIMEOUT_MS);
    if (ret == -1)
    {
      // continue loop if EINTR otherwise return
      if (ret != EINTR)
      {
        return -errno;
      }
    }
    else if (ret == 0)
    {
      return -ETIMEDOUT;
    }
    else if (ret == 1 && (fds[0].revents & POLLOUT))
    {
      return 1;
    }
  }
  return -EREMOTEIO;
}

/**
 * Wrapper for connect() with timeout.
 *
 * @param sock Socket (fd)
 * @param address Points to a sockaddr structure containing the peer address.
 * @param address_len Specifies the length of the sockaddr structure pointed
 *                    to by the address argument.
 * @return 0 if successful, < 0 if not
 */
static int connect_timeout(int sock, const struct sockaddr *address, socklen_t adress_len)
{
  int ret = 0;
  set_blocking_mode(sock, 0);
  while (connect(sock, address, adress_len) < 0 && ret == 0)
  {
    if (errno == EISCONN)
    {
      break;   // ret is 0
    }
    else if (errno == EAGAIN || errno == EINPROGRESS || errno == EALREADY)
    {
      // We expect wait_ready() to return 1. If so we will break the while loop
      // on errno == EISCONN.
      int socket_ready = wait_ready(sock);
      if (socket_ready != 1)
      {
        ret = socket_ready;  // negative
        break;
      }
    }
    else if (errno != EINTR)
    {
      ret = -errno;
      break;
    }
  }
  set_blocking_mode(sock, 1);
  if (ret != 0)
  {
    IDD_LOGE("failure %s: socket %d, returning %d", __func__, sock, ret);
  }
  return ret;
}

static const char* get_err(int errcode, char* buf_p, size_t len)
{
  if (strerror_r(errcode, buf_p, len) != 0)
  {
    strncpy(buf_p, "<failed to get error code>", len);
    buf_p[len - 1] = '\0';
  }
  return buf_p;
}

/**
 * Set send and recieve timeouts on the given socket.
 *
 * @param sock Socket (fd)
 */
static void set_timeouts(int sock)
{
  static struct timeval tv = {
    tv_sec: SOCKET_TIMEOUT,
    tv_usec: 0
  };
  // Log failure but do not propagate to caller
  if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) == -1 ||
      setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv)) == -1)
  {
    char errbuf[STR_ERROR_BUF_SIZE] = {0};
    IDD_LOGE("%s: failed to set socket timeouts, errcode=%d (%s)",
             __func__, errno, get_err(errno, errbuf, sizeof(errbuf)));
  }
}

/**
 * Set max size of socket message.
 *
 * The max size to send on a socket is set by the kernel. See socket() documentation.
 * " Max is the /proc/sys/net/core/wmem_max doubled"
 * In a Togari with KitKat: 131071 * 2 = 262142 = (255 * 1024) + 1022.
 *
 * @param sock Socket (fd)
 */
static void set_maxsize(int sock)
{
  int msgsize = (IDD_MAX_PACKET_SIZE + MESSAGE_HEADER_SIZE) / 2;
  int res = setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &msgsize, sizeof(msgsize));
  if(res == -1)
  {
    char errbuf[STR_ERROR_BUF_SIZE] = {0};
    IDD_LOGE("%s: failed to set socket maxsize, errcode=%d (%s)",
             __func__, errno, get_err(errno, errbuf, sizeof(errbuf)));
  }
}

/**
 * Sets SO_LINGER socket option
 *
 * @param sock Socket (fd)
 */
static void set_linger(int sock) {
  static struct linger so_linger;
  so_linger.l_onoff = 1;
  so_linger.l_linger = SOCKET_TIMEOUT;

  if (setsockopt(sock, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger)) == -1)
  {
    char errbuf[STR_ERROR_BUF_SIZE] = {0};
    IDD_LOGE("%s: failed to set socket linger_on option, errcode=%d (%s)",
             __func__, errno, get_err(errno, errbuf, sizeof(errbuf)));
  }
}

/**
 * Set socket functionality.
 *
 * @param sock Socket (fd)
 */
static void set_socket(int sock)
{
  // The socket is in blocking mode by default, set send and recv timeouts
  // (recv timeout not really needed since we do not read from this socket)
  set_timeouts(sock);
  set_maxsize(sock);
  set_linger(sock);
}

static int log_invalid_args(const char *func)
{
  IDD_LOGE("%s: illegal args", func);
  return -EINVAL;
}

static int log_send_failed(int sock, const char *func)
{
  char errbuf[STR_ERROR_BUF_SIZE] = {0};
  IDD_LOGE("%s: send failed, socket %d, errcode %d (%s)",
           func, sock, -errno, get_err(errno, errbuf, sizeof(errbuf)));
  return -errno;
}

static int log_recv_failed(int sock, const char *func)
{
  char errbuf[STR_ERROR_BUF_SIZE] = {0};
  IDD_LOGE("%s: receive failed, socket %d, errcode %d (%s)",
           func, sock, -errno, get_err(errno, errbuf, sizeof(errbuf)));
  return -errno;
}

static int log_connect_failed(int sock, const char *func, int errcode)
{
  char errbuf[STR_ERROR_BUF_SIZE] = {0};
  IDD_LOGE("%s: connect failed, socket %d, errcode %d (%s)",
           func, sock, -errcode, get_err(errno, errbuf, sizeof(errbuf)));
  return -errcode;
}

static void log_wait_ready_failed(int sock, const char *func, int errcode)
{
  char errbuf[STR_ERROR_BUF_SIZE] = {0};
  IDD_LOGE("%s: wait_ready failed, socket %d, errcode %d (%s)",
           func, sock, -errcode, get_err(errno, errbuf, sizeof(errbuf)));
}

IDD_t* idd_create(void)
{
  return idd_create_from_path(IDD_DEFAULT_BASE_DIR);
}

IDD_t* idd_create_from_path(const char* base_dir_p)
{
  struct idd_instance_t* this_p;

  this_p = malloc(sizeof(*this_p));

  if (this_p != NULL)
  {
    memset(&this_p->server_addr, 0, sizeof(this_p->server_addr));
    this_p->server_addr.sun_family = AF_UNIX;
    strncpy(this_p->server_addr.sun_path, IDD_CL_SOCK_FNAME, sizeof(this_p->server_addr.sun_path));

    if ((this_p->server_sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
    {
      free(this_p);
      return NULL;
    }
    set_socket(this_p->server_sock);
  }

  return (IDD_t*)this_p;
}

void idd_destroy(IDD_t** idd_pp)
{
  if (idd_pp != NULL && *idd_pp != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)*idd_pp;
    if (this_p->server_sock != -1)
    {
      shutdown(this_p->server_sock, SHUT_RDWR);
      close(this_p->server_sock);
    }

    free(this_p);
    *idd_pp = NULL;
  }
}

/**
 * Wrapper around sendto.
 *
 * By definition, sendto will only do atomic sends, i.e. it will never
 * send partial packets.
 *
 * @param this_p IDD instance
 * @param buffer_p The buffer to send
 * @param len The number of bytes to send
 *
 * @return this_p->buflen on success, -1 on failure
 */
static ssize_t socket_sendto(const struct idd_instance_t *this_p,
                             const void* buffer_p, size_t len)
{
  return RETRY_IF_EINTR(sendto(this_p->server_sock, buffer_p, len, 0,
                               (struct sockaddr *)&this_p->server_addr,
                               sizeof(this_p->server_addr)));
}

/**
 * Send data to the IDD daemon using the DGRAM type of socket
 * (connection-less).
 *
 * @param this_p IDD instance
 * @param func Name of calling function, used for error handling.
 * @return 0 if successful, < 0 if not. This does not indicate that
 * the command has actually been successfully executed or not by
 * the daemon.
 */
static int send_dgram(struct idd_instance_t *this_p, const char *func,
                      const void* buf, const size_t len)
{
  int ret = wait_ready(this_p->server_sock);  // negative if failure

  if (ret == 1)
  {
    ret = socket_sendto(this_p, buf, len);
    if (ret < 0)
    {
      ret = log_send_failed(this_p->server_sock, func);
    }
    else
    {
      ret = 0;
    }
  }
  else
  {
    log_wait_ready_failed(this_p->server_sock, func, -ret);
    ret = -1;
  }

  return ret;
}

/**
 * Send data to the IDD daemon using the SEQPACKET type of socket
 * (connection-oriented).
 *
 * @param sock
 * @param buf Buffer to send.
 * @param len Number of bytes to send.
 * @return non 0 if successful
 */
static int send_seq(int sock, const void *buf, size_t len)
{
  return (RETRY_IF_EINTR(send(sock, buf, len, 0)) == (ssize_t) len);
}

/**
 * Send data to the IDD daemon and wait for the result.
 * The result is one byte where '1' is OK, everything else is NOT_OK.
 *
 * @param idd IDD instance
 * @param buf Buffer to send, must be null terminated.
 * @param func The calling function, to improve error prints.
 * @return 1 if successful
 */
static int send_wait(IDD_t* idd, char *buf, const char *func)
{
  int ret = 0;
  if ((idd != NULL) && buf && *buf)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    int sock = connect_idd(this_p, IDD_CO_SOCK_FNAME, func);
    if (sock >= 0)
    {
      if (send_seq(sock, buf, strlen(buf)))
      {
        char reply[2] = {0};
        if (RETRY_IF_EINTR(recv(sock, reply, sizeof(reply), 0)) > 0)
        {
          if (reply[0] == '1')
          {
            ret = 1;
          }
        }
        else
        {
          ret = log_recv_failed(sock, func);
        }
      }
      else
      {
        ret = log_send_failed(sock, func);
      }
      close(sock);
    }
    else
    {
      ret = sock;
    }
  }
  else
  {
    ret = log_invalid_args(func);
  }
  return ret;
}

static int get_fd_msg(int sock, int ret_default)
{
  struct msghdr msg;
  struct iovec iov[1] = {0};
  ssize_t n;
  int recvfd = ret_default;
  char buf[10] = {0};

  union {
    struct cmsghdr cm;
    char control[CMSG_SPACE(sizeof(int))];
  } control_un;
  struct cmsghdr *cmsg_p;

  msg.msg_control = control_un.control;
  msg.msg_controllen = sizeof(control_un.control);

  msg.msg_name = NULL;
  msg.msg_namelen = 0;

  iov[0].iov_base = buf;
  iov[0].iov_len = sizeof(buf) - 1;
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  msg.msg_flags = 0;

  if ((n = RETRY_IF_EINTR(recvmsg(sock, &msg, 0))) > 0)
  {
    if ((cmsg_p = CMSG_FIRSTHDR(&msg)) != NULL &&
         cmsg_p->cmsg_len == CMSG_LEN(sizeof(int)))
    {
      if ((cmsg_p->cmsg_level == SOL_SOCKET) && cmsg_p->cmsg_type == SCM_RIGHTS)
      {
        recvfd = *((int *) CMSG_DATA(cmsg_p));
      }
    }
  }
  else if (n == -1)
  {
    recvfd = log_recv_failed(sock, __func__);
  }
  return recvfd;
}

/*
 * IDD api functions
 */
int idd_send_raw(IDD_t* idd, const char* raw_p, size_t raw_size)
{
  if (idd != NULL && !(raw_size > 0 && raw_p == NULL))
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    return send_dgram(this_p, __func__, raw_p, raw_size);
  }
  return log_invalid_args(__func__);
}

int idd_set_counter(IDD_t* idd, const char* package_p, const char* name_p, int64_t value)
{
  if (idd != NULL && package_p != NULL && name_p != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    char buf[3 + IDD_MAX_PROBE_ID_SIZE + 2 + 40] = {0};
    snprintf(buf, sizeof(buf), "CS %s %s %" PRId64, package_p, name_p, value);
    return send_dgram(this_p, __func__, buf, strlen(buf));
  }
  return log_invalid_args(__func__);
}

int idd_inc_counter(IDD_t* idd, const char* package_p, const char* name_p, int64_t delta)
{
  if (idd != NULL && package_p != NULL && name_p != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    char buf[3 + IDD_MAX_PROBE_ID_SIZE + 2 + 40] = {0};
    snprintf(buf, sizeof(buf), "CI %s %s %" PRId64, package_p, name_p, delta);
    return send_dgram(this_p, __func__, buf, strlen(buf));
  }
  return log_invalid_args(__func__);
}

int idd_set_sdata(IDD_t* idd, const char* package_p, const char* name_p, const char* value_p)
{
  if (idd != NULL && package_p != NULL && name_p != NULL && value_p != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    char buf[3 + IDD_MAX_PROBE_ID_SIZE + 2 + IDD_MAX_SDATA_SIZE + 1] = {0};
    snprintf(buf, sizeof(buf), "SD %s %s %s", package_p, name_p, value_p);
    return send_dgram(this_p, __func__, buf, strlen(buf));
  }
  return log_invalid_args(__func__);
}

static const char* get_sock_ret_msg(int reply)
{
  int i;
  for (i = 0; i < sizeof(idd_sock_ret)/sizeof(idd_sock_ret[0]) - 1; i++)
  {
    if (idd_sock_ret[i].reply == reply)
    {
      goto EXIT;
    }
  }
EXIT:
  return idd_sock_ret[i].msg;
}

static int send_stream(struct idd_instance_t *this_p, const char *func, char* buf, const size_t len)
{
  int ret = -1;
  int sock = connect_idd(this_p, IDD_CO_STREAM_SOCK_FNAME, __func__);
  if (sock >= 0)
  {
    size_t head_len = (size_t)log10(len) + 1;
    char* header = malloc(head_len + 1);
    if (header != NULL)
    {
      // Sending its size before starting stream socket
      snprintf(header, head_len + 1, "%zu", len);
      head_len = strlen(header);
      if (RETRY_IF_EINTR(write(sock, header, head_len)) == (ssize_t) head_len)
      {
        iddd_ret_val_t reply;
        if (RETRY_IF_EINTR(read(sock, &reply, sizeof(reply))) > 0)
        {
          if (reply == IDD_CO_SOCK_SEND_SUCCESS)
          {
            if (RETRY_IF_EINTR(write(sock, buf, len)) == (ssize_t) len)
            {
              ret = 0;
            }
            else
            {
              IDD_LOGE("%s: Failed to write %d bytes data to IDD daemon",__func__, len);
              ret = log_send_failed(sock, func);
            }
          }
          else
          {
            IDD_LOGE("%s: Failure at IDD daemon after writing header. Received %s error",
                                     __func__, get_sock_ret_msg(reply));
          }
        }
        else
        {
          IDD_LOGE("%s: Failed to get reply from IDD daemon",__func__);
          ret = log_recv_failed(sock, func);
        }
      }
      else
      {
        IDD_LOGE("%s: Failed to write %d bytes of header to IDD daemon",__func__, head_len);
        ret = log_send_failed(sock, func);
      }
      free(header);
    }
    close(sock);
  }
  return ret;
}

static int add_event(IDD_t* idd, char* cmd, char* extra, const char* package_p,
                     const char* name_p, const char* payload_p, size_t payload_size)
{
  if (idd != NULL && package_p != NULL && name_p != NULL && !(payload_size > 0 && payload_p == NULL))
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    int ret = -1;
    // NOTE: head_len will not include space for the trailing null that is
    // important to allocate if no payload (since using snprintf).
    size_t head_len = 3 /*EP/EV */ + strlen(extra) /* plugin_id */ + strlen(package_p) + 1 +
                      strlen(name_p) + 1;
    size_t buffer_len = head_len + payload_size;
    char* buf_p = malloc(buffer_len + 1);
    if (buf_p != NULL)
    {
      snprintf(buf_p, head_len + 1, "%s %s%s %s ", cmd, extra, package_p, name_p);
      memcpy(buf_p + head_len, payload_p, payload_size);
      if (buffer_len <= IDD_MAX_PACKET_SIZE)
      {
        ret = send_dgram(this_p, __func__, buf_p, buffer_len);
        if (ret == -ENOBUFS)
        {
          IDD_LOGE("%s: ENOBUFS (105) occured. Switching to stream socket...", __func__);
          ret = send_stream(this_p, __func__, buf_p, buffer_len);
        }
      }
      else
      {
        ret = send_stream(this_p, __func__, buf_p, buffer_len);
      }
    }
    free(buf_p);
    return ret;
  }
  return log_invalid_args(__func__);
}

int idd_add_event(IDD_t* idd, const char* package_p, const char* name_p,
                  const char* payload_p, size_t payload_size)
{
  return add_event(idd, "EV", "", package_p, name_p, payload_p, payload_size);
}

int idd_add_plugin_event(IDD_t* idd, int plugin_id, const char* package_p, const char* name_p,
                         const char* payload_p, size_t payload_size)
{
  char plugin[INT_MAX_LEN + 2] = {0};
  snprintf(plugin, sizeof(plugin), "%d ", plugin_id);
  return add_event(idd, "EP", plugin, package_p, name_p, payload_p, payload_size);
}

static int add_app_event(IDD_t* idd, char* cmd, char* extra, const char* app_package_p,
                         const char* app_version_name_p, int app_version_code,
                         const char* timestamp_p,
                         int payload_type, const char* payload_p, size_t payload_size)
{
  if (idd != NULL && app_package_p != NULL && app_version_name_p != NULL && timestamp_p != NULL &&
      !(payload_size > 0 && payload_p == NULL))
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    int ret = -1;
    // NOTE: head_len will not include space for the trailing null that is
    // important to allocate if no payload (since using snprintf).
    size_t head_len = 3 /* AD/AZ */ + strlen(extra) /* uncompressed_size */ +
                      strlen(app_package_p) + 1 +
                      strlen(app_version_name_p) + 1 +
                      (size_t)log10(app_version_code) + 1 + 1 +
                      strlen(timestamp_p) + 1 +
                      (size_t)log10(payload_type) + 1 + 1;
    size_t buffer_len = head_len + payload_size;
    char* buf_p = malloc(buffer_len + 1);
    if (buf_p != NULL)
    {
      snprintf(buf_p, head_len + 1, "%s %s%s %s %d %s %d", cmd, extra, app_package_p,
               app_version_name_p, app_version_code, timestamp_p, payload_type);
      memcpy(buf_p + head_len, payload_p, payload_size);
      if (buffer_len <= IDD_MAX_PACKET_SIZE)
      {
        ret = send_dgram(this_p, __func__, buf_p, buffer_len);
        if (ret == -ENOBUFS)
        {
          IDD_LOGE("%s: ENOBUFS (105) occured. Switching to stream socket...", __func__);
          ret = send_stream(this_p, __func__, buf_p, buffer_len);
        }
      }
      else
      {
        ret = send_stream(this_p, __func__, buf_p, buffer_len);
      }
      free(buf_p);
    }
    return ret;
  }
  return log_invalid_args(__func__);
}

int idd_add_app_event(IDD_t* idd, const char* app_package_p,
                      const char* app_version_name_p, int app_version_code,
                      const char* timestamp_p,
                      int payload_type, const char* payload_p, size_t payload_size)
{
  return add_app_event(idd, "AV", "", app_package_p, app_version_name_p, app_version_code,
                       timestamp_p, payload_type, payload_p, payload_size);
}

int idd_register_to_use_add_app_event(IDD_t* idd, const char* app_package_p,
                                      const char* class_name_p)
{
  int ret = 0;
  if (idd != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    int sock = connect_idd(this_p, IDD_CO_SOCK_FNAME, __func__);
    if (sock >= 0)
    {
      int bytes;
      char buf_p[3 + MAX_APP_EVENT_PACKAGE_NAME_SIZE + MAX_APP_EVENT_PACKAGE_LIST_LINE_LENGTH + 1]
          = {0};
      bytes = snprintf(buf_p, sizeof(buf_p), "RA %s %s", app_package_p, class_name_p);
      if (send_seq(sock, buf_p, bytes))
      {
        memset(buf_p, '\0', sizeof(buf_p));
        if (RETRY_IF_EINTR(recv(sock, buf_p, sizeof(buf_p), 0)) > 0)
        {
          ret = atoi(buf_p);
        }
        else
        {
          ret = log_recv_failed(sock, __func__);
        }
      }
      else
      {
        ret = log_send_failed(sock, __func__);
      }
      close(sock);
    }
    else
    {
      ret = sock;
    }
  }
  else
  {
    ret = log_invalid_args(__func__);
  }

  return ret;
}

int idd_get_app_event_enabled_packages(IDD_t* idd, char* registered_pkg)
{
  int ret = 0;
  if (idd != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    int sock = connect_idd(this_p, IDD_CO_SOCK_FNAME, __func__);
    if (sock >= 0)
    {
      if (send_seq(sock, "RP", 2))
      {
        char reply[MAX_APP_EVENT_ENABLED_PACKAGES_SIZE] = {0};
        if (RETRY_IF_EINTR(recv(sock, reply, sizeof(reply), 0)) > 0)
        {
          strncpy(registered_pkg, reply, MAX_APP_EVENT_ENABLED_PACKAGES_SIZE);
          ret = 0;
        }
        else
        {
          memset(registered_pkg, '\0', sizeof(registered_pkg));
          ret = log_recv_failed(sock, __func__);
        }
      }
      else
      {
        ret = log_send_failed(sock, __func__);
      }
      close(sock);
    }
    else
    {
      ret = sock;
    }
  }
  else
  {
    ret = log_invalid_args(__func__);
  }
  return ret;
}

int idd_flush(IDD_t* idd)
{
  if (idd != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    return send_dgram(this_p, __func__, "FL", 2);
  }
  return log_invalid_args(__func__);
}

int idd_write_config(IDD_t* idd, const char* config_p, size_t config_size)
{
  if (idd != NULL && !(config_size > 0 && config_p == NULL))
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    int ret = -1;
    // NOTE: head_len will not include space for the trailing null that is
    // important to allocate if no payload (since using snprintf).
    size_t head_len = 2;
    size_t buffer_len = head_len + 1 + config_size;
    char* buf_p = malloc(buffer_len);
    if (buf_p != NULL)
    {
      snprintf(buf_p, head_len + 1, "WC");
      memcpy(buf_p + head_len + 1, config_p, config_size);
      ret = send_dgram(this_p, __func__, buf_p, buffer_len);
      free(buf_p);
    }
    return ret;
  }
  return log_invalid_args(__func__);
}

int idd_notify_config(IDD_t* idd)
{
  return send_wait(idd, "NC", __func__);
}

/**
 * Connects to the IDD daemon using the SEQPACKET type of socket.
 *
 * @param this_p IDD instance
 * @param sock_fname Socket name
 * @param func Name of calling function, used for error handling.
 * @return 0 if successful, < 0 if not
 */
static int connect_idd(struct idd_instance_t *this_p, const char *sock_fname, const char *func)
{
  int sock, len, connect_res;
  struct sockaddr_un remote;
  int sock_type = -1;

  if (!strcmp(sock_fname, IDD_CO_SOCK_FNAME))
  {
    sock_type = SOCK_SEQPACKET;
    strncpy(remote.sun_path, IDD_CO_SOCK_FNAME, sizeof(remote.sun_path));
  }
  else if (!strcmp(sock_fname, IDD_CO_STREAM_SOCK_FNAME))
  {
    sock_type = SOCK_STREAM;
    strncpy(remote.sun_path, IDD_CO_STREAM_SOCK_FNAME, sizeof(remote.sun_path));
  }
  else
  {
    sock_type = SOCK_DGRAM;
    strncpy(remote.sun_path, IDD_CL_SOCK_FNAME, sizeof(remote.sun_path));
  }

  if ((sock = socket(AF_UNIX, sock_type, 0)) == -1)
  {
    return log_connect_failed(sock, func, errno);
  }

  remote.sun_family = AF_UNIX;

  len = strlen(remote.sun_path) + sizeof(remote.sun_family);
  connect_res = connect_timeout(sock, (struct sockaddr *)&remote, len);
  if (connect_res < 0)
  {
    close(sock);
    return log_connect_failed(sock, func, -connect_res);
  }
  set_socket(sock);
  return sock;
}

int idd_is_active(IDD_t* idd)
{
  return send_wait(idd, "IA", __func__);
}

int idd_is_probe_active(IDD_t* idd, const char* package_p, const char* name_p)
{
  char buf[3 + IDD_MAX_PROBE_ID_SIZE + 2] = {0};
  snprintf(buf, sizeof(buf), "PA %s %s", package_p, name_p);
  return send_wait(idd, buf, __func__);
}

int idd_get_config(IDD_t* idd, const char* package_p, const char* name_p)
{
  int fd = IDD_NO_CONFIG_AVAILABLE;
  if (idd != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    int sock = connect_idd(this_p, IDD_CO_SOCK_FNAME, __func__);

    if (sock >= 0)
    {
      char buf[3 + IDD_MAX_PROBE_ID_SIZE + 2] = {0};
      int bytes = snprintf(buf, sizeof(buf), "GC %s %s", package_p, name_p);

      if (send_seq(sock, buf, bytes))
      {
        fd = get_fd_msg(sock, IDD_NO_CONFIG_AVAILABLE);
      }
      else
      {
        fd = log_send_failed(sock, __func__);
      }
      close(sock);
    }
    else
    {
      fd = sock;
    }
  }
  else
  {
    fd = log_invalid_args(__func__);
  }
  return fd;
}

int idd_get_config_notify(IDD_t* idd)
{
  int fd = -1;
  if (idd != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    int sock = connect_idd(this_p, IDD_CO_SOCK_FNAME, __func__);

    if (sock >= 0)
    {
      if (send_seq(sock, "CN", 2))
      {
        fd = get_fd_msg(sock, -1);
      }
      else
      {
        fd = log_send_failed(sock, __func__);
      }
      close(sock);
    }
    else
    {
      fd = sock;
    }
  }
  else
  {
    fd = log_invalid_args(__func__);
  }
  return fd;
}

int idd_config_wait(IDD_t* idd, int fd)
{
  if (idd != NULL && fd >= 0)
  {
    char buf[64] = {0};
    int ret = read(fd, buf, sizeof(buf));
    if (ret < 0)
    {
      return -errno;
    }
    if (ret == sizeof(buf)) {
      long flag = fcntl(fd, F_GETFL);
      if (flag == -1)
      {
        IDD_LOGE("%s: failed to get status flag: %s", __func__, strerror(errno));
        return -errno;
      }
      if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1)
      {
        IDD_LOGE("%s: failed to set status flag(O_NONBLOCK): %s", __func__, strerror(errno));
        return -errno;
      }
      while (read(fd, buf, sizeof(buf)) > 0)
      {
        IDD_LOGW("Non blocking read loop");
      }
      if (fcntl(fd, F_SETFL, flag) == -1)
      {
        IDD_LOGE("%s: failed to set status flag: %s", __func__, strerror(errno));
        return -errno;
      }
    }
  }
  else
  {
    return log_invalid_args(__func__);
  }
  return 1;
}

int idd_set_imei(IDD_t* idd, const char* imei_p)
{
  if (idd != NULL && imei_p != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    char buf[3 + IDD_MAX_DEVICE_IMEI_SIZE + 1] = {0};
    snprintf(buf, sizeof(buf), "IM %s", imei_p);
    return send_dgram(this_p, __func__, buf, strlen(buf));
  }
  return log_invalid_args(__func__);
}

int idd_set_android_id(IDD_t* idd, const char* android_id_p)
{
  if (idd != NULL && android_id_p != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    char buf[3 + IDD_MAX_ANDROID_ID_SIZE + 1] = {0};
    snprintf(buf, sizeof(buf), "AI %s", android_id_p);
    return send_dgram(this_p, __func__, buf, strlen(buf));
  }
  return log_invalid_args(__func__);
}

int idd_set_xs_android_id(IDD_t* idd, const char* xs_android_id_p)
{
  if (idd != NULL && xs_android_id_p != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    char buf[3 + IDD_MAX_XS_ANDROID_ID_SIZE + 1] = {0};
    snprintf(buf, sizeof(buf), "XA %s", xs_android_id_p);
    return send_dgram(this_p, __func__, buf, strlen(buf));
  }
  return log_invalid_args(__func__);
}

int idd_set_xs_guid(IDD_t* idd, const char* xs_guid_p)
{
  if (idd != NULL && xs_guid_p != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    char buf[3 + IDD_MAX_XS_GUID_SIZE + 1] = {0};
    snprintf(buf, sizeof(buf), "XG %s", xs_guid_p);
    return send_dgram(this_p, __func__, buf, strlen(buf));
  }
  return log_invalid_args(__func__);
}

int idd_set_user_consent(IDD_t* idd, int user_consent)
{
  if (idd != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    char buf[5] = {0};
    snprintf(buf, sizeof(buf), "UC %d", user_consent);
    return send_dgram(this_p, __func__, buf, sizeof(buf));
  }
  return log_invalid_args(__func__);
}

int idd_force_save(IDD_t* idd)
{
  if (idd != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    return send_dgram(this_p, __func__, "FS", 2);
  }
  return log_invalid_args(__func__);
}

int idd_set_serial_number(IDD_t* idd, const char* serial_number_p)
{
  if (idd != NULL && serial_number_p != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    char buf[3 + IDD_MAX_SERIAL_NUMBER_SIZE + 1] = {0};
    snprintf(buf, sizeof(buf), "SN %s", serial_number_p);
    return send_dgram(this_p, __func__, buf, strlen(buf));
  }
  return log_invalid_args(__func__);
}

int idd_set_data_collection_enable(IDD_t* idd, int enable)
{
  if (idd != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    char buf[5] = {0};
    snprintf(buf, sizeof(buf), "DC %d", enable);
    return send_dgram(this_p, __func__, buf, sizeof(buf));
  }
  return log_invalid_args(__func__);
}

int idd_set_group_id(IDD_t* idd, const char* group_id_p)
{
  if (idd != NULL && group_id_p != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    char buf[3 + IDD_MAX_GROUP_ID_SIZE + 1] = {0};
    snprintf(buf, sizeof(buf), "SG %s", group_id_p);
    return send_dgram(this_p, __func__, buf, strlen(buf));
  }
  return log_invalid_args(__func__);
}

int idd_delete_group_id(IDD_t* idd, const char* del_group_id_p)
{
  if (idd != NULL && del_group_id_p != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    char buf[3 + IDD_MAX_GROUP_ID_SIZE + 1] = {0};
    snprintf(buf, sizeof(buf), "DG %s", del_group_id_p);
    return send_dgram(this_p, __func__, buf, strlen(buf));
  }
  return log_invalid_args(__func__);
}

int idd_replace_group_id(IDD_t* idd, const char* to_group_id_p, const char* from_group_id_p)
{
  if (idd != NULL && to_group_id_p != NULL && from_group_id_p != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    char buf[3 + IDD_MAX_GROUP_ID_SIZE + 1 + IDD_MAX_GROUP_ID_SIZE + 1] = {0};
    snprintf(buf, sizeof(buf), "RG %s %s", to_group_id_p, from_group_id_p);
    return send_dgram(this_p, __func__, buf, strlen(buf));
  }
  return log_invalid_args(__func__);
}

int idd_remove_report(IDD_t* idd, int report_num)
{
  if (idd != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    char buf[5] = {0};
    snprintf(buf, sizeof(buf), "RM %d", report_num);
    return send_dgram(this_p, __func__, buf, sizeof(buf));
  }
  return log_invalid_args(__func__);
}

int idd_get_device_id(IDD_t* idd, char* device_id)
{
  int ret;
  if (idd != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    int sock = connect_idd(this_p, IDD_CO_SOCK_FNAME, __func__);

    if (sock >= 0)
    {
      if (send_seq(sock, "GD", 2))
      {
        char reply[IDD_MAX_DEVICE_ID_SIZE] = {0};
        memset(reply, '\0', sizeof(reply));
        if (RETRY_IF_EINTR(recv(sock, reply, sizeof(reply), 0)) > 0)
        {
          strncpy(device_id, reply, IDD_MAX_DEVICE_ID_SIZE);
          ret = 0;
        }
        else
        {
          ret = log_recv_failed(sock, __func__);
        }
      }
      else
      {
        ret = log_send_failed(sock, __func__);
      }
      close(sock);
    }
    else
    {
      ret = sock;
    }
  }
  else
  {
    ret = log_invalid_args(__func__);
  }
  return ret;
}

int idd_config_request_get(IDD_t* idd, char* cr, int cr_size)
{
  int cr_len = 0;
  if (idd != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    int sock = connect_idd(this_p, IDD_CO_SOCK_FNAME, __func__);

    if (sock >= 0)
    {
      if (send_seq(sock, "CR", 2))
      {
        if ((cr_len = RETRY_IF_EINTR(recv(sock, cr, cr_size, 0))) > 0)
        {
          if (DAEMON_ERROR(cr, cr_len))
          {
            cr_len = IDD_NO_DATA;
          }
        }
        else
        {
          cr_len = log_recv_failed(sock, __func__);
        }
      }
      else
      {
        cr_len = log_send_failed(sock, __func__);
      }
      close(sock);
    }
    else
    {
      cr_len = sock;
    }
  }
  else
  {
    cr_len = log_invalid_args(__func__);
  }
  return cr_len;
}

int idd_get_max_payload(void)
{
  return IDD_MAX_EVENT_SIZE;
}

int idd_do_washing(IDD_t* idd)
{
  return send_wait(idd, "DW", __func__);
}

int idd_set_timezone(IDD_t* idd)
{
  if (idd != NULL)
  {
    struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
    return send_dgram(this_p, __func__, "TZ", 2);
  }
  return log_invalid_args(__func__);
}

int idd_get_config_path(IDD_t* idd, const char* package_p, const char* name_p, char* out_path)
{
  if (idd == NULL) return 0;

  struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
  int sock = connect_idd(this_p, IDD_CO_SOCK_FNAME, __func__);
  if (sock < 0) return 0;

  int ret = 0;
  const int _buf_max = 4 + IDD_MAX_PROBE_ID_SIZE + 2;
  char buf[_buf_max] = {0};
  snprintf(buf, sizeof(buf), "GCP %s %s", package_p, name_p);
  if (send_seq(sock, buf, strnlen(buf, _buf_max)))
  {
    char reply[PATH_MAX] = {0};
    memset(reply, '\0', sizeof(reply));
    if (RETRY_IF_EINTR(recv(sock, reply, sizeof(reply), 0)) > 0)
    {
      strncpy(out_path, reply, PATH_MAX);
      if (strncmp(out_path, "-1", 2) != 0) ret = 1;
    }
    else
    {
      ret = log_recv_failed(sock, __func__);
    }
  }
  else
  {
    ret = log_send_failed(sock, __func__);
  }
  close(sock);

  return ret;
}

static int split(char* input, const char* separator, char** result, int result_max) {
  if (input == NULL) return -1;
  int i = 0;
  char* p = strtok(input, separator);
  while (p != NULL) {
    if (i >= result_max) break;
    result[i] = p;
    i++;
    p = strtok(NULL, separator);
  }
  return i;
}

int idd_get_report_stats(IDD_t* idd, int* oldest_file_nr, int* newest_file_nr, int* total_file_nr, size_t* sum_size)
{
  if (idd == NULL) return 0;

  struct idd_instance_t* this_p = (struct idd_instance_t*)idd;
  int sock = connect_idd(this_p, IDD_CO_SOCK_FNAME, __func__);
  if (sock < 0) return 0;

  int ret = 0;
  const int _buf_max = 4 + IDD_MAX_PROBE_ID_SIZE + 2;
  char buf[_buf_max] = {0};
  snprintf(buf, sizeof(buf), "GRS");
  if (send_seq(sock, buf, strnlen(buf, _buf_max)))
  {
    char reply[PATH_MAX] = {0};
    memset(reply, '\0', sizeof(reply));
    if (RETRY_IF_EINTR(recv(sock, reply, sizeof(reply), 0)) > 0)
    {
      IDD_LOGD("%s", reply);
      int param_num = 4;
      char* result[param_num];
      int result_num = split(reply, ",", result, param_num);
      if (result_num != param_num) ret = -1;
      *oldest_file_nr = atoi(result[0]);
      *newest_file_nr = atoi(result[1]);
      *total_file_nr = atoi(result[2]);
      *sum_size = atoi(result[3]);
      ret = 1;
    }
    else
    {
      ret = log_recv_failed(sock, __func__);
    }
  }
  else
  {
    ret = log_send_failed(sock, __func__);
  }
  close(sock);

  return ret;
}
