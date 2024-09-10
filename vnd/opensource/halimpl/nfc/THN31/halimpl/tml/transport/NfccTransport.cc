/******************************************************************************
 *
 *  Copyright 2020-2021 NXP
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

#include <NfccTransport.h>

int NfccTransport::nfccReset(__attribute__((unused)) void *pDevHandle,
                             __attribute__((unused)) NfccResetType type) {
    return NFCSTATUS_SUCCESS;
}

int NfccTransport::eseReset(__attribute__((unused)) void *pDevHandle,
                            __attribute__((unused)) EseResetType type) {
    return NFCSTATUS_SUCCESS;
}
int NfccTransport::eseGetPower(__attribute__((unused)) void *pDevHandle,
                               __attribute__((unused)) uint32_t level) {
    return NFCSTATUS_SUCCESS;
}

bool NfccTransport::flushdata(__attribute__((unused)) pTmlNfcConfig_t pConfig) {
    return true;
}