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

package com.tms.nfc.dhimpl;

import android.util.Log;

public class TmsNativeWiredSe {
    private static final String TAG = "NativeWiredSe";

    public TmsNativeWiredSe() {

    }

    public int doOpenWiredSeConnection() {
        return doNativeOpenWiredSeConnection();
    }

    public void doCloseWiredSeConnection() {
        doNativeCloseWiredSeConnection();
    }

    public byte[] doWiredSeGetAtr() {
        return doNativeWiredSeGetAtr();
    }

    public byte[] doWiredSeTransceive(byte[] cApdu) {
        return doNativeWiredSeTransceive(cApdu);
    }

    private native int doNativeOpenWiredSeConnection();
    private native void doNativeCloseWiredSeConnection();
    private native byte[] doNativeWiredSeGetAtr();
    private native byte[] doNativeWiredSeTransceive(byte[] cApdu);
}
