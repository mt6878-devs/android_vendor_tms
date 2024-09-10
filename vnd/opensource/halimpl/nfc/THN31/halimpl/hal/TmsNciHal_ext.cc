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

#include <log/log.h>
#include <Dal4Nfc_messageQueueLib.h>
#include <TmsConfig.h>
#include <TmsLog.h>
#include <TmsNciHal.h>
#include <TmsNciHal_Adaptation.h>
#include <TmsNciHal_ext.h>
#include <TmlNfc.h>
#include "TmsNciHal.h"
#include "TmsNciHal_IoctlOperations.h"

/* Timeout value to wait for response from THN31 */
#define HAL_EXTNS_WRITE_RSP_TIMEOUT (1000)
#define NCI_NFC_DEP_RF_INTF 0x03
#define NCI_STATUS_OK 0x00
#define NCI_MODE_HEADER_LEN 3

/******************* Global variables *****************************************/
uint8_t gIcodeDetected = 0x00;
uint8_t gIcodeSendEof = 0x00;
static uint8_t gEeDiscDone = 0x00;
/* External global variable to get FW version from FW file*/
/* local pBuffer to store CORE_INIT response */
static uint32_t gCoreInitRsp[40];
static uint32_t gCoreInitRspLen;
uint32_t gCleanupTtimer;

/************** HAL extension functions ***************************************/
static void halExtnsWriteRspTimeoutCb(uint32_t timerId, void *pContext);

/*Proprietary cmd sent to HAL to send reader mode flag
 * Last byte of 4 byte proprietary cmd data contains ReaderMode flag
 * If this flag is enabled, NFC-DEP protocol is modified to T3T protocol
 * if FrameRF interface is selected. This needs to be done as the FW
 * always sends Ntf for FrameRF with NFC-DEP even though FrameRF with T3T is
 * previously selected with DISCOVER_SELECT_CMD
 */
#define PROPRIETARY_CMD_FELICA_READER_MODE 0xFE
static uint8_t gFelicaReaderMode;
static NFCSTATUS tmsNciHalExtProcessNfcInitRsp(uint8_t *pNtf,
        uint16_t *pLen);

/*******************************************************************************
**
** Function         tmsNciHalExtInit
**
** Description      initialize extension function
**
*******************************************************************************/
void tmsNciHalExtInit(void) {
    gIcodeDetected = 0x00;
}

/*******************************************************************************
**
** Function         tmsNciHalExtSendSramConfigToFlash
**
** Description      This function is called to update the SRAM contents such as
**                  set config to FLASH for permanent storage.
**                  Note: This function has to be called after set config and
**                  before sending  core_reset command again.
**
*******************************************************************************/
NFCSTATUS tmsNciHalExtSendSramConfigToFlash() {
    TMSLOG_NCIHAL_D("tmsNciHalExtSendSramConfigToFlash  send");
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    uint8_t sendSramFlash[] = {TMS_PROPCMD_GID, TMS_FLUSH_SRAM_AO_TO_FLASH, 0x00};
    status = tmsNciHalSendExtCmd(sizeof(sendSramFlash), sendSramFlash);
    return status;

}
/*******************************************************************************
**
** Function         tmsNciHalProcessExtRsp
**
** Description      Process extension function response
**
** Returns          NFCSTATUS_SUCCESS if success
**
*******************************************************************************/
NFCSTATUS tmsNciHalProcessExtRsp(uint8_t *pNtf, uint16_t *pLen) {
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    if (pNtf[0] == 0x61 && pNtf[1] == 0x05 && *pLen < 14) {
        if (*pLen <= 6) {
            android_errorWriteLog(0x534e4554, "118152591");
        }
        TMSLOG_NCIHAL_E("RF_INTF_ACTIVATED_NTF length error!");
        status = NFCSTATUS_FAILED;
        return status;
    }

    if (pNtf[0] == 0x61 && pNtf[1] == 0x05 && pNtf[4] == 0x01 &&
            pNtf[5] == 0x05 && pNtf[6] == 0x02 && gFelicaReaderMode) {
        /*If FelicaReaderMode is enabled,Change Protocol to T3T from NFC-DEP
             * when FrameRF interface is selected*/
        pNtf[5] = 0x03;
        TMSLOG_NCIHAL_D("FelicaReaderMode:Activity 1.1");
    }

    status = NFCSTATUS_SUCCESS;

    if (pNtf[0] == 0x61 && pNtf[1] == 0x05) {

        switch (pNtf[4]) {
            case 0x00:
                TMSLOG_NCIHAL_D("TmsNci: RF Interface = NFCEE Direct RF");
                break;
            case 0x01:
                TMSLOG_NCIHAL_D("TmsNci: RF Interface = Frame RF");
                break;
            case 0x02:
                TMSLOG_NCIHAL_D("TmsNci: RF Interface = ISO-DEP");
                break;
            case 0x03:
                TMSLOG_NCIHAL_D("TmsNci: RF Interface = NFC-DEP");
                break;
            case 0x80:
                TMSLOG_NCIHAL_D("TmsNci: RF Interface = MIFARE");
                break;
            default:
                TMSLOG_NCIHAL_D("TmsNci: RF Interface = Unknown");
                break;
        }

        switch (pNtf[5]) {
            case 0x01:
                TMSLOG_NCIHAL_D("TmsNci: Protocol = T1T");
                tmsDtaT1TEnable();
                break;
            case 0x02:
                TMSLOG_NCIHAL_D("TmsNci: Protocol = T2T");
                break;
            case 0x03:
                TMSLOG_NCIHAL_D("TmsNci: Protocol = T3T");
                break;
            case 0x04:
                TMSLOG_NCIHAL_D("TmsNci: Protocol = ISO-DEP");
                break;
            case 0x05:
                TMSLOG_NCIHAL_D("TmsNci: Protocol = NFC-DEP");
                break;
            case 0x06:
                TMSLOG_NCIHAL_D("TmsNci: Protocol = 15693");
                break;
            case 0x80:
                TMSLOG_NCIHAL_D("TmsNci: Protocol = MIFARE");
                break;
            case 0x81:
                TMSLOG_NCIHAL_D("TmsNci: Protocol = Kovio");
                break;
            default:
                TMSLOG_NCIHAL_D("TmsNci: Protocol = Unknown");
                break;
        }

        switch (pNtf[6]) {
            case 0x00:
                TMSLOG_NCIHAL_D("TmsNci: Mode = A Passive Poll");
                break;
            case 0x01:
                TMSLOG_NCIHAL_D("TmsNci: Mode = B Passive Poll");
                break;
            case 0x02:
                TMSLOG_NCIHAL_D("TmsNci: Mode = F Passive Poll");
                break;
            case 0x03:
                TMSLOG_NCIHAL_D("TmsNci: Mode = A Active Poll");
                break;
            case 0x05:
                TMSLOG_NCIHAL_D("TmsNci: Mode = F Active Poll");
                break;
            case 0x06:
                TMSLOG_NCIHAL_D("TmsNci: Mode = 15693 Passive Poll");
                break;
            case 0x70:
                TMSLOG_NCIHAL_D("TmsNci: Mode = Kovio");
                break;
            case 0x80:
                TMSLOG_NCIHAL_D("TmsNci: Mode = A Passive Listen");
                break;
            case 0x81:
                TMSLOG_NCIHAL_D("TmsNci: Mode = B Passive Listen");
                break;
            case 0x82:
                TMSLOG_NCIHAL_D("TmsNci: Mode = F Passive Listen");
                break;
            case 0x83:
                TMSLOG_NCIHAL_D("TmsNci: Mode = A Active Listen");
                break;
            case 0x85:
                TMSLOG_NCIHAL_D("TmsNci: Mode = F Active Listen");
                break;
            case 0x86:
                TMSLOG_NCIHAL_D("TmsNci: Mode = 15693 Passive Listen");
                break;
            default:
                TMSLOG_NCIHAL_D("TmsNci: Mode = Unknown");
                break;
        }
    }
    tmsNciHalExtProcessNfcInitRsp(pNtf, pLen);

    if (pNtf[0] == NCI_MT_NTF && ((pNtf[1] & NCI_OID_MASK) == NCI_MSG_CORE_RESET) &&
            pNtf[3] == CORE_RESET_TRIGGER_TYPE_POWERED_ON) {
        status = NFCSTATUS_FAILED;
        TMSLOG_NCIHAL_D("Skipping power on reset notification!!:");
        return status;
    }
    if (pNtf[0] == 0x42 && pNtf[1] == 0x01 && pNtf[2] == 0x01 && pNtf[3] == 0x00) {
        if ((*getTmsNciHalCtrl()).halExtEnabled == TRUE && (*getNfcFL()).chipType >= thn31) {
            (*getTmsNciHalCtrl()).nciInfo.waitForNtf = TRUE;
            TMSLOG_NCIHAL_D(" Mode set received");
        }
    } else if (pNtf[0] == 0x61 && pNtf[1] == 0x05 && pNtf[2] == 0x15
               && pNtf[4] == 0x01 && pNtf[5] == 0x06 && pNtf[6] == 0x06) {
        TMSLOG_NCIHAL_D("> Going through workaround - notification of ISO 15693");
        gIcodeDetected = 0x01;
        pNtf[21] = 0x01;
        pNtf[22] = 0x01;
    } else if (pNtf[0] == 0x61 && pNtf[1] == 0x06 && gIcodeDetected == 1) {
        TMSLOG_NCIHAL_D("> Polling Loop Re-Started");
        gIcodeDetected = 0;
    } else if (*pLen == 4 && pNtf[0] == 0x40 && pNtf[1] == 0x02 &&
               pNtf[2] == 0x01 && pNtf[3] == 0x06) {
        /*TMSLOG_NCIHAL_D("> Deinit workaround for LLCP set_config 0x%x 0x%x 0x%x",
                        pNtf[21], pNtf[22], pNtf[23]);*/
        pNtf[0] = 0x40;
        pNtf[1] = 0x02;
        pNtf[2] = 0x02;
        pNtf[3] = 0x00;
        pNtf[4] = 0x00;
        *pLen = 5;
    }

    if((pNtf[0] == 0x60 && pNtf[1] == 0x07 && pNtf[2] == 0x01 && ((pNtf[3] == 0xE5) ||
      (pNtf[3] == 0x60)))||(pNtf[0] == 0x61 && pNtf[1] == 0x21 && pNtf[2] == 0x00)) {
        status = NFCSTATUS_FAILED;
        TMSLOG_NCIHAL_D("ignore core generic error");
        return status;
    }
    // 4200 02 00 01
    else if (pNtf[0] == 0x42 && pNtf[1] == 0x00 && gEeDiscDone == 0x01) {
        TMSLOG_NCIHAL_D("Going through workaround - NFCEE_DISCOVER_RSP");
        if (pNtf[4] == 0x01) {
            pNtf[4] = 0x00;

            gEeDiscDone = 0x00;
        }
        TMSLOG_NCIHAL_D("Going through workaround - NFCEE_DISCOVER_RSP - END");

    } else if (*pLen == 4 && pNtf[0] == 0x4F && pNtf[1] == 0x11 && pNtf[2] == 0x01) {
        if (pNtf[3] == 0x00) {
            TMSLOG_NCIHAL_D(
                ">  Workaround for ISO-DEP Presence Check, ignore response and wait "
                "for notification");
            pNtf[0] = 0x60;
            pNtf[1] = 0x06;
            pNtf[2] = 0x03;
            pNtf[3] = 0x01;
            pNtf[4] = 0x00;
            pNtf[5] = 0x01;
            *pLen = 6;
        } else {
            TMSLOG_NCIHAL_D(
                ">  Workaround for ISO-DEP Presence Check, presence check return "
                "failed");
            pNtf[0] = 0x60;
            pNtf[1] = 0x08;
            pNtf[2] = 0x02;
            pNtf[3] = 0xB2;
            pNtf[4] = 0x00;
            *pLen = 5;
        }
    } else if (*pLen == 4 && pNtf[0] == 0x6F && pNtf[1] == 0x11 &&
               pNtf[2] == 0x01) {
        if (pNtf[3] == 0x01) {
            TMSLOG_NCIHAL_D(
                ">  Workaround for ISO-DEP Presence Check - Card still in field");
            pNtf[0] = 0x00;
            pNtf[1] = 0x00;
            pNtf[2] = 0x01;
            pNtf[3] = 0x7E;
        } else {
            TMSLOG_NCIHAL_D(
                ">  Workaround for ISO-DEP Presence Check - Card not in field");
            pNtf[0] = 0x60;
            pNtf[1] = 0x08;
            pNtf[2] = 0x02;
            pNtf[3] = 0xB2;
            pNtf[4] = 0x00;
            *pLen = 5;
        }
    }

    return status;
}

/******************************************************************************
 * Function         tmsNciHalExtProcessNfcInitRsp
 *
 * Description      This function is used to process the HAL NFC core reset rsp
 *                  and ntf and core init rsp of NCI 1.0 or NCI2.0 and update
 *                  NCI version.
 *                  It also handles error response such as core_reset_ntf with
 *                  error status in both NCI2.0 and NCI1.0.
 *
 * Returns          Returns NFCSTATUS_SUCCESS if parsing response is successful
 *                  or returns failure.
 *
******************************************************************************/
static NFCSTATUS tmsNciHalExtProcessNfcInitRsp(uint8_t *pNtf, uint16_t *pLen) {
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    /* Parsing CORE_RESET_RSP and CORE_RESET_NTF to update NCI version.*/
    if (pNtf == NULL || *pLen < 2) {
        return NFCSTATUS_FAILED;
    }
    if (pNtf[0] == NCI_MT_RSP && ((pNtf[1] & NCI_OID_MASK) == NCI_MSG_CORE_RESET)) {
        if (*pLen < 4) {
            android_errorWriteLog(0x534e4554, "169258455");
            return NFCSTATUS_FAILED;
        }
        if (pNtf[2] == 0x01 && pNtf[3] == 0x00) {
            TMSLOG_NCIHAL_D("CORE_RESET_RSP NCI2.0");
            if ((*getTmsNciHalCtrl()).halExtEnabled == TRUE) {
                (*getTmsNciHalCtrl()).nciInfo.waitForNtf = TRUE;
            }
        } else if (pNtf[2] == 0x03 && pNtf[3] == 0x00) {
            if (*pLen < 5) {
                android_errorWriteLog(0x534e4554, "169258455");
                return NFCSTATUS_FAILED;
            }
            TMSLOG_NCIHAL_D("CORE_RESET_RSP NCI1.0");
            (*getTmsNciHalCtrl()).nciInfo.nciVersion = pNtf[4];
        } else {
            status = NFCSTATUS_FAILED;
        }
    } else if (pNtf[0] == NCI_MT_NTF && ((pNtf[1] & NCI_OID_MASK) == NCI_MSG_CORE_RESET)) {
        if (*pLen < 4) {
            android_errorWriteLog(0x534e4554, "169258455");
            return NFCSTATUS_FAILED;
        }
        if (pNtf[3] == CORE_RESET_TRIGGER_TYPE_CORE_RESET_CMD_RECEIVED) {
            if (*pLen < 6) {
                android_errorWriteLog(0x534e4554, "169258455");
                return NFCSTATUS_FAILED;
            }
            TMSLOG_NCIHAL_D("CORE_RESET_NTF NCI2.0 reason CORE_RESET_CMD received !");
            (*getTmsNciHalCtrl()).nciInfo.nciVersion  = pNtf[5];
            if (!(*getTmsNciHalCtrl()).halOpenStatus) {
                tmsNciHalConfigFeatureList(pNtf, *pLen);
            }
            int len = pNtf[2] + 2; /*include 2 byte header*/
            if (len != *pLen - 1) {
                TMSLOG_NCIHAL_E("tmsNciHalExtProcessNfcInitRsp invalid NTF length");
                android_errorWriteLog(0x534e4554, "121263487");
                return NFCSTATUS_FAILED;
            }
            (*getFwVerRsp()) = (((uint32_t)pNtf[len - 2]) << 16U) |
                        (((uint32_t)pNtf[len - 1]) << 8U) | pNtf[len];
            TMSLOG_NCIHAL_D("TmsNci> FW Version: %x.%x.%x", pNtf[len - 2], pNtf[len - 1], pNtf[len]);
        } else {
            uint32_t i;
            char printBuffer[*pLen * 3 + 1];

            memset(printBuffer, 0, sizeof(printBuffer));
            for (i = 0; i < *pLen; i++) {
                snprintf(&printBuffer[i * 2], 3, "%02X", pNtf[i]);
            }
            TMSLOG_NCIHAL_D("CORE_RESET_NTF received !");
            TMSLOG_NCIR_E("len = %3d > %s", *pLen, printBuffer);
            tmsNciHalEmergencyRecovery(pNtf[3]);
            status = NFCSTATUS_FAILED;
        } /* Parsing CORE_INIT_RSP*/
    } else if (pNtf[0] == NCI_MT_RSP && ((pNtf[1] & NCI_OID_MASK) == NCI_MSG_CORE_INIT)) {
        if ((*getTmsNciHalCtrl()).nciInfo.nciVersion == NCI_VERSION_2_0) {
            TMSLOG_NCIHAL_D("CORE_INIT_RSP NCI2.0 received !");
        } else {
            TMSLOG_NCIHAL_D("CORE_INIT_RSP NCI1.0 received !");
            if (!(*getTmsNciHalCtrl()).halOpenStatus &&
                    (*getTmsNciHalCtrl()).nciInfo.nciVersion != NCI_VERSION_2_0) {
                tmsNciHalConfigFeatureList(pNtf, *pLen);
            }
            if (*pLen < 3) {
                android_errorWriteLog(0x534e4554, "169258455");
                return NFCSTATUS_FAILED;
            }
            int len = pNtf[2] + 2; /*include 2 byte header*/
            if (len != *pLen - 1) {
                TMSLOG_NCIHAL_E("tmsNciHalExtProcessNfcInitRsp invalid NTF length");
                android_errorWriteLog(0x534e4554, "121263487");
                return NFCSTATUS_FAILED;
            }
            (*getFwVerRsp()) = (((uint32_t)pNtf[len - 2]) << 16U) |
                        (((uint32_t)pNtf[len - 1]) << 8U) | pNtf[len];
            if ((*getFwVerRsp()) == 0) {
                status = NFCSTATUS_FAILED;
            }
            gCoreInitRspLen = *pLen;
            memcpy(gCoreInitRsp, pNtf, *pLen);
            TMSLOG_NCIHAL_D("TmsNci> FW Version: %x.%x.%x", pNtf[len - 2],
                            pNtf[len - 1], pNtf[len]);
        }
    }
    return status;
}

/******************************************************************************
 * Function         tmsNciHalProcessExtCmdRsp
 *
 * Description      This function process the extension command response. It
 *                  also checks the received response to expected response.
 *
 * Returns          returns NFCSTATUS_SUCCESS if response is as expected else
 *                  returns failure.
 *
 ******************************************************************************/
static NFCSTATUS tmsNciHalProcessExtCmdRsp(uint16_t cmdLen,
        uint8_t *pCmd) {
    NFCSTATUS status = NFCSTATUS_FAILED;
    uint16_t dataWritten = 0;

    /* Check NCI Command is well formed */
    if ((cmdLen != (pCmd[2] + NCI_MODE_HEADER_LEN))) {
        TMSLOG_NCIHAL_E("NCI command not well formed");
        return NFCSTATUS_FAILED;
    }


    /* Create the local semaphore */
    if (tmsNciHalInitCbData(&(*getTmsNciHalCtrl()).extCbData, NULL) !=
            NFCSTATUS_SUCCESS) {
        TMSLOG_NCIHAL_D("Create extCbData failed");
        return NFCSTATUS_FAILED;
    }

    (*getTmsNciHalCtrl()).extCbData.status = NFCSTATUS_SUCCESS;

    /* Send ext command */
    dataWritten = tmsNciHalWriteUnlocked(cmdLen, pCmd, ORIG_TMSHAL);
    if (dataWritten != cmdLen) {
        TMSLOG_NCIHAL_D("tmsNciHalWrite failed for hal ext");
        goto clean_and_return;
    }

    /* Start timer */
    status = osalNfcTimerStart(*getTimeoutTimerId(), HAL_EXTNS_WRITE_RSP_TIMEOUT,
                                   &halExtnsWriteRspTimeoutCb, NULL);
    if (NFCSTATUS_SUCCESS == status) {
        TMSLOG_NCIHAL_D("Response timer started");
    } else {
        TMSLOG_NCIHAL_E("Response timer not started!!!");
        status = NFCSTATUS_FAILED;
        goto clean_and_return;
    }

    /* Wait for rsp */
    TMSLOG_NCIHAL_D("Waiting after ext cmd sent");
    if (SEM_WAIT((*getTmsNciHalCtrl()).extCbData)) {
        TMSLOG_NCIHAL_E("p_hal_ext->extCbData.sem semaphore error");
        goto clean_and_return;
    }

    /* Stop Timer */
    status = osalNfcTimerStop(*getTimeoutTimerId());
    if (NFCSTATUS_SUCCESS == status) {
        TMSLOG_NCIHAL_D("Response timer stopped");
    } else {
        TMSLOG_NCIHAL_E("Response timer stop ERROR!!!");
        status = NFCSTATUS_FAILED;
        goto clean_and_return;
    }

    if (cmdLen < 3) {
        android_errorWriteLog(0x534e4554, "153880630");
        status = NFCSTATUS_FAILED;
        goto clean_and_return;
    }

    /* No NTF expected for OMAPI command */
    if (pCmd[0] == 0x2F && pCmd[1] == 0x1 &&  pCmd[2] == 0x01) {
        (*getTmsNciHalCtrl()).nciInfo.waitForNtf = FALSE;
    }
    /* Start timer to wait for NTF*/
    if ((*getTmsNciHalCtrl()).nciInfo.waitForNtf == TRUE) {
        status = osalNfcTimerStart(*getTimeoutTimerId(), HAL_EXTNS_WRITE_RSP_TIMEOUT,
                                       &halExtnsWriteRspTimeoutCb, NULL);
        if (NFCSTATUS_SUCCESS == status) {
            TMSLOG_NCIHAL_D("Response timer started");
        } else {
            TMSLOG_NCIHAL_E("Response timer not started!!!");
            status = NFCSTATUS_FAILED;
            goto clean_and_return;
        }
        if (SEM_WAIT((*getTmsNciHalCtrl()).extCbData)) {
            TMSLOG_NCIHAL_E("p_hal_ext->extCbData.sem semaphore error");
            /* Stop Timer */
            status = osalNfcTimerStop(*getTimeoutTimerId());
            goto clean_and_return;
        }
        status = osalNfcTimerStop(*getTimeoutTimerId());
        if (NFCSTATUS_SUCCESS == status) {
            TMSLOG_NCIHAL_D("Response timer stopped");
        } else {
            TMSLOG_NCIHAL_E("Response timer stop ERROR!!!");
            status = NFCSTATUS_FAILED;
            goto clean_and_return;
        }
    }

    if ((*getTmsNciHalCtrl()).extCbData.status != NFCSTATUS_SUCCESS) {
        TMSLOG_NCIHAL_E(
            "Callback Status is failed!! Timer Expired!! Couldn't read it! 0x%x",
            (*getTmsNciHalCtrl()).extCbData.status);
        status = NFCSTATUS_FAILED;
        goto clean_and_return;
    }

    TMSLOG_NCIHAL_D("Checking response");
    status = NFCSTATUS_SUCCESS;

    /*Response check for Set config, Core Reset & Core init command sent part of HAL_EXT*/
    if ((*getTmsNciHalCtrl()).pRxData[0] == 0x40 &&
            (*getTmsNciHalCtrl()).pRxData[1] <= 0x02 &&
            (*getTmsNciHalCtrl()).pRxData[2] != 0x00) {

        status = (*getTmsNciHalCtrl()).pRxData[3];
        if (status != NCI_STATUS_OK) {
            /*Add 500ms delay for FW to flush circular pBuffer */
            usleep(500 * 1000);
            TMSLOG_NCIHAL_D("Status Failed. Status = 0x%02x", status);
        }
    }

clean_and_return:
    tmsNciHalCleanupCbData(&(*getTmsNciHalCtrl()).extCbData);
    (*getTmsNciHalCtrl()).nciInfo.waitForNtf = FALSE;
    HAL_DISABLE_EXT();
    return status;
}

/******************************************************************************
 * Function         tmsNciHalWriteExt
 *
 * Description      This function inform the status of tmsNciHalOpen
 *                  function to libnfc-nci.
 *
 * Returns          It return NFCSTATUS_SUCCESS then continue with send else
 *                  sends NFCSTATUS_FAILED direct response is prepared and
 *                  do not send anything to NFCC.
 *
 ******************************************************************************/
NFCSTATUS tmsNciHalWriteExt(uint16_t *pCmdLen, uint8_t *pCmdData,
                                uint16_t *pRspLen, uint8_t *pRspData) {
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    if (tmsDtaIsEnable() == true) {
        status = tmsNHalDtaUpdate(pCmdLen, pCmdData, pRspLen, pRspData);
    }

    if (pCmdData[0] == PROPRIETARY_CMD_FELICA_READER_MODE &&
            pCmdData[1] == PROPRIETARY_CMD_FELICA_READER_MODE &&
            pCmdData[2] == PROPRIETARY_CMD_FELICA_READER_MODE) {
        TMSLOG_NCIHAL_D("Received proprietary command to set Felica Reader mode:%d",
                        pCmdData[3]);
        gFelicaReaderMode = pCmdData[3];
        /* frame the dummy response */
        *pRspLen = 4;
        pRspData[0] = 0x00;
        pRspData[1] = 0x00;
        pRspData[2] = 0x00;
        pRspData[3] = 0x00;
        status = NFCSTATUS_FAILED;
    }

    if ((*pCmdLen >= 6) &&
               (pCmdData[3] == 0x81 && pCmdData[4] == 0x01 &&
                pCmdData[5] == 0x03)) {
        TMSLOG_NCIHAL_D("> Going through the set host list");
        if ((*getNfcFL()).chipType >= thn31) {
            *pCmdLen = 10;

            pCmdData[2] = 0x07;

            pCmdData[6] = 0x02;
            pCmdData[7] = 0x80;
            pCmdData[8] = 0x81;
            pCmdData[9] = 0xC0;
        } else {
            *pCmdLen = 8;

            pCmdData[2] = 0x05;
            pCmdData[6] = 0x02;
            pCmdData[7] = 0xC0;
        }
        status = NFCSTATUS_SUCCESS;
    } else if (gIcodeDetected) {
        if (pCmdData[3] == 0x20 || pCmdData[3] == 0x24 ||
                pCmdData[3] == 0x60) {
            TMSLOG_NCIHAL_D("> NFC ISO_15693 Proprietary CMD ");
            pCmdData[3] += 0x02;
        }
    } else if (pCmdData[0] == 0x21 && pCmdData[1] == 0x03) {
        TMSLOG_NCIHAL_D("> Polling Loop Started");
        gIcodeDetected = 0;
    }
    // 22000100
    else if (pCmdData[0] == 0x22 && pCmdData[1] == 0x00 &&
             pCmdData[2] == 0x01 && pCmdData[3] == 0x00) {
        // gEeDiscDone = 0x01;//Reader Over SWP event getting
        *pRspLen = 0x05;
        pRspData[0] = 0x42;
        pRspData[1] = 0x00;
        pRspData[2] = 0x02;
        pRspData[3] = 0x00;
        pRspData[4] = 0x00;
        tmsNciHalPrintPacket("RECV", pRspData, 5);
        status = NFCSTATUS_FAILED;
    }
    // 2002 0904 3000 3100 3200 5000
    else if ((pCmdData[0] == 0x20 && pCmdData[1] == 0x02) &&
             ((pCmdData[2] == 0x09 && pCmdData[3] == 0x04) /*||
            (pCmdData[2] == 0x0D && pCmdData[3] == 0x04)*/
             )) {
        *pCmdLen += 0x01;
        pCmdData[2] += 0x01;
        pCmdData[9] = 0x01;
        pCmdData[10] = 0x40;
        pCmdData[11] = 0x50;
        pCmdData[12] = 0x00;

        TMSLOG_NCIHAL_D("> Going through workaround - Dirty Set Config ");
        //        tmsNciHalPrintPacket("SEND", pCmdData, *pCmdLen);
        TMSLOG_NCIHAL_D("> Going through workaround - Dirty Set Config - End ");
    }
    //    20020703300031003200
    //    2002 0301 3200
    else if ((pCmdData[0] == 0x20 && pCmdData[1] == 0x02) &&
             ((pCmdData[2] == 0x07 && pCmdData[3] == 0x03) ||
              (pCmdData[2] == 0x03 && pCmdData[3] == 0x01 &&
               pCmdData[4] == 0x32))) {
        TMSLOG_NCIHAL_D("> Going through workaround - Dirty Set Config ");
        tmsNciHalPrintPacket("SEND", pCmdData, *pCmdLen);
        *pRspLen = 5;
        pRspData[0] = 0x40;
        pRspData[1] = 0x02;
        pRspData[2] = 0x02;
        pRspData[3] = 0x00;
        pRspData[4] = 0x00;

        tmsNciHalPrintPacket("RECV", pRspData, 5);
        status = NFCSTATUS_FAILED;
        TMSLOG_NCIHAL_D("> Going through workaround - Dirty Set Config - End ");
    }

    // 2002 0D04 300104 310100 320100 500100
    // 2002 0401 320100
    else if ((pCmdData[0] == 0x20 && pCmdData[1] == 0x02) &&
             (
                 /*(pCmdData[2] == 0x0D && pCmdData[3] == 0x04)*/
                 (pCmdData[2] == 0x04 && pCmdData[3] == 0x01 &&
                  pCmdData[4] == 0x32 && pCmdData[5] == 0x00))) {
        //        pCmdData[12] = 0x40;

        TMSLOG_NCIHAL_D("> Going through workaround - Dirty Set Config ");
        tmsNciHalPrintPacket("SEND", pCmdData, *pCmdLen);
        pCmdData[6] = 0x60;

        tmsNciHalPrintPacket("RECV", pRspData, 5);
        //        status = NFCSTATUS_FAILED;
        TMSLOG_NCIHAL_D("> Going through workaround - Dirty Set Config - End ");
    }

    return status;
}

/******************************************************************************
 * Function         tmsNciHalSendExtCmd
 *
 * Description      This function send the extension command to NFCC. No
 *                  response is checked by this function but it waits for
 *                  the response to come.
 *
 * Returns          Returns NFCSTATUS_SUCCESS if sending cmd is successful and
 *                  response is received.
 *
 ******************************************************************************/
NFCSTATUS tmsNciHalSendExtCmd(uint16_t cmdLen, uint8_t *pCmd) {
    NFCSTATUS status = NFCSTATUS_FAILED;
    (*getTmsNciHalCtrl()).cmdLen = cmdLen;
    memcpy((*getTmsNciHalCtrl()).cmdData, pCmd, cmdLen);
    status = tmsNciHalProcessExtCmdRsp((*getTmsNciHalCtrl()).cmdLen,
             (*getTmsNciHalCtrl()).cmdData);

    return status;
}

/******************************************************************************
 * Function         halExtnsWriteRspTimeoutCb
 *
 * Description      Timer call back function
 *
 * Returns          None
 *
 ******************************************************************************/
static void halExtnsWriteRspTimeoutCb(uint32_t timerId, void *pContext) {
    UNUSED_PROP(timerId);
    UNUSED_PROP(pContext);
    TMSLOG_NCIHAL_D("halExtnsWriteRspTimeoutCb - write timeOut!!!");
    (*getTmsNciHalCtrl()).extCbData.status = NFCSTATUS_FAILED;
    usleep(1);
    sem_post(&((*getTmsNciHalCtrl()).syncSpiNfc));
    SEM_POST(&((*getTmsNciHalCtrl()).extCbData));

    return;
}

