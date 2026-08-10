#!/bin/bash

function file_exist () {
    _file_path="${1}"
    _ret=$(adb shell ls "${_file_path}" 2>&1)
    if [ "${_file_path}" != "${_ret}" ] ; then
        echo "Error: ${_file_path} not found"
        return 1
    fi
    return 0
}

function dir_exist () {
    _dir_path="${1}"
    _ret=$(adb shell ls -d "${_dir_path}" 2>&1)
    if [ "${_dir_path}" != "${_ret}" ] ; then
        echo "Error: ${_dir_path} not found"
        return 1
    fi
    return 0
}

function grep_file_idd () {
    _file_path="${1}"
    file_exist "${_file_path}"
    if [ $? -ne 0 ] ; then
        echo "Error: ${_file_path} not found"
        return 1
    fi

    _grep=$(adb shell grep -i -e idd "${_file_path}" 2>&1)
    if [ -z "${_grep}" ] ; then
        echo "Error: idd not found in ${_file_path}"
        return 1
    fi
    return 0
}

function grep_file_rca () {
    _file_path="${1}"
    file_exist "${_file_path}"
    if [ $? -ne 0 ] ; then
        echo "Error: ${_file_path} not found"
        return 1
    fi

    _grep=$(adb shell grep -i -e rca "${_file_path}" 2>&1)
    if [ -z "${_grep}" ] ; then
        echo "Error: rca not found in ${_file_path}"
        return 1
    fi
    return 0
}

function check_dir_permission () {
    _dir_path="${1}"
    _req_user="${2}"
    _req_group="${3}"
    _req_secontext="${4}"

    _ret=$(adb shell ls -d -laZ "${_dir_path}" 2>&1)

    _user=$(echo "${_ret}" | awk '{print $3}')
    _group=$(echo "${_ret}" | awk '{print $4}')
    _secontext=$(echo "${_ret}" | awk '{print $5}')
    if [ "${_req_user}" !=  "${_user}" ] ; then
        echo "Error: ${_req_user} is not found"
        return 1
    fi
    if [ "${_req_group}" !=  "${_group}" ] ; then
        echo "Error: ${_req_group} is not found"
        return 1
    fi
    if [ "${_req_secontext}" !=  "${_secontext}" ] ; then
        echo "Error: ${_req_secontext} is not found"
        return 1
    fi
    return 0
}

function check_proc () {
    _proc_name="${1}"
    _required_user_name="${2}"
    _user_name=$(adb shell ps -ef | grep ${_proc_name} | awk '{print $1}')
    if [ "${_required_user_name}" !=  "${_user_name}" ] ; then
        echo "Error: ${_proc_name} is not found or invalid user"
        return 1
    fi
    return 0
}

function check_randomUID_proc () {
    _proc_name="${1}"
    _user_name=$(adb shell ps -ef | grep ${_proc_name} | awk '{print $1}')
    if [[ "$_user_name" != u0_* ]]; then
        echo "Error: process ${_proc_name} is not found or invalid user"
        return 1
    fi
    return 0
}

function kill_proc () {
    _proc_name="${1}"
    _pid=$(adb shell ps -ef | grep ${_proc_name} | awk '{print $2}')
    echo "${_proc_name}: pid=$_pid"
    $(adb shell kill $_pid)
    if [ $? -ne 0 ] ; then
        echo "Error: ${_proc_name}:$_pid kill failed"
        return 1
    fi
    return 0
}

function vold_restart_test () {
    # if idd partitions are unmounted when vold restart, missing http://review.sonyericsson.net/#/c/2320741/
    kill_proc vold
    if [ $? -ne 0 ] ; then
        echo "Error: vold_restart_test fail"
        return 1
    fi
    # needs 30sec for vold unmount
    echo "Wait: 30sec"
    sleep 30s
    return 0
}

function is_userdebug () {
    build_type=$(adb shell getprop | grep -i ro.build.type | awk '{print $2}')
    if [ "${build_type}" != "[userdebug]" ] ; then
        echo "Build variant is user, checking additional files"
        return 1
    fi
    return 0
}

function verify () {
    _func="${1}"

    if [ -z "$2" ]; then
        if ${_func}; then
            echo "OK: ${_func} $file"
        else
           echo "NG: ${_func} $file"
           exit 1
        fi
        return 0
    fi

    declare -n _items="${2}"
    for item in "${_items[@]}"; do
        if ${_func} ${item}; then
           echo "OK: ${_func} ${item}"
        else
           echo "NG: ${_func} ${item}"
        fi
    done

    return 0
}

Idd_Partitions=(
    "/dev/block/bootdevice/by-name/appslog"
    "/dev/block/bootdevice/by-name/diag"
    )
Idd_Platform_Configs=(
    "/vendor/etc/idd.fstab"
    )
Idd_Platform_Configs_User=(
    "/vendor/etc/iddd_user.conf"
    )
Idd_Platform_Configs_Userdebug=(
    "/vendor/etc/iddd_debug.conf"
    )
Idd_Initrc_Files=(
    "/vendor/etc/init/init.sony.idd.rc"
    "/vendor/etc/init/vendor.semc.system.idd@1.0-service.rc"
    )
Idd_Certificate_Files=(
    "/vendor/etc/security/idd_report.pem"
    "/vendor/etc/security/idd_config.pem"
    )
Idd_Hidl_Manifest_Files=(
    "/system/etc/vintf/compatibility_matrix.device.xml"
    "/vendor/etc/vintf/manifest/vendor.semc.system.idd.manifest.xml"
    )
Idd_Vendor_Binaries=(
    "/vendor/bin/iddd"
    "/vendor/bin/idd-logreader"
    "/vendor/bin/hw/vendor.semc.system.idd@1.0-service"
    )
Idd_Vendor_Libraries=(
    "/vendor/lib/libidd.so"
    "/vendor/lib/vendor.semc.system.idd@1.0.so"
    "/vendor/lib64/libidd.so"
    "/vendor/lib64/libprotobuf-c-idd.so"
    "/vendor/lib64/vendor.semc.system.idd@1.0.so"
    "/vendor/lib64/libprotobuf-cpp-2.3.0-lite.so"
    "/vendor/lib/libprotobuf-cpp-2.3.0-lite.so"
    )
Idd_Product_Libraries=(
    "/system_ext/lib64/libprotobuf-cpp-2.3.0-lite-plat.so"
    "/system_ext/lib/libprotobuf-cpp-2.3.0-lite-plat.so"
    )
# AID(Application Identifier) definition check on device/somc/$(SOMC_PLATFORM)/config.fs
# [AID_VENDOR_IDD]
# value:2987
Idd_Vendor_Etc_Files=(
    "/vendor/etc/group"
    )
Idd_System_apks=(
    "/system_ext/app/IddAgent/IddAgent.apk"
    "/system_ext/priv-app/DataDisclaimer/DataDisclaimer.apk"
    )
Idd_System_Libraries=(
    "/system_ext/lib/libidd-plat.so"
    "/system_ext/lib/vendor.semc.system.idd@1.0.so"
    "/system_ext/lib64/libidd-plat.so"
    "/system_ext/lib64/vendor.semc.system.idd@1.0.so"
    )
Idd_System_Others=(
    "/system_ext/framework/com.sonymobile.securedatamanager_impl.jar"
    "/system_ext/etc/permissions/com.sonymobile.vendor_idd.xml"
    "/product/framework/com.sonyericsson.idd_impl.jar"
    "/product/etc/permissions/com.sonyericsson.idd.xml"
    )
Idd_System_Others_Userdebug=(
    "/system_ext/etc/default-permissions/com.sonymobile.crashloguploader.xml"
    )
Idd_Vendor_Binaries_userdebug=(
    "/vendor/bin/iddc"
    )
Proc_Mounts=(
    "/proc/mounts"
   )
IDD_Socket_Files=(
    "/dev/socket/idd/idd_co_stream"
    "/dev/socket/idd/idd_co"
    "/dev/socket/idd/idd_cl"
    )
IDD_PARTITION_DIRS=(
    # dir                    user       group      selinux permissions
    "/mnt/idd                vendor_idd vendor_idd u:object_r:idd_root_dir:s0"
    "/mnt/idd/fsck           root       vendor_idd u:object_r:idd_fsck_file:s0"
    "/mnt/idd/input          vendor_idd vendor_idd u:object_r:idd_data_file:s0"
    "/mnt/idd/output         vendor_idd vendor_idd u:object_r:idd_output_file:s0"
    "/mnt/idd/private        vendor_idd vendor_idd u:object_r:idd_data_file:s0"
    )
RCA_PARTITION_DIRS=(
    # dir                          user       group      selinux permissions
    "/mnt/rca                      vendor_idd vendor_idd u:object_r:idd_rca_dir:s0"
    "/mnt/rca/plugins              vendor_idd vendor_idd u:object_r:idd_rca_file:s0"
    "/mnt/rca/plugins/rca-noncrash vendor_idd vendor_idd u:object_r:idd_rca_file:s0"
    )
Process_List=(
    #Process name                       user
    "iddd                               vendor_idd"
    "vendor.semc.system.idd@1.0-service vendor_idd"
    "idd-logreader                      vendor_idd"
    )

RandomUID_Process_List=(
    #Process name
    "com.sonyericsson.idd.agent"
    )

Idd_Files_List=(
    "${Idd_Partitions[@]}"
    "${Idd_Initrc_Files[@]}"
    "${Idd_Platform_Configs[@]}"
    "${Idd_Vendor_Binaries[@]}"
    "${Idd_Vendor_Libraries[@]}"
    "${Idd_Product_Libraries[@]}"
    "${Idd_System_apks[@]}"
    "${Idd_System_Libraries[@]}"
    "${Idd_System_Others[@]}"
    "${IDD_Socket_Files[@]}"
    )

Idd_Files_List_User=(
    "${Idd_Platform_Configs_User[@]}"
    "${Idd_Certificate_Files[@]}"
    )

Idd_Files_List_Userdebug=(
    "${Idd_Vendor_Binaries_userdebug[@]}"
    "${Idd_Platform_Configs_Userdebug[@]}"
    "${Idd_System_Others_Userdebug[@]}"
    )

Grep_Idd_List=(
    "${Idd_Hidl_Manifest_Files[@]}"
    "${Idd_Vendor_Etc_Files[@]}"
    "${Proc_Mounts[@]}"
    )

Grep_Rca_List=(
    "${Proc_Mounts[@]}"
    )

Partition_List=(
    "${IDD_PARTITION_DIRS[@]}"
    "${RCA_PARTITION_DIRS[@]}"
    )


function help_func () {
    echo
    echo "----------------------------------------------------------------------------"
    echo "Please re-flash sw via fg4 with \"Flash the diagnostics(diag) image\" option"
    echo "Helper functions:
    file_exist <path>            - Check for the file exists in the <path>
    grep_file_idd <path>         - grep the idd in the <path>
    grep_file_rca <path>         - grep the rca in the <path>
    check_dir_permission <input> - Check the input <dir user group  selinux permissions>
    check_proc <input>           - Check the running process input <process_name user>
    check_randomUID_proc <input> - Check the running process input that has random UID.
    vold_restart_test            - kills the vold process & system will restart anyway"
    echo "----------------------------------------------------------------------------"
    echo
}

adb wait-for-device
adb root
help_func
verify file_exist Idd_Files_List
verify grep_file_idd Grep_Idd_List
verify grep_file_rca Grep_Rca_List
verify check_dir_permission Partition_List
verify check_proc Process_List
verify check_randomUID_proc RandomUID_Process_List
verify vold_restart_test
if ! is_userdebug; then
    verify file_exist Idd_Files_List_User
fi
if is_userdebug; then
    verify file_exist Idd_Files_List_Userdebug
fi

if [ $? -eq 0 ] ; then
echo "Success"
else
echo "Fail"
fi
