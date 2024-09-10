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

import java.util.Locale;
import android.bluetooth.BluetoothA2dp;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothHeadset;
import android.os.Build;
import java.lang.reflect.Method;

public class TmsUtils {
    public static final int BLUETOOTH_PROFILE_CONNECTION_POLICY_FORBIDDEN = 0;
    public static final int BLUETOOTH_PROFILE_PRIORITY_OFF = 0;

    public static String byteArray2Hex(byte[] input) {
        if (input == null || input.length == 0) {
            return "";
        }
        StringBuilder output = new StringBuilder(input.length * 2);
        for (byte b : input) {
            String s = Integer.toHexString(b & 0xFF);
            if (s.length() == 1) {
                output.append("0");
            }
            output.append(s);
        }
        return output.toString().toUpperCase(Locale.ENGLISH);
    }

    public static byte[] hexString2ByteArray(String s) {
        if (s == null || s.length() == 0) return null;
        int len = s.length();
        if (len % 2 != 0) {
            s = '0' + s;
            len++;
        }
        byte[] data = new byte[len / 2];
        for (int i = 0; i < len; i += 2) {
            data[i / 2] = (byte) ((Character.digit(s.charAt(i), 16) << 4)
                    + Character.digit(s.charAt(i + 1), 16));
        }
        return data;
    }

    public static String getSuffix(String input) {
        if (input == null || input.length() == 0) {
            return "";
        }
        int index = input.lastIndexOf(".");
        return index < 0 ? input : input.substring(index + 1);
    }

    public static boolean isHeadsetProfileAvailable(BluetoothHeadset headset, BluetoothDevice device){

        try {
            Class cls = headset.getClass();

            if(Build.VERSION.SDK_INT <= Build.VERSION_CODES.Q){
                Method method = cls.getMethod("getPriority", BluetoothDevice.class);
                if((Integer)method.invoke(headset, device) == BLUETOOTH_PROFILE_PRIORITY_OFF) {
                    return false;
                } else {
                    return true;
                }
            } else {
                Method method = cls.getMethod("getConnectionPolicy", BluetoothDevice.class);
                if((Integer) method.invoke(headset, device) == BLUETOOTH_PROFILE_CONNECTION_POLICY_FORBIDDEN) {
                    return false;
                } else {
                    return true;
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return false;
    }

    public static boolean isA2dpProfileAvailable(BluetoothA2dp a2dp, BluetoothDevice device){

        try {
            Class cls = a2dp.getClass();
            if(Build.VERSION.SDK_INT <= Build.VERSION_CODES.Q){
                Method method = cls.getMethod("getPriority", BluetoothDevice.class);
                if((Integer)method.invoke(a2dp, device) == BLUETOOTH_PROFILE_PRIORITY_OFF) {
                    return false;
                } else {
                    return true;
                }
            } else {
                Method method = cls.getMethod("getConnectionPolicy", BluetoothDevice.class);
                if((Integer) method.invoke(a2dp, device) == BLUETOOTH_PROFILE_CONNECTION_POLICY_FORBIDDEN) {
                    return false;
                } else {
                    return true;
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return false;
    }

}
