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

package com.tms.cosdl;

import static com.tms.cosdl.TmsLog.debug;
import static com.tms.cosdl.TmsLog.debugTracePrint;
import static com.tms.cosdl.TmsLog.error;
import static com.tms.cosdl.TmsLog.info;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;
import java.util.zip.CRC32;

/**
 * TmsCosDl module utility class
 *
 * @since 1.0
 */
public class Utils {

    private static final char[] HEX_CHARS = {'0', '1', '2', '3', '4', '5', '6',
                                                '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    /**
     * Decode bin to ascii
     *
     * @param input bin byte array
     * @return ascii byte array
     * @since 1.0
     */
    public static byte[] binDecode(byte[] input) {
        if (input == null) {
            return new byte[0];
        }
        for (int i = 0; i < input.length - 1; i++) {
            input[i] = (byte) (input[i + 1] ^ input[i]);
        }
        input[input.length - 1] = (byte) (input[input.length - 1] ^ 0x84);
        return input;
    }

    /**
     * Covert hex string to byte array
     *
     * @param input hex string
     * @return byte array
     * @since 1.0
     */
    public static byte[] str2hex(String input) {
        if (input == null) {
            return new byte[0];
        }
        int len = input.length() / 2;
        byte[] output = new byte[len];
        for (int i = 0; i < len; i++) {
            output[i] = (byte) (safeParseInt(input.substring(i * 2, i * 2 + 2), 16, 0) & 0xFF);
        }
        return output;
    }

    /**
     * Covert byte array to hex string
     *
     * @param input byte array
     * @return hex string
     * @since 1.0
     */
    public static String hex2str(byte[] input) {
        if (input == null) {
            return "";
        }
        StringBuilder output = new StringBuilder(input.length * 2);
        for (byte bInput : input) {
            output.append(HEX_CHARS[(bInput >>> 4) & 0xF]);
            output.append(HEX_CHARS[bInput & 0xF]);
        }
        return output.toString();
    }

    /**
     * Read crc value from file
     *
     * @param file Read the CRC from this file
     * @return crc Returns the CRC value if the read was successful, -1 otherwise
     * @since 1.0
     */
    public static long readCrcFromFile(File file) {
        if (file == null || !file.exists()) {
            return -1;
        }
        try (FileInputStream fis = new FileInputStream(file);
                DataInputStream dis = new DataInputStream(fis)
        ) {
            return dis.readLong();
        } catch (IOException e) {
            error("readCrcFromFile", "failed, " + e.getLocalizedMessage());
        }
        return -1;
    }

    /**
     * Writes the given CRC value to a file
     *
     * @param file File that needs to be written
     * @param crc The CRC value to be written
     * @since 1.0
     */
    public static void writeCrcToFile(File file, long crc) {
        if (file == null) {
            error("writeCrcToFile", "failed, invaild parameter.");
            return;
        }
        try (FileOutputStream fos = new FileOutputStream(file);
                DataOutputStream dos = new DataOutputStream(fos)
        ) {
            dos.writeLong(crc);
        } catch (IOException e) {
            error("writeCrcToFile", "failed, " + e.getLocalizedMessage());
        }
    }

    /**
     * Computes the CRC value for the given file
     *
     * @param file A file that needs to calculate CRC values
     * @return Returns its CRC value if the calculation is successful, otherwise -1
     * @since 1.0
     */
    public static long computerCrc(File file) {
        if (file == null || !file.exists()) {
            return -1;
        }
        CRC32 crc32 = new CRC32();
        byte[] buffer = new byte[4 * 1024];
        try (FileInputStream fis = new FileInputStream(file)) {
            int len;
            while ((len = fis.read(buffer)) != -1) {
                crc32.update(buffer, 0, len);
            }
            return crc32.getValue();
        } catch (IOException e) {
            error("computerCrc", "failed, " + e.getLocalizedMessage());
        }
        return -1;
    }

    /**
     * Check the CRC and copy the file. If the CRC changes, copy the file from the SRC directory to
     * the DST directory and update the CRC file. Otherwise, no action is taken
     *
     * @param name Name of the file to check
     * @param suffix File suffixes
     * @param src Source directory: Files are copied from the source directory to the destination directory
     * @param dst Destination directory. Files are copied from the source directory to the destination directory
     * @since 1.0
     */
    public static void checkCrcAndCopyFile(String name, String suffix, String src, String dst) {
        if (name == null || suffix == null || src == null || dst == null) {
            error("checkCrcAndCopyFile", "failed, invaild parameters.");
            return;
        }
        try {
            String tag = "checkCrcAndCopyFile";
            File file = new File(src, name + suffix);
            if (!file.exists()) {
                TmsLog.warn(tag, file.getCanonicalPath() + " not exists");
                return;
            }
            File crcFile = new File(dst, CosPatchDlConfig.COS_PATCH_CRC_NAME + suffix + CosPatchDlConfig.SUFFIX_CRC);
            long crcFromFile = readCrcFromFile(crcFile);
            long crcFromPatch = computerCrc(file);

            info(tag, "suffix=" + suffix + " crcFromFile=" + crcFromFile + ", crcFromPatch=" + crcFromPatch);

            if (crcFromFile != crcFromPatch) {
                File target = new File(dst, file.getName());
                if (copyFileTo(file, target)) {
                    writeCrcToFile(crcFile, crcFromPatch);
                    info(tag, "copy " + file.getCanonicalPath() + " -> " + target.getCanonicalPath() + " done");
                } else {
                    error(tag, "copy " + file.getCanonicalPath() + " -> " + target.getCanonicalPath() + " failed");
                }
            } else {
                debug(tag, "patch file not updated");
            }
        } catch (IOException e) {
            debugTracePrint(e);
            error("checkCrcAndCopyFile", "failed, " + e.getLocalizedMessage());
        }
    }

    /**
     * Copy source files to target files
     *
     * @param source Source file: The source file is copied to the destination file
     * @param target 目标文件：源文件被复制到目标文件
     * @return Returns whether the replication succeeded
     * @since 1.0
     */
    public static boolean copyFileTo(File source, File target) {
        if (source == null || !source.exists() || target == null) {
            return false;
        }
        try {
            Files.copy(source.toPath(), target.toPath(), StandardCopyOption.REPLACE_EXISTING);
            return true;
        } catch (IOException e) {
            error("copyFileTo", "failed, " + e.getLocalizedMessage());
        }
        return false;
    }

    /**
     * Safely parse to int
     * 
     * @param input the {@code String} containing the integer representation to be parsed
     * @param radix the radix to be used while parsing
     * @param defaultValue Returns this value if the exception is resolved
     * @return the integer represented by the string argument in the specified radix.
     * @since 1.0
     */
    public static int safeParseInt(String input, int radix, int defaultValue) {
        try {
            return Integer.parseInt(input, radix);
        } catch (NumberFormatException e) {
            error("safeParseInt", "parse: " + input + " failed, " + e.getLocalizedMessage());
            return defaultValue;
        }
    }

    /**
     * Safely parse to long
     * 
     * @param input the {@code String} containing the long representation to be parsed
     * @param radix the radix to be used while parsing
     * @param defaultValue Returns this value if the exception is resolved
     * @return the long represented by the string argument in the specified radix.
     * @since 1.0
     */
    public static long safeParseLong(String input, int radix, long defaultValue) {
        try {
            return Long.parseLong(input, radix);
        } catch (NumberFormatException e) {
            error("safeParseLong", "parse: " + input + " failed, " + e.getLocalizedMessage());
            return defaultValue;
        }
    }

    /**
     * Returns true if a and b are equal, including if they are both null.
     * <p><i>Note: In platform versions 1.1 and earlier, this method only worked well if
     * both the arguments were instances of String.</i></p>
     *
     * @param a first CharSequence to check
     * @param b second CharSequence to check
     * @return true if a and b are equal
     */
    public static boolean equals(CharSequence a, CharSequence b) {
        if (a == b) return true;
        int length;
        if (a != null && b != null && (length = a.length()) == b.length()) {
            if (a instanceof String && b instanceof String) {
                return a.equals(b);
            } else {
                for (int i = 0; i < length; i++) {
                    if (a.charAt(i) != b.charAt(i)) return false;
                }
                return true;
            }
        }
        return false;
    }

    /**
     * Returns true if the string is null or 0-length.
     *
     * @param str the string to be examined
     * @return true if str is null or zero length
     */
    public static boolean isEmpty(CharSequence str) {
        return str == null || str.length() == 0;
    }

}
