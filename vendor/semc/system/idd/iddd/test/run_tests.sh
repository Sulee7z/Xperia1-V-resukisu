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

if [ "$1" = "rerun" ]; then
    echo "Re-running test case without updating the tests"
else
    echo "Loading test device with busybox and iddd tests from" $OUT/system/bin/
    adb shell "rm -r /data/idd_test"
    adb shell "mkdir /data/busybox ; mkdir /data/idd_test/" > /dev/null
    adb push busybox /data/busybox
    adb shell "cd /data/busybox && chmod 4775 /data/busybox/busybox && /data/busybox/busybox --install"
    rm -f test-files.zip
    echo $1 | egrep "tc|TC"
    IS_SINGLE_TEST_SUITE=$?
    if [ "$IS_SINGLE_TEST_SUITE" == "0" ]; then
        zip -rq test-files.zip $1 *-data test-lib.sh iddd_tests.sh
    else
        zip -rq test-files.zip tc-* test-lib.sh iddd_tests.sh
    fi
    adb push test-files.zip /data/idd_test/
    adb shell "cd /data/idd_test && /data/busybox/unzip -oq test-files.zip"
fi

adb shell "mv /system/etc/iddd.conf /system/etc/iddd_backup.conf"

START=$(date +%s)
adb shell "cd /data/idd_test; /data/busybox/sh /data/idd_test/iddd_tests.sh $@"
END=$(date +%s)
DIFF=$(($END - $START))
echo "It took $DIFF seconds"

adb shell "mv /system/etc/iddd_backup.conf /system/etc/iddd.conf"
