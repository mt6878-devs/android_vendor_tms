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

package com.tms.cosdl.adaptation;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.net.UnknownHostException;

/**
 * Host system adaptation layer Log print interface
 *
 * @since 2.0
 */
public interface Log {

    /**
     * define log level
     *
     * @since 2.0
     */
    enum Level {
        VERBOSE(0), DEBUG(1), INFO(2), WARN(3), ERROR(4), ASSERT(5);

        final int code;

        Level(int code) {
            this.code = code;
        }

        public int getCode() {
            return code;
        }
    }

    /**
     * Send a {@link #{Level.VERBOSE}} log message.
     *
     * @param tag Used to identify the source of a log message.  It usually identifies
     *            the class or activity where the log call occurs.
     * @param msg The message you would like logged.
     */
    default void v(String tag, String msg) {
        println(Level.VERBOSE, tag, msg);
    }

    /**
     * Send a {@link #{Level.VERBOSE}} log message and log the exception.
     *
     * @param tag Used to identify the source of a log message.  It usually identifies
     *            the class or activity where the log call occurs.
     * @param msg The message you would like logged.
     * @param tr  An exception to log
     */
    default void v(String tag, String msg, Throwable tr) {
        println(Level.VERBOSE, tag, msg + '\n' + getStackTraceString(tr));
    }

    /**
     * Send a {@link #{Level.DEBUG}} log message.
     *
     * @param tag Used to identify the source of a log message.  It usually identifies
     *            the class or activity where the log call occurs.
     * @param msg The message you would like logged.
     */
    default void d(String tag, String msg) {
        println(Level.DEBUG, tag, msg);
    }

    /**
     * Send a {@link #{Level.DEBUG}} log message and log the exception.
     *
     * @param tag Used to identify the source of a log message.  It usually identifies
     *            the class or activity where the log call occurs.
     * @param msg The message you would like logged.
     * @param tr  An exception to log
     */
    default void d(String tag, String msg, Throwable tr) {
        println(Level.DEBUG, tag, msg + '\n' + getStackTraceString(tr));
    }

    /**
     * Send an {@link #{Level.INFO}} log message.
     *
     * @param tag Used to identify the source of a log message.  It usually identifies
     *            the class or activity where the log call occurs.
     * @param msg The message you would like logged.
     */
    default void i(String tag, String msg) {
        println(Level.INFO, tag, msg);
    }

    /**
     * Send a {@link #{Level.INFO}} log message and log the exception.
     *
     * @param tag Used to identify the source of a log message.  It usually identifies
     *            the class or activity where the log call occurs.
     * @param msg The message you would like logged.
     * @param tr  An exception to log
     */
    default void i(String tag, String msg, Throwable tr) {
        println(Level.INFO, tag, msg + '\n' + getStackTraceString(tr));
    }

    /**
     * Send a {@link #{Level.WARN}} log message.
     *
     * @param tag Used to identify the source of a log message.  It usually identifies
     *            the class or activity where the log call occurs.
     * @param msg The message you would like logged.
     */
    default void w(String tag, String msg) {
        println(Level.WARN, tag, msg);
    }

    /**
     * Send a {@link #{Level.WARN}} log message and log the exception.
     *
     * @param tag Used to identify the source of a log message.  It usually identifies
     *            the class or activity where the log call occurs.
     * @param msg The message you would like logged.
     * @param tr  An exception to log
     */
    default void w(String tag, String msg, Throwable tr) {
        println(Level.WARN, tag, msg + '\n' + getStackTraceString(tr));
    }

    /**
     * Send a {@link #{Level.WARN}} log message and log the exception.
     *
     * @param tag Used to identify the source of a log message.  It usually identifies
     *            the class or activity where the log call occurs.
     * @param tr  An exception to log
     */
    default void w(String tag, Throwable tr) {
        println(Level.WARN, tag, getStackTraceString(tr));
    }

    /**
     * Send an {@link #{Level.ERROR}} log message.
     *
     * @param tag Used to identify the source of a log message.  It usually identifies
     *            the class or activity where the log call occurs.
     * @param msg The message you would like logged.
     */
    default void e(String tag, String msg) {
        println(Level.ERROR, tag, msg);
    }

    /**
     * Send a {@link #{Level.ERROR}} log message and log the exception.
     *
     * @param tag Used to identify the source of a log message.  It usually identifies
     *            the class or activity where the log call occurs.
     * @param msg The message you would like logged.
     * @param tr  An exception to log
     */
    default void e(String tag, String msg, Throwable tr) {
        println(Level.ERROR, tag, msg + '\n' + getStackTraceString(tr));
    }

    /**
     * Handy function to get a loggable stack trace from a Throwable
     *
     * @param tr An exception to log
     */
    default String getStackTraceString(Throwable tr) {
        if (tr == null) {
            return "";
        }

        // This is to reduce the amount of log spew that apps do in the non-error
        // condition of the network being unavailable.
        Throwable t = tr;
        while (t != null) {
            if (t instanceof UnknownHostException) {
                return "";
            }
            t = t.getCause();
        }

        StringWriter sw = new StringWriter();
        PrintWriter pw = new PrintWriter(sw);
        tr.printStackTrace(pw);
        pw.flush();
        return sw.toString();
    }

    void println(Level level, String tag, String msg);
}
