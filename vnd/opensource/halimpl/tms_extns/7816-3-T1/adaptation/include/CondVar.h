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

#ifndef _TMS_COND_VAR_H_
#define _TMS_COND_VAR_H_

#include <pthread.h>
#include "Mutex.h"

class CondVar
{
  public:
    /*******************************************************************************
    **
    ** Function:        CondVar
    **
    ** Description:     Initialize member variables.
    **
    ** Returns:         None.
    **
    *******************************************************************************/
    CondVar();

    /*******************************************************************************
    **
    ** Function:        ~CondVar
    **
    ** Description:     Cleanup all resources.
    **
    ** Returns:         None.
    **
    *******************************************************************************/
    ~CondVar();

    /*******************************************************************************
    **
    ** Function:        wait
    **
    ** Description:     Block the caller and wait for a condition.
    **
    ** Returns:         None.
    **
    *******************************************************************************/
    void wait(Mutex& mutex);

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
    bool wait(Mutex& mutex, long millisec);

    /*******************************************************************************
    **
    ** Function:        notifyOne
    **
    ** Description:     Unblock the waiting thread.
    **
    ** Returns:         None.
    **
    *******************************************************************************/
    void notifyOne();

  private:
    pthread_cond_t mCondition;
};

#endif