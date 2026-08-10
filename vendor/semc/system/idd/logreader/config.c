/*
 * Copyright 2014 Sony Corporation
 */

/**
 * @brief Handles generic configuration for idd-logreader.
 *
 * @author Leif Martensson (leif.martensson@sonymobile.com)
 */

#include <stdlib.h>
#include "../api/idd_api.h"
#define LOG_TAG "idd-logreader"
#include "../iddd/logging.h"
#include "idd/logreader_configuration.pb-c.h"
#include "util.h"
#include "rules.h"
#include "config.h"

/**
 * The idd-logreader IDD configuration 'idd.system_logging:ConfigLogreader'
 */
#define CONFIGPART_LOGREADER_PKG "idd.system_logging"
#define CONFIGPART_LOGREADER_NAME "ConfigLogreader"


static Idd__SystemLogging__ConfigLogreader* g_conf = NULL;

/**
 *
 */
static void config_free()
{
  if (g_conf)
  {
    idd__system_logging__config_logreader__free_unpacked(g_conf, NULL);
    g_conf = NULL;
  }
}

/**
 *
 */
void config_read(IDD_t* idd_p)
{
  config_free();
  int fd = idd_get_config(idd_p, CONFIGPART_LOGREADER_PKG, CONFIGPART_LOGREADER_NAME);
  if (fd > -1)
  {
    size_t file_size;
    unsigned char* buf_p = read_entire_file(fd, &file_size);
    if (buf_p != NULL)
    {
      g_conf = idd__system_logging__config_logreader__unpack(NULL, file_size, buf_p);
      free(buf_p);
    }
  }
}

/**
 *
 */
void config_probes_add(struct Probe** probe)
{
  if (g_conf)
  {
    size_t i;
    for (i = 0; i < g_conf->n_log_probe; i++)
    {
      struct Probe* new_probe = create_probe(parse_rules_from_str(g_conf->log_probe[i]->rules),
                                             g_conf->log_probe[i]->plugin_id);
      if (new_probe)
      {
        probe_add(probe, new_probe);
      }
    }
  }
}
