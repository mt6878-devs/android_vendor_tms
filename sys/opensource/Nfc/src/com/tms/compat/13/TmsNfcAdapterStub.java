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
import android.nfc.BeamShareData;
import android.nfc.INfcAdapter;
import android.os.RemoteException;
import android.util.Log;
import com.android.nfc.NfcPermissions;
import com.android.nfc.NfcService;

import java.util.ArrayList;
import java.util.Optional;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;

public abstract class TmsNfcAdapterStub extends INfcAdapter.Stub {
    private static final String TAG = "TmsNfcAdapterStub-13";
    private static final boolean DBG = NfcService.DBG; // TMS_NFC
    private Context mContext;

    public TmsNfcAdapterStub(Context context) {
        mContext = context;
    }

    public void initTmsNfcAdapterStub() {
        if (DBG) Log.i(TAG, "Not necessary in Android T");
    }

    // Comment the implement of p2p function, just for build
    @Override
    public boolean isNdefPushEnabled() throws RemoteException {
        // #ifndef TMS_NFC
        // synchronized (NfcService.this) {
        //     return mState == NfcAdapter.STATE_ON && mIsNdefPushEnabled;
        // }
        // #else
        return false;
        // #endif
    }

    // Comment the implement of p2p function, just for build
    @Override
    public boolean enableNdefPush() throws RemoteException {
        NfcPermissions.enforceAdminPermissions(mContext);
        // #ifndef TMS_NFC
        // synchronized (NfcService.this) {
        //     if (mIsNdefPushEnabled || !mIsBeamCapable) {
        //         return true;
        //     }
        //     Log.i(TAG, "enabling NDEF Push");
        //     mPrefsEditor.putBoolean(PREF_NDEF_PUSH_ON, true);
        //     mPrefsEditor.apply();
        //     mIsNdefPushEnabled = true;
        //     // Propagate the state change to all user profiles
        //     UserManager um = (UserManager) mContext.getSystemService(Context.USER_SERVICE);
        //     List <UserHandle> luh = um.getUserProfiles();
        //     for (UserHandle uh : luh){
        //         enforceBeamShareActivityPolicy(mContext, uh);
        //     }
        //     enforceBeamShareActivityPolicy(mContext, new UserHandle(mUserId));
        //     if (isNfcEnabled()) {
        //         mP2pLinkManager.enableDisable(true, true);
        //     }
        //     mBackupManager.dataChanged();
        // }
        // #endif
        return true;
    }

    // Comment the implement of p2p function, just for build
    @Override
    public boolean disableNdefPush() throws RemoteException {
        NfcPermissions.enforceAdminPermissions(mContext);
        // #ifndef TMS_NFC
        // synchronized (NfcService.this) {
        //     if (!mIsNdefPushEnabled || !mIsBeamCapable) {
        //         return true;
        //     }
        //     Log.i(TAG, "disabling NDEF Push");
        //     mPrefsEditor.putBoolean(PREF_NDEF_PUSH_ON, false);
        //     mPrefsEditor.apply();
        //     mIsNdefPushEnabled = false;
        //     // Propagate the state change to all user profiles
        //     UserManager um = (UserManager) mContext.getSystemService(Context.USER_SERVICE);
        //     List <UserHandle> luh = um.getUserProfiles();
        //     for (UserHandle uh : luh){
        //         enforceBeamShareActivityPolicy(mContext, uh);
        //     }
        //     enforceBeamShareActivityPolicy(mContext, new UserHandle(mUserId));
        //     if (isNfcEnabled()) {
        //         mP2pLinkManager.enableDisable(false, true);
        //     }
        //     mBackupManager.dataChanged();
        // }
        // #endif
        return true;
    }

    // Comment the implement of p2p function, just for build
    @Override
    public void invokeBeam() {
        // #ifndef TMS_NFC
        // if (!mIsBeamCapable) {
        //     return;
        // }
        // NfcPermissions.enforceUserPermissions(mContext);

        // if (mForegroundUtils.isInForeground(Binder.getCallingUid())) {
        //     mP2pLinkManager.onManualBeamInvoke(null);
        // } else {
        //     Log.e(TAG, "Calling activity not in foreground.");
        // }
        // #endif
    }

    // Comment the implement of p2p function, just for build
    @Override
    public void invokeBeamInternal(BeamShareData shareData) {
        NfcPermissions.enforceAdminPermissions(mContext);
        // #ifndef TMS_NFC
        // Message msg = Message.obtain();
        // msg.what = MSG_INVOKE_BEAM;
        // msg.obj = shareData;
        // // We have to send this message delayed for two reasons:
        // // 1) This is an IPC call from BeamShareActivity, which is
        // //    running when the user has invoked Beam through the
        // //    share menu. As soon as BeamShareActivity closes, the UI
        // //    will need some time to rebuild the original Activity.
        // //    Waiting here for a while gives a better chance of the UI
        // //    having been rebuilt, which means the screenshot that the
        // //    Beam animation is using will be more accurate.
        // // 2) Similarly, because the Activity that launched BeamShareActivity
        // //    with an ACTION_SEND intent is now in paused state, the NDEF
        // //    callbacks that it has registered may no longer be valid.
        // //    Allowing the original Activity to resume will make sure we
        // //    it has a chance to re-register the NDEF message / callback,
        // //    so we share the right data.
        // //
        // //    Note that this is somewhat of a hack because the delay may not actually
        // //    be long enough for 2) on very slow devices, but there's no better
        // //    way to do this right now without additional framework changes.
        // mHandler.sendMessageDelayed(msg, INVOKE_BEAM_DELAY_MS);
        // #endif
    }

    // Comment the implement of p2p function, just for build
    @Override
    public void setP2pModes(int initiatorModes, int targetModes) throws RemoteException {
        NfcPermissions.enforceAdminPermissions(mContext);
        // #ifndef TMS_NFC
        // mDeviceHost.setP2pInitiatorModes(initiatorModes);
        // mDeviceHost.setP2pTargetModes(targetModes);
        // mNfcService.applyRouting(true);
        // #endif
    }
}


