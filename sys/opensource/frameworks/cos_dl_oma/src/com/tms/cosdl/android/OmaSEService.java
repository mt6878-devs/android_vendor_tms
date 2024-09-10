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

import android.content.Context;

import com.tms.cosdl.adaptation.Reader;
import com.tms.cosdl.adaptation.SEService;

import java.util.concurrent.Executor;

/**
 * Implementation of SE Service Interface of SE Communication Adaptation Layer on Android System
 *
 * @since 2.0
 */
class OmaSEService implements SEService {

    private final android.se.omapi.SEService mSEService;

    OmaSEService(Context context, Executor executor, OnConnectedListener listener) {
        mSEService = new android.se.omapi.SEService(context, executor, listener::onConnected);
    }

    @Override
    public Reader[] getReaders() {
        android.se.omapi.Reader[] readers = mSEService.getReaders();
        OmaReader[] omaReaders = new OmaReader[readers.length];
        for (int i = 0; i < readers.length; i++) {
            omaReaders[i] = new OmaReader(this, readers[i]);
        }
        return omaReaders;
    }

    @Override
    public void shutdown() {
        mSEService.shutdown();
    }

    @Override
    public String getVersion() {
        return mSEService.getVersion();
    }
}
