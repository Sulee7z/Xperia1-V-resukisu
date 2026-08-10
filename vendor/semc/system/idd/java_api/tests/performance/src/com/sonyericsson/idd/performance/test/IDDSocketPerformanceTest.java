/*
 * Copyright 2012 Sony Corporation
 */

package com.sonyericsson.idd.performance.test;

import com.sonyericsson.idd.api.Idd;
import com.sonyericsson.idd.api.IddPrivate;
import com.sonyericsson.idd.api.IddSocket;
import android.test.AndroidTestCase;
import android.util.Log;

/**
 * Test cases for Socket API performance,
 * one is the performance of API calls, another is the performance
 * of API function
 *
 */
public class IDDSocketPerformanceTest extends AndroidTestCase {

    private static final String mTestPkg = "com.test.performance";

    private static final String mTestName = "javaAPI";

    private static final int COUNTER_NUM = 10;

    private static final int COUNTER_DELTA = 1;

    private static final int LOOP_TIMES = 1000;

    private static final int SLEEP_TIME = 4000;

    private static final String mStaticData = "staticData";

    private static final String mImei = "004402143810574";

    private static final String mSerialNumber = "CB5A1JVM7C";

    private static byte[] mPayload = null;

    //Set 5ms interval in order to clear influence in loop
    private static int mIntervals = 5;

    private static final String LOG_TAG = "IDD Sock perf";

    @Override
    public void setUp() throws Exception {
        super.setUp();
        IddSocket.removeReport(0);
        //Generate 2k buffer for addEvent
        mPayload = new byte[1024 * 2];
        for (int index = 0; index < 1024 * 2; index++) {
            mPayload[index] = 0x28;
        }
        Util.threadSleep(SLEEP_TIME);
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
        IddSocket.removeReport(0);
        Util.threadSleep(SLEEP_TIME);
    }

    /**
     * Verifies the performance of setCounter() function.
     */
    public void testSetCounterFunction() {
        //Remove old records in order to clear impact among different cases
        Util.removePerformanceLog();
        long totalTime = 0;
        long startTime = 0;
        //We set non strict target in order to run in different platform, user could see actual value by logcat
        int setCounterFunctionTarget = 10000;

        for (int times = 0; times < LOOP_TIMES; times++) {
            startTime += System.currentTimeMillis() * 1000;
            IddSocket.setCounter(mTestPkg, mTestName, COUNTER_NUM);
            Util.threadSleep(mIntervals);
        }

        totalTime = Util.getPerformanceTime("CS-end: " + mTestPkg + ": ", startTime, LOOP_TIMES);
        Log.d(LOG_TAG, "Run 'IddSocket.setCounter'(function) " + LOOP_TIMES + " times, total time is " + totalTime / 1000 + " ms");
        assertTrue(String.format("'IddSocket.setCounter'(function) costs much time, got %d ms, but target is %d ms", totalTime / 1000, setCounterFunctionTarget), totalTime / 1000 <= setCounterFunctionTarget);
    }

    /**
     * Verifies the performance of setCounter() calls.
     */
    public void testSetCounterAPI() {
        long totalTime = 0;
        long startTime = 0;
        //We set non strict target in order to run in different platform, user could see actual value by logcat
        int setCounterAPITarget = 5000;

        for (int times = 0; times < LOOP_TIMES; times++) {
            startTime = System.nanoTime();
            IddSocket.setCounter(mTestPkg, mTestName, COUNTER_NUM);
            totalTime += (System.nanoTime() / 1000 - startTime / 1000);
            Util.threadSleep(mIntervals);
        }

        Log.d(LOG_TAG, "Run 'IddSocket.setCounter'(calls) " + LOOP_TIMES + " times, total time is " + totalTime / 1000 + " ms");
        assertTrue(String.format("'IddSocket.setCounter'(calls) costs much time, got %d ms, but target is %d ms", totalTime / 1000, setCounterAPITarget), totalTime / 1000 <= setCounterAPITarget);
    }

    /**
     * Verifies the performance of incCounter() function.
     */
    public void testIncCounterFunction() {
        //Remove old records in order to clear impact among different cases
        Util.removePerformanceLog();
        long totalTime = 0;
        long startTime = 0;
        //We set non strict target in order to run in different platform, user could see actual value by logcat
        int incCounterFunctionTarget = 10000;

        for (int times = 0; times < LOOP_TIMES; times++) {
            startTime += System.currentTimeMillis() * 1000;
            IddSocket.incCounter(mTestPkg, mTestName, COUNTER_DELTA);
            Util.threadSleep(mIntervals);
        }

        totalTime = Util.getPerformanceTime("CI-end: " + mTestPkg + ": ", startTime, LOOP_TIMES);
        Log.d(LOG_TAG, "Run 'IddSocket.incCounter'(function) " + LOOP_TIMES + " times, total time is " + totalTime / 1000 + " ms");
        assertTrue(String.format("'IddSocket.incCounter'(function) costs much time, got %d ms, but target is %d ms", totalTime / 1000, incCounterFunctionTarget), totalTime / 1000 <= incCounterFunctionTarget);
    }

    /**
     * Verifies the performance of incCounter() calls.
     */
    public void testIncCounterAPI() {
        long totalTime = 0;
        long startTime = 0;
        //We set non strict target in order to run in different platform, user could see actual value by logcat
        int incCounterAPITarget = 5000;

        for (int times = 0; times < LOOP_TIMES; times++) {
            startTime = System.nanoTime();
            IddSocket.incCounter(mTestPkg, mTestName, COUNTER_DELTA);
            totalTime += (System.nanoTime() / 1000 - startTime / 1000);
            Util.threadSleep(mIntervals);
        }

        Log.d(LOG_TAG, "Run 'IddSocket.incCounter'(calls) " + LOOP_TIMES + " times, total time is " + totalTime / 1000 + " ms");
        assertTrue(String.format("'IddSocket.incCounter'(calls) costs much time, got %d ms, but target is %d ms", totalTime / 1000, incCounterAPITarget), totalTime / 1000 <= incCounterAPITarget);
    }

    /**
     * Verifies the performance of addEvent() function.
     */
    public void testAddEventFunction() {
        //Remove old records in order to clear impact among different cases
        Util.removePerformanceLog();
        long totalTime = 0;
        long startTime = 0;
        //We set non strict target in order to run in different platform, user could see actual value by logcat
        int addEventFunctionTarget = 10000;

        for (int times = 0; times < LOOP_TIMES; times++) {
            startTime += System.currentTimeMillis() * 1000;
            IddSocket.addEvent(mTestPkg, mTestName, mPayload);
            Util.threadSleep(mIntervals);
        }

        totalTime = Util.getPerformanceTime("EV-end: " + mTestPkg + ": ", startTime, LOOP_TIMES);
        Log.d(LOG_TAG, "Run 'IddSocket.addEvent'(function) " + LOOP_TIMES + " times, total time is " + totalTime / 1000 + " ms");
        assertTrue(String.format("'IddSocket.addEvent'(function) costs much time, got %d ms, but target is %d ms", totalTime / 1000, addEventFunctionTarget), totalTime / 1000 <= addEventFunctionTarget);
    }

    /**
     * Verifies the performance of addEvent() calls.
     */
    public void testAddEventAPI() {
        long totalTime = 0;
        long startTime = 0;
        //We set non strict target in order to run in different platform, user could see actual value by logcat
        int addEventAPITarget = 5000;

        for (int times = 0; times < LOOP_TIMES; times++) {
            startTime = System.nanoTime();
            IddSocket.addEvent(mTestPkg, mTestName, mPayload);
            totalTime += (System.nanoTime() / 1000 - startTime / 1000);
            Util.threadSleep(mIntervals);
        }

        Log.d(LOG_TAG, "Run 'IddSocket.addEvent'(calls) " + LOOP_TIMES + " times, total time is " + totalTime / 1000 + " ms");
        assertTrue(String.format("'IddSocket.addEvent'(calls) costs much time, got %d ms, but target is %d ms", totalTime / 1000, addEventAPITarget), totalTime / 1000 <= addEventAPITarget);
    }

    /**
     * Verifies the performance of setStaticData() function.
     */
    public void testSetStaticDataFunction() {
        //Remove old records in order to clear impact among different cases
        Util.removePerformanceLog();
        long totalTime = 0;
        long startTime = 0;
        //We set non strict target in order to run in different platform, user could see actual value by logcat
        int setStaticDataFunctionTarget = 10000;

        for (int times = 0; times < LOOP_TIMES; times++) {
            startTime += System.currentTimeMillis() * 1000;
            IddSocket.setStaticData(mTestPkg, mTestName, mStaticData);
            Util.threadSleep(mIntervals);
        }

        totalTime = Util.getPerformanceTime("SD-end: " + mTestPkg + ": ", startTime, LOOP_TIMES);
        Log.d(LOG_TAG, "Run 'IddSocket.setStaticData'(function) " + LOOP_TIMES + " times, total time is " + totalTime / 1000 + " ms");
        assertTrue(String.format("'IddSocket.setStaticData'(function) costs much time, got %d ms, but target is %d ms", totalTime / 1000, setStaticDataFunctionTarget), totalTime / 1000 <= setStaticDataFunctionTarget);
    }

    /**
     * Verifies the performance of setStaticData() calls.
     */
    public void testSetStaticDataAPI() {
        long totalTime = 0;
        long startTime = 0;
        //We set non strict target in order to run in different platform, user could see actual value by logcat
        int setStaticDataAPITarget = 5000;

        for (int times = 0; times < LOOP_TIMES; times++) {
            startTime = System.nanoTime();
            IddSocket.setStaticData(mTestPkg, mTestName, mStaticData);
            totalTime += (System.nanoTime() / 1000 - startTime / 1000);
            Util.threadSleep(mIntervals);
        }

        Log.d(LOG_TAG, "Run 'IddSocket.setStaticData'(calls) " + LOOP_TIMES + " times, total time is " + totalTime / 1000 + " ms");
        assertTrue(String.format("'IddSocket.setStaticData'(calls) costs much time, got %d ms, but target is %d ms", totalTime / 1000, setStaticDataAPITarget), totalTime / 1000 <= setStaticDataAPITarget);
    }

    /**
     * Verifies the performance of flush() function.
     */
    public void testFlushFunction() {
        long totalTime = 0;
        long startTime = 0;
        int flushTimes = 10;
        //In flush we need large intervals
        int flushIntervals = 100;
        //We set non strict target in order to run in different platform, user could see actual value by logcat
        int flushFunctionTarget = 1000;
        //Remove old records in order to clear impact among different cases
        Util.removePerformanceLog();

        for (int times = 0; times < flushTimes; times++) {
            IddSocket.addEvent(mTestPkg, mTestName, mPayload);
            startTime += System.currentTimeMillis() * 1000;
            IddSocket.flush();
            Util.threadSleep(flushIntervals);
        }

        totalTime = Util.getPerformanceTime("FL-end: test: ", startTime, flushTimes);
        Log.d(LOG_TAG, "Run 'IddSocket.flush'(function) " + flushTimes + " times, total time is " + totalTime / 1000 + " ms");
        assertTrue(String.format("'IddSocket.flush'(function) costs much time, got %d ms, but target is %d ms", totalTime / 1000, flushFunctionTarget), totalTime / 1000 <= flushFunctionTarget);
    }

    /**
     * Verifies the performance of flush() calls.
     */
    public void testFlushAPI() {
        long totalTime = 0;
        long startTime = 0;
        int flushTimes = 10;
        //In flush we need large intervals
        int flushIntervals = 100;
        //We set non strict target in order to run in different platform, user could see actual value by logcat
        int flushAPITarget = 100;

        for (int times = 0; times < flushTimes; times++) {
            startTime = System.nanoTime();
            IddSocket.flush();
            totalTime += (System.nanoTime() / 1000 - startTime / 1000);
            Util.threadSleep(flushIntervals);
        }

        Util.threadSleep(SLEEP_TIME);
        Log.d(LOG_TAG, "Run 'IddSocket.flush'(calls) " + flushTimes + " times, total time is " + totalTime / 1000 + " ms");
        assertTrue(String.format("'IddSocket.flush'(calls) costs much time, got %d ms, but target is %d ms", totalTime / 1000, flushAPITarget), totalTime / 1000 <= flushAPITarget);
    }

    /**
     * Verifies the performance of getConfiguration() function.
     * due to Daemon doesn't return value if fd is -1, we use "iddd", "FilterConfig" in order to get return value from Daemon.
     */
    public void testGetConfiguration() {
        long totalTime = 0;
        long startTime = 0;
        byte[] configuration = null;
        int getConfigurationTimes = 10;
        //In getConfiguration we need large intervals
        int getConfigurationIntervals = 50;
        //We set non strict target in order to run in different platform, user could see actual value by logcat
        int getConfigurationTarget = 500;

        for (int times = 0; times < getConfigurationTimes; times++) {
            startTime = System.nanoTime();
            configuration = IddSocket.getConfiguration("iddd", "FilterConfig");
            totalTime += (System.nanoTime() / 1000 - startTime / 1000);
            Util.threadSleep(getConfigurationIntervals);
        }

        Log.d(LOG_TAG, "Run 'IddSocket.getConfiguration'(function) " + getConfigurationTimes + " times, total time is " + totalTime / 1000 + " ms");
        assertTrue(String.format("'IddSocket.getConfiguration'(function) costs much time, got %d ms, but target is %d ms", totalTime / 1000, getConfigurationTarget), totalTime / 1000 <= getConfigurationTarget);
    }
}
