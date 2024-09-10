/*
 * Copyright 2012-2021 NXP
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/******************************************************************************
 *
 *  The original Work has been changed by Tsingteng MicroSystem.
 *
 *  Copyright (C) 2021-2022 Tsingteng MicroSystem
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  NOT A CONTRIBUTION
 ******************************************************************************/

#include <dlfcn.h>
#include <log/log.h>
#include <android-base/file.h>
#include <android-base/strings.h>
#include <Dal4Nfc_messageQueueLib.h>
#include <TmsConfig.h>
#include <TmsLog.h>
#include <TmsNciHal.h>
#include <TmsNciHal_Adaptation.h>

#include <TmsNciHal_ext.h>
#include <TmlNfc.h>
#include "TmsNciHal_IoctlOperations.h"
#include <sys/stat.h>

#include <android-base/stringprintf.h>
#include "NfccTransportFactory.h"
#include "TmsNfcThreadMutex.h"

#include <cutils/properties.h>
#include "tmsCosI2cDl.h"
#include "tmsNfccDl.h"

using android::base::StringPrintf;
using android::base::WriteStringToFile;


/*********************** Global Variables *************************************/
#define NCI_HEADER_SIZE 3
#define NCI_SE_CMD_LEN  4
#define CORE_RES_STATUS_BYTE 3
#define MAX_TMS_HAL_EXTN_BYTES 10
static const char *gpRfBlockNum[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10",
                                     "11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
                                     "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", NULL
                                    };
const char *gpRfBlockName = "TMS_RF_CONF_BLK_";
/* FW download success flag */
static uint8_t sgFwDownloadSuccess = 0;
static uint8_t gConfigAccess = false;
#ifdef TMS_NFC
    NfcHalThreadMutex ghalFnLock;
#else
    static NfcHalThreadMutex ghalFnLock;
#endif

/* NCI HAL Control structure */
TmsNciHalControl_t gTmsNciHalCtrl;
nfc_stack_callback_t *gpNfcStackCallbackBackup;
/* global variable to get FW version from NCI response or dl get version response*/
uint32_t gFwVerRsp;
/* External global variable to get FW version */
uint32_t gFwVer;
static uint8_t gWriteUnlockedStatus = NFCSTATUS_SUCCESS;
uint8_t gFwUpdateReq = false;
uint8_t gRfUpdateReq = false;
uint32_t gTimeoutTimerId = 0;
bool gNfcDebugEnabled = true;
static bool gIsHalAidlService = false;
/*  Used to send Callback Transceive data during Mifare Write.
 *  If this flag is enabled, no need to send response to Upper layer */
bool gSendRspToUpperLayer = true;

TmsNciHalSem_t gConfigData;

volatile bool_t gIsFirstHalMinOpen = true;

void *pTmsNciHalClientThread(void *arg);
/**************** local methods used in this file only ************************/
static void tmsNciHalOpenComplete(NFCSTATUS status);
static void tmsNciHalMinOpenComplete(NFCSTATUS status);
static void tmsNciHalWriteComplete(void *pContext,
                                       TmlNfcTransactInfo_t *pInfo);
static void tmsNciHalReadComplete(void *pContext,
                                      TmlNfcTransactInfo_t *pInfo);
static void tmsNciHalCloseComplete(NFCSTATUS status);
static void tmsNciHalCoreInitializedComplete(NFCSTATUS status);
static void tmsNciHalCoreInitializedFailed();
static void tmsNciHalPowerCycleComplete(NFCSTATUS status);
static void tmsNciHalKillClientThread(
    TmsNciHalControl_t *pTmsNciHalCtrl);
static void tmsNciHalHciNetworkReset(void);
static NFCSTATUS tmsNciHalDoSwpSessionReset(void);
static void tmsNciHalPrintResStatus(uint8_t *pRxData, uint16_t *pLen);
static void tmsNciHalConfigNciParser(bool enable);
static void tmsNciHalInitializeDebugEnabledFlag();
static NFCSTATUS tmsNciHalCheckRFCmdRespStatus();
static int tmsNciHalMinOpenClean(char *pNfcDevNode);
NFCSTATUS tmsNciHalEnableTmlRead();

bool *getNfcDebugEnabled(void)
{
    return &gNfcDebugEnabled;
}

TmsNciHalControl_t *getTmsNciHalCtrl(void)
{
    return &gTmsNciHalCtrl;
}

uint32_t *getFwVerRsp(void)
{
    return &gFwVerRsp;
}

uint32_t *getFwVer(void)
{
    return &gFwVer;
}

uint32_t *getTimeoutTimerId(void)
{
    return &gTimeoutTimerId;
}

nfc_stack_callback_t **getNfcStackCallbackBackup(void)
{
    return &gpNfcStackCallbackBackup;
}

volatile bool_t *getIsFirstHalMinOpen(void)
{
    return &gIsFirstHalMinOpen;
}

/******************************************************************************
 * Function         tmsNciHalInitializeDebugEnabledFlag
 *
 * Description      This function gets the value for *getNfcDebugEnabled()
 *
 * Returns          void
 *
 ******************************************************************************/
static void tmsNciHalInitializeDebugEnabledFlag() {
    unsigned long num = 0;
    char valueStr[PROP_VALUE_MAX] = {0};
    if (getTmsNumValue(NAME_NFC_DEBUG_ENABLED, &num, sizeof(num))) {
        *getNfcDebugEnabled() = (num == 0) ? false : true;

    }

    int len = propertyGet("nfc.debugEnabled", valueStr, "");
    if (len > 0) {
        // let Android property override .conf variable
        unsigned debugEnabled = 0;
        sscanf(valueStr, "%u", &debugEnabled);
        *getNfcDebugEnabled() = (debugEnabled == 0) ? false : true;
    }
    TMSLOG_NCIHAL_D("getNfcDebugEnabled() : %d", *getNfcDebugEnabled());

}

/******************************************************************************
 * Function         pTmsNciHalClientThread
 *
 * Description      This function is a thread handler which handles all TML and
 *                  NCI messages.
 *
 * Returns          void
 *
 ******************************************************************************/
void *pTmsNciHalClientThread(void *arg) {
    TmsNciHalControl_t *pTmsNciHalCtrl = (TmsNciHalControl_t *)arg;
    NciHalMessage_t msg;

    TMSLOG_NCIHAL_D("thread started");

    while (pTmsNciHalCtrl->threadRunning == 1) {
        /* Fetch next message from the NFC stack message queue */
        if (dal4NfcMsgRcv(pTmsNciHalCtrl->drvCfg.clientId, &msg, 0, 0) ==
                -1) {
            TMSLOG_NCIHAL_E("NFC client received bad message");
            continue;
        }

        if (pTmsNciHalCtrl->threadRunning == 0) {
            break;
        }

        switch (msg.msgType) {
            case PH_LIBNFC_DEFERREDCALL_MSG: {
                NciHalDeferredCall_t *pDeferCall =
                    (NciHalDeferredCall_t *)(msg.pMsgData);

                REENTRANCE_LOCK();
                pDeferCall->pCallback(pDeferCall->pParameter);
                REENTRANCE_UNLOCK();

                break;
            }

            case NCI_HAL_OPEN_CPLT_MSG: {
                REENTRANCE_LOCK();
                if ((*getTmsNciHalCtrl()).pNfcStackCallback != NULL) {
                    /* Send the event */
                    (*(*getTmsNciHalCtrl()).pNfcStackCallback)(HAL_NFC_OPEN_CPLT_EVT,
                                                        HAL_NFC_STATUS_OK);
                }
                REENTRANCE_UNLOCK();
                break;
            }

            case NCI_HAL_CLOSE_CPLT_MSG: {
                REENTRANCE_LOCK();
                if ((*getTmsNciHalCtrl()).pNfcStackCallback != NULL) {
                    /* Send the event */
                    (*(*getTmsNciHalCtrl()).pNfcStackCallback)(HAL_NFC_CLOSE_CPLT_EVT,
                                                        HAL_NFC_STATUS_OK);
                }
                tmsNciHalKillClientThread(&*getTmsNciHalCtrl());
                REENTRANCE_UNLOCK();
                break;
            }

            case NCI_HAL_POST_INIT_CPLT_MSG: {
                REENTRANCE_LOCK();
                if ((*getTmsNciHalCtrl()).pNfcStackCallback != NULL) {
                    /* Send the event */
                    (*(*getTmsNciHalCtrl()).pNfcStackCallback)(HAL_NFC_POST_INIT_CPLT_EVT,
                                                        HAL_NFC_STATUS_OK);
                }
                REENTRANCE_UNLOCK();
                break;
            }

            case NCI_HAL_PRE_DISCOVER_CPLT_MSG: {
                REENTRANCE_LOCK();
                if ((*getTmsNciHalCtrl()).pNfcStackCallback != NULL) {
                    /* Send the event */
                    (*(*getTmsNciHalCtrl()).pNfcStackCallback)(HAL_NFC_PRE_DISCOVER_CPLT_EVT,
                                                        HAL_NFC_STATUS_OK);
                }
                REENTRANCE_UNLOCK();
                break;
            }

            case NCI_HAL_HCI_NETWORK_RESET_MSG: {
                REENTRANCE_LOCK();
                if ((*getTmsNciHalCtrl()).pNfcStackCallback != NULL) {
                    /* Send the event */
                    (*(*getTmsNciHalCtrl()).pNfcStackCallback)(HAL_NFC_HCI_NETWORK_RESET,
                                                        HAL_NFC_STATUS_OK);
                }
                REENTRANCE_UNLOCK();
                break;
            }

            case NCI_HAL_ERROR_MSG: {
                REENTRANCE_LOCK();
                if ((*getTmsNciHalCtrl()).pNfcStackCallback != NULL) {
                    /* Send the event */
                    (*(*getTmsNciHalCtrl()).pNfcStackCallback)(HAL_NFC_ERROR_EVT,
                                                        HAL_NFC_STATUS_FAILED);
                }
                REENTRANCE_UNLOCK();
                break;
            }

            case NCI_HAL_RX_MSG: {
                REENTRANCE_LOCK();
                if ((*getTmsNciHalCtrl()).pNfcStackDataCallback != NULL) {
                    (*(*getTmsNciHalCtrl()).pNfcStackDataCallback)((*getTmsNciHalCtrl()).rspLen,
                            (*getTmsNciHalCtrl()).rspData);
                }
                REENTRANCE_UNLOCK();
                break;
            }
            case HAL_NFC_FW_UPDATE_STATUS_EVT: {
                REENTRANCE_LOCK();
                if ((*getTmsNciHalCtrl()).pNfcStackCallback != NULL) {
                    /* Send the event */
                    (*(*getTmsNciHalCtrl()).pNfcStackCallback)(msg.msgType,
                                                        *((uint8_t *)msg.pMsgData));
                }
                REENTRANCE_UNLOCK();
                break;
            }

            case NCI_HAL_INIT_FAILED_MSG: {
                REENTRANCE_LOCK();
                if ((*getTmsNciHalCtrl()).pNfcStackCallback != NULL) {
                    /* Send the event */
                    (*(*getTmsNciHalCtrl()).pNfcStackCallback)(NCI_HAL_INIT_FAILED_EVT,
                                                        HAL_NFC_STATUS_FAILED);
                }
                REENTRANCE_UNLOCK();
                break;
            }
        }
    }

    TMSLOG_NCIHAL_D("TmsNciHal thread stopped");

    return NULL;
}

/******************************************************************************
 * Function         tmsNciHalKillClientThread
 *
 * Description      This function safely kill the client thread and clean all
 *                  resources.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void tmsNciHalKillClientThread(
    TmsNciHalControl_t *pTmsNciHalCtrl) {
    TMSLOG_NCIHAL_D("Terminating tmsNciHal client thread...");
    if (pTmsNciHalCtrl != NULL) {
        pTmsNciHalCtrl->pNfcStackCallback = NULL;
        pTmsNciHalCtrl->pNfcStackDataCallback = NULL;
        pTmsNciHalCtrl->threadRunning = 0;
    }

    return;
}

/******************************************************************************
 * Function         tmsNciHalMinOpenClean
 *
 * Description      This function shall be called from tmsNciHalMinOpen when
 *                  any unrecoverable error has encountered which needs to mark
 *                  min open as failed, HAL status as closed & deallocate any
 *                  memory if allocated.
 *
 * Returns          This function always returns Failure
 *
 ******************************************************************************/
static int tmsNciHalMinOpenClean(char *pNfcDevNode) {
    if (pNfcDevNode != NULL) {
        free(pNfcDevNode);
        pNfcDevNode = NULL;
    }
    osalNfcTimerCleanup();
    /* Release clientThread create in MinOpen */
    tmsNciHalKillClientThread(&*getTmsNciHalCtrl());
    dal4NfcMsgRelease(gTmsNciHalCtrl.drvCfg.clientId);
    if (0 != pthread_join((*getTmsNciHalCtrl()).clientThread, (void **)NULL)) {
        TMSLOG_TML_E("Fail to kill client thread!");
    }
    /* Report error status */
    tmsNciHalCleanupMonitor();
    (*getTmsNciHalCtrl()).halStatus = HAL_STATUS_CLOSE;
    return NFCSTATUS_FAILED;
}

/******************************************************************************
 * Function         tmsNciHalMinOpen
 *
 * Description      This function initializes the least required resources to
 *                  communicate to NFCC.This is mainly used to communicate to
 *                  NFCC when NFC service is not available.
 *
 *
 * Returns          This function return NFCSTATUS_SUCCES (0) in case of success
 *                  In case of failure returns other failure value.
 *
 ******************************************************************************/
NFCSTATUS tmsNciHalMinOpen() {
    OsalNfcConfig_t osalConfig;
    TmlNfcConfig_t tmlConfig;
    char *pNfcDevNode = NULL;
    const uint16_t maxLen = 260;
    NFCSTATUS configStatus = NFCSTATUS_SUCCESS;
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    static uint8_t cmdResetNci[] = {0x20, 0x00, 0x01, 0x00};
    static uint8_t cmdInitNci2_0[] = {0x20, 0x01, 0x02, 0x00, 0x00};
    int8_t retVal = 0x00;
    int initRetryCnt = 0;
    TMSLOG_NCIHAL_D("tmsNciHalMinOpen(): enter");

    //  property_get("tms.nfc.secos.download",value, "");
    //  if(!strcmp(value,"downloading")){
    //    TMSLOG_NCIHAL_D("secos downloading return");
    //    return NFCSTATUS_FAILED;
    //  }

    if ((*getTmsNciHalCtrl()).halStatus == HAL_STATUS_MIN_OPEN) {
        TMSLOG_NCIHAL_D("tmsNciHalMinOpen(): already open");
        return NFCSTATUS_SUCCESS;
    }

    tmsNciHalInitializeDebugEnabledFlag();

    /* initialize trace level */
    tmsLogInitializeLogLevel();

    if (tmsNciHalInitMonitor() == NULL) {
        TMSLOG_NCIHAL_E("Init monitor failed");
        return NFCSTATUS_FAILED;
    }

    /*Create the timer for extns write response*/
    *getTimeoutTimerId() = osalNfcTimerCreate();

    CONCURRENCY_LOCK();
    memset(&osalConfig, 0x00, sizeof(osalConfig));
    memset(&tmlConfig, 0x00, sizeof(tmlConfig));

    /*Init binary semaphore for Spi Nfc synchronization*/
    if (0 != sem_init(&(*getTmsNciHalCtrl()).syncSpiNfc, 0, 1)) {
        TMSLOG_NCIHAL_E("sem_init() FAiled, errno = 0x%02X", errno);
        CONCURRENCY_UNLOCK();
        return tmsNciHalMinOpenClean(pNfcDevNode);
    }

    /* By default HAL status is HAL_STATUS_OPEN */
    (*getTmsNciHalCtrl()).halStatus = HAL_STATUS_OPEN;

    /*nci version NCI_VERSION_2_0 version by default for THN31 chip type*/
    (*getTmsNciHalCtrl()).nciInfo.nciVersion = NCI_VERSION_2_0;
    /* Read the nfc device node name */
    pNfcDevNode = (char *)malloc(maxLen * sizeof(char));
    if (pNfcDevNode == NULL) {
        TMSLOG_NCIHAL_D("malloc of pNfcDevNode failed ");
        CONCURRENCY_UNLOCK();
        sem_destroy(&(*getTmsNciHalCtrl()).syncSpiNfc);
        return tmsNciHalMinOpenClean(pNfcDevNode);
    } else if (!getTmsStrValue(NAME_TMS_NFC_DEV_NODE, pNfcDevNode,
                               maxLen)) {
        TMSLOG_NCIHAL_D(
            "Invalid nfc device node name keeping the default device node "
            "/dev/thn31");
        strlcpy(pNfcDevNode, "/dev/tms_nfc", (maxLen * sizeof(char)));
    }
    /* Configure hardware link */
    (*getTmsNciHalCtrl()).drvCfg.clientId = dal4NfcMsgGet(0, 0600);
    tmlConfig.pDevName = (int8_t *)pNfcDevNode;
    osalConfig.callbackThreadId = (uintptr_t)(*getTmsNciHalCtrl()).drvCfg.clientId;
    osalConfig.pLogFile = NULL;
    tmlConfig.getMsgThreadId = (uintptr_t)(*getTmsNciHalCtrl()).drvCfg.clientId;

    /* Set Default Fragment Length */
    tmlConfig.fragmentLen = NCI_CMDRESP_MAX_BUFF_SIZE_THN31;

    /* initialize TML layer */
    configStatus = tmlNfcInit(&tmlConfig);
    if (configStatus != NFCSTATUS_SUCCESS) {
        TMSLOG_NCIHAL_E("tmlNfcInit Failed");
        CONCURRENCY_UNLOCK();
        sem_destroy(&(*getTmsNciHalCtrl()).syncSpiNfc);
        return tmsNciHalMinOpenClean(pNfcDevNode);
    } else {
        if (pNfcDevNode != NULL) {
            free(pNfcDevNode);
            pNfcDevNode = NULL;
        }
    }

    /* Create the client thread */
    (*getTmsNciHalCtrl()).threadRunning = 1;
    retVal = pthread_create(&(*getTmsNciHalCtrl()).clientThread, NULL,
                             pTmsNciHalClientThread, &*getTmsNciHalCtrl());
    if (retVal != 0) {
        TMSLOG_NCIHAL_E("pthread_create failed");
        configStatus = tmlNfcShutdownCleanUp();
        CONCURRENCY_UNLOCK();
        sem_destroy(&(*getTmsNciHalCtrl()).syncSpiNfc);
        return tmsNciHalMinOpenClean(pNfcDevNode);
    }

    CONCURRENCY_UNLOCK();

    (*getTmsNciHalCtrl()).readyToShutdown = false;

    /* call read pending */
    status = tmlNfcRead(
                 (*getTmsNciHalCtrl()).rspData, NCI_MAX_DATA_LEN,
                 (pTmlNfcTransactCompletionCb_t)&tmsNciHalReadComplete, NULL);
    if (status != NFCSTATUS_PENDING) {
        TMSLOG_NCIHAL_E("TML Read status error status = %x", status);
        configStatus = tmlNfcShutdownCleanUp();
        configStatus = NFCSTATUS_FAILED;
        sem_destroy(&(*getTmsNciHalCtrl()).syncSpiNfc);
        return tmsNciHalMinOpenClean(pNfcDevNode);
    }

initRetry:
  tmsNciHalExtInit();

  status = tmsNciHalSendExtCmd(sizeof(cmdResetNci), cmdResetNci);
  if ((status != NFCSTATUS_SUCCESS) &&
      ((*getTmsNciHalCtrl()).retryCnt >= MAX_RETRY_COUNT)) {
    TMSLOG_NCIHAL_E("Force FW Download, NFCC not coming out from Standby");
    configStatus = NFCSTATUS_FAILED;
    goto force_download;
  } else if (status != NFCSTATUS_SUCCESS) {
    TMSLOG_NCIHAL_E("NCI_CORE_RESET: Failed");
    if (initRetryCnt < 3) {
      initRetryCnt++;
      goto initRetry;
    } else if(initRetryCnt < MAX_RETRY_COUNT) {
          TMSLOG_NCIHAL_E("invlaid core reset rsp received. Trying Force FW download");
          (void)tmsNciHalPowerCycle();
          goto force_download;
    } else {
      initRetryCnt = 0;
    }
    configStatus = tmlNfcShutdownCleanUp();
    configStatus = NFCSTATUS_FAILED;
    sem_destroy(&(*getTmsNciHalCtrl()).syncSpiNfc);
    return tmsNciHalMinOpenClean(pNfcDevNode);
  }

  initRetryCnt = 0;
  (*getTmsNciHalCtrl()).retryCnt = 0;

  status = tmsNciHalSendExtCmd(sizeof(cmdInitNci2_0), cmdInitNci2_0);
  if ((status != NFCSTATUS_SUCCESS) &&
      ((*getTmsNciHalCtrl()).retryCnt >= MAX_RETRY_COUNT)) {
    TMSLOG_NCIHAL_E("Force FW Download, NFCC not coming out from Standby");
    configStatus = NFCSTATUS_FAILED;
    goto force_download;
  } else if (status != NFCSTATUS_SUCCESS) {
    TMSLOG_NCIHAL_E("NCI_CORE_INIT: Failed");
    if (initRetryCnt < 3) {
      initRetryCnt++;
      goto initRetry;
    } else if (initRetryCnt < MAX_RETRY_COUNT) {
      TMSLOG_NCIHAL_E(
          "invlaid core init rsp received. Trying Force FW download");
      (void)tmsNciHalPowerCycle();
      goto force_download;
    } else {
      initRetryCnt = 0;
    }
    configStatus = tmlNfcShutdownCleanUp();
    configStatus = NFCSTATUS_FAILED;
    sem_destroy(&(*getTmsNciHalCtrl()).syncSpiNfc);
    return tmsNciHalMinOpenClean(pNfcDevNode);
  }

  CheckFlashRequired(&gFwUpdateReq, getFwVer(), getFwVerRsp());

  TMSLOG_NCIHAL_D("FW version from device = 0x%x FW file = 0x%x", (*getFwVerRsp()),(*getFwVer()));
  if (!gFwUpdateReq) {
    TMSLOG_NCIHAL_D("FW update not required");
    property_set("nfc.fw.downloadmode_force", "0");
  } else {
force_download:
  TMSLOG_NCIHAL_E("FW version for FW file = 0x%x", (*getFwVer()));
  TMSLOG_NCIHAL_E("FW version from device = 0x%x", (*getFwVerRsp()));
    if ((*getFwVerRsp()) == 0) {
      (*getNfcFL()).chipType = thn31;
      NfcChipType chipType = thn31;
      CONFIGURE_FEATURELIST(chipType);
      (*getNfcFL()).nfccFL._NFCC_DWNLD_MODE = NFCC_DWNLD_WITH_VEN_RESET;
    }

    TMSLOG_NCIHAL_D("FW update required");
    sgFwDownloadSuccess = 0;

    usleep(50 * 1000);

    /* Abort TML read operation which is always kept open */
    status =tmlNfcReadAbort();

    if (NFCSTATUS_SUCCESS != status) {
      /* TODO:-Action to take in this case:-Tml read abort failed!? */
      TMSLOG_NCIHAL_E("Tml Read Abort failed!!");
    }

    status = NfccFwDownload();
    if (NFCSTATUS_SUCCESS != status) {
      TMSLOG_NCIHAL_E("NfccFwDownload failed");
    } else {
      configStatus = NFCSTATUS_SUCCESS;
      sgFwDownloadSuccess = 1;
    }
    property_set("nfc.fw.downloadmode_force", "0");
    status = eseCosDownloadI2C();
    if (NFCSTATUS_SUCCESS != status) {
      TMSLOG_NCIHAL_E("EseCosDownloadI2C failed");
    }

    status = NfccBlDownload();
    if (NFCSTATUS_SUCCESS != status) {
      TMSLOG_NCIHAL_E("NfccBlDownload failed");
    }

    /* call read pending */
    status = tmlNfcRead(
                 (*getTmsNciHalCtrl()).rspData, NCI_MAX_DATA_LEN,
                 (pTmlNfcTransactCompletionCb_t)&tmsNciHalReadComplete, NULL);
    if (status != NFCSTATUS_PENDING) {
      TMSLOG_NCIHAL_E("TML Read status error status = %x", status);
      configStatus = tmlNfcShutdownCleanUp();
      configStatus = NFCSTATUS_FAILED;
      sem_destroy(&(*getTmsNciHalCtrl()).syncSpiNfc);
      return tmsNciHalMinOpenClean(pNfcDevNode);
    }
  }

  (*getTmsNciHalCtrl()).retryCnt = 0;

  /* Call open complete */
  tmsNciHalMinOpenComplete(configStatus);
  TMSLOG_NCIHAL_D("tmsNciHalMinOpen(): exit");
  return configStatus;

}


/******************************************************************************
 * Function         tmsNciHalOpen
 *
 * Description      This function is called by libnfc-nci during the
 *                  initialization of the NFCC. It opens the physical connection
 *                  with NFCC (THN31) and creates required client thread for
 *                  operation.
 *                  After open is complete, status is informed to libnfc-nci
 *                  through callback function.
 *
 * Returns          This function return NFCSTATUS_SUCCES (0) in case of success
 *                  In case of failure returns other failure value.
 *
 ******************************************************************************/
NFCSTATUS tmsNciHalOpen(nfc_stack_callback_t *pCallback,
                        nfc_stack_data_callback_t *pDataCallback) {
    NFCSTATUS configStatus = NFCSTATUS_SUCCESS;
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    TMSLOG_NCIHAL_E("tmsNciHalOpen NFC HAL OPEN");
    NfcHalAutoThreadMutex a(ghalFnLock);

    if ((*getTmsNciHalCtrl()).halStatus == HAL_STATUS_OPEN) {
        TMSLOG_NCIHAL_D("tmsNciHalOpen already open");

        //    property_get("tms.nfc.secos.download",value, "");
        //    if(!strcmp(value,"success")){
        //      TMSLOG_NCIHAL_D("secos download success");
        //      if (pCallback != NULL) {
        //        *getNfcStackCallbackBackup() = pCallback;
        //        (*pCallback)(HAL_NFC_OPEN_CPLT_EVT,
        //                   HAL_NFC_STATUS_OK);
        //        property_set("tms.nfc.secos.download", "null");
        //      }
        //    }
        if (gIsHalAidlService) {
            tmsNciHalOpenComplete(configStatus);
        }
        return NFCSTATUS_SUCCESS;
    } else if ((*getTmsNciHalCtrl()).halStatus == HAL_STATUS_CLOSE) {
        memset(&*getTmsNciHalCtrl(), 0x00, sizeof(*getTmsNciHalCtrl()));
        (*getTmsNciHalCtrl()).pNfcStackCallback = pCallback;
        (*getTmsNciHalCtrl()).pNfcStackDataCallback = pDataCallback;
        status = tmsNciHalMinOpen();
        if (status != NFCSTATUS_SUCCESS) {
            TMSLOG_NCIHAL_E("tmsNciHalMinOpen failed");
            goto clean_and_return;
        }    /*else its already in MIN_OPEN state. continue with rest of functionality*/
    } else {
        (*getTmsNciHalCtrl()).pNfcStackCallback = pCallback;
        (*getTmsNciHalCtrl()).pNfcStackDataCallback = pDataCallback;
    }
    /* Call open complete */
    tmsNciHalOpenComplete(configStatus);

    return configStatus;

clean_and_return:
    CONCURRENCY_UNLOCK();
    /* Report error status */
    if (pCallback != NULL) {
        (*pCallback)(HAL_NFC_OPEN_CPLT_EVT,
                   HAL_NFC_STATUS_FAILED);
    }

    (*getTmsNciHalCtrl()).pNfcStackCallback = NULL;
    (*getTmsNciHalCtrl()).pNfcStackDataCallback = NULL;
    tmsNciHalCleanupMonitor();
    (*getTmsNciHalCtrl()).halStatus = HAL_STATUS_CLOSE;
    return NFCSTATUS_FAILED;
}

/******************************************************************************
 * Function         tmsNciHalMinOpenComplete
 *
 * Description      This function updates the status of tmsNciHalMinOpenComplete
 *                  to halstatus.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void tmsNciHalMinOpenComplete(NFCSTATUS status) {
    *getIsFirstHalMinOpen() = false;
    if (status == NFCSTATUS_SUCCESS) {
        (*getTmsNciHalCtrl()).halStatus = HAL_STATUS_MIN_OPEN;
    }

    return;
}

/******************************************************************************
 * Function         tmsNciHalOpenComplete
 *
 * Description      This function inform the status of tmsNciHalOpen
 *                  function to libnfc-nci.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void tmsNciHalOpenComplete(NFCSTATUS status) {
    static NciHalMessage_t sMsg;

    if (status == NFCSTATUS_SUCCESS) {
        sMsg.msgType = NCI_HAL_OPEN_CPLT_MSG;
        (*getTmsNciHalCtrl()).halOpenStatus = true;
        (*getTmsNciHalCtrl()).halStatus = HAL_STATUS_OPEN;
    } else {
        sMsg.msgType = NCI_HAL_ERROR_MSG;
    }

    sMsg.pMsgData = NULL;
    sMsg.size = 0;

    tmlNfcDeferredCall((*getTmlNfcContext())->callbackThreadId,
                          (NciHalMessage_t *)&sMsg);

    return;
}

/******************************************************************************
 * Function         tmsNciHalWrite
 *
 * Description      This function write the data to NFCC through physical
 *                  interface (e.g. I2C) using the THN31 driver interface.
 *                  Before sending the data to NFCC, tmsNciHalWriteExt
 *                  is called to check if there is any extension processing
 *                  is required for the NCI packet being sent out.
 *
 * Returns          It returns number of bytes successfully written to NFCC.
 *
 ******************************************************************************/
NFCSTATUS tmsNciHalWrite(uint16_t dataLen, const uint8_t *pData) {
    return tmsNciHalWriteInternal(dataLen, pData);
}

/******************************************************************************
 * Function         tmsNciHalWriteInternal
 *
 * Description      This function write the data to NFCC through physical
 *                  interface (e.g. I2C) using the THN31 driver interface.
 *                  Before sending the data to NFCC, tmsNciHalWriteExt
 *                  is called to check if there is any extension processing
 *                  is required for the NCI packet being sent out.
 *
 * Returns          It returns number of bytes successfully written to NFCC.
 *
 ******************************************************************************/
int tmsNciHalWriteInternal(uint16_t dataLen, const uint8_t *pData) {
    NFCSTATUS status = NFCSTATUS_FAILED;
    static NciHalMessage_t sMsg;
    if ((*getTmsNciHalCtrl()).halStatus != HAL_STATUS_OPEN) {
        return NFCSTATUS_FAILED;
    }
    /* Create local copy of cmd_data */
    memcpy((*getTmsNciHalCtrl()).cmdData, pData, dataLen);
    (*getTmsNciHalCtrl()).cmdLen = dataLen;
    if (((*getTmsNciHalCtrl()).cmdLen + MAX_TMS_HAL_EXTN_BYTES) > NCI_MAX_DATA_LEN) {
        TMSLOG_NCIHAL_D("pCmdLen exceeds limit NCI_MAX_DATA_LEN");
        goto clean_and_return;
    }
    /* Check for TMS ext before sending write */
    status =
        tmsNciHalWriteExt(&(*getTmsNciHalCtrl()).cmdLen, (*getTmsNciHalCtrl()).cmdData,
                              &(*getTmsNciHalCtrl()).rspLen, (*getTmsNciHalCtrl()).rspData);
    if (status != NFCSTATUS_SUCCESS) {
        /* Do not send packet to THN31, send response directly */
        sMsg.msgType = NCI_HAL_RX_MSG;
        sMsg.pMsgData = NULL;
        sMsg.size = 0;

        tmlNfcDeferredCall((*getTmlNfcContext())->callbackThreadId,
                              (NciHalMessage_t *)&sMsg);
        goto clean_and_return;
    }

    CONCURRENCY_LOCK();
    dataLen = tmsNciHalWriteUnlocked((*getTmsNciHalCtrl()).cmdLen,
                                          (*getTmsNciHalCtrl()).cmdData, ORIG_LIBNFC);
    CONCURRENCY_UNLOCK();

clean_and_return:
    /* No data written */
    return dataLen;
}

/******************************************************************************
 * Function         tmsNciHalWriteUnlocked
 *
 * Description      This is the actual function which is being called by
 *                  tmsNciHalWrite. This function writes the data to NFCC.
 *                  It waits till write callback provide the result of write
 *                  process.
 *
 * Returns          It returns number of bytes successfully written to NFCC.
 *
 ******************************************************************************/
NFCSTATUS tmsNciHalWriteUnlocked(uint16_t dataLen, const uint8_t *pData,
                                 int origin) {
    NFCSTATUS status = NFCSTATUS_INVALID_PARAMETER;
    TmsNciHalSem_t cbData;
    (*getTmsNciHalCtrl()).retryCnt = 0;
    int semVal = 0;
    static uint8_t resetNtf[] = {0x60, 0x00, 0x06, 0xA0, 0x00,
                                  0xC7, 0xD4, 0x00, 0x00
                                 };
    /* Create the local semaphore */
    if (tmsNciHalInitCbData(&cbData, NULL) != NFCSTATUS_SUCCESS) {
        TMSLOG_NCIHAL_D("tmsNciHalWriteUnlocked Create cb data failed");
        dataLen = 0;
        goto clean_and_return;
    }

    /* Create local copy of cmd_data */
    memcpy((*getTmsNciHalCtrl()).cmdData, pData, dataLen);
    (*getTmsNciHalCtrl()).cmdLen = dataLen;
    gWriteUnlockedStatus = NFCSTATUS_FAILED;
    /* check for write synchronyztion */
    if (tmsNciHalCheckNciCmdWriteWindow((*getTmsNciHalCtrl()).cmdLen,
            (*getTmsNciHalCtrl()).cmdData) != NFCSTATUS_SUCCESS) {
        TMSLOG_NCIHAL_D("tmsNciHalWriteUnlocked  CMD window  check failed");
        dataLen = 0;
        goto clean_and_return;
    }

    if (origin == ORIG_TMSHAL) {
        HAL_ENABLE_EXT();
    }

retry:

    dataLen = (*getTmsNciHalCtrl()).cmdLen;

    status = tmlNfcWrite(
                 (uint8_t *)(*getTmsNciHalCtrl()).cmdData, (uint16_t)(*getTmsNciHalCtrl()).cmdLen,
                 (pTmlNfcTransactCompletionCb_t)&tmsNciHalWriteComplete,
                 (void *)&cbData);
    if (status != NFCSTATUS_PENDING) {
        TMSLOG_NCIHAL_E("writeUnlocked status error");
        dataLen = 0;
        goto clean_and_return;
    }

    /* Wait for callback response */
    if (SEM_WAIT(cbData)) {
        TMSLOG_NCIHAL_E("writeUnlocked semaphore error");
        dataLen = 0;
        goto clean_and_return;
    }

    if (cbData.status != NFCSTATUS_SUCCESS) {
        dataLen = 0;
        if ((*getTmsNciHalCtrl()).retryCnt++ < MAX_RETRY_COUNT) {
            TMSLOG_NCIHAL_D(
                "writeUnlocked failed - THN31 Maybe in Standby Mode - Retry");
#ifdef TMS_NFC
            /* 30ms delay for waiting NFCC exiting LP PMU loop */
            usleep(1000 * 30);
#else
            /* 10ms delay to give NFCC wake up delay */
            usleep(1000 * 10);
#endif
            goto retry;
        } else {
            TMSLOG_NCIHAL_E(
                "writeUnlocked failed - THN31 Maybe in Standby Mode (max count = "
                "0x%x)",
                (*getTmsNciHalCtrl()).retryCnt);

            status = tmlNfcIoCtl(TMLNFC_RESET_DEVICE);

            if (NFCSTATUS_SUCCESS == status) {
                TMSLOG_NCIHAL_D("THN31 Reset - SUCCESS\n");
            } else {
                TMSLOG_NCIHAL_D("THN31 Reset - FAILED\n");
            }
            if ((*getTmsNciHalCtrl()).pNfcStackDataCallback != NULL &&
                    (*getTmsNciHalCtrl()).halOpenStatus == true) {
                if ((*getTmsNciHalCtrl()).pRxData != NULL) {
                    TMSLOG_NCIHAL_D(
                        "Send the Core Reset NTF to upper layer, which will trigger the "
                        "recovery\n");
                    // Send the Core Reset NTF to upper layer, which will trigger the
                    // recovery.
                    abort();
                    (*getTmsNciHalCtrl()).rxDataLen = sizeof(resetNtf);
                    memcpy((*getTmsNciHalCtrl()).pRxData, resetNtf, sizeof(resetNtf));
                    (*(*getTmsNciHalCtrl()).pNfcStackDataCallback)((*getTmsNciHalCtrl()).rxDataLen,
                            (*getTmsNciHalCtrl()).pRxData);
                } else {
                    (*(*getTmsNciHalCtrl()).pNfcStackDataCallback)(0x00, NULL);
                }
                gWriteUnlockedStatus = NFCSTATUS_FAILED;
            }
        }
    } else {
        gWriteUnlockedStatus = NFCSTATUS_SUCCESS;
    }

clean_and_return:
    if (gWriteUnlockedStatus == NFCSTATUS_FAILED) {
        sem_getvalue(&((*getTmsNciHalCtrl()).syncSpiNfc), &semVal);
        if ((((*getTmsNciHalCtrl()).cmdData[0] & NCI_MT_MASK) == NCI_MT_CMD)  && semVal == 0) {
            sem_post(&((*getTmsNciHalCtrl()).syncSpiNfc));
            TMSLOG_NCIHAL_D(
                "HAL write  failed CMD window check releasing \n");
        }
    }
    tmsNciHalCleanupCbData(&cbData);
    return dataLen;
}

/******************************************************************************
 * Function         tmsNciHalWriteComplete
 *
 * Description      This function handles write callback.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void tmsNciHalWriteComplete(void *pContext,
                                       TmlNfcTransactInfo_t *pInfo) {
    TmsNciHalSem_t *pCbData = (TmsNciHalSem_t *)pContext;
    if (pInfo->status == NFCSTATUS_SUCCESS) {
        TMSLOG_NCIHAL_D("write successful status = 0x%x", pInfo->status);
    } else {
        TMSLOG_NCIHAL_D("write error status = 0x%x", pInfo->status);
    }

    pCbData->status = pInfo->status;

    SEM_POST(pCbData);

    return;
}

/******************************************************************************
 * Function         tmsNciHalReadComplete
 *
 * Description      This function is called whenever there is an NCI packet
 *                  received from NFCC. It could be RSP or NTF packet. This
 *                  function provide the received NCI packet to libnfc-nci
 *                  using data callback of libnfc-nci.
 *                  There is a pending read called from each
 *                  tmsNciHalReadComplete so each a packet received from
 *                  NFCC can be provide to libnfc-nci.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void tmsNciHalReadComplete(void *pContext,
                                      TmlNfcTransactInfo_t *pInfo) {
    NFCSTATUS status = NFCSTATUS_FAILED;
    int semVal;
    bool isHalClose = ((*getTmsNciHalCtrl()).halStatus == HAL_STATUS_CLOSE)
        || (*getTmsNciHalCtrl()).readyToShutdown;
    UNUSED_PROP(pContext);
    if ((*getTmsNciHalCtrl()).readRetryCnt == 1) {
        (*getTmsNciHalCtrl()).readRetryCnt = 0;
    }
    if (pInfo->status == NFCSTATUS_SUCCESS) {
        TMSLOG_NCIHAL_D("read successful status = 0x%x", pInfo->status);

        /*Check the Omapi command response and store in dedicated pBuffer to solve sync issue*/
        if ((*getNfcFL()).chipType <= thn31 && pInfo->pBuff[0] == 0x4F && pInfo->pBuff[1] == 0x01 &&
                pInfo->pBuff[2] == 0x01) {
            (*getTmsNciHalCtrl()).pRxEseData = pInfo->pBuff;
            (*getTmsNciHalCtrl()).rxEseDataLen = pInfo->length;
            SEM_POST(&((*getTmsNciHalCtrl()).extCbData));
        } else {
            (*getTmsNciHalCtrl()).pRxData = pInfo->pBuff;
            (*getTmsNciHalCtrl()).rxDataLen = pInfo->length;
            status = tmsNciHalProcessExtRsp((*getTmsNciHalCtrl()).pRxData,
                                                 &(*getTmsNciHalCtrl()).rxDataLen);
        }
        tmsNciHalPrintResStatus(pInfo->pBuff,
                                     &pInfo->length);

        /* Check if response should go to hal module only */
        if ((*getTmsNciHalCtrl()).halExtEnabled == TRUE &&
                ((*getTmsNciHalCtrl()).pRxData[0x00] & NCI_MT_MASK) == NCI_MT_RSP) {
            if (status == NFCSTATUS_FAILED) {
                TMSLOG_NCIHAL_D("enter into NFCC init recovery");
                (*getTmsNciHalCtrl()).extCbData.status = status;
            }
            /* Unlock semaphore only for responses*/
            if (((*getTmsNciHalCtrl()).pRxData[0x00] & NCI_MT_MASK) == NCI_MT_RSP) {
                /* Unlock semaphore */
                SEM_POST(&((*getTmsNciHalCtrl()).extCbData));
            }
        }  // Notification Checking
        else if (((*getTmsNciHalCtrl()).halExtEnabled == TRUE) &&
                 (((*getTmsNciHalCtrl()).pRxData[0x00] & NCI_MT_MASK) == NCI_MT_NTF) &&
                 (((*getTmsNciHalCtrl()).cmdData[0x00] & NCI_GID_MASK) ==
                  ((*getTmsNciHalCtrl()).pRxData[0x00] & NCI_GID_MASK)) &&
                 (((*getTmsNciHalCtrl()).cmdData[0x01] & NCI_OID_MASK) ==
                  ((*getTmsNciHalCtrl()).pRxData[0x01] & NCI_OID_MASK)) &&
                 ((*getTmsNciHalCtrl()).nciInfo.waitForNtf == TRUE)) {
            /* Unlock semaphore waiting for only  ntf*/
            (*getTmsNciHalCtrl()).nciInfo.waitForNtf = FALSE;
            SEM_POST(&((*getTmsNciHalCtrl()).extCbData));
        }
        /* Read successful send the event to higher layer */
        else if (((*getTmsNciHalCtrl()).pNfcStackDataCallback != NULL) &&
                 (status == NFCSTATUS_SUCCESS)) {
            (*(*getTmsNciHalCtrl()).pNfcStackDataCallback)((*getTmsNciHalCtrl()).rxDataLen,
                    (*getTmsNciHalCtrl()).pRxData);
        }
        /* Unblock next Write Command Window */
        sem_getvalue(&((*getTmsNciHalCtrl()).syncSpiNfc), &semVal);
        if (((pInfo->pBuff[0] & NCI_MT_MASK) == NCI_MT_RSP)  && semVal == 0) {
            sem_post(&((*getTmsNciHalCtrl()).syncSpiNfc));
        }
    } else {
        TMSLOG_NCIHAL_E("read error status = 0x%x", pInfo->status);
    }

    if (isHalClose && ((*getTmsNciHalCtrl()).cmdData[0x00] & NCI_GID_MASK) ==
            ((*getTmsNciHalCtrl()).pRxData[0x00] & NCI_GID_MASK) &&
            ((*getTmsNciHalCtrl()).cmdData[0x01] & NCI_OID_MASK) ==
            ((*getTmsNciHalCtrl()).pRxData[0x01] & NCI_OID_MASK) &&
            (*getTmsNciHalCtrl()).nciInfo.waitForNtf == FALSE) {
        TMSLOG_NCIHAL_D(" Ignoring read , HAL close triggered");
        return;
    }

    status = tmlNfcRead(
                 (*getTmsNciHalCtrl()).rspData, NCI_MAX_DATA_LEN,
                 (pTmlNfcTransactCompletionCb_t)&tmsNciHalReadComplete, NULL);
    if (status != NFCSTATUS_PENDING) {
        TMSLOG_NCIHAL_E("read status error status = %x", status);
        /* TODO: Not sure how to handle this ? */
    }

    return;
}

/*******************************************************************************
 **
 ** Function:        tmsNciHalLastResetNtfReason()
 **
 ** Description:     Returns and clears last reset notification reason.
 **                      Intended to be called only once during recovery.
 **
 ** Returns:         reasonCode
 **
 ********************************************************************************/
uint8_t tmsNciHalLastResetNtfReason(void) {
    uint8_t reasonCode = (*getTmsNciHalCtrl()).nciInfo.lastResetNtfReason;

    (*getTmsNciHalCtrl()).nciInfo.lastResetNtfReason = 0;

    return reasonCode;
}

/******************************************************************************
 * Function         getSystemPropertySeType
 *
 * Description      This will read NFCEE status from system properties
 *                  and returns status.
 *
 * Returns          NFCEE enabled(0x01)/disabled(0x00)
 *
 ******************************************************************************/
static int8_t getSystemPropertySeType(uint8_t seType) {
    int8_t retVal = -1;
    char valueStr[PROP_VALUE_MAX] = {0};
    if (seType >= NUM_SE_TYPES) {
        return retVal;
    }
    int len = 0;
    switch (seType) {
        case SE_TYPE_ESE:
            len = propertyGet("nfc.product.support.ese", valueStr, "");
            break;
        case SE_TYPE_UICC:
            len = propertyGet("nfc.product.support.uicc", valueStr, "");
            break;
        case SE_TYPE_UICC2:
            len = propertyGet("nfc.product.support.uicc2", valueStr, "");
            break;
    }
    if (strlen(valueStr) == 0 || len <= 0) {
        return retVal;
    }
    retVal = atoi(valueStr);
    return retVal;
}

/******************************************************************************
 * Function         tmsNciHalReadAndUpdateSeState
 *
 * Description      This will read NFCEE status from system properties
 *                  and update to NFCC to enable/disable.
 *
 * Returns          none
 *
 ******************************************************************************/
void tmsNciHalReadAndUpdateSeState() {
    NFCSTATUS status = NFCSTATUS_FAILED;
    int16_t i = 0;
    int8_t  val = -1;
    int16_t numSe = 0;
    uint8_t retryCnt = 0;
    int8_t values[NUM_SE_TYPES];

    for (i = 0; i < NUM_SE_TYPES; i++) {
        val = getSystemPropertySeType(i);
        switch (i) {
            case SE_TYPE_ESE:
                TMSLOG_NCIHAL_D("Get property : SUPPORT_ESE %d", val);
                values[SE_TYPE_ESE] = val;
                if (val > -1) {
                    numSe++;
                }
                break;
            case SE_TYPE_UICC:
                TMSLOG_NCIHAL_D("Get property : SUPPORT_UICC %d", val);
                values[SE_TYPE_UICC] = val;
                if (val > -1) {
                    numSe++;
                }
                break;
            case SE_TYPE_UICC2:
                values[SE_TYPE_UICC2] = val;
                if (val > -1) {
                    numSe++;
                }
                TMSLOG_NCIHAL_D("Get property : SUPPORT_UICC2 %d", val);
                break;
        }
    }
    if (numSe < 1) {
        return;
    }
    uint8_t setCfgCmd[NCI_HEADER_SIZE + 1 + (numSe * NCI_SE_CMD_LEN)]; // 1 for Number of Argument
    uint8_t *pIndex = &setCfgCmd[0];
    *pIndex++ = NCI_MT_CMD;
    *pIndex++ = TMS_CORE_SET_CONFIG_CMD;
    *pIndex++ = (numSe * NCI_SE_CMD_LEN) + 1;
    *pIndex++ = numSe;
    for (i = 0; i < NUM_SE_TYPES; i++) {
        switch (i) {
            case SE_TYPE_ESE:
                if (values[SE_TYPE_ESE] > -1) {
                    *pIndex++ = 0xA0;
                    *pIndex++ = 0xED;
                    *pIndex++ = 0x01;
                    *pIndex++ = values[SE_TYPE_ESE];
                }
                break;
            case SE_TYPE_UICC:
                if (values[SE_TYPE_UICC] > -1) {
                    *pIndex++ = 0xA0;
                    *pIndex++ = 0xEC;
                    *pIndex++ = 0x01;
                    *pIndex++ = values[SE_TYPE_UICC];
                }
                break;
            case SE_TYPE_UICC2:
                if (values[SE_TYPE_UICC2] > -1) {
                    *pIndex++ = 0xA0;
                    *pIndex++ = 0xD4;
                    *pIndex++ = 0x01;
                    *pIndex++ = values[SE_TYPE_UICC2];
                }
                break;
        }
    }

    while (status != NFCSTATUS_SUCCESS && retryCnt < 3) {
        status = tmsNciHalSendExtCmd(sizeof(setCfgCmd), setCfgCmd);
        retryCnt++;
        TMSLOG_NCIHAL_E("Get Cfg Retry cnt=%x", retryCnt);
    }
}

/******************************************************************************
 * Function         tmsNciHalEnableTmlRead
 *
 * Description      Invokes TmlNfc Read to make sure always read thread is
 *                  pending
 *
 * Returns          Returns read status
 *
 ******************************************************************************/
NFCSTATUS tmsNciHalEnableTmlRead() {
  /* Read again because read must be pending always.*/
  NFCSTATUS status = tmlNfcRead(
    (*getTmsNciHalCtrl()).rspData, NCI_MAX_DATA_LEN,
    (pTmlNfcTransactCompletionCb_t)&tmsNciHalReadComplete, NULL);
  if (status != NFCSTATUS_PENDING) {
    TMSLOG_NCIHAL_E("read status error status = %x", status);
  }
  return status;
}

/******************************************************************************
 * Function         tmsNciHalCoreInitialized
 *
 * Description      This function is called by libnfc-nci after successful open
 *                  of NFCC. All proprietary setting for THN31 are done here.
 *                  After completion of proprietary settings notification is
 *                  provided to libnfc-nci through callback function.
 *
 * Returns          Always returns NFCSTATUS_SUCCESS (0).
 *
 ******************************************************************************/
int tmsNciHalCoreInitialized(uint16_t coreInitRspParamsLen, uint8_t *pCoreInitRspParams) {
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    uint8_t *pBuffer = NULL;
    int isFound = 0;
    uint8_t fwDwnldFlag = false;
    uint8_t setConfigAlways = false;
    long buffLen = 260;
    long retLen = 0;
    unsigned long num = 0;
    /*NCI_INIT_CMD*/
    static uint8_t cmdInitNci[] = {0x20, 0x01, 0x00};
    /*NCI_RESET_CMD*/
    static uint8_t cmdResetNci[] = {0x20, 0x00, 0x01,0x00};  // keep configuration
    static uint8_t cmdInitNci2_0[] = {0x20, 0x01, 0x02, 0x00, 0x00};
#ifdef TMS_NFC
    /*Ndef Nfcee Config CMD*/
    static uint8_t cmdNdefNfceeConfig[] = {0x20, 0x02, 0x05, 0x01, 0xA0, 0x95, 0x01, 0x00};
#endif
    /* reset config cache */
    uint8_t retryCoreInitCnt = 0;
    if ((*getTmsNciHalCtrl()).halStatus != HAL_STATUS_OPEN) {
        return NFCSTATUS_FAILED;
    }
    if (coreInitRspParamsLen >= 1 &&
            (*pCoreInitRspParams > 0) &&
            (*pCoreInitRspParams < 4)) { // initializing for recovery.
retryCoreInit:
        gConfigAccess = false;
        if (pBuffer != NULL) {
            free(pBuffer);
            pBuffer = NULL;
        }
        if (retryCoreInitCnt > 3) {
            TMSLOG_NCIHAL_E("retryCoreInitCnt > 3, tmsNciHalCoreInitialized failed!");
            tmsNciHalCoreInitializedFailed();
            return NFCSTATUS_FAILED;
        }

        status = tmsNciHalSendExtCmd(sizeof(cmdResetNci), cmdResetNci);
        if ((status != NFCSTATUS_SUCCESS) &&
                ((*getTmsNciHalCtrl()).retryCnt >= MAX_RETRY_COUNT)) {
            TMSLOG_NCIHAL_E("Force FW Download, NFCC not coming out from Standby");
            retryCoreInitCnt++;
            goto retryCoreInit;
        } else if (status != NFCSTATUS_SUCCESS) {
            TMSLOG_NCIHAL_E("NCI_CORE_RESET: Failed");
            retryCoreInitCnt++;
            goto retryCoreInit;
        }

        if ((*getTmsNciHalCtrl()).nciInfo.nciVersion == NCI_VERSION_2_0) {
            status = tmsNciHalSendExtCmd(sizeof(cmdInitNci2_0), cmdInitNci2_0);
        } else {
            status = tmsNciHalSendExtCmd(sizeof(cmdInitNci), cmdInitNci);
        }
        if (status != NFCSTATUS_SUCCESS) {
            TMSLOG_NCIHAL_E("NCI_CORE_INIT : Failed");
            retryCoreInitCnt++;
            goto retryCoreInit;
        }
    }

#ifdef TMS_NFC
    if (((*getFwVerRsp())  & 0xFF0000) == 0xD10000) { // thn31 EC2(FW:D1.XX.XX) need different RF config
        setTmsRfConfigPath("/vendor/etc/libnfc-tms_RF_EC2.conf");
    }
    if (((*getFwVerRsp())  & 0xFF0000) == 0xD20000) { // thn31 GB1(FW:D2.XX.XX) need different RF config
        setTmsRfConfigPath("/vendor/etc/libnfc-tms_RF_GB1.conf");
    }
#endif

    pBuffer = (uint8_t *)malloc(buffLen * sizeof(uint8_t));
    if (NULL == pBuffer) {
        return NFCSTATUS_FAILED;
    }
    gConfigAccess = true;
    retLen = 0;
    isFound = getTmsByteArrayValue(NAME_TMS_ACT_PROP_EXTN, (char *)pBuffer, buffLen,
                                   &retLen);
    if (isFound > 0 && retLen > 0) {
        /* TMS ACT Proprietary Ext */
        status = tmsNciHalSendExtCmd(retLen, pBuffer);
        if (status != NFCSTATUS_SUCCESS) {
            TMSLOG_NCIHAL_E("TMS ACT Proprietary Ext failed");
            retryCoreInitCnt++;
            goto retryCoreInit;
        }
    }

    fwDwnldFlag |= (bool)sgFwDownloadSuccess;
    if (fwDwnldFlag == true) {
      tmsNciHalHciNetworkReset();
    }

#ifdef TMS_NFC
    uint8_t cmdTmsNfcForumDisable[] = {0x20, 0x02, 0x05, 0x01, 0xA0, 0x44, 0x01, 0x01};
    status = tmsNciHalSendExtCmd(8, cmdTmsNfcForumDisable);
    if (status != NFCSTATUS_SUCCESS) {
        TMSLOG_NCIHAL_E("TMS NFC Forum Disable Proprietary set failed");
        retryCoreInitCnt++;
        goto retryCoreInit;
    }
#endif

    gConfigAccess = true;
    setConfigAlways = false;
    isFound = getTmsNumValue(NAME_TMS_SET_CONFIG_ALWAYS, &num, sizeof(num));
    if (isFound > 0) {
        setConfigAlways = num;
    }
    TMSLOG_NCIHAL_D("EEPROM_fw_dwnld_flag : 0x%02x SetConfigAlways flag : 0x%02x",
                    fwDwnldFlag, setConfigAlways);

    if ((true == fwDwnldFlag) || (true == setConfigAlways) ||
            isTmsConfigModified() || (gRfUpdateReq == true)) {
#ifdef TMS_NFC
        TMSLOG_NCIHAL_D("Performing ndef nfcee config Settings");
        isFound = getTmsNumValue(NAME_TMS_T4T_NFCEE_ENABLE, &num, sizeof(num));
        if (isFound > 0) {
            cmdNdefNfceeConfig[7] = num == 0x01? 0x01: 0x00; // 0x01/0x00 is enable/disable ndef nfcee
        }
        status = tmsNciHalSendExtCmd(sizeof(cmdNdefNfceeConfig), cmdNdefNfceeConfig);
        if (status != NFCSTATUS_SUCCESS) {
            TMSLOG_NCIHAL_E("TMS NFC Forum Config Ndef Nfcee failed");
            retryCoreInitCnt++;
            goto retryCoreInit;
        }
#endif
        retLen = 0;
        TMSLOG_NCIHAL_D("Performing NAME_TMS_CORE_CONF_EXTN Settings");
        isFound = getTmsByteArrayValue(NAME_TMS_CORE_CONF_EXTN, (char *)pBuffer,
                                       buffLen, &retLen);
        if (isFound > 0 && retLen > 0) {
            /* TMS ACT Proprietary Ext */
            status = tmsNciHalSendExtCmd(retLen, pBuffer);
            if (status != NFCSTATUS_SUCCESS) {
                TMSLOG_NCIHAL_E("Set Config failed(TMS_CORE_CONF_EXTN)");
                retryCoreInitCnt++;
                goto retryCoreInit;
            }
        }

        TMSLOG_NCIHAL_D("Performing SE Settings");
        tmsNciHalReadAndUpdateSeState();

        TMSLOG_NCIHAL_D("Performing NAME_TMS_CORE_CONF Settings");
        retLen = 0;
        isFound = getTmsByteArrayValue(NAME_TMS_CORE_CONF, (char *)pBuffer, buffLen,
                                       &retLen);
        if (isFound > 0 && retLen > 0) {
            /* TMS ACT Proprietary Ext */
            status = tmsNciHalSendExtCmd(retLen, pBuffer);
            if (status != NFCSTATUS_SUCCESS) {
                TMSLOG_NCIHAL_E("Set Config failed(TMS_CORE_CONF)");
                retryCoreInitCnt++;
                goto retryCoreInit;
            }
        }
    }
    gConfigAccess = false;
    if ((true == fwDwnldFlag) || (true == setConfigAlways) ||
            isTmsRFConfigModified()) {
        unsigned long loopCnt = 0;

        do {
            char rfConfBlock[22] = {'\0'};
            strlcpy(rfConfBlock, gpRfBlockName, sizeof(rfConfBlock));
            retLen = 0;
            strlcat(rfConfBlock, gpRfBlockNum[loopCnt++],
                    sizeof(rfConfBlock));
            isFound = getTmsByteArrayValue(rfConfBlock, (char *)pBuffer, buffLen,
                                           &retLen);
            if (isFound > 0 && retLen > 0) {
                TMSLOG_NCIHAL_D(" Performing RF Settings BLK %ld", loopCnt);
                status = tmsNciHalSendExtCmd(retLen, pBuffer);

                if (status == NFCSTATUS_SUCCESS) {
                    status = tmsNciHalCheckRFCmdRespStatus();
                    /*STATUS INVALID PARAM 0x09*/
                    if (status == 0x09) {
//                        tmsNciHalRFConfigCmdRecSequence();
                        retryCoreInitCnt++;
                        goto retryCoreInit;
                    }
                } else if (status != NFCSTATUS_SUCCESS) {
                    TMSLOG_NCIHAL_E("Set Config failed(TMS_RF_CONF_BLK_%ld)", loopCnt);
                    retryCoreInitCnt++;
                    goto retryCoreInit;
                }
            }
        } while (gpRfBlockNum[loopCnt] != NULL);
        loopCnt = 0;
    }

    retLen = 0;
    gConfigAccess = false;

    tmsNciHalConfigNciParser(true);
    TMSLOG_NCIHAL_D("NCI Parser is enabled");

    gConfigAccess = false;
    {
        if (isTmsRFConfigModified() || isTmsConfigModified() || fwDwnldFlag ||
                setConfigAlways) {
            if ((*getNfcFL()).chipType >= thn31) {
                status = tmsNciHalExtSendSramConfigToFlash();
                if (status != NFCSTATUS_SUCCESS) {
                    TMSLOG_NCIHAL_E("Updation of the SRAM contents failed");
                }
            }
            status = tmsNciHalSendExtCmd(sizeof(cmdResetNci), cmdResetNci);
            if (status == NFCSTATUS_SUCCESS) {
                if ((*getTmsNciHalCtrl()).nciInfo.nciVersion == NCI_VERSION_2_0) {
                    status = tmsNciHalSendExtCmd(sizeof(cmdInitNci2_0),
                                                      cmdInitNci2_0);
                } else {
                    status = tmsNciHalSendExtCmd(sizeof(cmdInitNci), cmdInitNci);
                }
            }
        }
    }
    retryCoreInitCnt = 0;

    if (pBuffer) {
        free(pBuffer);
        pBuffer = NULL;
    }

    tmsNciHalCoreInitializedComplete(status);
    if (isTmsConfigModified()) {
        updateTmsConfigTimestamp();
    }
    if (isTmsRFConfigModified()) {
        updateTmsRfConfigTimestamp();
    }
    return NFCSTATUS_SUCCESS;
}
/******************************************************************************
 * Function         tmsNciHalCheckRFCmdRespStatus
 *
 * Description      This function is called to check the resp status of
 *                  RF update commands.
 *
 * Returns          NFCSTATUS_SUCCESS           if successful,
 *                  NFCSTATUS_INVALID_PARAMETER if parameter is inavlid
 *                  NFCSTATUS_FAILED            if failed response
 *
 ******************************************************************************/
NFCSTATUS tmsNciHalCheckRFCmdRespStatus() {
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    static uint16_t invalidParam = 0x09;
    if (((*getTmsNciHalCtrl()).rxDataLen > 0) && ((*getTmsNciHalCtrl()).pRxData[2] > 0)) {
        if ((*getTmsNciHalCtrl()).pRxData[3] == 0x09) {
            status = invalidParam;
        } else if ((*getTmsNciHalCtrl()).pRxData[3] != NFCSTATUS_SUCCESS) {
            status = NFCSTATUS_FAILED;
        }
    }
    return status;
}

static void tmsNciHalCoreInitializedFailed() {
    static NciHalMessage_t sMsg;
    sMsg.msgType = NCI_HAL_INIT_FAILED_MSG;
    sMsg.pMsgData = NULL;
    sMsg.size = 0;

    tmlNfcDeferredCall((*getTmlNfcContext())->callbackThreadId,
                          (NciHalMessage_t *)&sMsg);
    return;
}

/******************************************************************************
 * Function         tmsNciHalCoreInitializedComplete
 *
 * Description      This function is called when tmsNciHalCoreInitialized
 *                  complete all proprietary command exchanges. This function
 *                  informs libnfc-nci about completion of core initialize
 *                  and result of that through callback.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void tmsNciHalCoreInitializedComplete(NFCSTATUS status) {
    static NciHalMessage_t sMsg;

    if (status == NFCSTATUS_SUCCESS) {
        sMsg.msgType = NCI_HAL_POST_INIT_CPLT_MSG;
    } else {
        sMsg.msgType = NCI_HAL_ERROR_MSG;
    }
    sMsg.pMsgData = NULL;
    sMsg.size = 0;

    tmlNfcDeferredCall((*getTmlNfcContext())->callbackThreadId,
                          (NciHalMessage_t *)&sMsg);
    return;
}

/******************************************************************************
 * Function         tmsNciHalPreDiscover
 *
 * Description      This function is called by libnfc-nci to perform any
 *                  proprietary exchange before RF discovery.
 *
 * Returns          It always returns NFCSTATUS_SUCCESS (0).
 *
 ******************************************************************************/
int tmsNciHalPreDiscover(void) {
    /* Nothing to do here for initial version */
    return NFCSTATUS_FAILED;
}

/******************************************************************************
 * Function         tmsNciHalClose
 *
 * Description      This function close the NFCC interface and free all
 *                  resources.This is called by libnfc-nci on NFC service stop.
 *
 * Returns          Always return NFCSTATUS_SUCCESS (0).
 *
 ******************************************************************************/
NFCSTATUS tmsNciHalClose(bool shutdown) {
    NFCSTATUS status = NFCSTATUS_FAILED;
    uint8_t cmdResetNci[] = {0x20, 0x00, 0x01, 0x00};
    uint8_t cmdCeInPhoneOff[] = {0x20, 0x02, 0x05, 0x01,
                                     0xA0, 0x8E, 0x01, 0x00
                                    };
    uint8_t retry = 0;

    TMSLOG_NCIHAL_D("tmsNciHalClose: enter");

    //  property_get("tms.nfc.secos.download",value, "");
    //  if(!strcmp(value,"downloading")){
    //    TMSLOG_NCIHAL_D("secos downloading return");
    //    return NFCSTATUS_FAILED;
    //  }

    NfcHalAutoThreadMutex a(ghalFnLock);
    if ((*getTmsNciHalCtrl()).halStatus == HAL_STATUS_CLOSE) {
        TMSLOG_NCIHAL_D("tmsNciHalClose is already closed, ignoring close");
        return NFCSTATUS_FAILED;
    }

    CONCURRENCY_LOCK();
    int semVal;
    sem_getvalue(&((*getTmsNciHalCtrl()).syncSpiNfc), &semVal);
    if (semVal == 0) {
        sem_post(&((*getTmsNciHalCtrl()).syncSpiNfc));
    }
    if (!shutdown) {
        if ((*getNfcFL()).chipType >= thn31) {
            status = tmsNciHalSendExtCmd(sizeof(cmdCeInPhoneOff), cmdCeInPhoneOff);
            if (status != NFCSTATUS_SUCCESS) {
                TMSLOG_NCIHAL_E("CMD_CE_IN_PHONE_OFF: Failed");
            }
        }
    }

    if (gWriteUnlockedStatus == NFCSTATUS_FAILED) {
        TMSLOG_NCIHAL_D("tmsNciHalClose i2c write failed .Clean and Return");
        goto close_and_return;
    }

close_and_return:
    if (((*getNfcFL()).chipType <= thn31) || shutdown) {
        (*getTmsNciHalCtrl()).halStatus = HAL_STATUS_CLOSE;
    }

    if (!shutdown) { /* Should not send core-reset before AP shutdown */
        do { /*This is TMS_EXTNS code for retry*/
            status = tmsNciHalSendExtCmd(sizeof(cmdResetNci), cmdResetNci);

            if (status == NFCSTATUS_SUCCESS) {
                break;
            } else {
                TMSLOG_NCIHAL_E("NCI_CORE_RESET: Failed, perform retry after delay");
                usleep(1000 * 1000);
                if ((*getTmsNciHalCtrl()).halStatus == HAL_STATUS_CLOSE) {
                  // make sure read is pending
                  NFCSTATUS readStatus = tmsNciHalEnableTmlRead();
                  TMSLOG_NCIHAL_D("read status = %x", readStatus);
                }
                retry++;
                if (retry > 3) {
                    TMSLOG_NCIHAL_E(
                        "Maximum retries performed, shall restart HAL to recover");
                    abort();
                }
            }
        } while (1);
    }

    sem_destroy(&(*getTmsNciHalCtrl()).syncSpiNfc);

    if (NULL != (*getTmlNfcContext())->pDevHandle) {
        tmsNciHalCloseComplete(NFCSTATUS_SUCCESS);
        /* Abort any pending read and write */
        status = tmlNfcReadAbort();
        status = tmlNfcWriteAbort();

        osalNfcTimerCleanup();

        status = tmlNfcShutdown();

        if (0 != pthread_join((*getTmsNciHalCtrl()).clientThread, (void **)NULL)) {
            TMSLOG_TML_E("Fail to kill client thread!");
        }

        tmlNfcCleanUp();

        dal4NfcMsgRelease((*getTmsNciHalCtrl()).drvCfg.clientId);

        memset(&*getTmsNciHalCtrl(), 0x00, sizeof(*getTmsNciHalCtrl()));

        TMSLOG_NCIHAL_D("tmsNciHalClose - osalNfcDeInit completed");
    }

    CONCURRENCY_UNLOCK();

    tmsNciHalCleanupMonitor();
    gWriteUnlockedStatus = NFCSTATUS_SUCCESS;
    /* reset config cache */
    resetTmsConfig();
    /* Return success always */
    return NFCSTATUS_SUCCESS;
}

/******************************************************************************
 * Function         tmsNciHalCloseComplete
 *
 * Description      This function inform libnfc-nci about result of
 *                  tmsNciHalClose.
 *
 * Returns          void.
 *
 ******************************************************************************/
void tmsNciHalCloseComplete(NFCSTATUS status) {
    static NciHalMessage_t sMsg;

    if (status == NFCSTATUS_SUCCESS) {
        sMsg.msgType = NCI_HAL_CLOSE_CPLT_MSG;
    } else {
        sMsg.msgType = NCI_HAL_ERROR_MSG;
    }
    sMsg.pMsgData = NULL;
    sMsg.size = 0;
    (*getTmsNciHalCtrl()).halOpenStatus = false;
    tmlNfcDeferredCall((*getTmlNfcContext())->callbackThreadId, &sMsg);

    return;
}

#ifdef TMS_NFC
void tmsNciHalSetCeDiscShutdown(void) {
    int isFound = 0;
    unsigned long num = 0;
    isFound = getTmsNumValue(NAME_TMS_PWR_OFF_LISTEN_TECH_MASK, &num, sizeof(num));
    if (isFound > 0) {
        uint8_t cmdCeListenDiscNci[] = {0x21, 0x03, 0x01, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00
                                           };
        uint8_t numDiscConfig = 0;
        uint8_t *pPos = cmdCeListenDiscNci + 3;

        if (num & 0x01) {
            *(++pPos) = 0x80;
            *(++pPos) = 0x01;
            numDiscConfig ++;
        }
        if (num & 0x02) {
            *(++pPos) = 0x81;
            *(++pPos) = 0x01;
            numDiscConfig ++;
        }
        if (num & 0x04) {
            *(++pPos) = 0x82;
            *(++pPos) = 0x01;
            numDiscConfig ++;
        }
        TMSLOG_NCIHAL_D("NFC-NCI HAL:numDiscConfig:%d", numDiscConfig);
        //update number of configurations
        cmdCeListenDiscNci[3] = numDiscConfig;
        //update payload length
        cmdCeListenDiscNci[2] = 1 + (numDiscConfig << 1);
        if (NFCSTATUS_SUCCESS != tmsNciHalSendExtCmd(pPos - cmdCeListenDiscNci + 1, cmdCeListenDiscNci)) {
            TMSLOG_NCIHAL_E("NFC-NCI HAL: %s  set listen discover failed", __func__);
        }

    } else {
        TMSLOG_NCIHAL_E("NFC-NCI HAL: %s set default listen tech when power is off", __func__);
        uint8_t cmdCeListenDiscNci[] = {0x21, 0x03, 0x07, 0x03, 0x80,
                                            0x01, 0x81, 0x01, 0x82, 0x01
                                           };
        if (NFCSTATUS_SUCCESS != tmsNciHalSendExtCmd(sizeof(cmdCeListenDiscNci), cmdCeListenDiscNci)) {
            TMSLOG_NCIHAL_E("NFC-NCI HAL: %s  set listen discover failed", __func__);
        }
    }
}
#endif

/******************************************************************************
 * Function         tmsNciHalConfigDiscShutdown
 *
 * Description      Enable the CE and VEN config during shutdown.
 *
 * Returns          Always return NFCSTATUS_SUCCESS (0).
 *
 ******************************************************************************/
int tmsNciHalConfigDiscShutdown(void) {
    NFCSTATUS status;
    /*NCI_RESET_CMD*/

    uint8_t cmdDisableDisc[] = {0x21, 0x06, 0x01, 0x00};
    /**
     * set listen discover map.
     * Proto:IsoDep Mode:Listen Intf:IsoDep
     * Proto:NfcDep Mode:Listen Intf:NfcDep
     * Proto:Mifare Mode:Listen Intf:TAG-CMD
     */
    uint8_t cmdCeDiscMapNci[] = {0x21, 0x00, 0x0A, 0x03, 0x04, 0x02, 0x02,
                                     0x05, 0x02, 0x03, 0x80, 0x02, 0x80
                                    };
    /* Set Power Sub state. 0x7F-POWER_OFF */
    uint8_t cmdSetPowerOffState[] = {0x20, 0x09, 0x01, 0x7F};

    CONCURRENCY_LOCK();

    status = tmsNciHalSendExtCmd(sizeof(cmdDisableDisc), cmdDisableDisc);
    if (status != NFCSTATUS_SUCCESS) {
        TMSLOG_NCIHAL_E("CMD_DISABLE_DISCOVERY: Failed");
    }

    status = tmsNciHalSendExtCmd(sizeof(cmdCeDiscMapNci), cmdCeDiscMapNci);
    if (status != NFCSTATUS_SUCCESS) {
        TMSLOG_NCIHAL_E("cmd discover map failed");
    }

    status = tmsNciHalExtSendSramConfigToFlash();
    if (status != NFCSTATUS_SUCCESS) {
        TMSLOG_NCIHAL_E("Updation of the SRAM contents failed");
    }

    tmsNciHalSetCeDiscShutdown();

    /* set the readyToShutdown flag for not enabling tml read thread again after last response received */
    (*getTmsNciHalCtrl()).readyToShutdown = true;

    status = tmsNciHalSendExtCmd(sizeof(cmdSetPowerOffState), cmdSetPowerOffState);
    if (status != NFCSTATUS_SUCCESS) {
        TMSLOG_NCIHAL_E("set power off state failed");
    }

    CONCURRENCY_UNLOCK();

    status = tmsNciHalClose(true);
    if (status != NFCSTATUS_SUCCESS) {
        TMSLOG_NCIHAL_E("NCI_HAL_CLOSE: Failed");
    }

    /* Return success always */
    return NFCSTATUS_SUCCESS;
}


/******************************************************************************
 * Function         tmsNciHalControlGranted
 *
 * Description      Called by libnfc-nci when NFCC control is granted to HAL.
 *
 * Returns          Always returns NFCSTATUS_SUCCESS (0).
 *
 ******************************************************************************/
int tmsNciHalControlGranted(void) {
    /* Take the concurrency lock so no other calls from upper layer
     * will be allowed
     */
    CONCURRENCY_LOCK();

    if (NULL != (*getTmsNciHalCtrl()).pControlGrantedCallback) {
        (*(*getTmsNciHalCtrl()).pControlGrantedCallback)();
    }
    /* At the end concurrency unlock so calls from upper layer will
     * be allowed
     */
    CONCURRENCY_UNLOCK();
    return NFCSTATUS_SUCCESS;
}

/******************************************************************************
 * Function         tmsNciHalPowerCycle
 *
 * Description      This function is called by libnfc-nci when power cycling is
 *                  performed. When processing is complete it is notified to
 *                  libnfc-nci through tmsNciHalPowerCycleComplete.
 *
 * Returns          Always return NFCSTATUS_SUCCESS (0).
 *
 ******************************************************************************/
int tmsNciHalPowerCycle(void) {
    TMSLOG_NCIHAL_D("Power Cycle");
    NFCSTATUS status = NFCSTATUS_FAILED;
    if ((*getTmsNciHalCtrl()).halStatus != HAL_STATUS_OPEN) {
        TMSLOG_NCIHAL_D("Power Cycle failed due to hal status not open");
        return NFCSTATUS_FAILED;
    }
    status = tmlNfcIoCtl(TMLNFC_POWER_RESET);

    if (NFCSTATUS_SUCCESS == status) {
        TMSLOG_NCIHAL_D("THN31 Reset - SUCCESS\n");
    } else {
        TMSLOG_NCIHAL_D("THN31 Reset - FAILED\n");
    }

    tmsNciHalPowerCycleComplete(NFCSTATUS_SUCCESS);
    return NFCSTATUS_SUCCESS;
}

/******************************************************************************
 * Function         tmsNciHalPowerCycleComplete
 *
 * Description      This function is called to provide the status of
 *                  tmsNciHalPowerCycle to libnfc-nci through callback.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void tmsNciHalPowerCycleComplete(NFCSTATUS status) {
    static NciHalMessage_t sMsg;

    if (status == NFCSTATUS_SUCCESS) {
        sMsg.msgType = NCI_HAL_OPEN_CPLT_MSG;
    } else {
        sMsg.msgType = NCI_HAL_ERROR_MSG;
    }
    sMsg.pMsgData = NULL;
    sMsg.size = 0;

    tmlNfcDeferredCall((*getTmlNfcContext())->callbackThreadId, &sMsg);

    return;
}
/******************************************************************************
 * Function         tmsNciHalCheckNciCmdWriteWindow
 *
 * Description      This function is called to check the write synchroniztion
 *                  status if write already aquired then wait for corresponding
                    read to complete.
 *
 * Returns          return 0 on success and -1 on fail.
 *
 ******************************************************************************/

int tmsNciHalCheckNciCmdWriteWindow(uint16_t cmdLen, uint8_t *pCmd) {
    UNUSED_PROP(cmdLen);
    NFCSTATUS status = NFCSTATUS_FAILED;
    int semTimedOut = 2, s;
    struct timespec ts;

    if (cmdLen < 1) {
        android_errorWriteLog(0x534e4554, "153880357");
        return NFCSTATUS_FAILED;
    }

    if ((pCmd[0] & 0xF0) == 0x20) {
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += semTimedOut;
        while ((s = sem_timedwait(&(*getTmsNciHalCtrl()).syncSpiNfc, &ts)) == -1 &&
                errno == EINTR) {
            continue; /* Restart if interrupted by handler */
        }
        if (s != -1) {
            status = NFCSTATUS_SUCCESS;
        }
    } else {
        /* cmd window check not required for writing data packet */
        status = NFCSTATUS_SUCCESS;
    }
    return status;
}

/******************************************************************************
 * Function         tmsNciHalIoctl
 *
 * Description      This function is called by jni when wired mode is
 *                  performed.First Thn31 driver will give the access
 *                  permission whether wired mode is allowed or not
 *                  arg (0):
 * Returns          return 0 on success and -1 on fail, On success
 *                  update the acutual state of operation in arg pointer
 *
 ******************************************************************************/
int tmsNciHalIoctl(long arg, void *pData) {
    return tmsNciHalIoctlIf(arg, pData);
}

/******************************************************************************
 * Function         tmsNciHalNfccCoreResetInit
 *
 * Description      Helper function to do nfcc core reset & core init
 *
 * Returns          Status
 *
 ******************************************************************************/
NFCSTATUS tmsNciHalNfccCoreResetInit(bool keepConfig) {
    NFCSTATUS status = NFCSTATUS_FAILED;
    uint8_t retryCnt = 0;
    uint8_t cmdResetNci[] = {0x20, 0x00, 0x01, 0x01};

    if (keepConfig) {
        cmdResetNci[3] = 0x00;
    }
retry_core_reset:
    status = tmsNciHalSendExtCmd(sizeof(cmdResetNci), cmdResetNci);
    if ((status != NFCSTATUS_SUCCESS) && (retryCnt < 3)) {
        TMSLOG_NCIHAL_D("Retry: NCI_CORE_RESET");
        retryCnt++;
        goto retry_core_reset;
    } else if (status != NFCSTATUS_SUCCESS) {
        TMSLOG_NCIHAL_E("NCI_CORE_RESET failed!!!\n");
        return status;
    }

    retryCnt = 0;
    uint8_t cmdInitNci[] = {0x20, 0x01, 0x00};
    uint8_t cmdInitNci2_0[] = {0x20, 0x01, 0x02, 0x00, 0x00};
retryCoreInit:
    if ((*getTmsNciHalCtrl()).nciInfo.nciVersion == NCI_VERSION_2_0) {
        status = tmsNciHalSendExtCmd(sizeof(cmdInitNci2_0), cmdInitNci2_0);
    } else {
        status = tmsNciHalSendExtCmd(sizeof(cmdInitNci), cmdInitNci);
    }

    if ((status != NFCSTATUS_SUCCESS) && (retryCnt < 3)) {
        TMSLOG_NCIHAL_D("Retry: NCI_CORE_INIT\n");
        retryCnt++;
        goto retryCoreInit;
    } else if (status != NFCSTATUS_SUCCESS) {
        TMSLOG_NCIHAL_E("NCI_CORE_INIT failed!!!\n");
        return status;
    }

    return status;
}

/******************************************************************************
 * Function         tmsNciHalDoSwpSessionReset
 *
 * Description      This function is called to set the session id to default
 *                  value.
 *
 * Returns          NFCSTATUS.
 *
 ******************************************************************************/
static NFCSTATUS tmsNciHalDoSwpSessionReset(void) {
    NFCSTATUS status = NFCSTATUS_FAILED;
    static uint8_t resetSwpSessionIdentitySet[] = {
        0x20, 0x02, 0x17, 0x02, 0xA0, 0xEA, 0x08, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xA0, 0x1E, 0x08,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
    status = tmsNciHalSendExtCmd(sizeof(resetSwpSessionIdentitySet),
                                      resetSwpSessionIdentitySet);
    if (status != NFCSTATUS_SUCCESS) {
        TMSLOG_NCIHAL_E("TMS reset_ese_session_identity_set command failed");
    }
    return status;
}
/******************************************************************************
 * Function         tmsNciHalDoFactoryReset
 *
 * Description      This function is called during factory reset to clear/reset
 *                  nfc sub-system persistant data.
 *
 * Returns          void.
 *
 ******************************************************************************/
void tmsNciHalDoFactoryReset(void) {
    NFCSTATUS status = NFCSTATUS_FAILED;
    TMSLOG_NCIHAL_E("tmsNciHalDoFactoryReset enter");
    NfcHalAutoThreadMutex a(ghalFnLock);
    if ((*getTmsNciHalCtrl()).halStatus == HAL_STATUS_CLOSE) {
        status = tmsNciHalMinOpen();
        if (status != NFCSTATUS_SUCCESS) {
            TMSLOG_NCIHAL_E("%s: TMS Nfc Open failed", __func__);
            return;
        }
    }
    status = tmsNciHalDoSwpSessionReset();
    if (status != NFCSTATUS_SUCCESS) {
        TMSLOG_NCIHAL_E("%s failed. status = %x ", __func__, status);
    }
}

/******************************************************************************
 * Function         tmsNciHalHciNetworkReset
 *
 * Description      This function resets the session id's of all the se's
 *                  in the HCI network and notify to HCI_NETWORK_RESET event to
 *                  NFC HAL Client.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void tmsNciHalHciNetworkReset(void) {
    static NciHalMessage_t sMsg;
    sMsg.pMsgData = NULL;
    sMsg.size = 0;

    NFCSTATUS status = tmsNciHalDoSwpSessionReset();

    if (status != NFCSTATUS_SUCCESS) {
        sMsg.msgType = NCI_HAL_ERROR_MSG;
    } else {
        sMsg.msgType = NCI_HAL_HCI_NETWORK_RESET_MSG;
    }
    tmlNfcDeferredCall((*getTmlNfcContext())->callbackThreadId, &sMsg);
}

/******************************************************************************
 * Function         tmsNciHalPrintResStatus
 *
 * Description      This function is called to process the response status
 *                  and print the status byte.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void tmsNciHalPrintResStatus(uint8_t *pRxData, uint16_t *pLen) {
    UNUSED_PROP(pLen);
    static uint8_t responseBuf[][30] = {"STATUS_OK",
                                         "STATUS_REJECTED",
                                         "STATUS_RF_FRAME_CORRUPTED",
                                         "STATUS_FAILED",
                                         "STATUS_NOT_INITIALIZED",
                                         "STATUS_SYNTAX_ERROR",
                                         "STATUS_SEMANTIC_ERROR",
                                         "RFU",
                                         "RFU",
                                         "STATUS_INVALID_PARAM",
                                         "STATUS_MESSAGE_SIZE_EXCEEDED",
                                         "STATUS_UNDEFINED"
                                        };
    int statusByte;
    if (pRxData[0] == 0x40 && (pRxData[1] == 0x02 || pRxData[1] == 0x03)) {
        if (pRxData[2] && pRxData[3] <= 10) {
            statusByte = pRxData[CORE_RES_STATUS_BYTE];
            TMSLOG_NCIHAL_D("%s: response status =%s", __func__,
                            responseBuf[statusByte]);
        } else {
            TMSLOG_NCIHAL_D("%s: response status =%s", __func__, responseBuf[11]);
        }
    }

    if (pRxData[2] && (gConfigAccess == true)) {
        if (pRxData[3] != NFCSTATUS_SUCCESS) {
            TMSLOG_NCIHAL_W("Invalid Data from config file.");
        }
    }
}

/*******************************************************************************
**
** Function         tmsNciHalConfigFeatureList
**
** Description      Configures the featureList based on chip type &
**                  Configure fragmentation length based on chip type.
**                  HW Version information number will provide chipType.
**                  HW Version can be obtained from CORE_INIT_RESPONSE(NCI 1.0)
**                  or CORE_RST_NTF(NCI 2.0)
**
** Parameters       CORE_INIT_RESPONSE/CORE_RST_NTF, len
**
** Returns          none
*******************************************************************************/
void tmsNciHalConfigFeatureList(uint8_t *pInitRsp, uint16_t rspLen) {
    (*getTmsNciHalCtrl()).chipType = PCONFIGFL->processChipType(pInitRsp, rspLen);
    NfcChipType chipType = (*getTmsNciHalCtrl()).chipType;
    TMSLOG_NCIHAL_D("tmsNciHalConfigFeatureList ()chipType = %d", chipType);
    CONFIGURE_FEATURELIST(chipType);
    /* update fragment len based on the chip type.*/
    tmlNfcIoCtl(TMLNFC_SET_FRAGMENT_SIZE);
}

/*******************************************************************************
**
** Function         tmsNciHalConfigNciParser(bool enable)
**
** Description      Helper function to configure LxDebug modes
**
** Parameters       none
**
** Returns          void
*******************************************************************************/
void tmsNciHalConfigNciParser(bool enable) {
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    unsigned long lxDebugCfg = 0;
    int  isFound = 0;
    static uint8_t cmdLxDebug[] = { 0x20, 0x02, 0x06, 0x01, 0xA0, 0x1D, 0x02, 0x00, 0x00 };

    isFound = getTmsNumValue(NAME_TMS_CORE_PROP_SYSTEM_DEBUG, &lxDebugCfg, sizeof(lxDebugCfg));
    if (isFound > 0 && enable == true) {
        cmdLxDebug[7] = (uint8_t)lxDebugCfg & LX_DEBUG_CFG_MASK;
        cmdLxDebug[8] = (uint8_t)(lxDebugCfg >> 8) & LX_DEBUG_CFG_MASK;
    }
    status = tmsNciHalSendExtCmd(sizeof(cmdLxDebug) / sizeof(cmdLxDebug[0]), cmdLxDebug);
    if (status != NFCSTATUS_SUCCESS) {
        TMSLOG_NCIHAL_E("Set lxDebug config failed");
    }
}

bool isTmsChip(){
    int ret = -1;
    struct stat fileStat;
    const uint16_t maxLen = 260;
    char pNfcDevNode[maxLen] = {0};

    if (!getTmsStrValue(NAME_TMS_NFC_DEV_NODE, pNfcDevNode,
                        maxLen)) {
      ALOGE("Nfc device name not found in config, use default : /dev/tms_nfc");
      strlcpy(pNfcDevNode, "/dev/tms_nfc", (maxLen * sizeof(char)));
    }

    ret = stat(pNfcDevNode, &fileStat);
    if (0 == ret) {
        ALOGI("Chip is tms nfc");
        return true;
    }

    return false;
}

void tmsNciHalConfigAidlHalService() {
    ALOGI("config aidl hal service");
    gIsHalAidlService = true;
}

