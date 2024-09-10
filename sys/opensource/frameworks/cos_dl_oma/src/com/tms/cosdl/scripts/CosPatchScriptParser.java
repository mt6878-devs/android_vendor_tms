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
import static com.tms.cosdl.TmsLog.info;
import static com.tms.cosdl.TmsLog.warn;

import com.tms.cosdl.Utils;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Optional;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

/**
 * Cos Patch script file parsing class
 *
 * @since 1.0
 */
class CosPatchScriptParser {

    static final String TAG = "CosPatchScriptParser";

    private static final String MAX_SUPPORT_SCRIPT_VERSION = "010000";

    private static final long COS_VER_PATCH_MASK = 0xFFFF000000L;

    private static final Tag TAG_SCRIPTS_VER_BEGIN = new StringTag("SCRIPTS_VER_BEGIN");
    private static final Tag TAG_SCRIPTS_VER_END = new StringTag("SCRIPTS_VER_END");
    private static final Tag TAG_PATCH_VER_BEGIN =
        new PatternTag(Pattern.compile("^PATCH_([0-9a-fA-F]{4})?_?([0-9a-fA-F]{6})_VER_BEGIN$"));
    private static final Tag TAG_PATCH_VER_END =
        new PatternTag(Pattern.compile("^PATCH_([0-9a-fA-F]{4})?_?([0-9a-fA-F]{6})_VER_END$"));
    private static final Tag TAG_SCRIPTS_COMMANDS_BEGIN = new StringTag("SCRIPTS_COMMANDS_BEGIN");
    private static final Tag TAG_SCRIPTS_COMMANDS_END = new StringTag("SCRIPTS_COMMANDS_END");
    private static final Tag TAG_PATCH_BEGIN =
        new PatternTag(Pattern.compile("^PATCH_([0-9a-fA-F]{4})?_?([0-9a-fA-F]{6})_BEGIN$"));
    private static final Tag TAG_PATCH_END =
        new PatternTag(Pattern.compile("^PATCH_([0-9a-fA-F]{4})?_?([0-9a-fA-F]{6})_END$"));
    private static final Tag TAG_PATCH_VERSION = new PatternTag(Pattern.compile("^V(?:ER)? ([0-9a-fA-F]{10})$"));
    private static final Tag TAG_SCRIPT_VERSION = new PatternTag(Pattern.compile("^V(?:ER)? ([0-9a-fA-F]{6})$"));
    private static final Tag TAG_PATCH_VERSION_CLASSIC =
        new PatternTag(Pattern.compile("^#?.*V(?:ER)? ([0-9a-fA-F]{10})$"));
    private static final Tag TAG_PATCH_COMMAND_CLASSIC =
        new PatternTag(Pattern.compile("^(?:(?:apdu)?f|(?:APDU)?F)\\s*([0-9a-fA-F]+)"));

    private static final long CLASSIC_BASE_VERSION = 0x020100L;
    private static final int SCRIPT_VERSION_CLASSIC = 0;

    Optional<CosPatchScript> parse(CosPatchDecode decode, String name) throws IOException {
        List<String> lines = decode.getLines();
        if (lines.isEmpty()) {
            return Optional.empty();
        }
        int count = lines.size();
        debug(TAG, "start parse, line number: " + count);
        ParserContext ctx = new ParserContext(name);

        for (int i = 0; i < count; i++) {
            ctx.lineNumber += 1;
            String line = lines.get(i).trim().toUpperCase();
            if (Utils.isEmpty(line)) {
                continue;
            }
            if (ctx.getState() != State.INIT && line.startsWith("#")) {
                continue;
            }
            switch (ctx.getState()) {
                case INIT:
                    handleInit(ctx, line);
                    break;
                case SCRIPT_VERSION:
                    handleScriptVersion(ctx, line);
                    break;
                case PATCH_VERSION:
                    handlePatchVersion(ctx, line);
                    break;
                case SCRIPT_COMMAND:
                    handleScriptCommand(ctx, line);
                    break;
                case PATCH_COMMAND:
                    handlePatchCommand(ctx, line);
                    break;
                default:
                    warn(TAG, "parse, unknown state: " + ctx.getState());
                    break;
            }
        }
        if (ctx.getState() == State.PATCH_COMMAND) {
            ctx.collectionPatchCommand();
        }
        ctx.updateState(State.END);
        info(TAG, "parse finished, script version: " + String.format("%06X", ctx.scriptVersion));
        return Optional.of(new MultipleVersionControlSupportScript(name, ctx.scriptVersion, ctx.scripts));
    }

    private void handleInit(ParserContext ctx, String line) {
        Optional<String> find = TAG_SCRIPTS_VER_BEGIN.find(line);
        if (find.isPresent()) {
            ctx.updateState(State.SCRIPT_VERSION);
            return;
        }
        find = TAG_SCRIPTS_COMMANDS_BEGIN.find(line);
        if (find.isPresent()) {
            ctx.updateState(State.SCRIPT_COMMAND);
            return;
        }
        if (ctx.scriptVersion == -1) {
            // classic script parse
            find = TAG_PATCH_VERSION_CLASSIC.find(line);
            if (find.isPresent()) {
                info(TAG, "The version definition is found in init state. It should be a classic script file");
                ctx.scriptVersion = SCRIPT_VERSION_CLASSIC;
                ctx.updateState(State.PATCH_COMMAND);
                ctx.currRequireVersion = CLASSIC_BASE_VERSION;
                ctx.savePatchVersionForCurr(Utils.safeParseLong(find.get(), 16, -1));
                return;
            }
            find = TAG_PATCH_COMMAND_CLASSIC.find(line);
            if (find.isPresent()) {
                info(TAG, "Command is detected before other tags are read. It should be a classic script file");
                ctx.scriptVersion = SCRIPT_VERSION_CLASSIC;
                ctx.updateState(State.PATCH_COMMAND);
                ctx.currRequireVersion = CLASSIC_BASE_VERSION;
                ctx.savePatchVersionForCurr(-1);
                handlePatchCommand(ctx, line);
                return;
            }
        }
        warn(TAG, "handleInit, unknown line: " + line);
    }

    private void handleScriptVersion(ParserContext ctx, String line) {
        Optional<String> find = TAG_SCRIPT_VERSION.find(line);
        if (find.isPresent()) {
            info(TAG, "handleScriptVersion, script version is: " + find.get() +
                    ", code support max version is: " + MAX_SUPPORT_SCRIPT_VERSION);
            ctx.scriptVersion = Utils.safeParseInt(find.get(), 16, -1);
            return;
        }

        find = TAG_PATCH_VER_BEGIN.find(line);
        if (find.isPresent()) {
            ctx.updateState(State.PATCH_VERSION);
            ctx.currRequireVersion = Utils.safeParseLong(find.get(), 16, -1);
            return;
        }
        find = TAG_SCRIPTS_VER_END.find(line);
        if (find.isPresent()) {
            ctx.updateState(State.INIT);
            return;
        }
        warn(TAG, "handleScriptVersion, unknown line: " + line);
    }

    private void handlePatchVersion(ParserContext ctx, String line) {
        Optional<String> find = TAG_PATCH_VERSION.find(line);
        if (find.isPresent()) {
            ctx.savePatchVersionForCurr(Utils.safeParseLong(find.get(), 16, -1));
            debug(TAG, "handlePatchVersion, detect version: " + find.get());
            return;
        }
        find = TAG_PATCH_VER_END.find(line);
        if (find.isPresent()) {
            if (ctx.currRequireVersion != Utils.safeParseLong(find.get(), 16, -1)) {
                warn(TAG, "handlePatchVersion, The start and end version numbers are inconsistent");
            }
            ctx.updateState(State.SCRIPT_VERSION);
            ctx.currRequireVersion = -1;
            return;
        }
        warn(TAG, "handlePatchVersion, unknown line: " + line);
    }

    private void handleScriptCommand(ParserContext ctx, String line) {
        Optional<String> find = TAG_PATCH_BEGIN.find(line);
        if (find.isPresent()) {
            ctx.updateState(State.PATCH_COMMAND);
            ctx.currRequireVersion = Utils.safeParseLong(find.get(), 16, -1);
            ctx.currScriptLines.clear();
            return;
        }
        find = TAG_SCRIPTS_COMMANDS_END.find(line);
        if (find.isPresent()) {
            ctx.updateState(State.INIT);
            return;
        }
        warn(TAG, "handleScriptCommand, unknown line: " + line);
    }

    private void handlePatchCommand(ParserContext ctx, String line) {
        Optional<String> find = TAG_PATCH_END.find(line);
        if (find.isPresent()) {
            ctx.collectionPatchCommand();
            return;
        }
        Optional<Command> optCommand = parseCommand(ctx.lineNumber, line);
        if (optCommand.isPresent()) {
            ctx.saveCommandForCurr(optCommand.get());
            return;
        }
        warn(TAG, "handlePatchCommand, unknown line: " + line);
    }

    private Optional<Command> parseCommand(int lineNumber, String line) {
        List<String> split = Arrays.stream(line.split(" ")).collect(Collectors.toList());
        if (split.isEmpty()) {
            warn(TAG, "empty line");
            return Optional.empty();
        }
        String head = split.get(0);
        Optional<Command.Type> findType =
            Arrays.stream(Command.Type.values()).filter(type -> type.names.contains(head)).findFirst();
        if (!findType.isPresent()) {
            warn(TAG, "parseCommand failed, non-support command: " + line);
            return Optional.empty();
        }
        split.remove(0);
        List<String> params = new ArrayList<>(1);
        params.add(String.join("", split));
        return Optional.of(new Command(lineNumber, head, findType.get(), params));
    }

    public static CosPatchScriptParser getInstance() {
        return CosPatchScriptParserHolder.COS_PATCH_SCRIPT_PARSER;
    }

    private static class CosPatchScriptParserHolder {
        private static final CosPatchScriptParser COS_PATCH_SCRIPT_PARSER = new CosPatchScriptParser();
    }

    private static class ParserContext {

        private static final String TAG = "CosPatchScriptParser.ParserContext";

        final String name;
        long currRequireVersion = -1;
        final Map<Long, Long> patchVersions = new HashMap<>();
        final List<Command> currScriptLines = new ArrayList<>();
        final Map<Long, CoreScript> scripts = new HashMap<>();
        int scriptVersion = -1;
        int lineNumber = 0;
        private State state = State.INIT;

        private ParserContext(String name) {
            this.name = name;
        }

        void collectionPatchCommand() {
            if (currRequireVersion == -1) {
                return;
            }
            Long patchVersion = patchVersions.get(currRequireVersion);
            if (patchVersion == null) {
                return;
            }
            CoreScript script = scripts.get(currRequireVersion);
            if (script != null) {
                warn(TAG, String.format("The script of the same base version is found. " +
                    "The current version is: %010X, and the loaded version is: %010X",
                    patchVersion, script.getVersion()));
                if (script.getVersion() >= patchVersion) {
                    warn(TAG, "The current version is an older version and the current version is ignored");
                    return;
                }
                info(TAG, "The current version is a more recent version, overwriting the previous version");
            }
            long basePatchVersion = currRequireVersion & COS_VER_PATCH_MASK;
            debug(TAG, String.format(Locale.ROOT,
                "create script: name=%s, patchVersion=%010X, baseVersion=%010X, commandSize=%d",
                name, patchVersion, currRequireVersion, currScriptLines.size()));
            CoreScript newScript = new CoreScript(name, patchVersion, basePatchVersion,
                new ArrayList<>(currScriptLines));
            scripts.put(currRequireVersion, newScript);
            updateState(State.SCRIPT_COMMAND);
            currRequireVersion = -1;
            currScriptLines.clear();
        }

        void updateState(State newState) {
            if (state != newState) {
                debug(TAG, "state change: " + state + " -> " + newState);
            }
            state = newState;
        }

        void savePatchVersion(long baseVersion, long patchVersion) {
            if (baseVersion < 0) {
                warn(TAG, "invalidate base version: " + baseVersion);
                return;
            }
            patchVersions.put(baseVersion, patchVersion);
        }

        void savePatchVersionForCurr(long patchVersion) {
            savePatchVersion(currRequireVersion, patchVersion);
        }

        void saveCommandForCurr(Command command) {
            currScriptLines.add(command);
        }

        State getState() {
            return state;
        }
    }

    private interface Tag {
        /**
         * Find the tag in the given input
         *
         * @param input input string
         * @return Returns the matching string if a tag is found, null otherwise
         * @since 1.0
         */
        Optional<String> find(String input);
    }

    private static class StringTag implements Tag {
        private final String value;

        private StringTag(String value) {
            this.value = value;
        }

        @Override
        public Optional<String> find(String input) {
            if (Utils.equals(value, input)) {
                return Optional.of(input);
            }
            return Optional.empty();
        }
    }

    private static class PatternTag implements Tag {
        private final Pattern pattern;

        private PatternTag(Pattern pattern) {
            this.pattern = pattern;
        }

        @Override
        public Optional<String> find(String input) {
            Matcher matcher = pattern.matcher(input);
            if (matcher.find()) {
                StringBuilder sb = new StringBuilder();
                int groupCount = matcher.groupCount();
                for (int i = 0; i < groupCount; i++) {
                    String group = matcher.group(i + 1);
                    sb.append(group == null ? "" : group);
                }
                return Optional.of(sb.toString());
            }
            return Optional.empty();
        }
    }

    private enum State {
        INIT(0), SCRIPT_VERSION(1), PATCH_VERSION(2), SCRIPT_COMMAND(3),
        PATCH_COMMAND(4), END(5);
        int value;

        State(int value) {
            this.value = value;
        }
    }
}
