/*
 * Copyright (C) 2023 Tsingteng MicroSystem
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

package com.tms.nfc.dhimpl;

import android.nfc.tech.TagTechnology;

import com.android.nfc.dhimpl.NativeNfcTag;
import com.tms.nfc.TmsLog;
import com.tms.nfc.m1.TmsM1Extension;

import java.util.ArrayList;
import java.util.List;

public class TmsNativeNfcTag {

    private static final String TAG = "TmsNativeNfcTag";

    private final NativeNfcTag tag;

    private boolean isInit = false;
    private final List<Integer> techList;

    public TmsNativeNfcTag(NativeNfcTag tag) {
        this.tag = tag;
        techList = new ArrayList<>();
    }

    public boolean hookFormatNdef() {
        return isM1Card();
    }

    public boolean formatNdef(byte[] key) {
        try {
            if (isM1Card()) {
                return TmsM1Extension.getInstance().ndefFormat(tag);
            }
        } catch (Exception e) {
            TmsLog.e(TAG, "formatNdef failed", e);
        }
        return false;
    }

    private void init() {
        if (isInit) {
            return;
        }
        techList.clear();
        int[] list = tag.getTechList();
        for (int tech : list) {
            techList.add(tech);
        }
        isInit = true;
        TmsLog.i(TAG, "techs: " +techList);
    }

    private boolean isM1Card() {
        init();
        return !techList.contains(TagTechnology.ISO_DEP) && techList.contains(TagTechnology.MIFARE_CLASSIC);
    }

}
