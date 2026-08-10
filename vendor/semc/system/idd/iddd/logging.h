/*
 * Copyright 2010 Sony Corporation
 */

/**
 * @brief Logging for iddd
 *
 * @author Victor Boivie (victor.boivie@sonyericsson.com)
 */
#ifndef LOGGING_H_INCLUSION_GUARD
#define LOGGING_H_INCLUSION_GUARD

#ifndef LOG_TAG
#define LOG_TAG "iddd"
#endif

#ifdef PC_BUILD
# ifdef RUNNING_WITH_GCOV
/* We remove logging when analyzing code coverage - they are
 * not "real code".
 */
#  define IDD_LOGD(...) do { } while (0)
#  define IDD_LOGI(...) do { } while (0)
#  define IDD_LOGW(...) do { } while (0)
#  define IDD_LOGE(...) do { } while (0)

# else
/* Stubbed calls */
#  ifdef TESTING_ON_PC
#   define IDD_LOGD(...) ((void)0)
#  else
#   include "util.h"
#   define IDD_LOGD(...) do { if (is_debug()) { printf("D/" LOG_TAG ": " __VA_ARGS__); printf("\n"); } } while (0)
#  endif
#  define IDD_LOGI(...) do { printf("I/" LOG_TAG ": " __VA_ARGS__); printf("\n");} while (0)
#  define IDD_LOGW(...) do { printf("W/" LOG_TAG ": " __VA_ARGS__); printf("\n");} while (0)
#  define IDD_LOGE(...) do { printf("E/" LOG_TAG ": " __VA_ARGS__); printf("\n");} while (0)
# endif
#else
/* LOGD is disabled by default. To enable, uncomment the following line */
# define LOG_NDDEBUG 0

/* LOGI is enabled by default */
# define LOG_NIDEBUG 0

# include <log/log.h>
/* These macros will soon not only log, but also generate idd-probes
 * automatically. At least the error ones.
 */
# ifdef ENABLE_LOGD
#  include "util.h"
#  include <cutils/properties.h>
#  include <string.h>
/* From Jelly Bean LOGx has been renamed ALOGx */
#  if !defined(LOGD) && defined(ALOGD)
#   define IDD_LOGD(...) \
       do { char value[PROPERTY_VALUE_MAX] = {'\0'}; \
            property_get("vendor.idd.debug.log", value, "0"); \
            if (value[0] == '1') \
                { ALOGD(__VA_ARGS__); } \
          } while (0)
#  else
#   define IDD_LOGD(...) \
       do { char value[PROPERTY_VALUE_MAX] = {'\0'}; \
            property_get("vendor.idd.debug.log", value, "0"); \
            if (value[0] == '1') \
                { LOGD(__VA_ARGS__); } \
          } while (0)
#  endif
# else
#  define  IDD_LOGD(...) ((void)0)
# endif

/* From Jelly Bean LOGx has been renamed ALOGx */
# if !defined(LOGI) && defined(ALOGI)
#  define IDD_LOGI ALOGI
#  define IDD_LOGW ALOGW
#  define IDD_LOGE ALOGE
# else
#  define IDD_LOGI LOGI
#  define IDD_LOGW LOGW
#  define IDD_LOGE LOGE
# endif

#endif

#endif
