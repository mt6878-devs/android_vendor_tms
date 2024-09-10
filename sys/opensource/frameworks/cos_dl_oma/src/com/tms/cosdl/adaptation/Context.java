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

package com.tms.cosdl.adaptation;

import java.util.Optional;

/**
 * Host system adaptation layer context acquisition interface,
 * providing interfaces for acquiring SEService, SharedPreference and Log
 *
 * @since 2.0
 */
public interface Context {
    /**
     * Get the SEService object for the current platform
     *
     * @param timeout The timeout time for waiting for SEService to obtain success, in milliseconds
     * @return SEService object for the current platform
     */
    Optional<SEService> getSEService(long timeout);

    /**
     * Get the preference object for the current platform
     *
     * @param name preference name
     * @return The preference object for the current platform
     */
    Preferences getPreferences(String name);

    /**
     * Get the log output object of the current platform
     *
     * @return The log output object for the former platform
     */
    Log getLog();
}
