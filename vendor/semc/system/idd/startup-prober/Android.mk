LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS:= -Wall -O2

LOCAL_SRC_FILES:= main.c \
  memory.pb-c.c

LOCAL_MODULE := startup-prober
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_EXECUTABLES)
LOCAL_MODULE_OWNER := somc
LOCAL_VENDOR_MODULE := true

LOCAL_SHARED_LIBRARIES := \
  liblog \
  libidd \
  libprotobuf-c-idd

LOCAL_STATIC_LIBRARIES := idd_common_lib
LOCAL_HEADER_LIBRARIES := idd_config_header

include $(BUILD_EXECUTABLE)

