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

package com.tms.nfc;

import android.annotation.TargetApi;
import android.content.pm.PackageManager;
import android.os.Binder;
import android.os.Build;
import android.util.Log;

import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;

public class TmsLog {

    private static final int LEVEL_DEBUG = 4;
    private static final int LEVEL_INFO = 3;
    private static final int LEVEL_WARN = 2;
    private static final int LEVEL_ERROR = 1;
    private static final int LEVEL_NONE = 0;

    private static final int LEVEL_DEFAULT = LEVEL_INFO;

    private static int sLevel = LEVEL_DEFAULT;

    private static final String LOG_LEVEL_KEY = "TMSLOG_SERVICE_LOGLEVEL";


    private static final String TAG = "TmsNfcNci";

    private static final HashSet<String> sLogWhiteListPackage = new HashSet<>(Arrays.asList("com.android.nfc", "com.tms.nfc"));
    private static final HashSet<String> sLogBlackListPackage = new HashSet<>(Collections.singletonList("com.tms.nfc.TmsLog"));

    static {
        sLevel = TmsConfig.getInt(LOG_LEVEL_KEY, LEVEL_DEFAULT);
    }

    public static void dumpCaller(PackageManager pm) {
        if (!isDebugEnable()) {
            // output this log only in debug mode
            return;
        }
        int callingUid = Binder.getCallingUid();
        String[] packagesForUid = pm.getPackagesForUid(callingUid);
        String packageName = packagesForUid.length > 0 ? packagesForUid[0] : "";
        d("called by PID:" + Binder.getCallingPid() + ", UID:" + callingUid + ", pkn:" + packageName);
    }

    public static void d(String tag, String message) {
        if (isDebugEnable()) {
            Log.d(tag, getCallerInfo() + message);
        }
    }

    public static void d(String message) {
        if (isDebugEnable()) {
            Log.d(TAG, getCallerInfo() + message);
        }
    }

    public static void i(String tag, String message) {
        if (!isAllowPrintLog(LEVEL_INFO)) {
            return;
        }
        Log.i(tag, isDebugEnable() ? getCallerInfo() + message : message);
    }

    public static void i(String message) {
        if (!isAllowPrintLog(LEVEL_INFO)) {
            return;
        }
        Log.i(TAG, isDebugEnable() ? getCallerInfo() + message : message);
    }

    public static void w(String tag, String message) {
        if (!isAllowPrintLog(LEVEL_WARN)) {
            return;
        }
        Log.w(tag, isDebugEnable() ? getCallerInfo() + message : message);
    }

    public static void w(String message) {
        if (!isAllowPrintLog(LEVEL_WARN)) {
            return;
        }
        Log.w(TAG, isDebugEnable() ? getCallerInfo() + message : message);
    }

    public static void w(String tag, String message, Throwable throwable) {
        if (!isAllowPrintLog(LEVEL_WARN)) {
            return;
        }
        Log.w(tag, isDebugEnable() ? getCallerInfo() + message : message, throwable);
    }

    public static void w(String message, Throwable throwable) {
        if (!isAllowPrintLog(LEVEL_WARN)) {
            return;
        }
        Log.w(TAG, isDebugEnable() ? getCallerInfo() + message : message, throwable);
    }

    public static void e(String tag, String message) {
        if (!isAllowPrintLog(LEVEL_ERROR)) {
            return;
        }
        Log.e(tag, isDebugEnable() ? getCallerInfo() + message : message);
    }

    public static void e(String message) {
        if (!isAllowPrintLog(LEVEL_ERROR)) {
            return;
        }
        Log.e(TAG, isDebugEnable() ? getCallerInfo() + message : message);
    }

    public static void e(String tag, String message, Throwable throwable) {
        if (!isAllowPrintLog(LEVEL_ERROR)) {
            return;
        }
        Log.e(tag, isDebugEnable() ? getCallerInfo() + message : message, throwable);
    }

    public static void e(String message, Throwable throwable) {
        if (!isAllowPrintLog(LEVEL_ERROR)) {
            return;
        }
        Log.e(TAG, isDebugEnable() ? getCallerInfo() + message : message, throwable);
    }

    private static String getCallerInfo() {
        StackTraceElement caller = getCaller();
        if (caller == null) {
            return "";
        }
        return "[" + TmsUtils.getSuffix(caller.getClassName()) + ":" + caller.getMethodName() + "(" + caller.getLineNumber() + ")] ";
    }

    @TargetApi(Build.VERSION_CODES.N)
    private static StackTraceElement getCaller() {
        return Arrays.stream(Thread.currentThread().getStackTrace()).filter(stackTraceElement -> {
                    String className = stackTraceElement.getClassName();
                    return sLogWhiteListPackage.stream().anyMatch(className::startsWith) &&
                            sLogBlackListPackage.stream().noneMatch(className::startsWith);
                }
        ).findFirst().orElse(null);
    }

    private static boolean isDebugEnable() {
        return isAllowPrintLog(LEVEL_DEBUG);
    }

    private static boolean isAllowPrintLog(int level) {
        return sLevel >= level;
    }

}
