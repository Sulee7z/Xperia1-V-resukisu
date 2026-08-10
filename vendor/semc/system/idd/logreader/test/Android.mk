#
# Copyright 2010 Sony Corporation
#
# Author Carl-Emil Lagerstedt (carl-emil.lagerstedt@sonyericsson.com)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng debug

LOCAL_CFLAGS:= -Wall
LOCAL_SRC_FILES:= generate_testdata.c

LOCAL_MODULE := idd-logreader-generate

include $(BUILD_EXECUTABLE)

