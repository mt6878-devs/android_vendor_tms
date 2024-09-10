package com.tms.nfc.m1;

import com.android.nfc.dhimpl.NativeNfcTag;
import com.tms.nfc.TmsLog;

public class TmsM1Extension {

    static final String TAG = "TmsM1Extension";

    static final int LEN_CMD_AUTH = 12;
    static final int LEN_BLOCK = 16;
    static final int LEN_KEY = 6;
    static final int LEN_ACCESS_BIT = 4;

    static final byte CMD_AUTH_KEY_A = 0x60;
    static final byte CMD_AUTH_KEY_B = 0x61;
    static final byte CMD_READ = 0x30;
    static final byte CMD_WRITE = (byte) 0xA0;

    static final byte[] KEY_NDEF = new byte[]{(byte) 0xD3, (byte) 0XF7, (byte) 0xD3, (byte) 0XF7, (byte) 0xD3, (byte) 0XF7};
    static final byte[] KEY_MAD = new byte[]{(byte) 0xA0, (byte) 0XA1, (byte) 0xA2, (byte) 0XA3, (byte) 0xA4, (byte) 0XA5};
    static final byte[] KEY_DEFAULT = new byte[]{(byte) 0xFF, (byte) 0XFF, (byte) 0xFF, (byte) 0XFF, (byte) 0xFF, (byte) 0XFF};
    static final byte[][] KEYS = new byte[][]{
            KEY_MAD, KEY_NDEF, KEY_DEFAULT
    };
    static final byte[] ACCESS_PERMISSION_NFC = new byte[]{0x7F, 0x07, (byte) 0x88, 0x40};
    static final byte[] ACCESS_PERMISSION_MAD = new byte[]{0x78, 0x77, (byte) 0x88, (byte) 0xC1};
    static final byte[] MAD_B1 = new byte[]{0x14, 0x01, 0x03, (byte) 0xE1, 0x03, (byte) 0xE1, 0x03, (byte) 0xE1,
            0x03, (byte) 0xE1, 0x03, (byte) 0xE1, 0x03, (byte) 0xE1, 0x03, (byte) 0xE1};
    static final byte[] MAD_B2 = new byte[]{0x03, (byte) 0xE1, 0x03, (byte) 0xE1, 0x03, (byte) 0xE1, 0x03, (byte) 0xE1,
            0x03, (byte) 0xE1, 0x03, (byte) 0xE1, 0x03, (byte) 0xE1, 0x03, (byte) 0xE1};
    static final byte[] MAD_B64 = new byte[]{(byte) 0xE8, 0x01, 0x03, (byte) 0xE1, 0x03, (byte) 0xE1, 0x03, (byte) 0xE1,
            0x03, (byte) 0xE1, 0x03, (byte) 0xE1, 0x03, (byte) 0xE1, 0x03, (byte) 0xE1};
    static final byte[] NFC_B0 = new byte[]{0x03, 0x00, (byte) 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    private TmsM1Extension() {

    }

    public boolean ndefFormat(NativeNfcTag nativeNfcTag) {
        TmsLog.i(TAG, "ndefFormat enter");
        long startTime = System.currentTimeMillis();
        try {
            TmsM1Tag tag = new TmsM1Tag(nativeNfcTag);

            boolean status = tag.keyMap();
            if (!status) {
                return false;
            }

            byte[] madKeyBlock = new byte[LEN_BLOCK];
            System.arraycopy(KEY_MAD, 0, madKeyBlock, 0, 6);
            System.arraycopy(ACCESS_PERMISSION_MAD, 0, madKeyBlock, 6, 4);
            System.arraycopy(KEY_DEFAULT, 0, madKeyBlock, 10, 6);

            status = ndefFormat1KBlocks(tag, madKeyBlock);
            if (!status) {
                return false;
            }

            int sectorCount = tag.getSectorCount();
            if (sectorCount > 0x10) {
                status = ndefFormatExtrasBlocks(tag, madKeyBlock);
                if (!status) {
                    return false;
                }
            }
        } catch (Exception e) {
            TmsLog.e(TAG, "ndefFormat failed", e);
        }

        long useTime = System.currentTimeMillis() - startTime;

        TmsLog.i(TAG, "ndefFormat success, use time: " + useTime + "ms");

        return true;
    }

    private boolean ndefFormat1KBlocks(TmsM1Tag tag, byte[] madKeyBlock) {
        boolean status = tag.writeBlockWithAutoAuthenticate(1, MAD_B1);
        if (!status) {
            return false;
        }
        status = tag.writeBlockWithAutoAuthenticate(2, MAD_B2);
        if (!status) {
            return false;
        }

        status = tag.writeBlockWithAutoAuthenticate(3, madKeyBlock);
        if (!status) {
            return false;
        }

        // format sector 1
        status = tag.writeBlockWithAutoAuthenticate(4, NFC_B0);
        if (!status) {
            return false;
        }

        byte[] ndefKeyBlock = new byte[LEN_BLOCK];
        System.arraycopy(KEY_NDEF, 0, ndefKeyBlock, 0, 6);
        System.arraycopy(ACCESS_PERMISSION_NFC, 0, ndefKeyBlock, 6, 4);
        System.arraycopy(KEY_DEFAULT, 0, ndefKeyBlock, 10, 6);

        for (int i = 1; i < 0x10; i++) {
            int blockNumber = i * 4 + 3;
            status = tag.writeBlockWithAutoAuthenticate(blockNumber, ndefKeyBlock);
            if (!status) {
                return false;
            }
        }
        return true;
    }

    private boolean ndefFormatExtrasBlocks(TmsM1Tag tag, byte[] madKeyBlock) {
        int blockCount = tag.getBlockCount();
        boolean status = tag.writeBlockWithAutoAuthenticate(64, MAD_B64);
        if (!status) {
            return false;
        }
        status = tag.writeBlockWithAutoAuthenticate(65, MAD_B2);
        if (!status) {
            return false;
        }
        status = tag.writeBlockWithAutoAuthenticate(66, MAD_B2);
        if (!status) {
            return false;
        }
        int end1 = Math.min(blockCount-1, 127);
        for (int i = 67; i <= end1; i+=4) {
            status = tag.writeBlockWithAutoAuthenticate(67, madKeyBlock);
            if (!status) {
                return false;
            }
        }
        for (int i = 0x8F; i < blockCount; i+=16) {
            status = tag.writeBlockWithAutoAuthenticate(67, madKeyBlock);
            if (!status) {
                return false;
            }
        }
        return true;
    }


    public static TmsM1Extension getInstance() {
        return Holder.TMS_M_1_EXTENSION;
    }

    private static class Holder {
        private static final TmsM1Extension TMS_M_1_EXTENSION = new TmsM1Extension();
    }

}
