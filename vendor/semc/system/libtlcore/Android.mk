LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libtlcore
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DDEBUG_LEVEL=0
LOCAL_SHARED_LIBRARIES += liblog

LOCAL_SRC_FILES := \
	src/tlcore_header.c \
	src/tlcore.c \
	src/section.c \
	src/section_header.c \
	src/section_data.c \
	src/tlcore_utils.c \
	src/sha1.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/src
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_EXPORT_HEADER_LIBRARY_HEADERS := libtlcore_headers
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libtlcore_headers
LOCAL_MODULE_TAGS := optional
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_HEADER_LIBRARY)
