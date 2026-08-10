/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief In Device Diagnostics (Test) Client
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include "idd_config.h"
#include <idd_api.h>
#include "idd_private_api.h"
#include "iddd_protos/common.pb-c.h"
#include "iddconfigparser.h"
#include "envelope.h"

static const char* idd_path = IDD_DEFAULT_BASE_DIR;

#define CONFIG_REQUEST_RESPONSE_MAX_LEN 1024

static void die(const char* errmsg)
{
  puts(errmsg);
  exit(1);
}

size_t read_file(const char* fname_p, char* buf, size_t max)
{
  size_t read_items = 0;
  FILE* fp = fopen(fname_p, "rb");

  if (fp != NULL)
  {
    read_items = fread(buf, 1, max, fp);
    fclose(fp);
  }
  return read_items;
}

static EVP_PKEY* get_private_key(const char* filename_p)
{
  FILE* fp;
  EVP_PKEY* key_p = NULL;

  fp = fopen(filename_p, "rb");
  if (fp != NULL)
  {
    key_p = PEM_read_PrivateKey(fp, &key_p, NULL, NULL);
    fclose(fp);
  }

  return key_p;
}

int decrypt(const char* privkey_p, const char* data_p, const char* enckey_p, const char* iv_p)
{
  char key_buf[IDD_MAX_PUBLIC_KEY_SIZE / 8] = {0};
  char iv_buf[IDD_SYMMETRIC_KEY_SIZE / 8] = {0};
  size_t key_len, iv_len;
  EVP_PKEY* priv_key_p;
#define BUFFER_SIZE 1024
  char outbuf[BUFFER_SIZE] = {0};
  int outlen;
  EVP_CIPHER_CTX ctx;

  key_len = read_file(enckey_p, key_buf, sizeof(key_buf));
  if ((int)key_len <= 0)
  {
    printf("Failed to open file: %s, %s\n", enckey_p, strerror(errno));
    return EXIT_FAILURE;
  }

  iv_len = read_file(iv_p, iv_buf, sizeof(iv_buf));
  if ((int)iv_len <= 0)
  {
    printf("Failed to open file: %s, %s\n", iv_p, strerror(errno));
    return EXIT_FAILURE;
  }

  FILE* in_fp = fopen(data_p, "rb");

  if (in_fp == NULL)
  {
    printf("Failed to open file: %s, %s\n", data_p, strerror(errno));
    return EXIT_FAILURE;
  }

  priv_key_p = get_private_key(privkey_p);

  if (priv_key_p != NULL)
  {
    EVP_CIPHER_CTX_init(&ctx);
    int result = open_init(&ctx, EVP_aes_128_cbc(),
                           (unsigned char*)key_buf, key_len,
                           (unsigned char*)iv_buf, priv_key_p);

    EVP_PKEY_free(priv_key_p);

    if (result == 0)
    {
      printf("Failed to initialize EVP_OpenInit, %s\n", strerror(errno));
      fclose(in_fp);
      return EXIT_FAILURE;
    }

    for (;;)
    {
      char inbuf[BUFFER_SIZE] = {0};
      int inlen = (int)fread(inbuf, 1, sizeof(inbuf), in_fp);
      if (inlen <= 0)
      {
        break;
      }

      if (!open_update(&ctx, (unsigned char*)outbuf,
                       &outlen, (unsigned char*)inbuf,
                       inlen))
      {
        printf("Failed to decrypt data\n");
        fclose(in_fp);
        return EXIT_FAILURE;
      }
      if ((int)fwrite(outbuf, 1, outlen, stdout) < outlen)
      {
        printf("decrypt(): fwrite() failed, %s\n", strerror(errno));
      }
    }

    if (!open_final(&ctx, (unsigned char*)outbuf, &outlen))
    {
      fclose(in_fp);
      return 0;
    }

    if ((int)fwrite(outbuf, 1, outlen, stdout) < outlen)
    {
      printf("decrypt(): fwrite() failed, %s\n", strerror(errno));
    }
  }

  fclose(in_fp);
  return EXIT_SUCCESS;
}

/*
 * Returns the file size of the open file descriptor.
 * The file position will be set to the start of the file.
 *
 * Returns 0 on success, -1 on error.
 */
int get_file_size(FILE* in_fp, size_t* size_p)
{
  if (size_p == NULL)
  {
    return -1;
  }

  *size_p = 0;
  if (fseek(in_fp, 0, SEEK_END) == 0)
  {
    long size = ftell(in_fp);
    if (fseek(in_fp, 0, SEEK_SET) == 0)
    {
      *size_p = (size_t)size;
      return 0;
    }
  }
  return -1;
}

static void print_idd_failure(int rescode, const char *func)
{
  printf("%s failed, rescode=%d, %s\n", func, rescode, strerror(-rescode));
}

int main(int argc, char* argv[])
{
  IDD_t* idd;
  int raw_mode = 0;
  int option_index = 0;
  enum {NONE, INC_COUNTER, SET_COUNTER, SET_SDATA, FLUSH, SET_IMEI,
        SET_SN, SET_GROUP_ID, DELETE_GROUP_ID, REPLACE_GROUP_ID,
        SET_DATA_COLLECTION, REMOVE_REPORT, PUMP_EVENTS,
        PUMP_COUNTERS, PUMP_SDATA, LOAD_CONFIG, PATCH_FILES,
        PROBE_ACTIVE, GET_CONFIG, PRINT_IDD_CONFIG, DECRYPT,
        CRYPTO_SPLIT, IDD_ACTIVE, 
        FILE_ITERATOR_NEXT, FILE_GET, // deprecated
        CONFIG_REQUEST_GET, GET_MAX_PAYLOAD, ADD_EVENT, DO_WASHING,
        SET_ANDROID_ID, SET_TIMEZONE} command = NONE;
  unsigned int pump_cnt = 0;
  unsigned int pump_size = 0;
  unsigned int pump_rate = 0;
  int c;
  static const struct option long_options[] =
    {
      {"verbose", no_argument,       0, 'v'},
      {"raw",     no_argument,       0, 'r'},
      {"sock",    required_argument, 0, 's'},
      {"help",    no_argument,       0, 'h'},
      {"inc-counter", no_argument,   0, 'c'},
      {"set-counter", no_argument,   0, 'C'},
      {"set-sdata", no_argument,     0, 'd'},
      {"flush",     no_argument,     0, 'f'},
      {"set-imei",    no_argument,   0, 'i'},
      {"set-serial-number",    no_argument,   0, 'n'},
      {"remove-report",    no_argument,   0, 'R'},
      {"load-config", no_argument,   0, 'l'},
      {"active",    no_argument,     0, 'a'},
      {"get-config", no_argument,    0, 'g'},
      {"print-idd-config", no_argument,    0, 'p'},
      {"is-active", no_argument,     0, 'e'},
      {"get-max-payload", no_argument, 0, 'm'},
      /* Hidden commands. Only used for test suite */
      {"set-data-collection",    no_argument,   0, 'D'},
      {"set-group-id", no_argument, 0, SET_GROUP_ID},
      {"delete-group-id", no_argument, 0, DELETE_GROUP_ID},
      {"replace-group-id", no_argument, 0, REPLACE_GROUP_ID},
      {"pump-events", required_argument, 0, PUMP_EVENTS},
      {"pump-counters", required_argument, 0, PUMP_COUNTERS},
      {"pump-sdata", required_argument, 0, PUMP_SDATA},
      {"patch-output-files", no_argument, 0, PATCH_FILES},
      {"decrypt", no_argument, 0, DECRYPT},
      {"crypto-split", no_argument, 0, CRYPTO_SPLIT},
      {"config-request-get", no_argument, 0, 'q'},
      {"add-event", no_argument, 0, ADD_EVENT},
      {"do-washing", no_argument, 0, DO_WASHING},
      {"set-android-id", no_argument, 0, SET_ANDROID_ID},
      {"set-timezone", no_argument, 0, SET_TIMEZONE},
      {0, 0, 0, 0}
    };

  while ((c = getopt_long (argc, argv, "vrs:hcCdfuinDRlagpemq",
                           long_options, &option_index)) != -1)
  {
    switch(c) {
    case 's':
      idd_path = strdup(optarg);
      break;
    case 'r':
      raw_mode = 1;
      break;
    case 'h':
      printf("Syntax: iddc [OPTIONS]\n"
             "\n"
             "  -h, --help                        give this help list\n"
             "  -s  <server_path>                 use specified server path\n"
             "  -c, --inc-counter                 increase counter\n"
             "  -C, --set-counter                 set counter\n"
             "  -d, --set-sdata                   set static data\n"
             "  -f, --flush                       flush data\n"
             "  -i, --set-imei                    set IMEI\n"
             "  -n, --set-serial-number           set serial number\n"
             "  -R, --remove-report               remove report\n"
             "  -l, --load-config                 load new configuration\n"
             "  -r, --raw                         raw mode (reads from stdin)\n"
             "  -v, --verbose                     verbose output\n"
             "  -a, --active                      check if probe is active\n"
             "  -g, --get-config                  return configuration\n"
             "  -p, --print-idd-config            print idd configuration\n"
             "  -e, --is-active                   check if IDD is active\n"
             "  -m, --get-max-payload             get max IDD event payload\n"
             "  --do-washing                      Washing personal data?\n"
             "  --set-android-id                  set ANDROID_ID\n"
             "  --set-timezone                    set timezone\n"
#if 0
             /* Hidden commands: */
             "  --file-iterator-next client-id file_iterator    print next (first) file iterator for given client\n"
             "  --file-iterator-get client-id file_iterator     read file for given client\n"
             "  --set-group-id                    set group id\n"
             "  --delete-group-id                 delete group id\n"
             "  --replace-group-id                replace group id\n"
             "  -D, --set-data-collection         enable/disable data collection\n"
             "  --pump-events <cnt/size/rate> [package] [name]    pump events. \n"
             "  --pump-counters <cnt/size/rate> [package] [name]  pump counters. \n"
             "  --pump-sdata <cnt/size/rate> [package] [name]     pump static data. \n"
             "      <cnt>      number of events/counters/sdata to send.\n"
             "      <size>     size of each event (not used for the rest).\n"
             "      <rate>     ms to sleep between runs.\n"
             "                 e.g. 500/1000/30 will produce around 500kb of\n"
             "                 events and they will be sent during 15 seconds\n"

             "  --add-event [filename]            add event with payload from given file\n"
             "  --decrypt <privkey> <data> <enckey> <iv> decrypt file using parts\n"
             "  --crypto-split <privkey> <data>   splits input file to crypto_split.* files.\n"
             "  -q --config-request-get [filename]   Performs a config_request call to iddd, puts"
             "                                       the result in the given file (optional).\n"
#endif
             "\n");
      exit(0);
      break;
    case 'c': command = INC_COUNTER; break;
    case 'C': command = SET_COUNTER; break;
    case 'd': command = SET_SDATA;   break;
    case 'f': command = FLUSH;       break;
    case 'i': command = SET_IMEI;    break;
    case 'n': command = SET_SN;    break;
    case 'D': command = SET_DATA_COLLECTION;    break;
    case 'R': command = REMOVE_REPORT;    break;
    case 'l': command = LOAD_CONFIG; break;
    case 'a': command = PROBE_ACTIVE; break;
    case 'g': command = GET_CONFIG;  break;
    case 'p': command = PRINT_IDD_CONFIG; break;
    case 'e': command = IDD_ACTIVE;  break;
    case 'q': command = CONFIG_REQUEST_GET;  break;
    case 'm': command = GET_MAX_PAYLOAD;  break;
    case PUMP_EVENTS:
    case PUMP_COUNTERS:
    case PUMP_SDATA:
      // The rate of pump will be counted as microsecond
      if (sscanf(optarg, "%8u/%8u/%8u", &pump_cnt, &pump_size, &pump_rate) != 3)
      {
        printf("Bad syntax for --pump-[events/counters/sdata]\n");
        exit(1);
      }
      if (pump_size > IDD_MAX_EVENT_SIZE)
      {
        printf("Bad event size - %u requested, max is %d.\n", pump_size, IDD_MAX_EVENT_SIZE);
        exit(1);
      }
      command = c;
      break;
    case PATCH_FILES:
    case SET_GROUP_ID:
    case DELETE_GROUP_ID:
    case REPLACE_GROUP_ID:
    case DECRYPT:
    case CRYPTO_SPLIT:
    case CONFIG_REQUEST_GET:
    case GET_MAX_PAYLOAD:
    case ADD_EVENT:
    case DO_WASHING:
    case SET_ANDROID_ID:
    case SET_TIMEZONE:
      command = c;
      break;
    }
  }

  if (command == CRYPTO_SPLIT)
  {
    // Crude. Read everything into memory, parse it (creates a copy), then save all parts.
    FILE* in_fp = fopen(argv[optind], "rb");
    if (in_fp == NULL)
    {
      printf("Failed to open file: %s, %s\n", argv[optind], strerror(errno));
      exit(EXIT_FAILURE);
    }
    size_t file_size;
    (void)get_file_size(in_fp, &file_size);
    char* buf_p = malloc(file_size);
    if (fread(buf_p, 1, file_size, in_fp) < file_size)
    {
      printf("Failed to read file: %s, %s\n", argv[optind], strerror(errno));
    }
    fclose(in_fp);
    Idd__Report* report_p = idd__report__unpack(NULL, file_size, (unsigned char*)buf_p);
    if (report_p != NULL)
    {
      FILE *fp_tmp = fopen("crypto_split.key", "wb");
      printf("key: %zu bytes\n", report_p->encryption_key.len);
      if (fwrite(report_p->encryption_key.data, 1, report_p->encryption_key.len, fp_tmp) < report_p->encryption_key.len)
      {
        printf("decrypt(): fwrite() failed, %s\n", strerror(errno));
      }
      fclose(fp_tmp);

      printf("iv: %zu bytes\n", report_p->encryption_iv.len);
      fp_tmp = fopen("crypto_split.iv", "wb");
      if (fwrite(report_p->encryption_iv.data, 1, report_p->encryption_iv.len, fp_tmp) < report_p->encryption_iv.len)
      {
        printf("decrypt(): fwrite() failed, %s\n", strerror(errno));
      }
      fclose(fp_tmp);

      printf("data: %zu bytes\n", report_p->encrypted_compressed_data.len);
      fp_tmp = fopen("crypto_split.data", "wb");
      if (fwrite(report_p->encrypted_compressed_data.data, 1, report_p->encrypted_compressed_data.len, fp_tmp) < report_p->encrypted_compressed_data.len)
      {
        printf("decrypt(): fwrite() failed, %s\n", strerror(errno));
      }
      fclose(fp_tmp);
    }
    exit(EXIT_SUCCESS);
  }
  else if (command == DECRYPT)
  {
    const char* privkey_p = (optind < argc) ? argv[optind++] : NULL;
    const char* data_p = (optind < argc) ? argv[optind++] : NULL;
    const char* enckey_p = (optind < argc) ? argv[optind++] : NULL;
    const char* iv_p = (optind < argc) ? argv[optind++] : NULL;
    if (privkey_p == NULL || data_p == NULL || enckey_p == NULL || iv_p == NULL)
    {
      printf("Bad syntax: --decrypt <privkey> <data> <enckey> <iv>.\n");
      exit(EXIT_FAILURE);
    }
    exit(decrypt(privkey_p, data_p, enckey_p, iv_p));
  }
  else if (command == PATCH_FILES)
  {
    int i;
    argc -= optind;
    argv += optind;
    for (i = 0; i < argc; i++)
    {
      int patched = 0;
      FILE* fp = fopen(argv[i], "r+b");
      printf("Patching %s...", argv[i]);
      if (fp != NULL)
      {
        unsigned char signature[2] = {0};
        // Verify that it's a gzipped file.
        if (fread(signature, 1, sizeof(signature), fp) == sizeof(signature))
        {
          if (signature[0] == 0x1f && signature[1] == 0x8b)
          {
            // The XFL field is at offset 8 and should be 0.
            if (fseek(fp, 8, SEEK_SET) == 0)
            {
              char no_options[1] = {0};
              if (fwrite(no_options, 1, sizeof(no_options), fp) == 1)
              {
                patched = 1;
                printf("[OK].\n");
              }
            }
          }
        }
        if (!patched)
        {
          printf("[NOPE].\n");
        }
        fclose(fp);
      }
    }
    exit(EXIT_SUCCESS);
  }

  idd = idd_create_from_path(idd_path);
  if (!idd)
  {
    die("Failed to create IDD instance");
  }

  if (raw_mode)
  {
    char buffer[IDD_MAX_PACKET_SIZE] = {0};
    while(fgets(buffer, sizeof(buffer), stdin))
    {
      size_t len = strlen(buffer);
      if (len > 0 && buffer[len - 1] == '\n')
        buffer[--len] = '\0';
      int rescode = idd_send_raw(idd, buffer, len);
      if (rescode < 0)
      {
        print_idd_failure(rescode, "idd_send_raw()");
      }
    }
  }
  else if (command == GET_CONFIG)
  {
    const char* package_p = (optind < argc) ? argv[optind++] : NULL;
    const char* name_p = (optind < argc) ? argv[optind++] : NULL;
    int fd = idd_get_config(idd, package_p, name_p);
    if (fd >= 0)
    {
      char buf[1024] = {0};
      for (;;)
      {
        ssize_t bytes = read(fd, buf, sizeof(buf));
        if (bytes == 0)
        {
          break;
        }
        if (write(STDOUT_FILENO, buf, bytes) == -1)
        {
          printf("write(stdout,,) failed, %s\n", strerror(errno));
        }
      }
      close(fd);
    }
    else if(fd == IDD_NO_CONFIG_AVAILABLE)
    {
      printf("No configuration file found.\n");
    }
    else
    {
      printf("Error while retrieving configuration file.\n");
    }
  }
  else if (command == PRINT_IDD_CONFIG)
  {
    parse_idd_config(idd);
  }
  else if (command == IDD_ACTIVE)
  {
    int active = idd_is_active(idd);
    printf(active > 0 ? "ACTIVE\n" : "not active\n");
    exit(active > 0 ? 0 : 9);
  }
  else if (command == PROBE_ACTIVE)
  {
    const char* package_p = (optind < argc) ? argv[optind++] : NULL;
    const char* name_p = (optind < argc) ? argv[optind++] : NULL;
    int active = idd_is_probe_active(idd, package_p, name_p);
    if (active > 0)
    {
      printf("ACTIVE\n");
    }
    else if (active == 0)
    {
      printf("not active\n");
    }
    else
    {
      printf("ERROR\n");
    }
  }
  else if ((command == SET_COUNTER) || command == INC_COUNTER)
  {
    const char* package_p = (optind < argc) ? argv[optind++] : NULL;
    const char* name_p = (optind < argc) ? argv[optind++] : NULL;
    const char* value_p = (optind < argc) ? argv[optind++] : NULL;
    if (command == SET_COUNTER)
    {
      if (package_p == NULL || name_p == NULL || value_p == NULL)
        die("Syntax: iddc --set-counter <package> <name> <value>");
      int rescode = idd_set_counter(idd, package_p, name_p, atoi(value_p));
      if (rescode < 0)
      {
        print_idd_failure(rescode, "idd_set_counter()");
      }
    }
    else
    {
      if (package_p == NULL || name_p == NULL)
        die("Syntax: iddc --inc-counter <package> <name> [<value>]");
      int rescode = idd_inc_counter(idd, package_p, name_p, value_p == NULL ? 1 : atoi(value_p));
      if (rescode < 0)
      {
        print_idd_failure(rescode, "idd_inc_counter()");
      }
    }
  }
  else if (command == SET_SDATA)
  {
    const char* package_p = (optind < argc) ? argv[optind++] : NULL;
    const char* name_p = (optind < argc) ? argv[optind++] : NULL;
    const char* value_p = (optind < argc) ? argv[optind++] : NULL;
    if (package_p == NULL || name_p == NULL || value_p == NULL)
      die("Syntax: iddc --set-sdata <package> <name> \"<value>\"");
    int rescode = idd_set_sdata(idd, package_p, name_p, value_p);
    if (rescode < 0)
    {
      print_idd_failure(rescode, "idd_set_sdata()");
    }
  }
  else if (command == FLUSH)
  {
    int rescode = idd_flush(idd);
    if (rescode < 0)
    {
      print_idd_failure(rescode, "idd_flush()");
    }
  }
  else if (command == SET_IMEI)
  {
    const char* imei_p = (optind < argc) ? argv[optind++] : NULL;
    if (imei_p == NULL)
    {
      die("Syntax: iddc --set-imei <IMEI>");
    }
    int rescode = idd_set_imei(idd, imei_p);
    if (rescode < 0)
    {
      print_idd_failure(rescode, "idd_set_imei()");
    }
  }
  else if (command == SET_SN)
  {
    const char* sn_p = (optind < argc) ? argv[optind++] : NULL;
    if (sn_p == NULL)
    {
      die("Syntax: iddc --set-serial-number <serial number>");
    }
    int rescode = idd_set_serial_number(idd, sn_p);
    if (rescode < 0)
    {
      print_idd_failure(rescode, "idd_set_serial_number()");
    }
  }
  else if (command == SET_DATA_COLLECTION)
  {
    const char* value_p = (optind < argc) ? argv[optind++] : NULL;
    if (value_p == NULL)
    {
      die("Syntax: iddc --set-data-collection 1/0");
    }
    int rescode = idd_set_data_collection_enable(idd, atoi(value_p));
    if (rescode < 0)
    {
      print_idd_failure(rescode, "idd_set_data_collection_enable()");
    }
  }
  else if (command == REMOVE_REPORT)
  {
    const char* value_p = (optind < argc) ? argv[optind++] : NULL;
    if (value_p == NULL)
    {
      die("Syntax: iddc --remove-report <report_number>");
    }
    int rescode = idd_remove_report(idd, atoi(value_p));
    if (rescode < 0)
    {
      print_idd_failure(rescode, "idd_remove_report()");
    }
  }
  else if (command == LOAD_CONFIG)
  {
    int rescode = idd_notify_config(idd);
    if (rescode < 0)
    {
      print_idd_failure(rescode, "idd_notify_config()");
    }
  }
  else if (command == SET_GROUP_ID)
  {
    const char* gi_p = (optind < argc) ? argv[optind++] : NULL;
    if (gi_p == NULL)
    {
      die("Syntax: iddc --set-group-id <group id>");
    }
    int rescode = idd_set_group_id(idd, gi_p);
    if (rescode < 0)
    {
      print_idd_failure(rescode, "idd_set_group_id()");
    }
  }
  else if (command == DELETE_GROUP_ID)
  {
    const char* del_gi_p = (optind < argc) ? argv[optind++] : NULL;
    if (del_gi_p == NULL)
    {
      die("Syntax: iddc --delete-group-id <group id>");
    }
    int rescode = idd_delete_group_id(idd, del_gi_p);
    if (rescode < 0)
    {
      print_idd_failure(rescode, "idd_delete_group_id()");
    }
  }
  else if (command == REPLACE_GROUP_ID)
  {
    const char* to_gi_p = (optind < argc) ? argv[optind++] : NULL;
    const char* from_gi_p = (optind < argc) ? argv[optind++] : NULL;
    if (to_gi_p == NULL || from_gi_p == NULL)
    {
      die("Syntax: iddc --replace-group-id <to group id> <from group id>");
    }
    int rescode = idd_replace_group_id(idd, to_gi_p, from_gi_p);
    if (rescode < 0)
    {
      print_idd_failure(rescode, "idd_replace_group_id()");
    }
  }
  else if ((command == PUMP_EVENTS) ||
           (command == PUMP_COUNTERS) ||
           (command == PUMP_SDATA))
  {
    unsigned int i;
    char payload_buffer[IDD_MAX_EVENT_SIZE] = {0};
    struct timespec time_req;
    const char* pump_package = (optind < argc) ? argv[optind++] : "iddtest";
    const char* pump_name_arg = (optind < argc) ? argv[optind++] : NULL;
    char pump_name_generate[40] = {0};
    const char* pump_name = (pump_name_arg ? pump_name_arg : pump_name_generate);

    time_req.tv_sec = pump_rate / (1000*1000);
    time_req.tv_nsec = (pump_rate % (1000*1000)) *1000;

    memset(payload_buffer, 0, sizeof(payload_buffer));
    for (i = 0; i < pump_cnt; i++)
    {
      if (!pump_name_arg)
      {
        snprintf(pump_name_generate, sizeof(pump_name_generate), "PumpEvent%d", i);
      }

      if (i > 0)
        nanosleep(&time_req, NULL);

      if (command == PUMP_EVENTS)
      {
        int rescode = idd_add_event(idd, pump_package, pump_name, payload_buffer, pump_size);
        if (rescode < 0)
        {
          print_idd_failure(rescode, "idd_add_event()");
        }
      }
      else if (command == PUMP_COUNTERS)
      {
        int rescode = idd_set_counter(idd, pump_package, pump_name, 10000+i);
        if (rescode < 0)
        {
          print_idd_failure(rescode, "idd_set_counter()");
        }
      }
      else if (command == PUMP_SDATA)
      {
        int rescode = idd_set_sdata(idd, pump_package, pump_name, pump_name + 4);
        if (rescode < 0)
        {
          print_idd_failure(rescode, "idd_set_sdata()");
        }
      }
    }
    printf("Done.\n");
  }
  else if (command == CONFIG_REQUEST_GET)
  {
    unsigned char cr[CONFIG_REQUEST_RESPONSE_MAX_LEN] = {0};
    int cr_len = idd_config_request_get(idd, (char *)cr, sizeof(cr));
    if (cr_len >= 0)
    {
      const char* filename_p = (optind < argc) ? argv[optind++] : NULL;
      printf("idd_config_request_get OK, config_request size = %d, output file=[%s]\n",
             cr_len, filename_p);
      if (filename_p == NULL)
      {
        parse_config_request(cr, cr_len);
      }
      else
      {
        FILE* fp = fopen(filename_p, "wb");
        if (fp)
        {
          fwrite(cr, cr_len, 1, fp);
          fclose(fp);
        }
        else
        {
          printf("Could not create file [%s]\n", filename_p);
        }
      }
    }
    else
    {
      printf("idd_config_request_get FAILURE, %d\n", cr_len);
    }
  }
  else if (command == GET_MAX_PAYLOAD)
  {
    printf("Max payload of IDD event = %d\n", idd_get_max_payload());
  }
  else if (command == ADD_EVENT)
  {
    if (optind >= argc)
    {
      die("Syntax: iddc --add-event filename");
    }
    int fd = open(argv[optind], O_RDONLY);
    if (fd >= 0)
    {
      size_t file_size;
      char *payload = (char*)read_entire_file(fd, &file_size);
      idd_add_event(idd, "iddtest", "AddEvent", payload, file_size);
    }
    else
    {
      printf("Could not open file [%s]\n", argv[optind]);
    }
  }
  else if (command == DO_WASHING)
  {
    printf("Do%s wash personal data\n", idd_do_washing(idd) == 0 ? " not" : "");
  }
  else if (command == SET_ANDROID_ID)
  {
    const char* android_id_p = (optind < argc) ? argv[optind++] : NULL;
    if (android_id_p == NULL)
    {
      die("Syntax: iddc --set-android_id <ANDROID_ID>");
    }
    int rescode = idd_set_android_id(idd, android_id_p);
    if (rescode < 0)
    {
      print_idd_failure(rescode, "idd_android_id()");
    }
  }
  else if (command == SET_TIMEZONE)
  {
    int rescode = idd_set_timezone(idd);
    if (rescode < 0)
    {
      print_idd_failure(rescode, "idd_set_timezone()");
    }
  }
  else
  {
    die("No options given. See 'iddc --help'");
  }
  idd_destroy(&idd);
  return 0;
}
