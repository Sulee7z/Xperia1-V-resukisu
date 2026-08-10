#ifndef VENDOR_SEMC_SYSTEM_IDD_V1_1_IDD_H
#define VENDOR_SEMC_SYSTEM_IDD_V1_1_IDD_H

#include <vendor/semc/system/idd/1.1/IIdd.h>
#include "idd_api.h"
#include "idd_private_api.h"

#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>

namespace vendor {
namespace semc {
namespace system {
namespace idd {
namespace V1_1 {
namespace implementation {

using ::android::hidl::base::V1_0::DebugInfo;
using ::android::hidl::base::V1_0::IBase;
using ::vendor::semc::system::idd::V1_0::ConfigRequest;
using ::vendor::semc::system::idd::V1_0::DeviceId;
using ::vendor::semc::system::idd::V1_0::EnabledPackages;
using ::vendor::semc::system::idd::V1_1::IIdd;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;
using ::android::sp;

struct Idd : public IIdd {
    // Methods from ::vendor::semc::system::idd::V1_0::IIdd follow.
    Return<void> create() override;
    Return<void> createFromPath(const hidl_string& baseDir) override;
    Return<void> destroy() override;
    Return<int32_t> setCounter(const hidl_string& pkg, const hidl_string& name, int64_t value)
        override;
    Return<int32_t> incCounter(const hidl_string& pkg, const hidl_string& name, int64_t delta)
        override;
    Return<int32_t> setSdata(const hidl_string& pkg, const hidl_string& name,
        const hidl_string& value) override;
    Return<int32_t> addEvent(const hidl_string& pkg, const hidl_string& name,
        const hidl_vec<uint8_t>& payload, uint64_t payloadSize) override;
    Return<int32_t> addBigPayloadEvent(const hidl_string& pkg, const hidl_string& name,
        const hidl_memory& payload, uint64_t payloadSize) override;
    Return<int32_t> addAppEvent(const hidl_string& appPkg, const hidl_string& appVersionName,
        int32_t appVersionCode, const hidl_string& timestamp, int32_t payloadType,
        const hidl_vec<uint8_t>& payload, uint64_t payloadSize) override;
    Return<int32_t> registerToUseAddAppEvent(const hidl_string& appPkg,
        const hidl_string& className) override;
    Return<void> getAppEventEnabledPackages(getAppEventEnabledPackages_cb _hidl_cb) override;
    Return<int32_t> addPluginEvent(int32_t pluginId, const hidl_string& pkg,
        const hidl_string& name, const hidl_vec<uint8_t>& payload, uint64_t payloadSize) override;
    Return<int32_t> isActive() override;
    Return<int32_t> isProbeActive(const hidl_string& pkg, const hidl_string& name) override;
    Return<void> getConfig(const hidl_string& pkg, const hidl_string& name, getConfig_cb _hidl_cb)
        override;
    Return<void> getConfigNotify(getConfigNotify_cb _hidl_cb) override;
    Return<int32_t> configWait(int32_t fd) override;
    Return<int32_t> setGroupId(const hidl_string& groupId) override;
    Return<int32_t> deleteGroupId(const hidl_string& delGroupId) override;
    Return<int32_t> replaceGroupId(const hidl_string& toGroupId, const hidl_string& fromGroupId)
        override;
    Return<int32_t> doWashing() override;
    Return<void> llCreateEvent(const hidl_string& pkg, const hidl_string& name,
        uint64_t payloadSize) override;
    Return<int32_t> llDispatchEvent(uint64_t payloadSize) override;
    Return<int32_t> flush() override;
    Return<int32_t> sendRaw(const hidl_string& raw, uint64_t rawSize) override;
    Return<int32_t> setImei(const hidl_string& imei) override;
    Return<int32_t> setAndroidId(const hidl_string& androidId) override;
    Return<int32_t> setXsAndroidId(const hidl_string& xsAndroidId) override;
    Return<int32_t> setXsGuid(const hidl_string& xsGuid) override;
    Return<int32_t> setUserConsent(int32_t userConsent) override;
    Return<int32_t> notifyConfig() override;
    Return<int32_t> forceSave() override;
    Return<int32_t> setSerialNumber(const hidl_string& serialNumber) override;
    Return<int32_t> setDataCollectionEnable(int32_t enable) override;
    Return<int32_t> removeReport(int32_t reportNum) override;
    Return<void> getDeviceId(getDeviceId_cb _hidl_cb) override;
    Return<int32_t> writeConfig(const hidl_vec<uint8_t>& config, uint64_t configSize) override;
    Return<void> fileIteratorNext(int32_t clientId, int32_t fileIterator,
        fileIteratorNext_cb _hidl_cb) override;
    Return<void> fileGet(int32_t clientId, int32_t fileIter, fileGet_cb _hidl_cb) override;
    Return<void> configRequestGet(int32_t cr_size, configRequestGet_cb _hidl_cb) override;
    Return<int32_t> setTimezone() override;

    Return<void> getConfigData(const hidl_string& pkg, const hidl_string& name, getConfigData_cb _hidl_cb) override;
    Return<void> getReportDirStats(getReportDirStats_cb _hidl_cb) override;
    Return<void> getReportDataMemory(int32_t number, getReportDataMemory_cb _hidl_cb) override;
    Return<int32_t> setLastUploadedReportNumber(int32_t number) override;
    Return<int32_t> getLastUploadedReportNumber() override;

private:
    static IDD_t* idd_p;
    inline std::vector<uint8_t> read_vector_from_file(std::string file_path);
    std::pair<bool, sp<IMemory>> copyVectorToMemory(std::vector<uint8_t>* v, hidl_memory* mem);
};

template<typename T> const hidl_vec<T> toHidlVec(const std::vector<T> &vec) {
    hidl_vec<T> hVec;
    hVec.setToExternal(const_cast<T *>(vec.data()), vec.size());
    return hVec;
}

extern "C" IIdd* HIDL_FETCH_IIdd(const char* name);

}  // namespace implementation
}  // namespace V1_1
}  // namespace idd
}  // namespace system
}  // namespace semc
}  // namespace vendor

#endif  // VENDOR_SEMC_SYSTEM_IDD_V1_1_IDD_H
