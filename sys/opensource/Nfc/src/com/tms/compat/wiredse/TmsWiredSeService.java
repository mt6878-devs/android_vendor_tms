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

import java.util.ArrayList;
import android.util.Log;
import android.os.Handler;
import android.os.HwBinder;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import java.io.IOException;
import java.util.NoSuchElementException;
import com.android.nfc.NfcService;

import com.tms.nfc.dhimpl.TmsNativeWiredSe;
import vendor.tms.wiredse.V1_0.ITmsWiredSe;
import vendor.tms.wiredse.V1_0.ITmsWiredSeHalCallback;

public class TmsWiredSeService {
    static final String TAG = "TmsWiredSeService";

    private static TmsWiredSeService sService ;

    private static final int EVENT_GET_HAL = 1;
    private static final int MAX_GET_HAL_RETRY = 5;
    private static final int GET_SERVICE_DELAY_MILLIS = 200;
    private static int sNfcWiredSeHandle = 0;
    private static int sWiredSeGetHalRetry = 0;
    ITmsWiredSe mWiredSEHal = null;


    public static TmsWiredSeService getInstance() {
        if (sService == null) {
            sService = new TmsWiredSeService();
        }
        return sService;
    }

    private HwBinder.DeathRecipient mWiredSeDeathRecipient = new WiredSeDeathRecipient();

    private ITmsWiredSeHalCallback.Stub mWiredSeCallback = new ITmsWiredSeHalCallback.Stub() {
        private ArrayList<Byte> byteArrayToArrayList(byte[] array) {
            ArrayList<Byte> list = new ArrayList<Byte>();
            if (array != null) {
                for (Byte b : array) {
                    list.add(b);
                }
            }
            return list;
        }

        private byte[] arrayListToByteArray(ArrayList<Byte> list) {
            Byte[] byteArray = list.toArray(new Byte[list.size()]);
            int i = 0;
            byte[] result = new byte[list.size()];
            for (Byte b : byteArray) {
                result[i++] = b.byteValue();
            }
            return result;
        }

        @Override
        public int openWiredSe() {
            Log.d(TAG, "WiredSe: openWiredSe");
            int sNfcWiredSeHandle = NfcService.getInstance().getTmsNfcService().doOpenWiredSeConnection();
            if (sNfcWiredSeHandle == 0) {
                Log.e(TAG, "openWiredSe failed");
            } else {
                Log.e(TAG, "openWiredSe success");
            }
            return sNfcWiredSeHandle;
        }

        @Override
        public ArrayList<Byte> transmit(ArrayList<Byte> data, int wiredSeHandle) {
            Log.d(TAG, "WiredSe: transmit");
            byte[] rApdu =  NfcService.getInstance().getTmsNfcService().doWiredSeTransceive(arrayListToByteArray(data));
            return byteArrayToArrayList(rApdu);
        }

        @Override
        public ArrayList<Byte> getAtr(int wiredSeHandle)
            throws android.os.RemoteException {
            Log.d(TAG, "WiredSe: getAtr");
            synchronized(TmsWiredSeService.this) {
                return byteArrayToArrayList(NfcService.getInstance().getTmsNfcService().doWiredSeGetAtr());
            }
        }

        @Override
        public int closeWiredSe(int wiredSeHandle) {
            Log.d(TAG, "WiredSe: closeWiredSe");
            // NfcService.getInstance().doDisconnect(wiredSeHandle);
            sNfcWiredSeHandle = 0;
            // NfcService.getInstance().isWiredOpen = false;
            NfcService.getInstance().getTmsNfcService().doCloseWiredSeConnection();
            return 0;
        }
    };

    public void wiredSeInitialize() throws NoSuchElementException, RemoteException {
        Log.e(TAG, "wiredSeInitialize Enter");
        HwBinder.configureRpcThreadpool(2, false);
        if (mWiredSEHal == null) {
          mWiredSEHal = ITmsWiredSe.getService();
        }
        if (mWiredSEHal == null) {
          throw new NoSuchElementException("No HAL is provided for WiredSe");
        }
        Log.d(TAG, "get wired se hal success");
        mWiredSEHal.setWiredSeCallback(mWiredSeCallback);
        mWiredSEHal.linkToDeath(mWiredSeDeathRecipient, 0);
    }

    public void wiredSeDeInitialize() throws NoSuchElementException, RemoteException {
        Log.e(TAG, "wiredSeDeInitialize Enter");
        mWiredSEHal.setWiredSeCallback(null);
    }

    class WiredSeDeathRecipient implements HwBinder.DeathRecipient {
        @Override
        public void serviceDied(long cookie) {
            try {
                Log.d(TAG, "WiredSe: serviceDied !!");
                if(sNfcWiredSeHandle > 0) {
                    mWiredSeCallback.closeWiredSe(sNfcWiredSeHandle);
                    sNfcWiredSeHandle = 0;
                }
                mWiredSEHal.unlinkToDeath(mWiredSeDeathRecipient);
                mWiredSEHal = null;
                // NfcService.getInstance().isWiredOpen = false;
                mWiredSeHandler.sendMessageDelayed(mWiredSeHandler.obtainMessage(EVENT_GET_HAL, 0),
                    GET_SERVICE_DELAY_MILLIS);
            }catch(Exception e) {
                e.printStackTrace();
            }
        }
    }

    private Handler mWiredSeHandler = new Handler(Looper.getMainLooper()) {
        @Override
        public void handleMessage(Message message) {
            switch (message.what) {
                case EVENT_GET_HAL:
                    try {
                        if(sWiredSeGetHalRetry > MAX_GET_HAL_RETRY) {
                            Log.e(TAG, "WiredSe GET_HAL retry failed");
                            sWiredSeGetHalRetry = 0;
                            break;
                        }
                        wiredSeInitialize();
                        sWiredSeGetHalRetry = 0;
                    } catch (Exception e) {
                        Log.e(TAG, " could not get the service. trying again");
                        sWiredSeGetHalRetry++;
                        sendMessageDelayed(obtainMessage(EVENT_GET_HAL, 0),
                            GET_SERVICE_DELAY_MILLIS);
                    }
                    break;
                default:
                    break;
            }
        }
    };
}
