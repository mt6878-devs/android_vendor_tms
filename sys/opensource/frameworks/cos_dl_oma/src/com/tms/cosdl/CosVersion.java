/*
 * Copyright (c) Tsingteng MicroSystem  Co., Ltd. 2022-2022. All rights reserved.
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

package com.tms.cosdl;

import static com.tms.cosdl.TmsLog.info;

import java.util.Arrays;

/**
 * Defines the classes that store and calculate the Cos version number
 *
 * @since 1.0
 */
public class CosVersion {

    private static final String TAG = "CosVersion";

    private final byte[] cosData;
    private final byte[] patchVer;
    private final byte[] baseVer;
    private final int patchVersion;
    private final int baseVersion;
    private final long cosVersion;
    private final int javaPatchVersion;
    private final int nativePatchVersion;
    private final int majorVersion;
    private final int minorVersion;

    private CosVersion(byte[] cosData,
                            byte[] patchVer,
                            byte[] baseVer) {
        this.cosData = cosData;
        this.patchVer = patchVer;
        this.baseVer = baseVer;

        patchVersion = ((patchVer[0] & 0xFF) << 8) | (patchVer[1] & 0xFF);
        baseVersion = ((baseVer[0] & 0xFF) << 16) | ((baseVer[1] & 0xFF) << 8) | (baseVer[2] & 0xFF);
        cosVersion = ((((long) patchVersion) & 0xFFFF) << 24) | (((long) baseVersion) & 0xFFFFFF);
        javaPatchVersion = (patchVersion & 0xFF00) >>> 8;
        nativePatchVersion = (patchVersion & 0xFF);
        majorVersion = (baseVersion & 0x1F0000) >>> 16;
        minorVersion = (baseVersion & 0x7F00) >>> 8;
    }

    public long getCosVersion() {
        info(TAG, String.format("getCosVersion : chip cosVer = %X , pthVer = %04X, baseVer = %06X",
                                cosVersion, patchVersion, baseVersion));
        return cosVersion;
    }

    public int getMajorVersion() {
        return majorVersion;
    }

    public int getMinorVersion() {
        return minorVersion;
    }

    public int getNativePatchVersion() {
        return nativePatchVersion;
    }

    public int getJavaPatchVersion() {
        return javaPatchVersion;
    }

    @Override
    public String toString() {
        return String.format("CosVersion(patchVersion=%04X, baseVersion=%06X, cosVersion=%010X,"
                                + "javaPatchVersion=%02X, nativePatchVersion=%02X, majorVersion=%02X,"
                                + " minorVersion=%02X)",
                                patchVersion, baseVersion, cosVersion,
                                javaPatchVersion, nativePatchVersion, majorVersion, minorVersion);

    }

    /**
     * Parse to generate a CosVersion object from the response data that gets the COS version number
     *
     * @param cosVerRsp The response data for get COS version number command
     * @return CosVersion object is returned if parsing succeeds, null otherwise
     * @since 1.0
     */
    public static CosVersion decodeFromRsp(byte[] cosVerRsp) {
        int index = 5; // 5
        return new CosVersion(Arrays.copyOfRange(cosVerRsp, index, index += 4),
                                Arrays.copyOfRange(cosVerRsp, index, index += 2),
                                Arrays.copyOfRange(cosVerRsp, index, index + 3));
    }
}
