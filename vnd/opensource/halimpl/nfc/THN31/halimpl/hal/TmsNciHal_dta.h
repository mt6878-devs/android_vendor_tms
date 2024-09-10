/*
* Copyright (C) 2012-2014 NXP Semiconductors
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

#ifndef PHTMSNCIHAL_DTA_H
#define PHTMSNCIHAL_DTA_H

#include <TmsNciHal_utils.h>
/* DTA Control structure */
typedef struct TmsDtaControl {
    uint8_t dtaCtrlFlag;
    uint16_t dtaPatternNo;
    uint8_t dtaT1tFlag;
} TmsDtaControl_t;

void tmsEnableDtaMode(uint16_t patternNo);
void tmsDisableDtaMode(void);
NFCSTATUS tmsDtaIsEnable(void);
void tmsDtaT1TEnable(void);
NFCSTATUS tmsNHalDtaUpdate(uint16_t *pCmdLen, uint8_t *pCmdData,
                              uint16_t *pRspLen, uint8_t *pRspData);

#endif /* PHTMSNCIHAL_DTA_H */
