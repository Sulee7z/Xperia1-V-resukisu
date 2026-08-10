/*
 * Copyright 2020 Sony Corporation
 */

#include <unistd.h>
#include "protos/common.pb-c.h"

#define LOG_TAG "iddd-secure-mode"
#include "logging.h"

#ifndef NO_USE_DEVICE_SECURITY
#include <device_security_client.hpp>
static Idd__Header__SecureMode g_pb_sec_mode = IDD__HEADER__SECURE_MODE__LIVE;
static bool is_already_read_sec_mode = false;

/*
 */
static devsec_result_t get_security_config_with_retry(devsec_security_config_t* sec_mode)
{
  devsec_result_t sec_mode_result = DEVSEC_DEVICE_ERROR;
  for (int retry = 0; retry < 3; retry++)
  {
    if (retry != 0) usleep(200*1000);

    sec_mode_result = devsec_get_security_config(sec_mode);
    IDD_LOGD("Sec mode = %d, sec_mode_result %d ", *sec_mode, sec_mode_result);
    if (sec_mode_result == DEVSEC_RESULT_OK) break;
  }
  return sec_mode_result;
}

/*
 */
static void secure_mode_handle(void)
{
  devsec_security_config_t sec_mode;

  if (is_already_read_sec_mode) return;
  is_already_read_sec_mode = true;

  devsec_result_t sec_mode_result = get_security_config_with_retry(&sec_mode);
  if (sec_mode_result == DEVSEC_RESULT_OK)
  {
    if (sec_mode == DEVSEC_SECURITY_CONFIG_TEST)
    {
      g_pb_sec_mode = IDD__HEADER__SECURE_MODE__DEBUG;
    }
  }
  else
  {
    IDD_LOGE("Error in reading security config, sec_mode_result %d ", sec_mode_result);
  }
}
#endif

/*
 */
Idd__Header__SecureMode get_secure_mode(void)
{
#ifndef NO_USE_DEVICE_SECURITY
  secure_mode_handle();
  return g_pb_sec_mode;
#else
  return IDD__HEADER__SECURE_MODE__LIVE;
#endif
}

/*
 */
bool is_secure_mode_live(void)
{
  return get_secure_mode() == IDD__HEADER__SECURE_MODE__LIVE;
}
