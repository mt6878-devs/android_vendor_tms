package com.tms.nfc.m1;

import static android.nfc.tech.MifareClassic.SIZE_1K;
import static android.nfc.tech.MifareClassic.SIZE_2K;
import static android.nfc.tech.MifareClassic.SIZE_4K;
import static android.nfc.tech.MifareClassic.SIZE_MINI;
import static android.nfc.tech.MifareClassic.TYPE_CLASSIC;
import static android.nfc.tech.MifareClassic.TYPE_PLUS;
import static android.nfc.tech.MifareClassic.TYPE_PRO;
import static com.tms.nfc.TmsUtils.byteArray2Hex;
import static com.tms.nfc.m1.TmsM1Extension.CMD_AUTH_KEY_A;
import static com.tms.nfc.m1.TmsM1Extension.CMD_AUTH_KEY_B;
import static com.tms.nfc.m1.TmsM1Extension.CMD_READ;
import static com.tms.nfc.m1.TmsM1Extension.CMD_WRITE;
import static com.tms.nfc.m1.TmsM1Extension.KEYS;
import static com.tms.nfc.m1.TmsM1Extension.LEN_BLOCK;
import static com.tms.nfc.m1.TmsM1Extension.LEN_CMD_AUTH;

import android.nfc.tech.TagTechnology;
import android.os.Bundle;
import android.util.Pair;

import com.android.nfc.dhimpl.NativeNfcTag;
import com.tms.nfc.TmsLog;

import java.util.HashMap;

public class TmsM1Tag {

    private static final String TAG = TmsM1Extension.TAG;

    private static final String EXTRA_SAK = "sak";

    private static final int MAX_BLOCK_COUNT = 256;

    private final NativeNfcTag tag;

    private final int type;
    private final int blockCount;
    private final int sectorCount;

    private final HashMap<Integer, TmsM1KeyInfo> keyMaps = new HashMap<>();

    private int currentAuthenticatedSector = -1;
    private Pair<Integer, Boolean> currentAuthenticatedKey = Pair.create(-1, true);

    TmsM1Tag(NativeNfcTag tag) {
        this.tag = tag;
        int[] techList = tag.getTechList();
        Bundle[] techExtras = tag.getTechExtras();
        short sak = -1;
        for (int i = 0; i < techList.length; i++) {
            if (techList[i] == TagTechnology.NFC_A) {
                Bundle nfcAExtra = techExtras[i];
                sak = nfcAExtra.getShort(EXTRA_SAK);
                break;
            }
        }
        if (sak < 0) {
            throw new RuntimeException("Tag incorrectly enumerated as NfcA");
        }
        Pair<Integer, Integer> typeSize = getTypeSizeFromSak(sak);
        type = typeSize.first;
        blockCount = typeSize.second;
        sectorCount = getSectorCount(blockCount);
        TmsLog.i(TAG, "M1 type: " + type + ", sectorCount = " + sectorCount + ", blockCount = " + blockCount);
    }

    public int getType() {
        return type;
    }

    public int getBlockCount() {
        return blockCount;
    }

    public int getSectorCount() {
        return sectorCount;
    }

    boolean keyMap() {
        TmsLog.i(TAG, "KeyMap start");
        int prefsKeyAIndex = -1;
        int prefsKeyBIndex = -1;
        for (int i = 0; i < sectorCount; i++) {
            int keyAIndex = authenticateAndGetKeyIndex(true, prefsKeyAIndex, i);
            if (keyAIndex < 0) {
                TmsLog.e(TAG, "keyMap: sector " + i + " key A authenticate failed");
                return false;
            }
            prefsKeyAIndex = keyAIndex;
            int trailerBlockIndex = sectorToBlock(i) + 3;
            byte[] block = readBlock(trailerBlockIndex);

            int keyBIndex = authenticateAndGetKeyIndex(false, prefsKeyBIndex, i);
            if (keyBIndex < 0) {
                TmsLog.e(TAG, "keyMap: sector " + i + " key B authenticate failed");
                return false;
            }
            prefsKeyBIndex = keyBIndex;
            TmsM1KeyInfo keyInfo = TmsM1KeyInfo.decode(block, keyAIndex, keyBIndex);
            if (keyInfo == null) {
                TmsLog.e(TAG, "keyMap: sector " + i + " decode keyInfo failed");
                return false;
            }
            keyMaps.put(i, keyInfo);
        }
        TmsLog.i(TAG, "KeyMap end");
        return true;
    }

    private int authenticateAndGetKeyIndex(boolean isKeyA, int prefsKeyIndex, int sector) {
        int keyStart = (sector == 0 || sector == 0x10) ? 0 : 1;
        if (prefsKeyIndex >=0 && prefsKeyIndex < KEYS.length) {
            if (authenticate(isKeyA, prefsKeyIndex, sector)) {
                return prefsKeyIndex;
            }
        }
        for (int i = keyStart; i < KEYS.length; i++) {
            if (i == prefsKeyIndex) {
                continue;
            }
            if (authenticate(isKeyA, i, sector)) {
                return i;
            }
        }
        return -1;
    }

    private boolean authenticate(boolean isKeyA, int keyIndex, int sector) {
        if (sector == currentAuthenticatedSector && currentAuthenticatedKey != null &&
                (isKeyA == currentAuthenticatedKey.second) && keyIndex == currentAuthenticatedKey.first) {
            TmsLog.d(TAG, "authenticate sector:" + sector + ", but this sector is already authenticated");
            return true;
        }
        if (keyIndex < 0 || keyIndex >= KEYS.length) {
            TmsLog.w(TAG, "unsupport key index: " + keyIndex);
            return false;
        }
        byte[] key = KEYS[keyIndex];
        byte[] cmd = new byte[LEN_CMD_AUTH];
        int i = 0;
        cmd[i++] = isKeyA ? CMD_AUTH_KEY_A : CMD_AUTH_KEY_B;
        cmd[i++] = (byte) sectorToBlock(sector);
        byte[] uid = tag.getUid();
        System.arraycopy(uid, uid.length - 4, cmd, i, 4);
        i += 4;
        System.arraycopy(key, 0, cmd, i, 6);
        boolean status = transceiveWithStatus(cmd);
        if (status) {
            currentAuthenticatedSector = sector;
            currentAuthenticatedKey = Pair.create(keyIndex, isKeyA);
            TmsLog.i(TAG, "sector: " + sector + ", authenticate success with key" + (isKeyA ? "A" : "B") + " " + keyIndex);
        }
        return status;
    }

    private byte[] readBlock(int block) {
        byte[] cmd = {CMD_READ, (byte) block};
        return transceive(cmd);
    }

    private boolean writeBlock(int block, byte[] data) {
        if (data.length != LEN_BLOCK) {
            return false;
        }
        byte[] cmd = new byte[LEN_BLOCK + 2];
        cmd[0] = CMD_WRITE;
        cmd[1] = (byte) block;
        System.arraycopy(data, 0, cmd, 2, data.length);
        return transceiveWithStatus(cmd);
    }

    boolean writeBlockWithAutoAuthenticate(int block, byte[] data) {
        if (data.length != LEN_BLOCK) {
            return false;
        }
        int sector = blockToSector(block);

        TmsM1KeyInfo keyInfo = keyMaps.get(sector);
        if (keyInfo == null) {
            TmsLog.e(TAG, "sector " + sector + " keymap has not been done");
            return false;
        }
        Pair<Integer, Boolean> key = keyInfo.getKey(block, true);

        if (key!=null && authenticate(key.second, key.first, sector)) {
            return writeBlock(block, data);
        }

        return false;
    }

    private byte[] transceive(byte[] data) {
        int[] ret = new int[1];
        TmsLog.d(TAG, "send: " + byteArray2Hex(data));
        byte[] rsp = tag.transceive(data, true, ret);
        TmsLog.d(TAG, "recv: " + byteArray2Hex(rsp) + ", status = " + ret[0]);
        if (rsp == null || (rsp.length == 1 && rsp[0] != 0) || ret[0] != 0) {
            currentAuthenticatedSector = -1;
            currentAuthenticatedKey = null;
        }
        return rsp;
    }

    private boolean transceiveWithStatus(byte[] data) {
        byte[] rsp = transceive(data);
        return rsp != null && rsp.length == 1 && rsp[0] == 0;
    }

    private static int sectorToBlock(int sectorIndex) {
        if (sectorIndex < 32) {
            return sectorIndex * 4;
        } else {
            return 32 * 4 + (sectorIndex - 32) * 16;
        }
    }

    private static int blockToSector(int blockIndex) {
        validateBlock(blockIndex);
        if (blockIndex < 32 * 4) {
            return blockIndex / 4;
        } else {
            return 32 + (blockIndex - 32 * 4) / 16;
        }
    }
    
    private static Pair<Integer, Integer> getTypeSizeFromSak(short sak) {
        int type = 0;
        int size = 0;
        switch (sak) {
            case 0x01:
            case 0x08:
            case 0x28:
            case 0x88:
                type = TYPE_CLASSIC;
                size = SIZE_1K;
                break;
            case 0x09:
                type = TYPE_CLASSIC;
                size = SIZE_MINI;
                break;
            case 0x10:
                type = TYPE_PLUS;
                size = SIZE_2K;
                break;
            case 0x11:
                type = TYPE_PLUS;
                size = SIZE_4K;
                break;
            case 0x18:
            case 0x38:
                type = TYPE_CLASSIC;
                size = SIZE_4K;
                break;
            // NXP-tag: false
            case 0x98:
            case 0xB8:
                type = TYPE_PRO;
                size = SIZE_4K;
                break;
            default:
                throw new RuntimeException(
                        "Tag incorrectly enumerated as MIFARE Classic, SAK = " + sak);
        }
        return Pair.create(type, size);
    }

    public static int getSectorCount(int blockCount) {
        switch (blockCount) {
            case SIZE_1K:
                return 16;
            case SIZE_2K:
                return 32;
            case SIZE_4K:
                return 40;
            case SIZE_MINI:
                return 5;
            default:
                return 0;
        }
    }

    private static void validateBlock(int block) {
        // Just looking for obvious out of bounds...
        if (block < 0 || block >= MAX_BLOCK_COUNT) {
            throw new IndexOutOfBoundsException("block out of bounds: " + block);
        }
    }
}
