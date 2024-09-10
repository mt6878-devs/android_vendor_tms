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

#ifndef TMSSPILIB_TMS_REE_DL_COMMON_UTILS_H
#define TMSSPILIB_TMS_REE_DL_COMMON_UTILS_H

#include <sys/ioctl.h>

#include "SEApi.h"
#include "eseConfig.h"

#if defined (USE_TMS_NFC) || defined (USE_C1)
    // copied from nfc kernel [start]
    #define THN31_NFCC_MAGIC 0xE9
    #define THN31_NFCC_SET_PWR    _IOW(THN31_NFCC_MAGIC, 0x01, long)

    #define THN31_ESE_MAGIC 0xEA
    #define THN31_ESE_SET_PWR _IOW(THN31_ESE_MAGIC, 0x01, long)
    // copied from nfc kernel [end]
#else
    #define THN31_NFCC_MAGIC 0xE9
    #define THN31_NFCC_SET_PWR    _IOW(THN31_NFCC_MAGIC, 0x01, unsigned int)

    #define THN31_ESE_MAGIC 0xEA
    #define THN31_ESE_SET_PWR _IOW(THN31_ESE_MAGIC, 0x01, unsigned int)
#endif

#define VEN_SET_WAIT_TIME_20MS 20000
#define VEN_SET_WAIT_TIME_7MS 7000

bool IoctlNfc(long arg);

ESESTATUS openT1(SeInitMode initMode);

ESESTATUS initT1();

ESESTATUS deInitT1();

bool CloseT1();

void *VtpDownloadThread(void *arg);
void *GetSeBlVerThread(void *arg);
void *TryJumpToCosThread(void *arg);

bool ChipHardReset();
bool ChipHardReset(uint32_t dlDelay, uint32_t upDelay);

int GetNfccFwDlTime(void);
void SetNfccFwDlTime(int time);
int GetNfccBlDlTime(void);
void SetNfccBlDlTime(int time);
int GetSeCosI2cDlTime(void);
void SetSeCosI2cDlTime(int time);
#endif // TMSSPILIB_TMS_REE_DL_COMMON_UTILS_H
