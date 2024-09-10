/*
 * Copyright (c) Tsingteng MicroSystem Co., Ltd. 2021. All rights reserved.
 */
/******************************************************************************
 *
 *  Copyright 2018 NXP
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
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

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/strings.h>

#include <cstring>
#include "config.h"
#include "eseConfig.h"
#include "tmsCommon.h"
#include "tmslog.h"

#if defined (USE_TMS_NFC) || defined (USE_C1)
    #include "tmsSecureString.h"
#else
    #include "securec.h"
#endif

using namespace ::std;
using namespace ::android::base;

namespace {
std::string findConfigPath()
{
    const vector<string> search_path = {"/vendor/etc/", "/odm/etc/", "/etc/"};
#if defined (USE_TMS_NFC) || defined (USE_C1)
    const string file_name = "libese-tms.conf";
#else
    const string file_name = "libnfc-tms-thn31.conf";
#endif

    for (string path : search_path) {
        path.append(file_name);
        struct stat file_stat;
        if (stat(path.c_str(), &file_stat) != 0) {
            continue;
        }
        if (S_ISREG(file_stat.st_mode)) {
            return path;
        }
    }
    return "";
}
}  // namespace

/** Functions called by C [start]*/
#ifdef TAG
#undef TAG
#endif
#define TAG g_tag

static const char g_tag[] = "eseConfig";
unsigned int ConfigGetUnsigned(const char *key, int keyLen, const unsigned int defaultVal)
{
    UNUSED(keyLen);
    string sKey(key);
    return EseConfig::getUnsigned(sKey, defaultVal);
}

void ConfigGetString(char *buff, int buffLen,
                     const char *key, int keyLen,
                     const char *defaultVal, int defaultValLen)
{
    UNUSED(keyLen);
    UNUSED(defaultValLen);
    string sKey(key);
    string val(NULL == defaultVal ? "" : defaultVal);
    val = EseConfig::getString(sKey, val);
    int err = memcpy_s(buff, buffLen, val.c_str(), val.length());
    if (err != EOK) {
        TMS_LOG_E(g_tag, "%s: memcpy_s err. ret:%d", __FUNCTION__, err);
    }
}
/** Functions called by C [end]*/


EseConfig::EseConfig()
{
    string config_path = findConfigPath();
    CHECK(config_path != "");
    config_.parseFromFile(config_path);
}

EseConfig::~EseConfig()
{
}

EseConfig& EseConfig::getInstance()
{
    static EseConfig theInstance;
    return theInstance;
}

bool EseConfig::hasKey(const std::string& key)
{
    return getInstance().config_.hasKey(key);
}

std::string EseConfig::getString(const std::string& key)
{
    return getInstance().config_.getString(key);
}

std::string EseConfig::getString(const std::string& key,
                                 std::string default_value)
{
    if (hasKey(key)) {
        return getString(key);
    }
    return default_value;
}

unsigned EseConfig::getUnsigned(const std::string& key)
{
    return getInstance().config_.getUnsigned(key);
}

unsigned EseConfig::getUnsigned(const std::string& key,
                                unsigned default_value)
{
    if (hasKey(key)) {
        return getUnsigned(key);
    }
    return default_value;
}

std::vector<uint8_t> EseConfig::getBytes(const std::string& key)
{
    return getInstance().config_.getBytes(key);
}

void EseConfig::clear()
{
    getInstance().config_.clear();
}
