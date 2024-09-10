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

#ifndef PHTMSNCIHAL_ADAPTATION_H
#define PHTMSNCIHAL_ADAPTATION_H

#include <hardware/nfc.h>

#include <NfcTypes.h>

typedef struct Thn31Dev {
    struct nfc_nci_device nciDevice;

    /* Local definitions */
} Thn31Dev_t;

/* tms for aidl */
#define HAL_NFC_HCI_NETWORK_RESET 7

bool *getNfcDebugEnabled(void);

/* TMS HAL functions */
NFCSTATUS tmsNciHalOpen(nfc_stack_callback_t *pCallback,
                        nfc_stack_data_callback_t *pDataCallback);
NFCSTATUS tmsNciHalMinOpen();
NFCSTATUS tmsNciHalWrite(uint16_t dataLen, const uint8_t *pData);
int tmsNciHalWriteInternal(uint16_t dataLen, const uint8_t *pData);
int tmsNciHalCoreInitialized(uint16_t coreInitRspLen, uint8_t *pCoreInitRspParams);
int tmsNciHalPreDiscover(void);
NFCSTATUS tmsNciHalClose(bool);
int tmsNciHalConfigDiscShutdown(void);
int tmsNciHalControlGranted(void);
int tmsNciHalPowerCycle(void);
int tmsNciHalIoctl(long arg, void *pData);
void tmsNciHalDoFactoryReset(void);
bool isTmsChip();
void tmsNciHalConfigAidlHalService(void);
#endif /* PHTMSNCIHAL_ADAPTATION_H */
