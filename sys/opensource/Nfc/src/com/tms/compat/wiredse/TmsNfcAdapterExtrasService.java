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

import com.android.nfc.NfcService;

import android.nfc.INfcAdapterExtras;

import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;

public class TmsNfcAdapterExtrasService extends INfcAdapterExtras.Stub {

    private static final String TAG = "TmsNfcAdapterExtrasService";

    private static final String KEY_STATUS = "status";
    private static final String KEY_ATR = "atr";
    private static final String KEY_OUT = "out";

    private WiredSeDeathRecipient mWiredSeDeathRecipient = new WiredSeDeathRecipient();

    private int mNfcWiredSeHandle;
    private IBinder mWiredSeClient;

    public Bundle open(String pkg, IBinder b) {
        Bundle result = new Bundle();
        try {
            if (b == null) {
                result.putBoolean(KEY_STATUS, false);
                return result;
            }
            int status = NfcService.getInstance().getTmsNfcService().doOpenWiredSeConnection();
            mNfcWiredSeHandle = status;
            result.putBoolean(KEY_STATUS, status != 0);
            if (status == 0) {
                TmsLog.e(TAG, "openWiredSe failed");
            } else {
                byte[] atr = NfcService.getInstance().getTmsNfcService().doWiredSeGetAtr();
                result.putByteArray(KEY_ATR, atr);
            }
            if (mWiredSeClient != null) {
                mWiredSeClient.unlinkToDeath(mWiredSeDeathRecipient, 0);
            }
            mWiredSeClient = b;
            mWiredSeClient.linkToDeath(mWiredSeDeathRecipient, 0);
        } catch (RuntimeException | RemoteException e) {
            TmsLog.e(TAG, "openWiredSe failed", e);
            result.putBoolean(KEY_STATUS, false);
        }
        return result;
    }

    public Bundle close(String pkg, IBinder b) {
        Bundle result = new Bundle();
        try {
            mNfcWiredSeHandle = 0;
            NfcService.getInstance().getTmsNfcService().doCloseWiredSeConnection();
            result.putBoolean(KEY_STATUS, true);
        } catch (RuntimeException e) {
            TmsLog.e(TAG, "closeWiredSe failed", e);
            result.putBoolean(KEY_STATUS, false);
        }
        return result;
    }

    public Bundle transceive(String pkg, byte[] data_in) {
        Bundle result = new Bundle();
        try {
            byte[] rsp = NfcService.getInstance().getTmsNfcService().doWiredSeTransceive(data_in);
            result.putBoolean(KEY_STATUS, true);
            result.putByteArray(KEY_OUT, rsp);
        } catch (RuntimeException e) {
            TmsLog.e(TAG, "wiredSe transceive failed");
            result.putBoolean(KEY_STATUS, false);
        }
        return result;
    }

    public int getCardEmulationRoute(String pkg) {
        TmsLog.d(TAG, "getCardEmulationRoute called, pkg = " + pkg);
        return -1;
    }

    public void setCardEmulationRoute(String pkg, int route) {
        TmsLog.d(TAG, "setCardEmulationRoute called, pkg = " + pkg + ", route = " + route);
    }

    public void authenticate(String pkg, byte[] token) {
        TmsLog.d(TAG, "authenticate called, pkg = " + pkg);
    }

    public String getDriverName(String pkg) {
        TmsLog.d(TAG, "getDriverName called, pkg = " + pkg);
        return "tms_nfc";
    }

    private final class WiredSeDeathRecipient implements IBinder.DeathRecipient {
        @Override
        public void binderDied() {
            TmsLog.i(TAG, "WiredSe client dead");
            if (mNfcWiredSeHandle > 0) {
                mNfcWiredSeHandle = 0;
                NfcService.getInstance().getTmsNfcService().doCloseWiredSeConnection();
            }
            if (mWiredSeClient != null) {
                mWiredSeClient.unlinkToDeath(mWiredSeDeathRecipient, 0);
            }
            mWiredSeClient = null;
        }
    }
}
