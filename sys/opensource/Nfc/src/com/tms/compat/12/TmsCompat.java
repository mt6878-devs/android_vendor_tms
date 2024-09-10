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
import android.view.SurfaceControl;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.bluetooth.OobData;
import android.bluetooth.BluetoothDevice;
import android.view.Display;
import android.os.IBinder;
import android.nfc.cardemulation.ApduServiceInfo;


public class TmsCompat{
    public static Bitmap getScreenshotBitmap(Rect crop, int width, int height, Display dispaly){
        IBinder displayToken = SurfaceControl.getInternalDisplayToken();
        final SurfaceControl.DisplayCaptureArgs captureArgs =
                new SurfaceControl.DisplayCaptureArgs.Builder(displayToken)
                        .setSourceCrop(crop)
                        .setSize(width, height)
                        .build();
        SurfaceControl.ScreenshotHardwareBuffer screenshotBuffer =
                SurfaceControl.captureDisplay(captureArgs);
        return screenshotBuffer == null ? null : screenshotBuffer.asBitmap();
    }
    public static OobData getOobData(byte[] leScC, byte[] leScR, byte[] securityManagerTK, byte[] bdaddr, byte[] nameBytes, byte role){
        OobData oobData = new OobData.LeBuilder(leScC, bdaddr, (int)(role & 0xFF))
                .setRandomizerHash(leScR)
                .setDeviceName(nameBytes)
                .setLeTemporaryKey(securityManagerTK)
                .build();
        return oobData;
    }
    public static boolean createBondOutOfBand(BluetoothDevice device, int transport, OobData oobdata){
        return device.createBondOutOfBand(transport,null, oobdata);
    }
    public static boolean isRequiresScreenOn(boolean onHost, ApduServiceInfo serviceInfo) {
        return serviceInfo.requiresScreenOn();
    }
}

