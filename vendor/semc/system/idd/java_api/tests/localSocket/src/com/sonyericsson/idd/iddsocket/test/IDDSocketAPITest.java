/*
 * Copyright 2012 Sony Corporation
 */

package com.sonyericsson.idd.iddsocket.test;

import com.sonyericsson.idd.api.IddSocket;
import android.test.AndroidTestCase;

/**
 * Test cases for the IDD Socket API when client uses IDD Socket API not JNI.
 *
 */
public class IDDSocketAPITest extends AndroidTestCase {

    // Expected result code.
    private static final int IO_OK = 0;

    // Unexpected result code: IO Exception.
    private static final int IO_EX = -1;

    private static final String mTestPkg = "com.test.performance";

    private static final String mTestName = "javaAPI";

    private static final int mCounter = 10;

    private static final int mDeltaCounter = 1;

    private static final byte[] mDummyPayload = new byte[] {
            0x34, 0x00, 0x56, (byte)0xAB, (byte)0xDE, (byte)0xAD
    };

    private static final String mStaticData = "staticData";

    private static final String mImei = "004402143810574";

    private static final String mSerialNumber = "CB5A1JVM7C";

    @Override
    public void setUp() throws Exception {
        super.setUp();
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
    }

    /**
     * Verifies that the expected result code when calling
     * setCounter() is correct.
     */
    public void testsetCounterResCode() {
        IddSocket.setCounter(mTestPkg, mTestName, mCounter);
        int resCode = IddSocket.getLatestResult();
        assertEquals(String.format("Got unexpected result code %d from IDD Socket API method: setCounter", resCode),
                IO_OK, resCode);
    }

    /**
     * Verifies that the expected result code when calling
     * incCounter() is correct.
     */
    public void testincCounterResCode() {
        IddSocket.incCounter(mTestPkg, mTestName, mDeltaCounter);
        int resCode = IddSocket.getLatestResult();
        assertEquals(String.format("Got unexpected result code %d from IDD Socket API method: incCounter", resCode),
                IO_OK, resCode);
    }

    /**
     * Verifies that the expected result code when calling
     * addEvent() is correct.
     */
    public void testaddEventResCode() {
        IddSocket.addEvent(mTestPkg, mTestName, mDummyPayload);
        int resCode = IddSocket.getLatestResult();
        assertEquals(String.format("Got unexpected result code %d from IDD Socket API method: addEvent", resCode),
                IO_OK, resCode);
    }

    /**
     * Verifies that the expected result code when calling
     * setStaticData() is correct.
     */
    public void testsetStaticDataResCode() {
        IddSocket.setStaticData(mTestPkg, mTestName, mStaticData);
        int resCode = IddSocket.getLatestResult();
        assertEquals(String.format("Got unexpected result code %d from IDD Socket API method: setStaticData", resCode),
                IO_OK, resCode);
    }

    /**
     * Verifies that the expected result code when calling
     * flush() is correct.
     */
    public void testflushResCode() {
        IddSocket.flush();
        int resCode = IddSocket.getLatestResult();
        assertEquals(String.format("Got unexpected result code %d from IDD Socket API method: flush", resCode),
                IO_OK, resCode);
    }

    /**
     * Verifies that the expected result code when calling
     * forceSave() is correct.
     */
    public void testforceSaveResCode() {
        IddSocket.forceSave();
        int resCode = IddSocket.getLatestResult();
        assertEquals(String.format("Got unexpected result code %d from IDD Socket API method: forceSave", resCode),
                IO_OK, resCode);
    }

    /**
     * Verifies that the expected result code when calling
     * notifyNewConfiguration() is correct.
     */
    public void testnotifyNewConfigurationResCode() {
        IddSocket.notifyNewConfiguration();
        int resCode = IddSocket.getLatestResult();
        assertEquals(String.format("Got unexpected result code %d from IDD Socket API method: notifyNewConfiguration", resCode),
                IO_OK, resCode);
    }

    /**
     * Verifies that the expected result code when calling
     * setDataCollectionEnable() is correct.
     */
    public void testsetDataCollectionEnableResCode() {
        IddSocket.setDataCollectionEnable(true);
        int resCode = IddSocket.getLatestResult();
        assertEquals(String.format("Got unexpected result code %d from IDD Socket API method: setDataCollectionEnable", resCode),
                IO_OK, resCode);
    }

    /**
     * Verifies that the expected result code when calling
     * removeReport() is correct.
     */
    public void testremoveReportResCode() {
        IddSocket.removeReport(0);
        int resCode = IddSocket.getLatestResult();
        assertEquals(String.format("Got unexpected result code %d from IDD Socket API method: removeReport", resCode),
                IO_OK, resCode);
    }

    /**
     * Verifies that the expected result code when calling
     * setImei() is correct.
     */
    public void testsetImeiResCode() {
        IddSocket.setImei(mImei);
        int resCode = IddSocket.getLatestResult();
        assertEquals(String.format("Got unexpected result code %d from IDD Socket API method: setImei", resCode),
                IO_OK, resCode);
    }

    /**
     * Verifies that the expected result code when calling
     * setSerialNumber() is correct.
     */
    public void testsetSerialNumberResCode() {
        IddSocket.setSerialNumber(mSerialNumber);
        int resCode = IddSocket.getLatestResult();
        assertEquals(String.format("Got unexpected result code %d from IDD Socket API method: setSerialNumber", resCode),
                IO_OK, resCode);
    }

    /**
     * Verifies that the expected result code when calling
     * isActive() is correct.
     */
    public void testisActiveResCode() {
        boolean isActive = IddSocket.isActive();
        int resCode = IddSocket.getLatestResult();
        assertEquals(String.format("Got unexpected result code %d from IDD Socket API method: isActive", resCode),
                IO_OK, resCode);
    }

    /**
     * Verifies that the expected result code when calling
     * isProbeActive() is correct.
     */
    public void testisProbeActiveResCode() {
        boolean isProbeActive = IddSocket.isProbeActive(mTestPkg, mTestName);
        int resCode = IddSocket.getLatestResult();
        assertEquals(String.format("Got unexpected result code %d from IDD Socket API method: isProbeActive", resCode),
                IO_OK, resCode);
    }

    /**
     * Verifies that the expected result code when calling
     * getDeviceId() is correct.
     */
    public void testgetDeviceIdResCode() {
        String deviceID = IddSocket.getDeviceId();
        int resCode = IddSocket.getLatestResult();
        assertEquals(String.format("Got unexpected result code %d from IDD Socket API method: getDeviceId", resCode),
                IO_OK, resCode);
    }

    /**
     * Verifies that the expected result code when calling
     * getConfiguration() is correct.
     * due to Daemon doesn't return value if fd is -1, we use "idd", "AgentConfig" in order to get return value from Daemon.
     */
    public void testGetConfigurationResCode() {
        byte[] configuration = IddSocket.getConfiguration("iddd", "FilterConfig");
        int resCode = IddSocket.getLatestResult();
        assertEquals(String.format("Got unexpected result code %d from IDD Socket API method: getConfiguration", resCode),
                IO_OK, resCode);
    }
}
