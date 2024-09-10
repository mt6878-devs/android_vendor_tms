/*
 * Copyright (c) 2021 Tsingteng MicroSystem
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
 */

#pragma once

#include <aidl/android/hardware/secure_element/BnSecureElement.h>
#include <aidl/android/hardware/secure_element/ISecureElementCallback.h>
#include "SEApi.h"

namespace aidl {
namespace android {
namespace hardware {
namespace secure_element {

using ::ndk::ICInterface;
using ndk::ScopedAStatus;

using ::android::sp;

using ::aidl::android::hardware::secure_element::ISecureElementCallback;

#ifndef MAX_LOGICAL_CHANNELS
#define MAX_LOGICAL_CHANNELS 0x04
#endif
#ifndef MIN_APDU_LENGTH
#define MIN_APDU_LENGTH 0x04
#endif
#ifndef DEFAULT_BASIC_CHANNEL
#define DEFAULT_BASIC_CHANNEL 0x00
#endif

#ifndef MAX_SE_INIT_CNT
#define MAX_SE_INIT_CNT 0x28
#endif

struct SecureElement : public BnSecureElement {

  SecureElement();
  ::ndk::ScopedAStatus closeChannel(int8_t in_channelNumber) override;
  ::ndk::ScopedAStatus getAtr(std::vector<uint8_t>* _aidl_return) override;
  ::ndk::ScopedAStatus init(
      const std::shared_ptr<
          ::aidl::android::hardware::secure_element::ISecureElementCallback>&
          in_clientCallback) override;
  ::ndk::ScopedAStatus isCardPresent(bool* _aidl_return) override;
  ::ndk::ScopedAStatus openBasicChannel(
      const std::vector<uint8_t>& in_aid, int8_t in_p2,
      std::vector<uint8_t>* _aidl_return) override;
  ::ndk::ScopedAStatus openLogicalChannel(
      const std::vector<uint8_t>& in_aid, int8_t in_p2,
      ::aidl::android::hardware::secure_element::LogicalChannelResponse*
          _aidl_return) override;
  ::ndk::ScopedAStatus reset() override;
  ::ndk::ScopedAStatus transmit(const std::vector<uint8_t>& in_data,
                                std::vector<uint8_t>* _aidl_return) override;
  int seHalDeInit();
  static std::shared_ptr<ISecureElementCallback> mCallback;

 private:
  uint8_t mOpenedchannelCount = 0;
  bool mOpenedChannels[MAX_LOGICAL_CHANNELS];

  uint8_t mTrySeInitCnt = 0;
  bool mIsRunning = false;
  pthread_mutex_t mMutex = PTHREAD_MUTEX_INITIALIZER;

  int internalCloseChannel(uint8_t channelNumber);
  ESESTATUS seHalInit();
  bool isSeInitialized();
  void seHalResetSe();
  void trySeInit();
  void trySeInitThread();
};

}  // namespace secure_element
}  // namespace hardware
}  // namespace android
}  // namespace aidl
