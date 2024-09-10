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

import android.annotation.RequiresPermission;
import android.content.Context;
import android.nfc.INfcAdapter;
import android.nfc.NfcAdapter;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

public class TmsNfcAdapter {
    static INfcAdapter sService;
    static ITmsNfcAdapter sVendorService;
    static boolean sIsInitialized = false;

    private static final String TAG = "TmsNfcAdapter";

    public static final String SERVICE_NAME = "nfc_tms";

    /**
     * The NfcAdapter object for each application context. There is a 1-1 relationship between
     * application context and NfcAdapter object.
     */
    static HashMap<Context, TmsNfcAdapter> sTmsNfcAdapters = new HashMap();

    final Context mContext;

    /**
     * The mode flag to control each NFC mode. MODE_READER is used to switch Tag read/write mode
     *
     * @hide
     * @internal
     */
    public static final int MODE_READER = 1;

    /**
     * The mode flag to control each NFC mode. MODE_HCE is used to switch card emulation mode
     *
     * @hide
     * @internal
     */
    public static final int MODE_HCE = 2;

    /**
     * The mode flag to control each NFC mode. MODE_P2P is used to switch P2P mode
     *
     * @hide
     * @internal
     */
    public static final int MODE_P2P = 4;

    /**
     * To disable each NFC mode.
     *
     * @hide
     * @internal
     */
    public static final int FLAG_OFF = 0;

    /**
     * To enable each NFC mode.
     *
     * @hide
     * @internal
     */
    public static final int FLAG_ON = 1;

    /* Below values must be aligned with SecureElementSelector code */
    public static final String SE_SIM1 = "SIM1";
    public static final String SE_SIM2 = "SIM2";
    public static final String SE_ESE1 = "eSE";

    public static final String SE_STATE_ACTIVATED = "Active";
    public static final String SE_STATE_AVAILABLE = "Available";
    public static final String SE_STATE_NOT_AVAILABLE = "N/A";

    public TmsNfcAdapter(Context context) {
        mContext = context;
        sService = getServiceInterface();
    }

    /**
     * Helper to get the default TMS NFC Adapter.
     *
     * @param context the calling application's context
     * @return the default TMS NFC adapter, or null if no TMS NFC adapter exists
     */
    public static synchronized TmsNfcAdapter getDefaultAdapter(Context context) {
        if (NfcAdapter.getDefaultAdapter(context) == null) {
            Log.d(TAG, "getDefaultAdapter = null");
            return null;
        }

        TmsNfcAdapter adapter = sTmsNfcAdapters.get(context);
        if (adapter == null) {
            adapter = new TmsNfcAdapter(context);
            sTmsNfcAdapters.put(context, adapter);
        }

        if (!sIsInitialized) {
            if (sService == null) {
                sService = getServiceInterface();
                Log.d(TAG, "sService = " + sService);
            }

            if (sService == null) {
                Log.e(TAG, "could not retrieve NFC service");
                throw new UnsupportedOperationException();
            }

            sVendorService = getServiceVendorInterface();
            if (sVendorService == null) {
                Log.e(TAG, "could not retrieve TMS NFC service");
                throw new UnsupportedOperationException();
            }

            sIsInitialized = true;
        }

        Log.d(TAG, "adapter = " + adapter);
        return adapter;
    }

    /** get handle to NFC service interface */
    private static INfcAdapter getServiceInterface() {
        /* get a handle to NFC service */
        IBinder b = ServiceManager.getService("nfc");
        if (b == null) {
            return null;
        }
        return INfcAdapter.Stub.asInterface(b);
    }

    private static ITmsNfcAdapter getServiceVendorInterface() {
        if (sService == null) {
            throw new UnsupportedOperationException(
                    "You need a reference from NfcAdapter to use the TMS NFC APIs");
        }
        try {
            IBinder b = sService.getNfcAdapterVendorInterface(SERVICE_NAME);
            if (b == null) {
                return null;
            }
            return ITmsNfcAdapter.Stub.asInterface(b);
        } catch (RemoteException e) {
            return null;
        }
    }

    /**
     * NFC service dead - attempt best effort recovery
     * @hide
     */
    private static void attemptDeadServiceRecovery(Exception e) {
        Log.e(TAG, "Service dead - attempting to recover",e);
        INfcAdapter service = getServiceInterface();
        if (service == null) {
            Log.e(TAG, "could not retrieve NFC service during service recovery");
            // nothing more can be done now, sService is still stale, we'll hit
            // this recovery path again later
            return;
        }
        // assigning to sService is not thread-safe, but this is best-effort code
        // and on a well-behaved system should never happen
        sService = service;
        sVendorService = getServiceVendorInterface();
        return;
    }

    /**
     * Check whether the NFC service binder is alive.
     * If the NFC service binder is not alive, try recovery it
     */
    private static void checkNfcServiceAlive() {
        if(sVendorService == null || !sVendorService.asBinder().pingBinder()) {
            Log.e(TAG, "NfcService Died, now try to recovery");
            attemptDeadServiceRecovery(null);
        }
    }


    public static final String DEFAULT_AID_ROUTE = "default_aid_route";
    public static final String DEFAULT_MIFARE_ROUTE = "default_mifare_route";
    public static final String DEFAULT_ISO_DEP_ROUTE = "default_iso_dep_route";
    public static final String DEFAULT_FELICA_ROUTE = "default_felica_route";
    public static final String DEFAULT_AB_TECH_ROUTE = "default_ab_tech_route";
    public static final String DEFAULT_SC_ROUTE = "default_sc_route";

    public static final String UICC_ROUTE = "UICC";
    public static final String UICC2_ROUTE = "UICC2";
    public static final String ESE_ROUTE = "eSE";
    public static final String HCE_ROUTE = "HCE";
    public static final String DEFAULT_ROUTE = "Default";

    public static final int RF_PROTOCOL_NONE = -1;
    public static final int RF_PROTOCOL_ISO_DEP = 0x04;
    public static final int RF_PROTOCOL_NFC_DEP = 0x05;
    public static final int RF_PROTOCOL_MIFARE = 0x80;

    public static final int RF_LISTEN_MASK_A = 0x01;
    public static final int RF_LISTEN_MASK_B = 0x02;
    public static final int RF_LISTEN_MASK_F = 0x04;
    public static final int RF_LISTEN_NOT_SPECIFIED = 0xF0;

    public static final int RF_POLL_MASK_A = 0x01;
    public static final int RF_POLL_MASK_B = 0x02;
    public static final int RF_POLL_MASK_F = 0x04;
    public static final int RF_POLL_MASK_V = 0x08;
    public static final int RF_POLL_NOT_SPECIFIED = 0xF0;

    public static final int M1_RAW_DATA_MODE_STATE_OFF = 0;
    public static final int M1_RAW_DATA_MODE_STATE_TURNING_ON = 1;
    public static final int M1_RAW_DATA_MODE_STATE_ON = 2;

    public static final String FEATURE_NAME_MUTE_RATS = "muteRats";

    public static final int FEATURE_STATE_UNKNOWN = -1;
    public static final int FEATURE_STATE_DISABLE = 0;
    public static final int FEATURE_STATE_ENABLE = 1;

    /**
     * Set listen mode routing table configuration for Default Route. routeLoc is parameter which
     * fetch the text from UI and compare *
     *
     * <p>Requires {@link android.Manifest.permission#NFC} permission.
     *
     * @throws IOException If a failure occurred during Default Route Route set.
     */
    public void setUserDefaultRoutes(Map<String, String> routeList) throws IOException {
        checkNfcServiceAlive();

        for (Map.Entry<String, String> entry : routeList.entrySet()) {
            String routeKey = entry.getKey();
            String routeValue = entry.getValue();

            Log.d(TAG, "setUserDefaultRoutes() - " + routeKey + ": " + routeValue);

            if ((DEFAULT_AID_ROUTE.contentEquals(routeKey) == false)
                    && (DEFAULT_MIFARE_ROUTE.contentEquals(routeKey) == false)
                    && (DEFAULT_ISO_DEP_ROUTE.contentEquals(routeKey) == false)
                    && (DEFAULT_FELICA_ROUTE.contentEquals(routeKey) == false)
                    && (DEFAULT_AB_TECH_ROUTE.contentEquals(routeKey) == false)
                    && (DEFAULT_SC_ROUTE.contentEquals(routeKey) == false)) {

                Log.e(TAG, "setUserDefaultRoutes() - " + routeKey + " does not exists");
                throw new IOException(routeKey + " does not exists");
            }

            if ((UICC_ROUTE.contentEquals(routeValue) == false)
                    && (UICC2_ROUTE.contentEquals(routeValue) == false)
                    && (ESE_ROUTE.contentEquals(routeValue) == false)
                    && (HCE_ROUTE.contentEquals(routeValue) == false)
                    && (DEFAULT_ROUTE.contentEquals(routeValue)) == false) {

                Log.e(TAG, "setUserDefaultRoutes() - " + routeValue + " does not exists");
                throw new IOException(routeValue + " does not exists");
            }
        }

        try {
            sVendorService.setDefaultUserRoutes(routeList);
        } catch (RemoteException e) {
            Log.e(TAG, "setDefaultUserRoutes failed", e);
            attemptDeadServiceRecovery(e);
            throw new IOException("setDefaultUserRoutes failed");
        }
    }

    /**
     * Set listen mode routing table configuration for Default Route. routeLoc is parameter which
     * fetch the text from UI and compare *
     *
     * <p>Requires {@link android.Manifest.permission#NFC} permission.
     *
     * @throws IOException If a failure occurred during Default Route Route set.
     */
    public Map<String, String> getUserDefaultRoutes() throws IOException {
        checkNfcServiceAlive();
        try {
            return sVendorService.getDefaultUserRoutes();
        } catch (RemoteException e) {
            attemptDeadServiceRecovery(e);
            Log.e(TAG, "getUserDefaultRoutes failed", e);
            throw new IOException("getUserDefaultRoutes failed");
        }
    }

    /**
     * Set listen mode routing table configuration for Default Route. routeLoc is parameter which
     * fetch the text from UI and compare *
     *
     * <p>Requires {@link android.Manifest.permission#NFC} permission.
     *
     * @param route the default route to be set.
     * @see #UICC_ROUTE
     * @see #UICC2_ROUTE
     * @see #ESE_ROUTE
     * @see #HCE_ROUTE
     * @throws IOException If a failure occurred during Default Route set.
     */
    public void setAllUserDefaultRoutes(String route) throws IOException {
        checkNfcServiceAlive();

        Log.d(TAG, "setAllUserDefaultRoutes() - " + route);

        if (!UICC_ROUTE.equals(route) && !UICC2_ROUTE.equals(route) &&
            !ESE_ROUTE.equals(route) && !HCE_ROUTE.equals(route)) {
            Log.e(TAG, "setAllUserDefaultRoutes() - invaild parameter");
            return;
        }

        try {
            sVendorService.setAllDefaultUserRoutes(route);
        } catch (RemoteException e) {
            Log.e(TAG, "setAllUserDefaultRoutes failed", e);
            attemptDeadServiceRecovery(e);
            throw new IOException("setAllUserDefaultRoutes failed");
        }
    }

    /**
     * This interface is used to set cards for the specified protocol
     *    to be ignored when multi-protocol cards are detected
     *
     * @see #RF_PROTOCOL_ISO_DEP
     * @see #RF_PROTOCOL_NFC_DEP
     * @see #RF_PROTOCOL_MIFARE
     * @see #RF_PROTOCOL_NONE
     * @param protocol The protocol needs to be ignored
     */
    public void setSkipTagSelect(int protocol) {
        checkNfcServiceAlive();
        try {
            sVendorService.setSkipTagSelect(protocol);
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
    }

    /**
     * set rf listen enable
     *
     * @param listenMask listen mask
     * @see #RF_LISTEN_MASK_A
     * @see #RF_LISTEN_MASK_B
     * @see #RF_LISTEN_MASK_F
     * @see #RF_LISTEN_NOT_SPECIFIED
     */
    public void setRfListenMask(int listenMask) {
        checkNfcServiceAlive();
        try {
            sVendorService.setRfListenMask(listenMask);
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
    }

    /**
     * get current rf listen mask
     *
     * @return current rf listen mask
     */
    public int getRfListenMask() {
        checkNfcServiceAlive();
        try {
            return sVendorService.getRfListenMask();
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
        return -1;
    }

    /**
     * set rf poll enable
     *
     * @param pollMask poll mask
     * @see #RF_POLL_MASK_A
     * @see #RF_POLL_MASK_B
     * @see #RF_POLL_MASK_F
     * @see #RF_POLL_MASK_V
     * @see #RF_POLL_NOT_SPECIFIED
     */
    public void setRfPollMask(int pollMask) {
        checkNfcServiceAlive();
        try {
            sVendorService.setRfPollMask(pollMask);
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
    }

    /**
     * get current rf poll mask
     *
     * @return current rf poll mask
     */
    public int getRfPollMask() {
        checkNfcServiceAlive();
        try {
            return sVendorService.getRfPollMask();
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
        return -1;
    }

    /**
     * Send Raw Nci Command to NFCC
     * note that: the response of the command is returned by the return value of the function, but since
     * the notification message generated by this command will not be sent to the caller, and the notification
     * is accepted and processed by NFC MW
     *
     * <p class="note">Requires the {@link android.Manifest.permission.WRITE_SECURE_SETTINGS} permission.
     *
     * @param cmd the command to be sent
     * @return the response of this command, or null when sending failed
     *
     */
    @RequiresPermission(android.Manifest.permission.WRITE_SECURE_SETTINGS)
    public byte[] sendNciCommand(byte[] cmd) {
        checkNfcServiceAlive();
        try {
            return sVendorService.sendNciCommand(cmd);
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
        return null;
    }

    /**
     * This api is called by applications to update the NFC configurations which
     * are already part of libnfc-tms.conf and libnfc-brcm.conf <p>Requires
     * {@link android.Manifest.permission#NFC} permission.<ul> <li>This api
     * shall be called only Nfcservice is enabled. <li>This api shall be called
     * only when there are no NFC transactions ongoing
     * </ul>
     * @param  configs NFC Configuration to be updated.
     * @param  pkg package name of the caller
     * @return whether  the update of configuration is
     *          success or not.
     *          0xFF - failure
     *          0x00 - success
     * @throws  IOException if any exception occurs during setting the NFC
     * configuration.
     */
    public int setConfig(String configs) throws IOException {
      try {
        return sVendorService.setConfig(configs);
      } catch (RemoteException e) {
        e.printStackTrace();
        attemptDeadServiceRecovery(e);
        return 0xFF;
      }
    }

    /*
     * Get the NFC MW version number
     *
     * @return NFC MW version number text
     */
    public String getMwVersion() {
        checkNfcServiceAlive();
        try {
            return sVendorService.getMwVersion();
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
        return null;
    }

    public IBinder getHciAdapterService() {
        checkNfcServiceAlive();
        try {
            return sVendorService.getHciAdapterService();
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
        return null;
    }

    public byte[] getNfccSerialNumber() {
        checkNfcServiceAlive();
        try {
            return sVendorService.getNfccSerialNumber();
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
        return null;
    }

    public String getNfcModelName() {
        checkNfcServiceAlive();
        try {
            return sVendorService.getNfcModelName();
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
        return null;
    }

    public String getNfcFwVersion() {
        checkNfcServiceAlive();
        try {
            return sVendorService.getNfcFwVersion();
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
        return null;
    }

    /**
     * Turn on M1 RawData mode, when the opening succeeds or fails,
     * the callback interface will be called back to inform the result
     *
     * @param callback Callback for open result notification
     * @return Whether the operation is successful (it does not mean whether the opening is successful)
     */
    public boolean enableM1RawDataMode(M1RawDataModeCallback callback) {
        checkNfcServiceAlive();
        try {
            return sVendorService.setM1RawDataModeEnable(true, new M1RawDataModeCallbackImpl(callback));
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
        return false;
    }

    /**
     * Turn off M1 RawData mode
     *
     * @return Whether the operation is successful
     */
    public boolean disableM1RawDataMode() {
        checkNfcServiceAlive();
        try {
            return sVendorService.setM1RawDataModeEnable(false, null);
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
        return false;
    }

    /**
     * Check if M1 RawData mode is on
     *
     * @return Whether M1 RawData mode is on
     * @deprecated  Use the {@code getM1RawDataModeState} method to get a more precise state
     */
    @Deprecated
    public boolean isM1RawDataModeEnabled() {
        checkNfcServiceAlive();
        try {
            return sVendorService.isM1RawDataModeEnabled();
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
        return false;
    }

    /**
     * Get M1 RawData mode state
     *
     * @see #M1_RAW_DATA_MODE_STATE_OFF
     * @see #M1_RAW_DATA_MODE_STATE_TURNING_ON
     * @see #M1_RAW_DATA_MODE_STATE_ON
     *
     * @return M1 RawData mode current state
     */
    public int getM1RawDataModeState() {
        checkNfcServiceAlive();
        try {
            return sVendorService.getM1RawDataModeState();
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
        return M1_RAW_DATA_MODE_STATE_OFF;
    }

    /**
     * Set the time interval for issuing M1 RawData mode commands
     *
     * @param timeInterval Time interval for issuing commands in M1 RawData mode, in milliseconds
     * @return Whether the operation is successful
     */
    public boolean setM1RawDataModeTimeInterval(int timeInterval) {
        checkNfcServiceAlive();
        try {
            return sVendorService.setM1RawDataModeTimeInterval(timeInterval);
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
        return false;
    }

    /**
     * Callback interface for M1 RawData mode open result
     */
    public interface M1RawDataModeCallback {
        /**
         * This interface is called back when M1 RawData mode is successfully enabled or failed
         * @param status Whether the opening is successful or not, 0 means success, other means failure
         */
        void onM1RawDataModeAuthResult(int status);
    }

    private static final class M1RawDataModeCallbackImpl extends IM1RawDataModeCallback.Stub {

        private final M1RawDataModeCallback mCallback;

        public M1RawDataModeCallbackImpl(M1RawDataModeCallback mCallback) {
            this.mCallback = mCallback;
        }

        public void onM1RawDataModeAuthResult(int authStatus) {
            if (mCallback != null) {
                mCallback.onM1RawDataModeAuthResult(authStatus);
            }
        }
    }

    /**
     * This API can change RF register parameters immediately when the NFC is turned on, without restarting the NFC
     *
     * @param data Register parameters to set
     * @param lastCmd If it is set to true, rf discovery will be restarted after the register setting is complete
     * @return Setting the result State
     *         0 - STATUS_SUCCESS
     *        -1 - STATUS_FAILED
     *        -2 - ERROR_STATUS_BUSY
     *        -3 - ERROR_NFC_ON
     *        -4 - ERROR_EMPTY_PAYLOAD
     *        -5 - ERROR_INVALID_LENGTH
     *        -6 - ERROR_INVALID_COMMAND
     *      0xFF - NFC Service dead
     */
    public int changeRfParams(byte[] data, boolean lastCmd) {
        checkNfcServiceAlive();
        try {
            return sVendorService.changeRfParams(data, lastCmd);
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
        return 0xFF;
    }

    /**
     * This API performs getting the active secure element list.
     * @return Secure elements in active state.
     */
    public int[] getActiveSecureElementList() {
        checkNfcServiceAlive();
        try {
            return sVendorService.getActiveSecureElementList();
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
            return null;
        }
    }

    /**
     * This API performs writes of T4T data to Nfcee.
     * @param fileId File Id to which to write
     * @param data data bytes to be written
     * @param length current data length
     * @return number of bytes written if success else negative number of
                error code listed as here .
                -1  STATUS_FAILED
                -2  ERROR_RF_ACTIVATED
                -3  ERROR_MPOS_ON
                -4  ERROR_NFC_NOT_ON
                -5  ERROR_INVALID_FILE_ID
                -6  ERROR_INVALID_LENGTH
                -7  ERROR_CONNECTION_FAILED
                -8  ERROR_EMPTY_PAYLOAD
                -9  ERROR_NDEF_VALIDATION_FAILED
     * <p>Requires {@link   android.Manifest.permission#NFC} permission.
     */
    public int doWriteT4tData(byte[] fileId, byte[] data, int length) {
        checkNfcServiceAlive();
        try {
            return sVendorService.doWriteT4tData(fileId, data, length);
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
            return -1;
        }
    }

    /**
     * This API performs reading of T4T content of Nfcee.
     * @param fileId : File Id from which to read
     * @return read bytes :-Returns read message if success
     *                      Returns null if failed to read
     *                      Returns 0xFF if file is empty.
     * <p>Requires {@link   android.Manifest.permission#NFC} permission.
     */
    public byte[] doReadT4tData(byte[] fileId) {
        checkNfcServiceAlive();
        try {
            return sVendorService.doReadT4tData(fileId);
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
            return null;
        }
    }

    /**
     * This API performs enabling/disabling routing to T4T Nfcee.
     * @param enable : Decide to enable or disable.
     * @return Whether the operation is successful
     */
    public boolean enableT4tNfceeRoute(boolean enable) {
        checkNfcServiceAlive();
        try {
            return sVendorService.enableT4tNfceeRoute(enable);
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
            return false;
        }
    }

    /**
     * This API performs sending raw APDU to T4T Nfcee.
     * @param apdu : apdu command to be sent.
     * @param apduLen : length of apdu command to be sent.
     * @return response bytes :-Returns response if success
     *                          Returns null if failed
     */
    public byte[] sendT4tRawApdu(byte[] apdu, int apduLen) {
        checkNfcServiceAlive();
        try {
            return sVendorService.sendT4tRawApdu(apdu, apduLen);
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
            return null;
        }
    }

    /**
     * This API performs enabling/disabling contactless write to T4T Nfcee.
     * @param enable : Decide to enable or disable.
     * @return Whether the operation is successful
     */
    public boolean enableT4tContactlessWrite(boolean enable) {
        checkNfcServiceAlive();
        try {
            return sVendorService.enableT4tContactlessWrite(enable);
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
            return false;
        }
    }

    /**
     * This interface can force the SAK value of the NFCC
     *
     * @param isEnable Turn this function on or off, and SAK will return to default value
     * @param sak The sak value to be set
     * @return Return true if setting succeeds, return false if other cases
     */
    public boolean setForceSAK(boolean isEnable, byte sak) {
        checkNfcServiceAlive();
        try {
            return sVendorService.setForceSAK(isEnable, sak);
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
        return false;
    }

    /**
     * Add for HCE simulated access card
     *
     * @param enabled true means to set the specified atqa、sak and uid value
     * @param atqa The atqa value to be set
     * @param sak The sak value to be set
     * @param uid The uid value to be set
     * @return Return true if setting succeeds, return false if other cases
     */
    public boolean setHceTypeAConfig(boolean enabled, byte[] atqa, byte[] sak, byte[] uid) {
        checkNfcServiceAlive();
        try {
            return sVendorService.setHceTypeAConfig(enabled, atqa, sak, uid);
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
        return false;
    }

    /**
     * This API starts silent field detect mode.
     *
     * @param timeout: The time after 1st RF ON to exit extended filed detect mode(msec).
     * @return If true is returned, the operation is successful; otherwise, it fails.
     */
    public boolean startSilentFieldDetectMode(int timeout) {
        checkNfcServiceAlive();
        try {
            return sVendorService.startSilentFieldDetectMode(timeout);
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
        return false;
    }

    /**
     * This API stops silent field detect mode.
     *
     * @return If true is returned, the operation is successful; otherwise, it fails.
     */
    public boolean stopSilentFieldDetectMode() {
        checkNfcServiceAlive();
        try {
            return sVendorService.stopSilentFieldDetectMode();
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
        return false;
    }

    /**
     * @return whether the feature is enabled(true) disabled (false)
     */
    public boolean isSilentFieldDetectEnabled() {
        checkNfcServiceAlive();
        try {
            return sVendorService.isSilentFieldDetectEnabled();
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
        return false;
    }

    /**
     * Sets the state of the specified feature
     *
     * @param featureName The name of the feature to be set
     * @param state The state in which the feature is set
     * @param force If true, it will be set forcibly even if the status is unchanged; otherwise, it will not be set repeatedly.
     * @param extras  Additional parameter for switching states, which can be null 
     * @return The state of this feature after setting
     * @see #FEATURE_STATE_ENABLE
     * @see #FEATURE_STATE_DISABLE
     * @see #FEATURE_STATE_UNKNOWN
     */
    public int setFeatureState(String featureName, int state, boolean force, Bundle extras) {
        checkNfcServiceAlive();
        try {
            return sVendorService.setFeatureState(featureName, state, force, extras);
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
        return FEATURE_STATE_UNKNOWN;
    }

    /**
     * Gets the state of the specified feature
     *
     * @param featureName The name of the feature to be get
     * @return The current state of the feature
     * @see #FEATURE_STATE_ENABLE
     * @see #FEATURE_STATE_DISABLE
     * @see #FEATURE_STATE_UNKNOWN
     */
    public int getFeatureState(String featureName) {
        checkNfcServiceAlive();
        try {
            return sVendorService.getFeatureState(featureName);
        } catch (Exception e) {
            e.printStackTrace();
            attemptDeadServiceRecovery(e);
        }
        return FEATURE_STATE_UNKNOWN;
    }

    /**
     * Switch muteRats function
     *
     * @param isEnable true means to enable the muteRats function, otherwise it is disabled
     * @return Was the operation successful
     */
    public boolean switchMuteRats(boolean isEnable) {
        int state = isEnable ? FEATURE_STATE_ENABLE : FEATURE_STATE_DISABLE;
        int ret = setFeatureState(FEATURE_NAME_MUTE_RATS, state, false, null);
        return state == ret;
    }

    /**
     * Query whether the muteRats function is enabled
     *
     * @return True means open, otherwise closed
     */
    public boolean isMuteRatsEnabled() {
        int state = getFeatureState(FEATURE_NAME_MUTE_RATS);
        return state == FEATURE_STATE_ENABLE;
    }
}
