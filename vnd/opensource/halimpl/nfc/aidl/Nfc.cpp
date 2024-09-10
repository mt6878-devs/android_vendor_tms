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

#include "Nfc.h"

#include <android-base/logging.h>
#include "NfcCompat.h"
#include "TmsLog.h"
#include "NfcStatus.h"

namespace aidl {
namespace android {
namespace hardware {
namespace nfc {

std::shared_ptr<INfcClientCallback> Nfc::mCallback = nullptr;
AIBinder_DeathRecipient* clientDeathRecipient = nullptr;

void OnDeath(void* cookie) {
  if (Nfc::mCallback != nullptr &&
      !AIBinder_isAlive(Nfc::mCallback->asBinder().get())) {
    LOG(INFO) << __func__ << " Nfc service has died";
    Nfc* nfc = static_cast<Nfc*>(cookie);
    nfc->close(NfcCloseType::DISABLE);
  }
}

::ndk::ScopedAStatus Nfc::open(
    const std::shared_ptr<INfcClientCallback>& clientCallback) {
    LOG(INFO) << "Nfc::open";
    if (clientCallback == nullptr) {
        LOG(INFO) << "Nfc::open null callback";
        return ndk::ScopedAStatus::fromServiceSpecificError(
            static_cast<int32_t>(NfcStatus::FAILED));
    }

    Nfc::mCallback = clientCallback;

    clientDeathRecipient = AIBinder_DeathRecipient_new(OnDeath);
    auto linkRet = AIBinder_linkToDeath(clientCallback->asBinder().get(),
        clientDeathRecipient, this /* cookie */);
    if (linkRet != STATUS_OK) {
        LOG(ERROR) << __func__ << ": linkToDeath failed: " << linkRet;
        // Just ignore the error.
    }
    tmsNciHalConfigAidlHalService();
    int ret = tmsNciHalOpen(eventCallback, dataCallback);
    LOG(INFO) << "Nfc::open Exit";

    return ret == NFCSTATUS_SUCCESS
        ? ndk::ScopedAStatus::ok()
        : ndk::ScopedAStatus::fromServiceSpecificError(
            static_cast<int32_t>(NfcStatus::FAILED));
}

::ndk::ScopedAStatus Nfc::close(NfcCloseType type) {
    LOG(INFO) << "Nfc::close";
    if (Nfc::mCallback == nullptr) {
        LOG(ERROR) << __func__ << "mCallback null";
        return ndk::ScopedAStatus::fromServiceSpecificError(
            static_cast<int32_t>(NfcStatus::FAILED));
    }

    int ret = 0;
    if (type == NfcCloseType::HOST_SWITCHED_OFF) {
        ret = tmsNciHalConfigDiscShutdown();
    } else {
        ret = tmsNciHalClose(false);
    }
    Nfc::mCallback = nullptr;
    AIBinder_DeathRecipient_delete(clientDeathRecipient);
    clientDeathRecipient = nullptr;
    LOG(INFO) << "Nfc::close Exit";
    return ret == NFCSTATUS_SUCCESS
        ? ndk::ScopedAStatus::ok()
        : ndk::ScopedAStatus::fromServiceSpecificError(
            static_cast<int32_t>(NfcStatus::FAILED));
}

::ndk::ScopedAStatus Nfc::coreInitialized() {
    LOG(INFO) << "Nfc::coreInitialized";
    if (Nfc::mCallback == nullptr) {
        LOG(ERROR) << __func__ << "mCallback null";
        return ndk::ScopedAStatus::fromServiceSpecificError(
            static_cast<int32_t>(NfcStatus::FAILED));
    }

    const uint16_t coreInitRspParamsLen = 33;
    uint8_t coreInitRspParams[coreInitRspParamsLen] = {
        0x40, 0x01, 0x1E, 0x00, 0x1A, 0x6E, 0x06, 0x00, 0x01, 0x00, 0x03,
        0xFF, 0xFF, 0x01, 0xFF, 0x00, 0x08, 0x00, 0x00, 0x01, 0x00, 0x02,
        0x00, 0x03, 0x00, 0x80, 0x00, 0x82, 0x00, 0x83, 0x00, 0x84, 0x00
    };
    int ret =  tmsNciHalCoreInitialized(coreInitRspParamsLen, coreInitRspParams);
	LOG(INFO) << "Nfc::coreInitialized exit";
    return ret == NFCSTATUS_SUCCESS
        ? ndk::ScopedAStatus::ok()
        : ndk::ScopedAStatus::fromServiceSpecificError(
            static_cast<int32_t>(NfcStatus::FAILED));
}

::ndk::ScopedAStatus Nfc::factoryReset() {
   LOG(INFO) << "factoryReset";
   tmsNciHalDoFactoryReset();
   LOG(INFO) << "factoryReset exit";
   return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Nfc::getConfig(NfcConfig* _aidl_return) {
    LOG(INFO) << "getConfig";
    NfcConfig nfcVendorConfig;
    NfcCompat nfcCompat;
    nfcCompat.tmsNciHalGetConfig(nfcVendorConfig);

    *_aidl_return = nfcVendorConfig;
	LOG(INFO) << "getConfig exit";
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Nfc::powerCycle() {
    LOG(INFO) << "powerCycle";
    if (Nfc::mCallback == nullptr) {
        LOG(ERROR) << __func__ << "mCallback null";
        return ndk::ScopedAStatus::fromServiceSpecificError(
            static_cast<int32_t>(NfcStatus::FAILED));
    }
    int ret = tmsNciHalPowerCycle();
	LOG(INFO) << "powerCycle exit";
    return ret == NFCSTATUS_SUCCESS
        ? ndk::ScopedAStatus::ok()
        : ndk::ScopedAStatus::fromServiceSpecificError(
            static_cast<int32_t>(NfcStatus::FAILED));
}

::ndk::ScopedAStatus Nfc::preDiscover() {
    LOG(INFO) << "preDiscover";
    if (Nfc::mCallback == nullptr) {
        LOG(ERROR) << __func__ << "mCallback null";
        return ndk::ScopedAStatus::fromServiceSpecificError(
            static_cast<int32_t>(NfcStatus::FAILED));
    }
    int ret = tmsNciHalPreDiscover();
    LOG(INFO) << "preDiscover exit";

    return ret == NFCSTATUS_SUCCESS
        ? ndk::ScopedAStatus::ok()
        : ndk::ScopedAStatus::fromServiceSpecificError(
            static_cast<int32_t>(NfcStatus::FAILED));
}

::ndk::ScopedAStatus Nfc::write(const std::vector<uint8_t>& data,
                                int32_t* _aidl_return) {

    if (Nfc::mCallback == nullptr) {
        LOG(ERROR) << __func__ << "mCallback null";
        return ndk::ScopedAStatus::fromServiceSpecificError(
            static_cast<int32_t>(NfcStatus::FAILED));
    }
    *_aidl_return = tmsNciHalWrite(data.size(), &data[0]);

    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Nfc::setEnableVerboseLogging(bool enable) {
    LOG(INFO) << "setVerboseLogging:" << enable;

    if (enable) {
        tmsLogEnableDisableLogLevel(0);
        tmsLogEnableDisableLogLevel(1);
    } else {
        tmsLogEnableDisableLogLevel(1);
        tmsLogEnableDisableLogLevel(0);
    }

    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Nfc::isVerboseLoggingEnabled(bool* _aidl_return) {
    *_aidl_return = *getNfcDebugEnabled();
    return ndk::ScopedAStatus::ok();
}

}  // namespace nfc
}  // namespace hardware
}  // namespace android
}  // namespace aidl
