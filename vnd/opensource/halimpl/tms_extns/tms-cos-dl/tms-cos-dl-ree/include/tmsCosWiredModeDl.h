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

#ifndef _PHTMSSPILIB_COSDL_WIRED_SE_H_
#define _PHTMSSPILIB_COSDL_WIRED_SE_H_

#ifdef USED_COS_WIRED_SE_DL

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* DESCRIPTION
 * Check and download eSE COS by COS patch on wired SE(APDU Gate).
 *
 * RETURN VALUE
 * return 0 (ESESTATUS_SUCCESS) download COS successfully,  or has been latest version,
 *        else return error code(eg. ESESTATUS_FAILED).
 */
int16_t eseCosWiredSeDlPth();

#ifdef __cplusplus
};
#endif

#endif //USED_COS_WIRED_SE_DL
#endif //_PHTMSSPILIB_COSDL_WIRED_SE_H_