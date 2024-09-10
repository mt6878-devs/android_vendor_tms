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

#include "CondVar.h"

#include <errno.h>
#include <string.h>

#include <tmslog.h>

#ifdef TAG
#undef TAG
#endif
#define TAG g_tag

static const char g_tag[] = "CondVar";

/*******************************************************************************
**
** Function:        CondVar
**
** Description:     Initialize member variables.
**
** Returns:         None.
**
*******************************************************************************/
CondVar::CondVar()
{
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    (void)memset_s(&mCondition, sizeof(mCondition), 0, sizeof(mCondition));
    int const res = pthread_cond_init(&mCondition, &attr);
    if (res) {
        TMS_LOG_E(g_tag, "%s: fail init; error=0x%X", __FUNCTION__, res);
    }
}

/*******************************************************************************
**
** Function:        ~CondVar
**
** Description:     Cleanup all resources.
**
** Returns:         None.
**
*******************************************************************************/
CondVar::~CondVar()
{
    int const res = pthread_cond_destroy(&mCondition);
    if (res) {
        TMS_LOG_E(g_tag, "%s: fail destroy; error=0x%X", __FUNCTION__, res);
    }
}

/*******************************************************************************
**
** Function:        wait
**
** Description:     Block the caller and wait for a condition.
**
** Returns:         None.
**
*******************************************************************************/
void CondVar::wait(Mutex& mutex)
{
    int const res = pthread_cond_wait(&mCondition, mutex.nativeHandle());
    if (res) {
        TMS_LOG_E(g_tag, "%s: fail wait; error=0x%X", __FUNCTION__, res);
    }
}

/*******************************************************************************
**
** Function:        wait
**
** Description:     Block the caller and wait for a condition.
**                  millisec: Timeout in milliseconds.
**
** Returns:         True if wait is successful; false if timeout occurs.
**
*******************************************************************************/
bool CondVar::wait(Mutex& mutex, long millisec)
{
    bool retVal = false;
    struct timespec absoluteTime;

    if (clock_gettime(CLOCK_MONOTONIC, &absoluteTime) == -1) {
        TMS_LOG_E(g_tag, "%s: fail get time; errno=0x%X", __FUNCTION__, errno);
    } else {
        absoluteTime.tv_sec += millisec / 1000;
        long ns = absoluteTime.tv_nsec + ((millisec % 1000) * 1000000);
        if (ns > 1000000000) {
            absoluteTime.tv_sec++;
            absoluteTime.tv_nsec = ns - 1000000000;
        } else {
            absoluteTime.tv_nsec = ns;
        }
    }

    int waitResult =
        pthread_cond_timedwait(&mCondition, mutex.nativeHandle(), &absoluteTime);
    if ((waitResult != 0) && (waitResult != ETIMEDOUT)) {
        TMS_LOG_E(g_tag, "%s: fail timed wait; error=0x%X", __FUNCTION__, waitResult);
    }
    retVal = (waitResult == 0);  // waited successfully
    return retVal;
}

/*******************************************************************************
**
** Function:        notifyOne
**
** Description:     Unblock the waiting thread.
**
** Returns:         None.
**
*******************************************************************************/
void CondVar::notifyOne()
{
    int const res = pthread_cond_signal(&mCondition);
    if (res) {
        TMS_LOG_E(g_tag, "%s: fail signal; error=0x%X", __FUNCTION__, res);
    }
}
