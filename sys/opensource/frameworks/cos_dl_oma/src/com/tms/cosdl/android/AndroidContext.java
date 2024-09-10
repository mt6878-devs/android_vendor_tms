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

import static com.tms.cosdl.TmsLog.debug;
import static com.tms.cosdl.TmsLog.debugTracePrint;
import static com.tms.cosdl.TmsLog.info;
import static com.tms.cosdl.TmsLog.warn;

import android.os.Looper;

import com.tms.cosdl.adaptation.Context;
import com.tms.cosdl.adaptation.Log;
import com.tms.cosdl.adaptation.Preferences;
import com.tms.cosdl.adaptation.SEService;

import java.util.HashMap;
import java.util.Optional;
import java.util.concurrent.Executors;

/**
 * Implementation of host system adaptation layer context interface on Android system
 *
 * @since 2.0
 */
class AndroidContext implements Context {

    private static final String TAG = "AndroidContext";

    private final android.content.Context mContext;

    private final HashMap<String, Preferences> mPrefsMap = new HashMap<>();

    private SEService mSEService;

    AndroidContext(android.content.Context mContext) {
        this.mContext = mContext;
    }

    @Override
    public Optional<SEService> getSEService(long timeout) {
        return getSEService(mContext, timeout);
    }

    @Override
    public Preferences getPreferences(String name) {
        Preferences prefs = mPrefsMap.get(name);
        if (prefs == null) {
            prefs = new AndroidPreferences(mContext.getSharedPreferences(name, android.content.Context.MODE_PRIVATE));
        }
        mPrefsMap.put(name, prefs);
        return prefs;
    }

    @Override
    public Log getLog() {
        return AndroidLog.getInstance();
    }

    private synchronized Optional<SEService> getSEService(android.content.Context ctx, long timeout) {
        if (mSEService != null) {
            debug(TAG, "SEService is already connect");
            return Optional.of(mSEService);
        }
        if (Looper.getMainLooper() == Looper.myLooper()) {
            // It's called by the main thread
            return Optional.empty();
        }
        final boolean[] seConnected = {false};
        final SEService.OnConnectedListener seConnectCallback = () -> {
            info(TAG, "SEService connected");
            synchronized (this) {
                seConnected[0] = true;
                notifyAll();
            }
        };
        mSEService = new OmaSEService(ctx, Executors.newSingleThreadExecutor(), seConnectCallback);
        info(TAG, "Wait for the SEService to connect");
        try {
            wait(timeout);
        } catch (InterruptedException e) {
            debugTracePrint(e);
        }
        if (!seConnected[0]) {
            warn(TAG, "Wait SEService timeout");
            return Optional.empty();
        }
        return Optional.of(mSEService);
    }
}
