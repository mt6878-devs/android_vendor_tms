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

#ifdef USED_COS_I2C_DL

#include <cutils/properties.h>
#include <unistd.h>
#include <sys/time.h>

#if defined (USE_TMS_NFC) || defined (USE_C1)
    #include "tmsSecureString.h"
#else
    #include "securec.h"
#endif

#include "tmsDlCommonUtils.h"
#include "tmsReeDlCommonUtils.h"
#include "tmsDlNciUtils.h"

#include "tmsCosI2cDl.h"
#include "configC.h"

#include "TmsRee.h"
using vendor::tms::TmsRee;

#include "SEApi.h"

#include "tmslog.h"

#ifdef TAG
#undef TAG
#endif
#define TAG g_tag

static const char g_tag[] = "TmsCosDl";

/* DESCRIPTION
 * Check and download eSE COS.
 * Note: this function only can be called by nfcHal process.
 *
 * RETURN VALUE
 * return 0 (ESESTATUS_SUCCESS) download COS successfully,  or has been latest version,
 *        else return error code(eg. ESESTATUS_FAILED).
 */
int16_t eseCosDownloadI2C()
{
    TMS_LOG_D(g_tag, "%s: enter", __FUNCTION__);

    ESESTATUS status = ESESTATUS_FAILED;
    char seBlVerThreadName[] = "getSeBLVerThread";
    char fileNameKey[50] = {0}, defFileName[50] = {0};
    int32_t seBlVer = -1;
    SeInitMode initMode = ESE_MODE_NFCC_DL;
    bool forceCosDownload = false;
    bool eseSoftResetRes = true;
    bool ret = false;
    char fileName[256] = {0};
    char vtpThreadName[] = "VtpDownloadThread";
    DownloadCmdOp dlCmdOp;
    VtpParams vtpParams;
    // PT, pass-through transmission
    bool needPT2SeBl = false;
    struct timeval startTime, endTime;
    pthread_mutex_t mutex;
    int err;

    SetSeCosI2cDlTime(0);
    gettimeofday(&startTime, 0);
    pthread_mutex_lock(GetMutex());

    pthread_mutex_init(&mutex, NULL);
    if (GetGpCond() == NULL) {
        SetGpCond(new pthread_cond_t);
        pthread_cond_init(GetGpCond(), NULL);
    } else {
        TMS_LOG_E(g_tag, "%s: download is busy, eSE COS download failed", __FUNCTION__);
        goto exit_check_result;
    }

    if (!GetVaildFileName(fileName, sizeof(fileName),
                          NAME_TMS_ESE_COS_NAME, strlen(NAME_TMS_ESE_COS_NAME),
                          "THN31_ESE_VTP.txt", true) &&
        !GetVaildFileName(fileName, sizeof(fileName),
                          NAME_TMS_SEC_ESE_COS_NAME, strlen(NAME_TMS_SEC_ESE_COS_NAME),
                          "SEC_THN31_ESE_VTP.txt", true)) {
        // check eSE COS download
        TMS_LOG_I(g_tag, "cannot find VTP file, eSE COS doesn't need to download, end");
        status = ESESTATUS_SUCCESS;
        goto exit_check_result;
    }

    // 1. open nfc node
    status = openT1(initMode);
    if (ESESTATUS_SUCCESS != status) {
        TMS_LOG_E(g_tag, "open nfc node failed, eSE COS download failed");
        // open node failed, try release resource
        goto exit_close_t1;
    }

    // 2.1 DL flush I2C data
    ret = IoctlNfc(NFC_DLD_FLUSH);
    if (!ret) {
        TMS_LOG_E(g_tag, "DL flush I2C data failed, eseCosDownloadI2C download failed");
        goto exit_close_t1;
    }

    // 2.2 DL pin pull up
    ret = IoctlNfc(NFC_DLD_PWR_DL_ON);
    if (!ret) {
        TMS_LOG_E(g_tag, "DL pin pull up failed, eSE COS download failed");
        goto exit_close_t1;
    }

chip_reset:
    // initT1 failed will trigger close node, must open and try again
    if (!SeIsOpened()) {
        status = openT1(ESE_MODE_NFCC_DL);
        if (ESESTATUS_SUCCESS != status) {
            TMS_LOG_E(g_tag, "open_nfc_t1 failed, eSE COS download failed");
            // open node failed, try release resource
            goto exit_close_t1;
        }
    }
    if (forceCosDownload) {
        // 3.1 chip hard reset
        ret = ChipHardReset();
    } else {
        // 3.2 ese soft reset
        status = NfccSoftReset() ? ESESTATUS_SUCCESS : ESESTATUS_FAILED;
        if (ESESTATUS_SUCCESS != status) {
            ret = false;
        } else {
            ret = true;
        }
    }

    if (!ret) {
        if (!forceCosDownload) {
            TMS_LOG_I(g_tag, "NfccSoftReset failed, try chip hard reset");
            forceCosDownload = true;
            goto chip_reset;
        } else {
            TMS_LOG_E(g_tag, "chip hard eset failed, eSE COS download failed");

            status = ESESTATUS_FAILED;
            goto exit_close_t1;
        }
    }

    // 4. T=1 init
    status = initT1();
    if (ESESTATUS_SUCCESS != status) {
        if (!forceCosDownload) {
            // initT1 failed, try hard reset chip and force download
            TMS_LOG_I(g_tag, "EseSoftReset initT1 failed, try chip hard reset");
            forceCosDownload = true;
            goto chip_reset;
        } else {
            TMS_LOG_E(g_tag, "initT1 failed, eSE COS download failed");
            goto exit_close_t1;
        }
    }

    // 5. get BL version from chip (se BL version)
    needPT2SeBl = true;
    vtpParams = {.fileName = NULL,
                 .needPT2SeBl = needPT2SeBl,
                 .pTmsPhAbs = new TmsRee(execApduCmdChkRes),
                };
    needPT2SeBl = false;
    if (NULL == vtpParams.pTmsPhAbs) {
        status = ESESTATUS_FAILED;
        TMS_LOG_E(g_tag, "new TmsRee failed, get SE BL version failed");
        goto exit_deinit_t1;
    }

    dlCmdOp = {.routineName = seBlVerThreadName,
               .routine = GetSeBlVerThread,
               .pCond = GetGpCond(),
               .pMutex = &mutex,
               .pParameters = &vtpParams
              };
    dlCmdOp.pResStatus = new int32_t;
    if (dlCmdOp.pResStatus == NULL) {
        status = ESESTATUS_FAILED;
        TMS_LOG_E(g_tag, "new res memory failed, get SE BL version failed");
        goto exit_deinit_t1;
    }
    *((int32_t *)(dlCmdOp.pResStatus)) = -1;
    ret = startThread(&dlCmdOp, 2);  // 2s timeout
    seBlVer = *((int32_t *)(dlCmdOp.pResStatus));
    delete (int32_t *)dlCmdOp.pResStatus;
    dlCmdOp.pResStatus = NULL;
    delete vtpParams.pTmsPhAbs;
    vtpParams.pTmsPhAbs = NULL;

    if (-1 == seBlVer) {
        TMS_LOG_E(g_tag, "get SE BL version[-1] failed");
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
        TMS_LOG_I(g_tag, "is [%08x] BL, use %s script to upgrade SE COS", seBlVer, fileNameKey);
    } else {
        err = strcpy_s(fileNameKey, sizeof(fileNameKey), NAME_TMS_SEC_ESE_COS_NAME);
        if (err != EOK) {
            TMS_LOG_E(g_tag, "%s: strcpy_s err. ret:%d", __FUNCTION__, err);
        }
        err = strcpy_s(defFileName, sizeof(defFileName), "SEC_THN31_ESE_VTP.txt");
        if (err != EOK) {
            TMS_LOG_E(g_tag, "%s: strcpy_s err. ret:%d", __FUNCTION__, err);
        }
        TMS_LOG_I(g_tag, "is [%08x] BL, use %s script to upgrade SE COS", seBlVer, fileNameKey);
    }

    if (GetVaildFileName(fileName, sizeof(fileName),
                         fileNameKey, sizeof(fileNameKey), defFileName, true)) {
        property_set(PROP_KEY_SECOS_DL_STATE, "downloading");

        // 6. VTP download
        vtpParams = {.fileName = fileName,
                     .needPT2SeBl = needPT2SeBl,
                     .pTmsPhAbs = new TmsRee(execApduCmdChkRes),
                    };
        if (NULL == vtpParams.pTmsPhAbs) {
            status = ESESTATUS_FAILED;
            TMS_LOG_E(g_tag, "new TmsRee failed, SE COS I2C DL failed");
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
                   .pParameters = &vtpParams
                  };
        if ((NULL == dlCmdOp.rsp) || (NULL == dlCmdOp.pResStatus)) {
            TMS_LOG_E(g_tag, "new res memory failed, SE COS I2C DL failed");
            goto exit_deinit_t1;
        }
        *((ESESTATUS *)(dlCmdOp.pResStatus)) = ESESTATUS_FAILED;
        ret = startThread(&dlCmdOp, 5 * 60);  // 5 * 60 timeout
        status = *((ESESTATUS *)(dlCmdOp.pResStatus));
        if (ret && (ESESTATUS_SUCCESS != status)
                && !chkEseSoftReset(&dlCmdOp)) {
            eseSoftResetRes = false;
        }
        if (ESESTATUS_SUCCESS == status) {
            // I2C DL COS is long time, nfc process will be timeout and crashed.
            // rename DL script file to avoid DL COS again and again.
            char tmpFile[256] = {0};
            err = strcpy_s(tmpFile, sizeof(tmpFile), fileName);
            if (err != EOK) {
                TMS_LOG_E(g_tag, "%s: strcpy_s err. ret:%d", __FUNCTION__, err);
            }
            err = strcat_s(tmpFile, sizeof(tmpFile), ".tmp");
            if (err != EOK) {
                TMS_LOG_E(g_tag, "%s: strcat_s err. ret:%d", __FUNCTION__, err);
            }
            int res = rename(fileName, tmpFile);
            if (res != 0) {
                TMS_LOG_E(g_tag, "rename [%s] to [%s.tmp], errno = %d", fileName, fileName, errno);
            }
        }
    } else {
        // check eSE COS download
        TMS_LOG_I(g_tag, "eSE COS doesn't need to download, end");
        status = ESESTATUS_SUCCESS;
    }

exit_deinit_t1:
    // 7. T=1 deinitialized
    deInitT1();

    // 8. free memory
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

    // 9. maybe retry chip hard reset
    if (!eseSoftResetRes && !forceCosDownload) {
        // ESE soft reset failed, try hard reset chip and force download
        TMS_LOG_I(g_tag, "EseSoftReset vtp first cmd failed, try chip hard reset");
        eseSoftResetRes = true;
        forceCosDownload = true;
        goto chip_reset;
    }

exit_close_t1:
    // 10. DL pull down, hard reset and close /dev/tms_ese
    ret = IoctlNfc(NFC_DLD_PWR_DL_OFF);
    usleep(10 * 1000);  // sleep 10 * 1000us
    ret = ChipHardReset();
    CloseT1();

exit_check_result:
    // 11. set result to property
    if (ESESTATUS_SUCCESS == status) {
        property_set(PROP_KEY_SECOS_DL_STATE, "success");
    } else {
        property_set(PROP_KEY_SECOS_DL_STATE, "failure");
    }

    // 12. release resource
    if (GetGpCond() != NULL) {
        pthread_cond_destroy(GetGpCond());
        delete GetGpCond();
        SetGpCond(NULL);
    }
    pthread_mutex_destroy(&mutex);

    pthread_mutex_unlock(GetMutex());

    gettimeofday(&endTime, 0);
    SetSeCosI2cDlTime((int)(endTime.tv_sec - startTime.tv_sec));
    TMS_LOG_D(g_tag, "%s: exit. used time[%ds]. COS I2C DL status[%d]",
              __FUNCTION__, GetSeCosI2cDlTime(), status);
    if (GetNfcWatchDogTime() == 0) {
        SetNfcWatchDogTime(ConfigGetUnsigned(NAME_TMS_NFC_WATCH_DOG_TIMEOUT,
                                             strlen(NAME_TMS_NFC_WATCH_DOG_TIMEOUT), 89)); // 89 length
    }
    if ((GetSeCosI2cDlTime() + GetNfccFwDlTime() + GetNfccBlDlTime()) > GetNfcWatchDogTime()) {
        // NfcService has been crash, must exit nfcHal to reset sub-system
        abort();
    }
    return status;
}

#endif // USED_COS_I2C_DL
