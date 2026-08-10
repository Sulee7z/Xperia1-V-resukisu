LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CERTIFICATE := platform

LOCAL_JAVA_LIBRARIES := com.sonyericsson.idd
LOCAL_STATIC_JAVA_LIBRARIES := com.google.protobuf-2.3.0

LOCAL_SRC_FILES := \
    $(call all-java-files-under,src)

# Apk only relevant for test builds
LOCAL_MODULE_TAGS := tests

LOCAL_JAVA_LIBRARIES += android.test.runner
LOCAL_PACKAGE_NAME := IddApiPerformanceTest
include $(BUILD_PACKAGE)
