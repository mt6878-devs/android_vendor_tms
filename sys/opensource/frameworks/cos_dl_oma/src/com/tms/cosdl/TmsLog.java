/*
 * Copyright (c) Tsingteng MicroSystem Co., Ltd. 2022-2022. All rights reserved.
 */
/*
 * Copyright (C) 2022 Tsingteng MicroSystem
 *
 * All rights are reserved. Reproduction in whole or in part is
 * prohibited without the written consent of the copyright owner.
 *
 * Tsingteng reserves the right to make changes without notice at any time.
 *
 * Tsingteng makes no warranty, expressed, implied or statutory, including but
 * not limited to any implied warranty of merchantability or fitness for any
 * particular purpose, or that the use will not infringe any third party patent,
 * copyright or trademark. Tsingteng must not be liable for any loss or damage
 * arising from its use.
 */

package com.tms.cosdl;


import com.tms.cosdl.adaptation.Log;

/**
 * Logging utility class for the TmsCosDl module
 *
 * @since 1.0
 */
public class TmsLog {

    private static String TAG = "TmsCosDl";

    private static Level sLogLevel = Level.DEBUG;

    private static Log sLog = null;

    /**
     * Set the log output level of this module
     *
     * @param logLevel Log level to set
     * @since 1.0
     */
    public static void setLogLevel(Level logLevel) {
        sLogLevel = logLevel;
        if (sLog != null) {
            sLog.e(TAG, "set log level: " + logLevel);
        }
    }

    /**
     * Set the log output tag for this module
     * @param tag log tag to set
     */
    public static void setLogTag(String tag) {
        if (tag != null) {
            TAG = tag;
        }
    }

    /**
     * Set the log instance of this module
     *
     * @param log the log instance
     * @since 2.0
     */
    public static void setLog(Log log) {
        sLog = log;
    }

    /**
     * Whether debug level logs are allowed
     *
     * @return Returns true if the current log level is debug, false otherwise
     * @since 1.0
     */
    public static boolean isDebugEnable() {
        return sLogLevel.value <= Level.DEBUG.value;
    }

    /**
     * Prints the stack log for debugging
     *
     * @param exception Exception information to be printed
     * @since 1.0
     */
    public static void debugTracePrint(Exception exception) {
        if (sLog != null && isDebugEnable()) {
            sLog.e(TAG, "Error occurred", exception);
        }
    }

    /**
     * Prints debug level logs
     *
     * @param subTag sub tag
     * @param message log message
     * @since 1.0
     */
    public static void debug(String subTag, String message) {
        if (sLog == null || sLogLevel.value > Level.DEBUG.value) {
            return;
        }
        sLog.d(TAG, subTag + " " + message);
    }

    /**
     * Prints info level logs
     *
     * @param subTag sub tag
     * @param message log message
     * @since 1.0
     */
    public static void info(String subTag, String message) {
        if (sLog == null || sLogLevel.value > Level.INFO.value) {
            return;
        }
        sLog.i(TAG, subTag + " " + message);
    }

    /**
     * Prints warn level logs
     *
     * @param subTag sub tag
     * @param message log message
     * @since 1.0
     */
    public static void warn(String subTag, String message) {
        if (sLog == null || sLogLevel.value > Level.WARN.value) {
            return;
        }
        sLog.i(TAG, subTag + " " + message);
    }

    /**
     * Prints error level logs
     *
     * @param subTag sub tag
     * @param message log message
     * @since 1.0
     */
    public static void error(String subTag, String message) {
        if (sLog == null || sLogLevel.value > Level.ERROR.value) {
            return;
        }
        sLog.i(TAG, subTag + " " + message);
    }

    /**
     * Defines the log level of the TmsCosDl module
     *
     * @since 1.0
     */
    public enum Level {
        DEBUG(0), INFO(1), WARN(2), ERROR(3);
        int value;

        Level(int value) {
            this.value = value;
        }
    }
}
