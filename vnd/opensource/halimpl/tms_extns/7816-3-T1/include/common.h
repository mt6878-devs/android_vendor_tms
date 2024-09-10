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

#ifndef TMS_7816_3_T1_COMMON_H
#define TMS_7816_3_T1_COMMON_H

#include <errno.h>
#include <stdint.h>
#include "configC.h"

#include "tmsCommon.h"
#include "T1.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ESESTATUS EseLibStatus; // Indicate if 7816-3-T1 Lib is opened or closed
    int devHandle; // dev fd, such as, open /dev/tms_ese
    bool isReadDone; // read tryagain when isReadDone is false and sofCnt < maxReadRetryCnt
    uint32_t maxWriteRetryCnt; // ph write error retry counts.
    uint32_t writeRetryTime;   // if ph write error, sleep writeRetryTime microsecond and retry.
    uint32_t maxReadRetryCnt;  // ph read error retry counts.
    uint32_t readRetryTime;    // if ph read error, sleep readRetryTime microsecond and retry.
    uint32_t readTimeout;      // if ph read error, within the max timeout and retry .

    // T=1 protocol retry rules.
    // t1SendSFrame, t1SendIFrame or t1SendRFrame returns success status.
    // the max count of retry times when send frame error.
    uint32_t maxRecoveryCnt;

    // t1SendSFrame, t1SendIFrame or t1SendRFrame returns error status.
    uint8_t maxBlkRetryCnt;

    // max wtx request counter.
    uint16_t maxWTXCnt;

    // Mark resync recovery.
    // 1. If this flag is true, resync will not be send for recvery,
    //    until hard reset or T=1 protocol closed.
    // 2. isResyncRecoveryFlag will be reset to false:
    // 2.1 I/R or S(WTX response) t1DecodeFrame is ok
    // 2.2 Chip hard reset is ok
    // 2.3 T=1 protocol closed, such as, SEApi: seClose called.
    bool isResyncRecoveryFlag;

    uint16_t maxIFSD; // The interface device max IFS size.
    // Recommended value is 0x0102(258), apduRspMax(256) + sw1sw2(2)
    uint16_t maxIFSC; // The card max IFS size.
    // Recommended value is 0x0105(261), apduH(5) + 255(0xFF) + apduE(1)
    bool isT1ExtHdrLen;
    uint8_t seqNumDevice; // T1 device sequence number
    uint8_t seqNumCard;   // T1 card sequence number
    bool isSeqNumDR;      // true, seqNumDevice has been recovered
    T1TransceiveState nextT1State;

    /** T1 related **/
    T1Params *pT1Params;
} SEContext;

#ifdef __cplusplus
}
#endif

#endif // TMS_7816_3_T1_COMMON_H
