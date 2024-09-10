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

#ifndef TMS_ESE_CONFIG_C_H
#define TMS_ESE_CONFIG_C_H

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_TMS_SE_HAL_LOGLEVEL "TMS_SE_HAL_LOGLEVEL"

#define NAME_TMS_ESE_DEV_NODE "TMS_ESE_DEV_NODE"
#define NAME_TMS_NFC_DEV_NODE "TMS_NFC_DEV_NODE"

#define NAME_TMS_NFC_FW_NAME "TMS_FW_NAME"
#define NAME_TMS_SEC_NFC_FW_NAME "TMS_SEC_FW_NAME"

#define NAME_TMS_NFC_BL_NAME "TMS_NFC_BL_NAME"

#define NAME_TMS_ESE_COS_NAME "TMS_ESE_COS_NAME"
#define NAME_TMS_SEC_ESE_COS_NAME "TMS_SEC_ESE_COS_NAME"

#define NAME_TMS_ESE_COS_PATCH_NAME "TMS_ESE_COS_PATCH_NAME"
#define NAME_TMS_ESE_COS_PATCH_0001020000_NAME "TMS_ESE_COS_PATCH_0001020000_NAME"

#define NAME_TMS_PH_WRITE_RETRY_CNT "TMS_PH_WRITE_RETRY_CNT"
#define NAME_TMS_PH_WRITE_TIME_GAP "TMS_PH_WRITE_TIME_GAP"
#define NAME_TMS_PH_READ_RETRY_CNT "TMS_PH_READ_RETRY_CNT"
#define NAME_TMS_PH_READ_TIME_GAP "TMS_PH_READ_TIME_GAP"
#define NAME_TMS_IFSD "TMS_IFSD"
#define NAME_TMS_MAX_BLK_RETRY_CNT "TMS_MAX_BLK_RETRY_CNT"
#define NAME_TMS_MAX_WTX_CNT "TMS_MAX_WTX_CNT"
#define NAME_ENABLE_VEN_TOGGLE "ENABLE_VEN_TOGGLE"

#define NAME_TMS_NFC_WATCH_DOG_TIMEOUT "TMS_NFC_WATCH_DOG_TIMEOUT"
#define NAME_TMS_T1_READ_TIMEOUT "TMS_T1_READ_TIMEOUT"

#define NAME_TMS_NFCC_ERASURE_PROTECTION "TMS_NFCC_ERASURE_PROTECTION"

// If FW_DL_CHK_VER is 0, always DL FW, ignore FW version.
// If FW_DL_CHK_VER is 1, FW DL only new FW version is different with old version.
#define NAME_FW_DL_CHK_VER "FW_DL_CHK_VER"
// If COS_PTH_VER_CHK_DIFF is 1, if vtp version is different from chip, DL COS patch.
// If COS_PTH_VER_CHK_DIFF is 0, only if vtp version is larger than chip, DL COS patch.
#define NAME_COS_PTH_VER_CHK_DIFF "COS_PTH_VER_CHK_DIFF"

unsigned int ConfigGetUnsigned(const char *key, int keyLen, const unsigned int defaultVal);
void ConfigGetString(char *buff, int buffLen,
                     const char *key, int keyLen,
                     const char *defaultVal, int defaultValLen);

#ifdef __cplusplus
}
#endif

#endif
