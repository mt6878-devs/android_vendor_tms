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

import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.regex.PatternSyntaxException;
import java.util.stream.Collectors;

/**
 * Text decoding class
 *
 * @since 1.0
 */
class TextDecode extends CosPatchDecode {

    private final String text;

    TextDecode(String text) {
        this.text = text;
    }

    @Override
    List<String> decode() {
        try {
            return Arrays.stream(text.split("\\r?\\n|(?<!\\n)\\r\\n")).collect(Collectors.toList());
        } catch (PatternSyntaxException e) {
            debugTracePrint(e);
            return Collections.emptyList();
        }
    }
}
