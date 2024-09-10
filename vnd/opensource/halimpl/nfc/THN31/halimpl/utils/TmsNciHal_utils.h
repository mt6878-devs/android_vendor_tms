/*
 *
 *  Copyright (C) 2013-2018, 2021 NXP
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

#ifndef PHTMSNCIHAL_UTILS_H
#define PHTMSNCIHAL_UTILS_H

#include <assert.h>
#include <NfcStatus.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

/********************* Definitions and structures *****************************/

/* List structures */
struct listNode {
    void *pData;
    struct listNode *pNext;
};

struct listHead {
    struct listNode *pFirst;
    pthread_mutex_t mutex;
};

/* Semaphore handling structure */
typedef struct TmsNciHalSem {
    /* Semaphore used to wait for callback */
    sem_t sem;

    /* Used to store the status sent by the callback */
    NFCSTATUS status;

    /* Used to provide a local context to the callback */
    void *pContext;

} TmsNciHalSem_t;

/* Semaphore helper macros */
#define SEM_WAIT(cbData)                                                   \
    ((sem_wait(&((cbData).sem)) == 0) ? 0 : (errno == EINTR)                 \
     ? sem_wait(&((cbData).sem)) \
     : -1)

#define SEM_POST(pCbData) sem_post(&((pCbData)->sem))

/* Semaphore and mutex monitor */
typedef struct TmsNciHalMonitor {
    /* Mutex protecting native library against reentrance */
    pthread_mutex_t reEntranceMutex;

    /* Mutex protecting native library against concurrency */
    pthread_mutex_t conCurrencyMutex;

    /* List used to track pending semaphores waiting for callback */
    struct listHead semList;

} TmsNciHalMonitor_t;

/************************ Exposed functions ***********************************/
/* List functions */
int listInit(struct listHead *pList);
int listDestroy(struct listHead *pList);
int listAdd(struct listHead *pList, void *pData);
int listRemove(struct listHead *pList, void *pData);
int listGetAndRemoveNext(struct listHead *pList, void **ppData);
void listDump(struct listHead *pList);

/* TMS NCI HAL utility functions */
TmsNciHalMonitor_t *tmsNciHalInitMonitor(void);
void tmsNciHalCleanupMonitor(void);
TmsNciHalMonitor_t *tmsNciHalGetMonitor(void);
NFCSTATUS tmsNciHalInitCbData(TmsNciHalSem_t *pCallbackData,
                                   void *pContext);
void tmsNciHalCleanupCbData(TmsNciHalSem_t *pCallbackData);
void tmsNciHalReleaseAllCbData(void);
void tmsNciHalPrintPacket(const char *pString, const uint8_t *pData,
                              uint16_t len);
void tmsNciHalEmergencyRecovery(uint8_t status);

/* Lock unlock helper macros */
/* Lock unlock helper macros */
#define REENTRANCE_LOCK()        \
    if (tmsNciHalGetMonitor()) \
        pthread_mutex_lock(&tmsNciHalGetMonitor()->reEntranceMutex)
#define REENTRANCE_UNLOCK()      \
    if (tmsNciHalGetMonitor()) \
        pthread_mutex_unlock(&tmsNciHalGetMonitor()->reEntranceMutex)
#define CONCURRENCY_LOCK()       \
    if (tmsNciHalGetMonitor()) \
        pthread_mutex_lock(&tmsNciHalGetMonitor()->conCurrencyMutex)
#define CONCURRENCY_UNLOCK()     \
    if (tmsNciHalGetMonitor()) \
        pthread_mutex_unlock(&tmsNciHalGetMonitor()->conCurrencyMutex)

#endif /* PHTMSNCIHAL_UTILS_H */
