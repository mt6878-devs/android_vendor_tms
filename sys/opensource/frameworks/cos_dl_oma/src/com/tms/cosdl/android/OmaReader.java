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

import com.tms.cosdl.adaptation.Reader;
import com.tms.cosdl.adaptation.SEService;
import com.tms.cosdl.adaptation.Session;

import java.io.IOException;

/**
 * Implementation of Reader Interface of SE Communication Adaptation Layer on Android System
 *
 * @since 2.0
 */
class OmaReader implements Reader {

    private final android.se.omapi.Reader mReader;
    private final OmaSEService mSEService;

    OmaReader(OmaSEService seService, android.se.omapi.Reader reader) {
        this.mSEService = seService;
        this.mReader = reader;
    }

    @Override
    public String getName() {
        return mReader.getName();
    }

    @Override
    public Session openSession() throws IOException {
        return new OmaSession(this, mReader.openSession());
    }

    @Override
    public boolean isSecureElementPresent() {
        return mReader.isSecureElementPresent();
    }

    @Override
    public SEService getSEService() {
        return mSEService;
    }

    @Override
    public void closeSessions() {
        mReader.closeSessions();
    }
}
