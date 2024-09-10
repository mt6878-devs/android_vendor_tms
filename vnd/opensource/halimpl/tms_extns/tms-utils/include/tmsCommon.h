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

#ifndef TMS_COMMON_H
#define TMS_COMMON_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UNUSED
#define UNUSED(arg) (void)(arg)
#endif

#ifndef EOK
#define EOK 0
#endif

typedef enum {
    ESESTATUS_SUCCESS = 0,
    ESESTATUS_FAILED = 1,
    ESESTATUS_INVALID_PARAMETER = 2,
    ESESTATUS_NOT_INITIALISED = 3,
    ESESTATUS_ALREADY_INITIALISED = 4,
    ESESTATUS_FEATURE_NOT_SUPPORTED = 5,
    ESESTATUS_CONNECTION_SUCCESS = 6,
    ESESTATUS_CONNECTION_FAILED = 7,
    ESESTATUS_BUSY = 8,
    ESESTATUS_CLOSED = 9,
    ESESTATUS_OPENED = 10,
    ESESTATUS_MEM_EXCEPTION = 11,       // memory exception
    ESESTATUS_PH_IO = 12,               // write or read failed, return res < 0
    ESESTATUS_PH_IOR_INVALID_DATA = 13, // read invalid data
    ESESTATUS_RES_EXCEPTION = 14,       // resource exception
    ESESTATUS_PARITY_ERROR = 15,        // LRC or CRC error
    ESESTATUS_INVALID_PCB = 16,         // not I/R/S PCB
    ESESTATUS_INVALID_T1_LEN = 17,      // T1 INF length not matched
    ESESTATUS_FATAL_ERROR = 18,         // fatal error, need chip do hard reset to recovery.
    ESESTATUS_COS_RCV_SUCCESS = 19,     // COS DL is failed, but recovery is successfull.
    ESESTATUS_COS_RCV_FAILED = 20,      // Both COS DL and recovery are failed.
    ESESTATUS_DOWNLOAD_EXCESS = 21,     // Download FW/BL too frequently.
    ESESTATUS_UNKNOWN_ERROR,
} ESESTATUS;

// 7816-4 APDU data, an ATR RSP data, or a T=1 data(cmd or rsp)
typedef struct {
    uint16_t len;   // length of the buffer
    uint8_t *pData; // pointer to a buffer
} SeData;

#ifdef __cplusplus
}
#endif

#endif // TMS_COMMON_H

