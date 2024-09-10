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

package com.tms.cosdl;

import static com.tms.cosdl.TmsLog.debug;

import com.tms.cosdl.scripts.CosPatchScript;

import java.io.File;
import java.util.Optional;

/**
 * Defines the configuration parameters used by the TmsCosDl module
 *
 * @since 1.0
 */
public class CosPatchDlConfig {

    private static final String TAG = "CosDlConfig";

    /**
     * Default COS patch file name
     *
     * @since 1.0
     */
    public static final String COS_PATCH_FILE_NAME = "THN31_ESE_VTP.patch";

    /**
     * Default CRC file name
     *
     * @since 1.0
     */
    public static final String COS_PATCH_CRC_NAME = "ESE_COS_Check";

    /**
     * Default destination directory: /data/vendor/secure_element/
     *
     * @since 1.0
     */
    public static final String DESTINATION_DIR_0 = "/data/vendor/secure_element/";

    /**
     * Default destination directory: /data/nfc/
     *
     * @since 1.0
     */
    public static final String DESTINATION_DIR_1 = "/data/nfc/";

    /**
     * Default source directory: /vendor/etc/
     *
     * @since 1.0
     */
    public static final String SOURCE_DIR_0 = "/vendor/etc/";

    /**
     * Default source directory: /odm/etc/firmware/
     *
     * @since 1.0
     */
    public static final String SOURCE_DIR_1 = "/odm/etc/firmware/";

    /**
     * BIN File name extension
     *
     * @since 1.0
     */
    public static final String SUFFIX_BIN = ".bin";

    /**
     * TXT File name extension
     *
     * @since 1.0
     */
    public static final String SUFFIX_TXT = ".txt";

    /**
     * CRC File name extension
     *
     * @since 1.0
     */
    public static final String SUFFIX_CRC = ".crc";

    /**
     * Recovery File name extension
     *
     * @since 1.0
     */
    public static final String SUFFIX_RCV = ".rcv";

    /**
     * Recovery bin File name extension
     *
     * @since 1.0
     */
    public static final String SUFFIX_RCV_BIN = ".rcv.bin";

    /**
     * Recovery txt File name extension
     *
     * @since 1.0
     */
    public static final String SUFFIX_RCV_TXT = ".rcv.txt";

    /**
     * Default SEService wait timeout
     *
     * @since 1.0
     */
    public static final long CONNECT_SE_SERVICE_TIMEOUT = 4000L;

    /**
     * Default interval for a long period of SE erasure protection
     */
    public static final long DEFAULT_LONG_TIME_INTERVAL = 10 * 24 * 60 * 60 * 1000; // 10 day

    /**
     * Default interval for a short period of SE erasure protection
     */
    public static final long DEFAULT_SHORT_TIME_INTERVAL = 10 * 60 * 1000; // 10 minutes

    /**
     * Default value of the maximum number of the SE erasure protection mechanism in a long period
     */
    public static final int DEFAULT_LONG_TIME_MAX_COUNT = 100;

    /**
     * Default value of the maximum number of SE erase protection upgrades within a short period
     */
    public static final int DEFAULT_SHORT_TIME_MAX_COUNT = 10;

    /**
     * Default value of the prefix of the reader's name
     */
    private static final String DEFAULT_ESE_NAME_PREFIX = "eSE";

    /**
     * Default configuration: /vendor/etc/THN31_ESE_VTP.patch.bin ->
     * /data/vendor/secure_element/THN31_ESE_VTP.patch.bin
     *
     * @since 1.0
     */
    public static final CosPatchDlConfig DEFAULT_CONFIG_0 =
            new CosPatchDlConfig(COS_PATCH_FILE_NAME, SOURCE_DIR_0, DESTINATION_DIR_0);

    /**
     * Default configuration: odm/etc/firmware/THN31_ESE_VTP.patch.bin -> /data/nfc/THN31_ESE_VTP.patch.bin
     */
    public static final CosPatchDlConfig DEFAULT_CONFIG_1 =
            new CosPatchDlConfig(COS_PATCH_FILE_NAME, SOURCE_DIR_1, DESTINATION_DIR_1);

    private static final String[] SUFFIX_LIST = new String[]{SUFFIX_BIN, SUFFIX_TXT};

    /**
     * Patch file name
     *
     * @since 1.0
     */
    public final String patchFileName;

    /**
     * Source directory
     *
     * @since 1.0
     */
    public final String srcDir;

    /**
     * Destination directory
     *
     * @since 1.0
     */
    public final String dstDir;

    /**
     * Whether success is returned only for successful upgrades
     *
     * @since 1.0
     */
    private boolean isReturnSuccessOnlyUpdateSuccess;

    /**
     * Check whether the upgrade requires that the patch file version be larger than the current COS version
     *
     * @since 1.0
     */
    private boolean isRequireVersionBigger;

    /**
     * Timeout time to wait for SESevice connection to succeed
     *
     * @since 1.0
     */
    private long waitSeServiceTimeout;

    /**
     * Whether to enable the ability to open logical channels
     *
     * @since 1.0
     */
    private boolean isEnableLogicalChannel;

    /**
     * Log level used
     *
     * @since 1.0
     */
    private TmsLog.Level logLevel;

    /**
     * Whether to upgrade the patch script again after the upgrade is successful is used to deal with the situation that
     * a patch script relies on another script as the base patch
     *
     * @since 1.0
     */
    private boolean isUpgradeTwice;

    /**
     * Whether to enable the SE write protection mechanism. It is enabled by default
     *
     * @since 1.0
     */
    private boolean isSeErasureProtectionOn;

    /**
     * The SE erasure protection mechanism specifies the long cycle interval. The default period is 10 days
     *
     * @since 1.0
     */
    private long longTimeInterval;

    /**
     * The SE erasure protection mechanism specifies the long cycle interval. The default value is 5 minutes
     *
     * @since 1.0
     */
    private long shortTimeInterval;

    /**
     * Maximum number of times that the SE erasable protection mechanism can be upgraded in a long period.
     * The default value is 100
     *
     * @since 1.0
     */
    private int longTimeMaxCount;

    /**
     * Maximum number of times that the SE erasure protection mechanism can be upgraded in a short period.
     * The default value is 5
     *
     * @since 1.0
     */
    private int shortTimeMaxCount;

    /**
     * Specifies the prefix of the name of the reader used
     * The default value is eSE
     *
     * @since 2.1
     */
    private String readerNamePrefix;

    public CosPatchDlConfig(String patchFileName, String srcDir, String dstDir) {
        this.patchFileName = patchFileName;
        this.srcDir = srcDir;
        this.dstDir = dstDir;
        this.isReturnSuccessOnlyUpdateSuccess = false;
        this.isRequireVersionBigger = true;
        this.waitSeServiceTimeout = CONNECT_SE_SERVICE_TIMEOUT;
        this.isEnableLogicalChannel = false;
        this.logLevel = TmsLog.Level.DEBUG;
        this.isUpgradeTwice = false;
        this.isSeErasureProtectionOn = true;
        this.longTimeInterval = DEFAULT_LONG_TIME_INTERVAL;
        this.longTimeMaxCount = DEFAULT_LONG_TIME_MAX_COUNT;
        this.shortTimeInterval = DEFAULT_SHORT_TIME_INTERVAL;
        this.shortTimeMaxCount = DEFAULT_SHORT_TIME_MAX_COUNT;
        this.readerNamePrefix =  DEFAULT_ESE_NAME_PREFIX;
    }

    public boolean isReturnSuccessOnlyUpdateSuccess() {
        return isReturnSuccessOnlyUpdateSuccess;
    }

    public void setReturnSuccessOnlyUpdateSuccess(boolean isReturnSuccessOnlyUpdateSuccess) {
        this.isReturnSuccessOnlyUpdateSuccess = isReturnSuccessOnlyUpdateSuccess;
    }

    public boolean isRequireVersionBigger() {
        return isRequireVersionBigger;
    }

    public void setRequireVersionBigger(boolean isRequireVersionBigger) {
        this.isRequireVersionBigger = isRequireVersionBigger;
    }

    public long getWaitSeServiceTimeout() {
        return waitSeServiceTimeout;
    }

    public void setWaitSeServiceTimeout(long waitSeServiceTimeout) {
        this.waitSeServiceTimeout = waitSeServiceTimeout;
    }

    public boolean isEnableLogicalChannel() {
        return isEnableLogicalChannel;
    }

    public void setEnableLogicalChannel(boolean isEnableLogicalChannel) {
        this.isEnableLogicalChannel = isEnableLogicalChannel;
    }

    public TmsLog.Level getLogLevel() {
        return logLevel;
    }

    public void setLogLevel(TmsLog.Level logLevel) {
        this.logLevel = logLevel;
    }

    public boolean isUpgradeTwice() {
        return isUpgradeTwice;
    }

    public void setUpgradeTwice(boolean isUpgradeTwice) {
        this.isUpgradeTwice = isUpgradeTwice;
    }

    public boolean isSeErasureProtectionOn() {
        return isSeErasureProtectionOn;
    }

    public void setSeErasureProtectionOn(boolean seErasureProtectionOn) {
        isSeErasureProtectionOn = seErasureProtectionOn;
    }

    public long getLongTimeInterval() {
        return longTimeInterval;
    }

    public void setLongTimeInterval(long longTimeInterval) {
        this.longTimeInterval = longTimeInterval;
    }

    public long getShortTimeInterval() {
        return shortTimeInterval;
    }

    public void setShortTimeInterval(long shortTimeInterval) {
        this.shortTimeInterval = shortTimeInterval;
    }

    public int getLongTimeMaxCount() {
        return longTimeMaxCount;
    }

    public void setLongTimeMaxCount(int longTimeMaxCount) {
        this.longTimeMaxCount = longTimeMaxCount;
    }

    public int getShortTimeMaxCount() {
        return shortTimeMaxCount;
    }

    public void setShortTimeMaxCount(int shortTimeMaxCount) {
        this.shortTimeMaxCount = shortTimeMaxCount;
    }

    public String getReaderNamePrefix() {
        return readerNamePrefix;
    }

    public void setReaderNamePrefix(String prefix) {
        this.readerNamePrefix = prefix;
    }

    /**
     * Obtain the patch script object based on the configured value
     *
     * @return Return the script object if the script file is found, false otherwise
     * @since 1.0
     */
    public Optional<CosPatchScript> getCosPatchScript() {
        return CosPatchScript.createScriptFromFile(getPrimaryFile(srcDir, patchFileName));
    }

    /**
     * Obtain the recover patch script object based on the configured value
     *
     * @return Return the recover script object if the script file is found, false otherwise
     * @since 1.0
     */
    public Optional<CosPatchScript> getCosPatchRecoverScript() {
        return CosPatchScript.createScriptFromFile(getPrimaryFile(srcDir, patchFileName + SUFFIX_RCV));
    }

    private File getPrimaryFile(String dir, String name) {
        File primaryFile = null;
        for (String suffix : SUFFIX_LIST) {
            File file = new File(dir, name + suffix);
            if (file.exists()) {
                primaryFile = file;
                break;
            }
        }
        debug(TAG, "getPrimaryFile: dir=" + dir + ", name=" + name + ", file=" + primaryFile);
        return primaryFile;
    }

    @Override
    public String toString() {
        return "CosPatchDlConfig{" +
                "patchFileName='" + patchFileName + '\'' +
                ", srcDir='" + srcDir + '\'' +
                ", dstDir='" + dstDir + '\'' +
                ", isReturnSuccessOnlyUpdateSuccess=" + isReturnSuccessOnlyUpdateSuccess +
                ", isRequireVersionBigger=" + isRequireVersionBigger +
                ", waitSeServiceTimeout=" + waitSeServiceTimeout +
                ", isEnableLogicalChannel=" + isEnableLogicalChannel +
                ", logLevel=" + logLevel +
                ", isUpgradeTwice=" + isUpgradeTwice +
                ", isSeErasureProtectionOn=" + isSeErasureProtectionOn +
                ", longTimeInterval=" + longTimeInterval +
                ", longTimeMaxCount=" + longTimeMaxCount +
                ", shortTimeInterval=" + shortTimeInterval +
                ", shortTimeMaxCount=" + shortTimeMaxCount +
                ", readerNamePrefix=" + readerNamePrefix +
                '}';
    }
}
