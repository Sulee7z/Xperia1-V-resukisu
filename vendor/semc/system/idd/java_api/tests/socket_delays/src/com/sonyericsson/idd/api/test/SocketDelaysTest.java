/*
 * Copyright 2010 Sony Corporation
 */

package com.sonyericsson.idd.api.test;

import com.sonyericsson.idd.api.Idd;
import com.sonyericsson.idd.api.IddPrivate;
import android.test.AndroidTestCase;

/**
 * Test cases for the IDD API when the IDD daemon is running with the
 * 'socket-delays' switch. Since we cannot restart the daemon from within an
 * Java application this is done with a shell script ('runtests') i.e. these
 * tests must either be run using that script or the daemon must be restarted
 * manually with the 'socket-delays' switch: $ stop iddd $ iddd --socket-delays
 * 5000:5000
 *
 * @author Lars Brange (lars.brange@sonyericsson.com)
 *
 */
public class SocketDelaysTest extends AndroidTestCase {

    // 'socket-delays' value when starting the IDD daemon (ms). If changed in
    // the 'runtests' script
    // then this value must also be changed.
    private static final int IDD_DAEMON_SOCKET_DELAY = 5000;

    // Expected result code. Also see errno-base.h and errno.h in:
    // <repo_root>/external/kernel-headers/original/asm-generic/
    private static final int EAGAIN = -11;

    private static final String mTestPkg = "com.jabba.dabba";

    private static final String mTestName = "doooo";

    private final int API_CALL_COUNT = 20;

    @Override
    public void setUp() throws Exception {
        super.setUp();
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
    }

    /**
     * Verifies that we get EAGAIN when getting a time-out while waiting for a
     * response in idd_is_probe_active() in java_api.c. This test uses the
     * SEQPACKET type of socket. Also verifies the time-out length. This test
     * relies on the SOCKET_TIMEOUT in idd_api.c being 3 seconds.
     */
    public void testSEQRescodeAndTimeout() {
        long millis = System.currentTimeMillis();
        Idd.isProbeActive(mTestPkg, mTestName);
        int resCode = Idd.getLatestResult();
        millis = System.currentTimeMillis() - millis;
        assertEquals(String.format("Got unexpected result code %d from API method", resCode),
                EAGAIN, resCode);
        assertTrue(String.format("Timeout out of bounds (got %d ms)", millis), millis >= 2000
                && millis <= 4000);
    }

    /**
     * Verifies that the decoding of a failed result code when calling
     * getConfiguration() is correct.
     */
    public void testGetConfigurationResCode() {
        Idd.getConfiguration(mTestPkg, mTestName);
        int resCode = Idd.getLatestResult();
        assertEquals(String.format("Got unexpected result code %d from API method", resCode),
                EAGAIN, resCode);
    }

    /**
     * Verifies that we can send a bunch of commands over the DGRAM socket in
     * quick succession but that we finally gets timeouts. The number of
     * commands we can send before getting a timeout is controlled by the event
     * queue in epoll_wait() in iddd.c and the timeouts in idd_api.c and in
     * iddd.c (socket test mode). NOTE: It takes a long time to execute this
     * test (at least 200 s).
     */
    public void testDGRAMPublicRescodeAndTimeout() {
        int okCount = 0;
        int toCount = 0;
        int otherCount = 0;
        byte[] dummyPayload = new byte[] {
                0x34, 0x00, 0x56, (byte)0xAB, (byte)0xDE, (byte)0xAD
        };
        for (int count = 0; count < API_CALL_COUNT; ++count) {
            switch (count % 4) {
                case 0:
                    Idd.setCounter(mTestPkg, mTestName + count, 1);
                    break;
                case 1:
                    Idd.incCounter(mTestPkg, mTestName + count, 1);
                    break;
                case 2:
                    Idd.setStaticData(mTestPkg, mTestName, "Pebbles" + count);
                    break;
                case 3:
                    Idd.addEvent(mTestPkg, mTestName, dummyPayload);
                    break;
            }
            switch (Idd.getLatestResult()) {
                case 0:
                    ++okCount;
                    break;
                case EAGAIN:
                    ++toCount;
                    break;
                default:
                    ++otherCount;
            }
        }
        assertAPICallCounts(okCount, toCount, otherCount);
    }

    /**
     * Same as above but for the private API
     */
    public void testDGRAMPrivateRescodeAndTimeout() {
        int okCount = 0;
        int toCount = 0;
        int otherCount = 0;
        for (int count = 0; count < API_CALL_COUNT; ++count) {
            switch (count % 3) {
                case 0:
                    IddPrivate.flush();
                    break;
                case 1:
                    IddPrivate.setSerialNumber("not a serial number");
                    break;
                case 2:
                    IddPrivate.setImei("not an IMEI");
                    break;
            }
            switch (IddPrivate.getLatestResult()) {
                case 0:
                    ++okCount;
                    break;
                case EAGAIN:
                    ++toCount;
                    break;
                default:
                    ++otherCount;
            }
        }
        assertAPICallCounts(okCount, toCount, otherCount);
    }

    private void assertAPICallCounts(int okCount, int toCount, int otherCount) {
        assertTrue(String.format("Too few calls (%d) returned 0 (i.e. OK)", okCount), okCount >= 10);
        assertTrue(String.format("Too few calls (%d) timedout", toCount), toCount >= 4);
        assertTrue(String.format("Got %d unexpected result codes", otherCount), otherCount == 0);

        // We must wait until the daemon is done so other tests start in a
        // consistent state.
        synchronized (this) {
            try {
                wait(API_CALL_COUNT * IDD_DAEMON_SOCKET_DELAY);
            } catch (InterruptedException intEx) {
                assertFalse("Interrupted while waiting", true);
            }
        }
    }
}