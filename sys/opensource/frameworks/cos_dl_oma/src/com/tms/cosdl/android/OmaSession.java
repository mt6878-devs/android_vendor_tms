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
import com.tms.cosdl.adaptation.Reader;
import com.tms.cosdl.adaptation.Session;

import java.io.IOException;

/**
 * Implementation of SE Communication Adaptation Layer Session Interface on Android System
 *
 * @since 2.0
 */
class OmaSession implements Session {

    private final android.se.omapi.Session mSession;
    private final OmaReader mReader;

    OmaSession(OmaReader reader, android.se.omapi.Session session) {
        this.mReader = reader;
        this.mSession = session;
    }

    @Override
    public Reader getReader() {
        return mReader;
    }

    @Override
    public byte[] getATR() {
        return mSession.getATR();
    }

    @Override
    public void close() {
        mSession.close();
    }

    @Override
    public boolean isClosed() {
        return mSession.isClosed();
    }

    @Override
    public void closeChannels() {
        mSession.closeChannels();
    }

    @Override
    public Channel openBasicChannel(byte[] aid, byte p2) throws IOException {
        android.se.omapi.Channel channel = mSession.openBasicChannel(aid, p2);
        if (channel == null) {
            throw new IOException("OpenBasicChannel is null, include p2");
        }
        return new OmaChannel(this, channel);
    }

    @Override
    public Channel openBasicChannel(byte[] aid) throws IOException {
        android.se.omapi.Channel channel = mSession.openBasicChannel(aid);
        if (channel == null) {
            throw new IOException("OpenBasicChannel is null");
        }
        return new OmaChannel(this, channel);
    }

    @Override
    public Channel openLogicalChannel(byte[] aid, byte p2) throws IOException {
        android.se.omapi.Channel channel = mSession.openLogicalChannel(aid, p2);
        if (channel == null) {
            throw new IOException("openLogicalChannel is null, include p2");
        }
        return new OmaChannel(this, channel);
    }

    @Override
    public Channel openLogicalChannel(byte[] aid) throws IOException {
        android.se.omapi.Channel channel = mSession.openLogicalChannel(aid);
        if (channel == null) {
            throw new IOException("openLogicalChannel is null");
        }
        return new OmaChannel(this, channel);
    }
}
