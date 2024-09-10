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

import java.util.Arrays;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * Describes a minimum instruction execution unit
 *
 * @since 1.0
 */
class Command {
    final int lineNumber;
    final String head;
    final Type type;
    final List<String> params;

    Command(int lineNumber, String head, Type type, List<String> params) {
        this.lineNumber = lineNumber;
        this.head = head;
        this.type = type;
        this.params = params;
    }

    boolean isNoParam() {
        return params.isEmpty();
    }

    Optional<String> getParam(int index) {
        if (index >= params.size()) {
            return Optional.empty();
        }
        return Optional.of(params.get(index));
    }

    Optional<String> getFirstParam() {
        return getParam(0);
    }

    @Override
    public String toString() {
        return "Command{" +
                "lineNumber=" + lineNumber +
                ", head='" + head + '\'' +
                ", type=" + type +
                ", params=" + params +
                '}';
    }

    /**
     * Defines the type of command
     *
     * @since 1.0
     */
    enum Type {
        SEND(new String[]{"f", "apduf", "F", "APDUF"}), ASSERT(new String[]{"assert", "ASSERT"}),
        WAIT(new String[]{"wait", "WAIT"}), VERSION(new String[]{"v", "ver", "V", "VER"}),
        ASSERT_VERSION(new String[]{"assert_ver", "assert_version", "ASSERT_VER", "ASSERT_VERSION"});
        Set<String> names;

        Type(String[] names) {
            this.names = Arrays.stream(names).collect(Collectors.toSet());
        }
    }
}
