#!/system/bin/sh
# FEAS module service - start root Binder daemon
MODDIR=${0%/*}
JAR="$MODDIR/feasd.jar"
LOG="$MODDIR/feasd.log"

# Wait for boot
until [ "$(getprop sys.boot_completed)" = "1" ]; do
    sleep 2
done

start_daemon() {
    # app_process runs Java code as root; Binder service "feas" registered
    nohup app_process -Djava.class.path="$JAR" \
        /system/bin --nice-name=feasd com.sony.feas.daemon.Main \
        >> "$LOG" 2>&1 &
    echo $! > "$MODDIR/feasd.pid"
}

stop_daemon() {
    [ -f "$MODDIR/feasd.pid" ] && kill "$(cat "$MODDIR/feasd.pid")" 2>/dev/null
    pkill -f "nice-name=feasd" 2>/dev/null
    rm -f "$MODDIR/feasd.pid"
}

case "$1" in
    start) start_daemon ;;
    stop) stop_daemon ;;
    *) stop_daemon; start_daemon ;;
esac

# Watchdog
(
    while true; do
        sleep 30
        if [ -f "$MODDIR/feasd.pid" ] && ! kill -0 "$(cat "$MODDIR/feasd.pid")" 2>/dev/null; then
            start_daemon
        fi
    done
) &
