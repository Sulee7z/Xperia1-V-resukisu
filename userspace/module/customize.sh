#!/system/bin/sh
# FEAS All-in-One: verify bundled files
# $MODPATH = module install path (Magisk/KernelSU provides it)
SKIPMOUNT=false
PROPFILE=false
POSTFSDATA=false
LATESTARTSERVICE=true

# Compatible with both Magisk (ui_print) and KernelSU (ui_print)
if command -v ui_print >/dev/null 2>&1; then
    ui_print "- FEAS v3.0 (Binder event-driven)"
    if [ -f "$MODPATH/app-debug.apk" ]; then
        ui_print "- Bundled APK ready: app-debug.apk"
    else
        ui_print "- WARNING: bundled APK missing ($MODPATH/app-debug.apk)"
    fi
else
    # KernelSU: no ui_print, log to kmsg
    echo "FEAS: installing v3.0" >/dev/kmsg 2>/dev/null
fi

# Set correct perms for overlay APK (must be world-readable for RRO scan)
for file in "$MODPATH/system/product/overlay/"*; do
    if [ -f "$file" ]; then
        chmod 644 "$file" 2>/dev/null
        chown root:root "$file" 2>/dev/null
    fi
done
