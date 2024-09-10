/******************************************************************************
 *
 *  Copyright 2010-2018, 2021 NXP
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
 ******************************************************************************/

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

/*
 *  OSAL header files related to memory, debug, random, semaphore and mutex
 * functions.
 */

#ifndef PHNFCCOMMON_H
#define PHNFCCOMMON_H

/*
************************* Include Files ****************************************
*/

#include <Dal4Nfc_messageQueueLib.h>
#include <NfcStatus.h>
#include <OsalNfc_Timer.h>
#include <pthread.h>
#include <semaphore.h>

/*
 *  Component IDs
 *
 *  IDs for all NFC components. Combined with the Status Code they build the
 * value (status)
 *  returned by each function.
 *
 *  ID Number Spaces:
 *  - 01..1F: HAL
 *  - 20..3F: NFC-MW (Local Device)
 *  - 40..5F: NFC-MW (Remote Device)
 *  .
 *
 *         The value CID_NFC_NONE does not exist for Component IDs. Do not use
 * this value except
 *         for NFCSTATUS_SUCCESS. The enumeration function uses CID_NFC_NONE
 *         to mark unassigned "References".
 */
/* Unassigned or doesn't apply (see #NFCSTATUS_SUCCESS) */
#define CID_NFC_NONE 0x00
#define CID_NFC_TML 0x01 /* Transport Mapping Layer */
/* Operating System Abstraction Layer*/
#define CID_NFC_OSAL CID_NFC_NONE

/*
 *  information to configure OSAL
 */
typedef struct OsalNfcConfig {
    uint8_t *pLogFile;            /* Log File Name*/
    uintptr_t callbackThreadId; /* Client ID to which message is posted */
} OsalNfcConfig_t, *pOsalNfcConfig_t /* Pointer to #OsalNfcConfig_t */;

/*
 * Deferred call declaration.
 * This type of API is called from ClientApplication (main thread) to notify
 * specific callback.
 */
typedef void (*pOsalNfcDeferFuncPointer_t)(void *);

/*
 * Deferred message specific info declaration.
 */
typedef struct OsalNfcDeferedCallInfo {
    pOsalNfcDeferFuncPointer_t pDeferedCall; /* pointer to Deferred callback */
    void *pParam; /* contains timer message specific details*/
} OsalNfcDeferedCallInfo_t;

/*
 * States in which a OSAL timer exist.
 */
typedef enum OsalNfcTimerStates {
    timerIdle = 0,          /* Indicates Initial state of timer */
    timerRunning = 1,       /* Indicate timer state when started */
    timerStopped = 2        /* Indicates timer state when stopped */
} OsalNfcTimerStates_t; /* Variable representing State of timer */

/*
 **Timer Handle structure containing details of a timer.
 */
typedef struct OsalNfcTimerHandle {
    uint32_t timerId;     /* ID of the timer */
    timer_t timerHandle; /* Handle of the timer */
    /* Timer callback function to be invoked */
    pOsalNfcTimerCallbck_t pApplicationCallback;
    void *pContext; /* Parameter to be passed to the callback function */
    OsalNfcTimerStates_t state; /* Timer states */
    /* Osal Timer message posted on User Thread */
    NciHalMessage_t OsalMessage;
    /* Deferred Call structure to Invoke Callback function */
    OsalNfcDeferedCallInfo_t deferedCallInfo;
    /* Variables for Structure Instance and Structure Ptr */
} OsalNfcTimerHandle_t, *pOsalNfcTimerHandle_t;

#endif /*  PHOSALNFC_H  */
