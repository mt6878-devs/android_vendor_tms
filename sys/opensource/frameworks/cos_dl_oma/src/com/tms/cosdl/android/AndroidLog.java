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

package com.tms.cosdl.android;

import com.tms.cosdl.adaptation.Log;

/**
 * Implementation of the Log interface of the host system adaptation layer on the Android system
 *
 * @since 2.0
 */
class AndroidLog implements Log {

    @Override
    public void println(Level level, String tag, String msg) {
        switch (level) {
            case VERBOSE:
                android.util.Log.v(tag, msg);
                break;
            case DEBUG:
                android.util.Log.d(tag, msg);
                break;
            case INFO:
                android.util.Log.i(tag, msg);
                break;
            case WARN:
                android.util.Log.w(tag, msg);
                break;
            case ERROR:
                android.util.Log.e(tag, msg);
                break;
            default:
                break;
        }
    }

    private AndroidLog() {

    }

    static Log getInstance() {
        return AndroidLogHolder.ANDROID_LOG;
    }

    private static final class AndroidLogHolder {
        private static final AndroidLog ANDROID_LOG = new AndroidLog();
    }
}
