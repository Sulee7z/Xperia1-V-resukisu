#!/bin/bash


TARGET_NAME="iddd"

DEVICE_DIR="/vendor/bin"
BUILD_TARGET_DIR="$ANDROID_BUILD_TOP/out/target/product/$TARGET_PRODUCT/${DEVICE_DIR}"

DEVICE_TARGET_PATH="${DEVICE_DIR}/${TARGET_NAME}"
BUILD_TARGET_PATH="${BUILD_TARGET_DIR}/${TARGET_NAME}"

CMDNAME=`basename $0`
ADB_CMD="adb"
FLG_DEBUGLOG="FALSE"

while getopts ds: OPT
do
  case $OPT in
    "s" ) FLG_SERIAL="TRUE" ; VALUE_SERIAL="$OPTARG" ;;
    "d" ) FLG_DEBUGLOG="TRUE" ;;
     *  ) echo "Usage: $CMDNAME [-s DEVICE_SERIAL] -d" 1>&2
          exit 1 ;;
  esac
done


if [ "${TARGET_PRODUCT-UNDEF}" = "UNDEF" ] ; then
  echo ". build/envsetup.sh"
  echo "lunch"
  exit 1
fi

if [ ! `pwd | grep "$ANDROID_BUILD_TOP"` ] ; then
  echo "ANDROID_BUILD_TOP = $ANDROID_BUILD_TOP"
  echo "current directory = $(pwd)"
  exit 1
fi

if [ ! -f "${BUILD_TARGET_PATH}" ] ; then
  echo "${BUILD_TARGET_PATH} not found."
  exit 1
fi


if [ "$FLG_SERIAL" = "TRUE" ]; then
  ADB_CMD="adb -s $VALUE_SERIAL"
fi

#ADB_CMD="echo"
$ADB_CMD wait-for-device
sleep 5
echo "adb root"
RET=`$ADB_CMD root 2>&1`
#echo "$RET"
case "${RET}" in
  *cannot*)
    echo "Please change miscTA 2473 to 0x0c or use userdebug variant."
    exit 1
    ;;
  restarting*)
    sleep 2
    $ADB_CMD wait-for-device
    ;;
  *)
    sleep 2
    $ADB_CMD wait-for-device
    ;;
esac

echo "adb disable-verity"
RET=`$ADB_CMD disable-verity 2>&1`
#echo "$RET"
case "${RET}" in
  "")
    ;;
  *already*)
    ;;
  *)
    $ADB_CMD reboot
    sleep 5
    $ADB_CMD wait-for-device
    sleep 2
    $ADB_CMD root
    sleep 2
    $ADB_CMD wait-for-device
    ;;
esac

$ADB_CMD remount
echo "pushing..."
$ADB_CMD push ${BUILD_TARGET_PATH} ${DEVICE_TARGET_PATH} 2>&1
$ADB_CMD shell sync
$ADB_CMD reboot
echo "rebooting..."

if [ "$FLG_DEBUGLOG" = "TRUE" ]; then
  $ADB_CMD wait-for-device
  $ADB_CMD root
  $ADB_CMD wait-for-device
  $ADB_CMD shell setprop vendor.idd.debug.log 1
else
cat << EOS
if you need debug log, please use below command
  $ adb root
  $ adb shell setprop vendor.idd.debug.log 1
EOS
fi
echo ""

