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

#ifndef _TMS_MUTEX_H_
#define _TMS_MUTEX_H_

#include <pthread.h>

class Mutex
{
  public:
    /*******************************************************************************
    **
    ** Function:        Mutex
    **
    ** Description:     Initialize member variables.
    **
    ** Returns:         None.
    **
    *******************************************************************************/
    Mutex();

    /*******************************************************************************
    **
    ** Function:        ~Mutex
    **
    ** Description:     Cleanup all resources.
    **
    ** Returns:         None.
    **
    *******************************************************************************/
    ~Mutex();

    /*******************************************************************************
    **
    ** Function:        lock
    **
    ** Description:     Block the thread and try lock the mutex.
    **
    ** Returns:         None.
    **
    *******************************************************************************/
    void lock();

    /*******************************************************************************
    **
    ** Function:        unlock
    **
    ** Description:     Unlock a mutex to unblock a thread.
    **
    ** Returns:         None.
    **
    *******************************************************************************/
    void unlock();

    /*******************************************************************************
    **
    ** Function:        tryLock
    **
    ** Description:     Try to lock the mutex.
    **
    ** Returns:         True if the mutex is locked.
    **
    *******************************************************************************/
    bool tryLock();

    /*******************************************************************************
    **
    ** Function:        nativeHandle
    **
    ** Description:     Get the handle of the mutex.
    **
    ** Returns:         Handle of the mutex.
    **
    *******************************************************************************/
    pthread_mutex_t *nativeHandle();

    class Autolock
    {
      public:
        inline Autolock(Mutex& mutex) : mLock(mutex)
        {
            mLock.lock();
        }
        inline Autolock(Mutex *mutex) : mLock(*mutex)
        {
            mLock.lock();
        }
        inline ~Autolock()
        {
            mLock.unlock();
        }

      private:
        Mutex& mLock;
    };

  private:
    pthread_mutex_t mMutex;
};

typedef Mutex::Autolock AutoMutex;

#endif