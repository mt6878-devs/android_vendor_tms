/*
 * Copyright 2010-2014, 2020 NXP
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

/*
 * OSAL Implementation for Timers.
 */

#include <NfcCommon.h>
#include <NfcTypes.h>
#include <TmsLog.h>
#include <TmsNciHal.h>
#include <OsalNfc_Timer.h>
#include <signal.h>

#define PH_NFC_MAX_TIMER (5U)
static OsalNfcTimerHandle_t sgTimerInfo[PH_NFC_MAX_TIMER];

/*
 * Defines the base address for generating timerid.
 */
#define PH_NFC_TIMER_BASE_ADDRESS (100U)

/*
 *  Defines the value for invalid timerid returned during timeSetEvent
 */
#define PH_NFC_TIMER_ID_ZERO (0x00)

/*
 * Invalid timer ID type. This ID used indicate timer creation is failed */
#define PH_NFC_TIMER_ID_INVALID (0xFFFF)

/* Forward declarations */
static void osalNfcPostTimerMsg(NciHalMessage_t *pMsg);
static void osalNfcDeferredCall(void *pParams);
static void osalNfcTimerExpired(union sigval sv);

/*
 *************************** Function Definitions ******************************
 */

/*******************************************************************************
**
** Function         osalNfcTimerCreate
**
** Description      Creates a timer which shall call back the specified function
**                  when the timer expires. Fails if OSAL module is not
**                  initialized or timers are already occupied
**
** Parameters       None
**
** Returns          timerId
**                  timerId value of PH_OSALNFC_TIMER_ID_INVALID indicates that
**                  timer is not created
**
*******************************************************************************/
uint32_t osalNfcTimerCreate(void) {
    /* timerId is also used as an index at which timer object can be stored */
    uint32_t timerId = PH_OSALNFC_TIMER_ID_INVALID;
    static struct sigevent se;
    OsalNfcTimerHandle_t *pTimerHandle;
    /* Timer needs to be initialized for timer usage */

    se.sigev_notify = SIGEV_THREAD;
    se.sigev_notify_function = osalNfcTimerExpired;
    se.sigev_notify_attributes = NULL;
    timerId = utilNfcCheckForAvailableTimer();

    /* Check whether timers are available, if yes create a timer handle structure
     */
    if ((PH_NFC_TIMER_ID_ZERO != timerId) && (timerId <= PH_NFC_MAX_TIMER)) {
        pTimerHandle = (OsalNfcTimerHandle_t *)&sgTimerInfo[timerId - 1];
        /* Build the Timer Id to be returned to Caller Function */
        timerId += PH_NFC_TIMER_BASE_ADDRESS;
        se.sigev_value.sival_int = (int)timerId;
        /* Create POSIX timer */
        if (timer_create(CLOCK_REALTIME, &se, &(pTimerHandle->timerHandle)) ==
                -1) {
            TMSLOG_TML_E("timer_create failed!");
            timerId = PH_NFC_TIMER_ID_INVALID;
        } else {
            /* Set the state to indicate timer is ready */
            pTimerHandle->state = timerIdle;
            /* Store the Timer Id which shall act as flag during check for timer
             * availability */
            pTimerHandle->timerId = timerId;
        }
    } else {
        TMSLOG_TML_E("cannot get availableTimer! timerId = %d.", timerId);
        timerId = PH_NFC_TIMER_ID_INVALID;
    }

    /* Timer ID invalid can be due to Uninitialized state,Non availability of
     * Timer */
    return timerId;
}

/*******************************************************************************
**
** Function         osalNfcTimerStart
**
** Description      Starts the requested, already created, timer.
**                  If the timer is already running, timer stops and restarts
**                  with the new timeOut value and new callback function in case
**                  any ??????
**                  Creates a timer which shall call back the specified function
**                  when the timer expires
**
** Parameters       timerId - valid timer ID obtained during timer creation
**                  regTimeCnt - requested timeOut in milliseconds
**                  pApplicationCallback - application callback interface to be
**                                          called when timer expires
**                  pContext - caller context, to be passed to the application
**                             callback function
**
** Returns          NFC status:
**                  NFCSTATUS_SUCCESS - the operation was successful
**                  NFCSTATUS_NOT_INITIALISED - OSAL Module is not initialized
**                  NFCSTATUS_INVALID_PARAMETER - invalid parameter passed to
**                                                the function
**                  PH_OSALNFC_TIMER_START_ERROR - timer could not be created
**                                                 due to system error
**
*******************************************************************************/
NFCSTATUS osalNfcTimerStart(uint32_t timerId, uint32_t regTimeCnt,
                                pOsalNfcTimerCallbck_t pApplicationCallback,
                                void *pContext) {
    NFCSTATUS startStatus = NFCSTATUS_SUCCESS;

    struct itimerspec its;
    uint32_t index;
    OsalNfcTimerHandle_t *pTimerHandle;
    /* Retrieve the index at which the timer handle structure is stored */
    index = timerId - PH_NFC_TIMER_BASE_ADDRESS - 0x01;
    pTimerHandle = (OsalNfcTimerHandle_t *)&sgTimerInfo[index];
    /* OSAL Module needs to be initialized for timer usage */
    /* Check whether the handle provided by user is valid */
    if ((index < PH_NFC_MAX_TIMER) && (0x00 != pTimerHandle->timerId) &&
            (NULL != pApplicationCallback)) {
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
        its.it_value.tv_sec = regTimeCnt / 1000;
        its.it_value.tv_nsec = 1000000 * (regTimeCnt % 1000);
        if (its.it_value.tv_sec == 0 && its.it_value.tv_nsec == 0) {
            /* This would inadvertently stop the timer*/
            its.it_value.tv_nsec = 1;
        }
        pTimerHandle->pApplicationCallback = pApplicationCallback;
        pTimerHandle->pContext = pContext;
        pTimerHandle->state = timerRunning;
        /* Arm the timer */
        if ((timer_settime(pTimerHandle->timerHandle, 0, &its, NULL)) == -1) {
            startStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_TIMER_START_ERROR);
        }
    } else {
        startStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
    }

    return startStatus;
}

/*******************************************************************************
**
** Function         osalNfcTimerStop
**
** Description      Stops already started timer
**                  Allows to stop running timer. In case timer is stopped,
**                  timer callback will not be notified any more
**
** Parameters       timerId - valid timer ID obtained during timer creation
**
** Returns          NFC status:
**                  NFCSTATUS_SUCCESS - the operation was successful
**                  NFCSTATUS_NOT_INITIALISED - OSAL Module is not initialized
**                  NFCSTATUS_INVALID_PARAMETER - invalid parameter passed to
**                                                the function
**                  PH_OSALNFC_TIMER_STOP_ERROR - timer could not be stopped due
**                                                to system error
**
*******************************************************************************/
NFCSTATUS osalNfcTimerStop(uint32_t timerId) {
    NFCSTATUS wStopStatus = NFCSTATUS_SUCCESS;
    static struct itimerspec its = {{0, 0}, {0, 0}};

    uint32_t index;
    OsalNfcTimerHandle_t *pTimerHandle;
    index = timerId - PH_NFC_TIMER_BASE_ADDRESS - 0x01;
    pTimerHandle = (OsalNfcTimerHandle_t *)&sgTimerInfo[index];
    /* OSAL Module and Timer needs to be initialized for timer usage */
    /* Check whether the timerId provided by user is valid */
    if ((index < PH_NFC_MAX_TIMER) && (0x00 != pTimerHandle->timerId) &&
            (pTimerHandle->state != timerIdle)) {
        /* Stop the timer only if the callback has not been invoked */
        if (pTimerHandle->state == timerRunning) {
            if ((timer_settime(pTimerHandle->timerHandle, 0, &its, NULL)) == -1) {
                wStopStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_TIMER_STOP_ERROR);
            } else {
                /* Change the state of timer to Stopped */
                pTimerHandle->state = timerStopped;
            }
        }
    } else {
        wStopStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
    }

    return wStopStatus;
}

/*******************************************************************************
**
** Function         osalNfcTimerDelete
**
** Description      Deletes previously created timer
**                  Allows to delete previously created timer. In case timer is
**                  running, it is first stopped and then deleted
**
** Parameters       timerId - valid timer ID obtained during timer creation
**
** Returns          NFC status:
**                  NFCSTATUS_SUCCESS - the operation was successful
**                  NFCSTATUS_NOT_INITIALISED - OSAL Module is not initialized
**                  NFCSTATUS_INVALID_PARAMETER - invalid parameter passed to
**                                                the function
**                  PH_OSALNFC_TIMER_DELETE_ERROR - timer could not be stopped
**                                                  due to system error
**
*******************************************************************************/
NFCSTATUS osalNfcTimerDelete(uint32_t timerId) {
    NFCSTATUS deleteStatus = NFCSTATUS_SUCCESS;

    uint32_t index;
    OsalNfcTimerHandle_t *pTimerHandle;
    index = timerId - PH_NFC_TIMER_BASE_ADDRESS - 0x01;
    pTimerHandle = (OsalNfcTimerHandle_t *)&sgTimerInfo[index];
    /* OSAL Module and Timer needs to be initialized for timer usage */

    /* Check whether the timerId passed by user is valid and Deregistering of
     * timer is successful */
    if ((index < PH_NFC_MAX_TIMER) && (0x00 != pTimerHandle->timerId) &&
            (NFCSTATUS_SUCCESS == OsalNfcCheckTimerPresence(pTimerHandle))) {
        /* Cancel the timer before deleting */
        if (timer_delete(pTimerHandle->timerHandle) == -1) {
            deleteStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_TIMER_DELETE_ERROR);
        }
        /* Clear Timer structure used to store timer related data */
        memset(pTimerHandle, (uint8_t)0x00, sizeof(OsalNfcTimerHandle_t));
    } else {
        deleteStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
    }
    return deleteStatus;
}

/*******************************************************************************
**
** Function         osalNfcTimerCleanup
**
** Description      Deletes all previously created timers
**                  Allows to delete previously created timers. In case timer is
**                  running, it is first stopped and then deleted
**
** Parameters       None
**
** Returns          None
**
*******************************************************************************/
void osalNfcTimerCleanup(void) {
    /* Delete all timers */
    uint32_t index;
    OsalNfcTimerHandle_t *pTimerHandle;
    for (index = 0; index < PH_NFC_MAX_TIMER; index++) {
        pTimerHandle = (OsalNfcTimerHandle_t *)&sgTimerInfo[index];
        /* OSAL Module and Timer needs to be initialized for timer usage */

        /* Check whether the timerId passed by user is valid and Deregistering of
         * timer is successful */
        if ((0x00 != pTimerHandle->timerId) &&
                (NFCSTATUS_SUCCESS == OsalNfcCheckTimerPresence(pTimerHandle))) {
            /* Cancel the timer before deleting */
            if (timer_delete(pTimerHandle->timerHandle) == -1) {
                TMSLOG_TML_E("timer %d delete error!", index);
            }
            /* Clear Timer structure used to store timer related data */
            memset(pTimerHandle, (uint8_t)0x00, sizeof(OsalNfcTimerHandle_t));
        }
    }

    return;
}

/*******************************************************************************
**
** Function         osalNfcDeferredCall
**
** Description      Invokes the timer callback function after timer expiration.
**                  Shall invoke the callback function registered by the timer
**                  caller function
**
** Parameters       pParams - parameters indicating the ID of the timer
**
** Returns          None                -
**
*******************************************************************************/
static void osalNfcDeferredCall(void *pParams) {
    /* Retrieve the timer id from the parameter */
    unsigned long index;
    OsalNfcTimerHandle_t *pTimerHandle;
    if (NULL != pParams) {
        /* Retrieve the index at which the timer handle structure is stored */
        index = (uintptr_t)pParams - PH_NFC_TIMER_BASE_ADDRESS - 0x01;
        pTimerHandle = (OsalNfcTimerHandle_t *)&sgTimerInfo[index];
        if (pTimerHandle->pApplicationCallback != NULL) {
            /* Invoke the callback function with osal Timer ID */
            pTimerHandle->pApplicationCallback((uintptr_t)pParams,
                                               pTimerHandle->pContext);
        }
    }

    return;
}

/*******************************************************************************
**
** Function         osalNfcPostTimerMsg
**
** Description      Posts message on the user thread
**                  Shall be invoked upon expiration of a timer
**                  Shall post message on user thread through which timer
**                  callback function shall be invoked
**
** Parameters       pMsg - pointer to the message structure posted on user
**                         thread
**
** Returns          None
**
*******************************************************************************/
static void osalNfcPostTimerMsg(NciHalMessage_t *pMsg) {
    (void)dal4NfcMsgSnd(
        (*getTmsNciHalCtrl()).drvCfg
        .clientId /*gposalNfc_Context->dwCallbackThreadID*/,
        pMsg, 0);

    return;
}

/*******************************************************************************
**
** Function         osalNfcTimerExpired
**
** Description      posts message upon expiration of timer
**                  Shall be invoked when any one timer is expired
**                  Shall post message on user thread to invoke respective
**                  callback function provided by the caller of Timer function
**
** Returns          None
**
*******************************************************************************/
static void osalNfcTimerExpired(union sigval sv) {
    uint32_t index;
    OsalNfcTimerHandle_t *pTimerHandle;

    index = ((uint32_t)(sv.sival_int)) - PH_NFC_TIMER_BASE_ADDRESS - 0x01;
    pTimerHandle = (OsalNfcTimerHandle_t *)&sgTimerInfo[index];
    /* Timer is stopped when callback function is invoked */
    pTimerHandle->state = timerStopped;

    pTimerHandle->deferedCallInfo.pDeferedCall = &osalNfcDeferredCall;
    pTimerHandle->deferedCallInfo.pParam = (void *)((intptr_t)(sv.sival_int));

    pTimerHandle->OsalMessage.msgType = PH_LIBNFC_DEFERREDCALL_MSG;
    pTimerHandle->OsalMessage.pMsgData = (void *)&pTimerHandle->deferedCallInfo;

    /* Post a message on the queue to invoke the function */
    osalNfcPostTimerMsg((NciHalMessage_t *)&pTimerHandle->OsalMessage);

    return;
}

/*******************************************************************************
**
** Function         utilNfcCheckForAvailableTimer
**
** Description      Find an available timer id
**
** Parameters       void
**
** Returns          Available timer id
**
*******************************************************************************/
uint32_t utilNfcCheckForAvailableTimer(void) {
    /* Variable used to store the index at which the object structure details
       can be stored. initialize it as not available. */
    uint32_t index = 0x00;
    uint32_t retval = 0x00;

    /* Check whether Timer object can be created */
    for (index = 0x00; ((index < PH_NFC_MAX_TIMER) && (0x00 == retval));
            index++) {
        if (!(sgTimerInfo[index].timerId)) {
            retval = (index + 0x01);
        }
    }

    return (retval);
}

/*******************************************************************************
**
** Function         OsalNfcCheckTimerPresence
**
** Description      Checks the requested timer is present or not
**
** Parameters       pObjectHandle - timer context
**
** Returns          NFCSTATUS_SUCCESS if found
**                  Other value if not found
**
*******************************************************************************/
NFCSTATUS OsalNfcCheckTimerPresence(void *pObjectHandle) {
    uint32_t index;
    NFCSTATUS registerStatus = NFCSTATUS_INVALID_PARAMETER;

    for (index = 0x00;
            ((index < PH_NFC_MAX_TIMER) && (registerStatus != NFCSTATUS_SUCCESS));
            index++) {
        /* For Timer, check whether the requested handle is present or not */
        if (((&sgTimerInfo[index]) == (OsalNfcTimerHandle_t *)pObjectHandle) &&
                (sgTimerInfo[index].timerId)) {
            registerStatus = NFCSTATUS_SUCCESS;
        }
    }
    return registerStatus;
}
