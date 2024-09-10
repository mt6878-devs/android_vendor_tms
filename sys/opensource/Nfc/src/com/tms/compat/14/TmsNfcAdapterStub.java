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

import android.content.Context;
import android.nfc.AvailableNfcAntenna;
import android.nfc.INfcAdapter;
import android.nfc.NfcAdapter;
import android.nfc.NfcAntennaInfo;
import android.os.RemoteException;
import android.util.Log;
import com.android.nfc.NfcPermissions;
import com.android.nfc.NfcService;
import com.android.nfc.R;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

public abstract class TmsNfcAdapterStub extends INfcAdapter.Stub {
    private static final String TAG = "TmsNfcAdapterStub-14";
    private static final boolean DBG = NfcService.DBG; // TMS_NFC
    private Context mContext;
    private NfcService mNfcService;
    private boolean mIsTagAppPrefSupported;
    private HashMap<Integer, HashMap<String, Boolean>> mTagAppPrefList;

    public TmsNfcAdapterStub(Context context) {
        mContext = context;
    }

    public void initTmsNfcAdapterStub() {
        mNfcService = NfcService.getInstance();
        mIsTagAppPrefSupported = getNfcServiceFiledNotNull("mIsTagAppPrefSupported", Boolean.class);
        mTagAppPrefList = getNfcServiceFiledNotNull("mTagAppPrefList", HashMap.class);
    }

    @Override
    public NfcAntennaInfo getNfcAntennaInfo() {
        int positionX[] = mContext.getResources().getIntArray(
                R.array.antenna_x);
        int positionY[] = mContext.getResources().getIntArray(
                R.array.antenna_y);
        if(positionX.length != positionY.length){
            return null;
        }
        int width = mContext.getResources().getInteger(R.integer.device_width);
        int height = mContext.getResources().getInteger(R.integer.device_height);
        List<AvailableNfcAntenna> availableNfcAntennas = new ArrayList<>();
        for(int i = 0; i < positionX.length; i++){
            if(positionX[i] >= width | positionY[i] >= height){
                return null;
            }
            availableNfcAntennas.add(new AvailableNfcAntenna(positionX[i], positionY[i]));
        }
        return new NfcAntennaInfo(
                width,
                height,
                mContext.getResources().getBoolean(R.bool.device_foldable),
                availableNfcAntennas);
    }

    @Override
    public boolean isTagIntentAppPreferenceSupported() throws RemoteException {
        NfcPermissions.enforceAdminPermissions(mContext);
        return mIsTagAppPrefSupported;
    }
    @Override
    public Map getTagIntentAppPreferenceForUser(int userId) throws RemoteException {
        NfcPermissions.enforceAdminPermissions(mContext);
        if (!mIsTagAppPrefSupported) throw new UnsupportedOperationException();
        synchronized (mNfcService) {
            return mTagAppPrefList.getOrDefault(userId, new HashMap<>());
        }
    }
    @Override
    public int setTagIntentAppPreferenceForUser(int userId,
            String pkg, boolean allow) throws RemoteException {
        NfcPermissions.enforceAdminPermissions(mContext);
        if (!mIsTagAppPrefSupported) throw new UnsupportedOperationException();
        return setTagAppPreferenceInternal(userId, pkg, allow);
    }

    private int setTagAppPreferenceInternal(int userId, String pkg, boolean allow) {
        if (!isPackageInstalled(pkg, userId)) {
            return NfcAdapter.TAG_INTENT_APP_PREF_RESULT_PACKAGE_NOT_FOUND;
        }
        if (DBG) Log.i(TAG, "UserId:" + userId + " pkg:" + pkg + ":" + allow);
        synchronized (mNfcService) {
            mTagAppPrefList.computeIfAbsent(userId, key -> new HashMap<String, Boolean>())
                    .put(pkg, allow);
        }
        storeTagAppPrefList();
        return NfcAdapter.TAG_INTENT_APP_PREF_RESULT_SUCCESS;
    }

    private boolean isPackageInstalled(String pkgName, int userId) {
        Class<?>[] paramsClazz = new Class<?>[2];
        Object[] params = new Object[2];
        paramsClazz[0] = String.class;
        paramsClazz[1] = Integer.class;
        params[0] = pkgName;
        params[1] = userId;
        Optional<Boolean> opt = callNfcServiceMethod("isPackageInstalled", paramsClazz, params, Boolean.class);
        if (opt.isPresent()) {
            return opt.get();
        }
        throw new IllegalArgumentException("call NfcService isPackageInstalled failed");
    }

    private void storeTagAppPrefList() {
        Optional<Void> opt = callNfcServiceMethod("isPackageInstalled", new Class<?>[0], new Object[0], Void.class);
        if (opt.isPresent()) {
            opt.get();
        }
        throw new IllegalArgumentException("call NfcService isPackageInstalled failed");
    }

    private <T> Optional<T> getNfcServiceFiled(String fieldName, Class<T> clazz) {
        try {
            Field field = mNfcService.getClass().getDeclaredField(fieldName);
            field.setAccessible(true);
            Object obj = field.get(mNfcService);
            return Optional.ofNullable(clazz.cast(obj));
        } catch (NoSuchFieldException | SecurityException | IllegalArgumentException | IllegalAccessException e) {
            TmsLog.e(TAG, "getNfcServiceFiled: " + fieldName + ", failed", e);
        }
        return Optional.empty();
    }

    private <T> T getNfcServiceFiledNotNull(String fieldName, Class<T> clazz) {
        Optional<T> opt = getNfcServiceFiled(fieldName, clazz);
        if (opt.isPresent()) {
            return opt.get();
        }
        throw new NullPointerException("getNfcServiceFiled: " + fieldName + ", failed");
    }

    private <T> Optional<T> callNfcServiceMethod(String methodName, Class<?>[] paramsClazz, Object[] params, Class<T> returnClass) {
        try {
            Method method = mNfcService.getClass().getDeclaredMethod(methodName, paramsClazz);
            method.setAccessible(true);
            Object obj = method.invoke(mNfcService, params);
            if (returnClass == null) {
                return Optional.empty();
            }
            return Optional.ofNullable(returnClass.cast(obj));
        } catch (NoSuchMethodException | SecurityException | IllegalArgumentException | IllegalAccessException | InvocationTargetException e) {
            TmsLog.e(TAG, "callNfcServiceMethod: " + methodName + ", failed", e);
        }
        return Optional.empty();
    }
}


