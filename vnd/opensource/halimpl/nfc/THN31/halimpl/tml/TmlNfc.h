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
 * Transport Mapping Layer header files containing APIs related to initializing,
 * reading
 * and writing data into files provided by the driver interface.
 *
 * API listed here encompasses Transport Mapping Layer interfaces required to be
 * mapped
 * to different Interfaces and Platforms.
 *
 */

#ifndef PHTMLNFC_H
#define PHTMLNFC_H

#include <NfcCommon.h>

/*
 * Message posted by Reader thread upon
 * completion of requested operation
 */
#define PH_TMLNFC_READ_MESSAGE (0xAA)

/*
 * Message posted by Writer thread upon
 * completion of requested operation
 */
#define PH_TMLNFC_WRITE_MESSAGE (0x55)

/*
 * Value indicates to reset device
 */
#define PH_TMLNFC_RESETDEVICE (0x00008001)

/*
 * Fragment Length for THN31
 */
#define PH_TMLNFC_FRGMENT_SIZE_THN31 (0x22A)

/*
***************************Globals,Structure and Enumeration ******************
*/

/*
 * Transaction (Tx/Rx) completion information structure of TML
 *
 * This structure holds the completion callback information of the
 * transaction passed from the TML layer to the Upper layer
 * along with the completion callback.
 *
 * The value of field status can be interpreted as:
 *
 *     - NFCSTATUS_SUCCESS                    Transaction performed
 * successfully.
 *     - NFCSTATUS_FAILED                     Failed to wait on Read/Write
 * operation.
 *     - NFCSTATUS_INSUFFICIENT_STORAGE       Not enough memory to store data in
 * case of read.
 *     - NFCSTATUS_BOARD_COMMUNICATION_ERROR  Failure to Read/Write from the
 * file or timeOut.
 */

typedef struct TmlNfcTransactInfo {
    NFCSTATUS status;       /* Status of the Transaction Completion*/
    uint8_t *pBuff;          /* Response Data of the Transaction*/
    uint16_t length;        /* Data size of the Transaction*/
} TmlNfcTransactInfo_t; /* Instance of Transaction structure */

/*
 * TML transreceive completion callback to Upper Layer
 *
 * pContext - Context provided by upper layer
 * pInfo    - Transaction info. See TmlNfcTransactInfo
 */
typedef void (*pTmlNfcTransactCompletionCb_t)(
    void *pContext, TmlNfcTransactInfo_t *pInfo);

/*
 * TML Deferred callback interface structure invoked by upper layer
 *
 * This could be used for read/write operations
 *
 * dwMsgPostedThread Message source identifier
 * pParams Parameters for the deferred call processing
 */
typedef void (*pTmlNfcDeferFuncPointer_t)(uint32_t dwMsgPostedThread,
        void *pParams);

/*
 * Enum definition contains  supported ioctl control codes.
 *
 * tmlNfcIoCtl
 */
typedef enum {
    TMLNFC_INVALID = 0,
    TMLNFC_RESET_DEVICE = PH_TMLNFC_RESETDEVICE, /* Reset the device */
    TMLNFC_ENABLE_DL_MODE, /* Do the hardware setting to enter into
                                    download mode */
    TMLNFC_ENABLE_NORMAL_MODE, /* Hardware setting for normal mode of operation
                                 */
    TMLNFC_ENABLE_DL_MODE_WITH_VEN_RST,
    TMLNFC_ENABLE_VEN,         /* Enable Ven for THN31 chip*/
    TMLNFC_POWER_RESET = 5,
    TMLNFC_SET_FRAGMENT_SIZE,
} TmlNfcControlCode_t;     /* Control code for IOCTL call */

/*
 * Enable / Disable Re-Transmission of Packets
 *
 * tmlNfcConfigNciPktReTx
 */
typedef enum {
    TMLNFC_ENABLE_RETRANS = 0x00, /*Enable retransmission of Nci packet */
    TMLNFC_DISABLE_RETRANS = 0x01 /*Disable retransmission of Nci packet */
} TmlNfcConfigRetrans_t;        /* Configuration for Retransmission */

/*
 * Structure containing details related to read and write operations
 *
 */
typedef struct TmlNfcReadWriteInfo {
    volatile uint8_t bEnable; /*This flag shall decide whether to perform
                               Write/Read operation */
    uint8_t
    bThreadBusy; /*Flag to indicate thread is busy on respective operation */
    /* Transaction completion Callback function */
    pTmlNfcTransactCompletionCb_t pThread_Callback;
    void *pContext;        /*Context passed while invocation of operation */
    uint8_t *pBuffer;      /*Buffer passed while invocation of operation */
    uint16_t length;      /*Length of data read/written */
    NFCSTATUS wWorkStatus; /*Status of the transaction performed */
} TmlNfcReadWriteInfo_t;

/*
 *Base Context Structure containing members required for entire session
 */
typedef struct TmlNfcContext {
    pthread_t readerThread; /*Handle to the thread which handles write and read
                             operations */
    pthread_t writerThread;
    volatile uint8_t
    bThreadDone; /*Flag to decide whether to run or abort the thread */
    TmlNfcConfigRetrans_t
    configRetrans;             /*Retransmission of Nci Packet during timeOut */
    uint8_t retryCount;     /*Number of times retransmission shall happen */
    uint8_t writeCbInvoked; /* Indicates whether write callback is invoked during
                              retransmission */
    uint32_t timerId;      /* Timer used to retransmit nci packet */
    TmlNfcReadWriteInfo_t readInfo;  /*Pointer to Reader Thread Structure */
    TmlNfcReadWriteInfo_t writeInfo; /*Pointer to Writer Thread Structure */
    void *pDevHandle;                    /* Pointer to Device Handle */
    uintptr_t callbackThreadId; /* Thread ID to which message to be posted */
    uint8_t bEnableCrc;           /*Flag to validate/not CRC for input pBuffer */
    sem_t rxSemaphore;
    sem_t txSemaphore;      /* Lock/Aquire txRx Semaphore */
    sem_t postMsgSemaphore; /* Semaphore to post message atomically by Reader &
                             writer thread */
    pthread_cond_t waitBusyCondition; /*Condition to wait reader thread*/
    pthread_mutex_t waitBusyLock;     /*Condition lock to wait reader thread*/
    volatile uint8_t waitBusyFlag;    /*Condition flag to wait reader thread*/
    volatile uint8_t
    writerCbflag; /* flag to indicate write callback message is pushed to
                        queue*/
    long    nfcServicePid; /*NFC Service PID to be used by driver to signal*/
    uint16_t fragmentLen;
} TmlNfcContext_t;

/*
 * TML Configuration exposed to upper layer.
 */
typedef struct TmlNfcConfig {
    /* Port name connected to THN31
     *
     * Platform specific canonical device name to which THN31 is connected.
     *
     * e.g. On Linux based systems this would be /dev/THN31
     */
    int8_t *pDevName;
    /* Callback Thread ID
     *
     * This is the thread ID on which the Reader & Writer thread posts message. */
    uintptr_t getMsgThreadId;
    /* Communication speed between DH and THN31
     *
     * This is the baudrate of the bus for communication between DH and THN31 */
    uint32_t baudRate;
    uint16_t fragmentLen;
} TmlNfcConfig_t, *pTmlNfcConfig_t; /* pointer to TmlNfcConfig_t */

/*
 * TML Deferred Callback structure used to invoke Upper layer Callback function.
 */
typedef struct {
    /* Deferred callback function to be invoked */
    pTmlNfcDeferFuncPointer_t pDefCall;
    /* Source identifier
     *
     * Identifier of the source which posted the message
     */
    uint32_t dwMsgPostedThread;
    /** Actual Message
     *
     * This is passed as a parameter passed to the deferred callback function
     * pDefCall. */
    void *pParams;
} TmlNfcDeferMsg_t; /* DeferMsg structure passed to User Thread */

typedef enum {
    I2C_FRAGMENATATION_DISABLED, /*i2c fragmentation_disabled           */
    I2C_FRAGMENTATION_ENABLED    /*i2c_fragmentation_enabled          */
} TmlNfci2cFragmentation_t;
/* Function declarations */
NFCSTATUS tmlNfcInit(pTmlNfcConfig_t pConfig);
NFCSTATUS tmlNfcShutdown(void);
NFCSTATUS tmlNfcShutdownCleanUp();
void tmlNfcCleanUp(void);
NFCSTATUS tmlNfcWrite(uint8_t *pBuffer, uint16_t length,
                         pTmlNfcTransactCompletionCb_t pTmlWriteComplete,
                         void *pContext);
NFCSTATUS tmlNfcRead(uint8_t *pBuffer, uint16_t length,
                        pTmlNfcTransactCompletionCb_t pTmlReadComplete,
                        void *pContext);
NFCSTATUS tmlNfcWriteAbort(void);
NFCSTATUS tmlNfcReadAbort(void);
NFCSTATUS tmlNfcIoCtl(TmlNfcControlCode_t eControlCode);
void tmlNfcDeferredCall(uintptr_t dwThreadId,
                           NciHalMessage_t *pWorkerMsg);
void tmlNfcConfigNciPktReTx(TmlNfcConfigRetrans_t configRetrans,
                               uint8_t retryCount);
void tmlNfcSetFragmentationEnabled(TmlNfci2cFragmentation_t enable);
TmlNfci2cFragmentation_t tmlNfcGetFragmentationEnabled();
NFCSTATUS tmlNfcConfigTransport();
TmlNfcContext_t **getTmlNfcContext(void);
volatile bool_t *getIsFirstHalMinOpen(void);

#endif /*  PHTMLNFC_H  */
