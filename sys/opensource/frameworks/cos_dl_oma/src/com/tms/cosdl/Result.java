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

import java.util.Optional;

/**
 * Defines an execution return result that contains information about the immediate result status,
 * error messages, output data, and so on
 *
 * @since 1.0
 */
public class Result {

    /**
     * Check whether the execution result is successful
     *
     * @since 1.0
     */
    public final boolean isSuccess;

    /**
     * Error exception message executed
     *
     * @since 1.0
     */
    public final String message;

    /**
     * Status code of the execution result
     *
     * @since 1.0
     */
    public final long code;
    private final Object output;

    public Result(boolean isSuccess, String message, long code, Object out) {
        this.isSuccess = isSuccess;
        this.message = message;
        this.code = code;
        this.output = out;
    }

    /**
     * Whether the execution failed
     *
     * @return Return true on failure, false otherwise
     * @since 1.0
     */
    public boolean isFailure() {
        return !isSuccess;
    }

    /**
     * Gets the output value of the execution result
     *
     * @param type The type of output value
     * @param <T> The type of output value
     * @return Returns the value of the corresponding type if there is a type matching output value,
     * otherwise returns NULL
     * @since 1.0
     */
    public <T> Optional<T> getOutput(Class<T> type) {
        if (output != null && type.isInstance(output)) {
            return Optional.of(type.cast(output));
        }
        return Optional.empty();
    }

    /**
     * Create a Result object
     *
     * @param cpds CosPatch upgrades the status object
     * @param isSuccess whether the execution is successful.
     * @param message Error exception message executed
     * @param output The output value of the execution result
     * @return Return object
     * @since 1.0
     */
    public static Result create(CosPatchDlStatus cpds, boolean isSuccess, String message, Object output) {
        long errorCode = cpds == null ? -1 : cpds.getErrorCode();
        String errorMessage = message == null ? "" : message;
        return new Result(isSuccess, errorMessage, errorCode, output);
    }

    /**
     * Create a failed Result
     *
     * @param cpds CosPatch upgrades the status object
     * @param message Error exception message executed
     * @return Return object
     * @since 1.0
     */
    public static Result failure(CosPatchDlStatus cpds, String message) {
        return create(cpds, false, message, null);
    }

    /**
     * Create a successful Result
     *
     * @param cpds CosPatch upgrades the status object
     * @return Return object
     * @since 1.0
     */
    public static Result success(CosPatchDlStatus cpds) {
        return create(cpds, true, "success", null);
    }

    @Override
    public String toString() {
        return "Result{" +
                "isSuccess=" + isSuccess +
                ", message='" + message + '\'' +
                ", code=" + code +
                '}';
    }
}
