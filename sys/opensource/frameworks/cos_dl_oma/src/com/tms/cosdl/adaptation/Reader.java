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


import java.io.IOException;

/**
 * SE communication adaptation layer Reader interface
 *
 * @since 2.0
 */
public interface Reader {
    /**
     * Get the Reader's name
     *
     * @return The Reader's name
     */
    String getName();

    /**
     * Open a session for the current Reader
     *
     * @return Returns the session object if the open is successful
     * @throws IOException Throws an exception if the open fails
     */
    Session openSession() throws IOException;

    /**
     * Determine whether the security unit corresponding to this Reader is available
     *
     * @return Returns true if available, otherwise false
     */
    boolean isSecureElementPresent();

    /**
     * Get SEService object
     *
     * @return The SEService object associated with this Reader
     */
    SEService getSEService();

    /**
     * Close all open sessions of this Reader
     */
    void closeSessions();
}
