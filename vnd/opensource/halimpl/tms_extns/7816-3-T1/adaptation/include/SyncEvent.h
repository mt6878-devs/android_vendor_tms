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

#ifndef _TMS_SYNC_EVENT_H_
#define _TMS_SYNC_EVENT_H_

#include "CondVar.h"
#include "Mutex.h"

class SyncEvent
{
  public:
    /*******************************************************************************
    **
    ** Function:        ~SyncEvent
    **
    ** Description:     Cleanup all resources.
    **
    ** Returns:         None.
    **
    *******************************************************************************/
    ~SyncEvent()
    {
    }

    /*******************************************************************************
    **
    ** Function:        start
    **
    ** Description:     Start a synchronization operation.
    **
    ** Returns:         None.
    **
    *******************************************************************************/
    void start()
    {
        mMutex.lock();
    }

    /*******************************************************************************
    **
    ** Function:        wait
    **
    ** Description:     Block the thread and wait for the event to occur.
    **
    ** Returns:         None.
    **
    *******************************************************************************/
    void wait()
    {
        mCondVar.wait(mMutex);
    }

    /*******************************************************************************
    **
    ** Function:        wait
    **
    ** Description:     Block the thread and wait for the event to occur.
    **                  millisec: Timeout in milliseconds.
    **
    ** Returns:         True if wait is successful; false if timeout occurs.
    **
    *******************************************************************************/
    bool wait(long millisec)
    {
        bool retVal = mCondVar.wait(mMutex, millisec);
        return retVal;
    }

    /*******************************************************************************
    **
    ** Function:        notifyOne
    **
    ** Description:     Notify a blocked thread that the event has occured.
    *Unblocks it.
    **
    ** Returns:         None.
    **
    *******************************************************************************/
    void notifyOne()
    {
        mCondVar.notifyOne();
    }

    /*******************************************************************************
    **
    ** Function:        end
    **
    ** Description:     End a synchronization operation.
    **
    ** Returns:         None.
    **
    *******************************************************************************/
    void end()
    {
        mMutex.unlock();
    }

  private:
    CondVar mCondVar;
    Mutex mMutex;
};

/*****************************************************************************/
/*****************************************************************************/

/*****************************************************************************
**
**  Name:           SyncEventGuard
**
**  Description:    Automatically start and end a synchronization event.
**
*****************************************************************************/
class SyncEventGuard
{
  public:
    /*******************************************************************************
    **
    ** Function:        SyncEventGuard
    **
    ** Description:     Start a synchronization operation.
    **
    ** Returns:         None.
    **
    *******************************************************************************/
    SyncEventGuard(SyncEvent& event) : mEvent(event)
    {
        event.start();  // automatically start operation
    };

    /*******************************************************************************
    **
    ** Function:        ~SyncEventGuard
    **
    ** Description:     End a synchronization operation.
    **
    ** Returns:         None.
    **
    *******************************************************************************/
    ~SyncEventGuard()
    {
        mEvent.end();  // automatically end operation
    };

  private:
    SyncEvent& mEvent;
};


#endif