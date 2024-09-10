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

#include <pthread.h>

#include <android-base/file.h>

#if defined (USE_TMS_NFC) || defined (USE_C1)
    #include "tmsSecureString.h"
#else
    #include "securec.h"
#endif

#include "tmsDlCommonUtils.h"
#include "tmsReeDlCommonUtils.h"
#include "tmsDlNciUtils.h"
#include "tmslog.h"

#include "TmsRee.h"
using vendor::tms::TmsRee;

#include "SEApi.h"

#ifdef TAG
#undef TAG
#endif
#define TAG g_tag

static const char g_tag[] = "TmsCosDl:NciUtils";

bool g_nfccRwThreadRecovery = false;

#ifdef I2C_PATCH
    bool g_fixedI2CFlag = true;
#else
    // fixed compile ld error
    bool g_fixedI2CFlag = false;
#endif

bool GetFixedI2cFlag(void)
{
    return g_fixedI2CFlag;
}

void SetFixedI2cFlag(bool value)
{
    g_fixedI2CFlag = value;
}

static int NciReceive(int handle, unsigned char *pBuff, int buffLen)
{
    int numRead;
    struct timeval tv;
    fd_set rfds;
    int ret;
    FD_ZERO(&rfds);
    FD_SET(handle, &rfds);
    tv.tv_sec = 2;  // 2s
    tv.tv_usec = 1;
    ret = select(handle + 1, &rfds, NULL, NULL, &tv);
    if (ret <= 0) {
        TMS_LOG_W(g_tag, "%s: select may be timeout, errno = %d",
                  __FUNCTION__, errno);
        return 0;
    }

    ret = read(handle, pBuff, 3);  // 3 length
    if (ret <= 0) {
        TMS_LOG_E(g_tag, "%s: read head error, ret = %d, errno = %d",
                  __FUNCTION__, ret, errno);
        return 0;
    }
    numRead = 3;  // 3 length
    if (pBuff[2] + 3 > buffLen) {  // pBuff[2] + 3
        TMS_LOG_E(g_tag, "%s: read1 len error, buffLen = %d, expectLen = %d",
                  __FUNCTION__, buffLen, pBuff[2] + 3);  // pBuff[2] + 3
        return 0;
    }

#ifdef I2C_PATCH
    int len = pBuff[2];
    if (g_fixedI2CFlag && len == 1) {
        len++;
    }
    ret = read(handle, &pBuff[3], len);  // pBuff[3]
#else
    ret = read(handle, &pBuff[3], pBuff[2]);  // &pBuff[3], pBuff[2]
#endif
    if (ret <= 0) {
#ifdef I2C_PATCH
        g_fixedI2CFlag = false;
        TMS_LOG_E(g_tag, "%s: read data error, readLen[%d], rspLen[%d]",
                  __FUNCTION__, len, pBuff[2]);  // pBuff[2]
#else
        TMS_LOG_E(g_tag, "%s: read data error", __FUNCTION__);
#endif
        return 0;
    }
    numRead += ret;
#ifdef I2C_PATCH
    if (g_fixedI2CFlag && pBuff[2] == 1) {
        numRead--;
    }
#endif

    printHexPacket(g_tag, strlen(g_tag), "Rx", 4, pBuff, numRead);  // length 4
    return numRead;
}

/**
 NciCmdProcess function write a nci cmd and wait for rsp, or wait for ntf event.
 * Function         NciCmdProcess
 *
 * Description      function write a nci cmd and wait for rsp, or wait for ntf event.
 * Parameters       handle, the NFC device node handler.
 *                  cmd, execute cmd. If cmd is null, only wait for NTF response.
 * Returns          On success return true or else false.
 */
static bool NciCmdProcess(int handle, unsigned char *cmd, int cmdLen,
                          unsigned char *readbuffer, int *rspLenPtr)
{
    int retryCnt = 0, ret = 0;

retry_cmd_write:
    if (cmd != nullptr) {
        ret = write(handle, cmd, cmdLen);
        printHexPacket(g_tag, strlen(g_tag), "Tx", 4, cmd, cmdLen);  // length 4
        if (ret < 0 && (retryCnt < 8)) {  // retryCnt 8
            TMS_LOG_E(g_tag, "%s write cmd failure, retry = %d", __FUNCTION__, retryCnt + 1);
            usleep(5 * 1000);  // 5 * 1000 us
            retryCnt++;
            goto retry_cmd_write;
        } else if (ret < 0) {
            TMS_LOG_E(g_tag, "%s write cmd failed!!!", __FUNCTION__);
            return false;
        }
    }

    (void)memset_s(readbuffer, *rspLenPtr, 0, *rspLenPtr);
    int readLen = NciReceive(handle, readbuffer, *rspLenPtr);
    if (readLen) {
        *rspLenPtr = readLen;
    } else {
        TMS_LOG_E(g_tag, "nci receive data timeout");
    }

    return readLen ? true : false;
}

static uint32_t parseFwVerFromNciRsp(uint8_t *pNtf, uint16_t *pLen)
{
    uint32_t nfcFWVer = INVALID_FW_VER;
    if (pNtf == NULL || *pLen == 0x00) {
        return -1;
    }

    if (pNtf[0] == NCI_MT_RSP && ((pNtf[1] & NCI_OID_MASK) == NCI_MSG_CORE_RESET)) {
        if (pNtf[2] == 0x01 && pNtf[3] == 0x00) { // pNtf[2] == 0x01 && pNtf[3] == 0x00
            TMS_LOG_D(g_tag, "CORE_RESET_RSP NCI2.0");
        } else if (pNtf[2] == 0x03 && pNtf[3] == 0x00) {  // pNtf[2] == 0x03 && pNtf[3] == 0x00
            TMS_LOG_D(g_tag, "CORE_RESET_RSP NCI1.0");
        }
    } else if (pNtf[0] == NCI_MT_NTF && ((pNtf[1] & NCI_OID_MASK) == NCI_MSG_CORE_RESET)) {
        if (pNtf[3] == CORE_RESET_TRIGGER_TYPE_CORE_RESET_CMD_RECEIVED ||  // pNtf[3]
                pNtf[3] == CORE_RESET_TRIGGER_TYPE_POWERED_ON) {  // pNtf[3]
            TMS_LOG_D(g_tag, "CORE_RESET_NTF NCI2.0 reason CORE_RESET_CMD received !");
            int len = pNtf[2] + 2; /* include 2 byte header */
            nfcFWVer = (((uint32_t)pNtf[len - 2]) << 16U) |
                       (((uint32_t)pNtf[len - 1]) << 8U) | pNtf[len];
            TMS_LOG_I(g_tag, "TmsNci> FW Version: %x.%x.%x",
                      pNtf[len - 2], pNtf[len - 1], pNtf[len]);  // pNtf[len - 2], pNtf[len - 1]
        } else {
            uint32_t i;
            char printBuffer[*pLen * 3 + 1];  // *pLen * 3 + 1
            (void)memset_s(printBuffer, sizeof(printBuffer), 0, sizeof(printBuffer));
            for (i = 0; i < *pLen; i++) {
                int maxLen = (int)(sizeof(printBuffer) - (i * 2));
                // &printBuffer[i * 2], maxLen, 3, "%02X"
                snprintf_s(&printBuffer[i * 2], maxLen, 3, "%02X", pNtf[i]);
            }
            TMS_LOG_D(g_tag, "CORE_RESET_NTF received !");
            TMS_LOG_D(g_tag, "len = %3d > %s", *pLen, printBuffer);
            /* Retreive reset ntf reason code irrespective of NCI 1.0 or 2.0 */
            if (pNtf[3] == FW_DBG_REASON_AVAILABLE) {  // pNtf[3]
                nfcFWVer = INVALID_FW_VER;
            }
        } /* Parsing CORE_INIT_RSP */
    } else if (pNtf[0] == NCI_MT_RSP
               && ((pNtf[1] & NCI_OID_MASK) == NCI_MSG_CORE_INIT)) {
        TMS_LOG_D(g_tag, "CORE_INIT_RSP NCI1.0 received !");
        int len = pNtf[2] + 2; /* include 2 byte header */
        nfcFWVer = (((uint32_t)pNtf[len - 2]) << 16U) |
                   (((uint32_t)pNtf[len - 1]) << 8U) | pNtf[len];
        TMS_LOG_I(g_tag, "TmsNci> FW Version: %x.%x.%x",
                  pNtf[len - 2], pNtf[len - 1], pNtf[len]);  // pNtf[len - 2], pNtf[len - 1]
    }

    return nfcFWVer;
}

static uint32_t parseBlVerFromNciRsp(uint8_t *p_ntf, uint16_t *p_len)
{
    uint32_t nfcBlVer = INVALID_FW_VER;
    if (p_ntf == NULL || *p_len == 0x00) {
        return -1;
    }

    if ((*p_len > 3) && // *p_len > 3
        // (p_ntf[0] == 0x4F) && (p_ntf[1] == 0x25) && (p_ntf[2] == 0x05)
        (p_ntf[0] == 0x4F) && (p_ntf[1] == 0x25) && (p_ntf[2] == 0x05) &&
        // (p_ntf[3] == 0x00)
        (p_ntf[3] == 0x00)) {
        int len = p_ntf[2] + 2; // include 2 byte header, p_ntf[2] + 2
        nfcBlVer = (((uint32_t)p_ntf[len - 3]) << 24U) |  // p_ntf[len - 3]) << 24U
                   (((uint32_t)p_ntf[len - 2]) << 16U) |  // p_ntf[len - 2]) << 16U
                   (((uint32_t)p_ntf[len - 1]) << 8U)   | p_ntf[len];  // p_ntf[len - 1]) << 8U
        TMS_LOG_I(g_tag, "TmsNci> BL Version: %x.%x.%x.%x",
                  // p_ntf[len - 3] p_ntf[len - 2] p_ntf[len - 1]
                  p_ntf[len - 3], p_ntf[len - 2], p_ntf[len - 1], p_ntf[len]);
    }
    return nfcBlVer;
}

static ChipInfo ComputeNfccChipInfo(ChipInfo chipInfo)
{
    if (INVALID_CHIP_TYPE == chipInfo.chipType) {
        if (chipInfo.nfccBlVer < NFCC_BL_B400) {
            chipInfo.chipType = CHIP_EC1;
        } else if (chipInfo.nfccBlVer != INVALID_FW_VER) {
            chipInfo.chipType = CHIP_EC2;
        } else {
            TMS_LOG_E(g_tag, "%s: invalid chip type info, [%08X]", __FUNCTION__, chipInfo.nfccBlVer);
        }
    }

    if (INVALID_CHIP_KEY_TYPE == chipInfo.chipKeyType) {
        if (chipInfo.nfccBlVer <= NFCC_BL_B206) {
            chipInfo.chipKeyType = CHIP_T_KEY;
        } else if (INVALID_FW_VER != chipInfo.fwVer) {
            if (chipInfo.fwVer & 0x00001000) {
                chipInfo.chipKeyType = CHIP_R_KEY;
            } else {
                chipInfo.chipKeyType = CHIP_T_KEY;
            }
        } else if (INVALID_FW_VER == chipInfo.nfccBlVer) {
            // nfccBlVer && fwVer are both INVALID_FW_VER
            // default chipKeyType to release key
            chipInfo.chipKeyType = CHIP_R_KEY;
            TMS_LOG_E(g_tag, "%s: invalid chip key info, set release key for default", __FUNCTION__);
        } else {
            // Invalid FW and invalid key type.
            // [B304, FFFF), try compute it to CHIP_R_KEY
            chipInfo.chipKeyType = CHIP_R_KEY;
        }
    }

    return chipInfo;
}

static ChipInfo GetNfccChipInfoFromFW()
{
    uint32_t fwVer = INVALID_FW_VER;
    uint32_t blVer = INVALID_FW_VER;
    ChipInfo chipInfo;
    unsigned char coreResetCmd[] = {0x20, 0x00, 0x01, 0x00};
    unsigned char coreInitCmd[] = {0x20, 0x01, 0x00};
    unsigned char coreInitCmd20[] = {0x20, 0x01, 0x02, 0x00, 0x00};
    unsigned char getBlVerCmd[] = {0x2F, 0x25, 0x05, 0x00, 0x4A, 0x02, 0x00, 0x00};
    unsigned char nciRsp[NCI_RSP_LEN_MAX] = {0};
    int rspLen = NCI_RSP_LEN_MAX, retryCnt = 0;
    bool ret = false;
    SEContext *pEseCtx =  GetEseCtx();

retry_core_reset:
    if (!GetThreadRunning()) {
        TMS_LOG_E(g_tag, "%s, exit thread", __FUNCTION__);
        goto fail;
    }
    rspLen = sizeof(nciRsp);
    ret = NciCmdProcess(pEseCtx->devHandle,
                        coreResetCmd, sizeof(coreResetCmd),
                        nciRsp, &rspLen);
    if (!ret && (retryCnt < 4)) {  // retryCnt < 4
        TMS_LOG_W(g_tag, "%s Retry[%d]: NCI_CORE_RESET", __FUNCTION__, retryCnt);
        usleep(5 * 1000);  // 5 * 1000 us
        retryCnt++;
        goto retry_core_reset;
    } else if (!ret || (retryCnt >= 4)) {  // retryCnt >= 4
        TMS_LOG_E(g_tag, "%s NCI_CORE_RESET failed!!! Retry[%d]", __FUNCTION__, retryCnt);
        goto fail;
    }
    // Only the first reset cmd no response, retry it

    // Fixed 62010100 (NFCEE_MODE_SET_NTF) will break CORE_RESET_CMD_RSP.
    for (int i = 0; i < 10 && GetThreadRunning(); i++) {
        if ((nciRsp[0] == 0x40)
                && (nciRsp[1] == 0x00)) {
            if (nciRsp[3] != 0x00) {  // nciRsp[3] != 0x00
                retryCnt++;
                goto retry_core_reset;
            }

            if ((nciRsp[2] == 0x03)  // nciRsp[2] == 0x03
                    // NCI 1.0 need CORE_INIT_CMD to get FW version, nciRsp[4] == 0x10
                    && (nciRsp[4] == 0x10)) {
                TMS_LOG_D(g_tag, "%s: NCI 1.0 process", __FUNCTION__);
                rspLen = sizeof(nciRsp);
                ret = NciCmdProcess(pEseCtx->devHandle,
                                    coreInitCmd, sizeof(coreInitCmd),
                                    nciRsp, &rspLen);
                if (ret) {
                    fwVer = parseFwVerFromNciRsp(nciRsp, (uint16_t *) &rspLen);
                } else {
                    fwVer = INVALID_FW_VER;
                }
            } else if (nciRsp[2] == 0x01) { // May be NCI 2.0
                // Wait for CORE_RESET_NTF
                TMS_LOG_D(g_tag, "%s: NCI 2.0 process", __FUNCTION__);
                rspLen = sizeof(nciRsp);
                ret = NciCmdProcess(pEseCtx->devHandle,
                                    nullptr, 0,
                                    nciRsp, &rspLen);
                if (ret) {
                    fwVer = parseFwVerFromNciRsp(nciRsp, (uint16_t *) &rspLen);
                } else {
                    fwVer = INVALID_FW_VER;
                }

                rspLen = sizeof(nciRsp);
                ret = NciCmdProcess(pEseCtx->devHandle,
                                    coreInitCmd20, sizeof(coreInitCmd20),
                                    nciRsp, &rspLen);
            } else {
                // Cannot be NCI 1.0 or 2.0
                // Cannot get FW version
            }
            break;
        } else {
            // Wait for CORE_RESET_CMD_RSP
            TMS_LOG_D(g_tag, "%s: Wait for CORE_RESET_CMD_RSP", __FUNCTION__);
            rspLen = sizeof(nciRsp);
            NciCmdProcess(pEseCtx->devHandle,
                          nullptr, 0,
                          nciRsp, &rspLen);
        }
    }

    if (ret) {
        rspLen = sizeof(nciRsp);
        ret = NciCmdProcess(pEseCtx->devHandle,
                            getBlVerCmd, sizeof(getBlVerCmd),
                            nciRsp, &rspLen);
        blVer = parseBlVerFromNciRsp(nciRsp, (uint16_t *) &rspLen);
    } else {
        blVer = INVALID_FW_VER;
    }
    chipInfo.fwVer = fwVer;
    chipInfo.nfccBlVer = blVer;

fail:
    TMS_LOG_D(g_tag, "FW version = 0x%x\n", fwVer);

    return chipInfo;
}

static uint32_t GetBlVer(DownloadCmdOp *pDlCmdOp)
{
    uint32_t blVer = INVALID_FW_VER;
    for (int i = 0; i < 2; i++) {  // retry 2 times
        ESESTATUS status = execApduCmdChkRes(APDU_GET_BL_VER, strlen(APDU_GET_BL_VER), pDlCmdOp);
        if (ESESTATUS_SUCCESS == status) {
            int len = pDlCmdOp->rspLen;
            if (len < 6) { // len < 6(4-byte version + 2-byte sw)
                TMS_LOG_E(g_tag, "%s: get BL ver failed. rcv len:%d", __FUNCTION__, len);
                break;
            }
            uint8_t sw1 = pDlCmdOp->rsp[len - 2];
            uint8_t sw2 = pDlCmdOp->rsp[len - 1];
            if ((0x90 == sw1) && (0x00 == sw2)) { // ((0x90 == sw1) && (0x00 == sw2))
                blVer = (((uint32_t)pDlCmdOp->rsp[0]) << 24)  // rsp[0]) << 24
                          | (((uint32_t)pDlCmdOp->rsp[1]) << 16)  // rsp[1]) << 16
                          | (((uint32_t)pDlCmdOp->rsp[2]) << 8)  // rsp[2]) << 8
                          | pDlCmdOp->rsp[3];  // rsp[3]
            } else {
                TMS_LOG_E(g_tag, "%s: get BL ver failed [%d][%02X%02X]", __FUNCTION__, len, sw1, sw2);
            }
            break;
        }
    }
    return blVer;
}

static ChipInfo GetOTP(DownloadCmdOp *pDlCmdOp)
{
    ChipInfo chipInfo;
    int len = 0;
    uint8_t sw1 = 0x00;
    uint8_t sw2 = 0x00;
    for (int i = 0; i < 2; i++) {  // retry 2 times
        ESESTATUS status = execApduCmdChkRes(APDU_GET_OTP, strlen(APDU_GET_OTP), pDlCmdOp);
        len = pDlCmdOp->rspLen;
        if ((ESESTATUS_SUCCESS != status) || (len < 20)) { // len < 20
            TMS_LOG_E(g_tag, "%s: get OTP failed status[%d]", __FUNCTION__, status);
            continue;
        }
        sw1 = pDlCmdOp->rsp[len - 2];
        sw2 = pDlCmdOp->rsp[len - 1];
        if ((0x90 == sw1) && (0x00 == sw2)) { // ((0x90 == sw1) && (0x00 == sw2))
            if ((CHIP_EC1 == pDlCmdOp->rsp[13]) || (CHIP_EC2 == pDlCmdOp->rsp[13])) { //13 chip type offset
                chipInfo.chipType = pDlCmdOp->rsp[13]; // chipInfo.chipType = pDlCmdOp->rsp[13]
            } else {
                chipInfo.chipType = INVALID_CHIP_TYPE;
            }

            if ((CHIP_T_KEY == pDlCmdOp->rsp[16]) || (CHIP_R_KEY == pDlCmdOp->rsp[16])) { // 16 chip key offset
                chipInfo.chipKeyType = pDlCmdOp->rsp[16]; // chipInfo.chipKeyType = pDlCmdOp->rsp[16]
            } else {
                chipInfo.chipKeyType = INVALID_CHIP_KEY_TYPE;
            }
            break;
        } else {
            TMS_LOG_E(g_tag, "%s: get OTP failed sw[0x%02X%02X]", __FUNCTION__, sw1, sw2);
        }
    }
    return chipInfo;
}

static ChipInfo GetNfccChipInfoFromBL(void *arg)
{
    DownloadCmdOp *pDlCmdOp = (DownloadCmdOp *)arg;
    bool isNewRspMem = false;
    ChipInfo chipInfo;

    if (nullptr == pDlCmdOp->rsp) {
        pDlCmdOp->rsp = new uint8_t[NCI_RSP_LEN_MAX];
        pDlCmdOp->rspLen = NCI_RSP_LEN_MAX;
        if (nullptr == pDlCmdOp->rsp) {
            TMS_LOG_E(g_tag, "%s: new res memory failed", __FUNCTION__);
            return chipInfo;
        }
        isNewRspMem = true;
    }

    chipInfo = GetOTP(pDlCmdOp);
    chipInfo.nfccBlVer = GetBlVer(pDlCmdOp);

    TMS_LOG_D(g_tag, "%s: BL[%08X] CT[%02X] KT[%02X]",
              __FUNCTION__, chipInfo.nfccBlVer, chipInfo.chipType, chipInfo.chipKeyType);
    if (isNewRspMem && (nullptr != pDlCmdOp->rsp)) {
        delete[] pDlCmdOp->rsp;
        pDlCmdOp->rsp = nullptr;
        pDlCmdOp->rspLen = 0;
    }

    if ((INVALID_CHIP_TYPE == chipInfo.chipType) || (INVALID_CHIP_KEY_TYPE == chipInfo.chipKeyType)) {
        TMS_LOG_E(g_tag, "%s: get chip info exception, compute chip info", __FUNCTION__);
        chipInfo = ComputeNfccChipInfo(chipInfo);
        TMS_LOG_I(g_tag, "%s: BL[%08X] CT[%02X] KT[%02X]",
                  __FUNCTION__, chipInfo.nfccBlVer, chipInfo.chipType, chipInfo.chipKeyType);
    }

    return chipInfo;
}

static bool NciCmdSendAndChkRsp(unsigned char *cmd, int cmdLen,
                                unsigned char *rspChkHeader, int rspChkHeaderLen)
{
    unsigned char nciRsp[NCI_RSP_LEN_MAX] = {0};
    int rspLen = NCI_RSP_LEN_MAX, retryCnt = 0;
    bool ret = false;
    SEContext *pEseCtx = GetEseCtx();

retry_nci_cmd:
    if (!GetThreadRunning()) {
        TMS_LOG_E(g_tag, "%s exit thread", __FUNCTION__);
        goto exit;
    }
    rspLen = sizeof(nciRsp);
    ret = NciCmdProcess(pEseCtx->devHandle,
                        cmd, cmdLen,
                        nciRsp, &rspLen);
    if (!ret && (retryCnt < 4)) {  // retryCnt < 4
        TMS_LOG_W(g_tag, "%s Retry[%d]: NCI_CORE_RESET", __FUNCTION__, retryCnt);
        usleep(5 * 1000);  // 5 * 1000 us
        retryCnt++;
        goto retry_nci_cmd;
    } else if (!ret) {
        TMS_LOG_E(g_tag, "%s NCI_CORE_RESET failed!!! Retry[%d]", __FUNCTION__, retryCnt);
        goto exit;
    }

    retryCnt = 0;

chk_nci_rsp:
    if (!GetThreadRunning()) {
        TMS_LOG_E(g_tag, "%s exit thread", __FUNCTION__);
        goto exit;
    }
    if ((rspLen >= 4) && // valid response header min length, rspLen >= 4
        (rspChkHeaderLen >= 4) && // rspChkHeaderLen >= 4
        (nciRsp[0] == rspChkHeader[0]) &&
        ((nciRsp[1] & NCI_OID_MASK) == rspChkHeader[1])) {
        if (nciRsp[3] == rspChkHeader[3]) { // check the response status, rspChkHeader[3]
            ret = true;
        } else {
            ret = false;
        }
    } else {
        rspLen = sizeof(nciRsp);
        ret = NciCmdProcess(pEseCtx->devHandle,
                            NULL, 0,
                            nciRsp, &rspLen);
        if (!ret) {
            if (retryCnt < 2) {  // retryCnt < 2
                TMS_LOG_W(g_tag, "%s Retry[%d]: receive NCI RSP", __FUNCTION__, retryCnt);
                retryCnt++;
                goto chk_nci_rsp;
            } else {
                TMS_LOG_E(g_tag, "%s NCI_RSP failed!!! Retry[%d]", __FUNCTION__, retryCnt);
            }
        } else {
            goto chk_nci_rsp;
        }
    }

exit:
#ifdef COS_DLD_TEST
    TMS_LOG_D(g_tag, "%s exit, ret = %d", __FUNCTION__, ret);
#endif
    return ret;
}

static bool Nci20ResetInit()
{
    unsigned char coreInitCmd[] = {0x20, 0x01, 0x02, 0x00, 0x00};
    unsigned char coreResetCmd[] = {0x20, 0x00, 0x01, 0x00};
    unsigned char nciRsp[NCI_RSP_LEN_MAX] = {0};
    int rspLen = NCI_RSP_LEN_MAX, retryCnt = 0;
    SEContext *pEseCtx = GetEseCtx();

retry_core_reset:
    if (!GetThreadRunning()) {
        TMS_LOG_E(g_tag, "%s exit thread", __FUNCTION__);
        return false;
    }
    // 1. CORE_RESET_CMD and RSP
    rspLen = sizeof(nciRsp);
    bool ret = NciCmdProcess(pEseCtx->devHandle,
                             coreResetCmd, sizeof(coreResetCmd),
                             nciRsp, &rspLen);
    if (!ret && (retryCnt < 4)) {  // retryCnt < 4
        TMS_LOG_W(g_tag, "%s Retry[%d]: NCI_CORE_RESET", __FUNCTION__, retryCnt);
        usleep(5 * 1000);  // 5 * 1000 us
        retryCnt++;
        goto retry_core_reset;
    } else if (!ret) {
        TMS_LOG_E(g_tag, "%s NCI_CORE_RESET failed!!! Retry[%d]", __FUNCTION__, retryCnt);
        return false;
    }
    // Only the first reset cmd no response, retry it

    // 2. CORE_RESET_NTF
    //    Once the DH has received CORE_RESET_RSP, it SHALL NOT send any other command
    //    until it receives CORE_RESET_NTF
    ret = false;
    for (int i = 0; i < 10 && GetThreadRunning(); i++) {  // i = 0; i < 10
        rspLen = sizeof(nciRsp);
        ret = NciCmdProcess(pEseCtx->devHandle,
                            NULL, 0,
                            nciRsp, &rspLen);
        if (nciRsp[0] == NCI_MT_NTF && ((nciRsp[1] & NCI_OID_MASK) == NCI_MSG_CORE_RESET)) {
            ret = true;
            break;
        } else {
            ret = false;
        }
    }

    if (!ret) {
        TMS_LOG_E(g_tag, "%s CORE_RESET_NTF not received!!! RESET failed", __FUNCTION__);
        return false;
    }

    // 3. CORE_INIT_CMD and RSP
    rspLen = sizeof(nciRsp);
    ret = NciCmdProcess(pEseCtx->devHandle,
                        coreInitCmd, sizeof(coreInitCmd),
                        nciRsp, &rspLen);
    if (nciRsp[0] == NCI_MT_RSP
            && ((nciRsp[1] & NCI_OID_MASK) == NCI_MSG_CORE_INIT)) {
        ret = (nciRsp[3] == 0x00);  // nciRsp[3] == 0x00
    } else {
        ret = false;
        TMS_LOG_E(g_tag, "%s CORE_INIT_CMD RSP not received!!! INIT failed", __FUNCTION__);
    }

    return ret;
}

static void *NciCmdRspChkThread(void *arg)
{
    if (!RegisterExitSignal()) {
        return nullptr;
    }

    DownloadCmdOp *pDlCmdOp = (DownloadCmdOp *)arg;
    SetThreadRunning(true);
    *((bool *)(pDlCmdOp->pResStatus)) = NciCmdSendAndChkRsp(
        pDlCmdOp->cmd, pDlCmdOp->cmdLen,
        pDlCmdOp->rspChk, pDlCmdOp->rspChkLen);
    SetThreadRunning(false);

    pthread_mutex_lock(pDlCmdOp->pMutex);
    pthread_cond_signal(pDlCmdOp->pCond);
    pthread_mutex_unlock(pDlCmdOp->pMutex);

    return nullptr;
}

static void *EseSoftResetThread(void *arg)
{
    if (!RegisterExitSignal()) {
        return nullptr;
    }

    DownloadCmdOp *pDlCmdOp = (DownloadCmdOp *)arg;
    SetThreadRunning(true);
    bool ret = Nci20ResetInit();
    if (ret) {
        *((bool *)(pDlCmdOp->pResStatus)) =
            NciCmdSendAndChkRsp(pDlCmdOp->cmd, pDlCmdOp->cmdLen,
                                pDlCmdOp->rspChk, pDlCmdOp->rspChkLen);
    } else {
        *((bool *)(pDlCmdOp->pResStatus)) = false;
    }
    SetThreadRunning(false);

    pthread_mutex_lock(pDlCmdOp->pMutex);
    pthread_cond_signal(pDlCmdOp->pCond);
    pthread_mutex_unlock(pDlCmdOp->pMutex);

    return nullptr;
}

void *GetNfccChipInfoFromFWThread(void *arg)
{
    if (!RegisterExitSignal()) {
        return nullptr;
    }

    DownloadCmdOp *pDlCmdOp = (DownloadCmdOp *)arg;
    SetThreadRunning(true);
    ChipInfo *pRes = (ChipInfo *)(pDlCmdOp->pResStatus);
    *pRes = ComputeNfccChipInfo(GetNfccChipInfoFromFW());
    SetThreadRunning(false);

    pthread_mutex_lock(pDlCmdOp->pMutex);
    pthread_cond_signal(pDlCmdOp->pCond);
    pthread_mutex_unlock(pDlCmdOp->pMutex);

    return nullptr;
}

void *GetNfccChipInfoFromBLThread(void *arg)
{
    if (!RegisterExitSignal()) {
        return nullptr;
    }

    DownloadCmdOp *pDlCmdOp = (DownloadCmdOp *)arg;
    SetThreadRunning(true);
    VtpParams *pVtpParams = (VtpParams *)pDlCmdOp->pParameters;
    ChipInfo *pRes = (ChipInfo *)(pDlCmdOp->pResStatus);
    if (pVtpParams->isNfccBlState) {
        *pRes = GetNfccChipInfoFromBL(arg);
    } else {
        *pRes = ComputeNfccChipInfo(GetNfccChipInfoFromFW());
    }
    SetThreadRunning(false);

    pthread_mutex_lock(pDlCmdOp->pMutex);
    pthread_cond_signal(pDlCmdOp->pCond);
    pthread_mutex_unlock(pDlCmdOp->pMutex);

    return nullptr;
}

ESESTATUS execNciCmdChkRes(const char *cmd, int cmdStrLen, void *arg)
{
    static uint8_t rspCkhHeader[] = {0x4F, 0x25, 0x01, 0x00};
    ESESTATUS status = ESESTATUS_INVALID_PARAMETER;
    bool ret = false;
    UNUSED(arg);

    uint16_t cmdLen = (uint16_t) (cmdStrLen / 2);
    uint8_t *nciCmd = new uint8_t[cmdLen];
    if (!Cstr2hex(cmd, cmdStrLen, nciCmd, cmdLen)) {
        TMS_LOG_E(g_tag, "%s invalid Hex format cmd: %s", __FUNCTION__, cmd);
        goto cleanup;
    }

    ret = NciCmdSendAndChkRsp(nciCmd, cmdLen, rspCkhHeader, sizeof(rspCkhHeader));
    if (ret) {
        status = ESESTATUS_SUCCESS;
    }

cleanup:
    delete[] nciCmd;

    return status;
}

#if defined (USE_TMS_NFC) || defined (USE_C1)
/*******************************************************************************
**
** Function         nfccHalRestartThread
**
** Description      Initializes comport, reader and writer threads
**
** Parameters       None
**
** Returns          true - threads initialized successfully
**                  false - initialization failed due to system error
**
*******************************************************************************/
static bool nfccHalRestartThread(void *context, sem_t *pRxSemaphore,
                                 uint8_t *pReadEnable, uint8_t *pReadThreadBusy,
                                 uint8_t *pWriteEnable, uint8_t *pWriteThreadBusy)
{
    TMS_LOG_D(g_tag, "%s: enter", __FUNCTION__);
    TMS_LOG_D(TAG, "%s: g_nfccRwThreadRecovery = %d", __FUNCTION__, g_nfccRwThreadRecovery);

    /* Create Reader and Writer threads */
    if (g_nfccRwThreadRecovery && (context != NULL)) {
        int semVal = -1;

        *pReadEnable = 1;
        *pReadThreadBusy = 0;
        if ((sem_getvalue(pRxSemaphore, &semVal) == 0)
                && (0 == semVal)) {
            sem_post(pRxSemaphore);
        }

        *pWriteEnable = 0;
        *pWriteThreadBusy = 0;
        g_nfccRwThreadRecovery = false;
    }

    TMS_LOG_D(g_tag, "%s: exit", __FUNCTION__);
    return true;
}

static bool nfccHalTerminateThread(void *context,
                                   uint8_t *pReadEnable, uint8_t *pReadThreadBusy,
                                   uint8_t *pWriteEnable, uint8_t *pWriteThreadBusy)
{
    TMS_LOG_D(g_tag, "%s: enter", __FUNCTION__);

    bool ret = false;
    g_nfccRwThreadRecovery = false;
    if (context != NULL) {
        // NFC is turned on, should terminal nfcc hal read and write thread
        *pWriteEnable = 0;
        *pWriteThreadBusy = 1;

        *pReadEnable = 0;
        *pReadThreadBusy = 1;
        ret = IoctlNfc(NFC_DLD_FLUSH);
        if (!ret) {
            TMS_LOG_E(g_tag, "flush nfcHal I2C data failed, nfcHal read thread pause failed");
            goto exit;
        }
        usleep(10 * 1000);  // sleep 10 * 1000 us

        g_nfccRwThreadRecovery = true;

        ret = g_nfccRwThreadRecovery;
    } else {
        TMS_LOG_D(g_tag, "%s: NFCC has been turned off, do nothing", __FUNCTION__);
        ret = true;
    }

exit:
    TMS_LOG_D(g_tag, "%s: exit", __FUNCTION__);
    return ret;
}
#endif

bool NfccSoftReset()
{
    bool ret = false;
    unsigned char nfccResetDldCmd[] = {0x20, 0x00, 0x01, 0x80};
    unsigned char rspCkhHeader[] = {0x40, 0x00, 0x01, 0x00};
    char routineName[] = "NciCmdRspChkThread";
    DownloadCmdOp dlCmdOp;
    VtpParams vtpParams;

    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, nullptr);

    vtpParams = {.fileName = NULL,
                 .needPT2SeBl = false,
                 .isNfccBlState = false,
                 .pTmsPhAbs = new TmsRee(NULL),
                };
    if (NULL == vtpParams.pTmsPhAbs) {
        ret = false;
        TMS_LOG_E(g_tag, "%s: new TmsRee failed, get NFCC FW/BL version failed", __FUNCTION__);
        goto exit;
    }
    dlCmdOp = {.routineName = routineName,
               .cmd = nfccResetDldCmd,
               .rsp = NULL,
               .rspChk = rspCkhHeader,
               .cmdLen = sizeof(nfccResetDldCmd),
               .rspLen = 0,
               .rspChkLen = sizeof(rspCkhHeader),
               .pResStatus = NULL,
               .routine = NciCmdRspChkThread,
               .pCond = GetGpCond(),
               .pMutex = &mutex,
               .pParameters = &vtpParams,
              };

    dlCmdOp.pResStatus = new bool;
    if (dlCmdOp.pResStatus == NULL) {
        TMS_LOG_E(g_tag, "new res memory failed, nfcc soft reset failed");
        goto exit;
    }
    *((bool *)(dlCmdOp.pResStatus)) = false;

    ret = startThread(&dlCmdOp, 2);  // 2s timeout
    ret = ret && *((bool *)(dlCmdOp.pResStatus));
    delete (bool *)dlCmdOp.pResStatus;
    dlCmdOp.pResStatus = NULL;
    delete vtpParams.pTmsPhAbs;
    vtpParams.pTmsPhAbs = NULL;

    if (ret) {
        // FW soft reset need 200ms, wait 300ms for nfcc reset to DL mode
        usleep(200 * 1000);  // 200 * 1000 us
    }

exit:
    pthread_mutex_destroy(&mutex);

    if (vtpParams.pTmsPhAbs != NULL) {
      delete vtpParams.pTmsPhAbs;
      vtpParams.pTmsPhAbs = NULL;
    }

    return ret;
}

#if defined (USE_TMS_NFC) || defined (USE_C1)
bool EseSoftReset(void *context, sem_t *pRxSemaphore,
                  uint8_t *pReadEnable, uint8_t *pReadThreadBusy,
                  uint8_t *pWriteEnable, uint8_t *pWriteThreadBusy)
{
#else
bool EseSoftReset()
{
#endif

#ifdef USE_CHIP_HARD_RESET
    return ChipHardReset(VEN_SET_WAIT_TIME_20MS, VEN_SET_WAIT_TIME_7MS);
#else
    TMS_LOG_D(g_tag, "%s enter", __FUNCTION__);

    ESESTATUS status = ESESTATUS_FAILED;
    bool ret = false;
    unsigned char eseResetDldCmd[] = {0x22, 0x03, 0x02, 0xC0, 0x80};
    unsigned char rspCkhHeader[] = {0x42, 0x03, 0x01, 0x00};
    char routineName[] = "NciCmdRspChkThread";
    DownloadCmdOp dlCmdOp;
    VtpParams vtpParams;
    pthread_mutex_t mutex;

    // 1. lock and check download is busy or not
    pthread_mutex_lock(GetMutex());

    pthread_mutex_init(&mutex, nullptr);
    if (GetGpCond() == nullptr) {
        SetGpCond(new pthread_cond_t);
        pthread_cond_init(GetGpCond(), nullptr);
    } else {
        TMS_LOG_E(g_tag, "%s: download is busy, eSE soft reset failed", __FUNCTION__);
        goto exit;
    }

    // 2. open nfc node
    status = openT1(ESE_MODE_NFCC_DL);
    if (ESESTATUS_SUCCESS != status) {
        TMS_LOG_E(g_tag, "open nfc node failed, eSE soft reset failed");
        goto exit;
    }

    // 3. terminate nfcc hal read/write thread
#if defined (USE_TMS_NFC) || defined (USE_C1)
    if (!nfccHalTerminateThread(context,
                                pReadEnable, pReadThreadBusy,
                                pWriteEnable, pWriteThreadBusy)) {
        TMS_LOG_E(g_tag,
                  "nfccHalTerminateThread failed, eSE soft reset failed");
        goto T1close;
    }
#endif

    // 4. send nci cmd to reset ese
    vtpParams = {.fileName = NULL,
                 .needPT2SeBl = false,
                 .isNfccBlState = false,
                 .pTmsPhAbs = new TmsRee(NULL),
                };
    if (NULL == vtpParams.pTmsPhAbs) {
        status = ESESTATUS_FAILED;
        TMS_LOG_E(g_tag, "%s: new TmsRee failed, ese soft reset failed", __FUNCTION__);
        goto T1close;
    }

    dlCmdOp = {.routineName = routineName,
               .cmd = eseResetDldCmd,
               .rsp = NULL,
               .rspChk = rspCkhHeader,
               .cmdLen = sizeof(eseResetDldCmd),
               .rspLen = 0,
               .rspChkLen = sizeof(rspCkhHeader),
               .routine = EseSoftResetThread,
               .pCond = GetGpCond(),
               .pMutex = &mutex,
               .pParameters = &vtpParams,
              };
    dlCmdOp.pResStatus = new bool;
    if (dlCmdOp.pResStatus == NULL) {
        TMS_LOG_E(g_tag, "new res memory failed, eSE soft reset failed");
        goto T1close;
    }
    *((bool *)(dlCmdOp.pResStatus)) = false;
    ret = startThread(&dlCmdOp, 2);
    ret = ret && *((bool *)(dlCmdOp.pResStatus));
    delete (bool *)dlCmdOp.pResStatus;
    dlCmdOp.pResStatus = NULL;

    if (ret) {
        // ese soft reset need 200ms
        usleep(200 * 1000);
    }

T1close:
    // 5. close nfc node
    CloseT1();

    // 6. nfccHalRestartThread
#if defined (USE_TMS_NFC) || defined (USE_C1)
    nfccHalRestartThread(context, pRxSemaphore,
                         pReadEnable, pReadThreadBusy,
                         pWriteEnable, pWriteThreadBusy);
#endif

exit:
    // 7. release resource and unlock
    if (GetGpCond() != nullptr) {
        pthread_cond_destroy(GetGpCond());
        delete GetGpCond();
        SetGpCond(nullptr);
    }
    pthread_mutex_destroy(&mutex);

    pthread_mutex_unlock(GetMutex());

    return ret;
#endif
}

