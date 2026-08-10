/*
 * Copyright 2011 Sony Corporation
 */

/**
 * @brief IDD Configuration Parse Handling
 *
 * There are many configurations. This is about to parse configuration
 * are saved as one file per section.
 *
 * @author Fengjuan Yao (fengjuan3.yao@sonyericsson.com)
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "../iddd/protos/common.pb-c.h"
#include "../iddd/protos/configuration.pb-c.h"
#include "../iddd/protos/idd/iddd.pb-c.h"
#include "../api/idd_api.h"
#include "protos/agent.pb-c.h"

int get_file_size(FILE* in_fp, size_t* size_p);

void printf_table_head(char* title)
{
  printf("=================================================================================\n");
  printf("%s:\n", title);
  printf("*********************************************************************************\n");
}

void printf_table_tail()
{
  printf("*********************************************************************************\n\n");
}

unsigned char* read_entire_file(int fd, size_t* size_p)
{
  unsigned char* buf_p = NULL;
  FILE* fp = fdopen(fd, "rb");

  if (fp == NULL)
  {
    close(fd);
  }
  else
  {
    if (get_file_size(fp, size_p) == 0)
    {
      buf_p = malloc(*size_p);
      if (buf_p != NULL)
      {
        if (fread((char*)buf_p, 1, *size_p, fp) != *size_p)
        {
          free(buf_p);
          buf_p = NULL;
        }
      }
    }
    fclose(fp);
  }
  return buf_p;
}

/*
 * Unpack idd configuration protobuf file
 * part -- FilterConfig and print to stdout.
 */
static void parse_filter_config(int config_fd)
{
  printf_table_head("FilterConfig");
  if (config_fd != -1)
  {
    size_t file_size;
    unsigned char* buf_p = read_entire_file(config_fd, &file_size);
    if (buf_p != NULL)
    {
      Iddd__FilterConfig* fc_p = iddd__filter_config__unpack(NULL, file_size, buf_p);
      if ((fc_p != NULL) && (fc_p->event_filter != NULL))
      {
        if (fc_p->event_filter->type == IDDD__FILTER__TYPE__EXCLUDE)
        {
          printf("%s\t: %s\t\n", "event filter type", "EXCLUDE");
        }
        else
        {
          printf("%s\t: %s\t\n", "event filter type", "INCLUDE");
        }
        printf("%s\t: %zu\t\n", "event filter numbers", fc_p->event_filter->n_item);
        if (fc_p->event_filter->n_item > 0)
        {
          size_t i;
          for (i = 0; i < fc_p->event_filter->n_item; i++)
          {
            Iddd__FilterItem* ef_item_p = fc_p->event_filter->item[i];
            if ((ef_item_p == NULL) || (ef_item_p->package == NULL)|| (ef_item_p->name == NULL))
            {
              printf("\nInvalid event probe filter!\n");
            }
            else
            {
              printf("name: %s\t package: %s\t\n", ef_item_p->name, ef_item_p->package);
            }
          }
        }
      }

      if ((fc_p != NULL) && (fc_p->counter_filter != NULL))
      {
        if (fc_p->counter_filter->type == IDDD__FILTER__TYPE__EXCLUDE)
        {
          printf("%s\t: %s\t\n", "counter filter type", "EXCLUDE");
        }
        else
        {
          printf("%s\t: %s\t\n", "counter filter type", "INCLUDE");
        }
        printf("%s\t: %zu\t\n", "counter filter numbers", fc_p->counter_filter->n_item);
        if (fc_p->counter_filter->n_item > 0)
        {
          size_t i;
          for (i = 0; i < fc_p->counter_filter->n_item; i++)
          {
            Iddd__FilterItem* ef_item_p = fc_p->counter_filter->item[i];
            if ((ef_item_p == NULL) || (ef_item_p->package == NULL)|| (ef_item_p->name == NULL))
            {
              printf("\nInvalid counter probe filter!\n");
            }
            else
            {
              printf("name: %s\t package: %s\t\n", ef_item_p->name, ef_item_p->package);
            }
          }
        }
      }

      iddd__filter_config__free_unpacked(fc_p, NULL);
      free(buf_p);
    }
  }
  else
  {
    printf("Loading FilterConfig, failed, no active files\n");
  }
  printf_table_tail();
}

/*
 * Unpack idd configuration protobuf file
 * part -- RunActions and print to stdout.
 */
static void parse_runactions_config(int config_fd)
{
  printf_table_head("RunActionsConfig");
  if (config_fd != -1)
  {
    size_t file_size;
    unsigned char* buf_p = read_entire_file(config_fd, &file_size);
    if (buf_p != NULL)
    {
      Iddd__RunActions* ra_p = iddd__run_actions__unpack(NULL, file_size, buf_p);
      if ((ra_p != NULL))
      {
        if ((ra_p->has_run_log_reader == 1) && (ra_p->run_log_reader == 1))
        {
          printf("%s\t\t\t: %s\n", "run_log_reader", "true");
        }
        else
        {
          printf("%s\t\t\t: %s%s\n", "run_log_reader", "false", (ra_p->has_run_log_reader == 0) ? " (default)" : "");
        }

        if ((ra_p->has_activate_xperia_diagnostics == 1) &&
            (ra_p->activate_xperia_diagnostics == 1))
        {
          printf("%s\t: %s\n", "activate_xperia_diagnostics", "true");
        }
        else
        {
          printf("%s\t: %s%s\n", "activate_xperia_diagnostics", "false",
                 (ra_p->has_activate_xperia_diagnostics == 0) ? " (default)" : "");
        }
      }
      iddd__run_actions__free_unpacked(ra_p, NULL);
      free(buf_p);
    }
  }
  else
  {
    printf("Loading RunActions Config, failed, no active files\n");
  }
  printf_table_tail();
}


/*
 * Unpack idd configuration protobuf file
 * part -- RunActionsOnce and print to stdout.
 */
static void parse_runactionsonce_config(int config_fd)
{
  printf_table_head("RunActionsOnceConfig");
  if (config_fd != -1)
  {
    size_t file_size;
    unsigned char* buf_p = read_entire_file(config_fd, &file_size);
    if (buf_p != NULL)
    {
      Iddd__RunActionsOnce* rao_p = iddd__run_actions_once__unpack(NULL, file_size, buf_p);
      if ((rao_p != NULL))
      {
        if (rao_p->has_remove_reports == 1)
        {
          if (rao_p->remove_reports == 1)
          {
            printf("%s\t\t: %s\n", "remove_reports", "true");
          }
          else
          {
            printf("%s\t\t: %s\n", "remove_reports", "false");
          }
        }
        else
        {
          printf("%s\t: %s\n", "remove_reports", "config not include(optional key)");
        }

      }
      iddd__run_actions_once__free_unpacked(rao_p, NULL);
      free(buf_p);
    }
  }
  else
  {
    printf("Loading RunActionsOnce Config, failed, no active files\n");
  }
  printf_table_tail();
}

/*
 * Unpack idd configuration protobuf file
 * part -- Iddd settings and print to stdout.
 */
static void parse_iddd_settings_config(int config_fd)
{
  printf_table_head("Iddd settings");
  if (config_fd != -1)
  {
    size_t file_size;
    unsigned char* buf_p = read_entire_file(config_fd, &file_size);
    if (buf_p != NULL)
    {
      Iddd__Settings* is_p = iddd__settings__unpack(NULL, file_size, buf_p);
      if ((is_p != NULL))
      {
        if (is_p->has_max_report_size == 1)
        {
          printf("%s\t: %d\t\n", "max_report_size", is_p->max_report_size);
        }
        else
        {
          printf("%s\t: %s\t\n", "max_report_size", "config not include(optional key)");
        }
        if (is_p->has_max_output_dir_size == 1)
        {
          printf("%s\t: %d\t\n", "max_output_dir_size", is_p->max_output_dir_size);
        }
        else
        {
          printf("%s\t: %s\t\n", "max_output_dir_size", "config not include(optional key)");
        }
        if (is_p->has_permanently_disable == 1)
        {
          printf("%s\t: %d\t\n", "permanently_disable", is_p->permanently_disable);
        }
        else
        {
          printf("%s\t: %s\t\n", "permanently_disable", "config not include(optional key)");
        }
        if (is_p->has_expire_counters_age == 1)
        {
          printf("%s\t: %d\t\n", "expire_counters_age", is_p->expire_counters_age);
        }
        else
        {
          printf("%s\t: %s\t\n", "expire_counters_age", "config not include(optional key)");
        }
        if (is_p->has_expire_sdata_age == 1)
        {
          printf("%s\t: %d\t\n", "expire_sdata_age", is_p->expire_sdata_age);
        }
        else
        {
          printf("%s\t: %s\t\n", "expire_sdata_age", "config not include(optional key)");
        }
      }
      iddd__settings__free_unpacked(is_p, NULL);
      free(buf_p);
    }
  }
  else
  {
    printf("Loading Iddd settings Config, failed, no active files\n");
  }
  printf_table_tail();
}

/*
 * Unpack idd configuration protobuf file
 * part -- AgentConfig and print to stdout.
 */
static void parse_agent_config(int config_fd)
{
  printf_table_head("AgentConfig");
  if (config_fd != -1)
  {
    size_t file_size;
    unsigned char* buf_p = read_entire_file(config_fd, &file_size);
    if (buf_p != NULL)
    {
      Idd__AgentConfig* ac_p = idd__agent_config__unpack(NULL, file_size, buf_p);
      if (ac_p != NULL)
      {
        if (ac_p->n_config_servers > 0)
        {
          printf("%s\t\t:\n", "config_servers");
          size_t i;
          for (i = 0; i < ac_p->n_config_servers; i++)
          {
            Idd__AgentConfig__Server* cs_item_p = ac_p->config_servers[i];
            if ((cs_item_p == NULL) || (cs_item_p->url == NULL))
            {
              printf("Invalid config server!\n");
            }
            else
            {
              printf("    %s\n", cs_item_p->url);
            }
          }
        }
        if (ac_p->n_download_servers > 0)
        {
          printf("%s\t:\n", "download_servers");
          size_t i;
          for (i = 0; i < ac_p->n_download_servers; i++)
          {
            Idd__AgentConfig__Server* ds_item_p = ac_p->download_servers[i];
            if ((ds_item_p == NULL) || (ds_item_p->url == NULL))
            {
              printf("Invalid download server!\n");
            }
            else
            {
              printf("    %s\n", ds_item_p->url);
            }
          }
        }
        if (ac_p->n_gatekeeper_servers > 0)
        {
          printf("%s\t:\n", "gatekeeper_servers");
          size_t i;
          for (i = 0; i < ac_p->n_gatekeeper_servers; i++)
          {
            Idd__AgentConfig__Server* gs_item_p = ac_p->gatekeeper_servers[i];
            if ((gs_item_p == NULL) || (gs_item_p->url == NULL))
            {
              printf("Invalid gatekeeper server!\n");
            }
            else
            {
              printf("    %s\n", gs_item_p->url);
            }
          }
        }
        if (ac_p->n_report_servers > 0)
        {
          printf("%s\t\t:\n", "report_servers");
          size_t i;
          for (i = 0; i < ac_p->n_report_servers; i++)
          {
            Idd__AgentConfig__Server* rs_item_p = ac_p->report_servers[i];
            if ((rs_item_p == NULL) || (rs_item_p->url == NULL))
            {
              printf("Invalid report server!\n");
            }
            else
            {
              printf("    %s\n", rs_item_p->url);
            }
          }
        }
        if (ac_p->has_config_period == 1)
        {
          printf("%s\t: %d\t\n", "config_period(h)", ac_p->config_period/3600);
        }
        else
        {
          printf("%s\t: %s\t\n", "config_period", "config not include(optional key)");
        }
        if (ac_p->has_report_max_size == 1)
        {
          printf("%s\t\t: %d\t\n", "upload_limit(K)", ac_p->report_max_size/1024);
        }
        else
        {
          printf("%s\t: %s\t\n", "upload_limit", "config not include(optional key)");
        }
        if (ac_p->has_report_period == 1)
        {
          printf("%s\t: %d\t\n", "report_period(h)", ac_p->report_period/3600);
        }
        else
        {
          printf("%s\t: %s\t\n", "report_period", "config not include(optional key)");
        }
      }
      idd__agent_config__free_unpacked(ac_p, NULL);
      free(buf_p);
    }
  }
  else
  {
    printf("Loading AgentConfig, failed, no active files\n");
  }
  printf_table_tail();
}

/*
 * Unpack idd configuration protobuf file
 * part -- LogReader and print to stdout.
 */
static void parse_logreader_config(int config_fd, int logreader_num)
{
  if (logreader_num == 1)
  {
    printf_table_head("LogReaderConfig_1");
  }
  else
  {
    printf_table_head("LogReaderConfig_2");
  }
  if (config_fd != -1)
  {
    size_t file_size;
    unsigned char* buf_p = read_entire_file(config_fd, &file_size);
    if (buf_p != NULL)
    {
      printf("%s", buf_p);
    }
    free(buf_p);
  }
  else
  {
    printf("Loading LogReaderConfig, failed, no active files\n");
  }
  printf_table_tail();
}

void parse_idd_config(IDD_t* idd)
{
  int fd = idd_get_config(idd, "iddd", "FilterConfig");
  if (fd >= 0)
  {
    parse_filter_config(fd);
  }
  fd = idd_get_config(idd, "iddd", "RunActions");
  if (fd >= 0)
  {
    parse_runactions_config(fd);
  }
  fd = idd_get_config(idd, "iddd", "RunActionsOnce");
  if (fd >= 0)
  {
    parse_runactionsonce_config(fd);
  }
  fd = idd_get_config(idd, "idd", "AgentConfig");
  if (fd >= 0)
  {
    parse_agent_config(fd);
  }
  fd = idd_get_config(idd, "idd", "LogReader");
  if (fd >= 0)
  {
    parse_logreader_config(fd, 1);
  }

  // LogReader2 is introduced by: add system logcat
  // in the logreader but the old sw can't handle it.
  fd = idd_get_config(idd, "idd", "LogReader2");
  if (fd >= 0)
  {
    parse_logreader_config(fd, 2);
  }
  fd = idd_get_config(idd, "iddd", "Settings");
  if (fd >= 0)
  {
    parse_iddd_settings_config(fd);
  }
}

void parse_config_request(unsigned char* cr, int cr_size)
{
  if (cr != NULL)
  {
    Idd__Config__ConfigRequest* cr_p = idd__config__config_request__unpack(NULL, cr_size, cr);
    if ((cr_p != NULL) && (cr_p->header != NULL))
    {
      Idd__Header *hr = cr_p->header;
      printf("\nproduct_id: %s", hr->product_id);
      printf("\nsoftware_version: %s", hr->software_version);
      printf("\nsoftware_variant: %s", hr->software_variant);
      printf("\ndevice_id report: %s", hr->device_id);
      printf("\nbuild_type: %s", hr->build_type);
      if (hr->has_family) printf("\nfamily: %d", hr->family);
      printf("\nconfig_checksum: %s", hr->config_checksum);
      printf("\nreport_uuid: %s", hr->report_uuid);
      if (hr->has_timezone) printf("\ntimezone: %d", hr->timezone);
      printf("\nCDF_id: %s", hr->cdf_id);
      printf("\nserial_number: %s", hr->serial_number);
      printf("\ndevice_imei: %s", hr->device_imei);
      printf("\ninternal_product_name: %s", hr->internal_product_name);
      printf("\nsony_product_code: %s", hr->sony_product_code);
      printf("\ncustomer_id: %s", hr->customer_id);

      size_t i;
      for (i = 0; i < hr->n_group_id; i++)
      {
        printf("\n group_id[%zu]: %s", i, hr->group_id[i]);
      }
      printf("\nsecure_mode: %d", hr->secure_mode);
      if (cr_p->device_id) printf("\ndevice_id request: %s", cr_p->device_id);
      printf("\n");
    }
  }
}
