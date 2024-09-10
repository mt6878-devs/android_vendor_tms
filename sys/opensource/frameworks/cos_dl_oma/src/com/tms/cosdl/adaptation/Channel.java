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
 * SE communication adaptation layer channel interface
 *
 * @since 2.0
 */
public interface Channel {
    /**
     * Close the current channel
     */
    void close();

    /**
     * Check whether the current channel is opened
     *
     * @return Returns true if the current channel is opened, false otherwise
     */
    boolean isOpen();

    /**
     * Check whether the current channel is a base channel
     *
     * @return Return true if it is a base channel, false otherwise
     */
    boolean isBasicChannel();

    /**
     * Transmit commands to this channel
     *
     * @param command The command to be transferred
     * @return If the transmission is successful, return the response data returned by SE
     * @throws IOException Throws an exception if the transfer fails
     */
    byte[] transmit(byte[] command) throws IOException;

    /**
     * Get the session object associated with this channel
     *
     * @return The session object associated with this channel
     */
    Session getSession();

    /**
     * Get the response reply of the select aid instruction
     *
     * @return The response reply of the select aid instruction
     */
    byte[] getSelectResponse();
}
