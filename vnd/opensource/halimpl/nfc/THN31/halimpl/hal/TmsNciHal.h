/*
 * Copyright 2010-2021 NXP
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

#ifndef PHTMSNCIHAL_H
#define PHTMSNCIHAL_H

#include "TmsNfcCapability.h"
#include <hardware/nfc.h>
#include <TmsNciHal_utils.h>
#include "TmsNciHal_IoctlOperations.h"


/********************* Definitions and structures *****************************/
#define MAX_RETRY_COUNT 5
#define NCI_MAX_DATA_LEN 300
#define NCI_VERSION_2_0 0x20

/*Mem alloc with 8 byte alignment*/
#define SIZE_ALIGN(sz) ((((sz)-1) | 7) + 1)
#define TMS_MALLOC(size) malloc(SIZE_ALIGN((size)))

typedef void(tmsNciHalControlGrantedCallback_t)();

/*ROM CODE VERSION FW*/
#define NCI_CMDRESP_MAX_BUFF_SIZE_THN31  (0x22AU)
#define FW_DBG_REASON_AVAILABLE     (0xA3)

/* NCI Data */
#define CORE_RESET_TRIGGER_TYPE_CORE_RESET_CMD_RECEIVED 0x02
#define CORE_RESET_TRIGGER_TYPE_POWERED_ON              0x01
#define NCI2_0_CORE_RESET_TRIGGER_TYPE_OVER_TEMPERATURE ((uint8_t)0xA1)
#define CORE_RESET_TRIGGER_TYPE_UNRECOVERABLE_ERROR 0x00
#define CORE_RESET_TRIGGER_TYPE_FW_ASSERT ((uint8_t)0xA0)
#define CORE_RESET_TRIGGER_TYPE_WATCHDOG_RESET ((uint8_t)0xA3)
#define CORE_RESET_TRIGGER_TYPE_INPUT_CLOCK_LOST ((uint8_t)0xA4)
#define NCI_MT_MASK                  0xE0
#define NCI_OID_MASK                 0x3F
/* GID: Group Identifier (byte 0) */
#define NCI_GID_MASK                 0x0F
#define ORIG_TMSHAL 0x01
#define ORIG_LIBNFC 0x02
#define TMS_PROPCMD_GID              0x2F
#define TMS_FLUSH_SRAM_AO_TO_FLASH   0x21
#define TMS_CORE_SET_CONFIG_CMD      0x02
#define NCI_HEADER_SIZE 3

/* Lx_DEBUG_CFG_MASK,confiure for TMS_CORE_PROP_SYSTEM_DEBUG*/
#define LX_DEBUG_CFG_MASK 0x00FF

#ifdef TMS_NFC
/* Callback event. Make sure it's equal to what the libtmsnfc-nci defines. */
enum {
    NCI_HAL_INIT_FAILED_EVT = 101u,
};
#endif

typedef struct NciData {
    uint16_t len;
    uint8_t pData[NCI_MAX_DATA_LEN];
} NciData_t;

typedef enum {
    HAL_STATUS_CLOSE = 0,
    HAL_STATUS_OPEN,
    HAL_STATUS_MIN_OPEN
} TmsNciHalStatus;

typedef struct TmsNciHalInfo {
    uint8_t   nciVersion;
    bool_t    waitForNtf;
    uint8_t   lastResetNtfReason;
} TmsNciHalInfo_t;
/* NCI Control structure */
typedef struct TmsNciHalControl {
    TmsNciHalStatus halStatus; /* Indicate if hal is open or closed */
    pthread_t clientThread;      /* Integration thread handle */
    uint8_t threadRunning;       /* Thread running if set to 1, else set to 0 */
    NciHalSConfig_t drvCfg;   /* Driver config data */

    /* Rx data */
    uint8_t *pRxData;
    uint16_t rxDataLen;

    /* Rx data */
    uint8_t *pRxEseData;
    uint16_t rxEseDataLen;

    /* libnfc-nci callbacks */
    nfc_stack_callback_t *pNfcStackCallback;
    nfc_stack_data_callback_t *pNfcStackDataCallback;

    /* control granted callback */
    tmsNciHalControlGrantedCallback_t *pControlGrantedCallback;

    /* HAL open status */
    bool_t halOpenStatus;

    /* HAL extensions */
    uint8_t halExtEnabled;

    /* Waiting semaphore */
    TmsNciHalSem_t extCbData;
    sem_t syncSpiNfc;

    uint16_t cmdLen;
    uint8_t cmdData[NCI_MAX_DATA_LEN];
    uint16_t rspLen;
    uint8_t rspData[NCI_MAX_DATA_LEN];

    /* retry count used to force download */
    uint16_t retryCnt;
    uint8_t readRetryCnt;
    TmsNciHalInfo_t nciInfo;
    uint8_t halBootMode;
    NfcChipType chipType;

    bool readyToShutdown;
} TmsNciHalControl_t;

TmsNciHalControl_t *getTmsNciHalCtrl(void);
uint32_t *getFwVerRsp(void);
uint32_t *getFwVer(void);
uint32_t *getTimeoutTimerId(void);
nfc_stack_callback_t **getNfcStackCallbackBackup(void);

/* Macros to enable and disable extensions */
#define HAL_ENABLE_EXT() ((*getTmsNciHalCtrl()).halExtEnabled = 1)
#define HAL_DISABLE_EXT() ((*getTmsNciHalCtrl()).halExtEnabled = 0)

enum {
    SE_TYPE_ESE,
    SE_TYPE_UICC,
    SE_TYPE_UICC2,
    NUM_SE_TYPES
};

/* Internal messages to handle callbacks */
#define NCI_HAL_OPEN_CPLT_MSG 0x411
#define NCI_HAL_CLOSE_CPLT_MSG 0x412
#define NCI_HAL_POST_INIT_CPLT_MSG 0x413
#define NCI_HAL_PRE_DISCOVER_CPLT_MSG 0x414
#define NCI_HAL_ERROR_MSG 0x415
#define NCI_HAL_HCI_NETWORK_RESET_MSG 0x416
#define NCI_HAL_RX_MSG 0xF01
#ifdef TMS_NFC
/* Hal init failed msg to handle callbacks. This value is defined to avoid conflicts in the new android. */
#define NCI_HAL_INIT_FAILED_MSG 0x1001
#endif
#define HAL_NFC_FW_UPDATE_STATUS_EVT 0x0A


/******************** NCI HAL exposed functions *******************************/
int tmsNciHalCheckNciCmdWriteWindow(uint16_t cmdLen, uint8_t *pCmd);
NFCSTATUS tmsNciHalWriteUnlocked(uint16_t dataLen, const uint8_t *pData,
                                 int origin);
NFCSTATUS tmsNciHalNfccCoreResetInit(bool keepConfig = false);

/*******************************************************************************
**
** Function         tmsNciHalConfigFeatureList
**
** Description      Configures the featureList based on chip type
**                  HW Version information number will provide chipType.
**                  HW Version can be obtained from CORE_INIT_RESPONSE(NCI 1.0)
**                  or CORE_RST_NTF(NCI 2.0)
**
** Parameters       CORE_INIT_RESPONSE/CORE_RST_NTF, len
**
** Returns          none
*******************************************************************************/
void tmsNciHalConfigFeatureList(uint8_t *pInitRsp, uint16_t rspLen);

#endif /* PHTMSNCIHAL_H */
