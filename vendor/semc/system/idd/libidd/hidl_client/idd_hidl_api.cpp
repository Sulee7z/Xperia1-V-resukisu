/*
 * Copyright 2017 Sony Corporation
 */

#include <iostream>
#include <string>
#include <utils/Log.h>
#include <cutils/log.h>
#include <errno.h>

#include "idd_api.h"
#include "idd_private_api.h"
#include "idd_config.h"
#include "idd_hidl_cli.h"

#undef LOG_TAG
#define LOG_TAG "idd-hidl-api"

using namespace std;

IDD_t* idd_create() {
    IddHidlCli* idd_hidl_cli_instance = new IddHidlCli("default");
    if (idd_hidl_cli_instance != nullptr)
    {
        if (idd_hidl_cli_instance->isAvailable())
        {
            idd_hidl_cli_instance->create();
            return (IDD_t*)idd_hidl_cli_instance;
        }
        else
        {
            delete idd_hidl_cli_instance;
        }
    }
    return nullptr;
}

IDD_t* idd_create_from_path(const char* base_dir_p) {
    IddHidlCli* idd_hidl_cli_instance = new IddHidlCli("logreader");
    if (idd_hidl_cli_instance != nullptr)
    {
        if (idd_hidl_cli_instance->isAvailable())
        {
            idd_hidl_cli_instance->createFromPath(base_dir_p);
            return (IDD_t*)idd_hidl_cli_instance;
        }
        else
        {
            delete idd_hidl_cli_instance;
        }
    }
    return nullptr;
}

void idd_destroy(IDD_t** idd_pp) {
    if (idd_pp != nullptr && *idd_pp != nullptr) {
        IddHidlCli* idd_hidl_cli_instance = (IddHidlCli*)*idd_pp;
        idd_hidl_cli_instance->IddHidlCli::destroy();
        delete *idd_pp;
        *idd_pp = nullptr;
    }
}

int idd_send_raw(IDD_t* idd, const char* raw_p, size_t raw_size) {
    int ret = -1;
    if (idd != nullptr && !(raw_size > 0 && raw_p == nullptr)) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->sendRaw(raw_p, raw_size);
    }
    return ret;
}

int idd_set_counter(IDD_t* idd, const char* package_p, const char* name_p, int64_t value) {
    int ret = -1;
    if (idd != nullptr && package_p != nullptr && name_p != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->setCounter(package_p, name_p, value);
    }
    return ret;
}

int idd_inc_counter(IDD_t* idd, const char* package_p, const char* name_p, int64_t delta) {
    int ret = -1;
    if (idd != nullptr && package_p != nullptr && name_p != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->incCounter(package_p, name_p, delta);
    }
    return ret;
}

int idd_set_sdata(IDD_t* idd, const char* package_p, const char* name_p, const char* value_p) {
    int ret = -1;
    if (idd != nullptr && package_p != nullptr && name_p != nullptr && value_p != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->setSdata(package_p, name_p, value_p);
    }
    return ret;
}

int idd_add_event(IDD_t* idd, const char* package_p, const char* name_p,
                  const char* payload_p, size_t payload_size) {
    int ret = -1;
    if (idd != nullptr && package_p != nullptr && name_p != nullptr && !(payload_size > 0 &&
        payload_p == nullptr)) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->addEvent(package_p, name_p, payload_p,
                                                            payload_size);
    }
    return ret;
}

int idd_add_plugin_event(IDD_t* idd, int plugin_id, const char* package_p, const char* name_p,
                         const char* payload_p, size_t payload_size) {
    int ret = -1;
    if (idd != nullptr && package_p != nullptr && name_p != nullptr && !(payload_size > 0 &&
        payload_p == nullptr)) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->addPluginEvent(plugin_id, package_p, name_p,
                                                                  payload_p, payload_size);
    }
    return ret;
}

int idd_add_app_event(IDD_t* idd, const char* app_package_p, const char* app_version_name_p,
                      int app_version_code, const char* timestamp_p, int payload_type,
                      const char* payload_p, size_t payload_size) {
    int ret = -1;
    if (idd != NULL && app_package_p != NULL && app_version_name_p != NULL && timestamp_p != NULL &&
        !(payload_size > 0 && payload_p == NULL)) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->addAppEvent(app_package_p, app_version_name_p,
                                                               app_version_code, timestamp_p,
                                                               payload_type, payload_p,
                                                               payload_size);
    }
    return ret;
}

int idd_register_to_use_add_app_event(IDD_t* idd, const char* app_package_p,
                                      const char* class_name_p) {
    int ret = -1;
    if (idd != NULL && app_package_p != NULL && class_name_p != NULL) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->registerToUseAddAppEvent(app_package_p,
                                                                           class_name_p);
    }
    return ret;
}

int idd_get_app_event_enabled_packages(IDD_t* idd, char* package_p) {
    int ret = -1;
    if (idd != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->getAppEventEnabledPackages(package_p);
    }
    return ret;
}

int idd_flush(IDD_t* idd) {
    int ret = -1;
    if (idd != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->flush();
    }
    return ret;
}

int idd_write_config(IDD_t* idd, const char* config_p, size_t config_size) {
    int ret = -1;
    if (idd != nullptr && !(config_size > 0 && config_p == nullptr)) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->writeConfig(config_p, config_size);
    }
    return ret;
}

int idd_notify_config(IDD_t* idd) {
    int ret = -1;
    if (idd != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->notifyConfig();
    }
    return ret;
}

int idd_is_active(IDD_t* idd) {
    int ret = -1;
    if (idd != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->isActive();
    }
    return ret;
}

int idd_is_probe_active(IDD_t* idd, const char* package_p, const char* name_p) {
    int ret = -1;
    if (idd != nullptr && package_p != nullptr && name_p != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->isProbeActive(package_p, name_p);
    }
    return ret;
}

int idd_get_config(IDD_t* idd, const char* package_p, const char* name_p) {
    int fd = -1;
    if (idd != nullptr && package_p != nullptr && name_p != nullptr) {
        fd = reinterpret_cast<IddHidlCli*>(idd)->getConfig(package_p, name_p);
    }
    return fd;
}

int idd_get_config_notify(IDD_t* idd) {
    int fd = -1;
    if (idd != nullptr) {
        fd = reinterpret_cast<IddHidlCli*>(idd)->getConfigNotify();
    }
    return fd;
}

int idd_config_wait(IDD_t* idd, int fd) {
    if (idd != nullptr && fd >= 0)
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
                ALOGE("%s: failed to get status flag: %s", __func__, strerror(errno));
                return -errno;
            }
            if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1)
            {
                ALOGE("%s: failed to set status flag(O_NONBLOCK): %s", __func__,
                         strerror(errno));
                return -errno;
            }
            while (read(fd, buf, sizeof(buf)) > 0)
            {
                ALOGW("Non blocking read loop");
            }
            if (fcntl(fd, F_SETFL, flag) == -1)
            {
                ALOGE("%s: failed to set status flag: %s", __func__, strerror(errno));
                return -errno;
            }
        }
    }
    else
    {
        ALOGE("%s: illegal args", __func__);
        return -EINVAL;
    }
    return 1;
}

int idd_set_imei(IDD_t* idd, const char* imei_p) {
    int ret = -1;
    if (idd != nullptr && imei_p != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->setImei(imei_p);
    }
    return ret;
}

int idd_set_android_id(IDD_t* idd, const char* android_id_p) {
    int ret = -1;
    if (idd != nullptr && android_id_p != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->setAndroidId(android_id_p);
    }
    return ret;
}

int idd_set_xs_android_id(IDD_t* idd, const char* xs_android_id_p) {
    int ret = -1;
    if (idd != nullptr && xs_android_id_p != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->setXsAndroidId(xs_android_id_p);
    }
    return ret;
}

int idd_set_xs_guid(IDD_t* idd, const char* xs_guid_p) {
    int ret = -1;
    if (idd != nullptr && xs_guid_p != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->setXsGuid(xs_guid_p);
    }
    return ret;
}

int idd_set_user_consent(IDD_t* idd, int user_consent) {
    int ret = -1;
    if (idd != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->setUserConsent(user_consent);
    }
    return ret;
}

int idd_force_save(IDD_t* idd) {
    int ret = -1;
    if (idd != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->forceSave();
    }
    return ret;
}

int idd_set_serial_number(IDD_t* idd, const char* serial_number_p) {
    int ret = -1;
    if (idd != nullptr && serial_number_p != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->setSerialNumber(serial_number_p);
    }
    return ret;
}

int idd_set_data_collection_enable(IDD_t* idd, int enable) {
    int ret = -1;
    if (idd != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->setDataCollectionEnable(enable);
    }
    return ret;
}

int idd_set_group_id(IDD_t* idd, const char* group_id_p) {
    int ret = -1;
    if (idd != nullptr && group_id_p != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->setGroupId(group_id_p);
    }
    return ret;
}

int idd_delete_group_id(IDD_t* idd, const char* del_group_id_p) {
    int ret = -1;
    if (idd != nullptr && del_group_id_p != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->deleteGroupId(del_group_id_p);
    }
    return ret;
}

int idd_replace_group_id(IDD_t* idd, const char* to_group_id_p, const char* from_group_id_p) {
    int ret = -1;
    if (idd != nullptr && to_group_id_p != nullptr && from_group_id_p != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->replaceGroupId(to_group_id_p, from_group_id_p);
    }
    return ret;
}

int idd_remove_report(IDD_t* idd, int report_num) {
    int ret = -1;
    if (idd != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->removeReport(report_num);
    }
    return ret;
}

int idd_get_device_id(IDD_t* idd, char* device_id) {
    int ret = -1;
    if (idd != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->getDeviceId(device_id);
    }
    return ret;
}

int idd_config_request_get(IDD_t* idd, char* cr, int cr_size) {
    int cr_len = 0;
    if (idd != nullptr) {
        cr_len = reinterpret_cast<IddHidlCli*>(idd)->configRequestGet(cr, cr_size);
    }
    return cr_len;
}

int idd_get_max_payload(void) {
    return IDD_MAX_EVENT_SIZE;
}

int idd_do_washing(IDD_t* idd) {
    int ret = -1;
    if (idd != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->doWashing();
    }
    return ret;
}

int idd_set_timezone(IDD_t* idd) {
    int ret = -1;
    if (idd != nullptr) {
        ret = reinterpret_cast<IddHidlCli*>(idd)->setTimezone();
    }
    return ret;
}
