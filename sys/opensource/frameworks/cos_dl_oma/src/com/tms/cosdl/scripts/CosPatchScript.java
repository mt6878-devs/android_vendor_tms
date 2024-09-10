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
import static com.tms.cosdl.TmsLog.error;
import static com.tms.cosdl.TmsLog.warn;

import com.tms.cosdl.CosPatchDlConfig;
import com.tms.cosdl.CosPatchDlStatus;
import com.tms.cosdl.Result;

import java.io.File;
import java.io.IOException;
import java.util.List;
import java.util.Optional;

/**
 * Describes a COS Patch script file object
 *
 * @since 1.0
 */
public abstract class CosPatchScript {

    private static final String TAG = "CosPatchScript";

    private final String name;

    protected CosPatchScript(String name) {
        this.name = name;
    }

    /**
     * Gets all command in the script
     *
     * @return all command in the script
     * @since 1.0
     */
    public abstract List<Command> getCommands();

    /**
     * Gets the version of the script
     *
     * @return the version of the script
     * @since 1.0
     */
    public abstract long getVersion();

    /**
     * Check whether the script version number can be used for the upgrade
     *
     * @param chipCosVersion cos version from chip
     * @param cpds cos patch dl status
     * @param config config
     * @return the return of the script version number can be used for the upgrade
     * @since 1.0
     */
    public abstract Result checkScriptVersion(long chipCosVersion, CosPatchDlStatus cpds, CosPatchDlConfig config);

    /**
     * Convert to core scripts
     *
     * @return core scripts
     * @since 1.0
     */
    abstract CoreScript toCoreScript();

    public String getName() {
        return name;
    }

    /**
     * Creates a script object from the file
     *
     * @param file The source file
     * @return Returns the script object if the file was parsed successfully, null otherwise
     * @since 1.0
     */
    public static Optional<CosPatchScript> createScriptFromFile(File file) {
        if (file == null || !file.exists()) {
            return Optional.empty();
        }
        try {
            CosPatchDecode decode = null;
            String fileAbsolutePath = file.getCanonicalPath();
            if (fileAbsolutePath.endsWith(CosPatchDlConfig.SUFFIX_BIN)) {
                decode = new BinFileDecode(file);
            } else if (fileAbsolutePath.endsWith(CosPatchDlConfig.SUFFIX_TXT)) {
                decode = new TextFileDecode(file);
            } else {
                error(TAG, "unknown type file");
            }
            if (decode == null) {
                error(TAG, "unknown type file: " + fileAbsolutePath);
                return Optional.empty();
            }
            return CosPatchScriptParser.getInstance().parse(decode, fileAbsolutePath);
        } catch (IOException | ScriptVersionMultipleException | NumberFormatException e) {
            debugTracePrint(e);
            error(TAG, "createScriptFromFile failed, " + e.getLocalizedMessage());
        }
        return Optional.empty();
    }

    /**
     * Creates a script from the given text
     *
     * @param text the source text
     * @param name the script name
     * @return Returns the script object if the text was parsed successfully, null otherwise
     * @since 1.0
     */
    public static Optional<CosPatchScript> createScriptFromText(String text, String name) {
        if (text == null || name == null) {
            warn(TAG, "createScriptFromText, text or name is null");
            return Optional.empty();
        }
        try {
            Optional<CosPatchScript> script = CosPatchScriptParser.getInstance().parse(new TextDecode(text), name);
            if (script.isPresent()) {
                return Optional.of(script.get().toCoreScript());
            }
        } catch (IOException | ScriptVersionMultipleException | NumberFormatException e) {
            debugTracePrint(e);
            error(TAG, "createScriptFromText failed, " + e.getLocalizedMessage());
        }
        return Optional.empty();
    }

}
