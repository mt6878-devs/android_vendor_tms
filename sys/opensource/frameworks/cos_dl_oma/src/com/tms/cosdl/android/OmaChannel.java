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

import com.tms.cosdl.adaptation.Channel;
import com.tms.cosdl.adaptation.Session;

import java.io.IOException;

/**
 * Implementation of SE Communication Adaptation Layer Channel Interface on Android System
 *
 * @since 2.0
 */
class OmaChannel implements Channel {

    private final android.se.omapi.Channel mChannel;
    private final OmaSession mSession;

    OmaChannel(OmaSession session, android.se.omapi.Channel channel) {
        this.mSession = session;
        this.mChannel = channel;
    }

    @Override
    public void close() {
        mChannel.close();
    }

    @Override
    public boolean isOpen() {
        return mChannel.isOpen();
    }

    @Override
    public boolean isBasicChannel() {
        return mChannel.isBasicChannel();
    }

    @Override
    public byte[] transmit(byte[] command) throws IOException {
        return mChannel.transmit(command);
    }

    @Override
    public Session getSession() {
        return mSession;
    }

    @Override
    public byte[] getSelectResponse() {
        return mChannel.getSelectResponse();
    }
}
