/*
 * Copyright 2010 Sony Corporation
 */

package com.sonyericsson.idd.api;

import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Process;
import android.os.RemoteException;

import java.io.File;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;

import vendor.semc.system.idd.V1_0.ConfigRequest;
import vendor.semc.system.idd.V1_0.DeviceId;
import vendor.semc.system.idd.V1_0.EnabledPackages;
import vendor.semc.system.idd.V1_0.IIdd;
import vendor.semc.system.idd.V1_0.IIdd.configRequestGetCallback;
import vendor.semc.system.idd.V1_0.IIdd.getAppEventEnabledPackagesCallback;
import vendor.semc.system.idd.V1_0.IIdd.getDeviceIdCallback;

/**
 * <p>
 * In Device Diagnostics Private controller API for the IDD Daemon. This API
 * should not be used by applications. After every API call,
 * {@link #getLatestResult() getLatestResult()} should be called to verify that
 * the call succeeded. If it didn't, and to prevent ANR's, no more IDD API calls
 * should be done until control has been passed back to the Android system.
 * </p>
 *
 */
public final class IddPrivate {

    /**
     * This class must not be instantiated
     */
    private IddPrivate() {
    }

    /**
     * Submit an event to the IDD Daemon
     *
     * <p>
     * This is a synchronous API.
     * </p>
     *
     * @param pkg the package that the event belongs to
     * @param name name of the event
     * @param payload the payload of the event. Typically serialized protocol
     *            buffer data.
     * @return Returns 0 if successful, < 0 if not.
     */
    public static int addEventSynchronized(Context context, String pkg, String name, byte[] payload) {
        if (pkg == null || name == null || payload == null)
            throw new IllegalArgumentException("Idd.addEvent(): illegal arg");
        if (!checkPermissionIddPrivate(context)) {
            mLog.e("addEventSynchronized was denied due to permission!");
            return -1;
        }
        return Idd.addEventImpl(pkg, name, payload);
    }

    /**
     * Submit an event to the IDD Daemon
     *
     * <p>
     * This is a synchronous API.
     * </p>
     *
     * @param msg the protobuf container for the event. This is typically an
     *            object of the type GeneratedMessage or GeneratedMessageLite,
     *            created from builder.build(). This parameter is an Object to
     *            avoid referencing libraries that the target application may
     *            not wish to use.
     * @return Returns 0 if successful, < 0 if not.
     */
    public static int addEventSynchronized(Context context, Object msg) {
        if (msg == null)
            throw new IllegalArgumentException("Idd.addEvent(): illegal arg");
        Class<?> cls = msg.getClass();
        String pkg = cls.getPackage().getName();
        String name = cls.getSimpleName();
        int ret = -1;
        try {
            Method meth = cls.getMethod("toByteArray");
            Object res = meth.invoke(msg);
            if (res instanceof byte[]) {
                ret = addEventSynchronized(context, pkg, name, (byte[])res);
            } else {
                mLog.e("The return value of toByteArray was not a byte array");
            }
        } catch (NoSuchMethodException e) {
            mLog.e("The event passed in to addEvent is not a protocol buffer");
        } catch (Exception e) {
            mLog.e("There was an error calling addEvent(msg)");
        }
        return ret;
    }

    /**
     * Dispatches an event by plugin to the IDD Daemon
     *
     * <p>
     * This is a completely asynchronous event.
     * </p>
     *
     * @param pluginId the plugin id that the event belongs to
     * @param pkg the package that the event belongs to
     * @param name name of the event
     * @param payload the payload of the event. Typically serialized protocol
     *            buffer data.
     */
    public static void addPluginEvent(Context context, int pluginId, String pkg, String name, byte[] payload) {
        if (pkg == null || name == null || payload == null)
            throw new IllegalArgumentException("IddPrivate.addPluginEvent(): illegal arg");
        addPluginEventImpl(context, pluginId, pkg, name, payload);
    }

    private static int addPluginEventImpl(Context context, int pluginId, String pkg, String name, byte[] payload) {
        if (!checkPermissionIddPrivate(context)) {
            mLog.e("addPluginEventImpl was denied due to permission!");
            return -1;
        }
        ArrayList<Byte> listPayload = new ArrayList<>();
        for (byte b : payload) {
            listPayload.add(Byte.valueOf(b));
        }
        try {
            return IIdd.getService().addPluginEvent(pluginId, pkg, name,
                                                    listPayload, listPayload.size());
        } catch (RemoteException e) {
            mLog.e("Exception: " + e);
        }
        return -1;
    }

    /**
     * Dispatches an event by plugin to the IDD Daemon
     *
     * <p>
     * This is a completely asynchronous event.
     * </p>
     *
     * @param plugin the plugin that the event belongs to
     * @param msg the protobuf container for the event. This is typically an
     *            object of the type GeneratedMessage or GeneratedMessageLite,
     *            created from builder.build(). This parameter is an Object to
     *            avoid referencing libraries that the target application may
     *            not wish to use.
     */
    public static void addPluginEvent(Context context, Plugin plugin, Object msg) {
        if (msg == null)
            throw new IllegalArgumentException("IddPrivate.addPluginEvent(): illegal arg");
        Class<?> cls = msg.getClass();
        String pkg = cls.getPackage().getName();
        String name = cls.getSimpleName();
        try {
            Method meth = cls.getMethod("toByteArray");
            Object res = meth.invoke(msg);
            if (res instanceof byte[]) {
                addPluginEvent(context, plugin.getIntValue(), pkg, name, (byte[])res);
            } else {
                mLog.e("The return value of toByteArray was not a byte array");
            }
        } catch (NoSuchMethodException e) {
            mLog.e("The event passed in to addPluginEvent is not a protocol buffer");
        } catch (Exception e) {
            mLog.e("There was an error calling addPluginEvent(msg)");
        }
    }

    /**
     * Flush all data and generate a report in the output directory.
     */
    public static void flush(Context context) {
        if (!checkPermissionIddPrivate(context)) {
            mLog.e("flush was denied due to permission!");
            return;
        }
        try {
            IIdd.getService().flush();
        } catch (RemoteException e) {
            mLog.e("Exception: " + e);
        }
    }

    /**
     * Writes new configuration into idd input.
     *
     * @param path
     */
    public static void writeConfiguration(Context context, byte[] config) {
        if (config == null) {
            throw new IllegalArgumentException("IddPrivate.writeConfiguration(): illegal arg");
        }
        if (!checkPermissionIddPrivate(context)) {
            mLog.e("writeConfiguration was denied due to permission!");
            return;
        }
        ArrayList<Byte> listConfig = new ArrayList<>();
        for (byte b : config) {
            listConfig.add(Byte.valueOf(b));
        }
        try {
            IIdd.getService().writeConfig(listConfig, listConfig.size());
        } catch (RemoteException e) {
            mLog.e("Exception: " + e);
        }
    }

    /**
     * Notifies IDD about a configuration update. This should be called after
     * placing the configuration in IDD's incoming folder.
     */
    public static boolean notifyNewConfiguration(Context context) {
        if (!checkPermissionIddPrivate(context)) {
            mLog.e("notifyNewConfiguration was denied due to permission!");
            return false;
        }
        try {
            return (IIdd.getService().notifyConfig() != 0);
        } catch (RemoteException e) {
            mLog.e("Exception: " + e);
            return false;
        }
    }

    /**
     * Deliver the IMEI to the IDD Daemon
     *
     * @param imei
     */
    public static void setImei(Context context, String imei) {
        if (imei == null)
            throw new IllegalArgumentException("IddPrivate.setImei(): illegal arg");
        if (!checkPermissionIddPrivate(context)) {
            mLog.e("setImei was denied due to permission!");
            return;
        }
        try {
            IIdd.getService().setImei(imei);
        } catch (RemoteException e) {
            mLog.e("Exception: " + e);
        }
    }

    /**
     * Deliver the ANDROID_ID owned by the IDD agent to the IDD Daemon
     *
     * @param androidId
     */
    public static void setAndroidId(Context context, String androidId) {
        if (androidId == null)
            throw new IllegalArgumentException("IddPrivate.setAndroidId(): illegal arg");
        if (!checkPermissionIddPrivate(context)) {
            mLog.e("setAndroidId was denied due to permission!");
            return;
        }
        try {
            IIdd.getService().setAndroidId(androidId);
        } catch (RemoteException e) {
            mLog.e("Exception: " + e);
        }
    }

    /**
     * Deliver the ANDROID_ID owned by Xperia services to the IDD Daemon
     *
     * @param xsAndroidId
     */
    public static void setXsAndroidId(Context context, String xsAndroidId) {
        if (xsAndroidId == null)
            throw new IllegalArgumentException("IddPrivate.setXsAndroidId(): illegal arg");
        if (!checkPermissionIddPrivate(context)) {
            mLog.e("setXsAndroidId was denied due to permission!");
            return;
        }
        try {
            IIdd.getService().setXsAndroidId(xsAndroidId);
        } catch (RemoteException e) {
            mLog.e("Exception: " + e);
        }
    }

    /**
     * Deliver the GUID owned by Xperia services to the IDD Daemon
     *
     * @param xsGuId
     */
    public static void setXsGuid(Context context, String xsGuId) {
        if (xsGuId == null)
            throw new IllegalArgumentException("IddPrivate.setXsGuid(): illegal arg");
        if (!checkPermissionIddPrivate(context)) {
            mLog.e("setXsGuid was denied due to permission!");
            return;
        }
        try {
            IIdd.getService().setXsGuid(xsGuId);
        } catch (RemoteException e) {
            mLog.e("Exception: " + e);
        }
    }

    /**
     * Deliver the user consent level to the IDD Daemon
     *
     * @param userConsent
     */
    public static void setUserConsent(Context context, int userConsent) {
        if (!checkPermissionIddPrivate(context)) {
            mLog.e("setUserConsent was denied due to permission!");
            return;
        }
        try {
            IIdd.getService().setUserConsent(userConsent);
        } catch (RemoteException e) {
            mLog.e("Exception: " + e);
        }
    }

    /**
     * Deliver the serial number to the IDD Daemon
     *
     * @param serial number
     */
    public static void setSerialNumber(Context context, String serialNumber) {
        if (serialNumber == null)
            throw new IllegalArgumentException("IddPrivate.setSerialNumber(): illegal arg");
        if (!checkPermissionIddPrivate(context)) {
            mLog.e("setSerialNumber was denied due to permission!");
            return;
        }
        try {
            IIdd.getService().setSerialNumber(serialNumber);
        } catch (RemoteException e) {
            mLog.e("Exception: " + e);
        }
    }

    /**
     * Indicates if IDD has been permanently disabled.
     */
    public static boolean isDisabled(Context context) {
        if (!checkPermissionIddPrivate(context)) {
            mLog.e("isDisabled was denied due to permission!");
            return false;
        }
        return new File("/idd/output/idd_disabled").exists();
    }

    /**
     * Requests an upload of IDD data. This API should only be used by very few
     * users and access must be granted by FAT Diagnostics.
     *
     * @param ctx
     * @param withinTime Maximum seconds to wait before upload should be started
     *            0 => Immediate upload.
     */
    public static void forceUpload(Context ctx, int withinTime) {
        if (ctx == null) {
            throw new IllegalArgumentException();
        }
        if (!checkPermissionIddPrivate(ctx)) {
            mLog.e("forceUpload was denied due to permission!");
            return;
        }
        Intent intent = new Intent();
        intent.setAction(IddPrivate.INTENT_FORCE_UPLOAD);
        intent.setClassName("com.sonyericsson.idd.agent",
                "com.sonyericsson.idd.agent.IDDEnabledBroadcastReceiver");

        intent.putExtra(IddPrivate.KEY_UPLOAD_DELAY, withinTime);

        ctx.sendBroadcast(intent, PERMISSION_IDD_PRIVATE);
    }

    /**
     * Save Counters and Static data.
     * IDD will save Counters and Static data once in a every
     * configured time interval.
     * This API will force Save Counters and Static data.
     */
    public static void forceSave(Context context) {
        if (!checkPermissionIddPrivate(context)) {
            mLog.e("forceSave was denied due to permission!");
            return;
        }
        try {
            IIdd.getService().forceSave();
        } catch (RemoteException e) {
            mLog.e("Exception: " + e);
        }
    }

    /**
     * Set data collection enable or disable.
     *
     * @param enable: 1 is enable, 0 is disable
     */
    public static void setDataCollectionEnable(Context context, boolean enable) {
        if (!checkPermissionIddPrivate(context)) {
            mLog.e("setDataCollectionEnable was denied due to permission!");
            return;
        }
        int en = 0;
        if (enable) en = 1;
        try {
            IIdd.getService().setDataCollectionEnable(en);
        } catch (RemoteException e) {
            mLog.e("Exception: " + e);
        }
    }

    /**
     * Remove report.
     *
     * @param report_num: 0 is remove all reports, positive is specified report.
     */
    public static void removeReport(Context context, int report_num) {
        if (!checkPermissionIddPrivate(context)) {
            mLog.e("removeReport was denied due to permission!");
            return;
        }
        try {
            IIdd.getService().removeReport(report_num);
        } catch (RemoteException e) {
            mLog.e("Exception: " + e);
        }
    }

    /**
     * Get Device Id.
     *
     * @return device id
     */
    private static String deviceId;
    public static String getDeviceId(Context context) {
        if (!checkPermissionIddPrivate(context)) {
            mLog.e("getDeviceId was denied due to permission!");
            return null;
        }
        deviceId = "";
        try {
            IIdd.getService().getDeviceId(new getDeviceIdCallback() {
                    @Override
                    public void onValues (int res, vendor.semc.system.idd.V1_0.DeviceId di) {
                        if (res != 0) return;
                        deviceId = Arrays.toString(di.di);
                    }
                });
        } catch (RemoteException e) {
            mLog.e("Exception: " + e);
        }
        return deviceId;
    }

    /**
     * Get App Event Enabled Packages.
     *
     * @return enabled packages
     */
    private static String enabledPackages;
    public static String getAppEventEnabledPackages(Context context) {
        if (!checkPermissionIddPrivate(context)) {
            mLog.e("getAppEventEnabledPackages was denied due to permission!");
            return null;
        }
        enabledPackages = "";
        try {
            IIdd.getService().getAppEventEnabledPackages(new getAppEventEnabledPackagesCallback() {
                    @Override
                    public void onValues (int res, vendor.semc.system.idd.V1_0.EnabledPackages ep) {
                        if (res != 0) return;
                        enabledPackages = Arrays.toString(ep.ep);
                    }
                });
        } catch (RemoteException e) {
            mLog.e("Exception: " + e);
        }
        return enabledPackages;
    }

    /**
     * Step IDD file iterator.
     *
     * @param client_id: The IDD client id.
     * @param file_iterator: Current file iterator.
     *                       If 0, the file iterator will be set to first file.
     * @return File iterator Step file iterator to next file.
     *                       -1 if no more files or an error has occured.
     */
    public static int fileIteratorNext(int client_id, int file_iterator) {
        return -1;
    }

    /**
     * Returns the file identified by the specified client_id and file_iterator,
     * or null if the file does not exist or due to errors.
     *
     * <p>
     * NOTE: Since this method call is synchronous, it should not be called from
     * the main (UI) thread.
     * </p>
     *
     * @param client_id: The IDD client id.
     * @param file_iterator: The file iterator.
     * @return The content of the file, null if no file is
     *         available or on failure.
     */
    public static byte[] fileGet(int client_id, int file_iterator) {
        return null;
    }

    /**
     * Get data used in th the configuration request from the idd daemon.
     * The returned data is a ConfigRequest protobuf (See configuration.proto).
     *
     * @return The content of the protobuf, null if no file is available or on failure.
     */
    private static byte[] config;
    public static byte[] getConfigRequest(Context context) {
        if (!checkPermissionIddPrivate(context)) {
            mLog.e("getConfigRequest was denied due to permission!");
            return null;
        }
        config = null;
        try {
            final int CONFIG_REQUEST_RESPONSE_SIZE_MAX = (2 * 1024);
            IIdd.getService().configRequestGet(CONFIG_REQUEST_RESPONSE_SIZE_MAX,
                new configRequestGetCallback() {
                    @Override
                    public void onValues (int read_cr_size,
                            vendor.semc.system.idd.V1_0.ConfigRequest cr) {
                        if (0 < read_cr_size && read_cr_size < CONFIG_REQUEST_RESPONSE_SIZE_MAX) {
                            config = new byte[read_cr_size];
                            for (int i = 0; i < read_cr_size; i++) {
                                config[i] = cr.cr[i];
                            }
                        }
                    }
                });
        } catch (RemoteException e) {
            mLog.e("Exception: " + e);
        }
        return config;
    }

    /**
     * Set timezone for IDD report.
     */
    public static void setTimezone(Context context) {
        if (!checkPermissionIddPrivate(context)) {
            mLog.e("setTimezone was denied due to permission!");
            return;
        }
        try {
            IIdd.getService().setTimezone();
        } catch (RemoteException e) {
            mLog.e("Exception: " + e);
        }
    }

    /**
     * Returns the result code from the latest API call. This method is not
     * thread safe.
     *
     * @return 0 if successful, < 0 if not
     */
    public static int getLatestResult() {
        return 0;
    }

    /**
     * Check if the calling app is granted permission of com.sonyericsson.permission.IDD_PRIVATE.
     *
     * @return true if granted, false otherwise.
     */
    private static boolean checkPermissionIddPrivate(Context ctx) {
        if (ctx == null) return false;
        return ctx.checkSelfPermission(PERMISSION_IDD_PRIVATE) == PackageManager.PERMISSION_GRANTED;
    }

    public enum Plugin {
        IDD(0),
        XD(1),
        RCA_COMPACT(2),
        RCA_EXTENDED(3),
        RCA_NONCRASH(4),
        RCA_BATTERY (5),
        PDA(6);

        private final int value;

        Plugin(final int value) {
            this.value = value;
        }

        public int getIntValue() {
            return value;
        }
    };

    private static final Log mLog = Log.createLog("IddPrivate");

    public static final String INTENT_FORCE_UPLOAD = "com.sonyericsson.idd.FORCE_UPLOAD";

    public static final String PERMISSION_IDD_PRIVATE = "com.sonyericsson.permission.IDD_PRIVATE";

    public static final String KEY_PACKAGE = "package";

    public static final String KEY_NAME = "name";

    public static final String KEY_PATH = "path";

    public static final String KEY_NUMBER_OF_FILES = "numberOfFiles";

    public static final String KEY_UPLOAD_DELAY = "upload_delay";

    public static final String KEY_PATH_PREFIX = "path-";

    public static final String KEY_UUID_PREFIX = "uuid-";
}
