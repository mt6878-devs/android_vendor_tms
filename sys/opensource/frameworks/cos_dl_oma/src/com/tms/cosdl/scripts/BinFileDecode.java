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

package com.tms.cosdl.scripts;

import static com.tms.cosdl.TmsLog.debugTracePrint;

import com.tms.cosdl.Utils;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.regex.PatternSyntaxException;
import java.util.stream.Collectors;

/**
 * Bin file decoding class
 *
 * @since 1.0
 */
class BinFileDecode extends CosPatchDecode {
    private final File file;

    BinFileDecode(File file) {
        this.file = file;
    }

    @Override
    List<String> decode() {
        try {
            return Arrays.stream(new String(Utils.binDecode(Files.readAllBytes(file.toPath())),
                        StandardCharsets.US_ASCII).split("\\r?\\n|(?<!\\n)\\r\\n")).collect(Collectors.toList());
        } catch (IOException | SecurityException | IndexOutOfBoundsException | PatternSyntaxException e) {
            debugTracePrint(e);
            return Collections.emptyList();
        }
    }
}
