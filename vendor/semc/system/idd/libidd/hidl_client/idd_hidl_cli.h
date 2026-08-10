/*
 * Copyright 2017 Sony Corporation
 */
#ifndef IDDHIDLCLI_H
#define IDDHIDLCLI_H

#include <vendor/semc/system/idd/1.0/IIdd.h>

#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>

using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_memory;


using namespace std;

class IddHidlCli {
public:
    IddHidlCli(const char* instance);
    ~IddHidlCli();

    bool isAvailable();
    void create();
    void createFromPath(const char* baseDir);
    void destroy();
    int32_t sendRaw(const char* raw, uint64_t rawSize);
    int32_t setCounter(const char* pkg, const char* name, int64_t value);
    int32_t incCounter(const char* pkg, const char* name, int64_t delta);
    int32_t setSdata(const char* pkg, const char* name, const char* value);
    int32_t addEvent(const char* pkg, const char* name, const char* payload,
                     uint64_t payload_size);
    int32_t addPluginEvent(int32_t pluginId, const char* pkg, const char* name,
                           const char* payload, uint64_t payloadSize);
    int32_t addAppEvent(const char* appPkg, const char* appVersionName,
                        int32_t appVersionCode, const char* timestamp, int32_t payloadType,
                        const char* payload, uint64_t payloadSize);
    int32_t registerToUseAddAppEvent(const char* appPkg, const char* className);
    int32_t getAppEventEnabledPackages(char* regPkgs);
    int32_t flush();
    int32_t writeConfig(const char* config, size_t configSize);
    int32_t notifyConfig();
    int32_t isActive();
    int32_t isProbeActive(const char* pkg, const char* name);
    int32_t getConfig(const char* pkg, const char* name);
    int32_t getConfigNotify();
    int32_t configWait(int32_t fd);
    int32_t setImei(const char* imei);
    int32_t setAndroidId(const char* androidId);
    int32_t setXsAndroidId(const char* xsAndroidId);
    int32_t setXsGuid(const char* xsGuid);
    int32_t setUserConsent(int32_t userConsent);
    int32_t forceSave();
    int32_t setSerialNumber(const char* serialNumber);
    int32_t setDataCollectionEnable(int32_t enable);
    int32_t setGroupId(const char* groupId);
    int32_t deleteGroupId(const char* delGroupId);
    int32_t replaceGroupId(const char* toGroupId, const char* fromGroupId);
    int32_t removeReport(int reportNum);
    int32_t getDeviceId(char* deviceId);
    int32_t fileIteratorNext(int32_t clientId, int32_t* fileIter);
    int32_t fileGet(int clientId, int32_t fileIter);
    int32_t configRequestGet(char* cr, int32_t crSize);
    int32_t doWashing();
    int32_t setTimezone();

private:
    android::sp<vendor::semc::system::idd::V1_0::IIdd> idd_hidl_if;
    static android::hardware::hidl_memory idd_hidl_memory;
    static android::sp<IAllocator> ashmemAllocator;
    static android::sp<IMemory> mMemory;
};

#endif /* IDDHIDLCLI_H */
