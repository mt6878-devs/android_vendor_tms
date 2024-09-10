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
import static com.tms.cosdl.TmsLog.debugTracePrint;
import static com.tms.cosdl.TmsLog.error;
import static com.tms.cosdl.TmsLog.info;
import static com.tms.cosdl.TmsLog.warn;

import com.tms.cosdl.CosPatchDlConfig;
import com.tms.cosdl.CosPatchDlStatus;
import com.tms.cosdl.CosVersion;
import com.tms.cosdl.Result;
import com.tms.cosdl.Utils;
import com.tms.cosdl.adaptation.Channel;
import com.tms.cosdl.adaptation.Reader;
import com.tms.cosdl.adaptation.Session;

import java.io.Closeable;
import java.io.IOException;
import java.util.List;
import java.util.Optional;

/**
 * Cos Patch Script executor
 *
 * @since 1.0
 */
public class CosPatchScriptExecutor {

    private static final String TAG = "CosPatchScriptExecutor";

    private static final byte[] STATUS_CODE_SUCCESS = new byte[]{(byte) 0x90, 0x00};
    private static final int MAX_RETRY_COUNT = 3;

    private static final String GET_COS_VER =
        "f 00a4040008544D43524F4F5401" + System.lineSeparator() +
        "f 80E20000087072694E4643434D" + System.lineSeparator() +
        "f 80E265020E";

    /**
     * Run the COS patch script file
     *
     * @param cpds   cos patch update status update
     * @param reader ESE Reader given for executing OMA instructions
     * @param script The script file object that needs to be executed
     * @param config The cos patch update config
     * @return Status of the execution result
     */
    public Result execCosPatchScript(CosPatchDlStatus cpds,
                                            Reader reader,
                                            CosPatchScript script,
                                            CosPatchDlConfig config) {
        if (cpds == null || reader == null || script == null || config == null) {
            return Result.failure(cpds, "execCosPatchScript invalidate params");
        }
        Command currCommand = null;
        long startTime = System.currentTimeMillis();
        try (ExecutionContext ctx = new ExecutionContext(cpds, reader, config.isEnableLogicalChannel())) {
            List<Command> commands = script.getCommands();
            for (Command command : commands) {
                currCommand = command;
                cpds.update(command.lineNumber);
                Result ret = execCommand(ctx, command);
                if (ret.isFailure()) {
                    error(TAG, "execCosPatchScript: exec " + command + " failed, " + ret.message);
                    return ret;
                }
                // save last commands response
                Optional<byte[]> optRsp = ret.getOutput(byte[].class);
                if (optRsp.isPresent()) {
                    byte[] rsp = optRsp.get();
                    ctx.lastRsp = rsp;
                    cpds.update(rsp);
                }
            }
            return ctx.success(ctx.lastRsp);
        } catch (IOException | ScriptVersionNotMatchException e) {
            debugTracePrint(e);
            error(TAG, "execCosPatchScript : exec command(" + currCommand + ") failed: " + e.getLocalizedMessage());
            return cpds.failure("exec " + currCommand + " failed, " + e.getLocalizedMessage());
        } finally {
            long useTime = System.currentTimeMillis() - startTime;
            info(TAG, "exec " + script.getName() + " finish, use time: " + useTime + "ms");
        }
    }

    /**
     * Get the version number from Cos
     *
     * @param cpds   cos patch update status update
     * @param reader ESE Reader given for executing OMA instructions
     * @param config The cos patch update config
     * @return Return null if failed to get the version number,
     *  otherwise return the version number
     */
    public Result getCosVersionFromChip(CosPatchDlStatus cpds, Reader reader, CosPatchDlConfig config) {
        if (cpds == null || reader == null || config == null) {
            return Result.failure(cpds, "getCosVersionFromChip invalidate params");
        }
        Optional<CosPatchScript> optGetCosVersionScript =
            CosPatchScript.createScriptFromText(GET_COS_VER, "GET_COS_VER");
        if (!optGetCosVersionScript.isPresent()) {
            return cpds.failure("decode getCosVersion script failed");
        }
        CosPatchScript getCosVersionScript = optGetCosVersionScript.get();
        Result result = execCosPatchScript(cpds, reader, getCosVersionScript, config);
        if (result.isFailure()) {
            error(TAG, "getCosVersionFromChip failed, " + result.message);
            return result;
        }

        Optional<byte[]> optCosVerRsp = result.getOutput(byte[].class);
        if (!optCosVerRsp.isPresent()) {
            return cpds.failure("getCosVersionFromChip failed, get cos version failed");
        }

        byte[] cosVerRsp = optCosVerRsp.get();
        if (optCosVerRsp.get().length == 16) {
            try {
                CosVersion cosVersion = CosVersion.decodeFromRsp(cosVerRsp);
                cpds.update(cosVersion);
                return cpds.result(true, "get cos version success", cosVersion);
            } catch (ArrayIndexOutOfBoundsException e) {
                debugTracePrint(e);
                return cpds.failure("decode cos version failed, " + e.getLocalizedMessage());
            }
        }
        return cpds.failure("getCosVersionFromChip failed, invalidate cos version response: " +
                            Utils.hex2str(cosVerRsp));
    }

    private Result execCommand(ExecutionContext ctx, Command command) {
        Result ret;
        switch (command.type) {
            case SEND:
                ret = execSend(ctx, command);
                break;
            case ASSERT:
                ret = execAssert(ctx, command);
                break;
            case WAIT:
                ret = execWait(ctx, command);
                break;
            case VERSION:
                ret = execVersion(ctx, command);
                break;
            case ASSERT_VERSION:
                ret = execAssertVersion(ctx, command);
                break;
            default:
                ret = ctx.failure("Unsupported command: " + command);
                break;
        }
        return ret;
    }

    private Result execSend(ExecutionContext ctx, Command command) {
        Optional<String> optApdu = command.getFirstParam();
        if (!optApdu.isPresent()) {
            return ctx.failure("execSend: get apdu param failed");
        }
        String apdu = optApdu.get();
        if (apdu.length() >= 10 && "A40400".equals(apdu.substring(2, 8))) {
            // open channel command
            debug(TAG, "select command convert to open channel command");
            boolean isBasicChannel = (Utils.safeParseInt(apdu.substring(0, 2), 16, 0) & 0x40) == 0;
            if (!isBasicChannel && !ctx.isEnableLogicChannel) {
                warn(TAG, "open a logical channel is not allowed, force change to basic channel");
                isBasicChannel = true;
            }
            String aid = apdu.length() > 10 ? apdu.substring(10) : null;
            return execOpen(ctx, isBasicChannel, aid);
        }

        // normal apdu
        if (ctx.currChannel != null && ctx.currChannel.isOpen()) {
            try {
                debug(TAG, "send apdu: " + apdu);
                byte[] rsp = ctx.currChannel.transmit(Utils.str2hex(apdu));
                debug(TAG, "recv rsp : " + Utils.hex2str(rsp));
                return ctx.success(rsp == null ? STATUS_CODE_SUCCESS : rsp);
            } catch (IOException e) {
                debugTracePrint(e);
                return ctx.failure("transmit apdu(" + apdu + ") failed, " + e.getLocalizedMessage());
            }
        } else {
            return ctx.failure("channel is not opened");
        }
    }

    private Result execOpen(ExecutionContext ctx, boolean isBasicChannel, String aid) {
        debug(TAG, "open called, isBasicChannel=" + isBasicChannel + ", aid=" + aid);
        if (ctx.currChannel != null &&
            (ctx.currChannel.isBasicChannel() == isBasicChannel) &&
            Utils.equals(aid, ctx.selectedAid)) {
            info(TAG, "open same aid, do noting");
            return ctx.successWithSelectResponse();
        }
        if (ctx.currChannel != null && ctx.currChannel.isOpen()) {
            // channel is already opened, close current channel
            ctx.currChannel.close();
        }
        byte[] aidHex = Utils.str2hex(aid);
        try {
            ctx.currChannel = openChannel(ctx.session, isBasicChannel, aidHex);
        } catch (IOException e) {
            debugTracePrint(e);
            return ctx.failure("open channel failed, " + e.getLocalizedMessage());
        }
        ctx.selectedAid = aid;
        debug(TAG, "open channel success");
        return ctx.successWithSelectResponse();
    }

    private Channel openChannel(Session session, boolean isBasicChannel, byte[] aidHex) throws IOException {
        int retryCount = 0;
        while (retryCount < MAX_RETRY_COUNT) {
            try {
                return isBasicChannel ? session.openBasicChannel(aidHex) : session.openLogicalChannel(aidHex);
            } catch (IOException e) {
                warn(TAG, "open channel failed, " + e.getLocalizedMessage() + "retry: " + retryCount);
                retryCount++;
                try {
                    Thread.sleep(20);
                } catch (InterruptedException ie) {
                    debugTracePrint(ie);
                }
            }
        }
        throw new IOException("open channel failed");
    }

    private Result execWait(ExecutionContext ctx, Command command) {
        Optional<String> optWaitTime = command.getFirstParam();
        if (!optWaitTime.isPresent()) {
            return ctx.failure("execWait: The waiting time is not set");
        }
        long waitTime = Utils.safeParseLong(optWaitTime.get(), 10, -1);
        if (waitTime <= 0) {
            return ctx.failure("execWait: The waiting time is invalidate: " + waitTime);
        }
        debug(TAG, "wait called, " + waitTime + "ms");
        try {
            Thread.sleep(waitTime);
        } catch (InterruptedException e) {
            debugTracePrint(e);
        }
        return ctx.success();
    }

    private Result execAssert(ExecutionContext ctx, Command command) {
        if (command.isNoParam()) {
            return ctx.success();
        }
        Optional<String> optStatusCode = command.getFirstParam();
        if (!optStatusCode.isPresent()) {
            warn(TAG, "call assert with empty params, return success");
            return ctx.success();
        }
        String statusCode = optStatusCode.get();

        if ("X".equals(statusCode)) {
            // ignore status code check
            return ctx.success();
        }

        String lastRspText = Utils.hex2str(ctx.lastRsp);
        return lastRspText.endsWith(statusCode) ? ctx.success() : ctx.failure("assert failed, lastRsp: "
                + lastRspText + ", but require status code: " + statusCode);
    }

    private Result execAssertVersion(ExecutionContext ctx, Command command) {
        if (command.isNoParam()) {
            return ctx.success();
        }
        Optional<String> firstParamOpt = command.getFirstParam();
        if (!firstParamOpt.isPresent()) {
            warn(TAG, "call assert_version with empty params, return success");
            return ctx.success();
        }

        String assertValue = firstParamOpt.get();
        int versionBegin = assertValue.indexOf('[');
        int versionEnd = assertValue.lastIndexOf(']');
        String lastRsp = Utils.hex2str(ctx.lastRsp);
        if (versionBegin < 0 || versionEnd < 0) {
            // no version check
            return lastRsp.endsWith(assertValue) ? ctx.success() : ctx.failure("assert_version failed," +
                    " lastRsp: " + lastRsp + ", but require rsp: " + assertValue);
        }
        int assertRspLen = assertValue.length() - 2;
        if (assertRspLen != lastRsp.length()) {
            return ctx.failure("assert_version failed, length not equals, " +
                    " lastRsp: " + lastRsp + ", but require rsp: " + assertValue);
        }
        int lastRspVersionBegin = versionBegin;
        int lastRspVersionEnd = versionEnd - 1;
        if (!assertValue.substring(0, versionBegin).equalsIgnoreCase(lastRsp.substring(0, lastRspVersionBegin))) {
            // prefix not equals
            return ctx.failure("assert_version failed, prefix not equals, " +
                    " lastRsp: " + lastRsp + ", but require rsp: " + assertValue);
        }
        if (!assertValue.substring(versionEnd + 1).equalsIgnoreCase(lastRsp.substring(lastRspVersionEnd))) {
            // suffix not equals
            return ctx.failure("assert_version failed, suffix not equals, " +
                    " lastRsp: " + lastRsp + ", but require rsp: " + assertValue);
        }
        long assertVersion = Utils.safeParseLong(assertValue.substring(versionBegin + 1, versionEnd), 16, -1);
        if (assertVersion == -1) {
            return ctx.failure("assert_version failed, parse assert version failed: " +
                            assertValue.substring(versionBegin + 1, versionEnd));
        }
        long lastRspVersion = Utils.safeParseLong(lastRsp.substring(lastRspVersionBegin, lastRspVersionEnd), 16, -1);
        if (lastRspVersion < assertVersion) {
            return ctx.failure("assert_version failed, the version number comparison check failed, " +
                    "the required version is " + String.format("%04X", assertVersion) +
                    ", but the current version is " + String.format("%04X", lastRspVersion));
        }
        return ctx.success();
    }

    private Result execVersion(ExecutionContext ctx, Command command) {
        Optional<String> optVersion = command.getFirstParam();
        if (optVersion.isPresent()) {
            String version = optVersion.get();
            info(TAG, "patch file version: " + version);
        } else {
            warn(TAG, "execVersion: get version param failed");
        }
        return ctx.success();
    }

    private static class ExecutionContext implements Closeable {
        final Session session;
        final CosPatchDlStatus cpds;

        final boolean isEnableLogicChannel;

        byte[] lastRsp;
        Channel currChannel;
        String selectedAid;

        ExecutionContext(CosPatchDlStatus cpds, Reader reader, boolean isEnableLogicChannel) throws IOException {
            this.cpds = cpds;
            session = openSession(reader);
            this.isEnableLogicChannel = isEnableLogicChannel;
        }

        void cleanup() {
            session.close();
        }

        Result failure(String message) {
            return cpds.failure(message);
        }

        Result success(Object output) {
            return cpds.result(true, "success", output);
        }

        Result success() {
            return success(null);
        }

        Result successWithSelectResponse() {
            byte[] selectResponse = currChannel != null ? currChannel.getSelectResponse() : null;
            return success(selectResponse);
        }

        @Override
        public void close() throws IOException {
            cleanup();
        }

        private Session openSession(Reader reader) throws IOException {
            int retryCount = 0;
            while (retryCount < MAX_RETRY_COUNT) {
                try {
                    return reader.openSession();
                } catch (IOException e) {
                    warn(TAG, "open session failed, " + e.getLocalizedMessage() + "retry: " + retryCount);
                    retryCount++;
                    try {
                        Thread.sleep(20);
                    } catch (InterruptedException ie) {
                        debugTracePrint(ie);
                    }
                }
            }
            throw new IOException("open session failed");
        }
    }

    public static CosPatchScriptExecutor getInstance() {
        return CosPatchScriptExecutorHolder.COS_PATCH_SCRIPT_EXECUTOR;
    }

    private static class CosPatchScriptExecutorHolder {
        private static final CosPatchScriptExecutor COS_PATCH_SCRIPT_EXECUTOR = new CosPatchScriptExecutor();
    }
}
