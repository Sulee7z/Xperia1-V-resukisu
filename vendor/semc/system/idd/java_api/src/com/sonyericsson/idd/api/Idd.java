/*
 * Copyright 2010 Sony Corporation
 */

package com.sonyericsson.idd.api;

import android.content.Context;

import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.IOException;
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Locale;

import vendor.semc.system.idd.V1_0.IIdd;

/**
 * <p>
 * In Device Diagnostics (IDD) Java API.
 * </p>
 *
 * IDD supports three different types of probes.
 * <ul>
 * <li>Counters - Simple mechanism to log integer values and increment them.</li>
 *
 * <li>Static Data Probes - useful when logging data that never/rarely changes,
 * e.g hardware information.</li>
 *
 * <li>Events - Provides a way to log any type of events. Payload is typically a
 * serialized Protocol buffer.</li>
 * </ul>
 *
 */
public final class Idd {

    /**
     * This class must not be instantiated
     */
    private Idd() {
    }

    /**
     * Sets the value of a counter. Repeated calls with the same package and
     * name will replace the value stored by any previous invocations
     *
     * <p>
     * This is a completely asynchronous event.
     * </p>
     *
     * @param pkg name of the package that the counter belongs to
     * @param name counter name, must be unique within the package
     * @param value the value to set
     */
    public static void setCounter(String pkg, String name, long value) {
        if (pkg == null || name == null)
            throw new IllegalArgumentException("Idd.setCounter(): illegal arg");
        try {
            IIdd.getService().setCounter(pkg, name, value);
        } catch (Exception e) {
            mLog.e("Exception: " + e);
        }
    }

    /**
     * Increments the value of a counter.
     *
     * <p>
     * This is a completely asynchronous event.
     * </p>
     *
     * @param pkg the package that the counter belongs to
     * @param name counter name, must be unique within the package
     * @param delta the value to add to the counter
     */
    public static void incCounter(String pkg, String name, int delta) {
        if (pkg == null || name == null)
            throw new IllegalArgumentException("Idd.incCounter(): illegal arg");
        try {
            IIdd.getService().incCounter(pkg, name, delta);
        } catch (Exception e) {
            mLog.e("Exception: " + e);
        }
    }

    /**
     * Sets the value of a static data probe Repeated calls with the same
     * package and name will replace the value stored by any previous
     * invocations
     *
     * <p>
     * This is a completely asynchronous event.
     * </p>
     *
     * @param pkg the package that the static data belongs to
     * @param name name of the static data, must be unique within the package
     * @param value the value to store
     */
    public static void setStaticData(String pkg, String name, String value) {
        if (pkg == null || name == null || value == null)
            throw new IllegalArgumentException("Idd.setStaticData(): illegal arg");
        try {
            IIdd.getService().setSdata(pkg, name, value);
        } catch (Exception e) {
            mLog.e("Exception: " + e);
        }
    }

    /**
     * Dispatches an event to the IDD Daemon
     *
     * <p>
     * This is a completely asynchronous event.
     * </p>
     *
     * @param pkg the package that the event belongs to
     * @param name name of the event
     * @param payload the payload of the event. Typically serialized protocol
     *            buffer data.
     */
    public static void addEvent(String pkg, String name, byte[] payload) {
        if (pkg == null || name == null || payload == null)
            throw new IllegalArgumentException("Idd.addEvent(): illegal arg");
        if (addEventImpl(pkg, name, payload) < 0) {
            mLog.e("addEvent failed for package: "+pkg+" and event: "+name);
        }
    }

    static int addEventImpl(String pkg, String name, byte[] payload) {
        ArrayList<Byte> listPayload = new ArrayList<>();
        for (byte b : payload) {
            listPayload.add(Byte.valueOf(b));
        }
        try {
            return IIdd.getService().addEvent(pkg, name, listPayload, listPayload.size());
        } catch (Exception e) {
            mLog.e("Exception: " + e);
        }
        return -1;
    }

    /**
     * Dispatches an event to the IDD Daemon
     *
     * <p>
     * This is a completely asynchronous event.
     * </p>
     *
     * @param msg the protobuf container for the event. This is typically an
     *            object of the type GeneratedMessage or GeneratedMessageLite,
     *            created from builder.build(). This parameter is an Object to
     *            avoid referencing libraries that the target application may
     *            not wish to use.
     */
    public static void addEvent(Object msg) {
        if (msg == null)
            throw new IllegalArgumentException("Idd.addEvent(): illegal arg");
        Class<?> cls = msg.getClass();
        String pkg = cls.getPackage().getName();
        String name = cls.getSimpleName();
        try {
            Method meth = cls.getMethod("toByteArray");
            Object res = meth.invoke(msg);
            if (res instanceof byte[]) {
                addEvent(pkg, name, (byte[])res);
            } else {
                mLog.e("The return value of toByteArray was not a byte array");
            }
        } catch (NoSuchMethodException e) {
            mLog.e("The event passed in to addEvent is not a protocol buffer");
        } catch (Exception e) {
            mLog.e("There was an error calling addEvent(msg)");
        }
    }

    /**
     * Dispatches an app data of JSON format to the IDD Daemon
     *
     * Pack package name, version name, version code, timestamp, payload type and payload
     * being coverted to String format for the app data, then turn it into a protocol buffer.
     * Timestamp is created here in ISO8601 format.
     *
     * <p>
     * This is a completely asynchronous event.
     * </p>
     *
     * @param packageName the package that the event belongs to
     * @param versionName the versionName of the package
     * @param versionCode the versionCode of the package
     * @param payload the JSON payload of the event. The data format will be
     *           defined by clients
     *
     * @since 2
     */
    public static void addAppDataJSON(String packageName, String versionName,
            int versionCode, JSONObject payload) {
        if (packageName == null || versionName == null || payload == null ||
            packageName.matches("[ \t\r\n]") || versionName.matches("[ \t\r\n]")) {
                mLog.e("addAppDataJSON(): illegal arg from "
                        + (packageName == null ? "package whose name is null" : packageName));
                throw new IllegalArgumentException("Idd.addAppDataJSON(): illegal arg");
        }
        byte[] byte_array = payload.toString().getBytes(StandardCharsets.UTF_8);
        ArrayList<Byte> listPayload = new ArrayList<>();
        for (byte b : byte_array) {
            listPayload.add(Byte.valueOf(b));
        }
        try {
            IIdd.getService().addAppEvent(packageName, versionName, versionCode, getTimestamp(),
                    PayloadFormat.JSON.getIntValue(), listPayload, listPayload.size());
        } catch (Exception e) {
            mLog.e("Exception: " + e);
        }
    }

    /**
     * Register to use the API for app usage data collection
     *
     * All apps to use addAppDataJSON(), which is meant for app usage data collection, shall
     * register its use, in order to make its app surely submit app usage data. Because there could
     * be the period when app usage data is not collectable depending on IDD configuration.
     *
     * By registering, app to submit app usage data are able to know if its own app usage data is
     * already collectable. Even if its own app usage data has not yet been collectable, its app
     * will know that its own app usage data get collectable by receiving the intent from IDD.
     *
     * Apps have to pass its own package name and class name of broadcast receiver. Class name of
     * broadcast receiver is necessary to receive the intent to notify that its own app usage data
     * get collectable.
     *
     * If respective app usage data specified with package name is already enabled then immediately
     * return 1, otherwise 0. In the case of 0, if respective app usage data get collectable then
     * the intent, "com.sonymobile.idd.APP_DATA_JSON_ENABLED", is sent to broadcast receiver passed
     * by its app. Even in the case of 1, once respective app usage data get uncollectable and again
     * get collectable, "com.sonymobile.idd.APP_DATA_JSON_ENABLED" is sent.
     *
     * <p>
     * This is a completely synchronous event.
     * </p>
     *
     * @param packageName the package name of the app which wants to submit app usage data
     * @param className the class name of the receiver to receive broadcast to notify that
     *        respective app usage data get collectable
     *
     * @return  1: App usage data corresponding to packageName is collectable.
     *          0: App usage data corresponding to packageName is not collectable.
     *
     *          When its app usage data change from uncollectable to collectable, the intent,
     *          "com.sonymobile.idd.APP_DATA_JSON_ENABLED", is sent to broadcast receiver
     *          corresponding to className.
     */
    public static int registerToUseAddAppDataJSON(String packageName, String className) {
        if (packageName == null || packageName.matches("[ \t\r\n]")) {
            mLog.e("registerToUseAddAppDataJSON(): illegal arg from "
                    + (packageName == null ? "package whose name is null" : packageName));
            throw new IllegalArgumentException("Idd.registerToUseAddAppDataJSON(): illegal arg");
        }
        try {
            return IIdd.getService().registerToUseAddAppEvent(packageName, className);
        } catch (Exception e) {
            mLog.e("Exception: " + e);
            return 0;
        }
    }

    private static String getTimestamp() {
        long now = System.currentTimeMillis();
        synchronized (Idd.class) {
            if (sDateFormat == null) {
                sDateFormat = new SimpleDateFormat(FORMAT_TIMESTAMP_ISO8601, Locale.US);
                sDate = new Date();
            }
            sDate.setTime(now);
            return sDateFormat.format(sDate);
        }
    }

    /**
     * Indicates if the probe is active or not, which may change during run-time
     * by the IDD framework.
     *
     * Note that a client does not need to query this information. In many
     * cases, it's better to just register the probe (using addEvent), and let
     * IDD discard the event instead. That will simplify the code significantly.
     *
     * If an error occurs, false will be returned.
     *
     * <p>
     * NOTE: Since this method call is synchronous, it should not be called from
     * the main (UI) thread.
     * </p>
     *
     * @param pkg the package that the probe belongs to
     * @param name name of the probe
     * @return true if the probe is active and false otherwise
     */
    public static boolean isProbeActive(String pkg, String name) {
        if (pkg == null || name == null)
            throw new IllegalArgumentException("Idd.isProbeActive(): illegal arg");
        try {
            return (IIdd.getService().isProbeActive(pkg, name) != 0);
        } catch (Exception e) {
            mLog.e("Exception: " + e);
            return false;
        }
    }

    /**
     * Indicates if IDD is active or not.
     *
     * Note that the entire IDD API can be used even though this call indicates
     * that IDD is inactive. In that case, the method calls will either do
     * nothing or return an error where applicable.
     *
     * If an error occurs while calling this method, false will be returned.
     *
     * <p>
     * NOTE: Since this method call is synchronous, it should not be called from
     * the main (UI) thread.
     * </p>
     *
     * @return true if IDD is active and false otherwise
     */
    public static boolean isActive() {
        try {
            return (IIdd.getService().isActive() != 0);
        } catch (Exception e) {
            mLog.e("Exception: " + e);
            return false;
        }
    }

    /**
     * Returns the configuration identified by the specified package and name,
     * or null if the configuration does not exist or due to errors.
     *
     * Most clients does not need configuration, but those that do should
     * subscribe to the broadcast CONFIGURATION_UPDATED_INTENT. Then
     * configuration may have changed. Currently the only way to know is to get
     * and compare.
     *
     * <p>
     * NOTE: Since this method call is synchronous, it should not be called from
     * the main (UI) thread.
     * </p>
     *
     * @param pkg the package that the configuration belongs to
     * @param name name of the configuration
     * @return The content of the configuration, null if no configuration is
     *         available or on failure.
     */
    public static byte[] getConfiguration(String pkg, String name) {
        if (pkg == null || name == null)
            throw new IllegalArgumentException("Idd.getConfiguration(): illegal arg");
        mLog.e("getConfiguration: pkg: " + pkg + " name: " + name);
        try (android.os.NativeHandle handle = IIdd.getService().getConfig(pkg, name)) {
            mLog.e("getConfiguration: handle: " + handle);
            FileDescriptor fd = handle.getFileDescriptor();
            byte[] ret = convertFileDescriptor(fd);
            return ret;
        } catch (IOException ioe) {
            mLog.e("IOException " + pkg + "." + name + " : " + ioe);
        } catch (Exception e) {
            mLog.e("Exception " + pkg + "." + name + " : " + e);
        }
        return null;
    }

    private static byte[] convertFileDescriptor(FileDescriptor fd) {
        try (FileInputStream inputStream = new FileInputStream(fd);
             ByteArrayOutputStream bout = new ByteArrayOutputStream();) {
            byte[] buffer = new byte[1024];
            int len = 0;
            while((len = inputStream.read(buffer)) != -1) {
                bout.write(buffer, 0, len);
            }
            return bout.toByteArray();
        } catch (Exception e) {
            mLog.e("Exception: " + e);
            return null;
        }
    }

    // Stubs for removed functions
    // TODO: Remove completely

    public static String addFile(Context ctx, String pkg, String name, String path) {
        return  null;
    }

    public static String[] addFileSet(Context ctx, String pkg, String name, String[] paths) {
        return null;
    }

    /**
     * Returns the maximum size of the payload for an IDD event.
     * IDD will throw away events with too large payloads.
     * They will not be stored in an IDD report.
     * This functionallity is not implemented through the IddHidlCli api.
     *
     * @return The maxsize of the IDD payload.
     */
    public static int getMaxPayload() {
        // IDD_MAX_EVENT_SIZE = (IDD_MAX_PACKET_SIZE - IDD_MAX_PROBE_ID_SIZE)
        // (255 * 1024) - 256 = 260864
        return 260864;
    }

    /**
     * Shall data washing of personal data be done?.
     * Non prototypes shall wash personal data. However the IDD configuration flag
     * iddd.Settings.wash_data can be used to disable data washing.
     *
     * Returns true if data washing shall be done, false if no washing shall be done.
     */
    public static boolean doWashing() {
        try {
            return (IIdd.getService().doWashing() != 0);
        } catch (Exception e) {
            mLog.e("Exception: " + e);
            return false;
        }
    }

    /**
     * Returns the result code from the latest API call. As all threads share
     * the same IDD instance, care should be taken if calls are interleaved from
     * different threads.
     *
     * @return 0 if successful, < 0 if not
     */
    public static int getLatestResult() {
        return 0;
    }

    private static final String FORMAT_TIMESTAMP_ISO8601 = "yyyy-MM-dd'T'HH:mm:ss.SSSZ";
    private static SimpleDateFormat sDateFormat;
    private static Date sDate;

    private enum PayloadFormat {
        PLAIN_TEXT(0),
        JSON(1);

        private final int value;

        private PayloadFormat(final int value) {
            this.value = value;
        }

        public int getIntValue() {
            return value;
        }
    };

    private static final Log mLog = Log.createLog("Idd");
}
