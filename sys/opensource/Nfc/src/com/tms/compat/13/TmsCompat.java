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

import android.nfc.NfcAdapter;
import com.tms.nfc.TmsLog;
import java.util.Map;

public class TmsCompat{

    private static final String TAG = "TmsCompat-13";

    public static Map getTagIntentAppPreferenceForUser(NfcAdapter adapter, int userId) {
        throw new UnsupportedOperationException(TAG + " Not support getTagIntentAppPreferenceForUser!");
    }

    public static int setTagIntentAppPreferenceForUser(NfcAdapter adapter, int userId, String pkg, boolean allow) {
        throw new UnsupportedOperationException(TAG + " Not support setTagIntentAppPreferenceForUser!");
    }
}

