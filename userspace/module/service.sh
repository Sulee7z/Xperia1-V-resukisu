#!/system/bin/sh
# FEAS module service - start root daemon + install bundled APK
MODDIR=${0%/*}
JAR="$MODDIR/feasd.jar"
LOG="$MODDIR/feasd.log"
APK="$MODDIR/app-debug.apk"
APK_INSTALLED_MARK="$MODDIR/.apk_installed"

# ---- Resolution restore (pdx234-resolution-unlock verbatim, with log) ----
# query WindowManagerService for mSystemBooted=true
{
    echo "[$(date)] resolution restore start"
    if [ "$(getprop ro.build.version.sdk)" = 33 ]
    then
        echo "[$(date)] sdk 33, exit"
        exit 0
    fi

    count=0
    while let "count++ < 100"
    do
        dumpsys window | grep -q mSystemBooted=true && break
        sleep 1
    done

    rate="$(settings get global user_preferred_refresh_rate)"
    height="$(settings get global user_preferred_resolution_height)"
    width="$(settings get global user_preferred_resolution_width)"
    echo "[$(date)] got width=$width height=$height rate=$rate"

    cmd display set-user-preferred-display-mode "$width" "$height" "$rate"
    echo "[$(date)] cmd display rc=$?"
} >> "$MODDIR/res_restore.log" 2>&1

# Wait for boot
until [ "$(getprop sys.boot_completed)" = "1" ]; do
    sleep 2
done

# frame_total is 0644 in kernel; platform_app cannot write it (DAC).
# Fix perms at boot so the module's frame counter reporting works.
chmod 0666 /sys/kernel/perf_manager/frame_total 2>/dev/null
# /dev/perf_manager: ueventd may reset misc-device mode to 0600; keep 0666
chmod 0666 /dev/perf_manager 2>/dev/null

# Install bundled FEAS app on EVERY boot: the Xposed framework loads module
# code from the INSTALLED apk (/data/app), not from the module dir - the
# one-shot .apk_installed marker left stale code loaded after zip updates.
if [ -f "$APK" ]; then
    pm install -r "$APK" >/dev/null 2>&1
    rm -f "$APK_INSTALLED_MARK"
fi

# Remove legacy standalone pdx234 resolution module app if still present
# (v0.6.0 merges resolution unlock into the FEAS module APK itself)
pm uninstall xyz.cirno.pdx234.resolution_sup >/dev/null 2>&1

# Ensure module enabled + scopes.
# NOTE: In the Vector framework, system_server is addressed by the scope
# name "system" (NOT "android" as in LSPosed).
if [ -x /data/adb/lspd/cli ]; then
    /data/adb/lspd/cli modules enable com.sony.feas >/dev/null 2>&1
    /data/adb/lspd/cli scope add com.sony.feas com.android.systemui/0 >/dev/null 2>&1
    /data/adb/lspd/cli scope add com.sony.feas com.android.settings/0 >/dev/null 2>&1
    /data/adb/lspd/cli scope add com.sony.feas system/0 >/dev/null 2>&1
    sleep 3
    killall com.android.systemui >/dev/null 2>&1
fi

start_daemon() {
    # kill any stale daemon first (avoid duplicate listeners on /dev/input)
    pkill -9 -f 'nice-name=feasd' 2>/dev/null
    sleep 1
    nohup app_process -Djava.class.path="$JAR" \
        /system/bin --nice-name=feasd com.sony.feas.daemon.Main \
        >> "$LOG" 2>&1 &
    echo $! > "$MODDIR/feasd.pid"
}

stop_daemon() {
    [ -f "$MODDIR/feasd.pid" ] && kill "$(cat "$MODDIR/feasd.pid")" 2>/dev/null
    pkill -9 -f "nice-name=feasd" 2>/dev/null
    rm -f "$MODDIR/feasd.pid"
}

case "$1" in
    start) start_daemon ;;
    stop) stop_daemon ;;
    *) stop_daemon; start_daemon ;;
esac
