/*
 * Copyright (c) Tsingteng MicroSystem Co., Ltd. 2021. All rights reserved.
 */
/*
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
 */

#ifdef TMS_TEE

#include <dlfcn.h>
#include <string.h>

#include "TmsTee.h"
#include "tmsDlCommonUtils.h"
#include "tmslog.h"

#if defined (USE_TMS_NFC) || defined (USE_C1)
    #include "tmsSecureString.h"
#else
    #include "securec.h"
#endif

namespace vendor
{
namespace tms
{
static const char g_tag[] = "TmsCosDl:TeeSE";
static void *gpDlHandle = NULL;

static int teeOpenBasicChannel(uint8_t *pAid, uint16_t aidLen);
static int teeTransmit(uint8_t *pCmd, uint16_t cmdLen,
                       uint8_t **ppRsp, uint16_t *pRspLen);
static bool teeCloseChannel(int channelNum);

TmsTee::TmsTee(ExecCmdFun fun)
{
    mExecCmdFun = fun;
}

int TmsTee::openBasicChannel()
{
    int channelNum = -1;
    uint16_t aidLen = (uint16_t)strlen(PROP_AID) / 2;
    uint8_t *pAid = NULL;
    if (aidLen <= 0) {
        return -1;
    }
    if (NULL == gpDlHandle) {
        gpDlHandle = TmsDlopen(CA_SO_FILE_NAME, strlen(CA_SO_FILE_NAME));
        if (NULL == gpDlHandle) {
            return -1;
        }
    }

    pAid = new uint8_t[aidLen];
    if (Cstr2hex(PROP_AID, strlen(PROP_AID), pAid, aidLen)) {
        channelNum = teeOpenBasicChannel(pAid, aidLen);
    } else {
        TMS_LOG_E(g_tag, "%s: invalid aid[%s]", __FUNCTION__, PROP_AID);
    }
    delete[] pAid;
    return channelNum;
}

ESESTATUS TmsTee::transmit(SeData *pCmd, SeData *pRsp)
{
    ESESTATUS status = ESESTATUS_FAILED;
    int res = teeTransmit(pCmd->pData, pCmd->len, &pRsp->pData, &pRsp->len);
    if (0 == res) {
        status = ESESTATUS_SUCCESS;
    }

    return status;
}

bool TmsTee::closeChannel(int channelNum)
{
    bool res = false;
    if (-1 != channelNum) {
        //close channel
        res = teeCloseChannel(channelNum) == 0;
    } else {
        TMS_LOG_E(g_tag, "%s: invalid channelNum -1", __FUNCTION__);
    }
    TmsDlclose(&gpDlHandle);

    return res;
}

static int teeOpenBasicChannel(uint8_t *pAid, uint16_t aidLen)
{
    TeeOpenBasicChannel teeOpenBasicChannel = NULL;
    char *error = NULL;

    if (NULL == gpDlHandle) {
        TMS_LOG_E(g_tag, "%s: gpDlHandle is NULL", __FUNCTION__);
        return -1;
    }

    teeOpenBasicChannel = (TeeOpenBasicChannel) dlsym(gpDlHandle, FUN_OPEN_BASIC_CHANNEL);
    error = dlerror();
    if (error != NULL) {
        TMS_LOG_E(g_tag, "%s: dlsym[openBasicChannel], error[%s]", __FUNCTION__, error);
        return -1;
    }

    return teeOpenBasicChannel(pAid, aidLen);
}

static int teeTransmit(uint8_t *pCmd, uint16_t cmdLen,
                       uint8_t **ppRsp, uint16_t *pRspLen)
{
    TeeTransmit teeTransmit = NULL;
    char *error = NULL;

    if (NULL == gpDlHandle) {
        return -1;
    }

    teeTransmit = (TeeTransmit) dlsym(gpDlHandle, FUN_TRANSMIT);
    error = dlerror();
    if (error != NULL) {
        TMS_LOG_E(g_tag, "%s: dlsym[teeTransmit], error[%s]", __FUNCTION__, error);
        return -1;
    }

#ifdef TMS_TEE_CA_TA
    return teeTransmit(pCmd, cmdLen, ppRsp, pRspLen);
#else
    *pRspLen = NCI_RSP_LEN_MAX;
    *ppRsp = (uint8_t *)calloc(1, NCI_RSP_LEN_MAX);
    return teeTransmit(pCmd, cmdLen, *ppRsp, pRspLen);
#endif
}

static bool teeCloseChannel(int channelNum)
{
    TeeCloseChannel teeCloseChannel = NULL;
    char *error = NULL;

    if (NULL == gpDlHandle) {
        return -1;
    }

    teeCloseChannel = (TeeCloseChannel) dlsym(gpDlHandle, FUN_CLOSE_CHANNEL);
    error = dlerror();
    if (error != NULL) {
        TMS_LOG_E(g_tag, "%s: dlsym[teeCloseChannel], error[%s]", __FUNCTION__, error);
        return -1;
    }

#ifdef TMS_TEE_CA_TA
    return (teeCloseChannel(channelNum) == 0);
#else
    UNUSED(channelNum);
    return (teeCloseChannel() == 0);
#endif
}
}  // namespace tms
}  // namespace vendor

#endif //TMS_TEE