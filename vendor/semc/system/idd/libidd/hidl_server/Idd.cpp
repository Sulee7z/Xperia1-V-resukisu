/*
 * Copyright 2017 Sony Corporation
 */

#include <fstream>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utils/Log.h>
#include <log/log.h>
#include "Idd.h"
#include "idd_config.h"

#undef LOG_TAG
#define LOG_TAG "idd-hidl-server"

namespace vendor {
namespace semc {
namespace system {
namespace idd {
namespace V1_1 {
namespace implementation {

using ::android::hardware::hidl_handle;
using ::android::hidl::memory::V1_0::IMemory;

IDD_t* Idd::idd_p = nullptr;

// Methods from ::vendor::semc::system::idd::V1_0::IIdd follow.
Return<void> Idd::create() {
    if (idd_p == nullptr) {
        idd_p = idd_create();
    }
    return Void();
}

Return<void> Idd::createFromPath(const hidl_string& baseDir) {
    if (idd_p == nullptr) {
        idd_p = idd_create_from_path(baseDir.c_str());
    }
    return Void();
}

Return<void> Idd::destroy() {
    return Void();
}

Return<int32_t> Idd::setCounter(const hidl_string& pkg, const hidl_string& name, int64_t value) {
    return idd_set_counter(idd_p, pkg.c_str(), name.c_str(), value);
}

Return<int32_t> Idd::incCounter(const hidl_string& pkg, const hidl_string& name, int64_t delta) {
    return idd_inc_counter(idd_p, pkg.c_str(), name.c_str(), delta);
}

Return<int32_t> Idd::setSdata(const hidl_string& pkg, const hidl_string& name,
                              const hidl_string& value) {
    return idd_set_sdata(idd_p, pkg.c_str(), name.c_str(), value.c_str());
}

Return<int32_t> Idd::addEvent(const hidl_string& pkg, const hidl_string& name,
                              const hidl_vec<uint8_t>& payload, uint64_t payloadSize) {
    return idd_add_event(idd_p, pkg.c_str(), name.c_str(), (const char*)payload.data(), payloadSize);
}

Return<int32_t> Idd::addBigPayloadEvent(const hidl_string& pkg, const hidl_string& name,
                              const hidl_memory& mem_in, uint64_t payloadSize) {

    /* get hidl_memory */
    sp<IMemory> memory = mapMemory(mem_in);
    if (memory == nullptr) {
        ALOGE("Couldn't map hidl_memory");
        return -1;
    }
    /* get hidl_memory pointer */
    char* p_buf = static_cast<char*>(static_cast<void*>(memory->getPointer()));

    return idd_add_event(idd_p, pkg.c_str(), name.c_str(), (const char *)p_buf, payloadSize);
}

Return<int32_t> Idd::addAppEvent(const hidl_string& appPkg, const hidl_string& appVersionName,
                                 int32_t appVersionCode, const hidl_string& timestamp,
                                 int32_t payloadType, const hidl_vec<uint8_t>& payload,
                                 uint64_t payloadSize) {
    return idd_add_app_event(idd_p, appPkg.c_str(), appVersionName.c_str(), appVersionCode,
                      timestamp.c_str(), payloadType, (const char*)payload.data(), payloadSize);
}

Return<int32_t> Idd::registerToUseAddAppEvent(const hidl_string& appPkg,
                                              const hidl_string& className) {
    return idd_register_to_use_add_app_event(idd_p, appPkg.c_str(), className.c_str());
}

Return<void> Idd::getAppEventEnabledPackages(getAppEventEnabledPackages_cb _hidl_cb) {
    EnabledPackages ep;
    int32_t res = idd_get_app_event_enabled_packages(idd_p, (char*)&ep);
    _hidl_cb(res, ep);
    return Void();
}

Return<int32_t> Idd::addPluginEvent(int32_t pluginId, const hidl_string& pkg,
         const hidl_string& name, const hidl_vec<uint8_t>& payload, uint64_t payloadSize) {
    return idd_add_plugin_event(idd_p, pluginId, pkg.c_str(),
                                name.c_str(), (const char*)payload.data(), payloadSize);
}

Return<int32_t> Idd::isActive() {
    return idd_is_active(idd_p);
}

Return<int32_t> Idd::isProbeActive(const hidl_string& pkg, const hidl_string& name) {
    return idd_is_probe_active(idd_p, pkg.c_str(), name.c_str());
}


Return<void> Idd::getConfig(const hidl_string& pkg, const hidl_string& name, getConfig_cb _hidl_cb) {
    native_handle_t* configFdHandle = native_handle_create(1, 0);
    if (configFdHandle != nullptr) {
        configFdHandle->data[0] = idd_get_config(idd_p, pkg.c_str(), name.c_str());
        if (configFdHandle->data[0] >= 0) {
            hidl_handle fd(configFdHandle);
            _hidl_cb(fd);
            close(configFdHandle->data[0]);
        } else {
            _hidl_cb(nullptr);
        }
        native_handle_delete(configFdHandle);
    } else {
        _hidl_cb(nullptr);
    }
    return Void();
}

Return<void> Idd::getConfigNotify(getConfigNotify_cb _hidl_cb) {
    native_handle_t* configFdHandle = native_handle_create(1, 0);
    if (configFdHandle != nullptr) {
        configFdHandle->data[0] = idd_get_config_notify(idd_p);
        if (configFdHandle->data[0] >= 0) {
            hidl_handle fd(configFdHandle);
            _hidl_cb(fd);
            close(configFdHandle->data[0]);
        } else {
            _hidl_cb(nullptr);
        }
        native_handle_delete(configFdHandle);
    } else {
        _hidl_cb(nullptr);
    }
    return Void();
}

Return<int32_t> Idd::configWait(int32_t fd) {
    return idd_config_wait(idd_p, fd);
}

Return<int32_t> Idd::setGroupId(const hidl_string& groupId) {
    return idd_set_group_id(idd_p, groupId.c_str());
}

Return<int32_t> Idd::deleteGroupId(const hidl_string& delGroupId) {
    return idd_delete_group_id(idd_p, delGroupId.c_str());
}

Return<int32_t> Idd::replaceGroupId(const hidl_string& toGroupId, const hidl_string& fromGroupId) {
    return idd_replace_group_id(idd_p, toGroupId.c_str(), fromGroupId.c_str());
}

Return<int32_t> Idd::doWashing() {
    return idd_do_washing(idd_p);
}

Return<void> Idd::llCreateEvent(const hidl_string& pkg, const hidl_string& name, uint64_t payloadSize) {
    return Void();
}

Return<int32_t> Idd::llDispatchEvent(uint64_t payloadSize) {
    return int32_t {};
}

Return<int32_t> Idd::flush() {
    return idd_flush(idd_p);
}

Return<int32_t> Idd::sendRaw(const hidl_string& raw, uint64_t rawSize) {
    return idd_send_raw(idd_p, raw.c_str(), rawSize);
}

Return<int32_t> Idd::setImei(const hidl_string& imei) {
    return idd_set_imei(idd_p, imei.c_str());
}

Return<int32_t> Idd::setAndroidId(const hidl_string& androidId) {
    return idd_set_android_id(idd_p, androidId.c_str());
}

Return<int32_t> Idd::setXsAndroidId(const hidl_string& xsAndroidId) {
    return idd_set_xs_android_id(idd_p, xsAndroidId.c_str());
}

Return<int32_t> Idd::setXsGuid(const hidl_string& xsGuid) {
    return idd_set_xs_guid(idd_p, xsGuid.c_str());
}

Return<int32_t> Idd::setUserConsent(int32_t userConsent) {
    return idd_set_user_consent(idd_p, userConsent);
}

Return<int32_t> Idd::notifyConfig() {
    return idd_notify_config(idd_p);
}

Return<int32_t> Idd::forceSave() {
    return idd_force_save(idd_p);
}

Return<int32_t> Idd::setSerialNumber(const hidl_string& serialNumber) {
    return idd_set_serial_number(idd_p, serialNumber.c_str());
}

Return<int32_t> Idd::setDataCollectionEnable(int32_t enable) {
    return idd_set_data_collection_enable(idd_p, enable);
}

Return<int32_t> Idd::removeReport(int32_t reportNum) {
    return idd_remove_report(idd_p, reportNum);
}

Return<void> Idd::getDeviceId(getDeviceId_cb _hidl_cb) {
    DeviceId di;
    int32_t res = idd_get_device_id(idd_p, (char*)&di);
    _hidl_cb(res, di);
    return Void();
}

Return<int32_t> Idd::writeConfig(const hidl_vec<uint8_t>& config, uint64_t configSize) {
    return idd_write_config(idd_p, (const char*)config.data(), configSize);
}

Return<void> Idd::fileIteratorNext(int32_t clientId, int32_t fileIterator,
                                   fileIteratorNext_cb _hidl_cb) {
    return Void();
}

Return<void> Idd::fileGet(int32_t clientId, int32_t fileIter, fileGet_cb _hidl_cb) {
    return Void();
}

Return<void> Idd::configRequestGet(int32_t cr_size, configRequestGet_cb _hidl_cb) {
    ConfigRequest cr;
    int32_t size = idd_config_request_get(idd_p, (char*)&cr, sizeof(cr));
    _hidl_cb(size, cr);
    return Void();
}

Return<int32_t> Idd::setTimezone() {
    return idd_set_timezone(idd_p);
}

// Methods from ::android::hidl::base::V1_0::IBase follow.

IIdd* HIDL_FETCH_IIdd(const char* /* name */) {
    return new Idd();
}

Return<void> Idd::getConfigData(const hidl_string& pkg, const hidl_string& name, getConfigData_cb _hidl_cb) {
    char path[PATH_MAX] = {0};
    if (idd_get_config_path(idd_p, pkg.c_str(), name.c_str(), path) <= 0) {
        ALOGE("Error Get config file path'%s'::'%s' -> %s", pkg.c_str(), name.c_str(), path);
        _hidl_cb(-1, NULL, 0);
        return Void();
    }

    ALOGD("getConfigData: Get config file path'%s'::'%s' -> %s", pkg.c_str(), name.c_str(), path);
    std::string config_path = IDD_DEFAULT_BASE_DIR;
    config_path += path;

    std::vector<uint8_t> data = read_vector_from_file(config_path);
    if (data.size() <= 0) {
        ALOGE("Error config file size'%s' %lu", config_path.c_str(), data.size());
        _hidl_cb(-1, NULL, 0);
        return Void();
    }

    _hidl_cb(0, toHidlVec(data), data.size());
    ALOGD("config file size'%s' %lu", config_path.c_str(), data.size());

    return Void();
}

Return<void> Idd::getReportDirStats(getReportDirStats_cb _hidl_cb) {
    int oldest_file_nr = 0;
    int newest_file_nr = 0;
    int total_file_nr = 0;
    size_t sum_size = 0;
    int ret = idd_get_report_stats(idd_p, &oldest_file_nr, &newest_file_nr, &total_file_nr, &sum_size);
    if (ret == 1) {
        _hidl_cb(0, oldest_file_nr, newest_file_nr, total_file_nr, sum_size);
    } else {
        _hidl_cb(-1, -1, -1, -1, -1);
    }
    return Void();
}

Return<void> Idd::getReportDataMemory(int32_t number, getReportDataMemory_cb _hidl_cb) {
    hidl_memory memory;
    char path[PATH_MAX] = {0};
    snprintf(path, PATH_MAX, "output/idd_%d", number);

    std::string report_path = IDD_DEFAULT_BASE_DIR;
    report_path += path;

    std::vector<uint8_t> data = read_vector_from_file(report_path);
    if (data.size() <= 0) {
        ALOGE("Error report file size'%s' %lu", report_path.c_str(), data.size());
        _hidl_cb(-1, memory, 0);
        return Void();
    }

    ALOGD("report file size'%s' %lu", report_path.c_str(), data.size());

    auto result = copyVectorToMemory(&data, &memory);
    if (result.first) {
        _hidl_cb(0, memory, data.size());
    } else {
        _hidl_cb(-1, memory, 0);
    }
    return Void();
}

Return<int32_t> Idd::setLastUploadedReportNumber(int32_t number) {
    return 0;
}

Return<int32_t> Idd::getLastUploadedReportNumber() {
    return 0;
}

inline std::vector<uint8_t> Idd::read_vector_from_file(std::string file_path) {
    std::ifstream instream(file_path, std::ios::in | std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(instream)), std::istreambuf_iterator<char>());
    return data;
}

std::pair<bool, sp<IMemory>> Idd::copyVectorToMemory(std::vector<uint8_t>* v, hidl_memory* mem) {
    sp<IMemory> memory;
    if (v->size() == 0) {
        return std::make_pair(true, memory);
    }
    sp<IAllocator> ashmem = IAllocator::getService("ashmem");
    if (ashmem == 0) {
        ALOGE("Failed to retrieve ashmem allocator service");
        return std::make_pair(false, memory);
    }
    bool success = false;
    Return<void> r = ashmem->allocate(v->size(), [&](bool s, const hidl_memory& m) {
        success = s;
        if (success) *mem = m;
    });
    if (r.isOk() && success) {
        memory = android::hardware::mapMemory(*mem);
        if (memory != 0) {
            memory->update();
            memcpy(memory->getPointer(), v->data(), v->size());
            memory->commit();
            return std::make_pair(true, memory);
        } else {
            ALOGE("Failed to map allocated ashmem");
        }
    } else {
        ALOGE("Failed to allocate %llu bytes from ashmem", (unsigned long long)v->size());
    }
    return std::make_pair(false, memory);
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace idd
}  // namespace system
}  // namespace semc
}  // namespace vendor
