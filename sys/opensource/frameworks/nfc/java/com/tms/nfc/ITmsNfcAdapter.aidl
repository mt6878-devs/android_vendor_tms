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

import com.tms.nfc.IM1RawDataModeCallback;

import java.util.Map;

import android.os.Bundle;

/**
 * @hide
 */
interface ITmsNfcAdapter
{
    void setDefaultUserRoutes(in Map userRoutes);
    Map getDefaultUserRoutes();
    void setAllDefaultUserRoutes(String route);
    void setSkipTagSelect(int protocol);
    void setRfListenMask(int listenMask);
    int getRfListenMask();
    byte[] sendNciCommand(in byte[] cmd);
    int setConfig(String configs);
    String getMwVersion();
    void setRfPollMask(int listenMask);
    int getRfPollMask();
    IBinder getHciAdapterService();
    byte[] getNfccSerialNumber();
    String getNfcModelName();
    String getNfcFwVersion();
    boolean setM1RawDataModeEnable(boolean isEnable, in IM1RawDataModeCallback callback);
    boolean isM1RawDataModeEnabled();
    int getM1RawDataModeState();
    boolean setM1RawDataModeTimeInterval(int timeInterval);
    int changeRfParams(in byte[] data, boolean lastCmd);
    int[] getActiveSecureElementList();
    int doWriteT4tData(in byte[] fileId, in byte[] data, int length);
    byte[] doReadT4tData(in byte[] fileId);
    boolean enableT4tNfceeRoute(boolean enable);
    byte[] sendT4tRawApdu(in byte[] apdu, int apduLen);
    boolean enableT4tContactlessWrite(boolean enable);
    boolean setForceSAK(boolean isEnable, byte sak);
    boolean setHceTypeAConfig(boolean enabled, in byte[] atqa, in byte[] sak, in byte[] uid);
    boolean startSilentFieldDetectMode(int timeout);
    boolean stopSilentFieldDetectMode();
    boolean isSilentFieldDetectEnabled();
    int setFeatureState(String featureName, int state, boolean force, in Bundle extras);
    int getFeatureState(String featureName);
}
