package com.tms.nfc.m1;

import static com.tms.nfc.m1.TmsM1Extension.KEYS;
import static com.tms.nfc.m1.TmsM1Extension.LEN_ACCESS_BIT;
import static com.tms.nfc.m1.TmsM1Extension.LEN_BLOCK;
import static com.tms.nfc.m1.TmsM1Extension.LEN_KEY;

import android.util.Pair;

import java.util.Arrays;

public class TmsM1KeyInfo {
    private final int keyAIndex;
    private final int keyBIndex;
    private final AccessBit accessBit;

    public TmsM1KeyInfo(int keyAIndex, int keyBIndex, AccessBit accessBit) {
        this.keyAIndex = keyAIndex;
        this.keyBIndex = keyBIndex;
        this.accessBit = accessBit;
    }

    Pair<Integer, Boolean> getKey(int block, boolean isWriter) {
        int whatKey = isWriter ? accessBit.whatKeyCanWrite(block) : accessBit.whatKeyCanRead(block);
        if (whatKey == AccessBit.KEY_NEVER) {
            return null;
        }
        if ((whatKey & AccessBit.KEY_A) != 0) {
            return Pair.create(keyAIndex, true);
        }
        return Pair.create(keyBIndex, false);
    }

    static TmsM1KeyInfo decode(byte[] trailerBlock, int keyA, int keyB) {
        if (trailerBlock == null || trailerBlock.length < LEN_BLOCK) {
            return null;
        }
        if (keyA < 0 || keyA >= KEYS.length || keyB < 0 || keyB >= KEYS.length) {
            return null;
        }
        byte[] accessBit = new byte[LEN_ACCESS_BIT];

        System.arraycopy(trailerBlock, LEN_KEY, accessBit, 0, LEN_ACCESS_BIT);

        return new TmsM1KeyInfo(keyA, keyB, new AccessBit(accessBit));
    }

    private static class AccessBit {

        static final int KEY_NEVER = 0;
        static final int KEY_A = 0x01;
        static final int KEY_B = 0x02;

        private static final int KEY_AB = KEY_A | KEY_B;

        private static final int[] DATA_READ_MAP = new int[]{
                KEY_AB, KEY_AB, KEY_AB, KEY_B, KEY_AB, KEY_B, KEY_AB, KEY_NEVER
        };
        private static final int[] DATA_WRITE_MAP = new int[]{
                KEY_AB, KEY_NEVER, KEY_NEVER, KEY_B, KEY_B, KEY_B, KEY_B, KEY_NEVER
        };

        private static final int[] KEYA_WRITE_MAP = new int[]{
                KEY_A, KEY_A, KEY_NEVER, KEY_B, KEY_B, KEY_NEVER, KEY_NEVER, KEY_NEVER
        };
        private static final int[] ACCESS_BITS_READ_MAP = new int[]{
                KEY_A, KEY_A, KEY_A, KEY_AB, KEY_AB, KEY_AB, KEY_AB, KEY_AB
        };
        private static final int[] ACCESS_BITS_WRITE_MAP = new int[]{
                KEY_NEVER, KEY_A, KEY_NEVER, KEY_B, KEY_NEVER, KEY_B, KEY_NEVER, KEY_NEVER
        };
        private static final int[] KEYB_READ_MAP = new int[]{
                KEY_A, KEY_A, KEY_A, KEY_NEVER, KEY_NEVER, KEY_NEVER, KEY_NEVER, KEY_NEVER
        };
        private static final int[] KEYB_WRITE_MAP = new int[]{
                KEY_A, KEY_A, KEY_NEVER, KEY_B, KEY_B, KEY_NEVER, KEY_NEVER, KEY_NEVER
        };

        private final int[] bits = new int[4];

        AccessBit(byte[] b) {
            byte b7 = b[1];
            byte b8 = b[2];
            int c0 = ((b7 & 0x10) >>> 2) | ((b8 & 0x01) << 1) | ((b8 & 0x10) >>> 4);
            int c1 = ((b7 & 0x20) >>> 3) | (b8 & 0x02) | ((b8 & 0x20) >>> 5);
            int c2 = ((b7 & 0x40) >>> 4) | ((b8 & 0x04) >>> 1) | ((b8 & 0x40) >>> 6);
            int c3 = ((b7 & 0x80) >>> 5) | ((b8 & 0x08) >>> 2) | ((b8 & 0x80) >>> 7);
            bits[0] = c0;
            bits[1] = c1;
            bits[2] = c2;
            bits[3] = c3;
        }

        int whatKeyCanWrite(int block) {
            int index = block % 4;
            int accessBit = bits[index];
            if (index < 3) {
                return DATA_WRITE_MAP[accessBit];
            } else {
                return KEYA_WRITE_MAP[accessBit] & KEYB_WRITE_MAP[accessBit] & ACCESS_BITS_WRITE_MAP[accessBit];
            }
        }

        int whatKeyCanRead(int block) {
            int index = block % 4;
            int accessBit = bits[index];
            if (index < 3) {
                return DATA_READ_MAP[accessBit];
            } else {
                return ACCESS_BITS_READ_MAP[accessBit] | KEYB_READ_MAP[accessBit];
            }
        }

        @Override
        public String toString() {
            return "AccessBit{" +
                    "bits=" + Arrays.toString(bits) +
                    '}';
        }
    }
}
