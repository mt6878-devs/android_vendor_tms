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

#include <cutils/properties.h>
#include <cinttypes>
#include <sys/time.h>

#if defined (USE_TMS_NFC) || defined (USE_C1)
    #include "tmsSecureString.h"
#else
    #include "securec.h"
#endif

#include "tmsCommon.h"
#include "tmsDlCommonUtils.h"
#include "tmsCosPthDl.h"
#include "configC.h"
#include "eseConfig.h"

#include "tmslog.h"

#ifdef TAG
#undef TAG
#endif
#define TAG g_tag

static const char g_tag[] = "TmsCosDl:CosPthDl";

static ESESTATUS vtpDlPatch(void *arg);
static void *VtpDownloadPthThread(void *arg);
bool IsEndWith(const char *str, int strLen, const char *endStr, int endStrLen);

/* DESCRIPTION
 * Check and download eSE COS by COS patch.
 *
 * @params    isWiredMode - true, use I2C(ApudGate), or false use SPI to  download patch.
 * @params    isTEE - true, use TEE-SE-TA, or false use REE-SPI(T=1) to  download patch.
 * @params    searchNfcDir - true, search patch file in /data/vendor/nfc/,
              or false search patch file in /data/vendor/secure_element/.
 *
 * RETURN VALUE
 * return 0 (ESESTATUS_SUCCESS) download COS successfully,  or has been latest version,
 *        else return error code(eg. ESESTATUS_FAILED).
 */
int16_t eseCosDlPth(ITmsPhAbs *pTmsPhAbs, bool searchNfcDir)
{
    TMS_LOG_D(g_tag, "%s: enter", __FUNCTION__);

    ESESTATUS status = ESESTATUS_FAILED;
    bool dlRes = false;
    bool ret = false;
    char vtpThreadName[] = VTP_DL_CPS_PTH_ROUTINE;
    DownloadCmdOp dlCmdOp;
    VtpParams vtpParams;
    struct timeval startTime, endTime;
    pthread_mutex_t mutex;

    gettimeofday(&startTime, nullptr);
    pthread_mutex_lock(GetMutex());

    pthread_mutex_init(&mutex, NULL);
    if (GetGpCond() == NULL) {
        SetGpCond(new pthread_cond_t);
        pthread_cond_init(GetGpCond(), NULL);
    } else {
        TMS_LOG_E(g_tag, "%s: download is busy, SE COS patch DL failed", __FUNCTION__);
        goto exit_check_result;
    }

    property_set(PROP_KEY_SECOS_DL_STATE, "downloading");
    vtpParams = {
        .fileName = NULL,
        .needPT2SeBl = false,
        .isSearchNfcDir = searchNfcDir,
        .pTmsPhAbs = pTmsPhAbs,
    };
    dlCmdOp = {.routineName = vtpThreadName,
               .cmd = NULL,
               .rsp = new uint8_t[NCI_RSP_LEN_MAX],
               .rspChk = NULL,
               .cmdLen = 0,
               .rspLen = NCI_RSP_LEN_MAX,
               .rspChkLen = 0,
               .pResStatus = new ESESTATUS,
               .routine = VtpDownloadPthThread,
               .pCond = GetGpCond(),
               .pMutex = &mutex,
               .pParameters = &vtpParams,
              };
    if ((NULL == dlCmdOp.rsp) || (NULL == dlCmdOp.pResStatus)) {
        TMS_LOG_E(g_tag, "new res memory failed, eSE COS patch download failed");
        goto exit_cleanup;
    }
    *((ESESTATUS *)(dlCmdOp.pResStatus)) = ESESTATUS_FAILED;
    ret = startThread(&dlCmdOp, 60);  // 60 timeout
    status = *((ESESTATUS *)(dlCmdOp.pResStatus));

    dlRes = ret && (ESESTATUS_SUCCESS == status)
            && chkEseSoftReset(&dlCmdOp);

exit_cleanup:
    // 3. free memory
    if (NULL != dlCmdOp.rsp) {
        delete[](uint8_t *)dlCmdOp.rsp;
    }
    if (NULL != dlCmdOp.pResStatus) {
        delete (ESESTATUS *)dlCmdOp.pResStatus;
    }

exit_check_result:

    if (dlRes) {
        property_set(PROP_KEY_SECOS_DL_STATE, "success");
    } else {
        property_set(PROP_KEY_SECOS_DL_STATE, "failure");
    }

    if (GetGpCond() != NULL) {
        pthread_cond_destroy(GetGpCond());
        delete GetGpCond();
        SetGpCond(NULL);
    }
    pthread_mutex_destroy(&mutex);
    pthread_mutex_unlock(GetMutex());

    gettimeofday(&endTime, nullptr);
    TMS_LOG_D(g_tag, "%s: exit. used time[%ds]. SE COS patch DL status[%d]",
              __FUNCTION__, (int)(endTime.tv_sec - startTime.tv_sec), status);
    return status;
}

static uint64_t GetSeCosVerFromChip(void *arg)
{
    uint64_t cosVer = INVALID_COS_VER;
    DownloadCmdOp *pDlCmdOp = (DownloadCmdOp *)arg;
    ESESTATUS status = ESESTATUS_FAILED;
    bool isNewRspMem = false;

    if (NULL == pDlCmdOp->rsp) {
        pDlCmdOp->rsp = new uint8_t[NCI_RSP_LEN_MAX];
        pDlCmdOp->rspLen = NCI_RSP_LEN_MAX;
        if (NULL == pDlCmdOp->rsp) {
            TMS_LOG_E(g_tag, "%s: new res memory failed", __FUNCTION__);
            return INVALID_COS_VER;
        }
        isNewRspMem = true;
    }

    status = execApduCmdChkRes("00a4040008544D43524F4F5401",
                               strlen("00a4040008544D43524F4F5401"), pDlCmdOp);
    if (ESESTATUS_SUCCESS == status) {
        // Ignore check this cmd result
        status = execApduCmdChkRes("80E20000087072694E4643434D",
                                   strlen("80E20000087072694E4643434D"), pDlCmdOp);

        status = execApduCmdChkRes("80E265020E", strlen("80E265020E"), pDlCmdOp);
        if (ESESTATUS_SUCCESS == status) {
            int len = pDlCmdOp->rspLen;
            // Len is cosVerLen + sw1sw2 = 14 + 2 = 16
            if (len == 16) {
                CosVersion cv;
                uint32_t pthVer = 0;
                uint32_t baseVer = 0;
                int err = memcpy_s(&cv, sizeof(CosVersion), pDlCmdOp->rsp, sizeof(CosVersion));
                if (err != EOK) {
                    TMS_LOG_E(g_tag, "%s: memcpy_s err. ret:%d", __FUNCTION__, err);
                }
                pthVer = (((uint32_t)cv.patchVer[0]) << 8) |  // 8 length
                         cv.patchVer[1];
                baseVer = (((uint32_t)cv.baseVer[0]) << 16) |  // 16 length
                          (((uint32_t)cv.baseVer[1]) << 8) |  // 8 length
                          cv.baseVer[2];  // 2 baseversion
                cosVer = (static_cast<uint64_t>(pthVer) << 24) | static_cast<uint64_t>(baseVer);  // 24 pthVer
                TMS_LOG_I(g_tag, "%s: chip cosVer = %010" PRIx64", pthVer = %04x, baseVer = %06x",
                          __FUNCTION__, (uint64_t)cosVer, (uint32_t)pthVer, (uint32_t)baseVer);
            }
        }
    }

    if (isNewRspMem && (NULL != pDlCmdOp->rsp)) {
        delete[] pDlCmdOp->rsp;
        pDlCmdOp->rsp = NULL;
        pDlCmdOp->rspLen = 0;
    }

    return cosVer;
}

static ESESTATUS chkResAndTryRcv(DownloadCmdOp *pDlCmdOp, ESESTATUS status, uint64_t vtpVer)
{
    int err;

    if ((ESESTATUS_SUCCESS != status)
            || (vtpVer != GetSeCosVerFromChip(pDlCmdOp))) {
        TMS_LOG_E(g_tag, "%s: COS patch DL fail, status = %d",
                  __FUNCTION__, status);
        char *fileName = ((VtpParams *)pDlCmdOp->pParameters)->fileName;
        status = ESESTATUS_COS_RCV_SUCCESS;
        // Recover to base version
        if (IsEndWith(fileName, strlen(fileName), ".bin", strlen(".bin"))) {
            int nameLen = strlen(fileName) - strlen(".bin");
            err = strncpy_s(fileName + nameLen, FILE_NAME_LEN - nameLen, ".rcv.bin", strlen(".rcv.bin") + 1);
            if (EOK != err) {
                TMS_LOG_E(g_tag, "%s: strncpy_s err. ret:%d", __FUNCTION__, err);
            }
            status = vtpDownload(pDlCmdOp);
        } else {
            int nameLen = strlen(fileName);
            err = strncpy_s(fileName + nameLen, FILE_NAME_LEN - nameLen, ".rcv", 5);  // 5 length
            if (EOK != err) {
                TMS_LOG_E(g_tag, "%s: strncpy_s err. ret:%d", __FUNCTION__, err);
            }
            status = vtpDownload(pDlCmdOp);
        }

        if (ESESTATUS_SUCCESS != status) {
            status = ESESTATUS_COS_RCV_FAILED;
        } else {
            status = ESESTATUS_COS_RCV_SUCCESS;
        }
    }

    return status;
}

static ESESTATUS vtpDlPatch(void *arg)
{
    ESESTATUS status = ESESTATUS_FAILED;
    int channelNum = -1;

    DownloadCmdOp *pDlCmdOp = (DownloadCmdOp *)arg;
    VtpParams *pVtpParams = (VtpParams *)pDlCmdOp->pParameters;
    ITmsPhAbs *pTmsPhAbs = NULL;
    char fileName[FILE_NAME_LEN] = {0};
    ChipInfo chipInfo;
    uint64_t chipVer = INVALID_COS_VER;
    uint64_t vtpVer = INVALID_COS_VER;
    uint64_t chipVerMaster = 0;
    uint64_t chipVerPatch = 0;
    uint64_t vtpVerMaster = 0;
    uint64_t vtpVerPatch = 0;
    bool isExit = false;
    TMS_LOG_D(g_tag, "%s enter", __FUNCTION__);

    if (pVtpParams == NULL) {
        return status;
    } else {
        pTmsPhAbs = pVtpParams->pTmsPhAbs;
    }

    channelNum = pTmsPhAbs->openBasicChannel();
    if (0 != channelNum) {
        pTmsPhAbs->closeChannel(channelNum);
        return status;
    }

    chipVer = GetSeCosVerFromChip(pDlCmdOp);
    if (INVALID_COS_VER != chipVer) {
        chipVerMaster = chipVer & COS_VER_MASTER_MASK;
        chipInfo.cosBaseVer = (uint32_t)chipVerMaster;
        if (!GetVaildFileName(fileName, sizeof(fileName),
                              NAME_TMS_ESE_COS_PATCH_NAME, strlen(NAME_TMS_ESE_COS_PATCH_NAME),
                              "THN31_ESE_VTP.patch", pVtpParams->isSearchNfcDir)) {
            // check eSE COS download
            TMS_LOG_I(g_tag, "cannot find patch file, SE COS patch doesn't need to DL, end");
            status = ESESTATUS_SUCCESS;
            isExit = true;
        }

        if (!isExit) {
            pVtpParams->fileName = fileName;
            pVtpParams->pChipInfo = &chipInfo;
            vtpVer = GetCosVerFromVtp(fileName, strlen(fileName), chipInfo);
            if (INVALID_COS_VER != vtpVer) {
                chipVerPatch = chipVer & COS_VER_PATCH_MASK;
                vtpVerMaster = vtpVer & COS_VER_MASTER_MASK;
                vtpVerPatch = vtpVer & COS_VER_PATCH_MASK;
                if ((chipVerMaster == vtpVerMaster) &&
                    ((vtpVerPatch > chipVerPatch) ||
                     ((vtpVerPatch != chipVerPatch) &&
                      (1 == EseConfig::getUnsigned(NAME_COS_PTH_VER_CHK_DIFF, 1))))) {
                    // DL patch
                    status = vtpDownload(arg);

                    // COS has been system reset to make sure the patch has been running
                    // Fixed GetSeCosVerFromChip exception when COS has been system reset.
                    pTmsPhAbs->closeChannel(channelNum);
                    channelNum = pTmsPhAbs->openBasicChannel();

                    status = chkResAndTryRcv(pDlCmdOp, status, vtpVer);
                } else {
                    if (vtpVer == chipVer) {
                        TMS_LOG_I(g_tag, "%s: Has been latest COS", __FUNCTION__);
                        status = ESESTATUS_SUCCESS;
                    } else {
                        TMS_LOG_E(g_tag, "%s: cannot degrade version. chipVer = %010" PRIx64", vtpVer = %010" PRIx64,
                                  __FUNCTION__, chipVer, vtpVer);
                    }
                }
            } else {
                TMS_LOG_I(g_tag, "%s: GetCosVerFromVtp is -1, COS patch DL fail", __FUNCTION__);
            }
        }
    } else {
        TMS_LOG_I(g_tag, "%s: GetSeCosVerFromChip is -1, COS patch DL fail", __FUNCTION__);
    }

    pTmsPhAbs->closeChannel(channelNum);

    TMS_LOG_D(g_tag, "%s exit [%d]", __FUNCTION__, status);
    return status;
}

static void *VtpDownloadPthThread(void *arg)
{
    if (!RegisterExitSignal()) {
        return nullptr;
    }

    DownloadCmdOp *pDlCmdOp = (DownloadCmdOp *)arg;
    SetThreadRunning(true);
    *((int16_t *)(pDlCmdOp->pResStatus)) = vtpDlPatch(arg);
    SetThreadRunning(false);

    pthread_mutex_lock(pDlCmdOp->pMutex);
    pthread_cond_signal(pDlCmdOp->pCond);
    pthread_mutex_unlock(pDlCmdOp->pMutex);

    return nullptr;
}
