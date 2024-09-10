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

import java.util.regex.Pattern;

public class TmsConstant {

    public static final String PREFS_RF_LISTEN_MASK = "tms_rf_listen_mask";
    public static final String PREFS_RF_POLL_MASK = "tms_rf_poll_mask";

    public static final String CONFIG_LX_DEBUG_LEVEL = "TMS_LX_DEBUG_LEVEL"; // config key in libnfc-nci.conf

    public static final int LX_DEBUG_LEVEL_L1 = 0x01; // 6F35
    public static final int LX_DEBUG_LEVEL_L2 = 0x02; // 6F36
    public static final int LX_DEBUG_LEVEL_L3 = 0x04; // 6F41

    public static final int LX_DEBUG_LEVEL_ALL = LX_DEBUG_LEVEL_L1 | LX_DEBUG_LEVEL_L2 | LX_DEBUG_LEVEL_L3;

    public static final int DEFAULT_LX_DEBUG_LEVEL = 0;

    public static final Pattern RF_BLOCK_PATTERN = Pattern.compile("(\\w+)\\s*=\\s*\\{((?:[\\s,]*[0-9a-fA-F]{2}[\\s,]*)+)\\}");

    public static final String CONFIG_DISABLE_WIRED_SE = "TMS_DISABLE_WIRED_SE";
    public static final int DEFAULT_DISABLE_WIRED_SE_VALUE = 0;
}
