/******************************************************************************
 *
 *  Copyright 2019-2021 NXP
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

#define LOG_TAG "tmsnfc@2.0-service"
#include <android/hardware/nfc/1.1/INfc.h>
#include <vendor/tms/tmsnfc/1.0/ITmsNfc.h>
#include <unistd.h>

#include <android-base/file.h>

#include <hidl/LegacySupport.h>
#include "Nfc.h"
#include "TmsNfc.h"
#include <TmsConfig.h>

// Generated HIDL files
using android::hardware::nfc::V1_2::INfc;
using android::hardware::nfc::V1_2::implementation::Nfc;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::sp;
using android::status_t;
using android::OK;
using vendor::tms::tmsnfc::V1_0::ITmsNfc;
using vendor::tms::tmsnfc::V1_0::implementation::TmsNfc;

int main() {
    status_t status;
    sp<INfc> nfcService = nullptr;
    sp<ITmsNfc> tmsNfcService = nullptr;

    try {
        if(!isTmsChip()) {
            ALOGE(" TMS NFC service unused");
            configureRpcThreadpool(1, false /*callerWillJoin*/);
        } else {
            ALOGD("NFC HAL Service 1.2 is starting.");
            nfcService = new Nfc();
            if (nfcService == nullptr) {
                ALOGE("Can not create an instance of NFC HAL Iface, exiting.");
                return -1;
            }

            configureRpcThreadpool(2, true /*callerWillJoin*/);
            status = nfcService->registerAsService();
            if (status != OK) {
                LOG_ALWAYS_FATAL("Could not register service for NFC HAL Iface (%d).",
                                status);
                return -1;
            }
        }
        tmsNfcService = new TmsNfc();
        if (tmsNfcService == nullptr) {
            ALOGE("Can not create an instance of TMS NFC Extn Iface, exiting.");
            return -1;
        }

        ALOGI("TMS NFC Extn Service 1.0 is starting.");
        status = tmsNfcService->registerAsService();
        if (status != OK) {
            ALOGE("Could not register service for TMS NFC Extn Iface (%d).",
                status);
        }

        ALOGI("NFC service is ready");
        joinRpcThreadpool();
    } catch (const std::length_error& le) {
    } catch (const std::__1::ios_base::failure& e) {
    } catch (std::__1::system_error& e) {
    }
    return 1;
}
