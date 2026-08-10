#!/bin/bash

# In case we don't have ANDROID_HOME on PATH
PATH=$ANDROID_HOME:$PATH

# This has to be run together with an emulator or a real phone. In any case, it must be connected with ADB

RET=`adb version`

if [ "$?" -ne "0" ]; then
    echo "Can't find 'adb' in PATH"
    exit 1
fi

adb wait-for-device
adb root

# If adb root is restarting the service, adb remount will fail until service is up and running again.
# This will happen if the device to test is a non-eng build.
while ! adb remount 2>&1 | grep succeed > /dev/null; do
    sleep 1
done

echo "Loading test device with iddd and iddc from" $OUT/system/bin/
adb push $OUT/obj/lib/libidd.so /system/lib/
adb push $OUT/obj/lib/libprotobuf-c.so /system/lib/
adb push $OUT/system/bin/iddc /system/bin/
adb push $OUT/system/bin/iddd /system/bin/
adb push $OUT/system/bin/idd-logreader /system/bin/
