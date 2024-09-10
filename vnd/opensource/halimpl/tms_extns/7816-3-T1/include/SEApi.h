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

#ifndef TMS_7816_3_T1_SEAPI_H
#define TMS_7816_3_T1_SEAPI_H

#include <stdint.h>
#include <stdbool.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ESE_MODE_NORMAL = 0, // All wired transaction
    ESE_MODE_NFCC_DL,    // nfcc FW/BL download used
    ESE_MODE_ESE_DL,     // eSE COS download used
    ESE_MODE_ESE_PTH_DL, // eSE COS patch download used
} SeInitMode;

/**
 * @Function    seOpen
 *
 * @Description This function open the physical, SPI or I2C, device driver.
 *
 * @params      initMode - init mode for normal OMA or download mode
 *
 * @returns     This function return ESESTATUS_SUCCES (0) in case of success.
 *
 */
ESESTATUS seOpen(SeInitMode initMode);

/**
 * @Function     SeIsOpened
 *
 * @Description  This function checks if the hal is opened.
 *
 * @returns      false if it is close, otherwise true
 *
 */
bool SeIsOpened();

/**
 * @Function     SeIsInitialized
 *
 * @Description  This function checks if the SE is initialized.
 *
 * @returns      true if PCB=0xC4 transmit succesfully, otherwise false
 *
 */
bool SeIsInitialized();

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
ESESTATUS seInit(SeInitMode initMode);

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
ESESTATUS seTransceive(SeData *pCmd, SeData *pRsp);

/**
 * @Function seDeInit
 *
 * @Description  This function deinitializes the ESE interface and free all resources.
 *
 * @returns      ESESTATUS_SUCCESS Always return ESESTATUS_SUCCESS (0).
 *
 */
ESESTATUS seDeInit();

/**
 * @Function    seClose
 *
 * @Description This function close the physical, SPI or I2C, device driver,
 *              and release resources.
 *
 * @returns     This function return ESESTATUS_SUCCES (0) in case of success.
 *
 */
ESESTATUS seClose();

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
ESESTATUS seGetATR(SeData *pRsp);

/**
 * @Function seReset
 *
 * @Description  This function reset the SE, such as: N(S), chain flag, etc.
 *
 * @returns      ESESTATUS_SUCCESS is successful
 *
 */
ESESTATUS seReset();

/******************************************************************************
 * @Function     DoReadTerminate
 *
 * @Description  set isReadDone to true, T=1 read SOF will be terminated tryagain
 *
******************************************************************************/
void DoReadTerminate();

SEContext *GetEseCtx(void);

uint16_t GetDataRxLen(void);

uint8_t *GetDataRx(void);

SeInitMode GetInitMode(void);
#ifdef __cplusplus
}
#endif

#endif // TMS_7816_3_T1_SEAPI_H