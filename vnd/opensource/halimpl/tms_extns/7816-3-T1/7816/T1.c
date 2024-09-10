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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include "T1.h"
#include "tmslog.h"
#include "phDriver.h"
#include "SEApi.h"

#if defined (USE_TMS_NFC) || defined (USE_C1)
    #include "tmsSecureString.h"
#else
    #include "securec.h"
#endif

#ifdef DBG_LEVEL_STACK
    #include "callstack.h"
#endif

#ifdef TAG
#undef TAG
#endif
#define TAG g_tag

static const char g_tag[] = "T1protocol";

AtrInfo gAtr = {
    .len = 0x13,
    .vendorID = {0x00},
    .dllIC = 0x01,
    .bgt = {0x00, 0x01},
    .bwt = {0x03, 0xE8},
    .maxFreq = {0x4E, 0x20},
    .checksum = 0x00,
    .defaultIFSC = 0xFE,
    .numChannels = 0x01,
    .maxIFSC = {0x00, 0xFF},
    .capbilities = {0x00, 0x14},
};

static T1RecvBuffListT *gHead = NULL, *gCurrent = NULL;
static uint32_t gRecvTotalLen = 0;


static ESESTATUS transceiveProcess();
static ESESTATUS responseProcess();
static ESESTATUS t1SendSFrame();
static ESESTATUS t1SendIFrame();
static ESESTATUS t1SendRFrame();
static uint8_t t1CalculateLRC(uint8_t *pData, uint16_t offset, uint16_t len);
static uint16_t t1CalculateCRC(uint8_t *pData, uint16_t offset, uint16_t len);
static void t1SetEpilogue(uint8_t *pData, uint16_t offset, uint16_t len);
static ESESTATUS t1ChkEpilogue(uint8_t *pData, uint16_t offset, uint16_t len);
static ESESTATUS t1Read(uint8_t **ppData, uint16_t *pDataLen);
static ESESTATUS t1DecodeFrame(uint8_t *pData, uint16_t dataLen);
static uint8_t getHeaderLen(bool containsEpilogue);
static uint8_t setInfoLen(uint8_t *pData, uint16_t infoLen);
static uint16_t getInfoLen(uint8_t *pData, uint16_t dataLen);
static ESESTATUS decodeATR(uint8_t *pData, uint16_t dataLen, bool isCIP);
static ESESTATUS initIFrameFromCmdApdu(uint8_t *pCmdApdu, uint16_t cmdLen);
static void DeInitIFrameFromCmdApdu();
static void FreeTxPartMem();
static void ResetForResyncRsp();
static ESESTATUS doT1RecoveryAtUndecode();
static ESESTATUS doT1RecoveryAtDecodeIFrame();
static ESESTATUS doT1RecoveryAtDecodeSFrame();
static ESESTATUS doT1RecoveryAtDecodeRFrame();
static ESESTATUS t1RecvDataStoreInList(uint8_t *pData, uint16_t dataLen,
                                       uint16_t totalLen);
static void T1RecvDataListDestroy();
static void atrElements(const char *tag, uint16_t tagLen);
static void ResetATR();


/******************************************************************************
 * @Function     TransceiveProcess
 *
 * @Description  This internal function is used to
 *              1. Send the raw data received from application after computing LRC
 *              2. Receive the the response data from ESE, decode, process and
 *                 store the data.
 * @Returns      On success return ESESTATUS_SUCCESS or else ESESTATUS error.
 *
 ******************************************************************************/
static ESESTATUS transceiveProcess()
{
    ESESTATUS status = ESESTATUS_SUCCESS;
    SEContext *pEseCtx = GetEseCtx();
    pEseCtx->isReadDone = false;

    while (STATE_IDEL != pEseCtx->nextT1State) {
        switch (pEseCtx->pT1Params->txFrameType) {
            case IFRAME: {
                status = t1SendIFrame();
                break;
            }

            case RFRAME: {
                status = t1SendRFrame();
                break;
            }

            case SFRAME: {
                status = t1SendSFrame();
                break;
            }

            default: {
                status = ESESTATUS_INVALID_PARAMETER;
                TMS_LOG_W(g_tag, "%s : invalid T1 frame type: %d",
                          __FUNCTION__, pEseCtx->pT1Params->txFrameType);
            }
        }

        if (ESESTATUS_SUCCESS == status) {
#ifdef DBG_LEVEL_STACK
            SetDumpStackFlag(false);
#endif
            pEseCtx->pT1Params->blkRetryCnt = 0;
            status = responseProcess();
            // Do not need handle the status value
        } else {
            if (((ESESTATUS_PH_IO != status) && (ESESTATUS_MEM_EXCEPTION != status))
                    || (pEseCtx->pT1Params->blkRetryCnt >= pEseCtx->maxBlkRetryCnt)) {
                TMS_LOG_E(g_tag, "%s: send frame failed, frameType = %d, status = %d, retryCnt = %d",
                          __FUNCTION__, pEseCtx->pT1Params->txFrameType, status, pEseCtx->maxBlkRetryCnt);
                pEseCtx->nextT1State = STATE_IDEL;
                break;
            } else {
                // resend
                pEseCtx->pT1Params->blkRetryCnt++;
            }
        }
    }

    return status;
}

/******************************************************************************
 * @Function     responseProcess
 *
 * @Description  This internal function is used to
 *              1. Check the LRC
 *              2. Decoding of received frame of data.
 * @Returns      On success return ESESTATUS_SUCCESS or else ESESTATUS error.
 *
 ******************************************************************************/
static ESESTATUS responseProcess()
{
    ESESTATUS status = ESESTATUS_SUCCESS;
    uint16_t dataLen = 0;
    uint8_t *pData = NULL;

    // pData should not be free, it equals to gpDataRx.
    status = t1Read(&pData, &dataLen);
    if (ESESTATUS_SUCCESS == status) {
        if (dataLen < 4) { // 4 len: NAD + PCD + LEN + LRC/CRC
            return ESESTATUS_FAILED;
        }
        status = t1ChkEpilogue(pData, 0, dataLen);
        if (status == ESESTATUS_SUCCESS) {
            status = t1DecodeFrame(pData, dataLen);
            if (ESESTATUS_SUCCESS == status) {
            }
        } else {
            // NOTE1: I/R-recvInvalid by the seqNum, not LRC or CRC error, see t1DecodeFrame.
            // NOTE2: S-recvInvalid is only one case, that is LRC or CRC error.
            doT1RecoveryAtUndecode();
        }
    } else if (ESESTATUS_FATAL_ERROR == status) {
        GetEseCtx()->nextT1State = STATE_IDEL;
    } else {
        doT1RecoveryAtUndecode();
    }

    return status;
}

/******************************************************************************
 * @Function         t1SendSFrame
 *
 * @Description      This internal function is called to send S-frame with all
 *                   7816-3 header and epilogue field.
 *
 * @Returns          On success return ESESTATUS_SUCCESS or else ESESTATUS error.
 *
 ******************************************************************************/
static ESESTATUS t1SendSFrame()
{
    ESESTATUS status = ESESTATUS_SUCCESS;
    int ret = -1;
    uint8_t frameLen = 0;
    uint8_t *pFramebuff = NULL;
    SEContext *pEseCtx = GetEseCtx();

    if (SFRAME_TYPE_INVALID == pEseCtx->pT1Params->txSubSFrameType) {
        TMS_LOG_W(g_tag, "%s : invalid S-block frame type: %d",
                  __FUNCTION__, pEseCtx->pT1Params->txSubSFrameType);
        return ESESTATUS_INVALID_PARAMETER;
    } else {
        frameLen = getHeaderLen(true);
    }

    switch (pEseCtx->pT1Params->txSubSFrameType) {
        case RESYNC_REQ: {
            pFramebuff = (uint8_t *)calloc(1, frameLen);
            if (NULL == pFramebuff) {
                goto cleanup;
            }
            pFramebuff[T1_PCB_OFFSET] = (T1_S_BLOCK_REQ_MASK | RESYNC_REQ);
            break;
        }

        case IFS_REQ: {
            uint8_t lenOffset = 0, infoLen = 0x02;
            frameLen += infoLen;
            pFramebuff = (uint8_t *)calloc(1, frameLen);
            if (NULL == pFramebuff) {
                goto cleanup;
            }
            pFramebuff[T1_PCB_OFFSET] = (T1_S_BLOCK_REQ_MASK | IFS_REQ);
            lenOffset = setInfoLen(pFramebuff, infoLen);
            // pFramebuff[lenOffset + 1]，maxIFSD >> 8
            pFramebuff[lenOffset + 1] = (uint8_t)(pEseCtx->maxIFSD >> 8);
            // pFramebuff[lenOffset + 2]，>maxIFSD & 0xFF
            pFramebuff[lenOffset + 2] = (uint8_t)(pEseCtx->maxIFSD & 0xFF);
            break;
        }

        case IFS_RSP: {
            uint8_t lenOffset = 0, infoLen = 0;
            infoLen = getInfoLen(GetDataRx(), GetDataRxLen());
            frameLen += infoLen;
            pFramebuff = (uint8_t *)calloc(1, frameLen);
            if (NULL == pFramebuff) {
                goto cleanup;
            }
            pFramebuff[T1_PCB_OFFSET] = (T1_S_BLOCK_REQ_MASK | IFS_REQ);
            lenOffset = setInfoLen(pFramebuff, infoLen);
            int err = memcpy_s(&pFramebuff[lenOffset + 1], frameLen - lenOffset,
                &(GetDataRx()[lenOffset + 1]), infoLen);
            if (err != EOK) {
                TMS_LOG_E(g_tag, "%s: memcpy_s err. ret:%d", __FUNCTION__, err);
            }
            break;
        }

        case ABORT_REQ: {
            pFramebuff = (uint8_t *)calloc(1, frameLen);
            if (NULL == pFramebuff) {
                goto cleanup;
            }
            pFramebuff[T1_PCB_OFFSET] = (T1_S_BLOCK_REQ_MASK | ABORT_REQ);
            break;
        }

        //SE COS do not implement S_ABORT_RSP receive and R(0) response.
        //And we think this implementation is right.
        //So, if Device received ABORT_REQ(RSP) , release the resource and exit.
        //ABORT_RSP should not be sent to SE.
        /*
        case ABORT_RSP: {
          pFramebuff = (uint8_t *)calloc(1, frameLen);
          if (NULL == pFramebuff) {
            goto cleanup;
          }
          pFramebuff[T1_PCB_OFFSET] = (T1_S_BLOCK_REQ_MASK | ABORT_RSP);
          break;
        }
        */

        case WTX_RSP: {
            uint8_t lenOffset = 0, infoLen = 1;
            frameLen += infoLen;
            pFramebuff = (uint8_t *)calloc(1, frameLen);
            if (NULL == pFramebuff) {
                goto cleanup;
            }
            pFramebuff[T1_PCB_OFFSET] = (T1_S_BLOCK_REQ_MASK | WTX_RSP);
            lenOffset = setInfoLen(pFramebuff, infoLen);
            pFramebuff[lenOffset + 1] = pEseCtx->pT1Params->wtxInfo;
            break;
        }

        case CIP_REQ: {
            /* Note:T1 CIP_REQ for ATR, the frame's INFO length should be 1 byte,
             *      do not support 2 bytes length.
             *      And epiLogue field should use LRC (1 byte).
             *      See getHeaderLen function.
             */
            pFramebuff = (uint8_t *)calloc(1, frameLen);
            if (NULL == pFramebuff) {
                goto cleanup;
            }
            pFramebuff[T1_PCB_OFFSET] = (T1_S_BLOCK_REQ_MASK | CIP_REQ);
            if (GetEseCtx()->isT1ExtHdrLen) {
                GetEseCtx()->isT1ExtHdrLen = false;
            }
            break;
        }

        case PROP_END_APDU_REQ: {
            pFramebuff = (uint8_t *)calloc(1, frameLen);
            if (NULL == pFramebuff) {
                goto cleanup;
            }
            pFramebuff[T1_PCB_OFFSET] = (T1_S_BLOCK_REQ_MASK | PROP_END_APDU_REQ);
            break;
        }

        case HARD_RESET_REQ: {
            break;
        }

        case ATR_REQ: {
            pFramebuff = (uint8_t *)calloc(1, frameLen);
            if (NULL == pFramebuff) {
                goto cleanup;
            }
            pFramebuff[T1_PCB_OFFSET] = (T1_S_BLOCK_REQ_MASK | ATR_REQ);
            break;
        }

        default:
            TMS_LOG_W(g_tag, "%s : invalid S-block frame type: %X",
                      __FUNCTION__, pEseCtx->pT1Params->txSubSFrameType);
    }

    if (NULL == pFramebuff) {
        TMS_LOG_W(g_tag, "%s : invalid or unsupported S-block frame type: %X",
                  __FUNCTION__, pEseCtx->pT1Params->txSubSFrameType);
        goto cleanup;
    }

    pFramebuff[T1_NAD_OFFSET] = NAD_D2C;
    // caculate LRC or CRC and set it to pFramebuff
    t1SetEpilogue(pFramebuff, 0, frameLen);

    ret = PhWrite(pFramebuff, frameLen);
    if (ret == -1) {
        status = ESESTATUS_PH_IO;
        // length 6
        printHexPacket(g_tag, strlen(g_tag), "TxFail", 6, pFramebuff, frameLen);
        TMS_LOG_E(g_tag, "write fail: %s, errno = %d, ret = %d", __FUNCTION__, errno, ret)
    } else {
        // length 2
        printHexPacket(g_tag, strlen(g_tag), "Tx", 2, pFramebuff, frameLen);
    }

cleanup:
    if (NULL == pFramebuff) {
        TMS_LOG_E(g_tag, "%s failed, frame buffer malloc memory failed", __FUNCTION__);
        status = ESESTATUS_MEM_EXCEPTION;
    } else {
        free(pFramebuff);
    }

    return status;
}

/******************************************************************************
 * @Function         t1SendIFrame
 *
 * @Description      This internal function is called to send I-frame with all
 *                   7816-3 header and epilogue field.
 *
 * @Returns          On success return ESESTATUS_SUCCESS or else ESESTATUS error.
 *
 ******************************************************************************/
static ESESTATUS t1SendIFrame()
{
    ESESTATUS status = ESESTATUS_SUCCESS;
    int ret = -1;
    uint32_t frameLen = 0;
    uint8_t *pFramebuff = NULL;
    uint32_t lenOffset = 0;
    uint32_t sendLen = 0;
    uint32_t txDataOffset = 0;
    SEContext *pEseCtx = GetEseCtx();

    if (pEseCtx->pT1Params->recoveryCnt > pEseCtx->maxRecoveryCnt) {
        // Cannot execute here, fatal error
        TMS_LOG_E(g_tag, "%s fatal error, recoveryCnt = %d",
                  __FUNCTION__, pEseCtx->pT1Params->recoveryCnt);
        status = ESESTATUS_FATAL_ERROR;
        goto cleanup;
    } else  if ((pEseCtx->pT1Params->pDataTxPart != NULL)) {
        TMS_LOG_W(g_tag, "%s retry_send_part, txLenPart = %u",
                  __FUNCTION__, pEseCtx->pT1Params->txLenPart);
        // some error may be occurred, resend the pre-send data.
        pFramebuff = pEseCtx->pT1Params->pDataTxPart;
        frameLen = pEseCtx->pT1Params->txLenPart;
        goto retry_send_part;
    }

    txDataOffset = pEseCtx->pT1Params->txDataOffset;
    frameLen = (pEseCtx->isT1ExtHdrLen ? T1_EXT_HEADER_LEN : T1_HEADER_LEN)
               + ((LRC == gAtr.checksum) ? T1_LRC_LEN : T1_CRC_LEN);

    if ((pEseCtx->pT1Params->txLen - txDataOffset) > pEseCtx->maxIFSC) {
        pEseCtx->pT1Params->isDeviceChaining = true;
        sendLen = pEseCtx->maxIFSC;
        txDataOffset += pEseCtx->maxIFSC;
    } else {
        pEseCtx->pT1Params->isDeviceChaining = false;
        sendLen = pEseCtx->pT1Params->txLen - txDataOffset;
        txDataOffset += sendLen;
    }

    frameLen += sendLen;
    pFramebuff = (uint8_t *)calloc(1, frameLen);
    if (NULL == pFramebuff) {
        goto cleanup;
    }

    pFramebuff[T1_NAD_OFFSET] = NAD_D2C;
    lenOffset = setInfoLen(pFramebuff, sendLen);
    int err = memcpy_s((pFramebuff + lenOffset + 1), frameLen - lenOffset - 1,
        (pEseCtx->pT1Params->pDataTx + pEseCtx->pT1Params->txDataOffset), sendLen);
    if (err != EOK) {
        TMS_LOG_E(g_tag, "%s: memcpy_s err. ret:%d", __FUNCTION__, err);
    }
    pEseCtx->pT1Params->txDataOffset = txDataOffset;

    pEseCtx->pT1Params->pDataTxPart = pFramebuff;
    pEseCtx->pT1Params->txLenPart = frameLen;

retry_send_part:
    pFramebuff[T1_PCB_OFFSET] = 0x00;
    if (pEseCtx->pT1Params->isDeviceChaining) {
        pFramebuff[T1_PCB_OFFSET] |= T1_CHAINING_MASK;
    }
    // seqNumDevice << 6
    pFramebuff[T1_PCB_OFFSET] |= (pEseCtx->seqNumDevice << 6);

    // caculate LRC or CRC and set it to pFramebuff
    t1SetEpilogue(pFramebuff, 0, frameLen);

    ret = PhWrite(pFramebuff, frameLen);
    if (ret == -1) {
        status = ESESTATUS_PH_IO;
        // length 6
        printHexPacket(g_tag, strlen(g_tag), "TxFail", 6, pFramebuff, frameLen);
        TMS_LOG_E(g_tag, "write fail: %s, errno = %d, ret = %d", __FUNCTION__, errno, ret)
    } else {
        // length 2
        printHexPacket(g_tag, strlen(g_tag), "Tx", 2, pFramebuff, frameLen);
    }

cleanup:
    // Do not free the pFramebuff, it will be released when:
    // 1. Received IFrame successfully, or,
    // 2. Retry count > max, or,
    // 3. S_ABORT_RSP == pEseCtx->nextT1State

    if (S_ABORT_RSP == pEseCtx->nextT1State) {
        pEseCtx->nextT1State = STATE_IDEL;
        FreeTxPartMem();
    }

    return status;
}

/******************************************************************************
 * @Function         t1SendRFrame
 *
 * @Description      This internal function is called to send R-frame with all
 *                   7816-3 header and epilogue field.
 *
 * @Returns          On success return ESESTATUS_SUCCESS or else ESESTATUS error.
 *
 ******************************************************************************/
static ESESTATUS t1SendRFrame()
{
    ESESTATUS status = ESESTATUS_SUCCESS;
    int ret = -1;
    uint8_t frameLen = 0;
    uint8_t *pFramebuff = NULL;
    SEContext *pEseCtx = GetEseCtx();

    if (RNACK_INVALID_ERROR == pEseCtx->pT1Params->txSubRFrameType) {
        TMS_LOG_W(g_tag, "%s : invalid R-block frame type: %d",
                  __FUNCTION__, pEseCtx->pT1Params->txSubSFrameType);
        return ESESTATUS_INVALID_PARAMETER;
    } else {
        frameLen = getHeaderLen(true);
    }

    pFramebuff = (uint8_t *)calloc(1, frameLen);
    if (NULL == pFramebuff) {
        goto cleanup;
    }

    if (RNACK_PARITY_ERROR == pEseCtx->pT1Params->txSubRFrameType) {
        pFramebuff[1] = 0x81;
    } else if (RNACK_OTHER_ERROR == pEseCtx->pT1Params->txSubRFrameType) {
        pFramebuff[1] = 0x82;
    } else { // if (RACK == pEseCtx->pT1Params->subRFrameType) {
        pFramebuff[1] = 0x80;
    }
    // seqNumCard << 4
    pFramebuff[1] |= (pEseCtx->seqNumCard << 4);

    pFramebuff[T1_NAD_OFFSET] = NAD_D2C;
    // caculate LRC or CRC and set it to pFramebuff
    t1SetEpilogue(pFramebuff, 0, frameLen);

    ret = PhWrite(pFramebuff, frameLen);
    if (-1 == ret) {
        status = ESESTATUS_PH_IO;
        // length 6
        printHexPacket(g_tag, strlen(g_tag), "TxFail", 6, pFramebuff, frameLen);
        TMS_LOG_E(g_tag, "write fail: %s, errno = %d, ret = %d", __FUNCTION__, errno, ret)
    } else {
        // length 2
        printHexPacket(g_tag, strlen(g_tag), "Tx", 2, pFramebuff, frameLen);
    }

cleanup:
    if (NULL == pFramebuff) {
        TMS_LOG_E(g_tag, "%s failed, frame buffer malloc memory failed", __FUNCTION__);
        status = ESESTATUS_MEM_EXCEPTION;
    } else {
        free(pFramebuff);
    }

    return status;
}

/******************************************************************************
 * @Function     t1CalculateLRC
 *
 * @Description  This internal function is called calculate the LRC
 *
 * @params       pData  - data to compute the LRC over.
 *               offset - calculate start offset.
 *               len    - data length, not include the epilogue byte.
 *
 * @Returns      On success return the LRC of the data.
 *
 ******************************************************************************/
static uint8_t t1CalculateLRC(uint8_t *pData, uint16_t offset, uint16_t len)
{
    uint8_t LRC = 0;
    uint16_t i = 0;
    for (i = offset; i < len; i++) {
        LRC = LRC ^ pData[i];
    }
    return LRC;
}

/******************************************************************************
 * @Function     t1CalculateCRC
 *
 * @Description  This internal function is called calculate the CRC, see ISO/IEC 13239.
 *
 * @params       pData  - data to compute the CRC over.
 *               offset - calculate start offset.
 *               len    - data length, not include the epilogue bytes..
 *
 * @Returns      On success return the CRC of the data.
 *
 ******************************************************************************/
static uint16_t t1CalculateCRC(uint8_t *pData, uint16_t offset, uint16_t len)
{
    uint16_t crc;

    crc = (uint16_t)CRC_PRESET;

    int i, j;
    for (i = offset; i < len; i++) {
        crc = crc ^ ((unsigned short)pData[i]);
        for (j = 0; j < 8; j++) {  // j = 0; j < 8; j++
            if ((crc & 0x0001) == 0x0001) {
                crc = (crc >> 1) ^ CRC_POLYNOMIAL;
            } else {
                crc = crc >> 1;
            }
        }
    }
    crc = (uint16_t) (~crc);

    return crc;
}

/******************************************************************************
 * @Function     t1SetEpilogue
 *
 * @Description  This internal function is called caculate the LRC or CRC and
 *               set it to the data's epilogue.
 *
 * @params       pData  - data to compute the LRC or CRC over.
 *               offset - calculate start offset.
 *               len    - data length, include the epilogue byte(s).
 *
 ******************************************************************************/
static void t1SetEpilogue(uint8_t *pData, uint16_t offset, uint16_t len)
{
#ifndef LRC_CRC_0
    if ((ESE_MODE_NFCC_DL == gInitMode)
            || (ESE_MODE_ESE_DL == gInitMode)) {
        // TMS LRC for NFCC or ESE BL
        // keep offset unchanged (expected value is 0), do nothing
    } else {
        // LRC for C1
        TMS_LOG_D(g_tag, "%s adapter for C1, original offset = 0x%X, changed offset = 0x01",
                  __FUNCTION__, offset);
        offset = 1;
    }
#endif

    /* Note:T1 CIP_REQ for ATR, the frame's INFO length should be 1 byte,
     *      do not support 2 bytes length.
     *      And epiLogue field should use LRC (1 byte).
     */
    if ((LRC == gAtr.checksum) ||
        (CIP_REQ == GetEseCtx()->pT1Params->txSubSFrameType) ||
        /* || (ATR_REQ == GetEseCtx()->pT1Params->txSubSFrameType) */
        (CIP_RSP == GetEseCtx()->pT1Params->txSubSFrameType)
        /* || (ATR_RSP == GetEseCtx()->pT1Params->txSubSFrameType) */) {
        // LRC
        pData[len - 1] = t1CalculateLRC(pData, offset, len - 1);
    } else {
        // CRC
        // In APDU Transport over SPI / I2C – Public Release v1.0, chapter 4.2 Block Format:
        // The LEN and CRC fields shall have their Most Significant Byte sent first (i.e. big-endian order).
        const uint16_t CRC = t1CalculateCRC(pData, offset, len - 2);

        // In APDU Transport over SPI / I2C – Public Release v1.0, chapter 4.2 Block Format:
        // The LEN and CRC fields shall have their Most Significant Byte sent first (i.e. big-endian order).
        pData[len - 2] = (uint8_t)(CRC >> 8); // pData[len - 2],CRC >> 8
        pData[len - 1] = (uint8_t)CRC;
    }
}

/******************************************************************************
 * @Function     t1ChkEpilogue
 *
 * @Description  This internal function is called check the LRC or CRC
 *
 * @params       pData  - data to compute the LRC or CRC over.
 *               offset - calculate start offset.
 *               len    - data length, include the epilogue byte(s).
 *
 * @Returns      ESESTATUS_SUCCESS if caculate result is equal to response's epilogue,
 *               else ESESTATUS_PARITY_ERROR.
 *
 ******************************************************************************/
static ESESTATUS t1ChkEpilogue(uint8_t *pData, uint16_t offset, uint16_t len)
{
    ESESTATUS status = ESESTATUS_PARITY_ERROR;

#ifndef LRC_CRC_0
    if ((ESE_MODE_NFCC_DL == gInitMode)
            || (ESE_MODE_ESE_DL == gInitMode)) {
        // TMS LRC for NFCC or ESE BL
        // keep offset unchanged (expected value is 0), do nothing
    } else {
        // LRC for C1
        TMS_LOG_D(g_tag, "%s adapter for C1, original offset = 0x%X, changed offset = 0x01",
                  __FUNCTION__, offset);
        offset = 1;
    }
#endif

    /* Note:T1 CIP_REQ for ATR, the frame's INFO length should be 1 byte,
     *      do not support 2 bytes length.
     *      And epiLogue field should use LRC (1 byte).
     */
    if ((LRC == gAtr.checksum) ||
        (CIP_REQ == GetEseCtx()->pT1Params->txSubSFrameType) ||
        (CIP_RSP == GetEseCtx()->pT1Params->txSubSFrameType)) {
        // LRC
        const uint8_t LRC = t1CalculateLRC(pData, offset, len - 1);
        if (LRC == pData[len - 1]) {
            status = ESESTATUS_SUCCESS;
        } else {
            TMS_LOG_E(g_tag, "%s PARITY_ERROR, LRC-D[%02X], LRC-C[%02X]",
                      __FUNCTION__, LRC, pData[len - 1]);
        }
    } else {
        // CRC
        // In APDU Transport over SPI / I2C – Public Release v1.0, chapter 4.2 Block Format:
        // The LEN and CRC fields shall have their Most Significant Byte sent first (i.e. big-endian order).
        uint16_t rspCRC = ((uint16_t)pData[len - 2]) << 8;
        rspCRC |= pData[len - 1];
        const uint16_t CRC = t1CalculateCRC(pData, offset, len - 2);
        if (CRC == rspCRC) {
            status = ESESTATUS_SUCCESS;
        } else {
            TMS_LOG_E(g_tag, "%s PARITY_ERROR, CRC-D[%04X], LRC-C[%04X]",
                      __FUNCTION__, CRC, rspCRC);
        }
    }

    return status;
}

/******************************************************************************
 * @Function     t1Read
 *
 * @Description  This function read the data from slave device (e.g. eSE)
 *               through physical interface (e.g. SPI) using the  driver interface.
 *               First read the T=1 header, second read payload data and check sum (LRC or CRC).
 *
 * @params       ppData  - point to data buffer address, to be used to store the read content.
 *               pDataLen - point to data length.
 *
 * @Returns      On success return ESESTATUS_SUCCESS, else ESESTATUS_xxx.
 *
 ******************************************************************************/
static ESESTATUS t1Read(uint8_t **ppData, uint16_t *pDataLen)
{
    ESESTATUS status = ESESTATUS_SUCCESS;
    int ret = -1;
    SEContext *pEseCtx = GetEseCtx();
    const uint32_t sofMaxCnt = pEseCtx->maxReadRetryCnt;
    uint32_t sofCnt = 0;
    uint16_t totalCnt = 0, numBytesToRead = 0, readIndex = 0;
    struct timeval startTime, endTime;
    uint8_t *pDataRx = GetDataRx();

#ifdef USE_SPI_SPLIT_READ
    // Fixed the read gpDataRx[0] is 0xFF or 0x00, length need read 1 byte more.
    const uint8_t hdrLen = getHeaderLen(false) + 1;
#else
    const uint8_t hdrLen = pEseCtx->maxIFSD;
#endif

#ifdef DBG_LEVEL_STACK
    if (GetDumpStackFlag()) {
        DumpCallstack();
    }
    SetDumpStackFlag(true);
#endif

    TMS_LOG_D(g_tag, "%s readRetryTime = %uus, maxReadRetryCnt = %u",
              __FUNCTION__, pEseCtx->readRetryTime, pEseCtx->maxReadRetryCnt);

    (void)memset_s(pDataRx, GetDataRxLen(), 0x00, GetDataRxLen());
    gettimeofday(&startTime, 0);
    do {
        // delay 1ms (default delay time)
        usleep(pEseCtx->readRetryTime);
#ifdef DBG_LEVEL_1
        TMS_LOG_D(g_tag, "%s Normal Pkt, delay read %uus", __FUNCTION__, pEseCtx->readRetryTime);
#endif

        // SOF read gpDataRx[0] may be invalid, so the first read len is hdrLen + 1,
        // make sure contains NAD + PCB + INF LEN
        ret = PhRead(pDataRx, hdrLen);
        if (ret < 0) {
            TMS_LOG_W(g_tag, "_spi_read() [HDR]errno : %x ret : %X", errno, ret);
        } else {
            if (NAD_C2DT == pDataRx[0]) {
                TMS_LOG_D(g_tag, "%s Read HDR NAD + PCB + LEN + 1, readCount = %d", __FUNCTION__, hdrLen);
                if (pEseCtx->isT1ExtHdrLen) {
                    // In APDU Transport over SPI / I2C – Public Release v1.0, chapter 4.2 Block Format:
                    // The LEN and CRC fields shall have their Most Significant Byte sent first (i.e. big-endian order).
                    // EseCtx[T1_FRAME_LEN_OFFSET] << 8
                    numBytesToRead = ((uint16_t)(pDataRx[T1_FRAME_LEN_OFFSET])) << 8;
                    numBytesToRead |= pDataRx[T1_FRAME_LEN_OFFSET2];

                    // First read length is hdrLen + 1,
                    // CRC has 2 bytes, so the second read len is INF len + 1
                    if (CRC == gAtr.checksum) {
                        numBytesToRead += 1;
                    }
                } else {
                    // First read length is hdrLen + 1,
                    // LRC has 1 bytes, so the second read len is INF len
                    numBytesToRead = pDataRx[T1_FRAME_LEN_OFFSET];
                }
                readIndex = hdrLen;
                totalCnt += hdrLen;
                break;
            } else if (((pDataRx[0] == 0x00) || (pDataRx[0] == 0xFF))
                       && (NAD_C2DT == pDataRx[1])) {
                TMS_LOG_D(g_tag, "%s Read HDR NAD + PCB + LEN, readCount = %d", __FUNCTION__, ret);
                pDataRx[0] = pDataRx[1];  // pDataRx[0] = pDataRx[1]
                pDataRx[1] = pDataRx[2];  // pDataRx[1] = pDataRx[2]
                pDataRx[2] = pDataRx[3];  // pDataRx[2] = pDataRx[3]
                if (pEseCtx->isT1ExtHdrLen) {
                    pDataRx[3] = pDataRx[4];  // pDataRx[3] = pDataRx[4]
                    // In APDU Transport over SPI / I2C – Public Release v1.0, chapter 4.2 Block Format:
                    // The LEN and CRC fields shall have their Most Significant Byte sent first (i.e. big-endian order).
                    // pDataRx[T1_FRAME_LEN_OFFSET] << 8
                    numBytesToRead = ((uint16_t)(pDataRx[T1_FRAME_LEN_OFFSET])) << 8;
                    numBytesToRead |= pDataRx[T1_FRAME_LEN_OFFSET2];

                    // First read length is hdrLen + 1, but the index 0 is invalid,
                    // so only has been read HDR content.
                    // CRC has 2 bytes, so the second read len is INF len + 2
                    if (CRC == gAtr.checksum) {
                        numBytesToRead += 2;  // numBytesToRead + 2
                    }
                } else {
                    // First read length is hdrLen + 1, but the index 0 is invalid,
                    // so only has been read HDR content.
                    // LRC has 1 bytes, so the second read len is INF len + 1
                    numBytesToRead = (pDataRx[T1_FRAME_LEN_OFFSET]) + 1;
                }
                readIndex = hdrLen - 1;
                totalCnt += hdrLen - 1;
                break;
            }
#ifdef DBG_LEVEL_1
            else if (((pDataRx[0] == 0x00) && (pDataRx[1] == 0x00))
                     || ((pDataRx[0] == 0xFF) && (pDataRx[1] == 0xFF))) {
                TMS_LOG_D(g_tag, "read() Buf[0]: %02X Buf[1]: %02X", pDataRx[0], pDataRx[1]);
            } else if (pDataRx[0] == 0x7E) {
                // I2C driver response data will be filled 0x7E bytes when IRQ is L
                TMS_LOG_D(g_tag, "wait for ph respose data, Corruption Buf[0]: %02X Buf[1]: %02X, errno = %d",
                          pDataRx[0], pDataRx[1], errno);
            } else if (ret >= 0) {
                // Corruption happened during the receipt from Card, go flush out the data
                TMS_LOG_D(g_tag, "read() Corruption Buf[0]: %02X Buf[1]: %02X, len=%d",
                          pDataRx[0], pDataRx[1], ret);
                // Do nothing, and retry read
            } else {
                TMS_LOG_D(g_tag, "read() Buf[0]: %02X Buf[1]: %02X", pDataRx[0], pDataRx[1]);
            }
#endif
        }
        sofCnt++;
        gettimeofday(&endTime, 0);
    } while ((sofCnt < sofMaxCnt) && !pEseCtx->isReadDone
             && ((uint32_t)(endTime.tv_sec - startTime.tv_sec) < pEseCtx->readTimeout));

    /* SOF Read timeout happened, go for frame retransmission */
    if (sofCnt == sofMaxCnt) {
        if (ret >= 0) {
            ret = -1;
            status = ESESTATUS_PH_IOR_INVALID_DATA;
        } else {
            status = ESESTATUS_PH_IO;
        }
        TMS_LOG_E(g_tag, "_spi_read() [HDR] execption, ret = %d, sofCnt = %X", ret, sofCnt);
    }

#ifdef USE_SPI_SPLIT_READ
    if (ret < 0) {
        if ((ret == -1) && (errno == 4)) {  // errno == 4
            // Interrupted system call, is NFC_DLD_FLUSH
            status = ESESTATUS_FATAL_ERROR;
        }
    } else {
        TMS_LOG_D(g_tag, "%s [HDR] FOUND, try read payload", __FUNCTION__);
        if (0 != numBytesToRead) {
            ret = PhRead(&pDataRx[readIndex], numBytesToRead);
            if (ret < 0) {
                status = ESESTATUS_PH_IO;
                TMS_LOG_E(g_tag, "%s _spi_read() [payload] execption, ret = %x, errno = %d",
                          __FUNCTION__, ret, errno);
            } else {
                totalCnt += numBytesToRead;
            }
        }
    }
#endif

    if (ESESTATUS_SUCCESS != status) {
        ppData = NULL;
        *pDataLen = 0;
        // length 6
        printHexPacket(g_tag, strlen(g_tag), "RxFail", 6, pDataRx, totalCnt);
        TMS_LOG_E(g_tag, "read fail: %s, errno = %d, ret = %d", __FUNCTION__, errno, ret);
    } else {
        *ppData = pDataRx;
        *pDataLen = totalCnt;
        // length 2
        printHexPacket(g_tag, strlen(g_tag), "Rx", 2, pDataRx, totalCnt);
    }

    TMS_LOG_D(g_tag, "%s exit, status = %d, dataLen = %u", __FUNCTION__, status, totalCnt);

    return status;
}

/******************************************************************************
 * @Function     t1DecodeFrame
 *
 * @Description  This function decode the received data, and initalized the next action params.
 *
 * @params       pData  - point to data buffer.
 *               dataLen - data length.
 *
 * @Returns      On success return ESESTATUS_SUCCESS, else ESESTATUS_xxx.
 *
 ******************************************************************************/
static ESESTATUS t1DecodeFrame(uint8_t *pData, uint16_t dataLen)
{
    ESESTATUS status = ESESTATUS_SUCCESS;
    uint8_t pcb = pData[T1_PCB_OFFSET];
    T1PcbBits *pPcbBits;
    SEContext *pEseCtx = GetEseCtx();
    pPcbBits = &pEseCtx->pT1Params->lastRxPcbBits;
    (void)memset_s(pPcbBits, sizeof(T1PcbBits), 0x00, sizeof(T1PcbBits));
    int err = memcpy_s(pPcbBits, sizeof(T1PcbBits), &pcb, sizeof(uint8_t));
    if (err != EOK) {
        TMS_LOG_E(g_tag, "%s: memcpy_s err. ret:%d", __FUNCTION__, err);
    }
    uint16_t infoLen = getInfoLen(pData, dataLen);
    uint8_t dataOffset = getHeaderLen(false);
    if (0x00 == pPcbBits->msb) {
        // I-FRAME decoded
        pEseCtx->pT1Params->lastRxFrameType = IFRAME;
        pEseCtx->pT1Params->lastRxSubSFrameType = SFRAME_TYPE_INVALID;
        pEseCtx->pT1Params->lastRxSubRFrameType = RNACK_INVALID_ERROR;

        // I-FRAME received, wtxCnt should be reset to 0
        pEseCtx->pT1Params->wtxCnt = 0;

        if (pEseCtx->isSeqNumDR
                && (pPcbBits->bit7 != pEseCtx->seqNumCard)) {
            pEseCtx->seqNumCard ^= 0x01;
        }
        pEseCtx->isSeqNumDR = false;

        if (pPcbBits->bit7 == pEseCtx->seqNumCard) {
            // reset recovery count, reset resync flag.
            pEseCtx->pT1Params->recoveryCnt = 0;
            pEseCtx->isResyncRecoveryFlag = false;

            if (pPcbBits->bit6) {
                pEseCtx->pT1Params->isCardChaining = true;
                pEseCtx->seqNumCard = (pEseCtx->seqNumCard ^ 0x01);
                // Chaining, store the segments data to list
                status = t1RecvDataStoreInList(&pData[dataOffset], infoLen, dataLen);
                if (ESESTATUS_SUCCESS != status) {
                    // If received invalid I-block, it must be LRC/CRC error,
                    // otherwise, R-block will be received.
                    // So store failed should be terminated.
                    TMS_LOG_E(g_tag, "%s, t1RecvDataStoreInList failed, terminate transmission, status = %d",
                              __FUNCTION__, status);
                    pEseCtx->nextT1State = STATE_IDEL;
                    FreeTxPartMem();
                } else {
                    // Card response chaining and successfully, request the next segments data.
                    pEseCtx->nextT1State = R_ACK;
                    pEseCtx->pT1Params->txFrameType = RFRAME;
                    pEseCtx->pT1Params->txSubRFrameType = RACK;
                }
            } else {
                // Not chaining, and response success.
                pEseCtx->pT1Params->isCardChaining = false;
                status = t1RecvDataStoreInList(&pData[dataOffset], infoLen, dataLen);
                if (ESESTATUS_SUCCESS != status) {
                    // pPcbBits->bit7 == pEseCtx->seqNumCard, if received invalid I-block,
                    // it must be LRC/CRC error, otherwise, R-block will be received.
                    // So store failed should be terminated.
                    TMS_LOG_E(g_tag, "%s, t1RecvDataStoreInList failed, terminate transmission, status = %d",
                              __FUNCTION__, status);
                } else {
                    // Not chaining, and response success.
                }
                pEseCtx->seqNumDevice = (pEseCtx->seqNumDevice ^ 0x01);
                pEseCtx->seqNumCard = (pEseCtx->seqNumCard ^ 0x01);
                pEseCtx->nextT1State = STATE_IDEL;
                FreeTxPartMem();
            }
        } else {
            // The card response sequence number is error.
            // R-block error only LRC/CRC error and other error, so this is other error.
            // R(0) (0x82)
            TMS_LOG_E(g_tag, "%s, received invalid seqNum = %02X, seqNumCard = %02X",
                      __FUNCTION__, pPcbBits->bit7, pEseCtx->seqNumCard);
            doT1RecoveryAtDecodeIFrame();
        }
    } else if ((0x01 == pPcbBits->msb)
               && (0x00 == pPcbBits->bit7)) {
        // R-FRAME decoded
        pEseCtx->pT1Params->lastRxFrameType = RFRAME;
        pEseCtx->pT1Params->lastRxSubSFrameType = SFRAME_TYPE_INVALID;

        if ((0x00 == pPcbBits->lsb) && (0x00 == pPcbBits->bit2)) {
            pEseCtx->pT1Params->lastRxSubRFrameType = RACK;
            if ((NULL != pEseCtx->pT1Params->pDataTx) && pEseCtx->pT1Params->isDeviceChaining) {
                if (pEseCtx->seqNumDevice != pPcbBits->bit5) {
                    // Device chaining, valid card request the next segments of device data to send.
                    // seqNumDevice should changed, but the seqNumCard unchanged until received card I-block.
                    pEseCtx->seqNumDevice = pPcbBits->bit5;
                    pEseCtx->nextT1State = I_BLK;
                    pEseCtx->pT1Params->txFrameType = IFRAME;
                    pEseCtx->pT1Params->txSubRFrameType = RNACK_INVALID_ERROR;
                    FreeTxPartMem();
                    // others parameters should not be modified.
                } else {
                    // The card request next segments sequence number is error
                    // R-block error only LRC/CRC error and other error,
                    // so this is other error (only seqNum error occured).
                    // pEseCtx->pT1Params->lastRecvSubRFrameType = RNACK_OTHER_ERROR
                    // do case 8
                    doT1RecoveryAtDecodeRFrame();
                }
            } else if (NULL != pEseCtx->pT1Params->pDataTx) {
                // Device initiates block is unchaining IFrame, cannot receive R(N(R))(b00).
                // Do nothing
                status = ESESTATUS_INVALID_PARAMETER;
                TMS_LOG_E(g_tag, "%s: Device initiates unchaining I-block cannot receive RACK,"
                          " terminate transmit.", __FUNCTION__);
                // terminate transmit
                pEseCtx->nextT1State = STATE_IDEL;
            } else {
                // error-free acknowledgement only received when device transmit chaining I-block.
                // None-I-block will do nothing and terminate.
                pEseCtx->nextT1State = STATE_IDEL;
                // do not FreeTxPartMem(), t1TransceiveApdu will be free it.
            }
        } else {
            pEseCtx->pT1Params->lastRxSubSFrameType = SFRAME_TYPE_INVALID;
            if ((0x01 == pPcbBits->lsb) && (0x00 == pPcbBits->bit2)) {
                // LRC or CRC error
                pEseCtx->pT1Params->lastRxSubRFrameType = RNACK_PARITY_ERROR;
                status = ESESTATUS_PARITY_ERROR;
            } else if ((0x00 == pPcbBits->lsb) && (0x01 == pPcbBits->bit2)) {
                // Other error
                pEseCtx->pT1Params->lastRxSubRFrameType = RNACK_OTHER_ERROR;
                status = ESESTATUS_UNKNOWN_ERROR;
            } else {
                // Unkonwn error. Retry the last frame.
                pEseCtx->pT1Params->lastRxSubRFrameType = RNACK_INVALID_ERROR;
                status = ESESTATUS_UNKNOWN_ERROR;
            }

            TMS_LOG_D(g_tag, "%s: PCB = 0x%02X, recoveryCnt = %d, maxRecoveryCnt = %d",
                      __FUNCTION__, pcb, pEseCtx->pT1Params->recoveryCnt, pEseCtx->maxRecoveryCnt);
            doT1RecoveryAtDecodeRFrame();
        }
    } else if ((0x01 == pPcbBits->msb)
               && (0x01 == pPcbBits->bit7)) {
        // S-FRAME decoded
        pEseCtx->pT1Params->lastRxFrameType = SFRAME;
        pEseCtx->pT1Params->lastRxSubRFrameType = RNACK_INVALID_ERROR;

        TMS_LOG_D(g_tag, "%s S-Frame Received", __FUNCTION__);
        uint8_t subFrameType = pcb & T1_S_BLOCK_SUB_TYPE_MASK;
        if (RESYNC_RSP != subFrameType) {
            pEseCtx->isResyncRecoveryFlag = false;
        }

        switch (subFrameType) {
            case RESYNC_RSP: {
                pEseCtx->pT1Params->lastRxSubSFrameType = RESYNC_RSP;
                doT1RecoveryAtDecodeSFrame();
                break;
            }

            case IFS_REQ: {
                uint16_t IFSC = 0;
                if (2 == infoLen) { // infoLen == 2
                    IFSC = ((uint16_t)pData[dataOffset]) << 8;  // pData[dataOffset]) << 8
                    IFSC |= pData[dataOffset + 1];
                    TMS_LOG_I(g_tag, "%s IFS_RSP Received, IFSD = %04X", __FUNCTION__, IFSC);
                } else if (1 == infoLen) {
                    IFSC = (uint16_t)pData[dataOffset];
                    TMS_LOG_I(g_tag, "%s IFS_RSP Received, IFSD = %02X", __FUNCTION__, IFSC);
                } else {
                    TMS_LOG_E(g_tag, "%s Invalid IFS_RSP Received", __FUNCTION__);
                }
                pEseCtx->maxIFSC = IFSC;
                pEseCtx->nextT1State = S_IFS_RSP;
                pEseCtx->pT1Params->txFrameType = SFRAME;
                pEseCtx->pT1Params->txSubSFrameType = IFS_RSP;
                break;
            }

            case IFS_RSP: {
                pEseCtx->pT1Params->lastRxSubSFrameType = IFS_RSP;
                uint16_t IFSD = 0;
                uint8_t offset = getHeaderLen(false);
                if (2 == infoLen) { // infoLen == 2
                    IFSD = ((uint16_t)pData[offset]) << 8;  // pData[dataOffset]) << 8
                    IFSD |= pData[offset + 1];
                    TMS_LOG_I(g_tag, "%s IFS_RSP Received, IFSD = %04X", __FUNCTION__, IFSD);
                } else if (1 == infoLen) {
                    IFSD = (uint16_t)pData[offset];
                    TMS_LOG_I(g_tag, "%s IFS_RSP Received, IFSD = %02X", __FUNCTION__, IFSD);
                } else {
                    TMS_LOG_E(g_tag, "%s Invalid IFS_RSP Received", __FUNCTION__);
                }
                if (pEseCtx->maxIFSD != IFSD) {
                    TMS_LOG_W(g_tag, "%s : invalid IFS receive paylod, receivedIFSD = %X, sentIFSD = %X",
                              __FUNCTION__, IFSD, pEseCtx->maxIFSD);
                }
                // Need handle chaining IFrame
                doT1RecoveryAtDecodeSFrame();
                break;
            }

            case ABORT_REQ:
            case ABORT_RSP: {
                // Based on 7816-3, Rule 9:
                // only chaining cmd or rsp will send (receive) ABORT_REQ
                if (pEseCtx->pT1Params->isCardChaining || pEseCtx->pT1Params->isDeviceChaining) {
                    T1RecvDataListDestroy();
                }

                // SE COS do not implement S_ABORT_RSP receive and R(0) response.
                // And we think this implementation is right.
                // So, if Device received ABORT_REQ(RSP) , release the resource and exit.
                pEseCtx->nextT1State = STATE_IDEL;
                break;
            }

            case WTX_REQ: {
                if (pEseCtx->pT1Params->wtxCnt <= pEseCtx->maxWTXCnt) {
                    pEseCtx->pT1Params->wtxCnt++;

                    pEseCtx->pT1Params->wtxInfo = pData[getHeaderLen(false)];
                    pEseCtx->nextT1State = S_WTX_RSP;
                    pEseCtx->pT1Params->txFrameType = SFRAME;
                    pEseCtx->pT1Params->txSubSFrameType = WTX_RSP;
                } else {
                    TMS_LOG_W(g_tag, "%s : failed, WTX_REQ counter larger than maxWtxCnt", __FUNCTION__);
                    status = ESESTATUS_FAILED;
                    pEseCtx->nextT1State = STATE_IDEL;
                }
                break;
            }

            case CIP_RSP: {
                status = decodeATR(pData, dataLen, true);
                ResetForResyncRsp();
                pEseCtx->nextT1State = STATE_IDEL;
                break;
            }

            case PROP_END_APDU_RSP: {
                status = ESESTATUS_SUCCESS;
                pEseCtx->nextT1State = STATE_IDEL;
                break;
            }

            case HARD_RESET_RSP: {
                status = ESESTATUS_SUCCESS;
                pEseCtx->nextT1State = STATE_IDEL;
                break;
            }

            case ATR_RSP: {
                status = decodeATR(pData, dataLen, false);
                pEseCtx->nextT1State = STATE_IDEL;
                break;
            }

            default: {
                status = ESESTATUS_SUCCESS;
                pEseCtx->nextT1State = STATE_IDEL;
                TMS_LOG_W(g_tag, "%s : invalid S-block frame type: %X", __FUNCTION__, subFrameType);
            }
        }
    } else {
        TMS_LOG_E(g_tag, "%s Wrong-Frame Received, PCB = %X", __FUNCTION__, pcb);
        status = ESESTATUS_INVALID_PCB;
    }

    return status;
}

/** Contains the Prologue and (or not) Epilogue field length **/
static uint8_t getHeaderLen(bool containsEpilogue)
{
    uint8_t len = 0;
    SEContext *pEseCtx = GetEseCtx();
    bool isT1ExtHdrLen = pEseCtx->isT1ExtHdrLen;
    uint8_t checksum = gAtr.checksum;
    if ((SFRAME == pEseCtx->pT1Params->txFrameType)
            && (CIP_REQ == pEseCtx->pT1Params->txSubSFrameType)) {
        /* Note:T1 CIP_REQ for ATR, the frame's INFO length should be 1 byte,
         *      do not support 2 bytes length.
         *      And epiLogue field should use LRC (1 byte).
         */
        isT1ExtHdrLen = false;
        checksum = 0;
    }

    len = (isT1ExtHdrLen ? T1_EXT_HEADER_LEN : T1_HEADER_LEN);
    if (containsEpilogue) {
        len += ((checksum == 0) ? T1_LRC_LEN : T1_CRC_LEN);
    }

    return len;
}

static uint8_t setInfoLen(uint8_t *pData, uint16_t infoLen)
{
    uint8_t lenOffset = 0;
    SEContext *pEseCtx = GetEseCtx();
    bool isT1ExtHdrLen = pEseCtx->isT1ExtHdrLen;
    if ((SFRAME == pEseCtx->pT1Params->txFrameType)
            && (CIP_REQ == pEseCtx->pT1Params->txSubSFrameType)) {
        /* Note:T1 CIP_REQ for ATR, the frame's INFO length should be 1 byte,
         *      do not support 2 bytes length.
         *      And epiLogue field should use LRC (1 byte).
         */
        isT1ExtHdrLen = false;
    }

    if (isT1ExtHdrLen) {
        pData[T1_FRAME_LEN_OFFSET] = (uint8_t)(infoLen >> 8);  // infoLen >> 8
        pData[T1_FRAME_LEN_OFFSET2] = (uint8_t)(infoLen & 0xFF);
        lenOffset = T1_FRAME_LEN_OFFSET2;
    } else {
        pData[T1_FRAME_LEN_OFFSET] = (uint8_t)(infoLen & 0xFF);
        lenOffset = T1_FRAME_LEN_OFFSET;
    }
    return lenOffset;
}

static uint16_t getInfoLen(uint8_t *pData, uint16_t dataLen)
{
    uint16_t infoLen = 0;
    SEContext *pEseCtx = GetEseCtx();
    bool isT1ExtHdrLen = pEseCtx->isT1ExtHdrLen;
    if ((isT1ExtHdrLen && dataLen < T1_FRAME_LEN_OFFSET2 + 2)  // T1_FRAME_LEN_OFFSET2 + 2
            || (dataLen < T1_FRAME_LEN_OFFSET + 2)) {  // T1_FRAME_LEN_OFFSET + 2
        TMS_LOG_E(g_tag, "%s: invalid dataLen[%d]", __FUNCTION__, dataLen);
        return 0;
    }
    if ((SFRAME == pEseCtx->pT1Params->txFrameType)
            && (CIP_REQ == pEseCtx->pT1Params->txSubSFrameType)) {
        /* Note:T1 CIP_REQ for ATR, the frame's INFO length should be 1 byte,
         *      do not support 2 bytes length.
         *      And epiLogue field should use LRC (1 byte).
         */
        isT1ExtHdrLen = false;
    }

    if (isT1ExtHdrLen) {
        // pData[T1_FRAME_LEN_OFFSET]) << 8
        infoLen = ((uint16_t)pData[T1_FRAME_LEN_OFFSET]) << 8;
        infoLen |= pData[T1_FRAME_LEN_OFFSET2];
    } else {
        infoLen = pData[T1_FRAME_LEN_OFFSET];
    }
    return infoLen;
}

/******************************************************************************
 * @Function     decodeATR
 *
 * @Description  This internal function is called to decode and reintialize ATR.
 *               Note1:T1 response for ATR, the frame's INFO length should be 1 byte,
 *                     do not support 2 bytes length.
 *                     So, CIP and ATR response frame's INFO LEN is 1 byte.
 *
 * @params       pData  - received data and its must be PCB = 0xE4 or 0xE7.
 * @params       isCIP  - true if received data's PCB = 0xE4.
 *
 * @Returns      ESESTATUS_SUCCESS if decode ATR success, else ESESTATUS_INVALID_T1_LEN.
 *
 ******************************************************************************/
static ESESTATUS decodeATR(uint8_t *pData, uint16_t dataLen, bool isCIP)
{
    ESESTATUS status = ESESTATUS_SUCCESS;
    // T1 response for ATR INFO, length should be 1 byte for CIP_REQ, or 2 bytes for ATR_REQ.
    uint16_t infoLen = getInfoLen(pData, dataLen);
    uint8_t *pStart = &pData[T1_FRAME_LEN_OFFSET];
    SEContext *pEseCtx = GetEseCtx();
    if (!isCIP && pEseCtx->isT1ExtHdrLen) {
        // Currently, ATR length is 0x13, exttended frame length high byte should be 0x00
        pStart++;
    }

    if (sizeof(AtrInfo) == (infoLen + 1)) {
        int err = memcpy_s(&gAtr.len, sizeof(AtrInfo), pStart, sizeof(AtrInfo));
        if (err != EOK) {
            TMS_LOG_E(g_tag, "%s: memcpy_s err. ret:%d", __FUNCTION__, err);
        }
    } else if (0 == pData[T1_FRAME_LEN_OFFSET]) {
        // Do nothing, keep befor ATR value.
        ResetATR();
    } else {
        TMS_LOG_W(g_tag, "%s : invalid ATR response AtrInfoLen = %02X",
                  __FUNCTION__, (uint8_t)sizeof(AtrInfo));
        // length 10
        printHexPacket(g_tag, strlen(g_tag), "InvalidATR", 10,
                       pData, T1_HEADER_LEN + infoLen + T1_LRC_LEN);
        status = ESESTATUS_INVALID_T1_LEN;
    }

    if (ESESTATUS_SUCCESS == status) {
        // gAtr.maxIFSC[0]) << 8
        pEseCtx->maxIFSC = ((uint16_t)gAtr.maxIFSC[0]) << 8;
        pEseCtx->maxIFSC |= gAtr.maxIFSC[1];
        pEseCtx->isT1ExtHdrLen = (gAtr.capbilities[1] & 0x08) != 0;

        atrElements(g_tag, strlen(g_tag));
    }

    return status;
}

/******************************************************************************
 * @Function     initIFrameFromCmdApdu
 *
 * @Description  Prepare T=1 data form APDU.
 *
 * @params       pCmdApdu - 7816-4 APDU that shall be sent.
 *               cmdLen - Length of the apduCmd to be sent.
 *
 * @Returns          On success return ESESTATUS_SUCCESS or else ESESTATUS error.
 *
 * Note: Should free pT1Params->pDataTx if this function return ESESTATUS_SUCCESS.
 ******************************************************************************/
static ESESTATUS initIFrameFromCmdApdu(uint8_t *pCmdApdu, uint16_t cmdLen)
{
    ESESTATUS status = ESESTATUS_SUCCESS;
    SEContext *pEseCtx = GetEseCtx();

    T1Params *pT1Params = (T1Params *)calloc(1, sizeof(T1Params));
    if (NULL == pT1Params) {
        TMS_LOG_E(g_tag, "%s: Tx data malloc memory failed", __FUNCTION__);
        return ESESTATUS_MEM_EXCEPTION;
    }

    pEseCtx->pT1Params = pT1Params;
    pT1Params->isDeviceChaining = (cmdLen > pEseCtx->maxIFSC ? true : false);
    pEseCtx->nextT1State = I_BLK;
    pT1Params->txFrameType = IFRAME;
    pT1Params->pDataTx = pCmdApdu;
    pT1Params->txLen = cmdLen;

    return status;
}

/******************************************************************************
 * @Function     DeInitIFrameFromCmdApdu
 *
 * @Description  release IFrame resources.
 *
 * @Returns          On success return ESESTATUS_SUCCESS or else ESESTATUS error.
 *
 ******************************************************************************/
static void DeInitIFrameFromCmdApdu()
{
    SEContext *pEseCtx = GetEseCtx();
    if (NULL != pEseCtx->pT1Params) {
        // Should not be free pDataTx here. This will be free at SecureElement.cpp
        pEseCtx->pT1Params->pDataTx = NULL;

        if (NULL != pEseCtx->pT1Params->pDataTxPart) {
            free(pEseCtx->pT1Params->pDataTxPart);
            pEseCtx->pT1Params->pDataTxPart = NULL;
        }

        free(pEseCtx->pT1Params);
        pEseCtx->pT1Params = NULL;
    }
}

/******************************************************************************
 * @Function     FreeTxPartMem
 *
 * @Description  release IFrame part send successfully memory when is chaining.
 *
 ******************************************************************************/
static void FreeTxPartMem()
{
    SEContext *pEseCtx = GetEseCtx();
    if (NULL != pEseCtx->pT1Params) {
        if (NULL != pEseCtx->pT1Params->pDataTxPart) {
            free(pEseCtx->pT1Params->pDataTxPart);
            pEseCtx->pT1Params->pDataTxPart = NULL;
        }
    }
}

/******************************************************************************
 * @Function     ResetForResyncRsp
 *
 * @Description  reset recoveryCnt and I-block related paramters.
 *
 ******************************************************************************/
static void ResetForResyncRsp()
{
    SEContext *pEseCtx = GetEseCtx();
    pEseCtx->seqNumCard = 0;
    pEseCtx->seqNumDevice = 0;
    pEseCtx->pT1Params->recoveryCnt = 0;
    pEseCtx->pT1Params->wtxCnt = 0;
    pEseCtx->pT1Params->txDataOffset = 0;
    if (pEseCtx->pT1Params->isCardChaining) {
        T1RecvDataListDestroy();
    }
    if (NULL != pEseCtx->pT1Params->pDataTxPart) {
        pEseCtx->pT1Params->pDataTxPart = NULL;
    }
    pEseCtx->pT1Params->isDeviceChaining = false;
    pEseCtx->pT1Params->isCardChaining = false;
}

/* LRC or CRC error cases: Do recovery for invalid block (I/R/S block):
 * Rule 7.1 — When an I-block was transmitted and an invalid block is received or
 *            a BWT time-out (with the interface device) occurs, an R-block is transmitted,
 *            which requests with its N(R) for the expected I-block with N(S) = N(R).
 *
 * Rule 7.2 — When an R-block was transmitted and an invalid block is received or a BWT
 *            time-out (with the interface device) occurs, this R-block is retransmitted.
 *
 * Rule 7.3 — When S(… request) was transmitted and the received response is not
 *            S(… response) or a BWT time-out occurs (only with the interface device),
 *            S(… request) is retransmitted.
 *
 *            When S(… response) was transmitted and an invalid block is received
 *                 or a BWT time-out occurs (only with the interface device),
 *                 an R-block is transmitted.
 *
 * I. Depends on Rule 7.1/7.2/7.3:=================================================[start]
 * => invalid block: LRC or CRC error
 * => BWT time-out: t1Read error, do not read anything from card
 * case 1-1, Tx is I/R-block, invalid block, send R(0)(0x81) to card;
 * case 1-2, Tx is I/R-block, BWT time-out, send R(0)(0x82) to card;
 *
 * case 2-1, Tx is S(WTX response), invalid block, send R(0)(0x81) to card;
 * case 2-2, Tx is S(WTX response), BWT time-out, send R(0)(0x82) to card;
 *
 * case 3-1, Tx is others S(… request), invalid block, resend the last S-block;
 * case 3-2, Tx is others S(… request), BWT time-out, resend the last S-block.
 * I. Depends on Rule 7.1/7.2/7.3:===================================================[end]
 *
 *
 * Rule 6 — S(RESYNCH request) may be transmitted only by the interface device to
 *          reach resynchronization and to initiate resetting the communication
 *          parameters of the transmission protocol to its initial values.
 * Rule 6.4 — After the interface device has failed a maximum of three times in succession
 *            to reach the intended resynchronization by transmitting S(RESYNCH request),
 *            it performs either a warm reset or a deactivation.
 * Rule 6.5 — When S(RESYNCH request) is received, the previously transmitted block
 *            is assumed not to have been received.
 *
 * II. Depends on Rule 7.1/7.2/7.3:================================================[start]
 * Depends on Rule 6.4, maximum of three times:
 * case 4, Only the last send frame is S(WTX response) or non-S-block:
 *       =>RESYNCH request should be sent.
 *         All parameters should be reset, such as seqNum, recoveryCnt, etc.
 * case 5, The last send frame is S(...REQ) but not S(RESYNCH REQ):
 *       =>CIP request should be sent to performs a warm reset.
 *         All parameters should be reset, such as seqNum, recoveryCnt, etc.

 * Depends on Rule 6.5, maximum of three times,:
 * case 6, After RESYNC_REQ/RSP handle successfully,
 *         need resend the previous error block;
 *       =>Do resend in t1DecodeFrame S-Frame process code block.
 *
 * case 7, Depends on Rule 6 and Rule 6.5:
 *         If the previous error block is chaining block, the whole block should be resent.
 *       =>Do reset chaining block offset at RESYNC_RSP in t1DecodeFrame.
 * II. Depends on Rule 7.1/7.2/7.3:==================================================[end]
 */

/******************************************************************************
 * @Function     doT1RecoveryAtUndecode
 *
 * @Description  This function Resynchronization of the transmission protocol,
 *               depends on case 1 to case6, when some exceptions occurred.
 *
 * @Returns      On success return ESESTATUS_SUCCESS, else ESESTATUS_FATAL_ERROR.
 *
 * NOTE1: This function called only error occurred.
 * NOTE2: This function called only error block cannot decode by t1DecodeFrame,
 *        that is, only called it in the responseProcess function.
 ******************************************************************************/
static ESESTATUS doT1RecoveryAtUndecode()
{
    ESESTATUS status = ESESTATUS_SUCCESS;
    SEContext *pEseCtx = GetEseCtx();

    if (pEseCtx->pT1Params->recoveryCnt < pEseCtx->maxRecoveryCnt) {
        pEseCtx->pT1Params->recoveryCnt++;

        if ((SFRAME != pEseCtx->pT1Params->txFrameType)
                || (WTX_RSP == pEseCtx->pT1Params->txSubSFrameType)) {
            // Do case 1-1/1-2 and case 2-1/2-2
            pEseCtx->nextT1State = R_PARITY_ERR;
            pEseCtx->pT1Params->txFrameType = RFRAME;
            pEseCtx->pT1Params->txSubRFrameType = RNACK_PARITY_ERROR;
            // Should use the N(S), that is pEseCtx->seqNumDevice
        } else {
            // Do case 3-1/3-2
            // Only recoveryCnt++
        }
    } else {
        if (((SFRAME != pEseCtx->pT1Params->txFrameType)
                || (WTX_RSP == pEseCtx->pT1Params->txSubSFrameType)
            ) && !pEseCtx->isResyncRecoveryFlag) {
            // Mark resync recovery flag is true. Its whole descripted at its definication(common.h)
            // See 7816-3 A.3.5 Resynchronization, RESYNC_REQ can be triggered by any block from card.
            pEseCtx->isResyncRecoveryFlag = true;
            // RESYNC_REQ only send once, recoveryCnt should not be reset to zero.

            // Do case 4
            pEseCtx->nextT1State = S_RESYNC_REQ;
            pEseCtx->pT1Params->txFrameType = SFRAME;
            pEseCtx->pT1Params->txSubSFrameType = RESYNC_REQ;
        } else if (pEseCtx->isResyncRecoveryFlag && (pEseCtx->pT1Params->cipRecoveryCnt < pEseCtx->maxRecoveryCnt)) {
            // RESYNC_REQ has been sent to recover, but recovery failed.
            // CIP_REQ should be sent to warm reset.
            // Warm reset does not need to reset the recoveryCnt to zero.

            // Do case 5
            pEseCtx->nextT1State = S_CIP_REQ;
            pEseCtx->pT1Params->txFrameType = SFRAME;
            pEseCtx->pT1Params->txSubSFrameType = CIP_REQ;
            pEseCtx->pT1Params->cipRecoveryCnt++;
        }  else {
            // Chip hard reset if warm reset failed.
            if ((SFRAME == pEseCtx->pT1Params->txFrameType)
                    && (CIP_REQ == pEseCtx->pT1Params->txSubSFrameType)) {
                TMS_LOG_W(g_tag, "%s: warm reset failed, TODO need chip hard reset to recover",
                          __FUNCTION__);
                status = ESESTATUS_FATAL_ERROR;
                pEseCtx->nextT1State = STATE_IDEL;
            } else {
                TMS_LOG_W(g_tag, "%s: S(... REQ) failed, warm reset to recover"
                          "lastTxFrameType = %d, lastTxSFrameType = %d",
                          __FUNCTION__, pEseCtx->pT1Params->txFrameType, pEseCtx->pT1Params->txSubSFrameType);

                // RESYNC_REQ has been sent to recover, but recovery failed.
                // CIP_REQ should be sent to warm reset.
                // Warm reset does not need to reset the recoveryCnt to zero.
                // Do case 5
                pEseCtx->nextT1State = S_CIP_REQ;
                pEseCtx->pT1Params->txFrameType = SFRAME;
                pEseCtx->pT1Params->txSubSFrameType = CIP_REQ;
            }
        }
    }

    return status;
}

/******************************************************************************
 * @Function     doT1RecoveryAtDecodeIFrame
 *
 * @Description  This function recovery the transmission protocol.
 *               Only IFrame reveived and is invalid, but it can be decoded,
 *               such as seqNum is error.
 *
 * @Returns      On success return ESESTATUS_SUCCESS, else ESESTATUS_FATAL_ERROR.
 *
 * NOTE1: This function called only error occurred.
 * NOTE2: This function only can be called in the t1DecodeFrame function
 *        when device received an invalid IFrame.
 ******************************************************************************/
static ESESTATUS doT1RecoveryAtDecodeIFrame()
{
    ESESTATUS status = ESESTATUS_SUCCESS;
    SEContext *pEseCtx = GetEseCtx();
    T1FrameTypes lastRecvFrameType = pEseCtx->pT1Params->lastRxFrameType;
    T1PcbBits *pPcbBits = &pEseCtx->pT1Params->lastRxPcbBits;

    if (IFRAME == lastRecvFrameType) {
        if (pEseCtx->pT1Params->recoveryCnt < pEseCtx->maxRecoveryCnt) {
            pEseCtx->pT1Params->recoveryCnt++;
            // Received I-block, only may be seqNum is error.
            // Scenario 9, 11, 12, 13
            // RNAK_OTHER_ERROR should send to card.
            if (pPcbBits->bit7 != pEseCtx->seqNumCard) {
                pEseCtx->nextT1State = R_OTHER_ERR;
                pEseCtx->pT1Params->txFrameType = RFRAME;
                pEseCtx->pT1Params->txSubRFrameType = RNACK_OTHER_ERROR;
                // Should use the N(R), that is pEseCtx->seqNumCard
            } else {
                // Cannot be occured this case
                TMS_LOG_E(g_tag, "%s: an I-block received, seqNum exception, terminate transmission.\n"
                          "lastTxSeqNum = %d, lastRxExpectedNum = %d, lastRxSeqNum = %d",
                          __FUNCTION__, pEseCtx->seqNumDevice, pEseCtx->seqNumCard, pPcbBits->bit7);
                pEseCtx->nextT1State = STATE_IDEL;
                pEseCtx->isResyncRecoveryFlag = false;
                status = ESESTATUS_INVALID_PARAMETER;
                FreeTxPartMem();
            }
        } else {
            // When recoveryCnt has been larger than or equal to maxRecoveryCnt,
            // some RFrame must have been sent to recover.
            // Should transmit RESYNC_REQ to recover.
            TMS_LOG_D(g_tag, "%s: lastRecvFrameType = %d, recoveryCnt = %d, maxRecoveryCnt = %d"
                      "RESYNC_REQ should be sent to recover",
                      __FUNCTION__, lastRecvFrameType,
                      pEseCtx->pT1Params->recoveryCnt, pEseCtx->maxRecoveryCnt);
            // RESYNC_REQ only send once, recoveryCnt should not be reset to zero.
            pEseCtx->isResyncRecoveryFlag = true;

            // Do case 4
            pEseCtx->nextT1State = S_RESYNC_REQ;
            pEseCtx->pT1Params->txFrameType = SFRAME;
            pEseCtx->pT1Params->txSubSFrameType = RESYNC_REQ;
        }
    } else {
        status = ESESTATUS_INVALID_PARAMETER;
        TMS_LOG_E(g_tag, "%s: should be called when received an invalid IFrame, but non-IFrame"
                  " received, lastRecvFrameType = %d, recoveryCnt = %d, maxRecoveryCnt = %d",
                  __FUNCTION__, lastRecvFrameType,
                  pEseCtx->pT1Params->recoveryCnt, pEseCtx->maxRecoveryCnt);
        pEseCtx->nextT1State = STATE_IDEL;
        FreeTxPartMem();
    }

    return status;
}

/******************************************************************************
 * @Function     doT1RecoveryAtDecodeSFrame
 *
 * @Description  This function recover the transmission protocol.
 *               Only SFrame reveived and need to recover for non-SFrame,
 *               such as, device initiates block is an IFrame, but not received
 *               a valid IFrame and triggered RESYNC_REQ,
 *               RESYNC_RSP will be received and should be recovered continuously.
 *
 * @Returns      On success return ESESTATUS_SUCCESS, else ESESTATUS_FATAL_ERROR.
 *
 * NOTE1: This function called only error occurred.
 * NOTE2: This function only can be called in the t1DecodeFrame function
 *        when device received an invalid SFrame or need recover for non-SFrame.
 * NOTE3: Only device received RESYNC_RSP and IFS_RSP maybe need to recover continuously.
 *        For IFS_RSP, and must be device initiates and device ( or card) chaining IFrame.
 *        For RESYNC_RSP, and must be device initiates IFrame.
 ******************************************************************************/
static ESESTATUS doT1RecoveryAtDecodeSFrame()
{
    ESESTATUS status = ESESTATUS_SUCCESS;
    SEContext *pEseCtx = GetEseCtx();
    T1FrameTypes lastRecvFrameType = pEseCtx->pT1Params->lastRxFrameType;
    SFrameTypes lastRecvSubSFrameType = pEseCtx->pT1Params->lastRxSubSFrameType;

    if ((SFRAME == lastRecvFrameType)
            && (NULL != pEseCtx->pT1Params->pDataTx)) {
        // recoveryCnt will be ignored for RESYNC_RSP and IFS_RSP
        if (RESYNC_RSP == lastRecvSubSFrameType) {
            ResetForResyncRsp();
            pEseCtx->nextT1State = I_BLK;
            pEseCtx->pT1Params->txFrameType = IFRAME;
            pEseCtx->pT1Params->txSubSFrameType = SFRAME_TYPE_INVALID;
            pEseCtx->pT1Params->txSubRFrameType = RNACK_INVALID_ERROR;
        } else if (IFS_RSP == lastRecvSubSFrameType) {
            if (pEseCtx->pT1Params->isCardChaining) {
                pEseCtx->nextT1State = R_ACK;
                pEseCtx->pT1Params->txFrameType = RFRAME;
                pEseCtx->pT1Params->txSubSFrameType = SFRAME_TYPE_INVALID;
                pEseCtx->pT1Params->txSubRFrameType = RACK;
            } else if (pEseCtx->pT1Params->isDeviceChaining) {
                pEseCtx->nextT1State = I_BLK;
                pEseCtx->pT1Params->txFrameType = IFRAME;
                pEseCtx->pT1Params->txSubSFrameType = SFRAME_TYPE_INVALID;
                pEseCtx->pT1Params->txSubRFrameType = RNACK_INVALID_ERROR;
            } else {
                TMS_LOG_W(g_tag, "%s: device initiates block I-block is unchaining,"
                          "terminate transmit when received IFS_RSP", __FUNCTION__);
                // terminate transmit
                pEseCtx->nextT1State = STATE_IDEL;
            }
        } else {
            TMS_LOG_W(g_tag, "%s: device initiates block is an I-block, lastTxFrameType = %d,"
                      "but device received R-block, cannot be occurred.",
                      __FUNCTION__, lastRecvSubSFrameType);
            // terminate transmit
            pEseCtx->nextT1State = STATE_IDEL;
        }
    } else {
        status = ESESTATUS_INVALID_PARAMETER;
        TMS_LOG_E(g_tag, "%s: should be called when received an SFrame, but non-SFrame received,"
                  " lastRecvFrameType = %d, recoveryCnt = %d, maxRecoveryCnt = %d",
                  __FUNCTION__, lastRecvFrameType,
                  pEseCtx->pT1Params->recoveryCnt, pEseCtx->maxRecoveryCnt);
        // terminate transmit
        pEseCtx->nextT1State = STATE_IDEL;
    }
    return status;
}

/******************************************************************************
 * @Function     doT1RecoveryAtDecodeRFrame
 *
 * @Description  This function recover the transmission protocol.
 *               Only RFrame reveived and need to recovery.
 *
 * @Returns      On success return ESESTATUS_SUCCESS, else ESESTATUS_FATAL_ERROR.
 *
 * NOTE1: This function called only error occurred.
 * NOTE2: This function only can be called in the t1DecodeFrame function
 *        when device received an invalid RFrame.
 ******************************************************************************/
static ESESTATUS doT1RecoveryAtDecodeRFrame()
{
    ESESTATUS status = ESESTATUS_SUCCESS;
    SEContext *pEseCtx = GetEseCtx();
    T1FrameTypes lastRecvFrameType = pEseCtx->pT1Params->lastRxFrameType;
    RFrameTypes lastRecvSubRFrameType = pEseCtx->pT1Params->lastRxSubRFrameType;

    if (RFRAME == lastRecvFrameType) {
        if (pEseCtx->pT1Params->recoveryCnt < pEseCtx->maxRecoveryCnt) {
            pEseCtx->pT1Params->recoveryCnt++;

            if ((NULL != pEseCtx->pT1Params->pDataTx)) {
                // device initiates block is an I-block

                if (IFRAME == pEseCtx->pT1Params->txFrameType ||
                    ((SFRAME == pEseCtx->pT1Params->txFrameType) &&
                     (ABORT_RSP != pEseCtx->pT1Params->txSubSFrameType))) {
                    // The last send block is an IFRAME or SFRAME && subSFrame is not ABORT_RSP.
                    /*
                     * case 8, Device last send I-block chaining received R(N(R))(b00) and seqNumCard error,
                     *         but triggered recovery case is: Scenario 21 or Scenario 22
                     *         Device should send R(N(R))(b10).
                     *         Send R-block will use seqNumCard, seqNumCard unchanged.
                     * case 9, Device last send I-block unchaining, received R(N(R))(b00),
                     *         error-free acknowledgement only received when device transmit chaining I-block.
                     *         This case cannot be occured, doT1TryRecoveryAtDecode also cannot be called.
                     * case 10, Device last send I-block (un)chaining, received valid R(N(R))(b01),
                     *          recovery case is:
                     *          Card received I-block LRC/CRC error. Device should resend I(N(S)).
                     *          Send I-block will use seqNumDevice, seqNumDevice unchanged.
                     * case 11, Device last send I-block (un)chaining, received vaild R(N(R))(b10),
                     *          recovery case is:
                     *          Card received I-block other error(seqNumDevice error).
                     *          Device should resend I(N(S)).
                     *          seqNumDevice has been error,
                     *          should changed seqNumDevice, send I-block will use seqNumDevice.
                     *
                     * Rule 7.3 — When S(… response) was transmitted and an invalid block is received
                     *            or a BWT time-out occurs (only with the interface device),
                     *            an R-block is transmitted.
                     * case 12, Device last send S(WTX_RSP), received vaild R(N(R)),
                     *          recovery case is:
                     *          Card received S(WTX_RSP) invalid, such as, LRC/CRC error.
                     *          R(N(R)) should be received by device, but COS only resend S(...REQ) @zhanghao,
                     *          not R(N(R)), so this case cannot be occured. But if device polling read occurred
                     *          during card's read state, R(N(R)) will be received by device.
                     */
                    if (RACK == lastRecvSubRFrameType) {
                        // Do case 8, Device last send I-block chaining, device should send R(N(R))(b10).
                        pEseCtx->nextT1State = R_OTHER_ERR;
                        pEseCtx->pT1Params->txFrameType = RFRAME;
                        pEseCtx->pT1Params->txSubRFrameType = RNACK_OTHER_ERROR;
                        pEseCtx->pT1Params->txSubSFrameType = SFRAME_TYPE_INVALID;
                        TMS_LOG_I(g_tag, "%s: RACK for RNACK_OTHER_ERROR", __FUNCTION__);
                    } else if (RNACK_PARITY_ERROR == lastRecvSubRFrameType) {
                        if (pEseCtx->pT1Params->lastRxPcbBits.bit5 != pEseCtx->seqNumDevice) {
                            // Do Scenario 11
                            pEseCtx->nextT1State = R_PARITY_ERR;
                            pEseCtx->pT1Params->txFrameType = RFRAME;
                            pEseCtx->pT1Params->txSubRFrameType = RNACK_PARITY_ERROR;
                            pEseCtx->pT1Params->txSubSFrameType = SFRAME_TYPE_INVALID;
                            TMS_LOG_I(g_tag,
                                      "%s: RNACK_PARITY_ERROR, seqNumCard error, "
                                      "send RFRAME to request rsp resend from card",
                                      __FUNCTION__);
                        } else {
                            // Do case 10, device should resend I(N(S)).
                            pEseCtx->nextT1State = I_BLK;
                            pEseCtx->pT1Params->txFrameType = IFRAME;

                            TMS_LOG_I(g_tag, "%s: RNACK_PARITY_ERROR, resend IFRAME", __FUNCTION__);
                        }
                    } else if (RNACK_OTHER_ERROR == lastRecvSubRFrameType) {
                        // Do case 11, Device should resend I(N(S))
                        // seqNumDevice has been error, should changed seqNumDevice
                        pEseCtx->seqNumDevice ^= 0x01;
                        pEseCtx->isSeqNumDR = true;
                        pEseCtx->nextT1State = I_BLK;
                        pEseCtx->pT1Params->txFrameType = IFRAME;
                        TMS_LOG_I(g_tag, "%s: RNACK_OTHER_ERROR, resend IFRAME", __FUNCTION__);
                    } else {
                        // unknown R-block type, terminate transmit
                        pEseCtx->nextT1State = STATE_IDEL;
                        pEseCtx->isResyncRecoveryFlag = false;
                        TMS_LOG_E(g_tag, "%s: lastSendIFrame, lastRecvRFrame recovery failed, unkonwn "
                                  "lastRFrameType = %d, recoveryCnt = %d, maxRecoveryCnt = %d",
                                  __FUNCTION__, pEseCtx->pT1Params->lastRxSubRFrameType,
                                  pEseCtx->pT1Params->recoveryCnt, pEseCtx->maxRecoveryCnt);
                    }
                } else if (SFRAME == pEseCtx->pT1Params->txFrameType) {
                    // The last send block is an SFRAME, and
                    // device initiates block is an I-block.
                    if (/*(WTX_RSP == pEseCtx->pT1Params->txSubSFrameType) ||
                        (IFS_RSP == pEseCtx->pT1Params->txSubSFrameType) ||*/
                        (ABORT_RSP == pEseCtx->pT1Params->txSubSFrameType)) {
                        /* Rule 7.3 - When S(… response) was transmitted and an invalid block is received or a
                         * BWT time-out occurs (only with the interface device), an R-block is transmitted.
                         * case 12, Device last send S(WTX_RSP), received vaild R(N(R)),
                         *          recovery case is:
                         *          Card received S(WTX_RSP) invalid, such as, LRC/CRC error.
                         *          R(N(R)) should be received by device, but COS only resend S(...REQ) @zhanghao,
                         *          not R(N(R)), so this case cannot be occured. But if device polling read occurred
                         *          during card's read state, R(N(R)) will be received by device.
                         *          doT1RecoveryAtDecodeRFrame also cannot be called.
                         */
                        TMS_LOG_W(g_tag, "%s: device initiates block is an I-block, lastTxSubSFrameType = %d,"
                                  "but device received R-block, cannot be occurred.",
                                  __FUNCTION__, pEseCtx->pT1Params->txSubSFrameType);
                        // terminate transmit
                        pEseCtx->nextT1State = STATE_IDEL;
                        pEseCtx->isResyncRecoveryFlag = false;
                    } else {
                        /* Rule 7.3 - When S(… request) was transmitted and the received response
                         *            is not S(… response) or a BWT time-out occurs (only with the
                         *            interface device), S(… request) is retransmitted.
                         * case 13, Device last send S(...REQ), received vaild R(N(R)),
                         *          recovery case is:
                         *          Card received S(...REQ) invalid, such as, LRC/CRC error.
                         *          Device received invalid S(...RSP), such as an R(N(R)),
                         *          device should resend S(...REQ).
                         */
                        // Do case 13, device should resend S(...REQ).
                        // All parameters should not be changed, only recoveryCnt should be puls one.
                        TMS_LOG_E(g_tag, "%s: device initiates block is I-block, "
                                  "lastTxSubSFrameType = %d, recoveryCnt = %d", __FUNCTION__,
                                  pEseCtx->pT1Params->txSubSFrameType, pEseCtx->pT1Params->recoveryCnt);
                    }
                } else if (RFRAME == pEseCtx->pT1Params->txFrameType) {
                    // The last send block is an RFRAME, and
                    // device initiates block is an I-block.
                    /* Rule 7.2 — When an R-block was transmitted and an invalid block is received
                     *            or a BWT time-out (with the interface device) occurs, this R-block
                     *            is retransmitted.
                     * case 14, Device last send R(N(R)), received vaild R(N(R)),
                     *          recovery case is:
                     *          Device resend R(N(R)).
                     */
                    // Do case 14, device should resend R(N(R)).
                    // All parameters should not be changed, only recoveryCnt should be puls one.
                    TMS_LOG_E(g_tag, "%s: device initiates block is I-block, "
                              "lastTxSubRFrameType = %d, recoveryCnt = %d", __FUNCTION__,
                              pEseCtx->pT1Params->txSubRFrameType, pEseCtx->pT1Params->recoveryCnt);
                } else {
                    // unknown R-block type, terminate transmit
                    pEseCtx->nextT1State = STATE_IDEL;
                    pEseCtx->isResyncRecoveryFlag = false;
                    TMS_LOG_E(g_tag, "%s: lastSend unknown frame type, recoveryCnt = %d",
                              __FUNCTION__, pEseCtx->pT1Params->recoveryCnt);
                }
            } else if (SFRAME == pEseCtx->pT1Params->txFrameType) {
                // device initiates block is S-block
                // resend the last S-block
                // Do case 13, device should resend S(...REQ).
                // All parameters should not be changed, only recoveryCnt should be puls one.
                TMS_LOG_E(g_tag, "%s: device initiates block is S-block, "
                          "lastTxSubSFrameType = %d, recoveryCnt = %d", __FUNCTION__,
                          pEseCtx->pT1Params->txSubSFrameType, pEseCtx->pT1Params->recoveryCnt);
            } else {
                // device initiates block is R-block
                // device initiates block cannot be an R-block, do nothing
                // terminate transmit
                TMS_LOG_W(g_tag, "%s: device initiates block cannot be an R-block, lastTxFrameType = %d",
                          __FUNCTION__, pEseCtx->pT1Params->txFrameType);
                pEseCtx->nextT1State = STATE_IDEL;
                pEseCtx->isResyncRecoveryFlag = false;
            }
        } else {
            if (!pEseCtx->isResyncRecoveryFlag
                    && (NULL != pEseCtx->pT1Params->pDataTx)) {
                pEseCtx->isResyncRecoveryFlag = true;
                pEseCtx->nextT1State = S_RESYNC_REQ;
                pEseCtx->pT1Params->txFrameType = SFRAME;
                pEseCtx->pT1Params->txSubSFrameType = RESYNC_REQ;
            } else if (pEseCtx->isResyncRecoveryFlag
                       && (NULL != pEseCtx->pT1Params->pDataTx)
                       && (pEseCtx->pT1Params->cipRecoveryCnt < pEseCtx->maxRecoveryCnt)) {
                FreeTxPartMem();
                pEseCtx->isResyncRecoveryFlag = false;
                pEseCtx->nextT1State = S_CIP_REQ;
                pEseCtx->pT1Params->txFrameType = SFRAME;
                pEseCtx->pT1Params->txSubSFrameType = CIP_REQ;
                pEseCtx->pT1Params->cipRecoveryCnt++;
            } else {
                status = ESESTATUS_INVALID_PARAMETER;
                TMS_LOG_E(g_tag, "%s: should be called when received an RFrame, but non-RFrame received,"
                          " lastRecvFrameType = %d, recoveryCnt = %d, maxRecoveryCnt = %d",
                          __FUNCTION__, lastRecvFrameType,
                          pEseCtx->pT1Params->recoveryCnt, pEseCtx->maxRecoveryCnt);
                // terminate transmit
                pEseCtx->nextT1State = STATE_IDEL;
            }
        }
    }else {
        TMS_LOG_E(g_tag, "%s: should be called when received an RFrame, but non-RFrame received,"
                  " lastRecvFrameType = %d, recoveryCnt = %d, maxRecoveryCnt = %d",
                  __FUNCTION__, lastRecvFrameType,
                  pEseCtx->pT1Params->recoveryCnt, pEseCtx->maxRecoveryCnt);
        status = ESESTATUS_INVALID_PARAMETER;
        pEseCtx->nextT1State = STATE_IDEL;
    }
    return status;
}

/******************************************************************************
 * @Function     t1RecvDataStoreInList
 *
 * @Description  This function stores the received data in linked list.
 *
 * @params       pData - 7816-4 APDU response that shall be stored.
 *               dataLen - Length of the APDU response.
 *               totalLen - T=1 total length, contains prologue and epilogue.
 *
 * @Returns      On Success ESESTATUS_SUCCESS else ESESTATUS error.
 *
 ******************************************************************************/
static ESESTATUS t1RecvDataStoreInList(uint8_t *pData, uint16_t dataLen,
                                       uint16_t totalLen)
{
    ESESTATUS status = ESESTATUS_SUCCESS;
    T1RecvBuffListT *pNewNode = NULL;

    if ((0 == dataLen) || (dataLen != (totalLen - getHeaderLen(true)))) {
        // Other error, do nothing
        status = ESESTATUS_INVALID_T1_LEN;
        goto exit;
    }

    pNewNode = (T1RecvBuffListT *) calloc(1, sizeof(T1RecvBuffListT));
    if (NULL == pNewNode) {
        TMS_LOG_E(g_tag, "%s: malloc T1RecvBuffListT error", __FUNCTION__);
        status = ESESTATUS_MEM_EXCEPTION;
        goto exit;
    }

    pNewNode->data.pData = (uint8_t *)calloc(1, dataLen);
    if (NULL == pNewNode->data.pData) {
        TMS_LOG_E(g_tag, "%s: malloc data buffer error", __FUNCTION__);
        free(pNewNode);
        status = ESESTATUS_MEM_EXCEPTION;
        goto exit;
    }

    pNewNode->pNext = NULL;
    pNewNode->data.len = dataLen;
    int err = memcpy_s(pNewNode->data.pData, dataLen, pData, dataLen);
    if (err != EOK) {
        TMS_LOG_E(g_tag, "%s: memcpy_s err. ret:%d", __FUNCTION__, err);
    }
    gRecvTotalLen += dataLen;
    if (NULL == gHead) {
        gHead = pNewNode;
        gCurrent = pNewNode;
    } else {
        gCurrent->pNext = pNewNode;
        gCurrent = pNewNode;
    }

exit:
    return status;
}

/******************************************************************************
 * @Function     T1RecvDataListDestroy
 *
 * @Description  This function release gHead list all node memory.
 *
 ******************************************************************************/
static void T1RecvDataListDestroy()
{
    T1RecvBuffListT *current = NULL;
    while (NULL != gHead) {
        current = gHead;
        gHead = gHead->pNext;
        if (NULL != current->data.pData) {
            free(current->data.pData);
        }
        free(current);
    }
    gRecvTotalLen = 0;
}

static void atrElements(const char *tag, uint16_t tagLen)
{
    UNUSED(tagLen);
    TMS_LOG_D(tag, "%s: len = 0x%02X", __FUNCTION__, gAtr.len);
    TMS_LOG_D(tag, "%s: vendorID = 0x%02X.%02X.%02X.%02X.%02X", __FUNCTION__,
              // gAtr.vendorID[0], gAtr.vendorID[1], gAtr.vendorID[2], gAtr.vendorID[3], gAtr.vendorID[4]
              gAtr.vendorID[0], gAtr.vendorID[1], gAtr.vendorID[2], gAtr.vendorID[3], gAtr.vendorID[4]);
    TMS_LOG_D(tag, "%s: dllIC = 0x%02X", __FUNCTION__, gAtr.dllIC);
    TMS_LOG_D(tag, "%s: bgt = 0x%02X%02X", __FUNCTION__, gAtr.bgt[0], gAtr.bgt[1]);
    TMS_LOG_D(tag, "%s: bwt = 0x%02X%02X", __FUNCTION__, gAtr.bwt[0], gAtr.bwt[1]);
    TMS_LOG_D(tag, "%s: maxFreq = 0x%02X%02X", __FUNCTION__, gAtr.maxFreq[0], gAtr.maxFreq[1]);
    TMS_LOG_D(tag, "%s: defaultIFSC = 0x%02X", __FUNCTION__, gAtr.defaultIFSC);
    TMS_LOG_D(tag, "%s: checksum = 0x%02X", __FUNCTION__, gAtr.checksum);
    TMS_LOG_D(tag, "%s: numChannels = 0x%02X", __FUNCTION__, gAtr.numChannels);
    TMS_LOG_D(tag, "%s: maxIFSC = 0x%02X%02X", __FUNCTION__, gAtr.maxIFSC[0], gAtr.maxIFSC[1]);
    TMS_LOG_D(tag, "%s: capbilities = 0x%02X%02X",
              __FUNCTION__, gAtr.capbilities[0], gAtr.capbilities[1]);
}

static void ResetATR()
{
    AtrInfo atr = {
        .len = 0x13,
        .vendorID = {0x00},
        .dllIC = 0x01,
        .bgt = {0x00, 0x01},
        .bwt = {0x03, 0xE8},
        .maxFreq = {0x4E, 0x20},
        .checksum = 0x00,
        .defaultIFSC = 0xFE,
        .numChannels = 0x01,
        .maxIFSC = {0x00, 0xFF},
        .capbilities = {0x00, 0x14}, // Extended frame length feature doesn't support
    };

    gAtr = atr;
}

/******************************************************************************
 * @Function     t1TransceiveApdu
 *
 * @Description  Send an APDU command and receive its response.
 *
 * @params       pCmdApdu - 7816-4 APDU that shall be sent.
 *               cmdLen - Length of the apduCmd to be sent.
 *
 * @Returns          On success return ESESTATUS_SUCCESS or else ESESTATUS error.
 *
 ******************************************************************************/
ESESTATUS t1TransceiveApdu(
    uint8_t *pCmdApdu, uint16_t cmdLen)
{
    ESESTATUS status = ESESTATUS_SUCCESS;
    TMS_LOG_D(g_tag, "%s: Enter", __FUNCTION__);
    status = initIFrameFromCmdApdu(pCmdApdu, cmdLen);
    if (ESESTATUS_SUCCESS != status) {
        goto cleanup;
    }

    status = transceiveProcess();
    if (ESESTATUS_SUCCESS != status) {
        TMS_LOG_E(g_tag, "%s: t1TransceiveApdu failed, status = %d", __FUNCTION__, status);
    }

cleanup:
    DeInitIFrameFromCmdApdu();
    TMS_LOG_D(g_tag, "%s:Exit status = %d", __FUNCTION__, status);
    return status;
}

/******************************************************************************
 * @Function     t1CipReq
 *
 * @Description  Send CIP request(S-Block). CIP, Communication Interface Parameters,
 *               definition by GPC APDU Transport over SPI/I2C Version 1.0.
 *               TMS definition: response is ATR, refer to AtrInfo struct.
 *               Note1:T1 request for ATR, the frame's INFO length should be 1 byte,
 *                     do not support 2 bytes length.
 *                     So, CIP and ATR request frame's INFO LEN is 1 byte.
 *                     And epiLogue field should use LRC (1 byte).
 *               Note2:T1 response for ATR, the frame's INFO length should be 1 byte,
 *                     do not support 2 bytes length.
 *                     So, CIP and ATR response frame's INFO LEN is 1 byte.
 *                     And epiLogue field should use LRC (1 byte).
 *
 * @Returns      On success return ESESTATUS_SUCCESS or else ESESTATUS error.
 *
******************************************************************************/
ESESTATUS t1CIPReq()
{
    ESESTATUS status = ESESTATUS_SUCCESS;
    SEContext *pEseCtx = GetEseCtx();
    T1Params *pT1Params = (T1Params *)calloc(1, sizeof(T1Params));
    if (NULL == pT1Params) {
        TMS_LOG_E(g_tag, "%s: Tx data malloc memory failed", __FUNCTION__);
        return ESESTATUS_MEM_EXCEPTION;
    }

    pEseCtx->pT1Params = pT1Params;
    pT1Params->isDeviceChaining = false;
    pEseCtx->nextT1State = S_CIP_REQ;
    pT1Params->txFrameType = SFRAME;
    pT1Params->txSubSFrameType = CIP_REQ;

    status = transceiveProcess();
    free(pT1Params);
    pEseCtx->pT1Params = NULL;

    return status;
}

/******************************************************************************
 * @Function     t1ATRReq
 *
 * @Description  Send ATR request(S-Block). Refer to AtrInfo struct.
 *               Note1:T1 request for ATR, the frame's INFO length should be 1 byte,
 *                     do not support 2 bytes length.
 *                     So, CIP and ATR request frame's INFO LEN is 1 byte.
 *                     And epiLogue field should use LRC (1 byte).
 *               Note2:T1 response for ATR, the frame's INFO length should be 1 byte,
 *                     do not support 2 bytes length.
 *                     So, CIP and ATR response frame's INFO LEN is 1 byte.
 *                     And epiLogue field should use LRC (1 byte).
 *
 * @Returns      On success return ESESTATUS_SUCCESS or else ESESTATUS error.
 *
******************************************************************************/
ESESTATUS t1ATRReq()
{
    ESESTATUS status = ESESTATUS_SUCCESS;
    SEContext *pEseCtx = GetEseCtx();
    T1Params *pT1Params = (T1Params *)calloc(1, sizeof(T1Params));
    if (NULL == pT1Params) {
        TMS_LOG_E(g_tag, "%s: Tx data malloc memory failed", __FUNCTION__);
        return ESESTATUS_MEM_EXCEPTION;
    }

    pEseCtx->pT1Params = pT1Params;
    pT1Params->isDeviceChaining = false;
    pEseCtx->nextT1State = S_ATR_REQ;
    pT1Params->txFrameType = SFRAME;
    pT1Params->txSubSFrameType = ATR_REQ;

    status = transceiveProcess();
    free(pT1Params);
    pEseCtx->pT1Params = NULL;

    return status;
}

/******************************************************************************
 * @Function     t1IFSDeviceReq
 *
 * @Description  Send IFSD request(S-Block)
 *
 * @Returns      On success return ESESTATUS_SUCCESS or else ESESTATUS error.
 *
 ******************************************************************************/
ESESTATUS t1IFSDeviceReq()
{
    ESESTATUS status = ESESTATUS_SUCCESS;
    SEContext *pEseCtx = GetEseCtx();
    T1Params *pT1Params = (T1Params *)calloc(1, sizeof(T1Params));
    if (NULL == pT1Params) {
        TMS_LOG_E(g_tag, "%s: Tx data malloc memory failed", __FUNCTION__);
        return ESESTATUS_MEM_EXCEPTION;
    }

    pEseCtx->pT1Params = pT1Params;
    pT1Params->isDeviceChaining = false;
    pEseCtx->nextT1State = S_IFS_REQ;
    pT1Params->txFrameType = SFRAME;
    pT1Params->txSubSFrameType = IFS_REQ;

    status = transceiveProcess();
    free(pT1Params);
    pEseCtx->pT1Params = NULL;

    return status;
}

/******************************************************************************
 * @Function     t1PropEndApduReq
 *
 * @Description  Send PROP END APDU request(S-Block)
 *
 * @Returns      On success return ESESTATUS_SUCCESS or else ESESTATUS error.
 *
 ******************************************************************************/
ESESTATUS t1PropEndApduReq()
{
    ESESTATUS status = ESESTATUS_SUCCESS;
    SEContext *pEseCtx = GetEseCtx();
    T1Params *pT1Params = (T1Params *)calloc(1, sizeof(T1Params));
    if (NULL == pT1Params) {
        TMS_LOG_E(g_tag, "%s: Tx data malloc memory failed", __FUNCTION__);
        return ESESTATUS_MEM_EXCEPTION;
    }

    pEseCtx->pT1Params = pT1Params;
    pT1Params->isDeviceChaining = false;
    pEseCtx->nextT1State = S_PROP_END_APDU_REQ;
    pT1Params->txFrameType = SFRAME;
    pT1Params->txSubSFrameType = PROP_END_APDU_REQ;

    status = transceiveProcess();
    free(pT1Params);
    pEseCtx->pT1Params = NULL;

    return status;
}

/******************************************************************************
 * Function     t1RecvDataGet
 *
 * Description  This function get the len and received data.
 *
 * @params       ppData - a pointer address to the received data.
 *               dataLen - a pointer to the length of the received data.
 *
 * Returns      On success return ESESTATUS_SUCCESS or else ESESTATUS error.
 *
 ******************************************************************************/
ESESTATUS t1RecvDataGet(uint8_t **ppData, uint16_t *dataLen)
{
    ESESTATUS status = ESESTATUS_SUCCESS;

    T1RecvBuffListT *pNextNode = NULL;
    uint32_t offset = 0;
    uint8_t *pBuff = NULL;

    if ((NULL == gHead) || (NULL == ppData) || (0 == gRecvTotalLen)) {
        status =  ESESTATUS_INVALID_PARAMETER;
        TMS_LOG_E(g_tag, "%s: ESESTATUS_INVALID_PARAMETER, gHead = %d, ppData = %d, gRecvTotalLen = %u",
                  __FUNCTION__, NULL == gHead, NULL == ppData, gRecvTotalLen);
        goto cleanup;
    }

    pBuff = (uint8_t *)calloc(1, gRecvTotalLen);
    if (NULL == pBuff) {
        status =  ESESTATUS_MEM_EXCEPTION;
        TMS_LOG_E(g_tag, "%s: malloc memory failed", __FUNCTION__);
        goto cleanup;
    }

    pNextNode = gHead;
    while (pNextNode != NULL) {
        int err = memcpy_s((pBuff + offset), gRecvTotalLen - offset, pNextNode->data.pData, pNextNode->data.len);
        if (err != EOK) {
            TMS_LOG_E(g_tag, "%s: memcpy_s err. ret:%d", __FUNCTION__, err);
        }
        offset += pNextNode->data.len;
        pNextNode = pNextNode->pNext;
    }
    *dataLen = offset;
    *ppData = pBuff;

cleanup:
    T1RecvDataListDestroy();

    return status;
}

AtrInfo *GetAtr(void)
{
    return &gAtr;
}
