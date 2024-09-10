/*
 * Copyright (C) 2023 Tsingteng MicroSystem
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

import com.tms.nfc.TmsLog;
import android.nfc.INfcAdapter;
import android.content.Context;


public abstract class TmsNfcAdapterStub extends INfcAdapter.Stub {
    private static final String TAG = "TmsNfcAdapterStub";
    private Context mContext;
    private volatile boolean mIsAlwaysOnSupported;

    public TmsNfcAdapterStub(Context context) {
        mContext = context;
        mIsAlwaysOnSupported = false;
    }
    public void setAlwaysOnSupported(boolean support) {
        TmsLog.d(TAG, "setAlwaysOnSupported:" + support);
        mIsAlwaysOnSupported = support;
    }
    public void notifyControllerAlwaysOnListeners(boolean enabled) {
        TmsLog.d(TAG, "notifyControllerAlwaysOnListeners:" + enabled);
    }
}


