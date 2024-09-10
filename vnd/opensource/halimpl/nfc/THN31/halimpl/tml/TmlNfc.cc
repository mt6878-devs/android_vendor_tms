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

/*
 * TML Implementation.
 */

#include "NfccTransportFactory.h"
#include <Dal4Nfc_messageQueueLib.h>
#include <TmsConfig.h>
#include <TmsLog.h>
#include <TmsNciHal_utils.h>
#include <OsalNfc_Timer.h>
#include <TmlNfc.h>

/*
 * Duration of Timer to wait after sending an Nci packet
 */
#define PHTMLNFC_MAXTIME_RETRANSMIT (200U)
#define MAX_WRITE_RETRY_COUNT 0x03
#define MAX_READ_RETRY_DELAY_IN_MILLISEC (150U)
/* Retry Count = Standby Recovery time of NFCC / Retransmission time + 1 */
static uint8_t gCurrentRetryCount = (2000 / PHTMLNFC_MAXTIME_RETRANSMIT) + 1;

/* Value to reset variables of TML  */
#define PH_TMLNFC_RESET_VALUE (0x00)

/* Indicates a Initial or offset value */
#define PH_TMLNFC_VALUE_ONE (0x01)

spTransport gpTransportObj;

/* initialize Context structure pointer used to access context structure */
TmlNfcContext_t *gpTmlNfcContext = NULL;
/* Local Function prototypes */
static NFCSTATUS tmlNfcStartThread(void);
static void tmlNfcReadDeferredCb(void *pParams);
static void tmlNfcWriteDeferredCb(void *pParams);
static void *tmlNfcTmlThread(void *pParam);
static void *tmlNfcTmlWriterThread(void *pParam);
static void tmlNfcReTxTimerCb(uint32_t timerId, void *pContext);
static NFCSTATUS tmlNfcInitiateTimer(void);
static void tmlNfcSignalWriteComplete(void);
static int tmlNfcWaitReadInit(void);

/* Function definitions */

TmlNfcContext_t **getTmlNfcContext(void)
{
    return &gpTmlNfcContext;
}

spTransport *getTransportObj(void)
{
    return &gpTransportObj;
}

/*******************************************************************************
**
** Function         tmlNfcInit
**
** Description      Provides initialization of TML layer and hardware interface
**                  Configures given hardware interface and sends handle to the
**                  caller
**
** Parameters       pConfig - TML configuration details as provided by the upper
**                            layer
**
** Returns          NFC status:
**                  NFCSTATUS_SUCCESS - initialization successful
**                  NFCSTATUS_INVALID_PARAMETER - at least one parameter is
**                                                invalid
**                  NFCSTATUS_FAILED - initialization failed (for example,
**                                     unable to open hardware interface)
**                  NFCSTATUS_INVALID_DEVICE - device has not been opened or has
**                                             been disconnected
**
*******************************************************************************/
NFCSTATUS tmlNfcInit(pTmlNfcConfig_t pConfig) {
    NFCSTATUS wInitStatus = NFCSTATUS_SUCCESS;

    /* Check if TML layer is already Initialized */
    if (NULL != *getTmlNfcContext()) {
        /* TML initialization is already completed */
        wInitStatus = PHNFCSTVAL(CID_NFC_TML, NFCSTATUS_ALREADY_INITIALISED);
    }
    /* Validate Input parameters */
    else if ((NULL == pConfig) ||
             (PH_TMLNFC_RESET_VALUE == pConfig->getMsgThreadId)) {
        /*Parameters passed to TML init are wrong */
        wInitStatus = PHNFCSTVAL(CID_NFC_TML, NFCSTATUS_INVALID_PARAMETER);
    } else {
        /* Allocate memory for TML context */
        *getTmlNfcContext() = (TmlNfcContext_t *)malloc(sizeof(TmlNfcContext_t));

        if (NULL == *getTmlNfcContext()) {
            wInitStatus = PHNFCSTVAL(CID_NFC_TML, NFCSTATUS_FAILED);
        } else {
            /*Configure transport layer for communication*/
            if ((*getTransportObj() == NULL) &&
                    (NFCSTATUS_SUCCESS != tmlNfcConfigTransport())) {
                return NFCSTATUS_FAILED;
            }

            if (*getIsFirstHalMinOpen()) {
                if (!(*getTransportObj())->flushdata(pConfig)) {
                    TMSLOG_NCIHAL_E("Flushdata Failed");
                }
            }
            /* Initialise all the internal TML variables */
            memset(*getTmlNfcContext(), PH_TMLNFC_RESET_VALUE,
                   sizeof(TmlNfcContext_t));
            /* Make sure that the thread runs once it is created */
            (*getTmlNfcContext())->bThreadDone = 1;
            /* Open the device file to which data is read/written */
            wInitStatus = (*getTransportObj())->i2cOpenAndConfigure(
                              pConfig, &((*getTmlNfcContext())->pDevHandle));

            if (NFCSTATUS_SUCCESS != wInitStatus) {
                wInitStatus = PHNFCSTVAL(CID_NFC_TML, NFCSTATUS_INVALID_DEVICE);
                (*getTmlNfcContext())->pDevHandle = NULL;
            } else {
                (*getTmlNfcContext())->readInfo.bEnable = 0;
                (*getTmlNfcContext())->writeInfo.bEnable = 0;
                (*getTmlNfcContext())->readInfo.bThreadBusy = false;
                (*getTmlNfcContext())->writeInfo.bThreadBusy = false;
                (*getTmlNfcContext())->fragmentLen = pConfig->fragmentLen;

                if (0 != sem_init(&(*getTmlNfcContext())->rxSemaphore, 0, 0)) {
                    wInitStatus = NFCSTATUS_FAILED;
                } else if (0 != tmlNfcWaitReadInit()) {
                    wInitStatus = NFCSTATUS_FAILED;
                } else if (0 != sem_init(&(*getTmlNfcContext())->txSemaphore, 0, 0)) {
                    wInitStatus = NFCSTATUS_FAILED;
                } else if (0 != sem_init(&(*getTmlNfcContext())->postMsgSemaphore, 0, 0)) {
                    wInitStatus = NFCSTATUS_FAILED;
                } else {
                    sem_post(&(*getTmlNfcContext())->postMsgSemaphore);
                    /* Start TML thread (to handle write and read operations) */
                    if (NFCSTATUS_SUCCESS != tmlNfcStartThread()) {
                        wInitStatus = PHNFCSTVAL(CID_NFC_TML, NFCSTATUS_FAILED);
                    } else {
                        /* Create Timer used for Retransmission of NCI packets */
                        (*getTmlNfcContext())->timerId = osalNfcTimerCreate();
                        if (PH_OSALNFC_TIMER_ID_INVALID != (*getTmlNfcContext())->timerId) {
                            /* Store the Thread Identifier to which Message is to be posted */
                            (*getTmlNfcContext())->callbackThreadId =
                                pConfig->getMsgThreadId;
                            /* Enable retransmission of Nci packet & set retry count to
                             * default */
                            (*getTmlNfcContext())->configRetrans = TMLNFC_DISABLE_RETRANS;
                            /* Retry Count = Standby Recovery time of NFCC / Retransmission
                             * time + 1 */
                            (*getTmlNfcContext())->retryCount =
                                (2000 / PHTMLNFC_MAXTIME_RETRANSMIT) + 1;
                            (*getTmlNfcContext())->writeCbInvoked = false;
                        } else {
                            wInitStatus = PHNFCSTVAL(CID_NFC_TML, NFCSTATUS_FAILED);
                        }
                    }
                }
            }
        }
    }
    /* Clean up all the TML resources if any error */
    if (NFCSTATUS_SUCCESS != wInitStatus) {
        /* Clear all handles and memory locations initialized during init */
        tmlNfcShutdownCleanUp();
    }

    return wInitStatus;
}

/*******************************************************************************
**
** Function         tmlNfcConfigTransport
**
** Description      Configure Transport channel based on transport type provided
**                  in config file
**
** Returns          NFCSTATUS_SUCCESS If transport channel is configured
**                  NFCSTATUS_FAILED If transport channel configuration failed
**
*******************************************************************************/
NFCSTATUS tmlNfcConfigTransport() {
    unsigned long transportType = UNKNOWN;
    unsigned long value = 0;
    int isFound = getTmsNumValue(NAME_TMS_TRANSPORT, &value, sizeof(value));
    if (isFound > 0) {
        transportType = value;
    }
    *getTransportObj() = TRANSPORT_FACTORY.getTransport((transportIntf)transportType);
    if (*getTransportObj() == nullptr) {
        TMSLOG_TML_E("No Transport channel available \n");
        return NFCSTATUS_FAILED;
    }
    return NFCSTATUS_SUCCESS;
}
/*******************************************************************************
**
** Function         tmlNfcConfigNciPktReTx
**
** Description      Provides Enable/Disable Retransmission of NCI packets
**                  Needed in case of Timeout between Transmission and Reception
**                  of NCI packets. Retransmission can be enabled only if
**                  standby mode is enabled
**
** Parameters       configRetrans - values from TmlNfcConfigRetrans_t
**                  retryCount - Number of times Nci packets shall be
**                                retransmitted (default = 3)
**
** Returns          None
**
*******************************************************************************/
void tmlNfcConfigNciPktReTx(TmlNfcConfigRetrans_t eConfiguration,
                               uint8_t bRetryCounter) {
    /* Enable/Disable Retransmission */

    (*getTmlNfcContext())->configRetrans = eConfiguration;
    if (TMLNFC_ENABLE_RETRANS == eConfiguration) {
        /* Check whether Retry counter passed is valid */
        if (0 != bRetryCounter) {
            (*getTmlNfcContext())->retryCount = bRetryCounter;
        }
        /* Set retry counter to its default value */
        else {
            /* Retry Count = Standby Recovery time of NFCC / Retransmission time + 1
             */
            (*getTmlNfcContext())->retryCount =
                (2000 / PHTMLNFC_MAXTIME_RETRANSMIT) + 1;
        }
    }

    return;
}

/*******************************************************************************
**
** Function         tmlNfcStartThread
**
** Description      Initializes comport, reader and writer threads
**
** Parameters       None
**
** Returns          NFC status:
**                  NFCSTATUS_SUCCESS - threads initialized successfully
**                  NFCSTATUS_FAILED - initialization failed due to system error
**
*******************************************************************************/
static NFCSTATUS tmlNfcStartThread(void) {
    NFCSTATUS startStatus = NFCSTATUS_SUCCESS;
    void *pThreadsEvent = 0x00;
    int pThreadCreateStatus = 0;

    /* Create Reader and Writer threads */
    pThreadCreateStatus =
        pthread_create(&(*getTmlNfcContext())->readerThread, NULL,
                       &tmlNfcTmlThread, (void *)pThreadsEvent);
    if (0 != pThreadCreateStatus) {
        startStatus = NFCSTATUS_FAILED;
    } else {
        /*Start Writer Thread*/
        pThreadCreateStatus =
            pthread_create(&(*getTmlNfcContext())->writerThread, NULL,
                           &tmlNfcTmlWriterThread, (void *)pThreadsEvent);
        if (0 != pThreadCreateStatus) {
            startStatus = NFCSTATUS_FAILED;
        }
    }

    return startStatus;
}

/*******************************************************************************
**
** Function         tmlNfcReTxTimerCb
**
** Description      This is the timer callback function after timer expiration.
**
** Parameters       dwThreadId  - id of the thread posting message
**                  pContext    - context provided by upper layer
**
** Returns          None
**
*******************************************************************************/
static void tmlNfcReTxTimerCb(uint32_t timerId, void *pContext) {
    if (((*getTmlNfcContext())->timerId == timerId) && (NULL == pContext)) {
        /* If Retry Count has reached its limit,Retransmit Nci
           packet */
        if (0 == gCurrentRetryCount) {
            /* Since the count has reached its limit,return from timer callback
               Upper layer Timeout would have happened */
        } else {
            gCurrentRetryCount--;
            (*getTmlNfcContext())->writeInfo.bThreadBusy = true;
            (*getTmlNfcContext())->writeInfo.bEnable = 1;
        }
        sem_post(&(*getTmlNfcContext())->txSemaphore);
    }

    return;
}

/*******************************************************************************
**
** Function         tmlNfcInitiateTimer
**
** Description      Start a timer for Tx and Rx thread.
**
** Parameters       void
**
** Returns          NFC status
**
*******************************************************************************/
static NFCSTATUS tmlNfcInitiateTimer(void) {
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    /* Start Timer once Nci packet is sent */
    status = osalNfcTimerStart((*getTmlNfcContext())->timerId,
                                    (uint32_t)PHTMLNFC_MAXTIME_RETRANSMIT,
                                    tmlNfcReTxTimerCb, NULL);

    return status;
}

/*******************************************************************************
**
** Function         tmlNfcTmlThread
**
** Description      Read the data from the lower layer driver
**
** Parameters       pParam  - parameters for Writer thread function
**
** Returns          None
**
*******************************************************************************/
static void *tmlNfcTmlThread(void *pParam) {
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    int32_t bytesRead = PH_TMLNFC_RESET_VALUE;
    uint8_t temp[260];
    uint8_t readRetryDelay = 0;
    /* Transaction info pBuffer to be passed to Callback Thread */
    static TmlNfcTransactInfo_t transactionInfo;
    /* Structure containing Tml callback function and parameters to be invoked
       by the callback thread */
    static NciHalDeferredCall_t deferredInfo;
    /* initialize Message structure to post message onto Callback Thread */
    static NciHalMessage_t msg;
    UNUSED_PROP(pParam);
    TMSLOG_TML_D("THN31 - Tml Reader Thread Started................\n");

    /* Reader thread loop shall be running till shutdown is invoked */
    while ((*getTmlNfcContext())->bThreadDone) {
        /* If Tml read is requested */
        /* Set the variable to success initially */
        status = NFCSTATUS_SUCCESS;
        if (-1 == sem_wait(&(*getTmlNfcContext())->rxSemaphore)) {
            TMSLOG_TML_E("sem_wait didn't return success \n");
        }

        /* If Tml read is requested */
        if (1 == (*getTmlNfcContext())->readInfo.bEnable) {
            TMSLOG_TML_D("THN31 - Read requested.....\n");
            /* Set the variable to success initially */
            status = NFCSTATUS_SUCCESS;

            /* Variable to fetch the actual number of bytes read */
            bytesRead = PH_TMLNFC_RESET_VALUE;

            /* Read the data from the file onto the pBuffer */
            if (NULL != (*getTmlNfcContext())->pDevHandle) {
                TMSLOG_TML_D("THN31 - Invoking I2C Read.....\n");
                bytesRead =
                    (*getTransportObj())->i2cRead((*getTmlNfcContext())->pDevHandle, temp, 260);

                if (-1 == bytesRead) {
                    TMSLOG_TML_E("THN31 - Error in I2C Read.....\n");
                    if (readRetryDelay < MAX_READ_RETRY_DELAY_IN_MILLISEC) {
                        /*sleep for 30/60/90/120/150 msec between each read trial incase of read error*/
                        readRetryDelay += 30 ;
                    }
                    usleep(readRetryDelay * 1000);
                    sem_post(&(*getTmlNfcContext())->rxSemaphore);
                } else if (bytesRead > 260) {
                    TMSLOG_TML_E("Numer of bytes read exceeds the limit 260.....\n");
                    readRetryDelay = 0;
                    sem_post(&(*getTmlNfcContext())->rxSemaphore);
                } else {
                    memcpy((*getTmlNfcContext())->readInfo.pBuffer, temp, bytesRead);
                    readRetryDelay = 0;

                    TMSLOG_TML_D("THN31 - I2C Read successful.....\n");
                    /* This has to be reset only after a successful read */
                    (*getTmlNfcContext())->readInfo.bEnable = 0;
                    if ((TMLNFC_ENABLE_RETRANS == (*getTmlNfcContext())->configRetrans) &&
                            (0x00 != ((*getTmlNfcContext())->readInfo.pBuffer[0] & 0xE0))) {
                        TMSLOG_TML_D("THN31 - Retransmission timer stopped.....\n");
                        /* Stop Timer to prevent Retransmission */
                        uint32_t timerStatus =
                            osalNfcTimerStop((*getTmlNfcContext())->timerId);
                        if (NFCSTATUS_SUCCESS != timerStatus) {
                            TMSLOG_TML_E("THN31 - timer stopped returned failure.....\n");
                        } else {
                            (*getTmlNfcContext())->writeCbInvoked = false;
                        }
                    }
                    /* Update the actual number of bytes read including header */
                    (*getTmlNfcContext())->readInfo.length = (uint16_t)(bytesRead);
                    tmsNciHalPrintPacket("RECV",
                                             (*getTmlNfcContext())->readInfo.pBuffer,
                                             (*getTmlNfcContext())->readInfo.length);

                    bytesRead = PH_TMLNFC_RESET_VALUE;

                    /* Fill the Transaction info structure to be passed to Callback
                     * Function */
                    transactionInfo.status = status;
                    transactionInfo.pBuff = (*getTmlNfcContext())->readInfo.pBuffer;
                    /* Actual number of bytes read is filled in the structure */
                    transactionInfo.length = (*getTmlNfcContext())->readInfo.length;

                    /* Read operation completed successfully. Post a Message onto Callback
                     * Thread*/
                    /* Prepare the message to be posted on User thread */
                    deferredInfo.pCallback = &tmlNfcReadDeferredCb;
                    deferredInfo.pParameter = &transactionInfo;
                    msg.msgType = PH_LIBNFC_DEFERREDCALL_MSG;
                    msg.pMsgData = &deferredInfo;
                    msg.size = sizeof(deferredInfo);
                    TMSLOG_TML_D("THN31 - Posting read message.....\n");
                    tmlNfcDeferredCall((*getTmlNfcContext())->callbackThreadId, &msg);
                }
            } else {
                TMSLOG_TML_D("THN31 -(*getTmlNfcContext())->pDevHandle is NULL");
            }
        } else {
            TMSLOG_TML_D("THN31 - read request NOT enabled");
            usleep(10 * 1000);
        }
    } /* End of While loop */

    return NULL;
}

/*******************************************************************************
**
** Function         tmlNfcTmlWriterThread
**
** Description      Writes the requested data onto the lower layer driver
**
** Parameters       pParam  - context provided by upper layer
**
** Returns          None
**
*******************************************************************************/
static void *tmlNfcTmlWriterThread(void *pParam) {
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    int32_t bytesRead = PH_TMLNFC_RESET_VALUE;
    /* Transaction info pBuffer to be passed to Callback Thread */
    static TmlNfcTransactInfo_t transactionInfo;
    /* Structure containing Tml callback function and parameters to be invoked
       by the callback thread */
    static NciHalDeferredCall_t deferredInfo;
    /* initialize Message structure to post message onto Callback Thread */
    static NciHalMessage_t msg;
    /* In case of I2C Write Retry */
    static uint16_t retryCnt;
    UNUSED_PROP(pParam);
    TMSLOG_TML_D("THN31 - Tml Writer Thread Started................\n");

    /* Writer thread loop shall be running till shutdown is invoked */
    while ((*getTmlNfcContext())->bThreadDone) {
        TMSLOG_TML_D("THN31 - Tml Writer Thread Running................\n");
        if (-1 == sem_wait(&(*getTmlNfcContext())->txSemaphore)) {
            TMSLOG_TML_E("sem_wait didn't return success \n");
        }
        /* If Tml write is requested */
        if (1 == (*getTmlNfcContext())->writeInfo.bEnable) {
            TMSLOG_TML_D("THN31 - Write requested.....\n");
            /* Set the variable to success initially */
            status = NFCSTATUS_SUCCESS;
            if (NULL != (*getTmlNfcContext())->pDevHandle) {
                (*getTmlNfcContext())->writeInfo.bEnable = 0;
                /* Variable to fetch the actual number of bytes written */
                bytesRead = PH_TMLNFC_RESET_VALUE;
                /* Write the data in the pBuffer onto the file */
                TMSLOG_TML_D("THN31 - Invoking I2C Write.....\n");
                /* TML reader writer callback synchronization mutex lock --- START */
                pthread_mutex_lock(&(*getTmlNfcContext())->waitBusyLock);
                (*getTmlNfcContext())->writerCbflag = false;
                bytesRead = (*getTransportObj())->i2cWrite((*getTmlNfcContext())->pDevHandle,
                                                      (*getTmlNfcContext())->writeInfo.pBuffer,
                                                      (*getTmlNfcContext())->writeInfo.length);
                /* TML reader writer callback synchronization mutex lock --- END */
                pthread_mutex_unlock(&(*getTmlNfcContext())->waitBusyLock);

                /* Try I2C Write Five Times, if it fails : Raju */
                if (-1 == bytesRead) {
                    TMSLOG_TML_D("THN31 - Error in I2C Write.....\n");
                    status = PHNFCSTVAL(CID_NFC_TML, NFCSTATUS_FAILED);
                } else {
                    tmsNciHalPrintPacket("SEND",
                                             (*getTmlNfcContext())->writeInfo.pBuffer,
                                             (*getTmlNfcContext())->writeInfo.length);
                }
                retryCnt = 0;
                if (NFCSTATUS_SUCCESS == status) {
                    TMSLOG_TML_D("THN31 - I2C Write successful.....\n");
                    bytesRead = PH_TMLNFC_VALUE_ONE;
                }
                /* Fill the Transaction info structure to be passed to Callback Function
                 */
                transactionInfo.status = status;
                transactionInfo.pBuff = (*getTmlNfcContext())->writeInfo.pBuffer;
                /* Actual number of bytes written is filled in the structure */
                transactionInfo.length = (uint16_t)bytesRead;

                /* Prepare the message to be posted on the User thread */
                deferredInfo.pCallback = &tmlNfcWriteDeferredCb;
                deferredInfo.pParameter = &transactionInfo;
                /* Write operation completed successfully. Post a Message onto Callback
                 * Thread*/
                msg.msgType = PH_LIBNFC_DEFERREDCALL_MSG;
                msg.pMsgData = &deferredInfo;
                msg.size = sizeof(deferredInfo);

                /* Check whether Retransmission needs to be started,
                 * If yes, Post message only if
                 * case 1. Message is not posted &&
                 * case 11. Write status is success ||
                 * case 12. Last retry of write is also failure
                 */
                if ((TMLNFC_ENABLE_RETRANS == (*getTmlNfcContext())->configRetrans) &&
                        (0x00 != ((*getTmlNfcContext())->writeInfo.pBuffer[0] & 0xE0))) {
                    if ((*getTmlNfcContext())->writeCbInvoked == false) {
                        if ((NFCSTATUS_SUCCESS == status) || (gCurrentRetryCount == 0)) {
                            TMSLOG_TML_D("THN31 - Posting Write message.....\n");
                            tmlNfcDeferredCall((*getTmlNfcContext())->callbackThreadId,
                                                  &msg);
                            (*getTmlNfcContext())->writeCbInvoked = true;
                        }
                    }
                } else {
                    TMSLOG_TML_D("THN31 - Posting Fresh Write message.....\n");
                    tmlNfcDeferredCall((*getTmlNfcContext())->callbackThreadId, &msg);
                    if (NFCSTATUS_SUCCESS == status) {
                        /*TML reader writer thread callback syncronization---START*/
                        pthread_mutex_lock(&(*getTmlNfcContext())->waitBusyLock);
                        (*getTmlNfcContext())->writerCbflag = true;
                        tmlNfcSignalWriteComplete();
                        /*TML reader writer thread callback syncronization---END*/
                        pthread_mutex_unlock(&(*getTmlNfcContext())->waitBusyLock);
                    }
                }
            } else {
                TMSLOG_TML_D("THN31 - (*getTmlNfcContext())->pDevHandle is NULL");
            }

            /* If Data packet is sent, then NO retransmission */
            if ((TMLNFC_ENABLE_RETRANS == (*getTmlNfcContext())->configRetrans) &&
                    (0x00 != ((*getTmlNfcContext())->writeInfo.pBuffer[0] & 0xE0))) {
                TMSLOG_TML_D("THN31 - Starting timer for Retransmission case");
                status = tmlNfcInitiateTimer();
                if (NFCSTATUS_SUCCESS != status) {
                    /* Reset Variables used for Retransmission */
                    TMSLOG_TML_D("THN31 - Retransmission timer initiate failed");
                    (*getTmlNfcContext())->writeInfo.bEnable = 0;
                    gCurrentRetryCount = 0;
                }
            }
        } else {
            TMSLOG_TML_D("THN31 - Write request NOT enabled");
            usleep(10000);
        }

    } /* End of While loop */

    return NULL;
}

/*******************************************************************************
**
** Function         tmlNfcCleanUp
**
** Description      Clears all handles opened during TML initialization
**
** Parameters       None
**
** Returns          None
**
*******************************************************************************/
void tmlNfcCleanUp(void) {
    if (NULL == *getTmlNfcContext()) {
        return;
    }
    sem_destroy(&(*getTmlNfcContext())->rxSemaphore);
    sem_destroy(&(*getTmlNfcContext())->txSemaphore);
    sem_destroy(&(*getTmlNfcContext())->postMsgSemaphore);
    pthread_mutex_destroy(&(*getTmlNfcContext())->waitBusyLock);
    pthread_cond_destroy(&(*getTmlNfcContext())->waitBusyCondition);
    *getTransportObj() = NULL;
    /* Clear memory allocated for storing Context variables */
    free((void *)*getTmlNfcContext());
    /* Set the pointer to NULL to indicate De-Initialization */
    *getTmlNfcContext() = NULL;

    return;
}

/*******************************************************************************
**
** Function         tmlNfcShutdown
**
** Description      Uninitializes TML layer and hardware interface
**
** Parameters       None
**
** Returns          NFC status:
**                  NFCSTATUS_SUCCESS - TML configuration released successfully
**                  NFCSTATUS_INVALID_PARAMETER - at least one parameter is
**                                                invalid
**                  NFCSTATUS_FAILED - un-initialization failed (example: unable
**                                     to close interface)
**
*******************************************************************************/
NFCSTATUS tmlNfcShutdown(void) {
    NFCSTATUS wShutdownStatus = NFCSTATUS_SUCCESS;

    /* Check whether TML is Initialized */
    if (NULL != *getTmlNfcContext()) {
        /* Reset thread variable to terminate the thread */
        (*getTmlNfcContext())->bThreadDone = 0;
        usleep(1000);
        /* Clear All the resources allocated during initialization */
        sem_post(&(*getTmlNfcContext())->rxSemaphore);
        usleep(1000);
        sem_post(&(*getTmlNfcContext())->txSemaphore);
        usleep(1000);
        sem_post(&(*getTmlNfcContext())->postMsgSemaphore);
        usleep(1000);
        sem_post(&(*getTmlNfcContext())->postMsgSemaphore);
        usleep(1000);

        (*getTransportObj())->i2cClose((*getTmlNfcContext())->pDevHandle);
        (*getTmlNfcContext())->pDevHandle = NULL;
        if (0 != pthread_join((*getTmlNfcContext())->readerThread, (void **)NULL)) {
            TMSLOG_TML_E("Fail to kill reader thread!");
        }
        if (0 != pthread_join((*getTmlNfcContext())->writerThread, (void **)NULL)) {
            TMSLOG_TML_E("Fail to kill writer thread!");
        }
        TMSLOG_TML_D("bThreadDone == 0");

    } else {
        wShutdownStatus = PHNFCSTVAL(CID_NFC_TML, NFCSTATUS_NOT_INITIALISED);
    }

    return wShutdownStatus;
}

/*******************************************************************************
**
** Function         tmlNfcWrite
**
** Description      Asynchronously writes given data block to hardware
**                  interface/driver. Enables writer thread if there are no
**                  write requests pending. Returns successfully once writer
**                  thread completes write operation. Notifies upper layer using
**                  callback mechanism.
**
**                  NOTE:
**                  * it is important to post a message with id
**                    PH_TMLNFC_WRITE_MESSAGE to IntegrationThread after data
**                    has been written to THN31
**                  * if CRC needs to be computed, then input pBuffer should be
**                    capable to store two more bytes apart from length of
**                    packet
**
** Parameters       pBuffer - data to be sent
**                  length - length of data pBuffer
**                  pTmlWriteComplete - pointer to the function to be invoked
**                                      upon completion
**                  pContext - context provided by upper layer
**
** Returns          NFC status:
**                  NFCSTATUS_PENDING - command is yet to be processed
**                  NFCSTATUS_INVALID_PARAMETER - at least one parameter is
**                                                invalid
**                  NFCSTATUS_BUSY - write request is already in progress
**
*******************************************************************************/
NFCSTATUS tmlNfcWrite(uint8_t *pBuffer, uint16_t length,
                         pTmlNfcTransactCompletionCb_t pTmlWriteComplete,
                         void *pContext) {
    NFCSTATUS wWriteStatus;

    /* Check whether TML is Initialized */

    if (NULL != *getTmlNfcContext()) {
        if ((NULL != (*getTmlNfcContext())->pDevHandle) && (NULL != pBuffer) &&
                (PH_TMLNFC_RESET_VALUE != length) && (NULL != pTmlWriteComplete)) {
            if (!(*getTmlNfcContext())->writeInfo.bThreadBusy) {
                /* Setting the flag marks beginning of a Write Operation */
                (*getTmlNfcContext())->writeInfo.bThreadBusy = true;
                /* Copy the pBuffer, length and Callback function,
                   This shall be utilized while invoking the Callback function in thread
                   */
                (*getTmlNfcContext())->writeInfo.pBuffer = pBuffer;
                (*getTmlNfcContext())->writeInfo.length = length;
                (*getTmlNfcContext())->writeInfo.pThread_Callback = pTmlWriteComplete;
                (*getTmlNfcContext())->writeInfo.pContext = pContext;

                wWriteStatus = NFCSTATUS_PENDING;
                // FIXME: If retry is going on. Stop the retry thread/timer
                if (TMLNFC_ENABLE_RETRANS == (*getTmlNfcContext())->configRetrans) {
                    /* Set retry count to default value */
                    // FIXME: If the timer expired there, and meanwhile we have created
                    // a new request. The expired timer will think that retry is still
                    // ongoing.
                    gCurrentRetryCount = (*getTmlNfcContext())->retryCount;
                    (*getTmlNfcContext())->writeCbInvoked = false;
                }
                /* Set event to invoke Writer Thread */
                (*getTmlNfcContext())->writeInfo.bEnable = 1;
                sem_post(&(*getTmlNfcContext())->txSemaphore);
            } else {
                wWriteStatus = PHNFCSTVAL(CID_NFC_TML, NFCSTATUS_BUSY);
            }
        } else {
            wWriteStatus = PHNFCSTVAL(CID_NFC_TML, NFCSTATUS_INVALID_PARAMETER);
        }
    } else {
        wWriteStatus = PHNFCSTVAL(CID_NFC_TML, NFCSTATUS_NOT_INITIALISED);
    }

    return wWriteStatus;
}

/*******************************************************************************
**
** Function         tmlNfcRead
**
** Description      Asynchronously reads data from the driver
**                  Number of bytes to be read and pBuffer are passed by upper
**                  layer.
**                  Enables reader thread if there are no read requests pending
**                  Returns successfully once read operation is completed
**                  Notifies upper layer using callback mechanism
**
** Parameters       pBuffer - location to send read data to the upper layer via
**                            callback
**                  length - length of read data pBuffer passed by upper layer
**                  pTmlReadComplete - pointer to the function to be invoked
**                                     upon completion of read operation
**                  pContext - context provided by upper layer
**
** Returns          NFC status:
**                  NFCSTATUS_PENDING - command is yet to be processed
**                  NFCSTATUS_INVALID_PARAMETER - at least one parameter is
**                                                invalid
**                  NFCSTATUS_BUSY - read request is already in progress
**
*******************************************************************************/
NFCSTATUS tmlNfcRead(uint8_t *pBuffer, uint16_t length,
                        pTmlNfcTransactCompletionCb_t pTmlReadComplete,
                        void *pContext) {
    NFCSTATUS wReadStatus;
    int rxSemVal = 0, ret = 0;

    /* Check whether TML is Initialized */
    if (NULL != *getTmlNfcContext()) {
        if (((*getTmlNfcContext())->pDevHandle != NULL) && (NULL != pBuffer) &&
                (PH_TMLNFC_RESET_VALUE != length) && (NULL != pTmlReadComplete)) {
            if (!(*getTmlNfcContext())->readInfo.bThreadBusy) {
                /* Setting the flag marks beginning of a Read Operation */
                (*getTmlNfcContext())->readInfo.bThreadBusy = true;
                /* Copy the pBuffer, length and Callback function,
                   This shall be utilized while invoking the Callback function in thread
                   */
                (*getTmlNfcContext())->readInfo.pBuffer = pBuffer;
                (*getTmlNfcContext())->readInfo.length = length;
                (*getTmlNfcContext())->readInfo.pThread_Callback = pTmlReadComplete;
                (*getTmlNfcContext())->readInfo.pContext = pContext;
                wReadStatus = NFCSTATUS_PENDING;

                /* Set event to invoke Reader Thread */
                (*getTmlNfcContext())->readInfo.bEnable = 1;
                ret = sem_getvalue(&(*getTmlNfcContext())->rxSemaphore, &rxSemVal);
                /* Post rxSemaphore either if sem_getvalue() is failed or rxSemVal is 0 */
                if (ret || !rxSemVal) {
                    sem_post(&(*getTmlNfcContext())->rxSemaphore);
                } else {
                    TMSLOG_TML_D("%s: skip reader thread scheduling, ret=%x, rxSemaVal=%x",
                                 __func__, ret, rxSemVal);
                }
            } else {
                wReadStatus = PHNFCSTVAL(CID_NFC_TML, NFCSTATUS_BUSY);
            }
        } else {
            wReadStatus = PHNFCSTVAL(CID_NFC_TML, NFCSTATUS_INVALID_PARAMETER);
        }
    } else {
        wReadStatus = PHNFCSTVAL(CID_NFC_TML, NFCSTATUS_NOT_INITIALISED);
    }

    return wReadStatus;
}

/*******************************************************************************
**
** Function         tmlNfcReadAbort
**
** Description      Aborts pending read request (if any)
**
** Parameters       None
**
** Returns          NFC status:
**                  NFCSTATUS_SUCCESS - ongoing read operation aborted
**                  NFCSTATUS_INVALID_PARAMETER - at least one parameter is
**                                                invalid
**                  NFCSTATUS_NOT_INITIALIZED - TML layer is not initialized
**                  NFCSTATUS_BOARD_COMMUNICATION_ERROR - unable to cancel read
**                                                        operation
**
*******************************************************************************/
NFCSTATUS tmlNfcReadAbort(void) {
    NFCSTATUS status = NFCSTATUS_INVALID_PARAMETER;
    (*getTmlNfcContext())->readInfo.bEnable = 0;

    /*Reset the flag to accept another Read Request */
    (*getTmlNfcContext())->readInfo.bThreadBusy = false;
    status = NFCSTATUS_SUCCESS;

    return status;
}

/*******************************************************************************
**
** Function         tmlNfcWriteAbort
**
** Description      Aborts pending write request (if any)
**
** Parameters       None
**
** Returns          NFC status:
**                  NFCSTATUS_SUCCESS - ongoing write operation aborted
**                  NFCSTATUS_INVALID_PARAMETER - at least one parameter is
**                                                invalid
**                  NFCSTATUS_NOT_INITIALIZED - TML layer is not initialized
**                  NFCSTATUS_BOARD_COMMUNICATION_ERROR - unable to cancel write
**                                                        operation
**
*******************************************************************************/
NFCSTATUS tmlNfcWriteAbort(void) {
    NFCSTATUS status = NFCSTATUS_INVALID_PARAMETER;

    (*getTmlNfcContext())->writeInfo.bEnable = 0;
    /* Stop if any retransmission is in progress */
    gCurrentRetryCount = 0;

    /* Reset the flag to accept another Write Request */
    (*getTmlNfcContext())->writeInfo.bThreadBusy = false;
    status = NFCSTATUS_SUCCESS;

    return status;
}

/*******************************************************************************
**
** Function         tmlNfcIoCtl
**
** Description      Resets device when insisted by upper layer
**                  Number of bytes to be read and pBuffer are passed by upper
**                  layer
**                  Enables reader thread if there are no read requests pending
**                  Returns successfully once read operation is completed
**                  Notifies upper layer using callback mechanism
**
** Parameters       eControlCode       - control code for a specific operation
**
** Returns          NFC status:
**                  NFCSTATUS_SUCCESS  - ioctl command completed successfully
**                  NFCSTATUS_FAILED   - ioctl command request failed
**
*******************************************************************************/
NFCSTATUS tmlNfcIoCtl(TmlNfcControlCode_t eControlCode) {
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    uint8_t readFlag = ((*getTmlNfcContext())->readInfo.bEnable > 0);

    if (NULL == *getTmlNfcContext()) {
        status = NFCSTATUS_FAILED;
    } else {
        switch (eControlCode) {
            case TMLNFC_POWER_RESET: {
                if ((*getNfcFL()).chipType >= thn31) {
                    /*VEN_RESET*/
                    (*getTransportObj())->nfccReset((*getTmlNfcContext())->pDevHandle, MODE_POWER_RESET);
                } else {
                    (*getTransportObj())->nfccReset((*getTmlNfcContext())->pDevHandle, MODE_POWER_ON);
                    usleep(100 * 1000);
                    (*getTransportObj())->nfccReset((*getTmlNfcContext())->pDevHandle, MODE_POWER_OFF);
                    usleep(100 * 1000);
                    (*getTransportObj())->nfccReset((*getTmlNfcContext())->pDevHandle, MODE_POWER_ON);
                }
                break;
            }
            case TMLNFC_ENABLE_VEN: {
                break;
            }
            case TMLNFC_RESET_DEVICE:

            {
                break;
            }
            case TMLNFC_ENABLE_NORMAL_MODE: {
                /*Reset THN31*/
                (*getTmlNfcContext())->readInfo.bEnable = 0;
                if ((*getNfcFL()).nfccFL._NFCC_DWNLD_MODE == NFCC_DWNLD_WITH_VEN_RESET) {
                    TMSLOG_TML_D(" TMLNFC_ENABLE_NORMAL_MODE complete with VEN RESET ");
                    (*getTransportObj())->nfccReset((*getTmlNfcContext())->pDevHandle, MODE_FW_GPIO_LOW);
                } else if ((*getNfcFL()).nfccFL._NFCC_DWNLD_MODE == NFCC_DWNLD_WITH_NCI_CMD) {
                    TMSLOG_TML_D(" TMLNFC_ENABLE_NORMAL_MODE complete with NCI CMD ");
                    (*getTransportObj())->nfccReset((*getTmlNfcContext())->pDevHandle, MODE_FW_GPIO_LOW);
                }
                break;
            }
            case TMLNFC_ENABLE_DL_MODE: {
                tmlNfcConfigNciPktReTx(TMLNFC_DISABLE_RETRANS, 0);
                (*getTmlNfcContext())->readInfo.bEnable = 0;
                if ((*getNfcFL()).nfccFL._NFCC_DWNLD_MODE == NFCC_DWNLD_WITH_VEN_RESET) {
                    TMSLOG_TML_D(" TMLNFC_ENABLE_DL_MODE complete with VEN RESET ");
                    status = (*getTransportObj())->nfccReset((*getTmlNfcContext())->pDevHandle,
                                                        MODE_FW_DWNLD_WITH_VEN);
                } else if ((*getNfcFL()).nfccFL._NFCC_DWNLD_MODE == NFCC_DWNLD_WITH_NCI_CMD) {
                    TMSLOG_TML_D(" TMLNFC_ENABLE_DL_MODE complete with NCI CMD ");
                    status = (*getTransportObj())->nfccReset((*getTmlNfcContext())->pDevHandle,
                                                        MODE_FW_DWND_HIGH);
                }
                break;
            }
            case TMLNFC_ENABLE_DL_MODE_WITH_VEN_RST: {
                tmlNfcConfigNciPktReTx(TMLNFC_DISABLE_RETRANS, 0);
                (*getTmlNfcContext())->readInfo.bEnable = 0;
                TMSLOG_TML_D(" tmlNfce_EnableDownloadModewithVenRst complete with VEN RESET ");
                status = (*getTransportObj())->nfccReset((*getTmlNfcContext())->pDevHandle, MODE_FW_DWNLD_WITH_VEN);
                break;
            }
            case TMLNFC_SET_FRAGMENT_SIZE: {
                if ((*getNfcFL()).chipType == thn31) {
                    (*getTmlNfcContext())->fragmentLen = PH_TMLNFC_FRGMENT_SIZE_THN31;
                    TMSLOG_TML_D("TMLNFC_SET_FRAGMENT_SIZE 0x22A");
                }
                break;
            }
            default: {
                status = NFCSTATUS_INVALID_PARAMETER;
                break;
            }
        }
        if (readFlag && ((*getTmlNfcContext())->readInfo.bEnable == 0x00)) {
            (*getTmlNfcContext())->readInfo.bEnable = 1;
            sem_post(&(*getTmlNfcContext())->rxSemaphore);
        }
    }

    return status;
}

/*******************************************************************************
**
** Function         tmlNfcDeferredCall
**
** Description      Posts message on upper layer thread
**                  upon successful read or write operation
**
** Parameters       dwThreadId  - id of the thread posting message
**                  pWorkerMsg - message to be posted
**
** Returns          None
**
*******************************************************************************/
void tmlNfcDeferredCall(uintptr_t dwThreadId,
                           NciHalMessage_t *pWorkerMsg) {
    intptr_t pPostStatus;
    UNUSED_PROP(dwThreadId);
    /* Post message on the user thread to invoke the callback function */
    if (-1 == sem_wait(&(*getTmlNfcContext())->postMsgSemaphore)) {
        TMSLOG_TML_E("sem_wait didn't return success \n");
    }
    pPostStatus =
        dal4NfcMsgSnd((*getTmlNfcContext())->callbackThreadId, pWorkerMsg, 0);
    sem_post(&(*getTmlNfcContext())->postMsgSemaphore);
}

/*******************************************************************************
**
** Function         tmlNfcReadDeferredCb
**
** Description      Read thread call back function
**
** Parameters       pParams - context provided by upper layer
**
** Returns          None
**
*******************************************************************************/
static void tmlNfcReadDeferredCb(void *pParams) {
    /* Transaction info pBuffer to be passed to Callback Function */
    TmlNfcTransactInfo_t *pTransactionInfo = (TmlNfcTransactInfo_t *)pParams;

    /* Reset the flag to accept another Read Request */
    (*getTmlNfcContext())->readInfo.bThreadBusy = false;
    (*getTmlNfcContext())->readInfo.pThread_Callback(
        (*getTmlNfcContext())->readInfo.pContext, pTransactionInfo);

    return;
}

/*******************************************************************************
**
** Function         tmlNfcWriteDeferredCb
**
** Description      Write thread call back function
**
** Parameters       pParams - context provided by upper layer
**
** Returns          None
**
*******************************************************************************/
static void tmlNfcWriteDeferredCb(void *pParams) {
    /* Transaction info pBuffer to be passed to Callback Function */
    TmlNfcTransactInfo_t *pTransactionInfo = (TmlNfcTransactInfo_t *)pParams;

    /* Reset the flag to accept another Write Request */
    (*getTmlNfcContext())->writeInfo.bThreadBusy = false;
    (*getTmlNfcContext())->writeInfo.pThread_Callback(
        (*getTmlNfcContext())->writeInfo.pContext, pTransactionInfo);

    return;
}

void tmlNfcSetFragmentationEnabled(TmlNfci2cFragmentation_t result) {
    *getFragmentationEnabled() = result;
}

TmlNfci2cFragmentation_t tmlNfcGetFragmentationEnabled() {
    return *getFragmentationEnabled();
}

/*******************************************************************************
**
** Function         tmlNfcSignalWriteComplete
**
** Description      function to invoke reader thread
**
** Parameters       None
**
** Returns          None
**
*******************************************************************************/
static void tmlNfcSignalWriteComplete(void) {
    int ret = -1;
    if ((*getTmlNfcContext())->waitBusyFlag == true) {
        TMSLOG_TML_D("tmlNfcSignalWriteComplete - enter");
        (*getTmlNfcContext())->waitBusyFlag = false;

        ret = pthread_cond_signal(&(*getTmlNfcContext())->waitBusyCondition);
        if (ret) {
            TMSLOG_TML_E(" tmlNfcSignalWriteComplete failed, error = 0x%X", ret);
        }
        TMSLOG_TML_D("tmlNfcSignalWriteComplete - exit");
    }
}

/*******************************************************************************
**
** Function         tmlNfcWaitReadInit
**
** Description      init function for reader thread
**
** Parameters       None
**
** Returns          int
**
*******************************************************************************/
static int tmlNfcWaitReadInit(void) {
    int ret = -1;
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    memset(&(*getTmlNfcContext())->waitBusyCondition, 0,
           sizeof((*getTmlNfcContext())->waitBusyCondition));
    pthread_mutex_init(&(*getTmlNfcContext())->waitBusyLock, NULL);
    ret = pthread_cond_init(&(*getTmlNfcContext())->waitBusyCondition, &attr);
    if (ret) {
        TMSLOG_TML_E(" phTtmlNfcWaitReadInit failed, error = 0x%X", ret);
    }
    return ret;
}

/*******************************************************************************
**
** Function         tmlNfcShutdownCleanUp
**
** Description      wrapper function  for shutdown  and cleanup of resources
**
** Parameters       None
**
** Returns          NFCSTATUS
**
*******************************************************************************/
NFCSTATUS tmlNfcShutdownCleanUp() {
    NFCSTATUS wShutdownStatus = tmlNfcShutdown();
    tmlNfcCleanUp();
    return wShutdownStatus;
}
