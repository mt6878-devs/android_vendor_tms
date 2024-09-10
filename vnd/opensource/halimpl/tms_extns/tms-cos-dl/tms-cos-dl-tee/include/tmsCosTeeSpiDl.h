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


#ifndef _PHTMSSPILIB_COSDL_TEE_SPI_H_
#define _PHTMSSPILIB_COSDL_TEE_SPI_H_

#include <stdint.h>

#ifndef UNUSED
    #define UNUSED(arg) (void)(arg)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* DESCRIPTION
 * Check and download eSE COS.
 *
 * @params    searchNfcDir - true, search patch file in /data/vendor/nfc/,
              or false search patch file in /data/vendor/secure_element/.
 *
 * RETURN VALUE
 * return 0 (ESESTATUS_SUCCESS) download COS successfully,  or has been latest version,
 *        else return error code(eg. ESESTATUS_FAILED).
 */
int16_t eseCosTeeDl(bool searchNfcDir);

/* DESCRIPTION
 * Check and download eSE COS by COS patch.
 *
 * @params    searchNfcDir - true, search patch file in /data/vendor/nfc/,
              or false search patch file in /data/vendor/secure_element/.
 *
 * RETURN VALUE
 * return 0 (ESESTATUS_SUCCESS) download COS successfully,  or has been latest version,
 *        else return error code(eg. ESESTATUS_FAILED).
 */
int16_t eseCosTeeDlPth(bool searchNfcDir);

#ifdef __cplusplus
};
#endif

#endif //_PHTMSSPILIB_COSDL_TEE_SPI_H_
