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

import static com.tms.cosdl.TmsLog.debug;
import static com.tms.cosdl.TmsLog.error;
import static com.tms.cosdl.TmsLog.info;

import com.tms.cosdl.CosPatchDlConfig;
import com.tms.cosdl.CosPatchDlStatus;
import com.tms.cosdl.Result;

import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.stream.Collectors;

/**
 * Script classes that support multiple versioning
 *
 * @since 1.0
 */
class MultipleVersionControlSupportScript extends CosPatchScript {

    private static final String TAG = "MultipleVersionControlSupportScript";
    private static final String MESSAGE_NOT_MATCH =
        "checkScriptVersion was not called or an available patch script was not matched";
    private static final long COS_VER_MASTER_MASK = 0xFFFFFFL;
    private static final long COS_VER_PATCH_MASK = 0x7FFF000000L;
    private static final long COS_UPDATE_FLAG_MASK = 0x8000000000L;

    private final int scriptVersion;
    private final Map<Long, CoreScript> scripts;

    private final String supportBaseVersions;
    private final String availablePatchVersions;

    private CoreScript mMatchedScript;

    MultipleVersionControlSupportScript(String name, int scriptVersion, Map<Long, CoreScript> scripts) {
        super(name);
        this.scriptVersion = scriptVersion;
        this.scripts = scripts;

        supportBaseVersions = scripts.keySet().stream().
            map(baseVersion -> String.format("%010X", baseVersion)).collect(Collectors.joining(","));
        availablePatchVersions = scripts.values().stream().
            map(script -> String.format("%010X", script.getVersion())).collect(Collectors.joining(","));
    }

    @Override
    public List<Command> getCommands() {
        if (mMatchedScript == null) {
            throw new ScriptVersionNotMatchException("getCommands failed, " + MESSAGE_NOT_MATCH);
        }
        return mMatchedScript.getCommands();
    }

    @Override
    public long getVersion() {
        if (mMatchedScript == null) {
            throw new ScriptVersionNotMatchException("getVersion failed, " + MESSAGE_NOT_MATCH);
        }
        return mMatchedScript.getVersion();
    }

    @Override
    CoreScript toCoreScript() {
        if (mMatchedScript != null) {
            return mMatchedScript;
        }
        Optional<CoreScript> first = scripts.values().stream().findFirst();
        if (scripts.size() != 1) {
            throw new ScriptVersionMultipleException();
        }
        return first.get();
    }

    @Override
    public Result checkScriptVersion(long chipCosVersion, CosPatchDlStatus cpds, CosPatchDlConfig config) {
        if (cpds == null || config == null) {
            return Result.failure(cpds, "checkScriptVersion invalidate params");
        }
        info(TAG, "checkScriptVersion, availablePatchVersion: " + availablePatchVersions);
        long chipVerMaster = chipCosVersion & COS_VER_MASTER_MASK;
        long chipVerPatch = chipCosVersion & COS_VER_PATCH_MASK;
        cpds.update(CosPatchDlStatus.State.GET_SCRIPT_VERSION);
        Optional<CoreScript> optScript = findScript(chipVerMaster, chipVerPatch);
        if (!optScript.isPresent()) {
            return cpds.failure("The patch corresponding to the version("
                    + String.format("%010X", chipCosVersion) + ") is not found, support patch: "
                    + supportBaseVersions);
        }
        CoreScript script = optScript.get();
        long matchedScriptVersion = script.getVersion();
        long scriptVerMaster = matchedScriptVersion & COS_VER_MASTER_MASK;

        info(TAG, String.format("checkScriptVersion : cosVersionFromChip = %X, scriptVersion = %X",
                                chipCosVersion, matchedScriptVersion));

        cpds.update(CosPatchDlStatus.State.COMPARISON_VERSION);

        if (chipVerMaster != scriptVerMaster) {
            error(TAG, "The base version of the script is inconsistent with the base version of the chip,"
                    + "so it cannot be upgraded");
            return cpds.failure("The base version of the script is inconsistent with the base version of the chip,"
                                + "so it cannot be upgraded");
        }

        long scriptVerPatch = matchedScriptVersion & COS_VER_PATCH_MASK;
        boolean cosMustBeUpdate = (chipCosVersion & COS_UPDATE_FLAG_MASK) == 0;

        if (cosMustBeUpdate) {
            info(TAG, "cos must be update");
        } else {
            if (chipVerPatch == scriptVerPatch) {
                info(TAG, "cos is already latest version");
                return cpds.result(false, "cos is already latest version", !config.isReturnSuccessOnlyUpdateSuccess());
            }
    
            if (chipVerPatch > scriptVerPatch) {
                info(TAG, "checkScriptVersion : The script version was smaller than the chip version, "
                        + (config.isRequireVersionBigger() ? "so we do not upgrade" : "but we upgraded it anyway"));
                if (config.isRequireVersionBigger()) {
                    return cpds.failure("chip version is big than script version");
                }
            }
        }

        mMatchedScript = script;
        checkNewScripExist(script, config);
        return cpds.success();
    }

    private Optional<CoreScript> findScript(long baseVersion, long patchVersion) {
        return scripts.values().stream()
                .filter(script -> script.getBaseVersion() == baseVersion &&
                (script.getBasePatchVersion() & COS_VER_PATCH_MASK) <= patchVersion)
                .max(CoreScript::compareTo);
    }

    private void checkNewScripExist(CoreScript script, CosPatchDlConfig config) {
        boolean isNewerScriptExist = scripts.values().stream()
                .anyMatch(
                spt -> spt.getBaseVersion() == script.getBaseVersion() &&
                spt.getVersion() > script.getVersion());
        if (isNewerScriptExist) {
            info(TAG, "An newer patch based on the current patch exists");
        } else {
            debug(TAG, "The current patch is the latest patch");
        }
        config.setUpgradeTwice(isNewerScriptExist);
    }

    @Override
    public String toString() {
        return String.format("ScriptVersion(V%06X): %s%s%s", scriptVersion, getName(), System.lineSeparator(),
                                scripts.values().stream().map(CoreScript::toString)
                                .collect(Collectors.joining(System.lineSeparator())));
    }
}
