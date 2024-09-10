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

#include "NfcAdaptation.h"
#include <android/hardware/nfc/1.0/types.h>
#include <hwbinder/ProcessState.h>
#ifdef USE_TMS_NFC
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#endif
#include <pthread.h>

#include "tmslog.h"

static const char g_tag[] = "EseHal-NfcAdaptation";

using android::OK;
using android::sp;
using android::status_t;

using android::hardware::ProcessState;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::hidl_vec;
using android::hardware::hidl_death_recipient;

#ifdef USE_C1
    sp<INxpNfc> NfcAdaptation::mHalExtenNfc = nullptr;
#else
#ifdef TMS_NFC_AIDL
    std::shared_ptr<ITmsNfcAidl> NfcAdaptation::mHalExtenNfcAidl = nullptr;
    std::string TMSNFC_AIDL_HAL_SERVICE_NAME = "vendor.tms.tmsnfc_aidl.ITmsNfc/default";
    ::ndk::ScopedAIBinder_DeathRecipient mNfcAidlDeathRecipient;
#else
    android::sp<ITmsNfcHidl> NfcAdaptation::mHalExtenNfc = nullptr;
#endif

#endif


NfcAdaptation *NfcAdaptation::mpInstance = NULL;
ThreadMutex NfcAdaptation::sLock;

class NfcDeathRecipient : public hidl_death_recipient
{
    virtual void serviceDied(
        __attribute__((unused))uint64_t cookie,
        __attribute__((unused))const android::wp<::android::hidl::base::V1_0::IBase>& who)
    {
        // Deal with the fact that the service died
        TMS_LOG_E(g_tag, "NfcDeathRecipient: serviceDied");
        NfcAdaptation::GetInstance().NfcAdaptation::Deinit();
        usleep(1000 * 200);//200ms
        NfcAdaptation::GetInstance().Initialize();
    }
};

sp<NfcDeathRecipient> mNfcDeathRecipient = new NfcDeathRecipient();

#ifdef TMS_NFC_AIDL
void NfcAdaptation::HalAidlBinderDiedImpl() {
    TMS_LOG_E(g_tag, "ITmsNfc aidl hal died");
    NfcAdaptation::GetInstance().NfcAdaptation::Deinit();
    usleep(1000 * 200);//200ms
    NfcAdaptation::GetInstance().Initialize();
}

void NfcAdaptation::HalAidlBinderDied(void* cookie) {
    auto thiz = static_cast<NfcAdaptation*>(cookie);
    thiz->HalAidlBinderDiedImpl();
}
#endif

void NfcAdaptation::Initialize()
{
    const char *func = "NfcAdaptation::Initialize";
    TMS_LOG_D(g_tag, "%s", __FUNCTION__);
    AutoThreadMutex a(sLock);

    int count = 0;
#ifdef USE_C1
tryagain:
    if (mHalExtenNfc != nullptr) {
        TMS_LOG_D(g_tag, "%s successfully, exit", __FUNCTION__);
        return;
    }
    TMS_LOG_D(g_tag, "%s tryGetService %d", __FUNCTION__, count);
    mHalExtenNfc = INxpNfc::tryGetService();
    if (mHalExtenNfc != nullptr) {
        mHalExtenNfc->linkToDeath(mNfcDeathRecipient, 1418);
        TMS_LOG_E(g_tag, "%s: IExtenNfc::getService() returned %p (%s)",
                  func, mHalExtenNfc.get(),
                  (mHalExtenNfc->isRemote() ? "remote" : "local"));
    } else {
        usleep(1000 * 200);//200ms
        count++;
        if (count < 25) {//try within 5s
            goto tryagain;
        } else {
            TMS_LOG_E(g_tag, "%s failed", __FUNCTION__);
        }
    }
#else
tryagain:
#ifdef TMS_NFC_AIDL
    if (mHalExtenNfcAidl != nullptr ) {
#else
    if (mHalExtenNfc != nullptr) {
#endif
        TMS_LOG_D(g_tag, "%s try again successfully, exit", __FUNCTION__);
        return;
    }
    TMS_LOG_D(g_tag, "%s tryGetService %d", __FUNCTION__, count);
#ifdef TMS_NFC_AIDL
    ::ndk::SpAIBinder binder(
        AServiceManager_checkService(TMSNFC_AIDL_HAL_SERVICE_NAME.c_str()));
    if (binder != nullptr) {
        mHalExtenNfcAidl = ITmsNfcAidl::fromBinder(binder);
        if (mHalExtenNfcAidl != nullptr) {
            TMS_LOG_D(g_tag, "%s: ITmsNfcAidl::fromBinder returned", func);
            mNfcAidlDeathRecipient = ::ndk::ScopedAIBinder_DeathRecipient(
                AIBinder_DeathRecipient_new(NfcAdaptation::HalAidlBinderDied));
            AIBinder_linkToDeath(mHalExtenNfcAidl->asBinder().get(), mNfcAidlDeathRecipient.get(),
            this /* cookie */);
        } else {
            TMS_LOG_E(g_tag, "%s: Failed to retrieve the TmsNfc AIDL", func);
        }
    } else {
        TMS_LOG_E(g_tag, "%s: Faild to get tms nfc aidl service binder ", func);
    }
#else
    mHalExtenNfc = ITmsNfcHidl::tryGetService();
    if (mHalExtenNfc != nullptr) {
        mHalExtenNfc->linkToDeath(mNfcDeathRecipient, 1418);
            TMS_LOG_E(g_tag, "%s: IExtenNfc::getService() returned %p (%s)",
                func, mHalExtenNfc.get(),
                (mHalExtenNfc->isRemote() ? "remote" : "local"));
    } else {
        TMS_LOG_E(g_tag, "%s: IExtenNfc::getService() failed", func);
    }
#endif

#ifdef TMS_NFC_AIDL
    if (mHalExtenNfcAidl == nullptr) {
#else
    if (mHalExtenNfc == nullptr) {
#endif
        usleep(1000 * 200);//200ms
        count++;
        if (count < 25) {//try within 5s
            goto tryagain;
        } else {
            TMS_LOG_E(g_tag, "%s failed", __FUNCTION__);
        }
    }
#endif
    TMS_LOG_D(g_tag, "%s: exit", __FUNCTION__);
}
/*******************************************************************************
**
** Function:    NfcAdaptation::GetInstance()
**
** Description: access class singleton
**
** Returns:     pointer to the singleton object
**
*******************************************************************************/
NfcAdaptation& NfcAdaptation::GetInstance()
{
    AutoThreadMutex a(sLock);

    if (!mpInstance) {
        mpInstance = new NfcAdaptation;
    }
    return *mpInstance;
}

ThreadMutex& NfcAdaptation::GetLock()
{
    return sLock;
}

void NfcAdaptation::Deinit()
{
    TMS_LOG_D(g_tag, "NfcAdaptation::%s enter", __FUNCTION__);
    AutoThreadMutex a(NfcAdaptation::GetLock());
#ifdef TMS_NFC_AIDL
    mHalExtenNfcAidl = nullptr;
#else
    mHalExtenNfc = nullptr;
#endif
    TMS_LOG_D(g_tag, "NfcAdaptation::%s exit", __FUNCTION__);
}

ESESTATUS NfcAdaptation::EseSoftReset()
{
    ESESTATUS result = ESESTATUS_FAILED;
    bool ret = 0;
    TMS_LOG_D(g_tag, "NfcAdaptation::%s : enter", __FUNCTION__);

#ifdef TMS_NFC_AIDL
    if (mHalExtenNfcAidl != nullptr) {
        TMS_LOG_D(g_tag, "NfcAdaptation::aidl EseSoftReset");
        mHalExtenNfcAidl->EseSoftReset(&ret);
        if (ret) {
            TMS_LOG_E(g_tag, "NfcAdaptation::%s completed", __FUNCTION__);
            result = ESESTATUS_SUCCESS;
        } else {
            TMS_LOG_E(g_tag, "NfcAdaptation::%s failed", __FUNCTION__);
        }
        return result;
    } else {
        TMS_LOG_E(g_tag, "invalid aidl nfc extns");
    }
#else
    if (mHalExtenNfc != nullptr) {
        ret = mHalExtenNfc->EseSoftReset();
        if (ret) {
            TMS_LOG_E(g_tag, "NfcAdaptation::%s completed", __FUNCTION__);
            result = ESESTATUS_SUCCESS;
        } else {
            TMS_LOG_E(g_tag, "NfcAdaptation::%s failed", __FUNCTION__);
        }
    }
#endif
    return result;
}

/*******************************************************************************
**
** Function:    ThreadMutex::ThreadMutex()
**
** Description: class constructor
**
** Returns:     none
**
*******************************************************************************/
ThreadMutex::ThreadMutex()
{
    pthread_mutexattr_t mutexAttr;

    pthread_mutexattr_init(&mutexAttr);
    pthread_mutex_init(&mMutex, &mutexAttr);
    pthread_mutexattr_destroy(&mutexAttr);
}
/*******************************************************************************
**
** Function:    ThreadMutex::~ThreadMutex()
**
** Description: class destructor
**
** Returns:     none
**
*******************************************************************************/
ThreadMutex::~ThreadMutex()
{
    pthread_mutex_destroy(&mMutex);
}

/*******************************************************************************
**
** Function:    AutoThreadMutex::AutoThreadMutex()
**
** Description: class constructor, automatically lock the mutex
**
** Returns:     none
**
*******************************************************************************/
AutoThreadMutex::AutoThreadMutex(ThreadMutex& m) : mm(m)
{
    mm.lock();
}

/*******************************************************************************
**
** Function:    AutoThreadMutex::~AutoThreadMutex()
**
** Description: class destructor, automatically unlock the mutex
**
** Returns:     none
**
*******************************************************************************/
AutoThreadMutex::~AutoThreadMutex()
{
    mm.unlock();
}

/*******************************************************************************
**
** Function:    ThreadMutex::lock()
**
** Description: lock kthe mutex
**
** Returns:     none
**
*******************************************************************************/
void ThreadMutex::lock()
{
    pthread_mutex_lock(&mMutex);
}

/*******************************************************************************
**
** Function:    ThreadMutex::unblock()
**
** Description: unlock the mutex
**
** Returns:     none
**
*******************************************************************************/
void ThreadMutex::unlock()
{
    pthread_mutex_unlock(&mMutex);
}

/*******************************************************************************
**
** Function:    NfcAdaptation::NfcAdaptation()
**
** Description: class constructor
**
** Returns:     none
**
*******************************************************************************/
NfcAdaptation::NfcAdaptation()
{
}

/*******************************************************************************
**
** Function:    NfcAdaptation::~NfcAdaptation()
**
** Description: class destructor
**
** Returns:     none
**
*******************************************************************************/
NfcAdaptation::~NfcAdaptation()
{
    mpInstance = NULL;
}

/*******************************************************************************
**
** Function:    ThreadCondVar::ThreadCondVar()
**
** Description: class constructor
**
** Returns:     none
**
*******************************************************************************/
ThreadCondVar::ThreadCondVar()
{
    pthread_condattr_t CondAttr;

    pthread_condattr_init(&CondAttr);
    pthread_cond_init(&mCondVar, &CondAttr);

    pthread_condattr_destroy(&CondAttr);
}

/*******************************************************************************
**
** Function:    ThreadCondVar::~ThreadCondVar()
**
** Description: class destructor
**
** Returns:     none
**
*******************************************************************************/
ThreadCondVar::~ThreadCondVar()
{
    pthread_cond_destroy(&mCondVar);
}

#endif //defined (USE_TMS_NFC) || defined (USE_C1)
