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

import java.io.File;
import java.io.FileWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;
import java.util.Optional;
import java.util.regex.Matcher;
import java.util.NoSuchElementException;

import com.android.nfc.DeviceHost;
import com.android.nfc.NfcDiscoveryParameters;
import com.android.nfc.NfcPermissions;
import com.android.nfc.NfcService;
import com.android.nfc.cardemulation.AidRoutingManager;
import com.android.nfc.cardemulation.CardEmulationManager;
import com.tms.nfc.dhimpl.TmsNativeNfcManager;
import com.tms.nfc.dhimpl.TmsNativeWiredSe;
import com.tms.nfc.TmsDeviceHost.TmsDeviceHostListener;

import android.content.Context;
import android.content.SharedPreferences;
import android.nfc.INfcAdapter;
import android.nfc.INfcAdapterExtras;
import android.nfc.NfcAdapter;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.PowerManager;
import android.os.Message;
import android.os.RemoteException;

import com.tms.nfc.ITmsNfcAdapter;
import com.tms.nfc.IHciAdapter;
import com.tms.nfc.IHciCallback;

public class TmsNfcService implements TmsDeviceHostListener {

    private static final boolean DBG = NfcService.DBG; // TMS_NFC

    private static final String TAG = "TmsNfcService";

    private static final String PREF_DEFAULT_AID_ROUTE = "default_aid_route";
    private static final String PREF_DEFAULT_MIFARE_ROUTE = "default_mifare_route";
    private static final String PREF_DEFAULT_ISODEP_ROUTE = "default_iso_dep_route";
    private static final String PREF_DEFAULT_FELICA_ROUTE = "default_felica_route";
    private static final String PREF_DEFAULT_AB_TECH_ROUTE = "default_default_ab_tech_route";
    private static final String PREF_DEFAULT_SC_ROUTE = "default_sc_route";
    private static final String PREF_T4T_NFCEE_ENABLE = "t4t_nfcee_enable";
    private static final String PREF_HCE_TYPEA_ENABLE = "hce_typea_enable";
    private static final String PREF_HCE_TYPEA_ATQA = "hce_typea_atqa";
    private static final String PREF_HCE_TYPEA_SAK = "hce_typea_sak";
    private static final String PREF_HCE_TYPEA_UID = "hce_typea_uid";

    private static final String NAME_TMS_T4T_NFCEE_ENABLE = "TMS_T4T_NFCEE_ENABLE";

    private static final int TRANSIT_SETCONFIG_STAT_SUCCESS = 0x00;
    private static final int TRANSIT_SETCONFIG_STAT_FAILED  = 0xFF;
    private static final byte PROP_APDU_INS = 0x30;

    private static final int NFC_POLL_A = 0x01;
    private static final int NFC_POLL_B = 0x02;
    private static final int NFC_POLL_F = 0x04;
    private static final int NFC_POLL_V = 0x08;
    private static final int NFC_POLL_B_PRIME = 0x10;
    private static final int NFC_POLL_KOVIO = 0x20;

    private static final int FEATURE_DEFAULT_STATE_MUTE_RATS = TmsNfcAdapter.FEATURE_STATE_ENABLE;

    public static final String T4T_NFCEE_AID = "D2760000850101";
    public static final int ROUTE_ID_T4T_NFCEE = 0x10;
    public static final int AID_MATCHING_EXACT_ONLY = 0x02;

    public static final int MSG_INIT_WIRED_SE = NfcService.MSG_TMS_EXTNS_BEGINS + 1;
    public static final int MSG_READ_T4TNFCEE = NfcService.MSG_TMS_EXTNS_BEGINS + 2;
    public static final int MSG_WRITE_T4TNFCEE = NfcService.MSG_TMS_EXTNS_BEGINS + 3;
    public static final int MSG_ENABLE_T4TNFCEE = NfcService.MSG_TMS_EXTNS_BEGINS + 4;
    public static final int MSG_T4T_SEND_RAW_APDU = NfcService.MSG_TMS_EXTNS_BEGINS + 5;
    public static final int MSG_STOP_SILENT_FIELD_DETECT = NfcService.MSG_TMS_EXTNS_BEGINS + 6;
    // Equal to NfcService.MSG_RF_FIELD_DEACTIVATED
    private static final int MSG_RF_FIELD_DEACTIVATED = 10;

    private static final int M1_RAW_DATA_MODE_STATE_OFF = TmsNfcAdapter.M1_RAW_DATA_MODE_STATE_OFF;
    private static final int M1_RAW_DATA_MODE_STATE_TURNING_ON = TmsNfcAdapter.M1_RAW_DATA_MODE_STATE_TURNING_ON;
    private static final int M1_RAW_DATA_MODE_STATE_ON = TmsNfcAdapter.M1_RAW_DATA_MODE_STATE_ON;

    private static final int STATUS_SUCCESS = 0;
    private static final int STATUS_FAILED = -1;
    private static final int ERROR_STATUS_BUSY = -2;
    private static final int ERROR_NFC_ON = -3;
    private static final int ERROR_EMPTY_PAYLOAD = -4;
    private static final int ERROR_INVALID_LENGTH = -5;
    private static final int ERROR_INVALID_COMMAND = -6;
    private static final int T4TNFCEE_STATUS_FAILED = -1;

    private final Context mContext;
    private final TmsDeviceHost mTmsDeviceHost;
    private final NfcService mNfcService;
    private final TmsNfcAdapterService mTmsNfcAdapterService;
    private final TmsHciAdapterService mTmsHciAdapterService;
    private final AidRoutingManager mAidRoutingManager;
    private static boolean isInit = false;
    private static String sDefaultRoute = null;
    private PowerManager mPowerManager;

    private final int MSG_COMMIT_ROUTING;

    private DeviceHost mDeviceHost;
    private CardEmulationManager mCardEmulationManager;
    private Handler mHandler;
    private SharedPreferences mPrefs;
    private SharedPreferences.Editor mPrefsEditor;
    private INfcAdapter mNfcAdapter;
    private boolean mIsSecureNfcEnabled;
    private volatile boolean mCheckNfcState = true;

    private boolean mIsForceUpdateAidRoute = false;
    private int mTmsPollMask = -1;

    private int mEmptyAidPowerState = 0xFF;
    private int mEmptyAidRoute = 0xFF;
    private int mM1RawDataModeState = M1_RAW_DATA_MODE_STATE_OFF;

    private static final int RF_FIELD_ON_MIN_INTERVAL = 1500;
    private long mLastRfFieldActivatedTime = 0L;
    private boolean mRfFieldDeactivatedAllowed = false;
    private boolean mIsSilentFieldDetectModeEnabled = false;
    private int mSilentFieldDetectTimeout = -1;

    private PowerManager.WakeLock mWiredSeWakeLock;
    private TmsNativeWiredSe mTmsWiredSe;
    Class mTmsWiredSeClass;
    Object mTmsWiredSeObj;

    private INfcAdapterExtras mNfcAdapterExtraService;

    private Object mT4tNfcEeObj = new Object();
    private Bundle mT4tNfceeReturnBundle = new Bundle();

    private HashMap<String, Integer> mFeatureStateMap = new HashMap<>();

    {
        mFeatureStateMap.put(TmsNfcAdapter.FEATURE_NAME_MUTE_RATS, FEATURE_DEFAULT_STATE_MUTE_RATS);
    }

    public TmsNfcService(NfcService nfcService, Context context) {
        this.mNfcService = nfcService;
        this.mContext = context;
        this.mTmsDeviceHost = new TmsNativeNfcManager(this);
        this.mTmsNfcAdapterService = new TmsNfcAdapterService();
        this.mTmsHciAdapterService = new TmsHciAdapterService();

        this.mAidRoutingManager = new AidRoutingManager();

        MSG_COMMIT_ROUTING = getNfcServiceFiledNotNull("MSG_COMMIT_ROUTING", Integer.class);

        mDeviceHost = getNfcServiceFiledNotNull("mDeviceHost", DeviceHost.class);
        mCardEmulationManager = getNfcServiceFiledNotNull("mCardEmulationManager", CardEmulationManager.class);
        mHandler = getNfcServiceFiledNotNull("mHandler", Handler.class);
        mPrefs = getNfcServiceFiledNotNull("mPrefs", SharedPreferences.class);
        mPrefsEditor = mPrefs.edit();
        mNfcAdapter = getNfcServiceFiledNotNull("mNfcAdapter", INfcAdapter.class);
        mIsSecureNfcEnabled = getNfcServiceFiledNotNull("mIsSecureNfcEnabled", Boolean.class);
        mPowerManager = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
        mTmsWiredSe = new TmsNativeWiredSe();
        mWiredSeWakeLock = mPowerManager.newWakeLock(
                PowerManager.PARTIAL_WAKE_LOCK, "TmsNfcService:mWiredSeWakeLock");

        try {
            mTmsWiredSeClass = Class.forName("com.tms.nfc.TmsWiredSeService");
            mTmsWiredSeObj = mTmsWiredSeClass.newInstance();
        } catch (ClassNotFoundException | IllegalAccessException e){
            TmsLog.d(TAG, "TmsWiredSeService Class not found");
        } catch (InstantiationException e) {
            TmsLog.e(TAG, "TmsWiredSeService object Instantiation failed");
        } catch (Exception e) {
            TmsLog.e(TAG, e.toString());
        }

        try {
            Object obj = Class.forName("com.tms.nfc.TmsNfcAdapterExtrasService").getDeclaredConstructor().newInstance();
            mNfcAdapterExtraService = INfcAdapterExtras.class.cast(obj);
        } catch (Exception e) {
            TmsLog.e(TAG, "new TmsNfcAdapterExtrasService failed", e);
        }

        // When TMS_SET_ALL_ROUTE_DEFAULT equals 1, PreferredServices will check and set default route
        // from default payment before the constructor of TmsNfcService, also need to put the default route
        // into SharedPreferences after that.
        if (sDefaultRoute != null) {
            setAllRouteToPrefs(sDefaultRoute);
        }
        isInit = true;
    }

    public static void setInitDefaultRoute(String route) {
        // Default route will be put into SharedPreferences depend on default payment after NfcService
        // new a TmsNfcService.
        if (!isInit) {
            sDefaultRoute = route;
        } else {
            TmsLog.i(TAG, "TmsNfcService already init, stop setting initial route.");
        }
    }

    public ITmsNfcAdapter getTmsNfcAdapterService() {
        return mTmsNfcAdapterService;
    }

    public INfcAdapterExtras getNfcAdapterExtrasService() {
        int disableWiredSE = TmsConfig.getInt(TmsConstant.CONFIG_DISABLE_WIRED_SE, TmsConstant.DEFAULT_DISABLE_WIRED_SE_VALUE);
        TmsLog.i(TAG, "getNfcAdapterExtrasService called, disableWiredSE = " + disableWiredSE);
        if (disableWiredSE != TmsConstant.DEFAULT_DISABLE_WIRED_SE_VALUE) {
            return null;
        }
        return mNfcAdapterExtraService;
    }

    public void tmsNfcServiceHandler(Message msg) {
        switch (msg.what) {
            case MSG_INIT_WIRED_SE:
                handleInitWiredSe();
                break;
            case MSG_WRITE_T4TNFCEE: {
                Bundle writeBundle = (Bundle) msg.obj;
                byte[] fileId = writeBundle.getByteArray("fileId");
                byte[] writeData = writeBundle.getByteArray("writeData");
                int length = writeBundle.getInt("length");
                int status = mTmsDeviceHost.doWriteT4tData(fileId, writeData, length);
                mT4tNfceeReturnBundle.putInt("writeStatus", status);
                synchronized (mT4tNfcEeObj) {
                    mT4tNfcEeObj.notify();
                }
                break;
            }
            case MSG_READ_T4TNFCEE: {
                Bundle readBundle = (Bundle) msg.obj;
                byte[] fileId = readBundle.getByteArray("fileId");
                byte[] readData = mTmsDeviceHost.doReadT4tData(fileId);
                mT4tNfceeReturnBundle.putByteArray("readData", readData);
                synchronized (mT4tNfcEeObj) {
                    mT4tNfcEeObj.notify();
                }
                break;
            }
            case MSG_ENABLE_T4TNFCEE:
                Bundle enableBundle = (Bundle) msg.obj;
                boolean enable = enableBundle.getBoolean("enable");
                boolean enableStatus = mTmsDeviceHost.enableT4tNfcee(enable);
                mT4tNfceeReturnBundle.putBoolean("enableStatus", enableStatus);
                synchronized (mT4tNfcEeObj) {
                    mT4tNfcEeObj.notify();
                }
                break;
            case MSG_T4T_SEND_RAW_APDU: {
                Bundle apduBundle = (Bundle) msg.obj;
                byte[] apdu = apduBundle.getByteArray("apdu");
                int apduLen = apduBundle.getInt("apduLen");
                byte[] response = mTmsDeviceHost.doSendT4tRawApdu(apdu, apduLen);
                mT4tNfceeReturnBundle.putByteArray("response", response);
                synchronized (mT4tNfcEeObj) {
                    mT4tNfcEeObj.notify();
                }
                break;
            }
            case MSG_STOP_SILENT_FIELD_DETECT: {
                mTmsNfcAdapterService.stopSilentFieldDetectMode();
                break;
            }
        }
    }

    public void commitRouting() {
        TmsLog.d(TAG, "commitRouting() called");
        setUserDefaultRoutesPref();
    }

    public void handleCommitRouting() {
        TmsLog.d(TAG, "handleCommitRouting() called");
        setEmptyAidRoute();
    }

    private void initWiredSe() {
        TmsLog.d(TAG, "Init wired Se");
        mHandler.sendEmptyMessage(MSG_INIT_WIRED_SE);
    }

    public void handleInitWiredSe() {
        TmsLog.d(TAG, "handleInitWiredSe");
        try {
           Method mTmsWiredSeInitMethod = mTmsWiredSeClass.getDeclaredMethod("wiredSeInitialize");
           mTmsWiredSeInitMethod.invoke(mTmsWiredSeObj);
         } catch (NoSuchElementException | NoSuchMethodException e) {
           TmsLog.i(TAG, "No such Method wiredSeInitialize");
         } catch (RuntimeException | IllegalAccessException | InvocationTargetException e) {
           TmsLog.e(TAG, "Error in invoking wiredSeInitialize invocation");
           e.printStackTrace();
         } catch (Exception e) {
           TmsLog.e(TAG, e.toString());
         }
    }

    public void handleDeinitWiredSe() {
        TmsLog.d(TAG, "handleDeinitWiredSe");
        try {
           Method mTmsWiredSeInitMethod = mTmsWiredSeClass.getDeclaredMethod("wiredSeDeInitialize");
           mTmsWiredSeInitMethod.invoke(mTmsWiredSeObj);
         } catch (NoSuchElementException | NoSuchMethodException e) {
           TmsLog.i(TAG, "No such Method wiredSeDeInitialize");
         } catch (RuntimeException | IllegalAccessException | InvocationTargetException e) {
           TmsLog.e(TAG, "Error in invoking wiredSeDeInitialize invocation");
         } catch (Exception e) {
           TmsLog.e(TAG, e.toString());
         }
    }

    public void afterEnableInternal() {
        TmsLog.d(TAG, "afterEnableInternal() called");

        mNfcService.commitRouting();

        initWiredSe();

        int listenMask = mPrefs.getInt(TmsConstant.PREFS_RF_LISTEN_MASK, -1);
        if (listenMask != -1) {
            mTmsDeviceHost.setRfListenMask(listenMask, false);
        }

        // enable lx debug upload from config, libnfc-nci.conf
        int lxDebugLevel = TmsConfig.getInt(TmsConstant.CONFIG_LX_DEBUG_LEVEL, TmsConstant.DEFAULT_LX_DEBUG_LEVEL);
        mTmsDeviceHost.doSetLxDebugUploadEnabled(true, lxDebugLevel);

        boolean hce_typea_enable = mPrefs.getBoolean(PREF_HCE_TYPEA_ENABLE, false);
        if (hce_typea_enable) {
            byte[] hce_typea_atqa = TmsUtils.hexString2ByteArray(mPrefs.getString(PREF_HCE_TYPEA_ATQA, ""));
            byte[] hce_typea_sak = TmsUtils.hexString2ByteArray(mPrefs.getString(PREF_HCE_TYPEA_SAK, ""));
            byte[] hce_typea_uid = TmsUtils.hexString2ByteArray(mPrefs.getString(PREF_HCE_TYPEA_UID, ""));
            doSetHceTypeAConfig(hce_typea_enable, hce_typea_atqa, hce_typea_sak, hce_typea_uid);
        }
    }

    public void beforeDisableInternal() {
        TmsLog.d(TAG, "beforeDisableInternal() called");
        mDeviceHost.disableDiscovery();

        handleDeinitWiredSe();

        // disable all lx debug upload
        mTmsDeviceHost.doSetLxDebugUploadEnabled(false, TmsConstant.LX_DEBUG_LEVEL_ALL);

        cleanupSilentFieldDetectMode();
        cleanupFeatureState();
    }

    public boolean isCustomRfPollingControlEnable() {
        return mTmsPollMask != -1;
    }

    public void computeDiscoveryParameters(NfcDiscoveryParameters.Builder paramsBuilder) {
        TmsLog.d(TAG, "computeDiscoveryParameters() called, mTmsPollMask = " + mTmsPollMask);
        paramsBuilder.setTechMask(mTmsPollMask);
    }

    public void setNfcStateCheck(boolean check) {
        mCheckNfcState = check;
    }

    public boolean needCheckNfcState() {
        return mCheckNfcState;
    }

    @Override
    public void onPtmResponseReceive(int event, byte[] data) {
        mTmsHciAdapterService.onNciResponseReceive(event, data);
    }

    private String convertNfceeIdTypeToPrefRoute(int nfceeId) {
        TmsLog.d(TAG, "convertNfceeIdTypeToPrefRoute() called with: nfceeId = [" + nfceeId + "]");
        String prefs = TmsNfcAdapter.DEFAULT_ROUTE;
        switch (nfceeId) {
            case 0x00:
                prefs = "HCE";
                break;
            case 0x80:
                prefs = "UICC";
                break;
            case 0x81:
                prefs = "UICC2";
                break;
            case 0xC0:
                prefs = "eSE";
                break;
        }
        return prefs;
    }

    private int convertPrefRouteToNfceeIdType(String route) {
        String prefRoute = mPrefs.getString(route, TmsNfcAdapter.DEFAULT_ROUTE);
        int nciId = 0xFF;

        // set nciId a default value form config
        if (PREF_DEFAULT_AID_ROUTE.equals(route)) {
            nciId = mAidRoutingManager.doGetDefaultRouteDestination();
        } else if (PREF_DEFAULT_ISODEP_ROUTE.equals(route)) {
            nciId = mAidRoutingManager.doGetDefaultIsoDepRouteDestination();
        } else if (PREF_DEFAULT_FELICA_ROUTE.equals(route)) {
            nciId = mAidRoutingManager.doGetDefaultFelicaRouteDestination();
        } else if (PREF_DEFAULT_SC_ROUTE.equals(route)) {
            nciId = mAidRoutingManager.doGetDefaultSysCodeRouteDestination();
        } else {
            // include PREF_DEFAULT_MIFARE_ROUTE, PREF_DEFAULT_AB_TECH_ROUTE and other
            nciId = mAidRoutingManager.doGetDefaultOffHostRouteDestination();
        }

        if (prefRoute.equals("UICC")) {
            nciId = 0x80;
        } else if (prefRoute.equals("UICC2")) {
            nciId = 0x81;
        } else if (prefRoute.equals("eSE")) {
            nciId = 0xC0;
        } else if (prefRoute.equals("HCE")) {
            nciId = 0x00;
        }

        if (DBG) {
            TmsLog.d(TAG, "convertPrefRouteToNfceeIdType() - route Id: "
                            + route + ", prefRoute: "  + prefRoute + ", route: " + String.format("0x%02X", nciId));
        }

        return (nciId & 0xFF);
    }

    public boolean isForceUpdateAiRoute() {
        return mIsForceUpdateAidRoute;
    }

    /**
     * get default T4TNfcee power state supported
     */
    public int getT4TNfceePowerState() {
        int powerState = mTmsDeviceHost.doGetT4TNfceePowerState();
        if (mIsSecureNfcEnabled) {
          /* Secure nfc on,Setting power state screen on unlocked */
          powerState=0x01;
        }
        if (DBG) TmsLog.d(TAG, "T4TNfceePowerState : " + powerState);
        return powerState;
    }

    public boolean checkT4TNfceeFeature() {
        boolean ret = false;
        if (TmsConfig.hasKey(NAME_TMS_T4T_NFCEE_ENABLE)) {
            int enable = TmsConfig.getInt(NAME_TMS_T4T_NFCEE_ENABLE, 0x00);
            ret = (enable == 0x01);
        }
        return ret;
    }

    public void addT4TNfceeAid() {
        if (!checkT4TNfceeFeature()) {
            TmsLog.d(TAG, "T4t Nfcee feature is not enable. Stop adding T4T Nfcee Aid.");
            return;
        }
        boolean enable = mPrefs.getBoolean(PREF_T4T_NFCEE_ENABLE, true);
        if (enable) {
            TmsLog.i(TAG, "Adding T4T Nfcee AID");
            mNfcService.routeAids(T4T_NFCEE_AID, ROUTE_ID_T4T_NFCEE,
                    AID_MATCHING_EXACT_ONLY,
                    getT4TNfceePowerState());
        } else {
            mNfcService.unrouteAids(T4T_NFCEE_AID);
        }
    }

    public void updateRoutingTable() {
        TmsLog.i(TAG, "updateRoutingTable() ");

        mIsForceUpdateAidRoute = true;

        mAidRoutingManager.onNfccRoutingTableCleared();
        mCardEmulationManager.onRoutingTableChanged();

        mIsForceUpdateAidRoute = false;

        // If there was no AIDs to route, force routing
        if (mHandler.hasMessages(MSG_COMMIT_ROUTING) == false) {
            TmsLog.i(TAG, "updateRoutingTable() - No AID to route, force RT update");
            mHandler.sendEmptyMessage(MSG_COMMIT_ROUTING);
        }
    }

    private void setEmptyAidRoute() {
        int defaultAidRoute = getDefaultRoute(PREF_DEFAULT_AID_ROUTE);
        mDeviceHost.unrouteAid(TmsUtils.hexString2ByteArray(""));
        if (mEmptyAidRoute == 0xFF) {
            mEmptyAidRoute = defaultAidRoute;
        }
        if (mEmptyAidPowerState == 0xFF) {
            mEmptyAidPowerState = (mEmptyAidRoute == 0x00) ? 0x11 : 0x3B;
        }
        TmsLog.d(TAG, "setEmptyAidRoute() called with: route = [" + mEmptyAidRoute + "], powerstate = [" + mEmptyAidPowerState + "]");
        mTmsDeviceHost.setEmptyAidRoute(mEmptyAidRoute, mEmptyAidPowerState);
    }

    public void setActualEmptyAidPowerState(int power) {
        TmsLog.d(TAG, "setActualEmptyAidPowerState() called with: power = [" + power + "]");
        mEmptyAidPowerState = power;
    }

    public void setActualEmptyAidRoute(int route) {
        TmsLog.d(TAG, "setActualEmptyAidRoute() called with: route = [" + route + "]");
        mEmptyAidRoute = route;
    }

    public void setAllRouteToPrefs(String route) {
        TmsLog.d(TAG, "setAllRouteToPrefs() called with: route = [" + route + "]");
        mPrefsEditor = mPrefs.edit();
        mPrefsEditor.putString(PREF_DEFAULT_AID_ROUTE, route);
        mPrefsEditor.putString(PREF_DEFAULT_MIFARE_ROUTE, route);
        mPrefsEditor.putString(PREF_DEFAULT_ISODEP_ROUTE, route);
        mPrefsEditor.putString(PREF_DEFAULT_FELICA_ROUTE, route);
        mPrefsEditor.putString(PREF_DEFAULT_AB_TECH_ROUTE, route);
        mPrefsEditor.putString(PREF_DEFAULT_SC_ROUTE, route);
        mPrefsEditor.commit();
    }

    public void setAllRouteToPrefs(int route) {
        TmsLog.d(TAG, "setAllRouteToPrefs() called with: route = [" + route + "]");
        setAllRouteToPrefs(convertNfceeIdTypeToPrefRoute(route));
    }

    public int getDefaultRoute() {
        TmsLog.d(TAG, "getDefaultRoute() called");
        return convertPrefRouteToNfceeIdType(PREF_DEFAULT_AID_ROUTE);
    }

    public int getDefaultRoute(String routeType) {
        TmsLog.d(TAG, "getDefaultRoute() called, routeType: " + routeType);
        return convertPrefRouteToNfceeIdType(routeType);
    }

    public void setUserDefaultRoutesPref() {
        TmsLog.d(TAG, "setUserDefaultRoutesPref() called");
        mTmsDeviceHost.setUserDefaultRoutesPref(
                convertPrefRouteToNfceeIdType(PREF_DEFAULT_MIFARE_ROUTE),
                convertPrefRouteToNfceeIdType(PREF_DEFAULT_ISODEP_ROUTE),
                convertPrefRouteToNfceeIdType(PREF_DEFAULT_FELICA_ROUTE),
                convertPrefRouteToNfceeIdType(PREF_DEFAULT_AB_TECH_ROUTE),
                convertPrefRouteToNfceeIdType(PREF_DEFAULT_SC_ROUTE),
                convertPrefRouteToNfceeIdType(PREF_DEFAULT_AID_ROUTE));
    }

    public int doOpenWiredSeConnection() {
        if (!isNfcEnabled()) {
            TmsLog.d(TAG, "doOpenWiredSeConnection: nfc is off");
            return 0;
        }
        mWiredSeWakeLock.acquire();
        try {
            return mTmsWiredSe.doOpenWiredSeConnection();
        } finally {
            mWiredSeWakeLock.release();
        }
    }

    public void doCloseWiredSeConnection() {
        if (!isNfcEnabled()) {
            TmsLog.d(TAG, "doCloseWiredSeConnection: nfc is off");
            return;
        }
        mWiredSeWakeLock.acquire();
        try {
            mTmsWiredSe.doCloseWiredSeConnection();
        } finally {
            mWiredSeWakeLock.release();
        }

    }

    public byte[] doWiredSeGetAtr() {
        if (!isNfcEnabled()) {
            TmsLog.d(TAG, "doWiredSeGetAtr: nfc is off");
            return null;
        }
        mWiredSeWakeLock.acquire();
        try {
            return mTmsWiredSe.doWiredSeGetAtr();
        } finally {
            mWiredSeWakeLock.release();
        }
    }

    public byte[] doWiredSeTransceive(byte[] cApdu) {
        if (!isNfcEnabled()) {
            TmsLog.d(TAG, "doWiredSeTransceive: nfc is off");
            return null;
        }
        mWiredSeWakeLock.acquire();
        try {
            return mTmsWiredSe.doWiredSeTransceive(cApdu);
        } finally {
            mWiredSeWakeLock.release();
        }
    }

    public boolean isM1RawDataOn() {
        TmsLog.i(TAG, "mM1RawDataModeState: " + mM1RawDataModeState);
        return mM1RawDataModeState == M1_RAW_DATA_MODE_STATE_ON;
    }

    public synchronized boolean setM1RawDataModeEnable(boolean isEnable) {
        TmsLog.i(TAG, "setM1RawDataModeEnable : " + isEnable);
        if (!isEnable && mM1RawDataModeState == M1_RAW_DATA_MODE_STATE_OFF) {
            TmsLog.w(TAG, "setM1RawDataModeEnable: m1 raw data mode is already off");
            return true;
        }

        if (isEnable && mM1RawDataModeState >=  M1_RAW_DATA_MODE_STATE_ON) {
            TmsLog.w(TAG, "setM1RawDataModeEnable: m1 raw data mode is already on");
            return true;
        }

        byte status = (byte) (isEnable ? 0x01 : 0x00);
        byte[] cmd = new byte[] {0x20, 0x02, 0x05, 0x01, (byte) 0xA2, 0x45, 0x01, status};
        TmsLog.d(TAG, "setM1RawDataModeEnable: send > " + TmsUtils.byteArray2Hex(cmd));
        byte[] rsp = sendNciCommand(cmd);
        TmsLog.d(TAG, "setM1RawDataModeEnable: recv > " + TmsUtils.byteArray2Hex(rsp));
        if (rsp == null || rsp.length != 5 || rsp[4] != 0) {
            TmsLog.w(TAG, "setM1RawDataModeEnable: enable/disable auth detect failed");
            return false;
        }
        mTmsDeviceHost.setM1RawDataModeEnable(isEnable);
        mM1RawDataModeState = isEnable ? M1_RAW_DATA_MODE_STATE_TURNING_ON : M1_RAW_DATA_MODE_STATE_OFF;
        TmsLog.i(TAG, "setM1RawDataModeEnable: set success");
        return true;
    }

    public synchronized boolean setM1RawDataModeTimeInterval(int timeInterval) {
        TmsLog.i(TAG, "setM1RawDataModeTimeInterval : " + timeInterval);

        byte timeIntervalByte = (byte) timeInterval;

        byte[] cmd = new byte[]{0x20, 0x02, 0x08, 0x01,(byte) 0xA2,(byte) 0x86, 0x04, timeIntervalByte, 0x00, 0x01, 0x00};
        TmsLog.d(TAG, "setM1RawDataModeTimeInterval: send > " + TmsUtils.byteArray2Hex(cmd));
        byte[] rsp = sendNciCommand(cmd);
        TmsLog.d(TAG, "setM1RawDataModeTimeInterval: recv < " + TmsUtils.byteArray2Hex(rsp));

        if (rsp != null && rsp.length == 5 && rsp[3] == 0x00) {
            TmsLog.i(TAG, "setM1RawDataModeTimeInterval: set success");
            return true;
        } else {
            TmsLog.e(TAG, "setM1RawDataModeTimeInterval: set failed");
            return false;
        }
    }

    @Override
    public synchronized void onM1RawDataAuthCallback(int status) {
        if (mM1RawDataModeState == M1_RAW_DATA_MODE_STATE_OFF) {
            TmsLog.w(TAG, "onM1RawDataAuthCallback, but m1 raw data mode is off");
            return;
        }
        mM1RawDataModeState = (status == 0) ? M1_RAW_DATA_MODE_STATE_ON : M1_RAW_DATA_MODE_STATE_OFF;
        mTmsNfcAdapterService.onM1RawDataModeAuthResult(status);
    }

    private synchronized byte[] sendT4tRawApduInternal(byte[] apdu, int apduLen) {
        Bundle apduBundle = new Bundle();
        apduBundle.putByteArray("apdu", apdu);
        apduBundle.putInt("apduLen", apduLen);
        try {
            sendMessage(MSG_T4T_SEND_RAW_APDU, apduBundle);
            synchronized (mT4tNfcEeObj) {
                mT4tNfcEeObj.wait(1000);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        byte[] response = mT4tNfceeReturnBundle.getByteArray("response");
        mT4tNfceeReturnBundle.clear();
        return response;
    }

    public byte[] sendNciCommand(byte[] cmd) {
        return sendNciCommand(cmd, false);
    }

    public byte[] sendNciCommand(byte[] cmd, boolean restartRfDiscovery) {
        TmsLog.d(TAG, "sendNciCommand() called with: cmd = [" + TmsUtils.byteArray2Hex(cmd) + "], restartRfDiscovery =" + restartRfDiscovery);
        try {
            byte[] rsp = mTmsDeviceHost.sendRawNciCommand(cmd, restartRfDiscovery);
            TmsLog.d(TAG, "sendNciCommand() receive rsp = [" + TmsUtils.byteArray2Hex(rsp) + "]");
            return rsp;
        } catch (RuntimeException e) {
            TmsLog.e(TAG, "sendNciCommand failed", e);
            return null;
        }
    }

    private int checkRfConfigCommandValidate(byte[] data) {
        if (data == null || data.length < 3) {
            return ERROR_INVALID_LENGTH;
        }
        if (data[0] != 0x20 || data[1] != 0x02) { // data[0] and data[1] means nci head
            return ERROR_INVALID_COMMAND;
        }
        int len = data[2]; // data[2] means payload length
        if (len == 0) {
            return ERROR_EMPTY_PAYLOAD;
        }
        return ((len + 3) == data.length) ? STATUS_SUCCESS : ERROR_INVALID_LENGTH;
    }

    public int doChangeRfParams(byte[] data, boolean lastCmd) {
        int status = checkRfConfigCommandValidate(data);
        if (STATUS_SUCCESS != status) {
            return status;
        }
        if (!isNfcEnabled()) {
            return ERROR_NFC_ON;
        }
        if (mTmsDeviceHost.isNfccBusy()) {
            return ERROR_STATUS_BUSY;
        }
        byte[] rsp = sendNciCommand(data);
        if (rsp == null || rsp.length != 5 || rsp[3] != 0) { // 5 means normal length, rsp[3] means status code
            return STATUS_FAILED;
        }
        if (lastCmd) {
            execApplyRoutingTask();
        }
        return STATUS_SUCCESS;
    }

    void sendMessage(int what, Object obj) {
        Message msg = mHandler.obtainMessage();
        msg.what = what;
        msg.obj = obj;
        mHandler.sendMessage(msg);
    }

    public boolean doSetForceSAK(boolean enabled, byte sak) {
        TmsLog.d(TAG, "doSetForceSAK enabled=" + enabled + ", sak=" + sak);
        byte[] cmd = new byte[] {0x20, 0x02, 0x05, 0x01, (byte) 0xA1, 0x1B, 0x01, 0x00};
        if (enabled) {
            byte value;
            if ((sak & 0x20) == 0x20) {
                value = 0x03;
            } else {
                value = 0x02;
            }
            cmd[cmd.length - 1] = value;
        }
        byte[] rsp = sendNciCommand(cmd, true);
        boolean result = (rsp != null && rsp.length == 5 && rsp[3] == 0); // 5 means normal length, rsp[3] means status code
        TmsLog.d(TAG, "doSetForceSAK result=" + result);
        return result;
    }

    public boolean doSetHceTypeAConfig(boolean enabled, byte[] atqa, byte[] sak, byte[] uid) {
        byte[] rsp;
        if (!enabled) {
            byte[] recoverCmd = new byte[] {0x20, 0x02, 0x04, 0x01, (byte) 0x85, 0x01, 0x01};
            rsp = sendNciCommand(recoverCmd, true);
        } else {
            if ((atqa == null || atqa.length < 2) || (sak == null || sak.length < 1)
                    || (uid == null || uid.length < 1)) {
                TmsLog.d(TAG, "doSetHceTypeAConfig params error");
                return false;
            }
            byte[] cmdWithoutUid = new byte[] {0x20, 0x02, 0x0C, 0x05, (byte) 0x85, 0x01, 0x00, 0x30, 0x01, atqa[1],
                    0x31, 0x01, atqa[0], 0x32, 0x01, sak[0], 0x33, (byte) uid.length};
            byte[] cmd = new byte[cmdWithoutUid.length + uid.length];
            System.arraycopy(cmdWithoutUid, 0, cmd, 0, cmdWithoutUid.length);
            System.arraycopy(uid, 0, cmd, cmdWithoutUid.length, uid.length);
            cmd[2] = (byte) (cmd.length - 3);
            rsp = sendNciCommand(cmd, true);
        }
        boolean result = (rsp != null && rsp.length == 5 && rsp[3] == 0); // 5 means normal length, rsp[3] means status code
        if (result) {
            mPrefsEditor = mPrefs.edit();
            mPrefsEditor.putBoolean(PREF_HCE_TYPEA_ENABLE, enabled);
            mPrefsEditor.putString(PREF_HCE_TYPEA_ATQA, TmsUtils.byteArray2Hex(atqa));
            mPrefsEditor.putString(PREF_HCE_TYPEA_SAK, TmsUtils.byteArray2Hex(sak));
            mPrefsEditor.putString(PREF_HCE_TYPEA_UID, TmsUtils.byteArray2Hex(uid));
            mPrefsEditor.commit();
        }
        TmsLog.d(TAG, "doSetHceTypeAConfig result=" + result);
        return result;
    }

    public void onLxDebugUpload(int evnet, byte[] data) {
        TmsLog.i(TAG, String.format("onLxDebugUpload event=%02X data=%s", evnet, TmsUtils.byteArray2Hex(data)));
        // Add the processing code for lx debug here
    }

    public void onNfceeAidSelect(int nfceeId, byte[] aid) {
        TmsLog.i(TAG, String.format("onNfceeAidSelect nfceeId=%02X aid=%s", nfceeId, TmsUtils.byteArray2Hex(aid)));
    }

    private boolean setRfConfigDynamic(String config) {
        try {
            Matcher matcher = TmsConstant.RF_BLOCK_PATTERN.matcher(config);

            ArrayList<byte[]> blockList = new ArrayList<>();

            while (matcher.find()) {
                String blockName = matcher.group(1);
                String blockContent = matcher.group(2);
                int[] array = Arrays.stream(blockContent.split(",")).filter(s -> !s.trim().isEmpty()).mapToInt(s -> Integer.parseInt(s.trim(), 16)).toArray();
                byte[] cmd = new byte[array.length];
                for (int i = 0; i < array.length; i++) {
                    cmd[i] = (byte) array[i];
                }

                blockList.add(cmd);

                TmsLog.i(TAG, "setConfigInternal found block: " + blockName + ", cmd length=" + cmd.length);
            }

            if (blockList.isEmpty()) {
                TmsLog.i(TAG, "setConfigInternal no validate block found");
                return false;
            }

            TmsLog.i(TAG, "setConfigInternal block count = " + blockList.size() + ", start set to nfcc");
            byte[][] blocks = new byte[blockList.size()][];
            blockList.toArray(blocks);
            boolean ret = mTmsDeviceHost.setRfConfigs(blocks, false);
            TmsLog.i(TAG, "setConfigInternal " + (ret ? "success": "failed"));
            return ret;
        } catch (RuntimeException e) {
            TmsLog.e(TAG, "setConfigInternal failed", e);
        }
        return false;
    }

    public boolean isAllowSendRfFieldActivated() {
        if (mRfFieldDeactivatedAllowed) {
            TmsLog.i(TAG, "not allow to send activated msg");
            return false;
        }
        long nowTime = System.currentTimeMillis();
        if ((nowTime - mLastRfFieldActivatedTime) < RF_FIELD_ON_MIN_INTERVAL) {
            TmsLog.i(TAG, "The time interval between the last field-on is too short, don't send activated msg");
            return false;
        }
        mLastRfFieldActivatedTime = nowTime;
        mRfFieldDeactivatedAllowed = true;
        if (mHandler.hasMessages(MSG_RF_FIELD_DEACTIVATED)) {
            TmsLog.i(TAG, "already has deactivated msg, don't send activated msg");
            mHandler.removeMessages(MSG_RF_FIELD_DEACTIVATED);
            return false;
        }
        return true;
    }

    public boolean isAllowSendRfFieldDeactivated() {
        if (!mRfFieldDeactivatedAllowed) {
            TmsLog.i(TAG, "not allow to send deactivated msg");
            return false;
        }
        mRfFieldDeactivatedAllowed = false;
        if (mHandler.hasMessages(MSG_RF_FIELD_DEACTIVATED)) {
            TmsLog.i(TAG, "already has deactivated msg, don't send deactivated msg");
            return false;
        }
        return true;
    }

    public void onRemoteFieldActivated() {
        TmsLog.i(TAG, "onRemoteFieldActivated");
        if (mIsSilentFieldDetectModeEnabled && mSilentFieldDetectTimeout > 0 && !mHandler.hasMessages(MSG_STOP_SILENT_FIELD_DETECT)) {
            TmsLog.i(TAG, "onRemoteFieldActivated and SilentFieldDetect on, start timer timeout=" + mSilentFieldDetectTimeout);
            mHandler.sendEmptyMessageDelayed(MSG_STOP_SILENT_FIELD_DETECT, mSilentFieldDetectTimeout);
        }
    }

    public void cleanupSilentFieldDetectMode() {
        TmsLog.i(TAG, "cleanupSilentFieldDetectMode");
        mIsSilentFieldDetectModeEnabled = false;
        mSilentFieldDetectTimeout = -1;
        mHandler.removeMessages(MSG_STOP_SILENT_FIELD_DETECT);
    }

    final class TmsNfcAdapterService extends ITmsNfcAdapter.Stub {
        private static final int NFCC_DIEID_LENGTH = 24;
        private static final int NFCC_DIEID_OFFSET = 8;
        private static final int NFC_MODEL_NAME_MIN_LENGTH = 2;
        private static final int NFC_MODEL_NAME_INDEX = 1;
        private static final int NFC_FW_VERSION_MIN_LENGTH = 5;
        private static final int NFC_FW_VERSION_OFFSET = 2;
        private byte[] mCachedDieId = null;

        private IM1RawDataModeCallback mM1RawDataModeCallback;

        @Override
        public void setDefaultUserRoutes(Map inputUserRoutes) {
            NfcPermissions.enforceAdminPermissions(mContext);
            Map<String, String> userRoutes = (Map<String, String>) inputUserRoutes;

            for (Map.Entry<String, String> entry : userRoutes.entrySet()) {
                String routeName = entry.getKey();
                String routeLoc = entry.getValue();

                if (DBG) {
                    TmsLog.d(TAG, "TmsNfcAdapterService - setDefaultUserRoutes() - " + routeName + ": " + routeLoc);
                }

                switch (routeName) {
                    case TmsNfcAdapter.DEFAULT_AID_ROUTE:
                        if (mPrefs.getString(PREF_DEFAULT_AID_ROUTE, TmsNfcAdapter.DEFAULT_ROUTE) != routeLoc) {
                            mPrefsEditor = mPrefs.edit();
                            mPrefsEditor.putString(PREF_DEFAULT_AID_ROUTE, routeLoc);
                            mPrefsEditor.commit();
                        }
                        break;
                    case TmsNfcAdapter.DEFAULT_MIFARE_ROUTE:
                        if (mPrefs.getString(PREF_DEFAULT_MIFARE_ROUTE, TmsNfcAdapter.DEFAULT_ROUTE) != routeLoc) {
                            mPrefsEditor = mPrefs.edit();
                            mPrefsEditor.putString(PREF_DEFAULT_MIFARE_ROUTE, routeLoc);
                            mPrefsEditor.commit();
                        }
                        break;
                    case TmsNfcAdapter.DEFAULT_ISO_DEP_ROUTE:
                        if (mPrefs.getString(PREF_DEFAULT_ISODEP_ROUTE, TmsNfcAdapter.DEFAULT_ROUTE) != routeLoc) {
                            mPrefsEditor = mPrefs.edit();
                            mPrefsEditor.putString(PREF_DEFAULT_ISODEP_ROUTE, routeLoc);
                            mPrefsEditor.commit();
                        }
                        break;
                    case TmsNfcAdapter.DEFAULT_FELICA_ROUTE:
                        if (mPrefs.getString(PREF_DEFAULT_FELICA_ROUTE, TmsNfcAdapter.DEFAULT_ROUTE) != routeLoc) {
                            mPrefsEditor = mPrefs.edit();
                            mPrefsEditor.putString(PREF_DEFAULT_FELICA_ROUTE, routeLoc);
                            mPrefsEditor.commit();
                        }
                        break;
                    case TmsNfcAdapter.DEFAULT_AB_TECH_ROUTE:
                        if (mPrefs.getString(PREF_DEFAULT_AB_TECH_ROUTE, TmsNfcAdapter.DEFAULT_ROUTE) != routeLoc) {
                            mPrefsEditor = mPrefs.edit();
                            mPrefsEditor.putString(PREF_DEFAULT_AB_TECH_ROUTE, routeLoc);
                            mPrefsEditor.commit();
                        }
                        break;
                    case TmsNfcAdapter.DEFAULT_SC_ROUTE:
                        if (mPrefs.getString(PREF_DEFAULT_SC_ROUTE, TmsNfcAdapter.DEFAULT_ROUTE) != routeLoc) {
                            mPrefsEditor = mPrefs.edit();
                            mPrefsEditor.putString(PREF_DEFAULT_SC_ROUTE, routeLoc);
                            mPrefsEditor.commit();
                        }
                        break;
                    default:
                        break;
                }
            }
            updateRoutingTable();
        }

        @Override
        public Map getDefaultUserRoutes() {
            NfcPermissions.enforceAdminPermissions(mContext);
            Map<String, String> userRoutes = new HashMap<String, String>();

            if (DBG) {
                TmsLog.d(TAG, "TmsNfcAdapterService - getDefaultUserRoutes()");
            }

            userRoutes.put(TmsNfcAdapter.DEFAULT_AID_ROUTE,
                                mPrefs.getString(PREF_DEFAULT_AID_ROUTE,
                                convertNfceeIdTypeToPrefRoute(mAidRoutingManager.doGetDefaultRouteDestination())));
            userRoutes.put(TmsNfcAdapter.DEFAULT_MIFARE_ROUTE,
                                mPrefs.getString(PREF_DEFAULT_MIFARE_ROUTE, TmsNfcAdapter.DEFAULT_ROUTE));
            userRoutes.put(TmsNfcAdapter.DEFAULT_ISO_DEP_ROUTE,
                                mPrefs.getString(PREF_DEFAULT_ISODEP_ROUTE,
                                convertNfceeIdTypeToPrefRoute(mAidRoutingManager.doGetDefaultIsoDepRouteDestination())));
            userRoutes.put(TmsNfcAdapter.DEFAULT_FELICA_ROUTE,
                                mPrefs.getString(PREF_DEFAULT_FELICA_ROUTE, TmsNfcAdapter.DEFAULT_ROUTE));
            userRoutes.put(TmsNfcAdapter.DEFAULT_AB_TECH_ROUTE,
                                mPrefs.getString(PREF_DEFAULT_AB_TECH_ROUTE,
                                convertNfceeIdTypeToPrefRoute(mAidRoutingManager.doGetDefaultOffHostRouteDestination())));
            userRoutes.put(TmsNfcAdapter.DEFAULT_SC_ROUTE,
                                mPrefs.getString(PREF_DEFAULT_SC_ROUTE, TmsNfcAdapter.DEFAULT_ROUTE));

            return userRoutes;
        }

        @Override
        public void setAllDefaultUserRoutes(String route) {
            NfcPermissions.enforceAdminPermissions(mContext);
            if (DBG) {
                TmsLog.d(TAG, "TmsNfcAdapterService - setAllDefaultUserRoutes()");
            }
            setAllRouteToPrefs(route);
            updateRoutingTable();
        }

        @Override
        public void setSkipTagSelect(int protocol) {
            NfcPermissions.enforceAdminPermissions(mContext);
            mTmsDeviceHost.skipTagSelect(protocol);
        }

        @Override
        public void setRfListenMask(int listenMask) {
            NfcPermissions.enforceAdminPermissions(mContext);
            TmsLog.d(TAG, "setRfListenMask() called with: listenMask = [" + listenMask + "]");
            mTmsDeviceHost.setRfListenMask(listenMask, true);
            mPrefsEditor = mPrefs.edit();
            mPrefsEditor.putInt(TmsConstant.PREFS_RF_LISTEN_MASK, listenMask).commit();
        }

        public int getRfListenMask() {
            TmsLog.d(TAG, "getRfListenMask() called");
            return mTmsDeviceHost.getRfListenMask();
        }

        public void setRfPollMask(int pollMask) {
            NfcPermissions.enforceAdminPermissions(mContext);
            TmsLog.d(TAG, "setRfPollMask() called with: pollMask = [" + pollMask + "]");
            int oldMask = mTmsPollMask;
            if (pollMask == -1) {
                mTmsPollMask = -1;
            } else {
                int techMask = 0;
                if ((pollMask & TmsNfcAdapter.RF_POLL_MASK_A) == TmsNfcAdapter.RF_POLL_MASK_A) {
                    techMask |= NFC_POLL_A;
                }
                if ((pollMask & TmsNfcAdapter.RF_POLL_MASK_B) == TmsNfcAdapter.RF_POLL_MASK_B) {
                    techMask |= NFC_POLL_B;
                }
                if ((pollMask & TmsNfcAdapter.RF_POLL_MASK_F) == TmsNfcAdapter.RF_POLL_MASK_F) {
                    techMask |= NFC_POLL_F;
                }
                if ((pollMask & TmsNfcAdapter.RF_POLL_MASK_V) == TmsNfcAdapter.RF_POLL_MASK_V) {
                    techMask |= NFC_POLL_V;
                }
                mTmsPollMask = techMask;
            }
            if (mTmsPollMask != oldMask) {
                execApplyRoutingTask();
                mPrefsEditor = mPrefs.edit();
                mPrefsEditor.putInt(TmsConstant.PREFS_RF_POLL_MASK, mTmsPollMask).commit();
            }
        }

        public int getRfPollMask() {
            NfcPermissions.enforceAdminPermissions(mContext);
            TmsLog.d(TAG, "getRfPollMask() called, mTmsPollMask = " + mTmsPollMask);
            return mTmsPollMask;
        }

        public byte[] sendNciCommand(byte[] cmd) {
            NfcPermissions.enforceAdminPermissions(mContext);
            return TmsNfcService.this.sendNciCommand(cmd);
        }

        public String getMwVersion() {
            NfcPermissions.enforceAdminPermissions(mContext);
            TmsLog.dumpCaller(mContext.getPackageManager());
            StringBuilder builder = new StringBuilder();
            builder.append(mTmsDeviceHost.getMwVersion());
            if (DBG) {
                builder.append("(");
                builder.append(mTmsDeviceHost.getMwBuildTime());
                builder.append(")");
            }
            String mwVersion = builder.toString();
            TmsLog.i(TAG, "getMwVersion: " + mwVersion);
            return mwVersion;
        }

        private void WaitForAdapterChange(int state) {
            while (true) {
                synchronized(mNfcService) {
                    if(getNfcState() == state) {
                        break;
                    }
                }
                try {
                    Thread.sleep(100);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
            return;
        }

        @Override
        public int setConfig(String configs) {
            TmsLog.e(TAG, "Setting configs for Transit" );
            /*Check permissions*/
            NfcPermissions.enforceAdminPermissions(mContext);
            /*Check if any NFC transactions are ongoing*/
            if(mTmsDeviceHost.isNfccBusy())
            {
                TmsLog.e(TAG, "NFCC is busy.." );
                return TRANSIT_SETCONFIG_STAT_FAILED;
            }
            /*check if format of configs is fine*/
            /*Save configurations to file*/
            FileWriter fw = null;
            try {
                File newTextFile = new File("/data/nfc/libnfc-tmsTransit.conf");
                if(configs == null)
                {
                    if(newTextFile.delete()){
                        TmsLog.e(TAG, "Removing transit config file. Taking default Value" );
                    }else{
                        System.out.println("Error taking defualt value");
                    }
                }
                else
                {
                    fw = new FileWriter(newTextFile);
                    fw.write(configs);
                    TmsLog.e(TAG, "File Written to libnfc-tmsTransit.conf successfully" );
                }
                newTextFile = null;
                mTmsDeviceHost.setTransitConfig(configs);
            } catch (Exception e) {
                e.printStackTrace();
                return TRANSIT_SETCONFIG_STAT_FAILED;
            } finally {
                if (fw != null) {
                    try {
                    fw.close();
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
            }

            if (setRfConfigDynamic(configs)) {
                return TRANSIT_SETCONFIG_STAT_SUCCESS;
            }

            /*restart NFC service*/
            try {
                mNfcAdapter.disable(true);
                WaitForAdapterChange(NfcAdapter.STATE_OFF);
                mNfcAdapter.enable();
                WaitForAdapterChange(NfcAdapter.STATE_ON);
            } catch (Exception e) {
                TmsLog.e(TAG, "Unable to restart NFC Service");
                e.printStackTrace();
                return TRANSIT_SETCONFIG_STAT_FAILED;
            }
            return TRANSIT_SETCONFIG_STAT_SUCCESS;
        }

        public IBinder getHciAdapterService() {
            NfcPermissions.enforceAdminPermissions(mContext);
            TmsLog.dumpCaller(mContext.getPackageManager());
            return mTmsHciAdapterService;
        }

        public byte[] getNfccSerialNumber() {
            NfcPermissions.enforceAdminPermissions(mContext);
            if (!isNfcEnabled()) {
                TmsLog.w(TAG, "getNfccSerialNumber but NFC is not enable");
                return null;
            }
            if (mCachedDieId != null) {
                TmsLog.i(TAG, "getNfccSerialNumber: " + TmsUtils.byteArray2Hex(mCachedDieId) + " cached");
                return mCachedDieId;
            }
            byte[] getNfccSerialNumberCommand = new byte[] {0x20, 0x03, 0x03, 0x01, (byte) 0xA0, 0x01};
            byte[] rsp = sendNciCommand(getNfccSerialNumberCommand);
            if (rsp != null && rsp.length == NFCC_DIEID_LENGTH) {
                byte[] dieId = Arrays.copyOfRange(rsp, NFCC_DIEID_OFFSET, rsp.length);
                TmsLog.i(TAG, "getNfccSerialNumber: " + TmsUtils.byteArray2Hex(dieId));
                mCachedDieId = dieId;
                return dieId;
            }
            return null;
        }

        public String getNfcModelName() {
            NfcPermissions.enforceAdminPermissions(mContext);
            if (!isNfcEnabled()) {
                TmsLog.w(TAG, "getNfcModelName but NFC is not enable");
                return null;
            }
            byte[] infos = mTmsDeviceHost.getManufSpecInfo();
            if (infos == null || infos.length < NFC_MODEL_NAME_MIN_LENGTH) {
                TmsLog.w(TAG, "invalidate manufacturer specific informatcion: " + (infos == null ? -1 : infos.length));
                return null;
            }
            String modelName = String.format(Locale.ROOT, "TMS-THN31-%02X", infos[NFC_MODEL_NAME_INDEX]);
            TmsLog.i(TAG, "getNfcModelName: " + modelName);
            return modelName;
        }

        public String getNfcFwVersion() {
            NfcPermissions.enforceAdminPermissions(mContext);
            if (!isNfcEnabled()) {
                TmsLog.w(TAG, "getNfcFwVersion but NFC is not enable");
                return null;
            }
            byte[] infos = mTmsDeviceHost.getManufSpecInfo();
            if (infos == null || infos.length < NFC_FW_VERSION_MIN_LENGTH) {
                TmsLog.w(TAG, "invalidate manufacturer specific informatcion: " + (infos == null ? -1 : infos.length));
                return null;
            }
            int index = NFC_FW_VERSION_OFFSET;
            String fwVersion = String.format("%02X%02X%02X", infos[index++], infos[index++], infos[index++]);
            TmsLog.i(TAG, "getNfcFwVersion: " + fwVersion);
            return fwVersion;
        }

        public boolean setM1RawDataModeEnable(boolean isEnable, IM1RawDataModeCallback callback) {
            TmsLog.i(TAG, "setM1RawDataModeEnable: " + isEnable);
            NfcPermissions.enforceAdminPermissions(mContext);
            mM1RawDataModeCallback = callback;
            if (isEnable && mM1RawDataModeCallback != null) {
                try {
                    mM1RawDataModeCallback.asBinder().linkToDeath(mM1RawDataModeDeathRecipient, 0);
                } catch (RemoteException e) {
                    TmsLog.e(TAG, "linkToDeath failed", e);
                }
            }
            return TmsNfcService.this.setM1RawDataModeEnable(isEnable);
        }

        private final IBinder.DeathRecipient mM1RawDataModeDeathRecipient = () -> {
            TmsLog.i(TAG, "M1RawDataModeCallback client dead, disable M1RawDataMode");
            setM1RawDataModeEnable(false, null);
        };

        public boolean isM1RawDataModeEnabled() {
            NfcPermissions.enforceAdminPermissions(mContext);
            return TmsNfcService.this.isM1RawDataOn();
        }

        public int getM1RawDataModeState() {
            NfcPermissions.enforceAdminPermissions(mContext);
            TmsLog.i(TAG, "getM1RawDataModeState = " + mM1RawDataModeState);
            return mM1RawDataModeState;
        }

        public boolean setM1RawDataModeTimeInterval(int timeInterval) {
            NfcPermissions.enforceAdminPermissions(mContext);
            return TmsNfcService.this.setM1RawDataModeTimeInterval(timeInterval);
        }

        private void onM1RawDataModeAuthResult(int authStatus) {
            if (mM1RawDataModeCallback != null) {
                TmsLog.d(TAG, "onM1RawDataModeAuthResult: " + authStatus);
                try {
                    mM1RawDataModeCallback.onM1RawDataModeAuthResult(authStatus);
                } catch (RemoteException e) {
                    TmsLog.e(TAG, "onM1RawDataModeAuthResult failed", e);
                }
            }
        }

        public int changeRfParams(byte[] data, boolean lastCmd) {
            NfcPermissions.enforceUserPermissions(mContext);
            return doChangeRfParams(data, lastCmd);
        }

        public int[] getActiveSecureElementList() {
            NfcPermissions.enforceUserPermissions(mContext);
            int[] list = null;
            if (getNfcState() != NfcAdapter.STATE_ON) {
                TmsLog.i(TAG, "Nfc is not enabled.");
                return list;
            }
            list = mTmsDeviceHost.doGetActiveSecureElementList();
            return list;
        }

        public int doWriteT4tData(byte[] fileId, byte[] data, int length) {
            NfcPermissions.enforceUserPermissions(mContext);
            if (!checkT4TNfceeFeature()) {
                TmsLog.i(TAG, "T4t Nfcee feature is not enable.");
                return T4TNFCEE_STATUS_FAILED;
            }
            Bundle writeBundle = new Bundle();
            writeBundle.putByteArray("fileId", fileId);
            writeBundle.putByteArray("writeData", data);
            writeBundle.putInt("length", length);
            try {
                sendMessage(MSG_WRITE_T4TNFCEE, writeBundle);
                synchronized (mT4tNfcEeObj) {
                    mT4tNfcEeObj.wait(1000);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
            /*return T4TNFCEE_STATUS_FAILED(-1) if readData not found.
            This can happen in case of mT4tNfcEeObj timeout*/
            int status = mT4tNfceeReturnBundle.getInt("writeStatus", T4TNFCEE_STATUS_FAILED);
            mT4tNfceeReturnBundle.clear();
            return status;
        }

        public byte[] doReadT4tData(byte[] fileId) {
            NfcPermissions.enforceUserPermissions(mContext);
            if (!checkT4TNfceeFeature()) {
                TmsLog.i(TAG, "T4t Nfcee feature is not enable.");
                return null;
            }
            Bundle readBundle = new Bundle();
            readBundle.putByteArray("fileId", fileId);
            try {
                sendMessage(MSG_READ_T4TNFCEE, readBundle);
                synchronized (mT4tNfcEeObj) {
                    mT4tNfcEeObj.wait(1000);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
            /*getByteArray returns null if readData not found.
            This can happen in case of mT4tNfcEeObj timeout*/
            byte[] readData = mT4tNfceeReturnBundle.getByteArray("readData");
            mT4tNfceeReturnBundle.clear();
            return readData;
        }

        public boolean enableT4tNfceeRoute(boolean enable) {
            NfcPermissions.enforceUserPermissions(mContext);
            if (!checkT4TNfceeFeature()) {
                TmsLog.i(TAG, "T4t Nfcee feature is not enable.");
                return false;
            }
            if (getNfcState() != NfcAdapter.STATE_ON) {
                TmsLog.i(TAG, "Nfc is not enabled.");
                return false;
            }
            mPrefsEditor = mPrefs.edit();
            mPrefsEditor.putBoolean(PREF_T4T_NFCEE_ENABLE, enable);
            mPrefsEditor.commit();
            addT4TNfceeAid();
            updateRoutingTable();
            return true;
        }

        public byte[] sendT4tRawApdu(byte[] apdu, int apduLen) {
            NfcPermissions.enforceUserPermissions(mContext);
            if (!checkT4TNfceeFeature()) {
                TmsLog.i(TAG, "T4t Nfcee feature is not enable.");
                return null;
            }
            byte[] response = sendT4tRawApduInternal(apdu, apduLen);
            return response;
        }

        public boolean enableT4tContactlessWrite(boolean enable) {
            NfcPermissions.enforceUserPermissions(mContext);
            if (!checkT4TNfceeFeature()) {
                TmsLog.i(TAG, "T4t Nfcee feature is not enable.");
                return false;
            }
            boolean sendStatus = false;
            byte[] apdu = new byte[] { (byte)0x00, PROP_APDU_INS, (byte)0xE1, (byte)0x04, (byte)0x02, (byte)0x00, (byte)0x80};
            if (enable) {
                apdu[6] = (byte)0x00;
            }
            byte[] response = sendT4tRawApduInternal(apdu, apdu.length);
            int len = 0;
            if (response != null) {
                len = response.length;
                if (len >= 2 && response[len - 2] == (byte)0x90
                    && response[len - 1] == (byte)0x00) {
                    sendStatus = true;
                }
            }
            return sendStatus;
        }

        public boolean setForceSAK(boolean isEnable, byte sak) {
            NfcPermissions.enforceAdminPermissions(mContext);
            return doSetForceSAK(isEnable, sak);
        }

        public boolean startSilentFieldDetectMode(int timeout) {
            NfcPermissions.enforceAdminPermissions(mContext);

            TmsLog.i(TAG, "startSilentFieldDetectMode timeout = " + timeout);

            if (mIsSilentFieldDetectModeEnabled) {
                TmsLog.i(TAG, "startSilentFieldDetectMode, is alreay enabled");
                return true;
            }

            byte[] cmd = new byte[] {0x20, 0x02, 0x05, 0x01, (byte) 0xA2, 0x75, 0x01, 0x01};
            byte[] rsp = TmsNfcService.this.sendNciCommand(cmd, true);
            boolean result = (rsp != null && rsp.length == 5 && rsp[3] == 0); // 5 means normal length, rsp[3] means status code
            TmsLog.d(TAG, "startSilentFieldDetectMode result=" + result);

            if (!result) {
                return false;
            }

            mIsSilentFieldDetectModeEnabled = true;
            mSilentFieldDetectTimeout = timeout;
            return true;
        }

        public boolean stopSilentFieldDetectMode() {
            NfcPermissions.enforceAdminPermissions(mContext);
            if (!mIsSilentFieldDetectModeEnabled) {
                TmsLog.i(TAG, "stopSilentFieldDetectMode, is alreay disabled");
                return true;
            }
            byte[] cmd = new byte[] {0x20, 0x02, 0x05, 0x01, (byte) 0xA2, 0x75, 0x01, 0x00};
            byte[] rsp = TmsNfcService.this.sendNciCommand(cmd, true);
            boolean result = (rsp != null && rsp.length == 5 && rsp[3] == 0); // 5 means normal length, rsp[3] means status code
            TmsLog.d(TAG, "stopSilentFieldDetectMode result=" + result);
            cleanupSilentFieldDetectMode();
            return result;
        }

        public boolean isSilentFieldDetectEnabled() {
            NfcPermissions.enforceAdminPermissions(mContext);
            TmsLog.d(TAG, "isSilentFieldDetectEnabled = " + mIsSilentFieldDetectModeEnabled);
            return mIsSilentFieldDetectModeEnabled;
        }

        public int setFeatureState(String featureName, int state, boolean force, Bundle extras) {
            NfcPermissions.enforceAdminPermissions(mContext);
            TmsLog.i(TAG, "setFeatureState featureName= " + featureName + ", state=" + state + ", force = " + force + ", extras = " + extras);
            int oldState = mFeatureStateMap.getOrDefault(featureName, TmsNfcAdapter.FEATURE_STATE_UNKNOWN);
            if (oldState == state) {
                TmsLog.i(TAG, "Feature(" + featureName + ") state is already = " + state);
                if (force) {
                    TmsLog.i(TAG, "force set");
                } else {
                    TmsLog.i(TAG, "Do not repeat set");
                    return state;
                }
            }
            boolean setResult = false;
            if (TmsNfcAdapter.FEATURE_NAME_MUTE_RATS.equals(featureName)) {
                setResult = setMuteRatsState(state);
            }
            if (setResult) {
                mFeatureStateMap.put(featureName, state);
                TmsLog.i(TAG, "Feature(" + featureName + ") state: " + oldState + " -> " + state);
                return state;
            }
            TmsLog.e(TAG, "Feature(" + featureName + ") state: " + state + ", set failed");
            return TmsNfcAdapter.FEATURE_STATE_UNKNOWN;
        }

        public int getFeatureState(String featureName) {
            NfcPermissions.enforceAdminPermissions(mContext);
            int featureState = mFeatureStateMap.getOrDefault(featureName, TmsNfcAdapter.FEATURE_STATE_UNKNOWN);
            TmsLog.i(TAG, "getFeatureState featureName= " + featureName + ", state = " + featureState);
            return featureState;
        }

        public boolean setHceTypeAConfig(boolean enabled, byte[]atqa, byte[]sak, byte[]uid) {
            NfcPermissions.enforceAdminPermissions(mContext);
            return doSetHceTypeAConfig(enabled, atqa, sak, uid);
        }
    }

    private boolean setMuteRatsState(int status) {
        boolean isEnable = (status == TmsNfcAdapter.FEATURE_STATE_ENABLE);
        byte[] cmd = new byte[] {
            0x20, 0x02, 0x08, 0x01, (byte) 0xA0, (byte) 0x85, 0x04, 0x40, 0x02, 0x00, 0x00
        };
        if (!isEnable) {
            cmd[8] = 0x04;
        }
        byte[] rsp = sendNciCommand(cmd, true); // normal conditions: 4002020000
        return rsp != null && rsp.length == 5 && rsp[3] == 0; // 5 means normal length, rsp[3] means status code
    }

    private void cleanupFeatureState() {
        mFeatureStateMap.put(TmsNfcAdapter.FEATURE_NAME_MUTE_RATS, FEATURE_DEFAULT_STATE_MUTE_RATS);
        TmsLog.i(TAG, "cleanupFeatureState, mFeatureStateMap = " + mFeatureStateMap);
    }

    final class TmsHciAdapterService extends IHciAdapter.Stub {
        private IHciCallback mCallback;

        public void open(IHciCallback callback) {
            NfcPermissions.enforceAdminPermissions(mContext);
            TmsLog.i(TAG, "open called");
            mTmsDeviceHost.setPassthroughMode(1);
            this.mCallback = callback;
        }
        public void close() {
            NfcPermissions.enforceAdminPermissions(mContext);
            TmsLog.i(TAG, "close called");
            mTmsDeviceHost.setPassthroughMode(0);
            this.mCallback = null;
        }
        public void transceive(byte[] data) {
            NfcPermissions.enforceAdminPermissions(mContext);
            TmsLog.i(TAG, "transceive called, size = " + data.length);
            mTmsDeviceHost.sendRawPtmCommand(data);
        }

        void onNciResponseReceive(int event, byte[] rsp) {
            if (DBG) {
                TmsLog.d(TAG, "onNciResponseReceive() called with: event = [" + event + "], rsp size = [" + rsp.length + "]");
            }
            if (mCallback != null) {
                try {
                    mCallback.onHciDataReceive(rsp);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
    }

    private boolean isNfcEnabled() {
        Optional<Boolean> opt = callNfcServiceMethod("isNfcEnabled", new Class<?>[0], new Object[0], Boolean.class);
        if (opt.isPresent()) {
            return opt.get();
        }
        throw new IllegalArgumentException("call NfcService isNfcEnabled failed");
    }

    private int getNfcState() {
        return getNfcServiceFiledNotNull("mState", Integer.class);
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

    private void execApplyRoutingTask() {
        try {
            Class<?> clazz = Class.forName(mNfcService.getClass().getName() + "$ApplyRoutingTask");
            Constructor<?> constructor = clazz.getDeclaredConstructor(mNfcService.getClass());
            constructor.setAccessible(true);
            Object task = constructor.newInstance(mNfcService);
            ((AsyncTask<Integer, Void, Void>) task).execute();
        } catch (ReflectiveOperationException e) {
            TmsLog.e(TAG, "execApplyRoutingTask failed", e);
        }
    }
}
