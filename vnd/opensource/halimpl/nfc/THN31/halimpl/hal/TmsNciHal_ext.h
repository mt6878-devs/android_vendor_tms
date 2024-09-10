/*
 * Copyright (C) 2012-2020 NXP Semiconductors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

#ifndef PHTMSNCIHAL_EXT_H
#define PHTMSNCIHAL_EXT_H

#include <TmsNciHal.h>
#include <TmsNciHal_dta.h>
#include <string.h>
#define NCI_MT_CMD 0x20
#define NCI_MT_RSP 0x40
#define NCI_MT_NTF 0x60
#define NCI_MSG_CORE_RESET           0x00
#define NCI_MSG_CORE_INIT            0x01

void tmsNciHalExtInit(void);
NFCSTATUS tmsNciHalProcessExtRsp(uint8_t *pNtf, uint16_t *pLen);
NFCSTATUS tmsNciHalSendExtCmd(uint16_t cmdLen, uint8_t *pCmd);
NFCSTATUS tmsNciHalWriteExt(uint16_t *pCmdLen, uint8_t *pCmdData,
                                uint16_t *pRspLen, uint8_t *pRspData);
NFCSTATUS tmsNciHalExtSendSramConfigToFlash();

#define UINT8_TO_STREAM(p, u8) \
    { *(p)++ = (uint8_t)(u8); }


#endif /* PHTMSNCIHAL_EXT_H */
