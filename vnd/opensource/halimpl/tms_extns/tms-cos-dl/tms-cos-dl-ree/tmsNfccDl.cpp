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

#include <cstring>
#include <unistd.h>
#include <cutils/properties.h>

#include "tmsDlCommonUtils.h"
#include "tmsDlNciUtils.h"
#include "tmsNfccDl.h"
#include "tmsReeDlCommonUtils.h"
#include "configC.h"

#include "TmsRee.h"
using vendor::tms::TmsRee;

#include "tmslog.h"


#define NFC_FW_UPDATE_SUCC "1"
#define NFC_FW_UPDATE_FAIL "0"
#if defined (USE_TMS_NFC) || defined (USE_C1)
#include "tmsSecureString.h"
    #define PROP_KEY_FW_DL_STATE "vendor.tms.nfc.fw.download"
    #define PROP_KEY_NFCC_BL_DL_STATE "vendor.tms.nfc.bl.download"
    #define PROP_KEY_SECOS_DL_STATE "vendor.tms.nfc.secos.download"
    #define UPDATE_NFC_FW_STATE(state) { \
        UNUSED(state);                   \
    }
#else
#include "securec.h"
#include "phNxpNciHal_Adaptation.h"
// USE MACO Owner1 compile fail, but in RK build success
// MACO_INCLUDE_phC1NciHal_Adaptation
#define PROP_KEY_FW_DL_STATE "nfc.vendor.tms.nfc.fw.download"
#define PROP_KEY_NFCC_BL_DL_STATE "nfc.vendor.tms.nfc.bl.download"
#define PROP_KEY_SECOS_DL_STATE "nfc.vendor.tms.nfc.secos.download"
// #UPDATE_NFC_FW_STATE(state) will be defined in Android.bp/mk
#endif

#ifdef TAG
#undef TAG
#endif
#define TAG g_tag

static const char g_tag[] = "TmsCosDl:NfccDl";

static bool isNeedDownload(uint32_t vtpFwVer, uint32_t chipFwVer,
                           bool *forceFwDownload, ESESTATUS *status)
{
    bool isNeedDownloadFlag = false;
    if (vtpFwVer != INVALID_FW_VER) {
        if (INVALID_FW_VER == chipFwVer) {
            // force fw download
            *forceFwDownload = true;
            isNeedDownloadFlag = true;
        } else if (vtpFwVer != chipFwVer) {
            // soft reset and fw download
            isNeedDownloadFlag = true;
        } else {
            TMS_LOG_I(g_tag, "Has been latest FW: %06x", chipFwVer);
            *status = ESESTATUS_SUCCESS;
            isNeedDownloadFlag = false;
        }
    } else {
        TMS_LOG_E(g_tag, "GetCosVerFromVtp failed, NFCC FW download failed");
        isNeedDownloadFlag = false;
    }

    return isNeedDownloadFlag;
}

/* DESCRIPTION
 * Check and download NFCC FW
 *
 * RETURN VALUE
 * return 0 (ESESTATUS_SUCCESS) download FW successfully or has been latest version,
 *        else return error code(eg. ESESTATUS_FAILED).
 */
int16_t NfccFwDownload()
{
    ESESTATUS status = ESESTATUS_FAILED;
    char fileName[256] = { 0 };
    ChipInfo chipInfo;
    bool isBlState = false;
    uint32_t vtpFwVer = INVALID_FW_VER;
    uint32_t bakFwVer = INVALID_FW_VER;
    bool forceFwDownload = false;
    // ioctl result
    bool ret = false;

    char verThreadName[] = "getVerThread";
    char vtpThreadName[] = VTP_DL_FW_ROUTINE;
    DownloadCmdOp dlCmdOp;
    VtpParams vtpParams;

    struct timeval startTime, endTime;
    pthread_mutex_t mutex;

    InitSELogLevel();
    TMS_LOG_D(g_tag, "%s: enter", __FUNCTION__);

    SetNfccFwDlTime(0);
    gettimeofday(&startTime, 0);
    pthread_mutex_lock(GetMutex());

    pthread_mutex_init(&mutex, nullptr);
    if (GetGpCond() == nullptr) {
        SetGpCond(new pthread_cond_t);
        pthread_cond_init(GetGpCond(), nullptr);
    } else {
        TMS_LOG_E(g_tag, "%s: download is busy, NFCC FW download failed", __FUNCTION__);
        goto exit_check_result;
    }

    // 0.1 check if need to remove override nfcc fw file
    checkAndRemoveOverrideVtpFile();

    // 1. check download file exist or not
    if (!GetVaildFileName(fileName, 256,  // 256 length
                          NAME_TMS_SEC_NFC_FW_NAME, strlen(NAME_TMS_SEC_NFC_FW_NAME),
                          "SEC_THN31_FW_VTP.txt", true)) {
        // check eSE COS download
        TMS_LOG_I(g_tag, "%s: cannot find VTP file, FW doesn't need to download, end", __FUNCTION__);
        status = ESESTATUS_SUCCESS;
        goto exit_check_result;
    }

    // 2. open /dev/<nfcNode>
    status = openT1(ESE_MODE_NFCC_DL);
    if (ESESTATUS_SUCCESS != status) {
        TMS_LOG_E(g_tag, "%s: open_nfc_t1 failed, openT1 failed", __FUNCTION__);
        goto exit_check_result;
    }

    // 3.1 DL flush I2C data
    ret = IoctlNfc(NFC_DLD_FLUSH);
    usleep(10 * 1000);  // sleep 10 * 1000us
    if (!ret) {
        TMS_LOG_E(g_tag, "DL flush I2C data failed, NFCC FW download failed");
        goto exit_close_t1;
    }

    status = ESESTATUS_FAILED;
    // 3.2 get fw and fwBL version from chip (fwVer, fwBlVer)
    vtpParams = {.fileName = nullptr,
                 .needPT2SeBl = false,
                 .isNfccBlState = false,
                 .pTmsPhAbs = new TmsRee(nullptr),
                };
    if (nullptr == vtpParams.pTmsPhAbs) {
        status = ESESTATUS_FAILED;
        TMS_LOG_E(g_tag, "%s: new TmsRee failed, get NFCC FW/BL version failed", __FUNCTION__);
        goto exit_close_t1;
    }
    dlCmdOp = {.routineName = verThreadName,
               .cmd = nullptr,
               .rsp = nullptr,
               .rspChk = nullptr,
               .cmdLen = 0,
               .rspLen = 0,
               .rspChkLen = 0,
               .pResStatus = &chipInfo,
               .routine = GetNfccChipInfoFromFWThread,
               .pCond = GetGpCond(),
               .pMutex = &mutex,
               .pParameters = &vtpParams,
              };
    ret = startThread(&dlCmdOp, 4);  // 4s timeout
    delete vtpParams.pTmsPhAbs;
    vtpParams.pTmsPhAbs = nullptr;
    if (INVALID_FW_VER != chipInfo.fwVer) {
        bakFwVer = chipInfo.fwVer;
    }
VTP_FILE_BY_BL_VER:
    // 4. get fw version from VTP file (fwVerVtp)
    if (INVALID_FW_VER == chipInfo.nfccBlVer) {
        if (isBlState) {
            status = ESESTATUS_FATAL_ERROR;
            TMS_LOG_E(g_tag, "%s: get NFCC BL failed, NFCC FW DL failed", __FUNCTION__);
            goto exit_deinit_t1;
        } else {
            goto NFC_DLD_PWR_DL_ON;
        }
    }

    if (INVALID_FW_VER == chipInfo.fwVer) {
        chipInfo.fwVer = bakFwVer;
    }
    vtpFwVer = (uint32_t)GetCosVerFromVtp(fileName, sizeof(fileName), chipInfo);  // 256 length
    // 5. assert(fwVer, fwVerVtp)
    if (!isNeedDownload(vtpFwVer, chipInfo.fwVer, &forceFwDownload, &status)
            && (1 == EseConfig::getUnsigned(NAME_FW_DL_CHK_VER, 1))) {
        if (isBlState) {
            goto exit_deinit_t1;
        } else {
            goto exit_close_t1;
        }
    }

    if (isBlState) {
        goto VTP_DLD;
    }

NFC_DLD_PWR_DL_ON:
    // 6. DL pin pull up
    ret = IoctlNfc(NFC_DLD_PWR_DL_ON);
    if (!ret) {
        TMS_LOG_E(g_tag, "%s: DL pin pull up failed, NFCC FW download failed", __FUNCTION__);
        // isBlState is false
        goto exit_close_t1;
    }

chip_reset:
    // 7. nfcc chip soft or hard reset
    if (!SeIsOpened()) {
        status = openT1(ESE_MODE_NFCC_DL);
        if (ESESTATUS_SUCCESS != status) {
            TMS_LOG_E(g_tag, "%s: openT1 failed, NFCC FW DL failed", __FUNCTION__);
            // open node failed, try release resource
            goto exit_dl_pull_down;
        }
    }
    if (forceFwDownload) {
        usleep(10 * 1000);  // sleep 10 * 1000us
        ret = ChipHardReset();
    } else {
        ret = NfccSoftReset();
    }

    if (!ret) {
        if (!forceFwDownload) {
            TMS_LOG_I(g_tag, "%s: nfcc soft reset failed, try chip hard reset", __FUNCTION__);
            forceFwDownload = true;
            goto chip_reset;
        } else {
            TMS_LOG_E(g_tag, "%s: nfcc soft reset failed, NFCC FW DL failed", __FUNCTION__);
            // isBlState is false
            goto exit_dl_pull_down;
        }
    } else {
        isBlState = true;
    }

    // 8. T=1 init
    status = initT1();
    if (ESESTATUS_SUCCESS != status) {
        if (!forceFwDownload) {
            TMS_LOG_I(g_tag, "%s: initT1 failed, try FW force DL", __FUNCTION__);
            forceFwDownload = true;
            goto chip_reset;
        } else {
            TMS_LOG_E(g_tag, "%s: initT1 failed, NFCC FW DL failed", __FUNCTION__);
            goto exit_dl_pull_down;
        }
    }

    if (INVALID_FW_VER == chipInfo.nfccBlVer) {
        vtpParams = {.fileName = nullptr,
                     .needPT2SeBl = false,
                     .isNfccBlState = true,
                     .pTmsPhAbs = new TmsRee(execApduCmdChkRes),
                    };
        if (nullptr == vtpParams.pTmsPhAbs) {
            status = ESESTATUS_FAILED;
            TMS_LOG_E(g_tag, "%s: new TmsRee failed, get NFCC BL version failed", __FUNCTION__);
            goto exit_deinit_t1;
        }

        dlCmdOp = {.routineName = verThreadName,
                   .pResStatus = &chipInfo,
                   .routine = GetNfccChipInfoFromBLThread,
                   .pCond = GetGpCond(),
                   .pMutex = &mutex,
                   .pParameters = &vtpParams,
                  };
        ret = startThread(&dlCmdOp, 2);  // 2s timeout
        delete vtpParams.pTmsPhAbs;
        vtpParams.pTmsPhAbs = nullptr;
        goto VTP_FILE_BY_BL_VER;
    }

VTP_DLD:
    // 9. VTP download
    TMS_LOG_I(g_tag, "%s: NFCC FW downloading...", __FUNCTION__);
    property_set(PROP_KEY_FW_DL_STATE, "downloading");
    vtpParams = {.fileName = fileName,
                 .needPT2SeBl = false,
                 .isNfccBlState = true,
                 .pTmsPhAbs = new TmsRee(execApduCmdChkRes),
                 .pChipInfo = &chipInfo,
                };
    if (nullptr == vtpParams.pTmsPhAbs) {
        status = ESESTATUS_FAILED;
        TMS_LOG_E(g_tag, "%s: new TmsRee failed, NFCC FW DL failed", __FUNCTION__);
        goto exit_deinit_t1;
    }

    dlCmdOp = {.routineName = vtpThreadName,
               .cmd = nullptr,
               .rsp = nullptr,
               .rspChk = nullptr,
               .cmdLen = 0,
               .rspLen = 0,
               .rspChkLen = 0,
               .pResStatus = nullptr,
               .routine = VtpDownloadThread,
               .pCond = GetGpCond(),
               .pMutex = &mutex,
               .pParameters = &vtpParams
              };
    dlCmdOp.pResStatus = new ESESTATUS;
    if (dlCmdOp.pResStatus == nullptr) {
        TMS_LOG_E(g_tag, "%s: new res memory failed, NFCC FW DL failed", __FUNCTION__);
        delete vtpParams.pTmsPhAbs;
        vtpParams.pTmsPhAbs = nullptr;
        goto exit_deinit_t1;
    }
    *((ESESTATUS *)(dlCmdOp.pResStatus)) = ESESTATUS_FAILED;
    ret = startThread(&dlCmdOp, 60);  // 60s timeout
    status = *((ESESTATUS *)(dlCmdOp.pResStatus));
    delete (ESESTATUS *)dlCmdOp.pResStatus;
    dlCmdOp.pResStatus = nullptr;
    delete vtpParams.pTmsPhAbs;
    vtpParams.pTmsPhAbs = nullptr;

    if (ESESTATUS_SUCCESS == status) {
        isBlState = false; // if DL success, nfcc should already jump to FW state
#ifdef I2C_PATCH
        // FW download success, reset the g_fixedI2CFlag to true
        SetFixedI2cFlag(true);
#endif
    }

exit_deinit_t1:
    // 10. T=1 deinitialized
    deInitT1();

exit_dl_pull_down:
    // 11. DL pin pull down
    IoctlNfc(NFC_DLD_PWR_DL_OFF);
    if (ESESTATUS_SUCCESS != status || isBlState) {
        ret = ChipHardReset();
        if (!ret) {
            ret = ChipHardReset();
        }
    }

exit_close_t1:
    // 12. close /dev/tms_nfc
    CloseT1();

exit_check_result:
    // 13. set result to property
    if (ESESTATUS_SUCCESS == status) {
        UPDATE_NFC_FW_STATE(NFC_FW_UPDATE_SUCC);
        property_set(PROP_KEY_FW_DL_STATE, "success");
    } else {
        UPDATE_NFC_FW_STATE(NFC_FW_UPDATE_FAIL);
        property_set(PROP_KEY_FW_DL_STATE, "failure");
    }

    // 14. release resource and unlock
    if (vtpParams.pTmsPhAbs != NULL) {
        delete vtpParams.pTmsPhAbs;
        vtpParams.pTmsPhAbs = NULL;
    }

    if (GetGpCond() != NULL) {
        pthread_cond_destroy(GetGpCond());
        delete GetGpCond();
        SetGpCond(NULL);
    }
    pthread_mutex_destroy(&mutex);

    pthread_mutex_unlock(GetMutex());

    gettimeofday(&endTime, 0);
    SetNfccFwDlTime((int)(endTime.tv_sec - startTime.tv_sec));
    TMS_LOG_D(g_tag, "%s: exit. used time[%ds]. FW DL status[%d]",
              __FUNCTION__, GetNfccFwDlTime(), status);
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

/* DESCRIPTION
 * Check and download NFCC BL
 *
 * RETURN VALUE
 * return 0 (ESESTATUS_SUCCESS) download NFCC BL successfully or has been latest version,
 *        else return error code(eg. ESESTATUS_FAILED).
 */
int16_t NfccBlDownload()
{
    ESESTATUS status = ESESTATUS_FAILED;
    char *fileName = nullptr;
    uint32_t vtpBlVer = INVALID_FW_VER;
    ChipInfo chipInfo;
    // ioctl result
    bool ret = false;
    char blVerThreadName[] = "getBLVerThread";
    char vtpThreadName[] = VTP_DL_BL_ROUTINE;
    DownloadCmdOp dlCmdOp;
    VtpParams vtpParams;

    struct timeval startTime, endTime;
    pthread_mutex_t mutex;

    InitSELogLevel();
    TMS_LOG_D(g_tag, "%s: enter", __FUNCTION__);

    SetNfccBlDlTime(0);
    gettimeofday(&startTime, 0);
    pthread_mutex_lock(GetMutex());
    pthread_mutex_init(&mutex, NULL);

    if (GetGpCond() == NULL) {
        SetGpCond(new pthread_cond_t);
        pthread_cond_init(GetGpCond(), NULL);
    } else {
        TMS_LOG_E(g_tag, "%s: download is busy, NFCC BL DL failed", __FUNCTION__);
        goto exit_check_result;
    }

    // 1. get fw version from VTP file (fwVerVtp)
    fileName = new char[256] { 0 };
    if (nullptr == fileName) {
        TMS_LOG_E(g_tag, "%s: new fileName memory failed, NFCC BL DL failed", __FUNCTION__);
        goto exit_check_result;
    }
    if (!GetVaildFileName(fileName, 256,  // 256 length
                          NAME_TMS_NFC_BL_NAME, strlen(NAME_TMS_NFC_BL_NAME),
                          "THN31_NFCC_BL_VTP.txt", true)) {
        goto exit_check_result;
    }

    // 2. open /dev/tms_nfc
    status = openT1(ESE_MODE_NFCC_DL);
    if (ESESTATUS_SUCCESS != status) {
        TMS_LOG_E(g_tag, "%s: openT1 failed, NFCC BL DL failed", __FUNCTION__);
        goto exit_check_result;
    }

    // 3.1 DL flush I2C data
    ret = IoctlNfc(NFC_DLD_FLUSH);
    usleep(10 * 1000);  // sleep 10 * 1000us
    if (!ret) {
        TMS_LOG_E(g_tag, "DL flush I2C data failed, NfccBlDownload download failed");
        goto exit_close_t1;
    }

    // 3.2 get BL version from chip (fwVer)
    vtpParams = {.fileName = NULL,
                 .needPT2SeBl = false,
                 .isNfccBlState = false,
                 .pTmsPhAbs = new TmsRee(NULL),
                };
    dlCmdOp = {.routineName = blVerThreadName,
               .pResStatus = &chipInfo,
               .routine = GetNfccChipInfoFromBLThread,
               .pCond = GetGpCond(),
               .pMutex = &mutex,
               .pParameters = &vtpParams,
              };
    ret = startThread(&dlCmdOp, 2);  // 2s timeout
    delete vtpParams.pTmsPhAbs;
    vtpParams.pTmsPhAbs = NULL;

    // 4. assert(nfccBlVer, vtpBlVer)
    if (INVALID_FW_VER == chipInfo.nfccBlVer) {
        TMS_LOG_E(g_tag, "%s: get BL version failed, NFCC BL DL failed", __FUNCTION__);
        status = ESESTATUS_FAILED;
        goto exit_close_t1;
    }

    vtpBlVer = (uint32_t)GetCosVerFromVtp(fileName, 256, chipInfo); // 256 length
    if (INVALID_FW_VER == vtpBlVer) {
        TMS_LOG_E(g_tag, "%s: get invalid BL version, NFCC BL DL failed", __FUNCTION__);
        status = ESESTATUS_FAILED;
        goto exit_close_t1;
    }

    if (vtpBlVer == chipInfo.nfccBlVer) {
        TMS_LOG_I(g_tag, "%s: Has been latest NFCC-BL: %08x", __FUNCTION__, chipInfo.nfccBlVer);
        status = ESESTATUS_SUCCESS;
        goto exit_close_t1;
    } else if ((chipInfo.nfccBlVer < 0x00B104) || (vtpBlVer < 0x00B104) ||
               (((chipInfo.nfccBlVer & 0xFF000000U) != 0) &&
               (chipInfo.nfccBlVer != 0x03200004) /* B1.03 */)) {
        TMS_LOG_E(g_tag,
                  "%s: the old[%08x] or new[%08x] BL cannot support upgrade. NFCC BL DL failed",
                  __FUNCTION__, chipInfo.nfccBlVer, vtpBlVer);
        goto exit_close_t1;
    }

    // 5. VTP download
    TMS_LOG_I(g_tag, "%s: NFCC BL downloading...", __FUNCTION__);
    property_set(PROP_KEY_NFCC_BL_DL_STATE, "downloading");
    vtpParams = {.fileName = fileName,
                 .needPT2SeBl = false,
                 .pTmsPhAbs = new TmsRee(execNciCmdChkRes),
                 .pChipInfo = &chipInfo,
                };
    if (NULL == vtpParams.pTmsPhAbs) {
        status = ESESTATUS_FAILED;
        TMS_LOG_E(g_tag, "%s: new TmsRee failed, NFCC BL DL failed", __FUNCTION__);
        goto exit_close_t1;
    }

    dlCmdOp = {.routineName = vtpThreadName,
               .routine = VtpDownloadThread,
               .pCond = GetGpCond(),
               .pMutex = &mutex,
               .pParameters = &vtpParams
              };
    dlCmdOp.pResStatus = new ESESTATUS;
    if (dlCmdOp.pResStatus == NULL) {
        TMS_LOG_E(g_tag, "%s: new res memory failed, NFCC BL DL failed", __FUNCTION__);
        goto exit_close_t1;
    }
    *((ESESTATUS *)(dlCmdOp.pResStatus)) = ESESTATUS_FAILED;
    ret = startThread(&dlCmdOp, 60);  // 60s timeout
    status = *((ESESTATUS *)(dlCmdOp.pResStatus));
    delete (ESESTATUS *)dlCmdOp.pResStatus;
    dlCmdOp.pResStatus = NULL;

exit_close_t1:
    // 6. close /dev/tms_nfc
    CloseT1();

exit_check_result:
    // 7. set result to property
    if (ESESTATUS_SUCCESS == status) {
        property_set(PROP_KEY_NFCC_BL_DL_STATE, "success");
    } else {
        property_set(PROP_KEY_NFCC_BL_DL_STATE, "failure");
    }

    // 8. release resource
    if (fileName != nullptr) {
        delete[] fileName;
    }
    if (vtpParams.pTmsPhAbs != NULL) {
        delete vtpParams.pTmsPhAbs;
        vtpParams.pTmsPhAbs = NULL;
    }

    if (GetGpCond() != NULL) {
        pthread_cond_destroy(GetGpCond());
        delete GetGpCond();
        SetGpCond(NULL);
    }
    pthread_mutex_destroy(&mutex);

    pthread_mutex_unlock(GetMutex());

    gettimeofday(&endTime, 0);
    SetNfccBlDlTime((int)(endTime.tv_sec - startTime.tv_sec));
    TMS_LOG_D(g_tag, "%s: exit. used time[%ds]. NFCC BL DL status[%d]",
              __FUNCTION__, GetNfccBlDlTime(), status);
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


static ChipInfo TryComputeNfccChipInfo(uint32_t fwVer)
{
    ChipInfo chipInfo;
    if (fwVer & 0x00001000) {
        chipInfo.chipKeyType = CHIP_R_KEY;
    } else {
        chipInfo.chipKeyType = CHIP_T_KEY;
    }

    if ((fwVer & 0x00FF0000) == 0x00C20000) {
        chipInfo.chipType = CHIP_EC1;
    } else if ((fwVer & 0x00F00000) == 0x00D00000) {
        chipInfo.chipType = CHIP_EC2;
    } else {
        TMS_LOG_E(g_tag, "%s: invalid chip info, [%08X]", __FUNCTION__, fwVer);
        // Only used to check DL or not.
        // Set chipType to CHIP_EC2.
        chipInfo.chipType = CHIP_EC2;
        // If the chip is EC1, next DL steps will get the right chipType.
    }

    return chipInfo;
}

/*******************************************************************************
 **
 ** Function:        CheckFlashRequired()
 **
 ** Description:     Updates FW and Reg configurations if required
 **
 ** Returns:         status
 **
 ********************************************************************************/
void CheckFlashRequired(uint8_t *fwUpdateReq,
                        uint32_t *wFwVer,
                        uint32_t *wFwVerRsp)
{
    TMS_LOG_D(g_tag, "%s: enter", __FUNCTION__);
    char *fileName = nullptr;
    uint8_t wFwUpdateReq = false;
    uint32_t fwVer = 0;

    // check if need to remove override nfcc fw file
    checkAndRemoveOverrideVtpFile();

    fileName = new char[256] { 0 };
    if (!GetVaildFileName(fileName, 256,  // 256 length
                          NAME_TMS_NFC_FW_NAME, strlen(NAME_TMS_NFC_FW_NAME),
                          "THN31_FW_VTP.txt", true)
            && !GetVaildFileName(fileName, 256,  // 256 length
                                 NAME_TMS_SEC_NFC_FW_NAME, strlen(NAME_TMS_SEC_NFC_FW_NAME),
                                 "SEC_THN31_FW_VTP.txt", true)) {
        // check FW download
        TMS_LOG_I(g_tag, "cannot find FW VTP file, FW doesn't need to download");
    } else {
        ChipInfo chipInfo = TryComputeNfccChipInfo(*wFwVerRsp);
        fwVer = (uint32_t)GetCosVerFromVtp(fileName, 256, chipInfo);  // 256 length
        *wFwVer = fwVer;
        if (INVALID_FW_VER == fwVer) {
            TMS_LOG_I(g_tag, "cannot get Version from VTP file, FW doesn't need to download");
        } else {
            TMS_LOG_D(g_tag, "FW version of the binary = 0x%x", fwVer);
            TMS_LOG_D(g_tag, "FW version found on the device = 0x%x", *wFwVerRsp);
            wFwUpdateReq = (*wFwVerRsp  != (uint32_t)fwVer) ? true : false;
            wFwUpdateReq = wFwUpdateReq || (0 == EseConfig::getUnsigned(NAME_FW_DL_CHK_VER, 1));
        }
    }

    if (!GetVaildFileName(fileName, 256,  // 256 length
                          NAME_TMS_ESE_COS_NAME, strlen(NAME_TMS_ESE_COS_NAME),
                          "THN31_ESE_VTP.txt", true) &&
        !GetVaildFileName(fileName, 256,  // 256 length
                          NAME_TMS_SEC_ESE_COS_NAME, strlen(NAME_TMS_SEC_ESE_COS_NAME),
                          "SEC_THN31_ESE_VTP.txt", true)) {
        // check eSE COS download
        TMS_LOG_I(g_tag, "cannot find COS VTP file, eSE COS doesn't need to download");
    } else {
        TMS_LOG_I(g_tag, "COS VTP exist");
        wFwUpdateReq = true;
    }

    if (!GetVaildFileName(fileName, 256,  // 256 length
                          NAME_TMS_NFC_BL_NAME, strlen(NAME_TMS_NFC_BL_NAME),
                          "THN31_NFCC_BL_VTP.txt", true)) {
        TMS_LOG_I(g_tag, "cannot find BL VTP file, BL doesn't need to download");
    } else {
        TMS_LOG_I(g_tag, "BL VTP exist");
        wFwUpdateReq = true;
    }

    *fwUpdateReq = wFwUpdateReq;

    if (false == wFwUpdateReq) {
        TMS_LOG_D(g_tag, "Flash not required");
    } else {
        property_set("nfc.fw.downloadmode_force", "1");
    }

    TMS_LOG_D(g_tag, "CheckFlashRequired() : exit wFwUpdateReq=%u", *fwUpdateReq);

    if (fileName != nullptr) {
        delete[] fileName;
    }

    return;
}

