/*
 * Copyright (C) 2010-2019 NXP Semiconductors
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
 * DAL independent message queue implementation for Android (can be used under
 * Linux too)
 */

#include <errno.h>
#include <linux/ipc.h>
#include <Dal4Nfc_messageQueueLib.h>
#include <TmsLog.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct Dal4NfcMessageQueueItem {
    NciHalMessage_t msg;
    struct Dal4NfcMessageQueueItem *pPrev;
    struct Dal4NfcMessageQueueItem *pNext;
} Dal4NfcMessageQueueItem_t;

typedef struct Dal4NfcMessageQueue {
    Dal4NfcMessageQueueItem_t *pItems;
    pthread_mutex_t criticalSectionMutex;
    sem_t processSemaphore;

} Dal4NfcMessageQueue_t;

/*******************************************************************************
**
** Function         dal4NfcMsgGet
**
** Description      Allocates message queue
**
** Parameters       Ignored, included only for Linux queue API compatibility
**
** Returns          (int) value of pQueue if successful
**                  -1, if failed to allocate memory or to init mutex
**
*******************************************************************************/
intptr_t dal4NfcMsgGet(key_t key, int msgFlg) {
    Dal4NfcMessageQueue_t *pQueue;
    UNUSED_PROP(key);
    UNUSED_PROP(msgFlg);
    pQueue =
        (Dal4NfcMessageQueue_t *)malloc(sizeof(Dal4NfcMessageQueue_t));
    if (pQueue == NULL) {
        return -1;
    }
    memset(pQueue, 0, sizeof(Dal4NfcMessageQueue_t));
    if (pthread_mutex_init(&pQueue->criticalSectionMutex, NULL) != 0) {
        free(pQueue);
        return -1;
    }
    if (sem_init(&pQueue->processSemaphore, 0, 0) == -1) {
        free(pQueue);
        return -1;
    }

    return ((intptr_t)pQueue);
}

/*******************************************************************************
**
** Function         dal4NfcMsgRelease
**
** Description      Releases message queue
**
** Parameters       msqId - message queue handle
**
** Returns          None
**
*******************************************************************************/
void dal4NfcMsgRelease(intptr_t msqId) {
    Dal4NfcMessageQueue_t *pQueue = (Dal4NfcMessageQueue_t *)msqId;

    if (pQueue != NULL) {
        sem_post(&pQueue->processSemaphore);
        usleep(3000);
        if (sem_destroy(&pQueue->processSemaphore)) {
            TMSLOG_TML_E("Failed to destroy semaphore (errno=0x%08x)", errno);
        }
        pthread_mutex_destroy(&pQueue->criticalSectionMutex);

        free(pQueue);
    }

    return;
}

/*******************************************************************************
**
** Function         dal4NfcMsgCtl
**
** Description      Destroys message queue
**
** Parameters       msqId - message queue handle
**                  cmd, pBuf - ignored, included only for Linux queue API
**                  compatibility
**
** Returns          0,  if successful
**                  -1, if invalid handle is passed
**
*******************************************************************************/
int dal4NfcMsgCtl(intptr_t msqId, int cmd, void *pBuf) {
    Dal4NfcMessageQueue_t *pQueue;
    Dal4NfcMessageQueueItem_t *p;
    UNUSED_PROP(cmd);
    UNUSED_PROP(pBuf);
    if (msqId == 0) {
        return -1;
    }

    pQueue = (Dal4NfcMessageQueue_t *)msqId;
    pthread_mutex_lock(&pQueue->criticalSectionMutex);
    if (pQueue->pItems != NULL) {
        p = pQueue->pItems;
        while (p->pNext != NULL) {
            p = p->pNext;
        }
        while (p->pPrev != NULL) {
            p = p->pPrev;
            free(p->pNext);
            p->pNext = NULL;
        }
        free(p);
    }
    pQueue->pItems = NULL;
    pthread_mutex_unlock(&pQueue->criticalSectionMutex);
    pthread_mutex_destroy(&pQueue->criticalSectionMutex);
    free(pQueue);

    return 0;
}

/*******************************************************************************
**
** Function         dal4NfcMsgSnd
**
** Description      Sends a message to the queue. The message will be added at
**                  the end of the queue as appropriate for FIFO policy
**
** Parameters       msqId  - message queue handle
**                  pMsgp   - message to be sent
**                  pMsgsz  - message size
**                  msgFlg - ignored
**
** Returns          0,  if successful
**                  -1, if invalid parameter passed or failed to allocate memory
**
*******************************************************************************/
intptr_t dal4NfcMsgSnd(intptr_t msqId, NciHalMessage_t *pMsg, int msgFlg) {
    Dal4NfcMessageQueue_t *pQueue;
    Dal4NfcMessageQueueItem_t *p;
    Dal4NfcMessageQueueItem_t *pNew;
    UNUSED_PROP(msgFlg);
    if ((msqId == 0) || (pMsg == NULL)) {
        return -1;
    }

    pQueue = (Dal4NfcMessageQueue_t *)msqId;
    pNew = (Dal4NfcMessageQueueItem_t *)malloc(
               sizeof(Dal4NfcMessageQueueItem_t));
    if (pNew == NULL) {
        return -1;
    }
    memset(pNew, 0, sizeof(Dal4NfcMessageQueueItem_t));
    memcpy(&pNew->msg, pMsg, sizeof(NciHalMessage_t));
    pthread_mutex_lock(&pQueue->criticalSectionMutex);

    if (pQueue->pItems != NULL) {
        p = pQueue->pItems;
        while (p->pNext != NULL) {
            p = p->pNext;
        }
        p->pNext = pNew;
        pNew->pPrev = p;
    } else {
        pQueue->pItems = pNew;
    }
    pthread_mutex_unlock(&pQueue->criticalSectionMutex);

    sem_post(&pQueue->processSemaphore);

    return 0;
}

/*******************************************************************************
**
** Function         dal4NfcMsgRcv
**
** Description      Gets the oldest message from the queue.
**                  If the queue is empty the function waits (blocks on a mutex)
**                  until a message is posted to the queue with dal4NfcMsgSnd
**
** Parameters       msqId  - message queue handle
**                  pMsgp   - message to be received
**                  pMsgsz  - message size
**                  pMsgtyp - ignored
**                  msgFlg - ignored
**
** Returns          0,  if successful
**                  -1, if invalid parameter passed
**
*******************************************************************************/
int dal4NfcMsgRcv(intptr_t msqId, NciHalMessage_t *pMsg, long pMsgtyp,
                     int msgFlg) {
    Dal4NfcMessageQueue_t *pQueue;
    Dal4NfcMessageQueueItem_t *p;
    UNUSED_PROP(msgFlg);
    UNUSED_PROP(pMsgtyp);
    if ((msqId == 0) || (pMsg == NULL)) {
        return -1;
    }

    pQueue = (Dal4NfcMessageQueue_t *)msqId;

    if (-1 == sem_wait(&pQueue->processSemaphore)) {
        TMSLOG_TML_E("sem_wait didn't return success\n");
    }

    pthread_mutex_lock(&pQueue->criticalSectionMutex);

    if (pQueue->pItems != NULL) {
        memcpy(pMsg, &(pQueue->pItems)->msg, sizeof(NciHalMessage_t));
        p = pQueue->pItems->pNext;
        free(pQueue->pItems);
        pQueue->pItems = p;
    }
    pthread_mutex_unlock(&pQueue->criticalSectionMutex);

    return 0;
}
