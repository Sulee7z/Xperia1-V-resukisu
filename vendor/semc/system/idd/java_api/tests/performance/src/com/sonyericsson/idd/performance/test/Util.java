/*
 * Copyright 2012 Sony Corporation
 */

package com.sonyericsson.idd.performance.test;

import junit.framework.Assert;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;

/**
 * Utilities
 */

public class Util {

    public static void threadSleep(int millisSecond) {
        try {
            Thread.sleep(millisSecond);
        } catch (InterruptedException ex) {
            Assert.assertFalse(String.format("Failed, exception happened when sleep!"), true);
        }
    }

    public static void removePerformanceLog() {
        try {
            File performanceLogFile = null;
            if ((performanceLogFile = new File("/sdcard/performanceTest.log")).exists()) {
                int times = 20;
                while (0 < times) {
                    if (performanceLogFile.delete()) return;
                    threadSleep(10);
                    times--;
                }
                if (0 == times) throw new IOException("Couldn't remove performance log");
            }
        } catch (IOException ex) {
            Assert.assertFalse(String.format("Failed, the IOException is: %s", ex), true);
        }
    }

    /**
     * Get the time when daemon finished data processing,
     * it is usec.
     */
    public static long getPerformanceTime(String prefix, long totalStartTime, int times) {
        BufferedReader in = null;
        long totalEndTime = 0;
        int index = times;
        threadSleep(2000);

        try {
            File performanceLogFile = null;
            if (!(performanceLogFile = new File("/sdcard/performanceTest.log")).exists()) {
                throw new IOException("There is no file in sdcard!");
            }

            in = new BufferedReader(new FileReader(performanceLogFile));
            String line;
            while ((line = in.readLine()) != null) {
                if (line.startsWith(prefix)) {
                    totalEndTime += Long.parseLong(line.substring(line.lastIndexOf(':') + 2));
                    index--;
                }
            }

            if (0 != index) {
                Assert.assertFalse(String.format("some calls dropped, we expected %d records totally", times), true);
            }
        } catch (IOException ex) {
            Assert.assertFalse(String.format("Failed, the IOException is: %s", ex), true);
        } finally {
            if (in != null) {
                try {
                    in.close();
                } catch (IOException e) {
                }
            }
        }
        return (totalEndTime - totalStartTime);
    }

}
