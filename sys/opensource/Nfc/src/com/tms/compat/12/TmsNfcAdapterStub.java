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
import com.android.nfc.NfcPermissions;
import android.nfc.INfcControllerAlwaysOnListener;
import android.content.Context;
import android.os.RemoteException;
import java.util.HashSet;
import java.util.Set;
import java.util.Collections;

public abstract class TmsNfcAdapterStub extends INfcAdapter.Stub {
    private static final String TAG = "TmsNfcAdapterStub";
    private Context mContext;
    private volatile boolean mIsAlwaysOnSupported;
    private final Set<INfcControllerAlwaysOnListener> mAlwaysOnListeners =
            Collections.synchronizedSet(new HashSet<>());

    public TmsNfcAdapterStub(Context context) {
        mContext = context;
        mIsAlwaysOnSupported = false;
    }

    @Override
    public void registerControllerAlwaysOnListener(
            INfcControllerAlwaysOnListener listener) throws RemoteException {
        NfcPermissions.enforceSetControllerAlwaysOnPermissions(mContext);
        TmsLog.d(TAG, "registerControllerAlwaysOnListener:" + mIsAlwaysOnSupported);

        if (!mIsAlwaysOnSupported) return;
        synchronized (mAlwaysOnListeners) {
            mAlwaysOnListeners.add(listener);
        }
    }

    @Override
    public void unregisterControllerAlwaysOnListener(
            INfcControllerAlwaysOnListener listener) throws RemoteException {
        NfcPermissions.enforceSetControllerAlwaysOnPermissions(mContext);
        TmsLog.d(TAG, "unregisterControllerAlwaysOnListener:" + mIsAlwaysOnSupported);
        if (!mIsAlwaysOnSupported) return;
        synchronized (mAlwaysOnListeners) {
            mAlwaysOnListeners.remove(listener);
        }
    }

    public void setAlwaysOnSupported(boolean support) {
        TmsLog.d(TAG, "setAlwaysOnSupported:" + support);
        mIsAlwaysOnSupported = support;
    }

    public void notifyControllerAlwaysOnListeners(boolean enabled) {
        TmsLog.d(TAG, "notifyControllerAlwaysOnListeners:" + enabled);
        synchronized (mAlwaysOnListeners) {
            for (INfcControllerAlwaysOnListener listener : mAlwaysOnListeners) {
                try {
                    listener.onControllerAlwaysOnChanged(enabled);
                } catch (RemoteException e) {
                    TmsLog.e(TAG, "error in updateAlwaysOnState");
                }
            }
        }
    }

}


