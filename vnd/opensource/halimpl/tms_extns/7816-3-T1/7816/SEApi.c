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

#include <string.h>
#include <pthread.h>
#include <cutils/properties.h>

#include "tmslog.h"
#include "tmsVersion.h"

#include "common.h"
#include "SEApi.h"
#include "phDriver.h"
#include "T1.h"

#if defined (USE_TMS_NFC) || defined (USE_C1)
    #include "tmsSecureString.h"
#else
    #include "securec.h"
#endif

#ifdef TAG
#undef TAG
#endif
#define TAG g_tag

static const char g_tag[] = "SEApi";

/*********************** Global Variables *************************************/
bool gIsInitlized = false;
uint8_t gSEOpenedCnt = 0;
SeInitMode gInitMode;
SEContext gEseCtx;
const uint16_t gIFSDevice = T1_DEFAULT_IFS;
uint16_t gIFSCard = T1_DEFAULT_IFS;
/**
 * First read, use the default length(0xFE).
 * Ohters read, use the IFSD REQ length, if this REQ is success.
 * This memory cannot be freed, unitl this process died.
 */
uint8_t *gpDataRx = NULL;
// Default length: T1_HEADER_LEN + IFSD + LRC
uint16_t gDataRxLen = gIFSDevice + T1_HEADER_LEN + 1;

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;;

SEContext *GetEseCtx(void)
{
    return &gEseCtx;
}

uint8_t *GetDataRx(void)
{
    return gpDataRx;
}

uint16_t GetDataRxLen(void)
{
    return gDataRxLen;
}

SeInitMode GetInitMode(void)
{
    return gInitMode;
}

static void InitSECtxFromConfig();

/**
 * @Function    seOpen
 *
 * @Description This function open the physical, SPI or I2C, device driver.
 *
 * @params      initMode - init mode for normal OMA or download mode
 *
 * @returns     This function return ESESTATUS_SUCCES (0) in case of success.
 *
 * @Note:       This should only be called by ESE_MODE_NFCC_DL, ESE_MODE_ESE_DL
 *              or ESE_MODE_ESE_PTH_DL mode.
 *              ESE_MODE_NORMAL mode should call seInit function.
 */
ESESTATUS seOpen(SeInitMode initMode)
{
    int oFlag;
    char mwVersion[] = MW_VERSION;
#if defined (USE_TMS_NFC) || defined (USE_C1)
    char mw_build_time[] = MW_BUILD_TIME;
#endif
    ESESTATUS status = ESESTATUS_SUCCESS;

    pthread_mutex_lock(&g_mutex);

    InitSELogLevel();
    TMS_LOG_I(g_tag, "%s: SE Hal version: %s", __FUNCTION__, mwVersion);
#if defined (USE_TMS_NFC) || defined (USE_C1)
    TMS_LOG_I(g_tag, "%s: SE Hal build time: %s", __FUNCTION__, mw_build_time);
#endif

    if (ESESTATUS_OPENED == gEseCtx.EseLibStatus) {
        if (((ESE_MODE_NFCC_DL == initMode) ||
            (ESE_MODE_ESE_DL == initMode) ||
            (ESE_MODE_ESE_PTH_DL == initMode)) &&
            (ESE_MODE_NORMAL == gInitMode)) {
            TMS_LOG_W(g_tag, "%s: SE Hal already opened by normal, refused DL mode",
                      __FUNCTION__);
            status = ESESTATUS_BUSY;
        } else if (((ESE_MODE_NFCC_DL == gInitMode)
                    || (ESE_MODE_ESE_DL == gInitMode)
                    || (ESE_MODE_ESE_PTH_DL == gInitMode))
                   && (ESE_MODE_NORMAL == initMode)) {
            TMS_LOG_W(g_tag, "%s: SE Hal already opened by DL, refused normal mode",
                      __FUNCTION__);
            status = ESESTATUS_BUSY;
        } else {
            TMS_LOG_W(g_tag, "%s: SE Hal already opened, continue ...", __FUNCTION__);
            status = ESESTATUS_SUCCESS;
        }
        goto cleanup;
    }
    gInitMode = initMode;

    (void)memset_s(&gEseCtx, sizeof(gEseCtx), 0x00, sizeof(gEseCtx));
    gEseCtx.devHandle = -1;
    InitSECtxFromConfig();

    {
        // behind goto, only code block can definition variable
        char devName[50] = {0x00};
        if (ESE_MODE_NFCC_DL == gInitMode) {
            ConfigGetString(devName, 50,  // size 50
                            NAME_TMS_NFC_DEV_NODE, strlen(NAME_TMS_NFC_DEV_NODE),
                            "/dev/tms_nfc", strlen("/dev/tms_nfc"));
            oFlag = O_RDWR | O_NOCTTY;
        } else {
            ConfigGetString(devName, 50,  // size 50
                            NAME_TMS_ESE_DEV_NODE, strlen(NAME_TMS_NFC_DEV_NODE),
                            "/dev/tms_ese", strlen("/dev/tms_ese"));
            oFlag = O_RDWR | O_NOCTTY | O_NONBLOCK;
        }
        gEseCtx.devHandle = PhOpen(devName, strlen(devName), oFlag);
    }

    if (gEseCtx.devHandle != -1) {
        status = ESESTATUS_SUCCESS;
        gEseCtx.EseLibStatus = ESESTATUS_OPENED;
    } else {
        status = ESESTATUS_FAILED;
    }

cleanup:
    if (ESESTATUS_SUCCESS == status) {
        gSEOpenedCnt++;
    }
    pthread_mutex_unlock(&g_mutex);

    return status;
}

/**
 * @Function     SeIsOpened
 *
 * @Description  This function checks if the hal is opened.
 *
 * @returns      false if it is close, otherwise true
 *
 */
bool SeIsOpened()
{
    pthread_mutex_lock(&g_mutex);
    bool isOpened = false;
    TMS_LOG_D(g_tag, " %s  status 0x%x", __FUNCTION__, gEseCtx.EseLibStatus);
    isOpened = (gEseCtx.EseLibStatus == ESESTATUS_OPENED);
    pthread_mutex_unlock(&g_mutex);

    return isOpened;
}

/**
 * @Function     SeIsInitialized
 *
 * @Description  This function checks if the SE is initialized.
 *
 * @returns      true if PCB=0xC4 transmit succesfully, otherwise false
 *
 */
bool SeIsInitialized()
{
    pthread_mutex_lock(&g_mutex);
    bool isInitlized = false;
    isInitlized = gIsInitlized;
    pthread_mutex_unlock(&g_mutex);

    return isInitlized;
}

/**
 * @Function seInit
 *
 * @Description This function initializes 7816-3-T1 protocol's variables
 *
 * @params      initMode - init mode for normal OMA or download mode
 *
 * @returns     This function return ESESTATUS_SUCCES (0) in case of success.
 *
 */
ESESTATUS seInit(SeInitMode initMode)
{
    ESESTATUS status = ESESTATUS_SUCCESS;

    status = seOpen(initMode);

    pthread_mutex_lock(&g_mutex);
    if (ESESTATUS_SUCCESS != status) {
        TMS_LOG_E(g_tag, "%s: open device failed, errno = %d", __FUNCTION__, errno);
        goto cleanup;
    }
    if (gIsInitlized) {
        TMS_LOG_I(g_tag, "%s: T1 has been initialized", __FUNCTION__);
        goto cleanup;
    }

    if (NULL == gpDataRx) {
        gpDataRx = (uint8_t *)calloc(1, gDataRxLen);
        if (NULL == gpDataRx) {
            TMS_LOG_E(g_tag, "%s: Rx data malloc memory failed, gIFSDevice = %u", __FUNCTION__, gIFSDevice);
            status = ESESTATUS_MEM_EXCEPTION;
            goto cleanup;
        }
    }

    status = t1CIPReq();
    if (ESESTATUS_SUCCESS != status) {
        TMS_LOG_E(g_tag, "%s: CIP REQ failed[%d]", __FUNCTION__, status);
        goto cleanup;
    }
    gIsInitlized = true;

    // If do not support extend T=1 length, use the default 1 byte length 0xFE
    // t1IFSDeviceReq should not be called.
    if (!gEseCtx.isT1ExtHdrLen) {
        goto cleanup;
    }

    status = t1IFSDeviceReq();
    if (ESESTATUS_SUCCESS != status) {
        TMS_LOG_E(g_tag, "%s: IFSD REQ failed[%d]", __FUNCTION__, status);
        status = ESESTATUS_SUCCESS;
        goto cleanup;
    } else {
        free(gpDataRx);
        // HEADER + IFSD + LRC or CRC
        uint8_t hdrLen = gEseCtx.isT1ExtHdrLen ? (T1_EXT_HEADER_LEN + 2) : (T1_HEADER_LEN + 1);
        gDataRxLen = gEseCtx.maxIFSD + hdrLen;
        gpDataRx = (uint8_t *)calloc(1, gDataRxLen);
        if (NULL == gpDataRx) {
            TMS_LOG_E(g_tag, "%s: Rx data malloc memory failed, gIFSDevice = %u", __FUNCTION__, gIFSDevice);
            status = ESESTATUS_MEM_EXCEPTION;
        }
    }

cleanup:
    if (gIsInitlized) {
        pthread_mutex_unlock(&g_mutex);
    } else {
        pthread_mutex_unlock(&g_mutex);
        seClose();
    }

    return status;
}

/**
 * @Function seTransceive
 *
 * @Description  This function prepares the 7816-4 APDU CMD to 7816-3-T1 TPDU,
 *               send to ESE and then receives the response from ESE,
 *               decode it to 7816-4 APDU RSP, and returns data.
 *
 * @params       pCmd - Command to eSE
 *               pRsp - Response from eSE (Returned data to be freed after copying)
 *
 * @returns      On Success ESESTATUS_SUCCESS else an error code.
 *
 */
ESESTATUS seTransceive(SeData *pCmd, SeData *pRsp)
{
    ESESTATUS status = ESESTATUS_SUCCESS;

    pthread_mutex_lock(&g_mutex);

    if ((NULL == pCmd) || (NULL == pRsp)) {
        status = ESESTATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    if ((0 == pCmd->len) || (NULL == pCmd->pData)) {
        TMS_LOG_E(g_tag, "%s: Invalid Parameter no data", __FUNCTION__);
        status = ESESTATUS_INVALID_PARAMETER;
        goto cleanup;
    } else if (ESESTATUS_CLOSED == gEseCtx.EseLibStatus) {
        TMS_LOG_E(g_tag, " %s ESE Not opened", __FUNCTION__);
        status = ESESTATUS_CLOSED;
        gIsInitlized = false;
        goto cleanup;
    } else if (!gIsInitlized) {
        TMS_LOG_E(g_tag, " %s ESE Not Initialized", __FUNCTION__);
        status = ESESTATUS_NOT_INITIALISED;
        goto cleanup;
    }

    TMS_LOG_D(g_tag, " %s processing, dataLen = %u", __FUNCTION__, pCmd->len);
    status = t1TransceiveApdu(pCmd->pData, pCmd->len);
    if (ESESTATUS_SUCCESS == status) {
        status = t1RecvDataGet(&pRsp->pData, &pRsp->len);
    } else {
        TMS_LOG_D(g_tag, " %s failed, status = %d", __FUNCTION__, status);
        pRsp->pData = NULL;
    }
    TMS_LOG_D(g_tag, " %s Processing complete", __FUNCTION__);

cleanup:
    pthread_mutex_unlock(&g_mutex);

    return status;
}

/**
 * @Function seDeInit
 *
 * @Description  This function deinitializes the ESE interface and free all resources.
 *
 * @returns      ESESTATUS_SUCCESS Always return ESESTATUS_SUCCESS (0).
 *
 */
ESESTATUS seDeInit()
{
    ESESTATUS status = ESESTATUS_SUCCESS;

    pthread_mutex_lock(&g_mutex);

    if ((ESESTATUS_CLOSED == gEseCtx.EseLibStatus)) {
        TMS_LOG_E(g_tag, " %s ESE Not Initialized \n", __FUNCTION__);
        status = ESESTATUS_NOT_INITIALISED;
        goto cleanup;
    }

    if ((ESE_MODE_NFCC_DL == gInitMode)
            || (ESE_MODE_ESE_DL == gInitMode)) {
        ALOGI("%s DL mode, do not send PCB=0xC5, cleanup", __FUNCTION__);
        goto cleanup;
    }

    status = t1PropEndApduReq();
    if (ESESTATUS_SUCCESS != status) {
        TMS_LOG_W(g_tag, "%s: END_APUD REQ failed[%d]", __FUNCTION__, status);
        TMS_LOG_W(g_tag, "%s: IntfReset (CIP)", __FUNCTION__);
        status = t1CIPReq();
        if (status != ESESTATUS_SUCCESS) {
            TMS_LOG_E(g_tag, "%s: IntfReset Failed", __FUNCTION__);
        }
    } else {
        TMS_LOG_D(g_tag, "%s: END_APUD REQ success", __FUNCTION__);
    }

cleanup:
    if (gSEOpenedCnt <= 1) {
        gIsInitlized = false;
    }
    pthread_mutex_unlock(&g_mutex);

    status = seClose();

    return status;
}

/**
 * @Function    seClose
 *
 * @Description This function close the physical, SPI or I2C, device driver,
 *              and release resources.
 *
 * @returns     This function return ESESTATUS_SUCCES (0) in case of success.
 *
 * @Note:       This should only be called by ESE_MODE_NFCC_DL or ESE_MODE_ESE_DL
 *              mode. ESE_MODE_NORMAL mode should call seDeInit function.
 */
ESESTATUS seClose()
{
    ESESTATUS status = ESESTATUS_SUCCESS;

    pthread_mutex_lock(&g_mutex);

    if ((ESESTATUS_CLOSED == gEseCtx.EseLibStatus)) {
        TMS_LOG_E(g_tag, " %s ESE Not Opened", __FUNCTION__);
        status = ESESTATUS_CLOSED;
        goto cleanup;
    }

    if (1 == gSEOpenedCnt) {
        PhClose();
        gIsInitlized = false;
    } else {
        // Only gSEOpenedCnt--
        status = ESESTATUS_SUCCESS;
        goto cleanup;
    }

    if (gEseCtx.pT1Params != NULL) {
        free(gEseCtx.pT1Params);
        gEseCtx.pT1Params = NULL;
    }
    (void)memset_s(&gEseCtx, sizeof(SEContext), 0x00, sizeof(SEContext));
    gEseCtx.devHandle = -1;
    TMS_LOG_D(g_tag, "%s: ESE Context deinit completed", __FUNCTION__);

    gEseCtx.EseLibStatus = ESESTATUS_CLOSED;

    if (NULL != gpDataRx) {
        free(gpDataRx);
        gpDataRx = NULL;
    }

cleanup:
    if (ESESTATUS_SUCCESS == status) {
        if (gSEOpenedCnt > 0) {
            gSEOpenedCnt--;
        }
    }

    pthread_mutex_unlock(&g_mutex);
    return status;
}

/**
 * @Function seGetATR
 *
 * @Description  This function get the last ATR received.
 *
 * @params       pRsp - Response from eSE (Returned data to be freed after copying)
 *
 * @returns      ESESTATUS_SUCCESS Always return ESESTATUS_SUCCESS (0).
 *
 */
ESESTATUS seGetATR(SeData *pRsp)
{
    ESESTATUS status = ESESTATUS_SUCCESS;

    pthread_mutex_lock(&g_mutex);

    status = t1ATRReq();
    if (ESESTATUS_SUCCESS != status) {
        TMS_LOG_E(g_tag, "%s: ATR REQ failed[%d]", __FUNCTION__, status);
    } else {
        TMS_LOG_I(g_tag, "%s: ATR REQ success", __FUNCTION__);
    }
    if (0 != GetAtr()->len) {
        pRsp->len = sizeof(*GetAtr());
        pRsp->pData = (uint8_t *)calloc(1, sizeof(*GetAtr()));
        if (NULL != pRsp->pData) {
            int err = memcpy_s(pRsp->pData, pRsp->len, &GetAtr()->len, sizeof(*GetAtr()));
            if (err != EOK) {
                TMS_LOG_E(g_tag, "%s: memcpy_s err. ret:%d", __FUNCTION__, err);
            }
        }
    } else {
        pRsp->len = 0;
        pRsp->pData = NULL;
    }

    pthread_mutex_unlock(&g_mutex);

    return status;
}

/**
 * @Function seReset
 *
 * @Description  This function reset the SE, such as: N(S), chain flag, etc.
 *
 * @returns      ESESTATUS_SUCCESS is successful
 *
 */
ESESTATUS seReset()
{
    TMS_LOG_D(g_tag, "%s : Enter", __FUNCTION__);
    ESESTATUS status = ESESTATUS_SUCCESS;

    pthread_mutex_lock(&g_mutex);

    if ((ESESTATUS_CLOSED == gEseCtx.EseLibStatus)) {
        TMS_LOG_E(g_tag, " %s ESE Not Initialized \n", __FUNCTION__);
        status = ESESTATUS_NOT_INITIALISED;
        goto cleanup;
    }

    gEseCtx.isT1ExtHdrLen = false;

    TMS_LOG_W(g_tag, "%s: IntfReset (CIP)", __FUNCTION__);
    status = t1CIPReq();
    if (status != ESESTATUS_SUCCESS) {
        TMS_LOG_E(g_tag, "%s: IntfReset (CIP) Failed", __FUNCTION__);
    }

cleanup:
    pthread_mutex_unlock(&g_mutex);

    return status;
}

/******************************************************************************
 * @Function     DoReadTerminate
 *
 * @Description  set isReadDone to true, T=1 read SOF will be terminated tryagain
 *
******************************************************************************/
void DoReadTerminate()
{
    gEseCtx.isReadDone = true;
}


static void InitSECtxFromConfig()
{
    gEseCtx.maxWriteRetryCnt = ConfigGetUnsigned(NAME_TMS_PH_WRITE_RETRY_CNT,
                                                 // default 10
                                                 strlen(NAME_TMS_PH_WRITE_RETRY_CNT), 10);
    gEseCtx.writeRetryTime = ConfigGetUnsigned(NAME_TMS_PH_WRITE_TIME_GAP,
                                               // default 1000
                                               strlen(NAME_TMS_PH_WRITE_TIME_GAP), 1000);
    // WTX is 1s, read error will delay 1ms and retry, so the max retry is 1010
    gEseCtx.maxReadRetryCnt = ConfigGetUnsigned(NAME_TMS_PH_READ_RETRY_CNT,
                                                // default 1010
                                                strlen(NAME_TMS_PH_READ_RETRY_CNT), 1010);
    gEseCtx.readRetryTime = ConfigGetUnsigned(NAME_TMS_PH_READ_TIME_GAP,
                                              // default 1000
                                              strlen(NAME_TMS_PH_READ_TIME_GAP), 1000);
    gEseCtx.readTimeout = ConfigGetUnsigned(NAME_TMS_T1_READ_TIMEOUT,
                                            // default readTimeout is 10s
                                            strlen(NAME_TMS_T1_READ_TIMEOUT), 10);
    gEseCtx.maxIFSD = ConfigGetUnsigned(NAME_TMS_IFSD,
                                        // default 258
                                        strlen(NAME_TMS_IFSD), 258);

    // Rule 6.4 — After the interface device has failed a maximum of three times
    //            in succession to reach the intended resynchronization by transmitting
    //            S(RESYNCH request), it performs either a warm reset or a deactivation.
    gEseCtx.maxRecoveryCnt = 3;  // retry 3

    gEseCtx.maxBlkRetryCnt = ConfigGetUnsigned(NAME_TMS_MAX_BLK_RETRY_CNT,
                                               // default 3
                                               strlen(NAME_TMS_MAX_BLK_RETRY_CNT), 3);
    gEseCtx.maxWTXCnt = ConfigGetUnsigned(NAME_TMS_MAX_WTX_CNT,
                                          // default 30
                                          strlen(NAME_TMS_MAX_WTX_CNT), 30);
}
