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

import com.tms.cosdl.CosPatchDlConfig;
import com.tms.cosdl.CosPatchDlStatus;
import com.tms.cosdl.Result;

import java.util.List;
import java.util.stream.Collectors;

/**
 * Core script class
 *
 * @since 1.0
 */
class CoreScript extends CosPatchScript implements Comparable<CoreScript> {

    private static final long COS_VER_MASTER_MASK = 0xFFFFFFL;
    private static final long COS_VER_MASK = 0x7FFFFFFFFFL;

    private final long version;
    private final List<Command> commands;
    private final long basePatchVersion;
    private final long baseVersion;

    CoreScript(String name, long version, long basePatchVersion, List<Command> commands) {
        super(name);
        this.version = version;
        this.commands = commands;
        this.basePatchVersion = basePatchVersion;
        baseVersion = version & COS_VER_MASTER_MASK;
    }

    @Override
    public List<Command> getCommands() {
        return commands;
    }

    @Override
    public long getVersion() {
        return version;
    }

    @Override
    CoreScript toCoreScript() {
        return this;
    }

    @Override
    public Result checkScriptVersion(long chipCosVersion, CosPatchDlStatus cpds, CosPatchDlConfig config) {
        // CoreScript not check version
        if (cpds == null || config == null) {
            return Result.failure(cpds, "checkScriptVersion invalidate params");
        }
        return cpds.success();
    }

    public long getBasePatchVersion() {
        return basePatchVersion;
    }

    public long getBaseVersion() {
        return baseVersion;
    }

    @Override
    public String toString() {
        return String.format("<<V%010X_%04X>>%s%s%s<<V%010X_%04X>>",
            version,
            basePatchVersion,
            System.lineSeparator(),
            commands.stream().map(Command::toString).collect(Collectors.joining(System.lineSeparator())),
            System.lineSeparator(),
            version, basePatchVersion);
    }

    @Override
    public int compareTo(CoreScript other) {
        if (other == null) {
            return 1;
        }
        return Long.compare(version & COS_VER_MASK, other.version & COS_VER_MASK);
    }
}
