/*
 * Copyright (c) Tsingteng MicroSystem Co., Ltd. 2022-2022. All rights reserved.
 */
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

package com.tms.cosdl.android;

import android.content.Context;

import com.tms.cosdl.CosPatchDlConfig;
import com.tms.cosdl.Result;
import com.tms.cosdl.TmsCosPatchDlManager;

/**
 * Help class for triggering COS Patch upgrade for Android system
 *
 * @since 2.0
 */
public class TmsCosPatchDlAndroidHelper {
    /**
     * Trigger COS Patch upgrade on Android platform
     *
     * @param context Context object for Android applications
     * @param config  Related parameter configuration of COS Patch upgrade
     * @return The upgrade result object, which contains the status and error information of the upgrade, etc.
     */
    public static Result cosPatchUpdate(Context context, CosPatchDlConfig config) {
        return TmsCosPatchDlManager.getInstance().cosPatchUpdate(new AndroidContext(context), config);
    }
}
