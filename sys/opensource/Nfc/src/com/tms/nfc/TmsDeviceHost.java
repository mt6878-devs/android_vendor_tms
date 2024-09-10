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

public interface TmsDeviceHost {
    int NFA_PROTOCOL_NONE = -1;
    int NFA_PROTOCOL_ISO_DEP = 0x04;
    int NFA_PROTOCOL_MIFARE = 0x80;

    interface TmsDeviceHostListener {
        void onPtmResponseReceive(int event, byte[] data);
        void onM1RawDataAuthCallback(int status);
        void onLxDebugUpload(int event, byte[] data);
        void onNfceeAidSelect(int nfceeId, byte[] aid);
    }

    void skipTagSelect(int proto);

    void setRfListenMask(int listenMask, boolean restartRfDiscovery);

    int getRfListenMask();

    byte[] sendRawNciCommand(byte[] data, boolean restartRfDiscovery);

    boolean isNfccBusy();

    int setTransitConfig(String configs);

    void setUserDefaultRoutesPref(
        int mifareRoute,
        int isoDepRoute,
        int felicaRoute,
        int abTechRoute,
        int scRoute,
        int aidRoute);

    String getMwVersion();
    String getMwBuildTime();

    void setEmptyAidRoute(int defaultAidRoute, int emptyAidPower);

    void setPassthroughMode(int mode);
    boolean sendRawPtmCommand(byte[] data);

    byte[] getManufSpecInfo();

    void setM1RawDataModeEnable(boolean isEnable);

    int[] doGetActiveSecureElementList();

    public int doWriteT4tData(byte[] fileId, byte[] data, int length);

    public byte[] doReadT4tData(byte[] fileId);

    public boolean doClearNdefT4tData();

    public boolean enableT4tNfcee(boolean enable);

    public byte[] doSendT4tRawApdu(byte[] apdu, int apduLen);

    int doGetT4TNfceePowerState();

    void doSetLxDebugUploadEnabled(boolean isEnabled, int level);

    boolean setRfConfigs(byte[][] configs, boolean stopWhenFailed);
}
