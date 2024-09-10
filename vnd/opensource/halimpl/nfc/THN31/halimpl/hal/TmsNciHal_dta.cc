/*
* Copyright 2012-2014, 2021 NXP
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

#include <TmsConfig.h>
#include <TmsLog.h>
#include <TmsNciHal.h>
#include <TmsNciHal_dta.h>

/*********************** Global Variables *************************************/
static TmsDtaControl_t gTmsDtaCtrl = {0, 0, 0};
/*******************************************************************************
**
** Function         tmsEnableDtaMode
**
** Description      This function configures
**                  HAL in DTA mode
**
*******************************************************************************/
void tmsEnableDtaMode(uint16_t patternNo) {
    gTmsDtaCtrl.dtaCtrlFlag = false;
    gTmsDtaCtrl.dtaT1tFlag = false;
    gTmsDtaCtrl.dtaPatternNo = patternNo;
    TMSLOG_NCIHAL_D(">>>>DTA - Mode is enabled");
    gTmsDtaCtrl.dtaCtrlFlag = true;
}

/*******************************************************************************
**
** Function         tmsDisableDtaMode
**
** Description      This function disable DTA mode
**
*******************************************************************************/
void tmsDisableDtaMode(void) {
    gTmsDtaCtrl.dtaCtrlFlag = false;
    gTmsDtaCtrl.dtaT1tFlag = false;
    TMSLOG_NCIHAL_D(">>>>DTA - Mode is Disabled");
}

/******************************************************************************
 * Function         tmsDtaIsEnable
 *
 * Description      This function checks the DTA mode is enable or not.
 *
 * Returns          It returns TRUE if DTA enabled otherwise FALSE
 *
 ******************************************************************************/
NFCSTATUS tmsDtaIsEnable(void) {
    return gTmsDtaCtrl.dtaCtrlFlag;
}

/******************************************************************************
 * Function         tmsDtaT1TEnable
 *
 * Description      This function  enables  DTA mode for T1T tag.
 *
 *
 ******************************************************************************/
void tmsDtaT1TEnable(void) {
    gTmsDtaCtrl.dtaT1tFlag = true;
}
/******************************************************************************
 * Function         tmsNHalDtaUpdate
 *
 * Description      This function changes the command and responses specific
 *                  to make DTA application success
 *
 * Returns          It return NFCSTATUS_SUCCESS then continue with send else
 *                  sends NFCSTATUS_FAILED direct response is prepared and
 *                  do not send anything to NFCC.
 *
 ******************************************************************************/

NFCSTATUS tmsNHalDtaUpdate(uint16_t *pCmdLen, uint8_t *pCmdData,
                              uint16_t *pRspLen, uint8_t *pRspData) {
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    if (gTmsDtaCtrl.dtaCtrlFlag == true) {
        // Workaround for DTA, block the set config command with general bytes */
        if (pCmdData[0] == 0x20 && pCmdData[1] == 0x02 &&
                pCmdData[2] == 0x17 && pCmdData[3] == 0x01 &&
                pCmdData[4] == 0x29 && pCmdData[5] == 0x14) {
            *pRspLen = 5;
            TMSLOG_NCIHAL_D(">>>>DTA - Block set config command");
            tmsNciHalPrintPacket("DTASEND", pCmdData, *pCmdLen);

            pRspData[0] = 0x40;
            pRspData[1] = 0x02;
            pRspData[2] = 0x02;
            pRspData[3] = 0x00;
            pRspData[4] = 0x00;

            tmsNciHalPrintPacket("DTARECV", pRspData, 5);

            status = NFCSTATUS_FAILED;
            TMSLOG_NCIHAL_D(
                "Going through DTA workaround - Block set config command END");

        } else if (pCmdData[0] == 0x21 && pCmdData[1] == 0x08 &&
                   pCmdData[2] == 0x04 && pCmdData[3] == 0xFF &&
                   pCmdData[4] == 0xFF) {
            TMSLOG_NCIHAL_D(">>>>DTA Change Felica system code");
            *pRspLen = 4;
            pRspData[0] = 0x41;
            pRspData[1] = 0x08;
            pRspData[2] = 0x01;
            pRspData[3] = 0x00;
            status = NFCSTATUS_FAILED;

            tmsNciHalPrintPacket("DTARECV", pRspData, 4);
        } else if (pCmdData[0] == 0x20 && pCmdData[1] == 0x02 &&
                   pCmdData[2] == 0x10 && pCmdData[3] == 0x05 &&
                   pCmdData[10] == 0x32 && pCmdData[12] == 0x00) {
            TMSLOG_NCIHAL_D(">>>>DTA Update LA_SEL_INFO param");

            pCmdData[12] = 0x40;
            pCmdData[18] = 0x02;
            status = NFCSTATUS_SUCCESS;
        } else if (pCmdData[0] == 0x20 && pCmdData[1] == 0x02 &&
                   pCmdData[2] == 0x0D && pCmdData[3] == 0x04 &&
                   pCmdData[10] == 0x32 && pCmdData[12] == 0x00) {
            TMSLOG_NCIHAL_D(">>>>DTA Blocking dirty set config");
            *pRspLen = 5;
            pRspData[0] = 0x40;
            pRspData[1] = 0x02;
            pRspData[2] = 0x02;
            pRspData[3] = 0x00;
            pRspData[4] = 0x00;
            status = NFCSTATUS_FAILED;
            tmsNciHalPrintPacket("DTARECV", pRspData, 5);
        } else if (pCmdData[0] == 0x21 && pCmdData[1] == 0x03) {
            if (*pCmdLen > (NCI_MAX_DATA_LEN - 6)) {
                android_errorWriteLog(0x534e4554, "183487770");
                return NFCSTATUS_FAILED;
            }
            TMSLOG_NCIHAL_D(">>>>DTA Add NFC-F listen tech params");
            pCmdData[2] += 6;
            pCmdData[3] += 3;
            pCmdData[*pCmdLen] = 0x80;
            pCmdData[*pCmdLen + 1] = 0x01;
            pCmdData[*pCmdLen + 2] = 0x82;
            pCmdData[*pCmdLen + 3] = 0x01;
            pCmdData[*pCmdLen + 4] = 0x85;
            pCmdData[*pCmdLen + 5] = 0x01;

            *pCmdLen += 6;
            status = NFCSTATUS_SUCCESS;
        } else if (pCmdData[0] == 0x20 && pCmdData[1] == 0x02 &&
                   pCmdData[2] == 0x0D && pCmdData[3] == 0x04 &&
                   pCmdData[10] == 0x32 && pCmdData[12] == 0x20 &&
                   gTmsDtaCtrl.dtaPatternNo == 0x1000) {
            TMSLOG_NCIHAL_D(">>>>DTA Blocking dirty set config for analog testing");
            *pRspLen = 5;
            pRspData[0] = 0x40;
            pRspData[1] = 0x02;
            pRspData[2] = 0x02;
            pRspData[3] = 0x00;
            pRspData[4] = 0x00;
            status = NFCSTATUS_FAILED;
            tmsNciHalPrintPacket("DTARECV", pRspData, 5);
        } else if (pCmdData[0] == 0x20 && pCmdData[1] == 0x02 &&
                   pCmdData[2] == 0x0D && pCmdData[3] == 0x04 &&
                   pCmdData[4] == 0x32 && pCmdData[5] == 0x01 &&
                   pCmdData[6] == 0x00) {
            TMSLOG_NCIHAL_D(">>>>DTA Blocking dirty set config");
            *pRspLen = 5;
            pRspData[0] = 0x40;
            pRspData[1] = 0x02;
            pRspData[2] = 0x02;
            pRspData[3] = 0x00;
            pRspData[4] = 0x00;
            status = NFCSTATUS_FAILED;
            tmsNciHalPrintPacket("DTARECV", pRspData, 5);
        } else if (pCmdData[0] == 0x20 && pCmdData[1] == 0x02 &&
                   pCmdData[2] == 0x04 && pCmdData[3] == 0x01 &&
                   pCmdData[4] == 0x50 && pCmdData[5] == 0x01 &&
                   pCmdData[6] == 0x00 && gTmsDtaCtrl.dtaPatternNo == 0x1000) {
            TMSLOG_NCIHAL_D(">>>>DTA Blocking dirty set config for analog testing");
            *pRspLen = 5;
            pRspData[0] = 0x40;
            pRspData[1] = 0x02;
            pRspData[2] = 0x02;
            pRspData[3] = 0x00;
            pRspData[4] = 0x00;
            status = NFCSTATUS_FAILED;
            tmsNciHalPrintPacket("DTARECV", pRspData, 5);
        } else {
        }
        if (gTmsDtaCtrl.dtaT1tFlag == true) {
            if (pCmdData[2] == 0x07 && pCmdData[3] == 0x78 &&
                    pCmdData[4] == 0x00 && pCmdData[5] == 0x00) {
                /*if (gTmsDtaCtrl.dtaPatternNo == 0)
                {
                  TMSLOG_NCIHAL_D(">>>>DTA - T1T modification block RID command Custom
                Response (pattern 0)");
                  tmsNciHalPrintPacket("DTASEND", pCmdData, *pCmdLen);
                  *pRspLen = 10;
                  pRspData[0] = 0x00;
                  pRspData[1] = 0x00;
                  pRspData[2] = 0x07;
                  pRspData[3] = 0x12;
                  pRspData[4] = 0x49;
                  pRspData[5] = 0x00;
                  pRspData[6] = 0x00;
                  pRspData[7] = 0x00;
                  pRspData[8] = 0x00;
                  pRspData[9] = 0x00;

                  status = NFCSTATUS_FAILED;

                  tmsNciHalPrintPacket("DTARECV", pRspData, *pRspLen);
                }
                else
                {*/
                TMSLOG_NCIHAL_D("Change RID command's UID echo bytes to 0");

                gTmsDtaCtrl.dtaT1tFlag = false;
                pCmdData[6] = 0x00;
                pCmdData[7] = 0x00;
                pCmdData[8] = 0x00;
                pCmdData[9] = 0x00;
                status = NFCSTATUS_SUCCESS;
                /*}*/
            }
        }
    }
    return status;
}
