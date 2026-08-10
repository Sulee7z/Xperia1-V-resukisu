#
# Copyright 2010 Sony Corporation
#
#
# Author Carl-Emil Lagerstedt (carl-emil.lagerstedt@sonyericsson.com)
#
# Simple makfile for building idd-logreader for the host (as
# opposed to the target). This file should not really have to be
# modified, you will probably rather change Android.mk
#
#

include Android.mk

INCLUDE_PATH = \
    -I. \
    -I../../../../../frameworks/base/include \
    -I../../../../../system/core/include \
    -I../../../../../vendor/semc/system/idd/protobuf-c

idd-logreader: $(LOCAL_SRC_FILES) Android.mk
	gcc -O0 -std=gnu99 -ggdb -Wall -DPC_BUILD -DTESTING_ON_PC $(INCLUDE_PATH) $(LOCAL_SRC_FILES) ../protobuf-c/protobuf-c.a ../api/idd_api.c -o $(LOCAL_MODULE)
# NOTE: make cant find the libidd yet so we monolith that lib

clean:
	@rm idd-logreader

