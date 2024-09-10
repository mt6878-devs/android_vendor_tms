/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <thread>
#include "Nfc.h"
#include "extns/impl/nfc/1/TmsNfc.h"

using ::aidl::android::hardware::nfc::Nfc;
using ::aidl::vendor::tms::tmsnfc_aidl::TmsNfc;

void startTmsNfcAidlService()
{
    ALOGI("start tms nfc extn aidl hal service.");
    std::shared_ptr<TmsNfc> tms_nfc_service = ndk::SharedRefBase::make<TmsNfc>();
    const std::string instance =
        std::string() + TmsNfc::descriptor + "/default";
    ALOGI("register tms nfc extn aidl hal service name : %s", instance.c_str());
    binder_status_t status = AServiceManager_addService(
        tms_nfc_service->asBinder().get(), instance.c_str());

    CHECK(status == STATUS_OK);
    ALOGI(" tms nfc extn aidl hal service is ready");
    // ABinderProcess_joinThreadPool();
}

int main()
{
    ALOGI("start tms nfc aidl hal service");

    if (!ABinderProcess_setThreadPoolMaxThreadCount(2))
    {
        ALOGE("failed to set thread pool max thread count");
        return 1;
    }
    if (isTmsChip())
    {
        std::shared_ptr<Nfc> nfc_service = ndk::SharedRefBase::make<Nfc>();
        const std::string instance = std::string() + Nfc::descriptor + "/default";
        binder_status_t status = AServiceManager_addService(
            nfc_service->asBinder().get(), instance.c_str());
        CHECK(status == STATUS_OK);

        ALOGI("tms nfc aidl hal service is ready");
    }

    // std::thread t1(startTmsNfcAidlService);
    startTmsNfcAidlService();
    ABinderProcess_joinThreadPool();
    return 0;
}
