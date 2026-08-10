/*
 * Copyright 2017 Sony Corporation
 */

#define LOG_TAG "vendor.semc.system.idd@1.1-service"

#include <android/log.h>
#include <hidl/HidlTransportSupport.h>

#include "Idd.h"

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::sp;
using vendor::semc::system::idd::V1_1::IIdd;
using vendor::semc::system::idd::V1_1::implementation::Idd;
using namespace vendor::semc::system::idd::V1_1::implementation;

int main() {
    android::status_t status;
    configureRpcThreadpool(10, true);
    android::sp<IIdd> service = new Idd();
    android::sp<IIdd> serviceForLogReader = new Idd();
    status = service->registerAsService();
    LOG_ALWAYS_FATAL_IF(status != android::OK, "Could not register IIdd(default)");
    status = serviceForLogReader->registerAsService("logreader");
    LOG_ALWAYS_FATAL_IF(status != android::OK, "Could not register IIdd(logreader)");
    joinRpcThreadpool();
}
