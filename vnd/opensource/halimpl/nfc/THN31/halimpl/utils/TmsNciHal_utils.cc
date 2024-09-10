/*
 *
 *  Copyright 2013-2021 NXP
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

#include <errno.h>
#include <pthread.h>
#include <log/log.h>

#include <TmsLog.h>
#include <TmsNciHal.h>
#include <TmsNciHal_utils.h>

/*********************** Link list functions **********************************/

/*******************************************************************************
**
** Function         listInit
**
** Description      List initialization
**
** Returns          1, if list initialized, 0 otherwise
**
*******************************************************************************/
int listInit(struct listHead *pList) {
    pList->pFirst = NULL;
    if (pthread_mutex_init(&pList->mutex, NULL) != 0) {
        TMSLOG_NCIHAL_E("Mutex creation failed (errno=0x%08x)", errno);
        return 0;
    }

    return 1;
}

/*******************************************************************************
**
** Function         listDestroy
**
** Description      List destruction
**
** Returns          1, if list destroyed, 0 if failed
**
*******************************************************************************/
int listDestroy(struct listHead *pList) {
    int listNotEmpty = 1;
    while (listNotEmpty) {
        listNotEmpty = listGetAndRemoveNext(pList, NULL);
    }

    if (pthread_mutex_destroy(&pList->mutex) == -1) {
        TMSLOG_NCIHAL_E("Mutex destruction failed (errno=0x%08x)", errno);
        return 0;
    }

    return 1;
}

/*******************************************************************************
**
** Function         listAdd
**
** Description      Add a node to the list
**
** Returns          1, if added, 0 if otherwise
**
*******************************************************************************/
int listAdd(struct listHead *pList, void *pData) {
    struct listNode *pNode;
    struct listNode *pLastNode;
    int result;

    /* Create node */
    pNode = (struct listNode *)malloc(sizeof(struct listNode));
    if (pNode == NULL) {
        result = 0;
        TMSLOG_NCIHAL_E("Failed to malloc");
        goto clean_and_return;
    }
    pNode->pData = pData;
    pNode->pNext = NULL;
    pthread_mutex_lock(&pList->mutex);

    /* Add the node to the list */
    if (pList->pFirst == NULL) {
        /* Set the node as the head */
        pList->pFirst = pNode;
    } else {
        /* Seek to the end of the list */
        pLastNode = pList->pFirst;
        while (pLastNode->pNext != NULL) {
            pLastNode = pLastNode->pNext;
        }

        /* Add the node to the current list */
        pLastNode->pNext = pNode;
    }

    result = 1;

clean_and_return:
    pthread_mutex_unlock(&pList->mutex);
    return result;
}

/*******************************************************************************
**
** Function         listRemove
**
** Description      Remove node from the list
**
** Returns          1, if removed, 0 if otherwise
**
*******************************************************************************/
int listRemove(struct listHead *pList, void *pData) {
    struct listNode *pNode;
    struct listNode *pRemovedNode;
    int result;

    pthread_mutex_lock(&pList->mutex);

    if (pList->pFirst == NULL) {
        /* Empty list */
        TMSLOG_NCIHAL_D("Failed to deallocate (list empty)");
        result = 0;
        goto clean_and_return;
    }

    pNode = pList->pFirst;
    if (pList->pFirst->pData == pData) {
        /* Get the removed node */
        pRemovedNode = pNode;

        /* Remove the first node */
        pList->pFirst = pList->pFirst->pNext;
    } else {
        while (pNode->pNext != NULL) {
            if (pNode->pNext->pData == pData) {
                /* Node found ! */
                break;
            }
            pNode = pNode->pNext;
        }

        if (pNode->pNext == NULL) {
            /* Node not found */
            result = 0;
            TMSLOG_NCIHAL_E("Failed to deallocate (not found %8p)", pData);
            goto clean_and_return;
        }

        /* Get the removed node */
        pRemovedNode = pNode->pNext;

        /* Remove the node from the list */
        pNode->pNext = pNode->pNext->pNext;
    }

    /* Deallocate the node */
    free(pRemovedNode);

    result = 1;

clean_and_return:
    pthread_mutex_unlock(&pList->mutex);
    return result;
}

/*******************************************************************************
**
** Function         listGetAndRemoveNext
**
** Description      Get next node on the list and remove it
**
** Returns          1, if successful, 0 if otherwise
**
*******************************************************************************/
int listGetAndRemoveNext(struct listHead *pList, void **ppData) {
    struct listNode *pNode;
    int result;

    pthread_mutex_lock(&pList->mutex);

    if (pList->pFirst == NULL) {
        /* Empty list */
        TMSLOG_NCIHAL_D("Failed to deallocate (list empty)");
        result = 0;
        goto clean_and_return;
    }

    /* Work on the first node */
    pNode = pList->pFirst;

    /* Return the data */
    if (ppData != NULL) {
        *ppData = pNode->pData;
    }

    /* Remove and deallocate the node */
    pList->pFirst = pNode->pNext;
    free(pNode);

    result = 1;

clean_and_return:
    listDump(pList);
    pthread_mutex_unlock(&pList->mutex);
    return result;
}

/*******************************************************************************
**
** Function         listDump
**
** Description      Dump list information
**
** Returns          None
**
*******************************************************************************/
void listDump(struct listHead *pList) {
    struct listNode *pNode = pList->pFirst;

    TMSLOG_NCIHAL_D("Node dump:");
    while (pNode != NULL) {
        TMSLOG_NCIHAL_D("- %8p (%8p)", pNode, pNode->pData);
        pNode = pNode->pNext;
    }

    return;
}

/* END Linked list source code */

/****************** Semaphore and mutex helper functions **********************/

static TmsNciHalMonitor_t *gpTmsNciHalMonitor = NULL;

/*******************************************************************************
**
** Function         tmsNciHalInitMonitor
**
** Description      initialize the semaphore monitor
**
** Returns          Pointer to monitor, otherwise NULL if failed
**
*******************************************************************************/
TmsNciHalMonitor_t *tmsNciHalInitMonitor(void) {
    TMSLOG_NCIHAL_D("Entering tmsNciHalInitMonitor");

    if (gpTmsNciHalMonitor == NULL) {
        gpTmsNciHalMonitor =
            (TmsNciHalMonitor_t *)malloc(sizeof(TmsNciHalMonitor_t));
    }

    if (gpTmsNciHalMonitor != NULL) {
        memset(gpTmsNciHalMonitor, 0x00, sizeof(TmsNciHalMonitor_t));

        if (pthread_mutex_init(&gpTmsNciHalMonitor->reEntranceMutex, NULL) != 0) {
            TMSLOG_NCIHAL_E("reEntranceMutex creation returned 0x%08x", errno);
            goto clean_and_return;
        }

        if (pthread_mutex_init(&gpTmsNciHalMonitor->conCurrencyMutex, NULL) != 0) {
            TMSLOG_NCIHAL_E("conCurrencyMutex creation returned 0x%08x", errno);
            pthread_mutex_destroy(&gpTmsNciHalMonitor->reEntranceMutex);
            goto clean_and_return;
        }

        if (listInit(&gpTmsNciHalMonitor->semList) != 1) {
            TMSLOG_NCIHAL_E("Semaphore List creation failed");
            pthread_mutex_destroy(&gpTmsNciHalMonitor->conCurrencyMutex);
            pthread_mutex_destroy(&gpTmsNciHalMonitor->reEntranceMutex);
            goto clean_and_return;
        }
    } else {
        TMSLOG_NCIHAL_E("tmshal_monitor creation failed");
        goto clean_and_return;
    }

    TMSLOG_NCIHAL_D("Returning with SUCCESS");

    return gpTmsNciHalMonitor;

clean_and_return:
    TMSLOG_NCIHAL_D("Returning with FAILURE");

    if (gpTmsNciHalMonitor != NULL) {
        free(gpTmsNciHalMonitor);
        gpTmsNciHalMonitor = NULL;
    }

    return NULL;
}

/*******************************************************************************
**
** Function         tmsNciHalCleanupMonitor
**
** Description      Clean up semaphore monitor
**
** Returns          None
**
*******************************************************************************/
void tmsNciHalCleanupMonitor(void) {
    if (gpTmsNciHalMonitor != NULL) {
        pthread_mutex_destroy(&gpTmsNciHalMonitor->conCurrencyMutex);
        REENTRANCE_UNLOCK();
        pthread_mutex_destroy(&gpTmsNciHalMonitor->reEntranceMutex);
        tmsNciHalReleaseAllCbData();
        listDestroy(&gpTmsNciHalMonitor->semList);
    }

    free(gpTmsNciHalMonitor);
    gpTmsNciHalMonitor = NULL;

    return;
}

/*******************************************************************************
**
** Function         tmsNciHalGetMonitor
**
** Description      Get monitor
**
** Returns          Pointer to monitor
**
*******************************************************************************/
TmsNciHalMonitor_t *tmsNciHalGetMonitor(void) {
    if (gpTmsNciHalMonitor == NULL) {
        TMSLOG_NCIHAL_E("gpTmsNciHalMonitor is null");
    }
    return gpTmsNciHalMonitor;
}

/* initialize the callback data */
NFCSTATUS tmsNciHalInitCbData(TmsNciHalSem_t *pCallbackData,
                                   void *pContext) {
    /* Create semaphore */
    if (sem_init(&pCallbackData->sem, 0, 0) == -1) {
        TMSLOG_NCIHAL_E("Semaphore creation failed (errno=0x%08x)", errno);
        return NFCSTATUS_FAILED;
    }

    /* Set default status value */
    pCallbackData->status = NFCSTATUS_FAILED;

    /* Copy the context */
    pCallbackData->pContext = pContext;

    /* Add to active semaphore list */
    if (listAdd(&tmsNciHalGetMonitor()->semList, pCallbackData) != 1) {
        TMSLOG_NCIHAL_E("Failed to add the semaphore to the list");
    }

    return NFCSTATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         tmsNciHalCleanupCbData
**
** Description      Clean up callback data
**
** Returns          None
**
*******************************************************************************/
void tmsNciHalCleanupCbData(TmsNciHalSem_t *pCallbackData) {
    /* Destroy semaphore */
    if (sem_destroy(&pCallbackData->sem)) {
        TMSLOG_NCIHAL_E(
            "tmsNciHalCleanupCbData: Failed to destroy semaphore "
            "(errno=0x%08x)",
            errno);
    }

    /* Remove from active semaphore list */
    if (listRemove(&tmsNciHalGetMonitor()->semList, pCallbackData) != 1) {
        TMSLOG_NCIHAL_E(
            "tmsNciHalCleanupCbData: Failed to remove semaphore from the "
            "list");
    }

    return;
}

/*******************************************************************************
**
** Function         tmsNciHalReleaseAllCbData
**
** Description      Release all callback data
**
** Returns          None
**
*******************************************************************************/
void tmsNciHalReleaseAllCbData(void) {
    TmsNciHalSem_t *pCallbackData;

    while (listGetAndRemoveNext(&tmsNciHalGetMonitor()->semList,
                                (void **)&pCallbackData)) {
        pCallbackData->status = NFCSTATUS_FAILED;
        sem_post(&pCallbackData->sem);
    }

    return;
}

/* END Semaphore and mutex helper functions */

/**************************** Other functions *********************************/

/*******************************************************************************
**
** Function         tmsNciHalPrintPacket
**
** Description      Print packet
**
** Returns          None
**
*******************************************************************************/
void tmsNciHalPrintPacket(const char *pString, const uint8_t *pData,
                              uint16_t len) {
    uint32_t i;
    char *printBuffer = (char *)calloc((len * 3 + 1), sizeof(char));
    if (NULL != printBuffer) {
        for (i = 0; i < len; i++) {
            snprintf(&printBuffer[i * 2], 3, "%02X", pData[i]);
        }
        if (0 == memcmp(pString, "SEND", 0x04)) {
            TMSLOG_NCIX_D("len = %3d > %s", len, printBuffer);
        } else if (0 == memcmp(pString, "RECV", 0x04)) {
            TMSLOG_NCIR_D("len = %3d > %s", len, printBuffer);
        } else if (0 ==  memcmp(pString, "DEBUG", 0x05)) {
            TMSLOG_NCIHAL_D(" Debug Info > len = %3d > %s", len, printBuffer);
        }
        free(printBuffer);
    } else {
        TMSLOG_NCIX_E("\ntmsNciHal_print_packet:Failed to Allocate memory\n");
    }
    return;
}

/*******************************************************************************
**
** Function         tmsNciHalEmergencyRecovery
**
** Description      Abort the process in case of ESE_OVER_TEMP_ERROR, FW Assert, Watchdog Reset,
**                  Input Clock lost and unrecoverable error.
**                  Ignore the other status.
**
** Returns          None
**
*******************************************************************************/

void tmsNciHalEmergencyRecovery(uint8_t status) {
    TMSLOG_NCIHAL_D("%s: %d", __func__, status);

    switch (status) {
        case NCI2_0_CORE_RESET_TRIGGER_TYPE_OVER_TEMPERATURE:
        case CORE_RESET_TRIGGER_TYPE_FW_ASSERT:
        case CORE_RESET_TRIGGER_TYPE_WATCHDOG_RESET:
        case CORE_RESET_TRIGGER_TYPE_INPUT_CLOCK_LOST:
        case CORE_RESET_TRIGGER_TYPE_UNRECOVERABLE_ERROR: {
            TMSLOG_NCIHAL_E("abort()");
            abort();
        }
        case CORE_RESET_TRIGGER_TYPE_POWERED_ON: {
            if ((*getTmsNciHalCtrl()).halOpenStatus == true) {
                TMSLOG_NCIHAL_E("abort()");
                abort();
            }
        }
        break;
        default:
            TMSLOG_NCIHAL_E("%s: Core reset with Invalid status : %d ", __func__,
                            status);
            break;
    }
}
