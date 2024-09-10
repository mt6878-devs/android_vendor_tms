/*
 * Copyright (c) Tsingteng MicroSystem Co., Ltd. 2021. All rights reserved.
 */
/******************************************************************************
 *  Copyright 2018-2020 NXP
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
*
*  The original Work has been changed by Tsingteng MicroSystem.
*
*  Copyright (C) 2021-2022 Tsingteng MicroSystem
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*  http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
*  NOT A CONTRIBUTION
******************************************************************************/

#ifndef TMS_ESE_CONFIG_H
#define TMS_ESE_CONFIG_H

#include <string>
#include <vector>

#include "config.h"
#include "configC.h"

class EseConfig {
public:
    ~EseConfig();
    static bool hasKey(const std::string& key);
    static std::string getString(const std::string& key);
    static std::string getString(const std::string& key,
                                 std::string default_value);
    static unsigned getUnsigned(const std::string& key);
    static unsigned getUnsigned(const std::string& key, unsigned default_value);
    static std::vector<uint8_t> getBytes(const std::string& key);
    static void clear();

private:
    static EseConfig& getInstance();
    EseConfig();

    ConfigFile config_;
};

#ifdef __cplusplus
extern "C" {
#endif

unsigned int ConfigGetUnsigned(const char *key, int keyLen, const unsigned int defaultVal);
void ConfigGetString(char *buff, int buffLen,
                     const char *key, int keyLen,
                     const char *defaultVal, int defaultValLen);

#ifdef __cplusplus
}
#endif
#endif
