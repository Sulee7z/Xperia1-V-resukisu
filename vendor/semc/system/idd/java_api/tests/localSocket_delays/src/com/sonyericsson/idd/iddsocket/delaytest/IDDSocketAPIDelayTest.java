/*
 * Copyright 2012 Sony Corporation
 */

package com.sonyericsson.idd.iddsocket.delaytest;

import com.sonyericsson.idd.api.IddSocket;
import android.test.AndroidTestCase;

/**
 * Test cases for the IDD Socket API when client uses IDD Socket API not JNI
 * when the IDD daemon is running with the'socket-delays' switch.
 * Since we cannot restart the daemon from within an Java application.
 * This is done with a shell script or the daemon must be restarted
 * manually with the 'socket-delays' switch: $ stop iddd $ iddd --socket-delays 5000:5000
 *
 */
public class IDDSocketAPIDelayTest extends AndroidTestCase {

    private static final String mTestPkg = "com.test.performance";

    private static final String mTestName = "javaAPI";

    @Override
    public void setUp() throws Exception {
        super.setUp();
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
    }

    /**
     * Verifies that the expected timeout when calling
     * getDeviceId() with timeout. This test
     * relies on the SOCKET_TIMEOUT in Idd Socket being 3 seconds.
     */
    public void testgetDeviceIdResCode() {
        long millis = System.currentTimeMillis();
        String deviceID = IddSocket.getDeviceId();
        millis = System.currentTimeMillis()- millis;
        int resCode = IddSocket.getLatestResult();
        assertTrue(String.format("Timeout out of bounds (got %d ms)", millis), millis < 4000 && 2000 < millis);
    }

    /**
     * Verifies that the expected timeout when calling
     * getConfiguration() with timeout. This test
     * relies on the SOCKET_TIMEOUT in Idd Socket being 3 seconds.
     */
    public void testGetConfigurationResCode() {
        long millis = System.currentTimeMillis();
        byte[] configuration = IddSocket.getConfiguration(mTestPkg, mTestName);
        millis = System.currentTimeMillis()- millis;
        int resCode = IddSocket.getLatestResult();
        assertTrue(String.format("Timeout out of bounds (got %d ms)", millis), millis < 4000 && 2000 < millis);
    }

    /**
     * Verifies that the expected timeout when calling
     * isActive() with timeout. This test
     * relies on the SOCKET_TIMEOUT in Idd Socket being 3 seconds.
     */
    public void testisActiveResCode() {
        long millis = System.currentTimeMillis();
        boolean isActive = IddSocket.isActive();
        millis = System.currentTimeMillis()- millis;
        int resCode = IddSocket.getLatestResult();
        assertTrue(String.format("Timeout out of bounds (got %d ms)", millis), millis < 4000 && 2000 < millis);
    }

    /**
     * Verifies that the expected timeout when calling
     * isProbeActive() with timeout. This test
     * relies on the SOCKET_TIMEOUT in Idd Socket being 3 seconds.
     */
    public void testisProbeActiveResCode() {
        long millis = System.currentTimeMillis();
        boolean isProbeActive = IddSocket.isProbeActive(mTestPkg, mTestName);
        millis = System.currentTimeMillis()- millis;
        int resCode = IddSocket.getLatestResult();
        assertTrue(String.format("Timeout out of bounds (got %d ms)", millis), millis < 4000 && 2000 < millis);
    }
}

