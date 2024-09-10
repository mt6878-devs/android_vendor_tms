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

#ifndef PHTMSSPILIB_COSDL_I2C_H
#define PHTMSSPILIB_COSDL_I2C_H

#ifdef USED_COS_I2C_DL

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* DESCRIPTION
 * Check and download eSE COS.
 * Note: this function only can be called by nfcHal process.
 *
 * RETURN VALUE
 * return 0 (ESESTATUS_SUCCESS) download COS successfully,  or has been latest version,
 *        else return error code(eg. ESESTATUS_FAILED).
 */
int16_t eseCosDownloadI2C();

#ifdef __cplusplus
};
#endif

#endif // USED_COS_I2C_DL
#endif // PHTMSSPILIB_COSDL_I2C_H
