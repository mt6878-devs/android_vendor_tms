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

#include <sys/ioctl.h>

#include "tmsDlCommonUtils.h"
#include "tmsReeDlCommonUtils.h"
#include "tmslog.h"

static const char g_tag[] = "TmsCosDl:ReeDlCommonUtils";

int g_nfccFwDlTime = 0;
int g_nfccBlDlTime = 0;
int g_seCosI2cDlTime = 0;

int GetNfccFwDlTime(void)
{
    return g_nfccFwDlTime;
}

void SetNfccFwDlTime(int time)
{
    g_nfccFwDlTime = time;
}


int GetNfccBlDlTime(void)
{
    return g_nfccBlDlTime;
}

void SetNfccBlDlTime(int time)
{
    g_nfccBlDlTime = time;
}


int GetSeCosI2cDlTime(void)
{
    return g_seCosI2cDlTime;
}

void SetSeCosI2cDlTime(int time)
{
    g_seCosI2cDlTime = time;
}


// If return val < 0, ioctl failed.
static int Ioctl(int handle, long cmd, long arg)
{
    int ret = -1;
    if (handle < 0) {
        TMS_LOG_E(g_tag, "%s: handle[%d] has been closed, cmd = %ld, arg = %ld failed",
                  __FUNCTION__, handle, cmd, arg);
        return handle;
    }
    TMS_LOG_D(g_tag, "%s: VEN arg %ld", __FUNCTION__, arg);
    ret = ioctl(handle, cmd, arg);
    if (ret < 0) {
        TMS_LOG_E(g_tag, "%s: failed errno = 0x%x", __FUNCTION__, errno);
    } else {
        TMS_LOG_D(g_tag, "%s: success", __FUNCTION__);
    }
    return ret;
}

bool IoctlNfc(long arg)
{
    // ioctl on /dev/tms_ese or /dev/tms_nfc for NFCC VEN reset
    long cmd = -1;
    switch (GetInitMode()) {
        case ESE_MODE_ESE_DL:
        case ESE_MODE_ESE_PTH_DL: {
            // ioctl spi driver
            cmd = THN31_ESE_SET_PWR;
            break;
        }
        case ESE_MODE_NFCC_DL: {
            // ioctl i2c driver
            cmd = THN31_NFCC_SET_PWR;
            break;
        }
        default:
            TMS_LOG_E(g_tag, "%s: invalid initMode = %d", __FUNCTION__, GetInitMode());
            return false;
    }

    int ret = Ioctl(GetEseCtx()->devHandle, cmd, arg);
    return ret >= 0 ? true : false;
}


// open_t1 should be called, before all command send, include ioctl, nci and t=1
ESESTATUS openT1(SeInitMode initMode)
{
    ESESTATUS status = ESESTATUS_SUCCESS;

    // open /dev/tms_ese and config 7816-3 parameters
    status = seOpen(initMode);
    if (ESESTATUS_SUCCESS != status) {
        TMS_LOG_E(g_tag, "%s T1 open failed!!! status = %d", __FUNCTION__, status);
    }

    return status;
}

// If chip was reset(hard or soft reset), this function should be called.
ESESTATUS initT1()
{
    ESESTATUS status = ESESTATUS_SUCCESS;

    status = seInit(GetInitMode());
    if (ESESTATUS_SUCCESS == status) {
        TMS_LOG_D(g_tag, "ESE SPI init complete!!!");
    } else {
        TMS_LOG_D(g_tag, "ESE SPI init failed, seDeInit and seClose");
        seDeInit();
        seClose();
    }

    return status;
}

ESESTATUS deInitT1()
{
    return seDeInit();
}

bool CloseT1()
{
    ESESTATUS status = seClose();
    if (status == ESESTATUS_SUCCESS) {
        TMS_LOG_D(g_tag, "%s: Success", __FUNCTION__);
    } else {
        TMS_LOG_W(g_tag, "%s: Failed. deInitStatus = %x", __FUNCTION__, status);
    }

    return status == ESESTATUS_SUCCESS;
}

static int32_t getSeBlVerFromChip(void *arg)
{
    int32_t seBlVer = -1;
    DownloadCmdOp *pDlCmdOp = (DownloadCmdOp *)arg;
    VtpParams *pVtpParams = (VtpParams *)pDlCmdOp->pParameters;
    // PT, pass-through transmission
    bool needPT2SeBl = (pVtpParams == NULL ? false : pVtpParams->needPT2SeBl);
    ESESTATUS status = ESESTATUS_FAILED;
    bool isNewRspMem = false;

    if (NULL == pDlCmdOp->rsp) {
        pDlCmdOp->rsp = new uint8_t[NCI_RSP_LEN_MAX];
        pDlCmdOp->rspLen = NCI_RSP_LEN_MAX;
        if (NULL == pDlCmdOp->rsp) {
            TMS_LOG_E(g_tag, "%s: new res memory failed", __FUNCTION__);
            goto fail;
        }
        isNewRspMem = true;
    }

    if (needPT2SeBl) {
        status = execApduCmdChkRes("00F0FF0200", strlen("00F0FF0200"), pDlCmdOp);
        if (ESESTATUS_SUCCESS == status) {
            // When pass-through transmission, reset SE to reInit ATR information
            status = seReset();
            if (ESESTATUS_SUCCESS != status) {
                goto fail;
            }
        } else {
            goto fail;
        }
    } else {
        status = execApduCmdChkRes("004D020000", strlen("004D020000"), pDlCmdOp);
        if (ESESTATUS_SUCCESS != status) {
            TMS_LOG_E(g_tag, "%s: stay SE-BL fail[%d]", __FUNCTION__, status);
        }
    }

    for (int i = 0; i < 2 && GetThreadRunning(); i++) {  // 2 retry
        status = execApduCmdChkRes("004A020000", strlen("004A020000"), pDlCmdOp);
        if (ESESTATUS_SUCCESS != status) {
            seBlVer = -1;
        } else {
            int len = pDlCmdOp->rspLen;
            if ((len >= 6) && // 6 more length
                (0x90 == pDlCmdOp->rsp[len - 2]) && // byte len-2
                (0x00 == pDlCmdOp->rsp[len - 1])) {
                seBlVer = (((uint32_t)pDlCmdOp->rsp[0]) << 24)  // rsp[0]) << 24
                          | (((uint32_t)pDlCmdOp->rsp[1]) << 16)  // rsp[1]) << 16
                          | (((uint32_t)pDlCmdOp->rsp[2]) << 8)  // rsp[2]) << 8
                          | pDlCmdOp->rsp[3];  // rsp[3]
            } else {
                seBlVer = -1;
            }
            break;
        }
    }

fail:
    TMS_LOG_D(g_tag, "SE BL version = 0x%x", seBlVer);
    if (isNewRspMem && (NULL != pDlCmdOp->rsp)) {
        delete[] pDlCmdOp->rsp;
        pDlCmdOp->rsp = NULL;
        pDlCmdOp->rspLen = 0;
    }

    return seBlVer;
}

static void TryJumpToCos(void *arg)
{
    DownloadCmdOp *pDlCmdOp = (DownloadCmdOp *)arg;
    // PT, pass-through transmission
    ESESTATUS status = ESESTATUS_FAILED;
    bool isNewRspMem = false;

    if (NULL == pDlCmdOp->rsp) {
        pDlCmdOp->rsp = new uint8_t[NCI_RSP_LEN_MAX];
        pDlCmdOp->rspLen = NCI_RSP_LEN_MAX;
        if (NULL == pDlCmdOp->rsp) {
            TMS_LOG_E(g_tag, "%s: new res memory failed", __FUNCTION__);
            goto cleanup;
        }
        isNewRspMem = true;
    }

    for (int i = 0; i < 2; i++) {  // retry 2 times
        status = execApduCmdChkRes("004B020000", strlen("004B020000"), pDlCmdOp);
        if (ESESTATUS_SUCCESS == status) {
            break;
        }
    }

cleanup:
    if (isNewRspMem && (NULL != pDlCmdOp->rsp)) {
        delete[] pDlCmdOp->rsp;
        pDlCmdOp->rsp = NULL;
        pDlCmdOp->rspLen = 0;
    }
}


void *VtpDownloadThread(void *arg)
{
    if (!RegisterExitSignal()) {
        return nullptr;
    }

    DownloadCmdOp *pDlCmdOp = (DownloadCmdOp *)arg;
    SetThreadRunning(true);
    *((int16_t *)(pDlCmdOp->pResStatus)) = vtpDownload(arg);
    SetThreadRunning(false);

    pthread_mutex_lock(pDlCmdOp->pMutex);
    pthread_cond_signal(pDlCmdOp->pCond);
    pthread_mutex_unlock(pDlCmdOp->pMutex);

    return nullptr;
}

void *GetSeBlVerThread(void *arg)
{
    if (!RegisterExitSignal()) {
        return nullptr;
    }

    DownloadCmdOp *pDlCmdOp = (DownloadCmdOp *)arg;
    SetThreadRunning(true);
    *((int32_t *)(pDlCmdOp->pResStatus)) = getSeBlVerFromChip(arg);
    SetThreadRunning(false);

    pthread_mutex_lock(pDlCmdOp->pMutex);
    pthread_cond_signal(pDlCmdOp->pCond);
    pthread_mutex_unlock(pDlCmdOp->pMutex);

    return nullptr;
}

void *TryJumpToCosThread(void *arg)
{
    if (!RegisterExitSignal()) {
        return nullptr;
    }

    DownloadCmdOp *pDlCmdOp = (DownloadCmdOp *)arg;
    SetThreadRunning(true);
    TryJumpToCos(arg);
    SetThreadRunning(false);

    pthread_mutex_lock(pDlCmdOp->pMutex);
    pthread_cond_signal(pDlCmdOp->pCond);
    pthread_mutex_unlock(pDlCmdOp->pMutex);

    return nullptr;
}

bool ChipHardReset()
{
    return ChipHardReset(VEN_SET_WAIT_TIME_20MS, VEN_SET_WAIT_TIME_20MS);
}

bool ChipHardReset(uint32_t dlDelay, uint32_t upDelay)
{
    unsigned enableVenToggle = EseConfig::getUnsigned(NAME_ENABLE_VEN_TOGGLE, 1);
    if (0 == enableVenToggle) {
        TMS_LOG_I(g_tag, "ENABLE_VEN_TOGGLE is 0, skip hard reset");
        return true;
    }
    IoctlNfc(NFC_DLD_PWR_VEN_OFF);
    usleep(dlDelay);
    bool ret = IoctlNfc(NFC_DLD_PWR_VEN_ON);
    usleep(upDelay);

    return ret;
}
