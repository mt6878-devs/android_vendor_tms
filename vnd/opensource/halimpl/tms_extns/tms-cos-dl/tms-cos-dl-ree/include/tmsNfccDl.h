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

#ifndef PHTMSSPILIB_COSDL_NFCC_H
#define PHTMSSPILIB_COSDL_NFCC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* DESCRIPTION
 * Check and download NFCC FW
 *
 * RETURN VALUE
 * return 0 (ESESTATUS_SUCCESS) download FW successfully or has been latest version,
 *        else return error code(eg. ESESTATUS_FAILED).
 */
int16_t NfccFwDownload();

/* DESCRIPTION
 * Check and download NFCC BL
 *
 * RETURN VALUE
 * return 0 (ESESTATUS_SUCCESS) download NFCC BL successfully or has been latest version,
 *        else return error code(eg. ESESTATUS_FAILED).
 */
int16_t NfccBlDownload();

void CheckFlashRequired(uint8_t *fwUpdateReq,
                        uint32_t *wFwVer,
                        uint32_t *wFwVerRsp);

#ifdef __cplusplus
};
#endif

#endif // PHTMSSPILIB_COSDL_NFCC_H
