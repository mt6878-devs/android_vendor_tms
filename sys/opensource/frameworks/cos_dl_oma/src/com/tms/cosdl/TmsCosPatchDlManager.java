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

import static com.tms.cosdl.TmsLog.debugTracePrint;
import static com.tms.cosdl.TmsLog.error;
import static com.tms.cosdl.TmsLog.info;

import com.tms.cosdl.adaptation.Context;
import com.tms.cosdl.adaptation.Reader;
import com.tms.cosdl.adaptation.SEService;
import com.tms.cosdl.adaptation.Preferences;
import com.tms.cosdl.scripts.CosPatchScript;
import com.tms.cosdl.scripts.CosPatchScriptExecutor;

import java.util.Arrays;
import java.util.Optional;

/**
 * Cos Patch download manager
 *
 * @since 1.0
 */
public class TmsCosPatchDlManager {

    private static final String TAG = "CosPatchDlManager";

    private static final String PREFS_NAME = "TmsCosDl";
    private static final String PREFS_KEY_LAST_LONG_TIME = "TmsCosDl:last_long_time";
    private static final String PREFS_KEY_LAST_SHORT_TIME = "TmsCosDl:last_short_time";
    private static final String PREFS_KEY_LONG_TIME_COUNT = "TmsCosDl:long_time_count";
    private static final String PREFS_KEY_SHORT_TIME_COUNT = "TmsCosDl:short_time_count";

    private static final String VERSION = "2.1";

    /**
     * Upgrade the COS Patch. During the upgrade, the patch file is copied and the version is verified.
     * The upgrade is performed only when the upgrade is required
     * <p>
     * Note: This method cannot be called on the main thread
     *
     * @param ctx    Context
     * @param config Configure the parameters required for the upgrade, including the patch file name and path
     * @return Return to the status after the upgrade,
     *  as long as the COS and patch file versions are the same, it will return success
     */
    public Result cosPatchUpdate(Context ctx, CosPatchDlConfig config) {
        if (ctx == null || config == null) {
            return Result.failure(null, "ctx or config is null");
        }
        TmsLog.setLog(ctx.getLog());
        TmsLog.setLogLevel(config.getLogLevel());
        info(TAG, "cosPatchUpdate, version: " + VERSION + ", config: " + config);
        Result isAllowUpdateResult = isAllowToUpdate(ctx, config);
        if (isAllowUpdateResult.isFailure()) {
            return isAllowUpdateResult;
        }
        CosPatchDlStatus cpds = new CosPatchDlStatus();
        // 1.load patch script
        cpds.update(CosPatchDlStatus.State.LOAD_PATCH_SCRIPT);
        Optional<CosPatchScript> optScript = config.getCosPatchScript();
        if (!optScript.isPresent()) {
            return cpds.failure("patch file load failed");
        }
        CosPatchScript script = optScript.get();
        // 2.connect SEService
        cpds.update(CosPatchDlStatus.State.CONNECT_SE_SERVICE);
        Optional<SEService> seServiceOpt = ctx.getSEService(config.getWaitSeServiceTimeout());
        if (!seServiceOpt.isPresent()) {
            error(TAG, "cosPatchUpdate : The SE Service cannot be connected");
            return cpds.failure("SEService connect failed");
        }
        SEService seService = seServiceOpt.get();
        try {
            Result ret = cosPatchUpdateInternal(seService, cpds, config, script);
            if (ret.isFailure() || !config.isUpgradeTwice()) {
                return ret;
            }
            // After the patch is successfully upgraded, a patch script may exist that relies on the patch as the base
            // patch. Try to upgrade again
            info(TAG, "The upgrade has been successful and we are trying to upgrade to a newer version");
            config.setReturnSuccessOnlyUpdateSuccess(false);
            return cosPatchUpdateInternal(seService, cpds, config, script);
        } catch (Exception e) {
            debugTracePrint(e);
            error(TAG, "cosPatchUpdate : failed, " + e.getLocalizedMessage());
            return cpds.failure("exception occurred during the upgrade, " + e.getLocalizedMessage());
        } finally {
            seService.shutdown();
        }
    }

    private Result getEseReader(SEService seService, CosPatchDlStatus cpds, CosPatchDlConfig config) {
        cpds.update(CosPatchDlStatus.State.GET_ESE_READER);
        Optional<Reader> findEseReader =
            Arrays.stream(seService.getReaders()).
            filter(rReader -> rReader.getName().startsWith(config.getReaderNamePrefix())).findFirst();
        if (!findEseReader.isPresent()) {
            return cpds.failure("eSE reader not exist");
        }
        Reader reader = findEseReader.get();
        return cpds.result(true, "success", reader);
    }

    private Result getCosVersion(CosPatchDlStatus cpds, CosPatchDlConfig config, Reader reader) {
        // 3.get cos version from chip
        cpds.update(CosPatchDlStatus.State.GET_COS_VER_FROM_CHIP);
        Result getCosVersionResult =
            CosPatchScriptExecutor.getInstance().getCosVersionFromChip(cpds, reader, config);
        if (getCosVersionResult.isFailure()) {
            return getCosVersionResult;
        }
        Optional<CosVersion> optCosVersion = getCosVersionResult.getOutput(CosVersion.class);
        if (!optCosVersion.isPresent()) {
            return cpds.failure("failed get cos version from chip");
        }
        CosVersion cosVersion = optCosVersion.get();
        return cpds.result(true, "success", cosVersion);
    }

    private Result doUpdate(CosPatchDlStatus cpds, CosPatchDlConfig config, Reader reader, CosPatchScript script) {
        // 7.start update cos patch
        cpds.update(CosPatchDlStatus.State.EXEC_SCRIPT);
        Result execScriptResult =
                CosPatchScriptExecutor.getInstance().execCosPatchScript(cpds, reader, script, config);
        boolean isUpdateFailed = false;
        if (execScriptResult.isFailure()) {
            error(TAG, "cosPatchUpdate failed, " + execScriptResult.message);
            isUpdateFailed = true;
        }
        cpds.update(CosPatchDlStatus.State.VERIFY_VERSION);
        Result verifyCosVersionResult =
            CosPatchScriptExecutor.getInstance().getCosVersionFromChip(cpds, reader, config);
        if (verifyCosVersionResult.isFailure()) {
            error(TAG, "Failed to obtain the COS version when verifying the COS version, " +
                    verifyCosVersionResult.message);
            isUpdateFailed = true;
        } else {
            long scriptVersion = script.getVersion();
            Optional<CosVersion> optCurrCosVersion = verifyCosVersionResult.getOutput(CosVersion.class);
            if (!optCurrCosVersion.isPresent()) {
                error(TAG, "cosPatchUpdate not complete, get cos version failed");
                isUpdateFailed = true;
            } else if (optCurrCosVersion.get().getCosVersion() != scriptVersion) {
                error(TAG, "cosPatchUpdate not complete, current cos version=" +
                        optCurrCosVersion.get().getCosVersion() + ", but script version=" + scriptVersion);
                isUpdateFailed = true;
            } else {
                info(TAG, "cos version verify success");
            }
        }
        info(TAG, "cosPatchUpdate " + (isUpdateFailed ? "failed" : "success"));
        if (!isUpdateFailed) {
            cpds.update(CosPatchDlStatus.State.UPDATE_SUCCESS);
            return cpds.success();
        }
        return cpds.failure("update failed");
    }

    private void doRecover(CosPatchDlStatus cpds, CosPatchDlConfig config, Reader reader, CosVersion cosVersion) {
        // 8.recover when the update fails
        Result recoverCosResult = recoverCos(cpds, reader, config, cosVersion.getCosVersion());
        if (recoverCosResult.isFailure()) {
            error(TAG, "recover cos failed, " + recoverCosResult.message);
        } else {
            info(TAG, "recovery cos success");
        }
    }

    private Result cosPatchUpdateInternal(SEService seService, CosPatchDlStatus cpds, CosPatchDlConfig config, CosPatchScript script) {
        Result eseReaderResult = getEseReader(seService, cpds, config);
        if (eseReaderResult.isFailure()) {
            return eseReaderResult;
        }
        Optional<Reader> optReader = eseReaderResult.getOutput(Reader.class);
        if (!optReader.isPresent()) {
            return cpds.failure("get ese reader failed");
        }
        Reader reader = optReader.get();
        Result cosVersionResult = getCosVersion(cpds, config, reader);
        if (cosVersionResult.isFailure()) {
            return cosVersionResult;
        }
        Optional<CosVersion> optCosVersion = cosVersionResult.getOutput(CosVersion.class);
        if (!optCosVersion.isPresent()) {
            return cpds.failure("get cos version failed");
        }
        CosVersion cosVersion = optCosVersion.get();
        Result patchScriptResult = script.checkScriptVersion(cosVersion.getCosVersion(), cpds, config);
        if (patchScriptResult.isFailure()) {
            Optional<Boolean> optNeedReturnSuccess = patchScriptResult.getOutput(Boolean.class);
            if (optNeedReturnSuccess.orElse(false)) {
                return cpds.success();
            }
            return patchScriptResult;
        }
        Result updateResult = doUpdate(cpds, config, reader, script);
        if (updateResult.isSuccess) {
            return updateResult;
        }
        doRecover(cpds, config, reader, cosVersion);
        return cpds.failure("update cos patch failed");
    }

    private Result recoverCos(CosPatchDlStatus cpds, Reader reader, CosPatchDlConfig config, long cosVersion) {
        info(TAG, "start recover cos");
        cpds.update(CosPatchDlStatus.State.LOAD_RCV_PATCH_SCRIPT);
        Optional<CosPatchScript> optScript = config.getCosPatchRecoverScript();
        if (!optScript.isPresent()) {
            return cpds.failure("recovery script load failed");
        }
        CosPatchScript script = optScript.get();
        Result result = script.checkScriptVersion(cosVersion, cpds, config);
        if (result.isFailure()) {
            return result;
        }
        cpds.update(CosPatchDlStatus.State.RECOVER_COS);
        return CosPatchScriptExecutor.getInstance().execCosPatchScript(cpds, reader, script, config);
    }

    private Result isAllowToUpdate(Context context, CosPatchDlConfig config) {
        if (!config.isSeErasureProtectionOn()) {
            return Result.success(null);
        }
        Preferences prefs = context.getPreferences(PREFS_NAME);
        long lastLongTime = prefs.getLong(PREFS_KEY_LAST_LONG_TIME, 0);
        long lastShortTime = prefs.getLong(PREFS_KEY_LAST_SHORT_TIME, 0);
        int longTimeCount = prefs.getInt(PREFS_KEY_LONG_TIME_COUNT, 0);
        int shortTimeCount = prefs.getInt(PREFS_KEY_SHORT_TIME_COUNT, 0);
        long currTime = System.currentTimeMillis();

        if (currTime - lastLongTime > config.getLongTimeInterval()) {
            longTimeCount = 0;
            shortTimeCount = 0;
            lastLongTime = currTime;
            lastShortTime = currTime;
        } else {
            if (longTimeCount >= config.getLongTimeMaxCount()) {
                return Result.failure(null,
                        "The number of upgrades exceeded the limit in a long period of time");
            }
            if (currTime - lastShortTime > config.getShortTimeInterval()) {
                shortTimeCount = 0;
                lastShortTime = currTime;
            } else {
                if (shortTimeCount >= config.getShortTimeMaxCount()) {
                    return Result.failure(null,
                            "The number of upgrades exceeded the limit in a short period of time");
                }
            }
        }

        longTimeCount++;
        shortTimeCount++;
        saveAllToPrefs(prefs, lastLongTime, lastShortTime, longTimeCount, shortTimeCount);
        return Result.success(null);
    }

    private void saveAllToPrefs(Preferences prefs, long lastLongTime, long lastShortTime,
                                int longTimeCount, int shortTimeCount) {
        prefs.putLong(PREFS_KEY_LAST_LONG_TIME, lastLongTime);
        prefs.putLong(PREFS_KEY_LAST_SHORT_TIME, lastShortTime);
        prefs.putInt(PREFS_KEY_LONG_TIME_COUNT, longTimeCount);
        prefs.putInt(PREFS_KEY_SHORT_TIME_COUNT, shortTimeCount);
    }

    public static TmsCosPatchDlManager getInstance() {
        return TmsCosPatchDlManagerHolder.TMS_COS_PATCH_DL_MANAGER;
    }

    private static class TmsCosPatchDlManagerHolder {
        private static final TmsCosPatchDlManager TMS_COS_PATCH_DL_MANAGER = new TmsCosPatchDlManager();
    }
}
