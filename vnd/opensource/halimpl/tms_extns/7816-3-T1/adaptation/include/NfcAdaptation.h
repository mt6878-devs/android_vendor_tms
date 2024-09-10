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

#ifndef _TMS_NFC_ADAPTATION_H_
#define _TMS_NFC_ADAPTATION_H_

#if defined (USE_TMS_NFC) || defined (USE_C1)

#include <pthread.h>

#include <android/hardware/nfc/1.0/types.h>
#include <utils/RefBase.h>
#include "tmsCommon.h"

#ifdef USE_C1
    #include <vendor/nxp/nxpnfc/2.0/INxpNfc.h>
    using vendor::nxp::nxpnfc::V2_0::INxpNfc;
#else
#ifdef TMS_NFC_AIDL
    #include <aidl/vendor/tms/tmsnfc_aidl/ITmsNfc.h>
    using ITmsNfcAidl = aidl::vendor::tms::tmsnfc_aidl::ITmsNfc;
#else
    #include <vendor/tms/tmsnfc/1.0/ITmsNfc.h>
    using ITmsNfcHidl = vendor::tms::tmsnfc::V1_0::ITmsNfc;
#endif
#endif

class ThreadMutex
{
  public:
    ThreadMutex();
    virtual ~ThreadMutex();
    void lock();
    void unlock();
    operator pthread_mutex_t *()
    {
        return &mMutex;
    }

  private:
    pthread_mutex_t mMutex;
};

class ThreadCondVar : public ThreadMutex
{
  public:
    ThreadCondVar();
    virtual ~ThreadCondVar();
    void signal();
    void wait();
    operator pthread_cond_t *()
    {
        return &mCondVar;
    }
    operator pthread_mutex_t *()
    {
        return ThreadMutex::operator pthread_mutex_t *();
    }

  private:
    pthread_cond_t mCondVar;
};

class AutoThreadMutex
{
  public:
    AutoThreadMutex(ThreadMutex& m);
    virtual ~AutoThreadMutex();
    operator ThreadMutex& ()
    {
        return mm;
    }
    operator pthread_mutex_t *()
    {
        return (pthread_mutex_t *)mm;
    }

  private:
    ThreadMutex& mm;
};

class NfcAdaptation
{
  public:
    virtual ~NfcAdaptation();
    void Initialize();
    void Deinit();
    static NfcAdaptation& GetInstance();
    static ThreadMutex& GetLock();
    ESESTATUS EseSoftReset();

  private:
    NfcAdaptation();
    static NfcAdaptation *mpInstance;
    static ThreadMutex sLock;
    ThreadCondVar mCondVar;
#ifdef USE_C1
    static android::sp<INxpNfc> mHalExtenNfc;
#else
#ifdef TMS_NFC_AIDL
    static std::shared_ptr<ITmsNfcAidl> mHalExtenNfcAidl;
    static void HalAidlBinderDied(void* cookie);
    void HalAidlBinderDiedImpl();
#else
    static android::sp<ITmsNfcHidl> mHalExtenNfc;
#endif
#endif
};

#endif //defined (USE_TMS_NFC) || defined (USE_C1)
#endif //_TMS_NFC_ADAPTATION_H_