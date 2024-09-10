/******************************************************************************
 *
 *  Copyright 2015-2018,2020-2021 NXP
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

#define LOG_TAG "TmsHal"
#include "TmsNfcCapability.h"
#include <TmsLog.h>
#include <TmsConfig.h>

capability *capability::instance = NULL;
NfcChipType capability::chipType = thn31;
NfcFeatureList gNfcFL;

NfcFeatureList *getNfcFL(void)
{
    return &gNfcFL;
}

capability::capability() {}

capability *capability::getInstance() {
    if (NULL == instance) {
        instance = new capability();
    }
    return instance;
}

NfcChipType capability::processChipType(uint8_t *pMsg, uint16_t msgLen) {
    if ((pMsg != NULL) && (msgLen != 0)) {
        if (pMsg[0] == 0x60 && pMsg[1] == 0x00) {
            if (((pMsg[msgLen - 3] & 0xF0) == 0xC0) ||
                ((pMsg[msgLen - 3] & 0xF0) == 0xD0)) {
                chipType = thn31;
            }
        } else {
            ALOGD("%s Wrong msgLen. Setting Default ChiptType thn31", __func__);
            chipType = thn31;
        }
    }
    ALOGD("%s Product : %s", __func__, product[chipType]);
    return chipType;
}

uint32_t capability::getFWVersionInfo(uint8_t *pMsg, uint16_t msgLen) {
    uint32_t versionInfo = 0;
    if ((pMsg != NULL) && (msgLen != 0)) {
        if (pMsg[0] == 0x00) {
            versionInfo = pMsg[offsetFwRomCodeVersion] << 16;
            versionInfo |= pMsg[offsetFwMajorVersion] << 8;
            versionInfo |= pMsg[offsetFwMinorVersion];
        }
    }
    return versionInfo;
}
