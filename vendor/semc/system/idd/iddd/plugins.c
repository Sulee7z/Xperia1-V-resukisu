/*
 * Copyright 2014 Sony Corporation
 */

/**
 * @brief IDD plugin to IDD that stores appropriate IDD probes in its own folder.
 *
 * @author Leif Mårtensson (leif.martensson@sonymobile.com)
 */
#include <fcntl.h>
#include <stdlib.h>
#include <limits.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/wait.h>
#include <sys/wait.h>
#include <inttypes.h>

#define LOG_TAG "iddd-plugins"
#include "logging.h"
#include "serializer.h"
#include "util.h"
#include "compactor.h"
#include "config.h"
#include "configuration.h"
#include "protos/idd/app_usage_data.pb-c.h"
#include "protos/idd/iddd.pb-c.h"
#include "idd_config.h"
#include "file_utils.h"
#include "filter.h"
#include "plugins.h"
#include "iddd_miscta.h"

/*
 * The package and name for IDD plugins configuration
 */
#define PLUGINS_PROBE_PACKAGE   "iddd"
#define PLUGINS_PROBE_NAME      "ConfigPlugin"
#define PLUGINS_RCA_FOLDER      "plugins"
#define PLUGINS_DIR             PRIVATE_DIR_NAME PLUGINS_RCA_FOLDER
#define PLUGINS_RCA_ROOTDIR     "/mnt/rca/"
#define PLUGINS_RCA_DIR         PLUGINS_RCA_ROOTDIR PLUGINS_RCA_FOLDER
#define PLUGINS_ENCRYPT_TMPFILE "encrypt.tmp"
#define PLUGINS_FILE_MODE       PRIVATE_DIR_MODE

typedef enum {
  PLUGINS_SILENT,
  PLUGINS_VERBOSE
} plugins_remove_t;

static Iddd__ConfigPlugin* g_config = NULL;

typedef struct {
  char dir[PATH_MAX];      // Full path to plugin dir
  char current[PATH_MAX];  // Full path to current report. If current[0] == '\0' there is no
  int high_nr;             // The highest (newest) report nr in dir
  int low_nr;              // The lowest (oldest) report nr in dir
  size_t dir_size;         // The total size of all files but current uncompressed report in the
                           // plugin dir
  idd_filter_t filter[MAX_FILTERS];
} plugin_t;

static plugin_t* g_plugins = NULL;

/*
 * Minimum free space on RCA partition when a flush report is to be done for a RCA plugin.
 * Old reports will then be removed from that RCA plugin until this amount of space is free.
 */
#define RCA_MIN_FREE_SPACE (512 * 1024)

/**
 * Returns 1 if the plugin shall store data regardless if IDD is enabled. Otherwise 0.
 */
static int plugins_get_ignore_idd_enabled(Iddd__ConfigPlugin__Plugin* cfg)
{
  return cfg->has_ignore_idd_enabled ? cfg->ignore_idd_enabled : 0;
}

/**
 * Returns NO_CHECK_IDD_ENABLED if the plugin shall store data regardless if IDD is enabled.
 * Otherwise CHECK_IDD_ENABLED.
 */
static chk_idd_t plugins_get_check_idd_enabled(Iddd__ConfigPlugin__Plugin* cfg)
{
  return plugins_get_ignore_idd_enabled(cfg) ? NO_CHECK_IDD_ENABLED : CHECK_IDD_ENABLED;
}

/**
 * Returns true if the given plugin will store IDD data. Otherwise false.
 */
static int plugins_store_data(Iddd__ConfigPlugin__Plugin* cfg)
{
  return is_data_collection_enabled() || plugins_get_ignore_idd_enabled(cfg);
}

/**
 */
static int plugins_report_delete(char* file, plugins_remove_t mode)
{
  if (unlink(file) == (-1))
  {
    if (mode == PLUGINS_VERBOSE)
    {
      IDD_LOGE("Failed to delete %s, <%s>", file, strerror(errno));
    }
    return 0;
  }
  return 1;
}

/**
 * Checks if the report has reached max size.
 */
static int plugins_report_has_max_size(int fd, int max_size)
{
  if (fd != (-1))
  {
    off_t position = lseek(fd, 0, SEEK_END);
    if (position >= max_size)
    {
      IDD_LOGD("Max file size (%d) reached, opening new file", max_size);
      return 1;
    }
  }
  return 0;
}

/**
 * Scan the given plugin directory and update plugin structure.
 * If no reports found, low_nr (oldest report) is set to INT_MAX and high_nr is set to 0.
 * Returns 0 on success, -1 on error
 */
static int plugins_scandir(plugin_t* plugin, Iddd__ConfigPlugin__Plugin* cfg)
{
  char name[PATH_MAX] = {0};
  struct dirent *de;
  struct stat sb;
  DIR *dir;
  int low_nr = INT_MAX;
  int high_nr = 0;
  int high_is_unpacked = 0;

  plugin->dir_size = 0;

  dir = opendir(plugin->dir);
  if (dir == NULL)
  {
    return (-1);
  }

  rewinddir(dir);

  while ((de = readdir(dir)) != NULL)
  {
    int unpacked;
    int file_nr;
    if (strncmp(de->d_name, ".", PATH_MAX) == 0 ||
        strncmp(de->d_name, "..", PATH_MAX) == 0)
    {
      continue;
    }

    // Find out which report number this is
    snprintf(name, sizeof(name), "%s_%%d", cfg->report_prefix);
    if ((unpacked = sscanf(de->d_name, name, &file_nr)) != 1)
    {
      // Sum file sizes for all files without the current non compressed report
      snprintf(name, sizeof(name), "%s/%s", plugin->dir, de->d_name);
      if (lstat(name, &sb) == 0)
      {
        plugin->dir_size += sb.st_size;
      }
      else
      {
        IDD_LOGE("Scandir. Failed to stat %s: %s", name, strerror(errno));
      }

      snprintf(name, sizeof(name), "%s_%%d", cfg->report_prefix_packed);
      if (sscanf(de->d_name, name, &file_nr) != 1)
      {
        IDD_LOGW("Unexpected file in %s directory: %s, format <%s>, file_nr = %d",
                 plugin->dir, de->d_name, name, file_nr);
        continue;
      }
    }

    if (file_nr < low_nr)
    {
      low_nr = file_nr;
    }

    if (file_nr > high_nr)
    {
      high_nr = file_nr;
      high_is_unpacked = unpacked;
    }
  }

  plugin->high_nr = high_nr;
  plugin->low_nr = low_nr;
  if (high_is_unpacked == 1)
  {
    snprintf(plugin->current, PATH_MAX, "%s/%s_%d", plugin->dir, cfg->report_prefix, high_nr);
  }

  closedir(dir);
  return 0;
}

/**
 * Returns 1 if the given plugin is a RCA plugin. Otherwise 0
 */
static int plugins_rca_plugin(Iddd__ConfigPlugin__Plugin* cfg)
{
  return cfg->has_type && (cfg->type == IDDD__CONFIG_PLUGIN__PLUGIN__TYPE__RCA) ? 1 : 0;
}

/**
 * Delete reports for given plugin if
 * - More than one file exists for given plugin
 * and
 *   - given plugin of type RCA and not enough available space.
 *   or
 *   - max directory size is reached for given plugin.
 */
static int plugins_prune(plugin_t* plugin, Iddd__ConfigPlugin__Plugin* cfg, size_t extra_size)
{
  int res = 1;

  while (res)
  {
    // Since the compactor is working in another process we can't rely on the old dir_size
    if (plugins_scandir(plugin, cfg) == (-1))
    {
      IDD_LOGE("scandir(%s) failed: %d <%s>", plugin->dir, errno, strerror(errno));
      res = 0;
    }

    if (res & (plugin->low_nr >= plugin->high_nr))
    {
      IDD_LOGD("No delete! Only one or no file <%s>. low_nr = %d, high_nr = %d",
               plugin->dir, plugin->low_nr, plugin->high_nr);
      break;
    }

    if (res &&
        (!plugins_rca_plugin(cfg) ||
         is_free_space_enough(PLUGINS_RCA_ROOTDIR, RCA_MIN_FREE_SPACE + extra_size)) &&
        (((int64_t)plugin->dir_size + cfg->report_max_size) < cfg->max_output_dir_size))
    {
      IDD_LOGD("There is space enough in <%s>. %" PRId64 " < %" PRId64,
               plugin->dir, plugin->dir_size + cfg->report_max_size, cfg->max_output_dir_size);
      break;
    }

    if (res)
    {
      char name[PATH_MAX] = {0};
      struct stat sb;
      snprintf(name, PATH_MAX, "%s/%s_%d", plugin->dir, cfg->report_prefix_packed, plugin->low_nr);
      if (lstat(name, &sb) == 0)
      {
        IDD_LOGD("Max dir size or partition utilization reached, delete <%s>", name);
        if (!plugins_report_delete(name, PLUGINS_VERBOSE))
        {
          res = 0;
        }
      }
      else
      {
        IDD_LOGE("Max dir size or partition utilization reached. Failed to stat %s: %s",
                  name, strerror(errno));
        res = 0;
      }
    }
  }

  return res;
}

/**
 * Find the name of next report.
 * Also delete reports for plugin if needed.
 */
static int plugins_report_new_current(plugin_t* plugin, Iddd__ConfigPlugin__Plugin* cfg)
{
  int res = plugins_prune(plugin, cfg, 0);

  if (res)
  {
    plugin->high_nr++;
    snprintf(plugin->current, PATH_MAX, "%s/%s_%d", plugin->dir, cfg->report_prefix,
                                                    plugin->high_nr);
    if (plugin->low_nr > plugin->high_nr)
    {
      // This must be the first report, then low_nr shouldn't be INT_MAX anymore.
      plugin->low_nr = plugin->high_nr;
    }
  }

  return res;
}

/**
 * Opens the plugin current report.
 * returns -1 on failure.
 */
static int plugins_report_open_current(plugin_t* plugin, Iddd__ConfigPlugin__Plugin* cfg)
{
  int current_exist = (plugin->current[0] != '\0');
  if (!current_exist && !plugins_report_new_current(plugin, cfg))
  {
    return -1;
  }

  int fd = open(plugin->current, O_CREAT | O_WRONLY | O_APPEND, PLUGINS_FILE_MODE);
  if (fd == (-1))
  {
    IDD_LOGE("Plugin, Failed to open existing report %s, <%s>", plugin->current, strerror(errno));
  }
  else if (!current_exist && cfg->report_header)
  {
    if (plugins_rca_plugin(cfg))
    {
      write_rca_report_header(fd);
    }
    else
    {
      write_report_header(fd, FORCE_WRITE);
    }
  }

  return fd;
}

/**
 * Return the base plugin directory. For RCA plugins the reports shall be stored at the partition
 * EMMA will send, that is the appslog partition with mount point RCA_DIR.
 */
static char* plugins_base_dir_get(Iddd__ConfigPlugin__Plugin* cfg)
{
  return plugins_rca_plugin(cfg) ? PLUGINS_RCA_DIR : PLUGINS_DIR;
}

/**
 * Compress the current report.
 */
static void plugins_compress(plugin_t* plugin, Iddd__ConfigPlugin__Plugin* cfg)
{
  static pid_t plugins_compactor_pid = -1;
  char packed_file[PATH_MAX] = {0};
  snprintf(packed_file, PATH_MAX, "%s/%s_%d", plugin->dir, cfg->report_prefix_packed,
                                              plugin->high_nr);

  if (plugins_compactor_pid != -1)
  {
    /* The compactor has been running. In worst case, it's
     * still running, so we then wait for it here until it finishes.
     */
    IDD_LOGI("Compressing Report <%s>. Waiting...", plugin->current);
    waitpid(plugins_compactor_pid, NULL, 0);
    plugins_compactor_pid = -1;
  }

  IDD_LOGD("Compressing Report <%s>...", plugin->current);

  /* Now fork a child which will compress the output and put it in the correct directory */
  plugins_compactor_pid = fork();
  if (plugins_compactor_pid == 0)
  {
    /* I'm the child. */
    int res = report_compact(plugin->current, packed_file, cfg->report_header);
    plugins_report_delete(plugin->current, PLUGINS_VERBOSE);
    if (!res)
    {
      IDD_LOGE("Compressing Report <%s>. Failed...", packed_file);
      exit(EXIT_FAILURE);
    }
    IDD_LOGD("Compressing Report <%s>. Done...", packed_file);

    if (cfg->encrypt)
    {
      char tmp_encrypt_file[PATH_MAX] = {0};
      snprintf(tmp_encrypt_file, PATH_MAX, "%s/" PLUGINS_ENCRYPT_TMPFILE,
               plugins_base_dir_get(cfg));
      IDD_LOGD("Encrypting Report <%s> using tmpfile <%s>...", packed_file, tmp_encrypt_file);
      if (!encrypt_handler(packed_file, tmp_encrypt_file))
      {
        IDD_LOGE("Failed to encrypted report <%s>: %s", packed_file, strerror(errno));
        exit(EXIT_FAILURE);
      }
    }

    exit(EXIT_SUCCESS);
  }
  else if (plugins_compactor_pid == -1)
  {
    IDD_LOGE("Failed to fork compactor thread for <%s>.", plugin->current);
  }

  plugin->current[0] = '\0'; // To force a new report
}

/**
 * Opens the current plugin report file.
 * The current report is compressed if the file size has reached it's max value.
 * If there is no current report, a new empty report file is created as current.
 * returns -1 on failure.
 */
static int plugins_report_open(plugin_t* plugin, Iddd__ConfigPlugin__Plugin* cfg)
{
  int fd = plugins_report_open_current(plugin, cfg);
  if (plugins_report_has_max_size(fd, cfg->report_max_size))
  {
    close(fd);
    if (plugin->high_nr < INT_MAX)
    {
      plugins_compress(plugin, cfg);
    }
    else
    {
      IDD_LOGD("Max report nr reached <%s>, Clean dir and start from beginning.", plugin->current);
      (void)remove_directory(plugin->dir, 0);
      plugin->current[0] = '\0'; // To force a new report
    }
    fd = plugins_report_open_current(plugin, cfg);
  }
  return fd;
}

/**
 * Writes one event to the given file descriptor.
 *
 * @param fd filedescriptor to plugin
 * @param plugin internal plugin structure
 * @param cfg The IDD plugin configuration
 * @param package name of the package
 * @param name name of the event
 * @param payload incoming payload from client
 * @param payload_size size of the payload
 */

static int plugins_report_write(int fd, plugin_t* plugin, Iddd__ConfigPlugin__Plugin* cfg,
                               const char* package, const char* name, const char* payload,
                               size_t payload_size)
{
  /* The header buffer must be large enough to hold the event's header protocol buffer.
   * And the largest dynamic parts are the package and name size - the 100 is just a "very large
   * approximation of the rest" (which is rather around 20 bytes)
   */
  static char headerbuf[IDD_MAX_PROBE_ID_SIZE + 100] = {0};
  int64_t timestamp = get_timestamp();
  size_t header_size = ser_event_header(headerbuf, sizeof(headerbuf), package, name, timestamp,
                                        payload_size);
  int written = 0;

  if (header_size > 0)
  {
    int try = 0;
    off_t position = lseek(fd, 0, SEEK_END);
    if (position == -1)
    {
      IDD_LOGE("Failed to set end of file offset(lseek), errno %d <%s>", errno, strerror(errno));
      return 0;
    }
    while (try++ < 2)
    {
      errno = 0;
      written = write_all(fd, headerbuf, header_size);
      if (written >= 0 && payload_size > 0)
      {
        written = write_all(fd, payload, payload_size);
      }
      if (written < 0)
      {
        int errno_write = errno;
        errno = 0;
        if (errno_write == ENOSPC)
        {
          if (ftruncate(fd, position) == 0) /* Reset end of file */
          {
            if (lseek(fd, position, SEEK_SET) < 0)  /* Reset file pointer */
            {
              IDD_LOGE("Failed to set file position to %ld after file truncate. errno %d <%s>",
                       position, errno, strerror(errno));

            }
            if (try == 1)
            {
              IDD_LOGW("Failed to write event %s::%s with %zu bytes payload (ENOSPC). "
                       "Deleting old reports", package, name, payload_size);
              (void)plugins_prune(plugin, cfg, header_size + payload_size);
              continue; // Try to store the event again after deleting files.
            }
          }
          else
          {
            IDD_LOGE("ftruncate failed, errno %d <%s>", errno, strerror(errno));
          }
        }
        IDD_LOGE("Failed to write event %s::%s with %zu bytes header and %zu bytes payload."
                  " Error: %s", package, name, header_size, payload_size, strerror(errno_write));
      }
      break; /* Success or failure doesn't matter. Don't write the event again */
    }
  }
  else
  {
    IDD_LOGE("Failed to serialize header %s::%s", package, name);
  }

  return (written > 0);
}

/**
 *
 */
int plugins_report_iterator_next(int client_id, int iterator)
{
  int iter = -1;
  if (g_config)
  {
    size_t i;
    for (i = 0; i < g_config->n_plugin; i++)
    {
      Iddd__ConfigPlugin__Plugin* cfg = g_config->plugin[i];
      if (cfg->has_client_id && (cfg->client_id == client_id))
      {
        if ((g_plugins[i].high_nr >= g_plugins[i].low_nr) &&
            (++iterator <= g_plugins[i].high_nr))
        {
          iter = MAX(iterator, g_plugins[i].low_nr);
        }
        break;
      }
    }
  }
  return iter;
}

/**
 *
 */
int plugins_report_get(int client_id, int iterator)
{
  int fd = -1;
  if (g_config)
  {
    size_t i;
    for (i = 0; i < g_config->n_plugin; i++)
    {
      Iddd__ConfigPlugin__Plugin* cfg = g_config->plugin[i];
      if (cfg->has_client_id && (cfg->client_id == client_id))
      {
        char name[PATH_MAX] = {0};
        snprintf(name, PATH_MAX, "%s/%s_%d", g_plugins[i].dir, cfg->report_prefix_packed, iterator);
        fd = open(name, O_RDONLY);
        if (fd < 0)
        {
          snprintf(name, PATH_MAX, "%s/%s_%d", g_plugins[i].dir, cfg->report_prefix, iterator);
          fd = open(name, O_RDONLY);
        }
        break;
      }
    }
  }
  return fd;
}

/**
 *
 */
int plugins_filter_is_probe_active(const char* package, const char* name)
{
  int active = 0;
  if (g_config)
  {
    size_t i;
    for (i = 0; !active && (i < g_config->n_plugin); i++)
    {
      active = filter_is_probe_active(g_plugins[i].filter + EVENT, package, name,
                                      plugins_get_check_idd_enabled(g_config->plugin[i]));
    }
  }
  return active;
}

/**
 *
 */
void plugins_handle_event(int plugin_id, const char* package, const char* name,
                          size_t payload_size, const char* payload)
{
  if (g_config)
  {
    size_t i;
    for (i = 0; i < g_config->n_plugin; i++)
    {
      Iddd__ConfigPlugin__Plugin* cfg = g_config->plugin[i];
      if (((plugin_id == ALL_PLUGINS) || (plugin_id == (int)cfg->plugin_id)) &&
          filter_is_probe_active(g_plugins[i].filter + EVENT, package, name,
                                 plugins_get_check_idd_enabled(cfg)))
      {
        int fd = plugins_report_open(&g_plugins[i], cfg);
        if (fd != (-1))
        {
          char* pkg_p = NULL;
          char* name_p = NULL;
          if (!strncmp(package, PACKAGE_PROBE_ANDROID_IDD_VIRTUAL_APP_EVENT,
              sizeof(PACKAGE_PROBE_ANDROID_IDD_VIRTUAL_APP_EVENT)))
          {
            pkg_p = (char *)idd__app_event__descriptor.package_name;
            name_p = (char *)idd__app_event__descriptor.short_name;
          }
          else
          {
            pkg_p = (char *)package;
            name_p = (char *)name;
          }
          if (plugins_report_write(fd, &g_plugins[i], cfg, pkg_p, name_p, payload, payload_size) &&
              plugins_rca_plugin(cfg))
          {
            miscta_set_report_data();
          }
          close(fd);
          fd = (-1);
        }
      }
    }
  }
}

/**
 *
 */
static Iddd__ConfigPlugin__Plugin* plugins_get(char *dir)
{
  Iddd__ConfigPlugin__Plugin* plugin = NULL;

  if (g_config)
  {
    size_t i;
    for (i = 0; i < g_config->n_plugin; i++)
    {
      if (!strncmp(g_config->plugin[i]->dir, dir, PATH_MAX))
      {
        plugin = g_config->plugin[i];
        break;
      }
    }
  }
  return plugin;
}

/**
 *
 */
static void plugins_remove_unused(char* plugin_dir)
{
  DIR *d = opendir(plugin_dir);
  if (d)
  {
    struct dirent *p;
    while ((p = readdir(d)) != NULL)
    {
      // Skip the names "." and ".." as we don't want to recurse on them.
      if (!strncmp(p->d_name, ".", PATH_MAX) || !strncmp(p->d_name, "..", PATH_MAX))
      {
        continue;
      }

      size_t len = strnlen(plugin_dir, PATH_MAX) + strnlen(p->d_name, PATH_MAX) + 2;
      char *buf = malloc(len);
      if (buf)
      {
        struct stat statbuf;
        snprintf(buf, len, "%s/%s", plugin_dir, p->d_name);
        if (!stat(buf, &statbuf))
        {
          int r = 0;
          if (S_ISDIR(statbuf.st_mode))
          {
            Iddd__ConfigPlugin__Plugin* cfg = plugins_get(p->d_name);
            // Remove the directory if not an active plugin or plugin will not store data.
            if (!cfg || !plugins_store_data(cfg))
            {
              IDD_LOGD("Removing inactive plugin dir <%s>", buf);
              r = remove_directory(buf, 1);
            }
          }
          else
          {
            IDD_LOGE("Removing unexpect file in plugin dir <%s>", buf);
            r = unlink(buf);
          }
          if (r != 0)
          {
            IDD_LOGE("Failed to remove %s: %d <%s>", buf, errno, strerror(errno));
          }
        }
        else
        {
          IDD_LOGE("stat(%s) failed, <%s>", buf, strerror(errno));
        }
        free(buf);
      }
    }
    closedir(d);
  }
}

/**
 * Check and fix the given plugins active report.
 */
static void plugins_rescue_report(plugin_t* plugin, Iddd__ConfigPlugin__Plugin* cfg)
{
  if (plugins_scandir(plugin, cfg) == (-1))
  {
    IDD_LOGE("plugins_rescue_report: plugins_scandir() failed: %d <%s>",
              errno, strerror(errno));
    return;
  }
  if (plugin->current[0])
  {
    rescue_report(plugin->current,
                      cfg->report_header ? REPORT_WITH_HEADER : REPORT_WITHOUT_HEADER);
  }
}

/**
 *
 */
static void plugins_free()
{
  int nr_plugins = 0;
  if (g_config)
  {
    nr_plugins = g_config->n_plugin;
    iddd__config_plugin__free_unpacked(g_config, NULL);
    g_config = NULL;
  }

  if (g_plugins)
  {
    int i;
    for (i = 0; i < nr_plugins; i++)
    {
      filter_shutdown(g_plugins[i].filter);
    }
    free(g_plugins);
    g_plugins = NULL;
  }
}

/**
 * Unpack the IDD plugins from the IDD configuration (defined in configuration.proto).
 *
 * @param buf Plugins protobuf
 * @param buf_size Size of buf'
 */
static void plugins_load_configuration_buffer(const unsigned char* buf, size_t buf_size)
{
  plugins_free();
  g_config = iddd__config_plugin__unpack(NULL, buf_size, buf);

  plugins_remove_unused(PLUGINS_DIR);
  plugins_remove_unused(PLUGINS_RCA_DIR);
  if (g_config)
  {
    IDD_LOGI("Load %zu plugin", g_config->n_plugin);
    size_t i = g_config->n_plugin * sizeof(plugin_t);
    g_plugins = malloc(i);
    memset(g_plugins, 0, i);
    for (i = 0; i < g_config->n_plugin; i++)
    {
      (void)filter_init(g_plugins[i].filter);
      char* plugin_base_dir = plugins_base_dir_get(g_config->plugin[i]);
      if ((mkdir(plugin_base_dir, PRIVATE_DIR_MODE) == -1) && (errno != EEXIST))
      {
        IDD_LOGE("Failed to create plugin output base directory: %s", strerror(errno));
        continue;
      }
      snprintf(g_plugins[i].dir, PATH_MAX, "%s/%s", plugin_base_dir, g_config->plugin[i]->dir);
      if (mkdir(g_plugins[i].dir, PRIVATE_DIR_MODE) == -1)
      {
        if (errno == EEXIST)
        {
          plugins_rescue_report(&g_plugins[i], g_config->plugin[i]);
        }
        else
        {
          IDD_LOGE("Failed to create plugin output directory for %s: %s",
                    g_plugins[i].dir, strerror(errno));
          continue;
        }
      }
      filter_create(g_config->plugin[i]->probe_filter, g_plugins[i].filter);
    }
    IDD_LOGD("Load end");
  }
}

/**
 *
 */
void plugins_load_configuration()
{
  FILE* fp = config_open_file(PLUGINS_PROBE_PACKAGE, PLUGINS_PROBE_NAME);
  if (fp != NULL)
  {
    size_t file_size;
    unsigned char* buf_p = read_entire_file(fp, &file_size);
    fclose(fp);
    if (buf_p != NULL)
    {
      plugins_load_configuration_buffer(buf_p, file_size);
      free(buf_p);
    }
  }
}

/**
 *
 */
int plugins_max_size_get()
{
  int max_size_tot = 0;
  if (g_config)
  {
    size_t i;
    for (i = 0; i < g_config->n_plugin; i++)
    {
      if (!plugins_rca_plugin(g_config->plugin[i]))
      {
        max_size_tot += g_config->plugin[i]->max_output_dir_size;
      }
    }
  }
  return max_size_tot;
}

/**
 *
 */
void plugins_data_collection_enabled(int enabled)
{
  if (!enabled && g_config)
  {
    size_t i;
    for (i = 0; i < g_config->n_plugin; i++)
    {
      if (!plugins_get_ignore_idd_enabled(g_config->plugin[i]))
      {
        (void)remove_directory(g_plugins[i].dir, 0);
        g_plugins[i].current[0] = '\0'; // To force a new report
      }
    }
  }
}

/**
 *
 */
void plugins_flush()
{
  if (g_config)
  {
    size_t i;
    for (i = 0; i < g_config->n_plugin; i++)
    {
      if (g_config->plugin[i]->report_header && g_plugins[i].current[0])
      {
        plugins_compress(&g_plugins[i], g_config->plugin[i]);
      }
    }
  }
}
