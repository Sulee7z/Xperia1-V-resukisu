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

# ---- NOTE: Resolution restore removed ----
# 旧的 pdx234-resolution-unlock verbatim 段(shell 里 dumpsys/settings/cmd display)
# 在 KernelSU su 域下对 system_server 做 binder_call 被 SELinux 拦截,
# 一直报 "Failed transaction (2147483646)",从未生效过。
# 相同功能已由 FEAS 模块的 FeasModule.scheduleBootResolutionRestore 在
# system_server 进程内以反射方式实现(verified: 4K persists across reboots),
# 不走 su shell,无 SELinux 限制。删除本段:消除报错 + 减少开机轮询延迟。
# (原段内 sdk 33 分支的 `exit 0` 还会在 sdk 33 设备上跳过后续 daemon 启动,
#  一并移除。)

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
