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
 * SE communication adaptation layer session interface
 *
 * @since 2.0
 */
public interface Session {
    /**
     * Get the Reader associated with this session
     *
     * @return The Reader associated with this session
     */
    Reader getReader();

    /**
     * Gets the ATR of the Session
     *
     * @return The ATR
     */
    byte[] getATR();

    /**
     * Close the current Session
     */
    void close();

    /**
     * Check whether the current session is closed
     *
     * @return Returns true if the current session is closed, false otherwise
     */
    boolean isClosed();

    /**
     * Closes all open channels in the current session
     */
    void closeChannels();

    /**
     * Open the base channel with the given aid and p2 parameters
     *
     * @param aid The AID you want to select
     * @param p2  The p2 parameters
     * @return Returns the channel object if opened successfully
     * @throws IOException Throws an exception if opening fails
     */
    Channel openBasicChannel(byte[] aid, byte p2) throws IOException;

    /**
     * Open the base channel with the given aid parameters
     *
     * @param aid The AID you want to select
     * @return Returns the channel object if opened successfully
     * @throws IOException Throws an exception if opening fails
     */
    Channel openBasicChannel(byte[] aid) throws IOException;

    /**
     * Open the logic channel with the given aid and p2 parameters
     *
     * @param aid The AID you want to select
     * @param p2  The p2 parameters
     * @return Returns the channel object if opened successfully
     * @throws IOException Throws an exception if opening fails
     */
    Channel openLogicalChannel(byte[] aid, byte p2) throws IOException;

    /**
     * Open the logic channel with the given aid parameters
     *
     * @param aid The AID you want to select
     * @return Returns the channel object if opened successfully
     * @throws IOException Throws an exception if opening fails
     */
    Channel openLogicalChannel(byte[] aid) throws IOException;
}
