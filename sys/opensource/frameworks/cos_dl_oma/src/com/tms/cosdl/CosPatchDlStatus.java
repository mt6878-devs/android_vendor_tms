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

import static com.tms.cosdl.TmsLog.info;

/**
 * This class is used to manage and record various status during the upgrade process, facilitating fault locating
 *
 * @since 1.0
 */
public class CosPatchDlStatus {

    private static final String TAG = "CosPatchDlStatus";

    private static final int MAJOR_VERSION_MASK = 0b11111;
    private static final int MAJOR_VERSION_SHIFT = 64 - 5;

    private static final int MINOR_VERSION_MASK = 0b1111111;
    private static final int MINOR_VERSION_SHIFT = MAJOR_VERSION_SHIFT - 7;

    private static final int STATUS_MASK = 0b111111;
    private static final int STATUS_SHIFT = MINOR_VERSION_SHIFT - 6;

    private static final int LINE_NUMBER_MASK = 0b11111111111111;
    private static final int LINE_NUMBER_SHIFT = STATUS_SHIFT - 14;

    private static final int NATIVE_PATCH_VERSION_MASK = 0xFF;
    private static final int NATIVE_PATCH_VERSION_SHIFT = LINE_NUMBER_SHIFT - 8;

    private static final int JAVA_PATCH_VERSION_MASK = 0xFF;
    private static final int JAVA_PATCH_VERSION_SHIFT = NATIVE_PATCH_VERSION_SHIFT - 8;

    private static final int SW1_MASK = 0xFF;
    private static final int SW1_SHIFT = JAVA_PATCH_VERSION_SHIFT - 8;

    private static final int SW2_MASK = 0xFF;
    private static final int SW2_SHIFT = SW1_SHIFT - 8;

    private State status = State.INIT;
    private CosVersion cosVersionFromChip = null;
    private int currentLineNumber = 0;
    private int sw1 = 0;
    private int sw2 = 0;

    /**
     * Update current Status
     *
     * @param status the new state
     * @param cosVersionFromChip COS version number obtained from the chip
     * @param currentLineNumber The number of lines of current code execution
     * @param lastRsp The response data for the last instruction
     * @since 1.0
     */
    public void update(State status, CosVersion cosVersionFromChip, int currentLineNumber, byte[] lastRsp) {
        if (this.status != status) {
            info(TAG, "state change: " + this.status + " -> " + status);
        }
        this.status = status;
        this.cosVersionFromChip = cosVersionFromChip;
        this.currentLineNumber = currentLineNumber;
        if (lastRsp != null && lastRsp.length >= 2) {
            this.sw1 = lastRsp[lastRsp.length - 2] & SW1_MASK;
            this.sw2 = lastRsp[lastRsp.length - 1] & SW2_MASK;
        }
    }

    /**
     * Update current Status
     *
     * @param status the new state
     * @since 1.0
     */
    public void update(State status) {
        update(status, cosVersionFromChip, currentLineNumber, null);
    }

    /**
     * Update current Status
     *
     * @param cosVersion COS version number obtained from the chip
     * @since 1.0
     */
    public void update(CosVersion cosVersion) {
        update(status, cosVersion, currentLineNumber, null);
    }

    /**
     * Update current Status
     *
     * @param currentLineNumber The number of lines of current code execution
     * @since 1.0
     */
    public void update(int currentLineNumber) {
        update(status, cosVersionFromChip, currentLineNumber, null);
    }

    /**
     * Update current Status
     *
     * @param lastRsp The response data for the last instruction
     * @since 1.0
     */
    public void update(byte[] lastRsp) {
        update(status, cosVersionFromChip, currentLineNumber, lastRsp);
    }

    /**
     * Calculate the error code based on the recorded status data
     *
     * @return error code
     * @since 1.0
     */
    public long getErrorCode() {
        long out = 0L;
        out |= ((long) ((cosVersionFromChip != null ? cosVersionFromChip.getMajorVersion() : 0) &
                MAJOR_VERSION_MASK)) << MAJOR_VERSION_SHIFT;
        out |= ((long) ((cosVersionFromChip != null ? cosVersionFromChip.getMinorVersion() : 0) &
                MINOR_VERSION_MASK)) << MINOR_VERSION_SHIFT;
        out |= ((long) (status.value & STATUS_MASK)) << STATUS_SHIFT;
        out |= ((long) (currentLineNumber & LINE_NUMBER_MASK)) << LINE_NUMBER_SHIFT;
        out |= ((long) ((cosVersionFromChip != null ? cosVersionFromChip.getNativePatchVersion() : 0) &
                NATIVE_PATCH_VERSION_MASK)) << NATIVE_PATCH_VERSION_SHIFT;
        out |= ((long) ((cosVersionFromChip != null ? cosVersionFromChip.getJavaPatchVersion() : 0) &
                JAVA_PATCH_VERSION_MASK)) << JAVA_PATCH_VERSION_SHIFT;
        out |= ((long) (sw1 & SW1_MASK)) << SW1_SHIFT;
        out |= ((long) (sw2 & SW2_MASK)) << SW2_SHIFT;
        return out;
    }

    /**
     * Create a successful Result with an error code
     *
     * @return successful Result with an error code
     * @since 1.0
     */
    public Result success() {
        return Result.success(this);
    }

    /**
     * Create a failed Result with an error code
     *
     * @param message the error message
     * @return failed Result with an error code
     * @since 1.0
     */
    public Result failure(String message) {
        return Result.failure(this, message);
    }

    /**
     * Create a Result with an error code
     *
     * @param isSuccess is success
     * @param message error message
     * @param output the output data
     * @return A Result with an error code
     * @since 1.0
     */
    public Result result(boolean isSuccess, String message, Object output) {
        return Result.create(this, isSuccess, message, output);
    }

    /**
     * Define various states in cos Patch upgrade process
     *
     * @since 1.0
     */
    public enum State {
        INIT(0), LOAD_PATCH_SCRIPT(1), CONNECT_SE_SERVICE(2), GET_ESE_READER(3),
        GET_COS_VER_FROM_CHIP(4), COPY_PATCH_FILE(5) , GET_SCRIPT_VERSION(6),
        COMPARISON_VERSION(7), EXEC_SCRIPT(8), VERIFY_VERSION(9),
        LOAD_RCV_PATCH_SCRIPT(10), RECOVER_COS(11), UPDATE_SUCCESS(12);

        /**
         * status value
         *
         * @since 1.0
         */
        int value;

        State(int value) {
            this.value = value;
        }
    }

    @Override
    public String toString() {
        return "CosPatchDlStatus{" +
                "status=" + status +
                ", cosVersionFromChip=" + cosVersionFromChip +
                ", currentLineNumber=" + currentLineNumber +
                ", sw1=" + sw1 +
                ", sw2=" + sw2 +
                ", errorCode=" + getErrorCode() +
                '}';
    }
}
