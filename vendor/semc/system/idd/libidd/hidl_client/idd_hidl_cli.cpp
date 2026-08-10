/*
 * Copyright 2017 Sony Corporation
 */

#include <iostream>
#include <string>
#include <utils/Log.h>
#include <log/log.h>

#include "idd_api.h"
#include "idd_private_api.h"
#include "idd_config.h"
#include "idd_hidl_cli.h"

#undef LOG_TAG
#define LOG_TAG "idd-hidl-cli"

android::sp<IAllocator> IddHidlCli::ashmemAllocator = 0;
android::hardware::hidl_memory IddHidlCli::idd_hidl_memory;
android::sp<IMemory> IddHidlCli::mMemory = NULL;

IddHidlCli::IddHidlCli(const char* instance) {
    idd_hidl_if = vendor::semc::system::idd::V1_0::IIdd::getService(instance);
    if (idd_hidl_if == nullptr) {
        ALOGE("can not getService()");
    }
}

IddHidlCli::~IddHidlCli() {
    idd_hidl_if = nullptr;
}

bool IddHidlCli::isAvailable() {
    if (idd_hidl_if == nullptr) {
        return false;
    }
    return true;
}

void IddHidlCli::create() {
    idd_hidl_if->create();
}

void IddHidlCli::createFromPath(const char* baseDir) {
    idd_hidl_if->createFromPath(baseDir);
}

void IddHidlCli::destroy() {
    idd_hidl_if->destroy();
}

int32_t IddHidlCli::sendRaw(const char* raw, uint64_t rawSize) {
    return idd_hidl_if->sendRaw(raw, rawSize);
}

int32_t IddHidlCli::setCounter(const char* pkg, const char* name, int64_t value) {
    return idd_hidl_if->setCounter(pkg, name, value);
}

int32_t IddHidlCli::incCounter(const char* pkg, const char* name, int64_t delta) {
    return idd_hidl_if->incCounter(pkg, name, delta);
}

int32_t IddHidlCli::setSdata(const char* pkg, const char* name, const char* value) {
    return idd_hidl_if->setSdata(pkg, name, value);
}

int32_t IddHidlCli::addEvent(const char* pkg, const char* name, const char* payload,
                             uint64_t payloadSize) {
    if (payloadSize < IDD_MAX_PACKET_SIZE) {
      // addEvent is called for sending payload size less than 255KB
      std::vector<uint8_t> payloadData(payload, payload + payloadSize);
      payloadData.push_back('\0');
      return idd_hidl_if->addEvent(pkg, name, payloadData, payloadSize);
    }
    else
    {
      // addBigPayloadEvent is called for sending payload size greater than 255KB
      ashmemAllocator = IAllocator::getService("ashmem");
      ashmemAllocator->allocate(payloadSize,
        [&](bool success, const hidl_memory& mem)
        {
          if(success) {
            ALOGD("Allocation of %u bytes of ashmem succeeded", payloadSize);
            idd_hidl_memory = mem;
          }else {
            ALOGE("%s: Allocating ashmem of %u bytes failed!",__FUNCTION__, payloadSize);
          }
        });
      if (idd_hidl_memory.handle() != NULL) {
        mMemory = mapMemory(idd_hidl_memory);
        if(mMemory != NULL) {
          char* data = static_cast<char*>(static_cast<void*>(mMemory->getPointer()));
          mMemory->update();
          memcpy(data, payload, payloadSize);
          mMemory->commit();
          return idd_hidl_if->addBigPayloadEvent(pkg, name, idd_hidl_memory, payloadSize);
        }
        else {
          ALOGE("idd_hidl_memory mapping failed");
          mMemory = nullptr;
          return -1;
        }
      }
      else {
        ALOGE("idd_hidl_memory handle is uninitialized");
        return -1;
      }
  }
}

int32_t IddHidlCli::addPluginEvent(int32_t pluginId, const char* pkg, const char* name,
                                   const char* payload, uint64_t payloadSize) {
    std::vector<uint8_t> payloadData(payload, payload + payloadSize);
    payloadData.push_back('\0');
    return idd_hidl_if->addPluginEvent(pluginId, pkg, name, payloadData, payloadSize);
}

int32_t IddHidlCli::addAppEvent(const char* appPkg, const char* appVersionName,
                                int32_t appVersionCode, const char* timestamp, int32_t payloadType,
                                const char* payload, uint64_t payloadSize) {
    std::vector<uint8_t> payloadData(payload, payload + payloadSize);
    payloadData.push_back('\0');
    return idd_hidl_if->addAppEvent(appPkg, appVersionName, appVersionCode, timestamp,
                                     payloadType, payloadData, payloadSize);
}

int32_t IddHidlCli::registerToUseAddAppEvent(const char* appPkg, const char* className) {
    return idd_hidl_if->registerToUseAddAppEvent(appPkg, className);
}

int32_t IddHidlCli::getAppEventEnabledPackages(char* regPkgs) {
    int32_t res;
    vendor::semc::system::idd::V1_0::EnabledPackages ep;
    idd_hidl_if->getAppEventEnabledPackages([&res, &ep](
        int32_t r,
        vendor::semc::system::idd::V1_0::EnabledPackages s) {
            res = r;
            ep = s;
        });
    memcpy(regPkgs, &(ep), sizeof(ep));
    return res;
}

int32_t IddHidlCli::flush() {
    return idd_hidl_if->flush();
}

int32_t IddHidlCli::writeConfig(const char* config, size_t configSize) {
    std::vector<uint8_t> configData(config, config + configSize);
    return idd_hidl_if->writeConfig(configData, configSize);
}

int32_t IddHidlCli::notifyConfig() {
    return idd_hidl_if->notifyConfig();
}

int32_t IddHidlCli::isActive() {
    return idd_hidl_if->isActive();
}

int32_t IddHidlCli::isProbeActive(const char* pkg, const char* name) {
    return idd_hidl_if->isProbeActive(pkg, name);
}

int32_t IddHidlCli::getConfig(const char* pkg, const char* name) {
    hidl_handle fd;
    int32_t cfg = IDD_NO_CONFIG_AVAILABLE;
    idd_hidl_if->getConfig(pkg, name, [&fd](
        hidl_handle s) {
            fd = s;
        });
    if (fd != nullptr) {
        native_handle_t* fd_clone = native_handle_clone(fd);
        if (fd_clone != nullptr) {
            cfg = fd_clone->data[0];
            free(fd_clone);
        }
    }
    return cfg;
}

int32_t IddHidlCli::getConfigNotify() {
    hidl_handle fd;
    int32_t cfg_nfy = -1;
    idd_hidl_if->getConfigNotify([&fd](
        hidl_handle s) {
            fd = s;
        });
    if (fd != nullptr) {
        native_handle_t* fd_clone = native_handle_clone(fd);
        if (fd_clone != nullptr) {
            cfg_nfy = fd_clone->data[0];
            free(fd_clone);
        }
    }
    return cfg_nfy;
}

int32_t IddHidlCli::configWait(int32_t fd) {
    return idd_hidl_if->configWait(fd);
}

int32_t IddHidlCli::setImei(const char* imei) {
    return idd_hidl_if->setImei(imei);
}

int32_t IddHidlCli::setAndroidId(const char* androidId) {
    return idd_hidl_if->setAndroidId(androidId);
}

int32_t IddHidlCli::setXsAndroidId(const char* xsAndroidId) {
    return idd_hidl_if->setXsAndroidId(xsAndroidId);
}

int32_t IddHidlCli::setXsGuid(const char* xsGuid) {
    return idd_hidl_if->setXsGuid(xsGuid);
}

int32_t IddHidlCli::setUserConsent(int32_t userConsent) {
    return idd_hidl_if->setUserConsent(userConsent);
}

int32_t IddHidlCli::forceSave() {
    return idd_hidl_if->forceSave();
}

int32_t IddHidlCli::setSerialNumber(const char* serialNumber) {
    return idd_hidl_if->setSerialNumber(serialNumber);
}

int32_t IddHidlCli::setDataCollectionEnable(int32_t enable) {
    return idd_hidl_if->setDataCollectionEnable(enable);
}

int32_t IddHidlCli::setGroupId(const char* groupId) {
    return idd_hidl_if->setGroupId(groupId);
}

int32_t IddHidlCli::deleteGroupId(const char* delGroupId) {
    return idd_hidl_if->deleteGroupId(delGroupId);
}

int32_t IddHidlCli::replaceGroupId(const char* toGroupId, const char* fromGroupId) {
    return idd_hidl_if->replaceGroupId(toGroupId, fromGroupId);
}

int32_t IddHidlCli::removeReport(int32_t reportNum) {
    return idd_hidl_if->removeReport(reportNum);
}

int32_t IddHidlCli::getDeviceId(char* deviceId) {
    int32_t res;
    vendor::semc::system::idd::V1_0::DeviceId di;
    idd_hidl_if->getDeviceId([&res, &di](
        int32_t r,
        vendor::semc::system::idd::V1_0::DeviceId s) {
            res = r;
            di = s;
        });
    memcpy(deviceId, &(di), sizeof(di));
    return res;
}

int32_t IddHidlCli::fileIteratorNext(int32_t clientId, int32_t* fileIter) {
    int32_t res;
    vendor::semc::system::idd::V1_0::FileIterator fi;
    idd_hidl_if->fileIteratorNext(clientId, *fileIter, [&res, &fi](
        int32_t r,
        vendor::semc::system::idd::V1_0::FileIterator s) {
            res = r;
            fi = s;
        });
    memcpy(fileIter, &(fi), sizeof(fi));
    return res;
}

int32_t IddHidlCli::fileGet(int32_t clientId, int32_t fileIter) {
    hidl_handle fd;
    int32_t file_handle = IDD_NO_FILE;
    idd_hidl_if->fileGet(clientId, fileIter, [&fd](
        hidl_handle s) {
            fd = s;
        });
    if (fd != nullptr) {
        native_handle_t* fd_clone = native_handle_clone(fd);
        if (fd_clone != nullptr) {
            file_handle = fd_clone->data[0];
            free(fd_clone);
        }
    }
    return file_handle;
}

int32_t IddHidlCli::configRequestGet(char* cr, int32_t crSize) {
    int32_t read_cr_size;
    vendor::semc::system::idd::V1_0::ConfigRequest read_cr;
    idd_hidl_if->configRequestGet(crSize, [&read_cr_size, &read_cr](
        int32_t r,
        vendor::semc::system::idd::V1_0::ConfigRequest s) {
            read_cr_size = r;
            read_cr = s;
        });
    memcpy(cr, &(read_cr.cr), read_cr_size);
    return read_cr_size;
}

int32_t IddHidlCli::doWashing() {
    return idd_hidl_if->doWashing();
}

int32_t IddHidlCli::setTimezone() {
    return idd_hidl_if->setTimezone();
}
