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

import com.android.nfc.dhimpl.NativeNfcManager;
import com.android.nfc.dhimpl.NativeT4tNfceeManager;
import com.tms.nfc.TmsDeviceHost;

import com.tms.nfc.TmsUtils;

public class TmsNativeNfcManager implements TmsDeviceHost {

    private static final String TAG = "TmsNativeNfcManager";

    private final TmsDeviceHostListener mListener;
    private final NativeT4tNfceeManager mT4tNfceeMgr;

    /* Native structure */
    private long mNative;

    public TmsNativeNfcManager(TmsDeviceHostListener tmsListener) {
        this.mListener = tmsListener;
        initializeTmsNativeStructure();
        mT4tNfceeMgr = new NativeT4tNfceeManager();
    }

    private native boolean initializeTmsNativeStructure();

    private native void doSkipTagSelect(int proto);

    public void skipTagSelect(int proto){
        Log.d(TAG, "skipTagSelect() called with: proto = [" + proto + "]");
        doSkipTagSelect(proto);
    }

    private native void doSetRfListenMask(int listenMask, boolean restartRfDiscovery);

    @Override
    public void setRfListenMask(int listenMask, boolean restartRfDiscovery) {
        doSetRfListenMask(listenMask, restartRfDiscovery);
    }

    private native int doGetRfListenMask();

    @Override
    public int getRfListenMask() {
        return doGetRfListenMask();
    }

    private native byte[] doSendRawNciCommand(byte[] data, boolean restartRfDiscovery);

    @Override
    public byte[] sendRawNciCommand(byte[] data, boolean restartRfDiscovery) {
        return doSendRawNciCommand(data, restartRfDiscovery);
    }

    @Override
    public native boolean isNfccBusy();

    @Override
    public native int setTransitConfig(String configs);

    @Override
    public native void setEmptyAidRoute(int deafultAidroute, int emptyAidPower);


//#ifdef TMS_NFC
    // NfcSettingsAdapter
    public native void setUserDefaultRoutesPref(
            int mifareRoute,
            int isoDepRoute,
            int felicaRoute,
            int abTechRoute,
            int scRoute,
            int aidRoute);
//#endif

    private native String doGetMwVersion();

    public String getMwVersion() {
        String mwVersion = doGetMwVersion();
        Log.d(TAG, "getMwVersion() called, mwVersion = " + mwVersion);
        return mwVersion;
    }

    private native String doGetMwBuildTime();

    public String getMwBuildTime() {
        String mwBuildTime = doGetMwBuildTime();
        Log.d(TAG, "getMwBuildTime() called, buildTime = " + mwBuildTime);
        return mwBuildTime;
    }

    public native void doSetPassthroughMode(int mode);

    public void setPassthroughMode(int mode) {
        Log.d(TAG, "setPassthroughMode mode = " + mode);
        doSetPassthroughMode(mode);
    }

    public native boolean doSendRawPtmCommand(byte[] data);
    public boolean sendRawPtmCommand(byte[] data) {
        Log.d(TAG, "sendRawPtmCommand called, size = " + data.length);
        return doSendRawPtmCommand(data);
    }

    private void notifyRawPtmCommandCallback(int event, int len, byte[] data) {
        //Log.d(TAG, "notifyRawNciCommandCallback() called with: event = [" + event + "], len = [" + len + "], data = [" + TmsUtils.byteArray2Hex(data) + "]");
        mListener.onPtmResponseReceive(event, data);
    }

    private native byte[] doGetManufSpecInfo();

    @Override
    public byte[] getManufSpecInfo() {
        return doGetManufSpecInfo();
    }

    private native void doSetM1RawDataModeEnable(boolean isEnable);

    @Override
    public void setM1RawDataModeEnable(boolean isEnable) {
        doSetM1RawDataModeEnable(isEnable);
    }

    private void notifyM1RawDataAuthCallback(int authStatus) {
        Log.d(TAG, "notifyM1RawDataAuthCallback : " + authStatus);
        mListener.onM1RawDataAuthCallback(authStatus);
    }

    @Override
    public native int[] doGetActiveSecureElementList();

    @Override
    public int doWriteT4tData(byte[] fileId, byte[] data, int length) {
        return mT4tNfceeMgr.doWriteT4tData(fileId, data, length);
    }

    @Override
    public byte[] doReadT4tData(byte[] fileId) {
        return mT4tNfceeMgr.doReadT4tData(fileId);
    }

    @Override
    public boolean doClearNdefT4tData() {
        return mT4tNfceeMgr.doClearNdefT4tData();
    }
    @Override
    public boolean enableT4tNfcee(boolean enable) {
        return mT4tNfceeMgr.enableT4tNfcee(enable);
    }
    @Override
    public byte[] doSendT4tRawApdu(byte[] apdu, int apduLen) {
        return mT4tNfceeMgr.sendT4tRawApdu(apdu, apduLen);
    }

    @Override
    public native int doGetT4TNfceePowerState();

    @Override
    public native void doSetLxDebugUploadEnabled(boolean isEnabled, int level);

    private void notifyLxDebugUpload(int event, byte[] data) {
        mListener.onLxDebugUpload(event, data);
    }

    public native boolean setRfConfigs(byte[][] configs, boolean stopWhenFailed);

    private void notifyNfceeAidSelect(int nfceeId, byte[] aid) {
        mListener.onNfceeAidSelect(nfceeId, aid);
    }
}

