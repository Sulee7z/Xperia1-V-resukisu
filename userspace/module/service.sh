#!/system/bin/sh
# FEAS module service - start root daemon + install bundled APK
MODDIR=${0%/*}
JAR="$MODDIR/feasd.jar"
APK="$MODDIR/app-debug.apk"
# All runtime state OUTSIDE the module dir: KernelSU flags the module as
# "updated" whenever files inside the module dir change. Logs/pid/markers
# live under /data/adb/feas (KernelSU persistent data dir) instead - it is
# root-writable, survives module updates, and the OS never wipes it.
# (Previously /data/local/tmp/feas: Android can clear /data/local/tmp, which
# wiped the APK hash marker -> pm install -r every boot -> LSP detected the
# APK reinstall and prompted "module updated" on every boot.)
STATE=/data/adb/feas
mkdir -p "$STATE"
LOG="$STATE/feasd.log"
APK_HASH_FILE="$STATE/.apk_installed.hash"
SCOPE_MARK="$STATE/.scope_applied"
RES_LOG="$STATE/res_restore.log"

# ---- Resolution restore (pdx234-resolution-unlock verbatim, with log) ----
{
    echo "[$(date)] resolution restore start"
    if [ "$(getprop ro.build.version.sdk)" = 33 ]; then
        echo "[$(date)] sdk 33, exit"
        exit 0
    fi

    count=0
    while let "count++ < 100"; do
        dumpsys window | grep -q mSystemBooted=true && break
        sleep 1
    done

    rate="$(settings get global user_preferred_refresh_rate)"
    height="$(settings get global user_preferred_resolution_height)"
    width="$(settings get global user_preferred_resolution_width)"
    echo "[$(date)] got width=$width height=$height rate=$rate"

    cmd display set-user-preferred-display-mode "$width" "$height" "$rate"
    echo "[$(date)] cmd display rc=$?"
} >> "$RES_LOG" 2>&1

# Wait for boot
until [ "$(getprop sys.boot_completed)" = "1" ]; do
    sleep 2
done

# frame_total is 0644 in kernel; platform_app cannot write it (DAC).
# Fix perms at boot so the module's frame counter reporting works.
chmod 0666 /sys/kernel/perf_manager/frame_total 2>/dev/null
# /dev/perf_manager: ueventd may reset misc-device mode to 0600; keep 0666
chmod 0666 /dev/perf_manager 2>/dev/null

# Install bundled FEAS app ONLY when the APK content changed. The Xposed
# framework loads module code from the INSTALLED apk (/data/app), so the
# installed copy must track the bundled one across zip updates - but a
# hash marker (in /data/local/tmp, outside module dir) avoids reinstalling
# on every boot, which both wastes time and churns /data/app.
if [ -f "$APK" ]; then
    NEW_HASH="$(md5sum "$APK" 2>/dev/null | awk '{print $1}')"
    OLD_HASH="$(cat "$APK_HASH_FILE" 2>/dev/null)"
    if [ -z "$OLD_HASH" ] || [ "$OLD_HASH" != "$NEW_HASH" ]; then
        pm install -r "$APK" >/dev/null 2>&1
        echo "$NEW_HASH" > "$APK_HASH_FILE"
    fi
fi

# Remove legacy standalone pdx234 resolution module app if still present
# (v0.6.0 merges resolution unlock into the FEAS module APK itself)
pm uninstall xyz.cirno.pdx234.resolution_sup >/dev/null 2>&1

# Ensure module enabled + scopes.
# NOTE: In the Vector framework, system_server is addressed by the scope
# name "system" (NOT "android" as in LSPosed).
# IDEMPOTENT: cli scope add writes modules_config.db on every invocation;
# Vector reads that DB at boot and prompts "module updated" when it changed.
# Run scope config ONCE (marker v3), never touch the DB again on later boots.
if [ -x /data/adb/lspd/cli ]; then
    if [ ! -f "$SCOPE_MARK" ] || [ "$(cat "$SCOPE_MARK")" != "v3" ]; then
        /data/adb/lspd/cli modules enable com.sony.feas >/dev/null 2>&1
        /data/adb/lspd/cli scope add com.sony.feas com.android.systemui/0 >/dev/null 2>&1
        /data/adb/lspd/cli scope add com.sony.feas com.android.settings/0 >/dev/null 2>&1
        /data/adb/lspd/cli scope add com.sony.feas system/0 >/dev/null 2>&1
        # Also register the launcher + game scopes (frame reporting in every
        # UI process): the module must load into them to report frames.
        /data/adb/lspd/cli scope add com.sony.feas com.sony.sonyericsson.home/0 >/dev/null 2>&1
        echo "v3" > "$SCOPE_MARK"
        sleep 3
        killall com.android.systemui >/dev/null 2>&1
    fi
fi

start_daemon() {
    # kill any stale daemon first (avoid duplicate listeners on /dev/input)
    pkill -9 -f 'nice-name=feasd' 2>/dev/null
    sleep 1
    nohup app_process -Djava.class.path="$JAR" \
        /system/bin --nice-name=feasd com.sony.feas.daemon.Main \
        >> "$LOG" 2>&1 &
    echo $! > "$STATE/feasd.pid"
}

stop_daemon() {
    [ -f "$STATE/feasd.pid" ] && kill "$(cat "$STATE/feasd.pid")" 2>/dev/null
    pkill -9 -f "nice-name=feasd" 2>/dev/null
    rm -f "$STATE/feasd.pid"
}

case "$1" in
    start) start_daemon ;;
    stop) stop_daemon ;;
    *) stop_daemon; start_daemon ;;
esac
