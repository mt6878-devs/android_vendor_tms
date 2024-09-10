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

#ifdef USED_COS_REE_SPI_DL

#include <cutils/properties.h>
#include <sys/time.h>

#include "tmsDlCommonUtils.h"
#include "tmsReeDlCommonUtils.h"
#include "tmsCosPthDl.h"
#include "tmsCosReeSpiDl.h"

#include "TmsRee.h"
using vendor::tms::TmsRee;

#include "configC.h"
#include "tmslog.h"

#if defined (USE_TMS_NFC) || defined (USE_C1)
    #include "tmsSecureString.h"
    #include "NfcAdaptation.h"
#else
    #include "securec.h"
#endif

#include "SEApi.h"

#ifdef TAG
#undef TAG
#endif
#define TAG g_tag

static const char g_tag[] = "TmsCosDl:ReeSpiDl";


/* DESCRIPTION
 * Check and download eSE COS
 *
 * RETURN VALUE
 * return 0 (ESESTATUS_SUCCESS) download COS successfully,  or has been latest version,
 *        else return error code(eg. ESESTATUS_FAILED).
 */
int16_t eseCosReeDl()
{
    TMS_LOG_D(g_tag, "%s: enter", __FUNCTION__);

    ESESTATUS status = ESESTATUS_FAILED;
    char seBlVerThreadName[] = "getSeBLVerThread";
    char fileNameKey[50] = {0}, defFileName[50] = {0};
    int32_t seBlVer = -1;
    bool forceCosDownload = false;
    bool eseSoftResetRes = true;
    bool ret = false;
    bool t1IsOpened = false;
    bool jumpToCosFlag = false;
    char fileName[256] = {0};
    char vtpThreadName[] = "VtpDownloadThread";
    DownloadCmdOp dlCmdOp;
    VtpParams vtpParams;
    struct timeval startTime, endTime;
    pthread_mutex_t mutex;
    int err;

    gettimeofday(&startTime, 0);
    //1. lock and check download is busy or not
    pthread_mutex_lock(GetMutex());

    pthread_mutex_init(&mutex, NULL);
    if (GetGpCond() == NULL) {
        SetGpCond(new pthread_cond_t);
        pthread_cond_init(GetGpCond(), NULL);
    } else {
        TMS_LOG_E(g_tag, "%s: download is busy, SE COS DL failed", __FUNCTION__);
        goto exit_check_result;
    }

chip_reset:
    if (forceCosDownload) {
        //2. open nfc node
        status = openT1(ESE_MODE_ESE_DL);
        if (ESESTATUS_SUCCESS != status) {
            TMS_LOG_E(g_tag, "%s: openT1 failed, SE COS DL failed", __FUNCTION__);
            goto exit_check_result;
        }
        t1IsOpened = true;

        //3.1 chip hard reset
        ret = ChipHardReset(VEN_SET_WAIT_TIME_20MS, VEN_SET_WAIT_TIME_7MS);
    } else {
        //3.2 ese soft reset
#if defined (USE_TMS_NFC) || defined (USE_C1)
        NfcAdaptation nfcAdaptation = NfcAdaptation::GetInstance();
        nfcAdaptation.Initialize();
        status = nfcAdaptation.EseSoftReset();
        if (ESESTATUS_SUCCESS != status) {
            ret = false;
        } else {
            ret = true;
        }
#else
        EseSoftReset();
#endif
    }

    if (!ret) {
        if (!forceCosDownload) {
            TMS_LOG_I(g_tag, "%s: EseSoftReset failed, try chip hard reset", __FUNCTION__);
            forceCosDownload = true;
            goto chip_reset;
        } else {
            TMS_LOG_E(g_tag, "%s: chip hard eset failed, SE COS DL failed", __FUNCTION__);
            CloseT1();
            status = ESESTATUS_FAILED;
            goto exit_check_result;
        }
    }

    //4. open /dev/tms_ese
    if (!t1IsOpened) {
        status = openT1(ESE_MODE_ESE_DL);
        if (ESESTATUS_SUCCESS != status) {
            TMS_LOG_E(g_tag, "%s: openT1 failed, SE COS DL failed", __FUNCTION__);
            goto exit_check_result;
        }
    }

    //5. T=1 init
    status = initT1();
    if (ESESTATUS_SUCCESS != status) {
        if (!forceCosDownload) {
            //initT1 failed, try hard reset chip and force download
            TMS_LOG_I(g_tag, "%s: EseSoftReset initT1 failed, try chip hard reset", __FUNCTION__);
            forceCosDownload = true;
            goto chip_reset;
        } else {
            TMS_LOG_E(g_tag, "%s: initT1 failed, SE COS DL failed", __FUNCTION__);
            goto exit_check_result;
        }
    }

    jumpToCosFlag = true;
    //6. get BL version from chip (se BL version)
    vtpParams = {.fileName = NULL,
                 .needPT2SeBl = false,
                 .isNfccBlState = false,
                 .pTmsPhAbs = new TmsRee(execApduCmdChkRes),
                };
    if (NULL == vtpParams.pTmsPhAbs) {
        status = ESESTATUS_FAILED;
        TMS_LOG_E(g_tag, "%s: new TmsRee failed, get SE BL version failed", __FUNCTION__);
        goto exit_deinit_t1;
    }

    dlCmdOp = {.routineName = seBlVerThreadName,
               .routine = GetSeBlVerThread,
               .pCond = GetGpCond(),
               .pMutex = &mutex,
               .pParameters = &vtpParams,
              };
    dlCmdOp.pResStatus = new int32_t;
    if (dlCmdOp.pResStatus == NULL) {
        status = ESESTATUS_FAILED;
        TMS_LOG_E(g_tag, "%s: new res memory failed, get SE BL version failed", __FUNCTION__);
        goto exit_deinit_t1;
    }
    *((int32_t *)(dlCmdOp.pResStatus)) = -1;
    ret = startThread(&dlCmdOp, 2);
    seBlVer = *((int32_t *)(dlCmdOp.pResStatus));
    delete (int32_t *)dlCmdOp.pResStatus;
    dlCmdOp.pResStatus = NULL;
    delete vtpParams.pTmsPhAbs;
    vtpParams.pTmsPhAbs = NULL;

    if (-1 == seBlVer) {
        TMS_LOG_E(g_tag, "%s: get SE BL version[-1] failed", __FUNCTION__);
        status = ESESTATUS_FATAL_ERROR;
        goto exit_deinit_t1;
    } else if (seBlVer <= 0x00001106) {
        err = strcpy_s(fileNameKey, sizeof(fileNameKey), NAME_TMS_ESE_COS_NAME);
        if (err != EOK) {
            TMS_LOG_E(g_tag, "%s: strcpy_s err. ret:%d", __FUNCTION__, err);
        }
        err = strcpy_s(defFileName, sizeof(defFileName), "THN31_ESE_VTP.txt");
        if (err != EOK) {
            TMS_LOG_E(g_tag, "%s: strcpy_s err. ret:%d", __FUNCTION__, err);
        }
        TMS_LOG_I(g_tag, "%s: is [%08x] BL, use %s script to upgrade SE COS",
                  __FUNCTION__, seBlVer, fileNameKey);
    } else {
        err = strcpy_s(fileNameKey, sizeof(fileNameKey), NAME_TMS_SEC_ESE_COS_NAME);
        if (err != EOK) {
            TMS_LOG_E(g_tag, "%s: strcpy_s err. ret:%d", __FUNCTION__, err);
        }
        err = strcpy_s(defFileName, sizeof(defFileName), "SEC_THN31_ESE_VTP.txt");
        if (err != EOK) {
            TMS_LOG_E(g_tag, "%s: strcpy_s err. ret:%d", __FUNCTION__, err);
        }
        TMS_LOG_I(g_tag, "%s: is [%08x] BL, use %s script to upgrade SE COS",
                  __FUNCTION__, seBlVer, fileNameKey);
    }

    if (GetVaildFileName(fileName, sizeof(fileName),
                         fileNameKey, sizeof(fileNameKey), defFileName, false)) {
        TMS_LOG_I(g_tag, "%s: eSE COS needs to download, continue ...", __FUNCTION__);
        property_set(PROP_KEY_SECOS_DL_STATE, "downloading");

        //6. VTP download
        vtpParams = {.fileName = fileName,
                     .needPT2SeBl = false,
                     .isNfccBlState = false,
                     .pTmsPhAbs = new TmsRee(execApduCmdChkRes),
                     .pChipInfo = nullptr,
                    };
        if (NULL == vtpParams.pTmsPhAbs) {
            status = ESESTATUS_FAILED;
            TMS_LOG_E(g_tag, "%s: new TmsRee failed, SE COS DL failed", __FUNCTION__);
            goto exit_deinit_t1;
        }

        dlCmdOp = {.routineName = vtpThreadName,
                   .cmd = NULL,
                   .rsp = new uint8_t[NCI_RSP_LEN_MAX],
                   .rspChk = NULL,
                   .cmdLen = 0,
                   .rspLen = NCI_RSP_LEN_MAX,
                   .rspChkLen = 0,
                   .pResStatus = new ESESTATUS,
                   .routine = VtpDownloadThread,
                   .pCond = GetGpCond(),
                   .pMutex = &mutex,
                   .pParameters = &vtpParams,
                  };
        if ((NULL == dlCmdOp.rsp) || (NULL == dlCmdOp.pResStatus)) {
            TMS_LOG_E(g_tag, "%s: new res memory failed, SE COS DL failed", __FUNCTION__);
            goto exit_deinit_t1;
        }
        *((ESESTATUS *)(dlCmdOp.pResStatus)) = ESESTATUS_FAILED;
        ret = startThread(&dlCmdOp, 3 * 60);
        status = *((ESESTATUS *)(dlCmdOp.pResStatus));
        if (ret && (ESESTATUS_SUCCESS != status)
                && !chkEseSoftReset(&dlCmdOp)) {
            eseSoftResetRes = false;
        } else {
            if (!chkEseSoftReset(&dlCmdOp)) {
                jumpToCosFlag = true;
            } else {
                jumpToCosFlag = false;
            }
        }

        delete[](uint8_t *)dlCmdOp.rsp;
        dlCmdOp.rsp = NULL;
        delete (ESESTATUS *)dlCmdOp.pResStatus;
        dlCmdOp.pResStatus = NULL;
        delete vtpParams.pTmsPhAbs;
        vtpParams.pTmsPhAbs = NULL;
    } else {
        //check eSE COS download
        TMS_LOG_I(g_tag, "%s: SE COS doesn't need to DL, end", __FUNCTION__);
        status = ESESTATUS_SUCCESS;
    }

exit_deinit_t1:
    if (ESESTATUS_SUCCESS != status || jumpToCosFlag) {
        //7. try jump to COS from SE-BL
        //TryJumpToCos is a part of deinitialization
        vtpParams = {.fileName = NULL,
                     .needPT2SeBl = false,
                     .isNfccBlState = false,
                     .pTmsPhAbs = new TmsRee(execApduCmdChkRes),
                    };
        if (NULL != vtpParams.pTmsPhAbs) {
            dlCmdOp = {.routineName = seBlVerThreadName,
                       .routine = TryJumpToCosThread,
                       .pCond = GetGpCond(),
                       .pMutex = &mutex,
                       .pParameters = &vtpParams,
                      };
            ret = startThread(&dlCmdOp, 2);
            delete vtpParams.pTmsPhAbs;
            vtpParams.pTmsPhAbs = NULL;
        } else {
            TMS_LOG_E(g_tag, "%s: new TmsRee failed, TryJumpToCosThread failed", __FUNCTION__);
        }
    }

    //8. T=1 deinitialized and close /dev/tms_ese
    deInitT1();
    CloseT1();

    //9. free memory
    if (dlCmdOp.rsp != NULL) {
        delete[](uint8_t *)dlCmdOp.rsp;
        dlCmdOp.rsp = NULL;
    }
    if (dlCmdOp.pResStatus != NULL) {
        delete (ESESTATUS *)dlCmdOp.pResStatus;
        dlCmdOp.pResStatus = NULL;
    }
    if (vtpParams.pTmsPhAbs != NULL) {
        delete vtpParams.pTmsPhAbs;
        vtpParams.pTmsPhAbs = NULL;
    }

    //10. maybe retry chip hard reset
    if (!eseSoftResetRes && !forceCosDownload) {
        //ESE soft reset failed, try hard reset chip and force download
        TMS_LOG_I(g_tag, "%s: EseSoftReset vtp first cmd failed, try chip hard reset", __FUNCTION__);
        eseSoftResetRes = true;
        forceCosDownload = true;
        goto chip_reset;
    }

exit_check_result:
    //11. set result to property
    if (ESESTATUS_SUCCESS == status) {
        property_set(PROP_KEY_SECOS_DL_STATE, "success");
    } else {
        property_set(PROP_KEY_SECOS_DL_STATE, "failure");
    }

    //12. release resource and unlock
    if (GetGpCond() != NULL) {
        pthread_cond_destroy(GetGpCond());
        delete GetGpCond();
        SetGpCond(NULL);
    }
    pthread_mutex_destroy(&mutex);
    pthread_mutex_unlock(GetMutex());

    gettimeofday(&endTime, 0);
    TMS_LOG_D(g_tag, "%s: exit. used time[%ds]. SE COS DL status[%d]",
              __FUNCTION__, (int)(endTime.tv_sec - startTime.tv_sec), status);
    return status;
}

/* DESCRIPTION
 * Check and download eSE COS by COS patch.
 *
 * RETURN VALUE
 * return 0 (ESESTATUS_SUCCESS) download COS successfully,  or has been latest version,
 *        else return error code(eg. ESESTATUS_FAILED).
 */
int16_t eseCosReeDlPth()
{
    ESESTATUS status = ESESTATUS_FAILED;
    ITmsPhAbs *pTmsPhAbs = new TmsRee(execApduCmdChkRes);
    if (NULL == pTmsPhAbs) {
        TMS_LOG_E(g_tag, "new TmsRee failed, SE COS patch DL failed");
        status = ESESTATUS_MEM_EXCEPTION;
    } else {
        status = (ESESTATUS)eseCosDlPth(pTmsPhAbs, false);
        delete pTmsPhAbs;
    }

    return status;
}

#endif //USED_COS_REE_SPI_DL
