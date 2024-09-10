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

#if defined (USE_TMS_NFC) || defined (USE_C1)
    #include "tmsSecureString.h"
#else
    #include "securec.h"
#endif

#include "Mutex.h"

#include <errno.h>
#include <string.h>

#include <tmslog.h>

#ifdef TAG
#undef TAG
#endif
#define TAG g_tag

static const char g_tag[] = "Mutex";


/*******************************************************************************
**
** Function:        Mutex
**
** Description:     Initialize member variables.
**
** Returns:         None.
**
*******************************************************************************/
Mutex::Mutex()
{
    (void)memset_s(&mMutex, sizeof(mMutex), 0, sizeof(mMutex));
    int res = pthread_mutex_init(&mMutex, NULL);
    if (res != 0) {
        TMS_LOG_E(g_tag, "%s: fail init; error=0x%X", __FUNCTION__, res);
    }
}

/*******************************************************************************
**
** Function:        ~Mutex
**
** Description:     Cleanup all resources.
**
** Returns:         None.
**
*******************************************************************************/
Mutex::~Mutex()
{
    int res = pthread_mutex_destroy(&mMutex);
    if (res != 0) {
        TMS_LOG_E(g_tag, "%s: fail destroy; error=0x%X", __FUNCTION__, res);
    }
}

/*******************************************************************************
**
** Function:        lock
**
** Description:     Block the thread and try lock the mutex.
**
** Returns:         None.
**
*******************************************************************************/
void Mutex::lock()
{
    int res = pthread_mutex_lock(&mMutex);
    if (res != 0) {
        TMS_LOG_E(g_tag, "%s: fail lock; error=0x%X", __FUNCTION__, res);
    }
}

/*******************************************************************************
**
** Function:        unlock
**
** Description:     Unlock a mutex to unblock a thread.
**
** Returns:         None.
**
*******************************************************************************/
void Mutex::unlock()
{
    int res = pthread_mutex_unlock(&mMutex);
    if (res != 0) {
        TMS_LOG_E(g_tag, "%s: fail unlock; error=0x%X", __FUNCTION__, res);
    }
}

/*******************************************************************************
**
** Function:        tryLock
**
** Description:     Try to lock the mutex.
**
** Returns:         True if the mutex is locked.
**
*******************************************************************************/
bool Mutex::tryLock()
{
    int res = pthread_mutex_trylock(&mMutex);
    if ((res != 0) && (res != EBUSY)) {
        TMS_LOG_E(g_tag, "%s: error=0x%X", __FUNCTION__, res);
    }
    return res == 0;
}

/*******************************************************************************
**
** Function:        nativeHandle
**
** Description:     Get the handle of the mutex.
**
** Returns:         Handle of the mutex.
**
*******************************************************************************/
pthread_mutex_t *Mutex::nativeHandle()
{
    return &mMutex;
}
