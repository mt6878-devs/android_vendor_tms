/*
 * Copyright (c) Tsingteng MicroSystem Co., Ltd. 2021. All rights reserved.
 */
/******************************************************************************
 * Copyright (c) 2021-2022 Tsingteng MicroSystem
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
 *****************************************************************************/

#ifndef TMSSPILIB_DL_NCI_UTILS_H
#define TMSSPILIB_DL_NCI_UTILS_H

#if defined (USE_TMS_NFC) || defined (USE_C1)
    #include <semaphore.h>
#endif

#include "tmsCommon.h"
#include <string>
using std::string;

#define NCI_MT_RSP 0x40
#define NCI_OID_MASK 0x3F
#define NCI_MSG_CORE_RESET 0x00
#define NCI_MT_NTF 0x60
#define CORE_RESET_TRIGGER_TYPE_CORE_RESET_CMD_RECEIVED 0x02
#define CORE_RESET_TRIGGER_TYPE_POWERED_ON              0x01
#define NCI_MSG_CORE_RESET           0x00
#define NCI_MSG_CORE_INIT            0x01
#define NCI_MT_MASK                  0xE0
#define NCI_OID_MASK                 0x3F
#define FW_DBG_REASON_AVAILABLE     (0xA3)

#define NFCC_BL_B206 (0x0000B206)
#define NFCC_BL_B400 (0x0000B400)

#define APDU_GET_OTP "004A050000"
#define APDU_GET_BL_VER "004A020000"

void *GetNfccChipInfoFromFWThread(void *arg);

ESESTATUS execNciCmdChkRes(const char *cmd, int cmdStrLen, void *arg);
void *GetNfccChipInfoFromBLThread(void *arg);

bool NfccSoftReset();
#if defined (USE_TMS_NFC) || defined (USE_C1)
bool EseSoftReset(void *context, sem_t *pRxSemaphore,
                  uint8_t *pReadEnable, uint8_t *pReadThreadBusy,
                  uint8_t *pWriteEnable, uint8_t *pWriteThreadBusy);
#else
bool EseSoftReset();
#endif

bool GetFixedI2cFlag(void);
void SetFixedI2cFlag(bool value);

#endif // TMSSPILIB_DL_NCI_UTILS_H
