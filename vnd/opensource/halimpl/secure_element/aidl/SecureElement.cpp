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

#define LOG_TAG "SecureElement1.2"
#include <tmslog.h>

#include <thread>
#include <stdlib.h>
#include <string.h>
#include "SecureElement.h"

static const char TAG[] = "SecureElement";

extern bool ese_debug_enabled;
static bool OpenLogicalChannelProcessing = false;
static bool OpenBasicChannelProcessing = false;

namespace aidl {
namespace android {
namespace hardware {
namespace secure_element {

using ::aidl::android::hardware::secure_element::LogicalChannelResponse;

std::shared_ptr<ISecureElementCallback> SecureElement::mCallback = nullptr;
AIBinder_DeathRecipient* clientDeathRecipient = nullptr;

SecureElement::SecureElement()
    : mOpenedchannelCount(0), mOpenedChannels{false, false, false, false} {}

void OnDeath(void* cookie) {
  (void)cookie;
  TMS_LOG_E(TAG, "%s: SecureElement serviceDied!!!", __func__);
  SecureElement* se = static_cast<SecureElement*>(cookie);
  int sestatus = se->seHalDeInit();

  if (sestatus != ESESTATUS_SUCCESS) {
    TMS_LOG_E(TAG, "%s: seHalDeInit Failed!!!", __func__);
  }
  if (se->mCallback != nullptr) {
    se->mCallback = nullptr;
  }

}

ScopedAStatus SecureElement::init(
  const std::shared_ptr<ISecureElementCallback>& clientCallback) {
  ESESTATUS status = ESESTATUS_SUCCESS;

  TMS_LOG_D(TAG, "%s: Enter", __func__);
  if (clientCallback == nullptr) {
    return ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);
  } else {
    mCallback = clientCallback;
    clientDeathRecipient = AIBinder_DeathRecipient_new(OnDeath);
    auto ret =
        AIBinder_linkToDeath(clientCallback->asBinder().get(),
                             clientDeathRecipient, this /* cookie */);
    if (ret != STATUS_OK) {
      TMS_LOG_E(TAG, "%s: linkToDeath failed:%d", __func__, ret);
      // Just ignore the error.
    }
  }

  if (isSeInitialized()) {
    mCallback->onStateChange(true, "SE already initialized");
    return ScopedAStatus::ok();
  }

  status = seHalInit();
  //Ignore init status, deinit it. Init is only check SE has been ready.
  seDeInit();

  if (status != ESESTATUS_SUCCESS) {
    mCallback->onStateChange(false, "SE initialization failed");
    trySeInit();
    return ScopedAStatus::ok();
  } else {
    mCallback->onStateChange(true, "SE initialized");
    return ScopedAStatus::ok();
  }
}

ScopedAStatus SecureElement::getAtr(std::vector<uint8_t>* _aidl_return) {
  TMS_LOG_D(TAG, "%s: Enter", __func__);
  std::vector<uint8_t> response;
  SeData atr;
  bool isInited = false;

  if (!isSeInitialized()) {
    TMS_LOG_D(TAG, "%s: Enter SeInitialized", __func__);
    ESESTATUS status = seHalInit();
    if (status != ESESTATUS_SUCCESS) {
      TMS_LOG_E(TAG, "%s: seHalInit Failed!!!", __func__);
      *_aidl_return = response;
      OpenLogicalChannelProcessing = false;
      return ScopedAStatus::ok();
    }
    isInited = true;
  }

  memset(&atr, 0x00, sizeof(SeData));
  ESESTATUS status = seGetATR(&atr);
  if ((ESESTATUS_SUCCESS == status)
      && (atr.pData != nullptr) && (atr.len != 0)) {
    response.resize(atr.len);
    memcpy(&response[0], atr.pData, atr.len);
  }

  if (isInited) {
    seHalDeInit();
  }

  if (NULL != atr.pData) {
    free(atr.pData);
  }
  *_aidl_return = response;
  return ScopedAStatus::ok();
}

ScopedAStatus SecureElement::isCardPresent(bool* _aidl_return) {
  TMS_LOG_D(TAG, "%s: Enter", __func__);
  *_aidl_return = true;
  return ScopedAStatus::ok();
}

ScopedAStatus SecureElement::transmit(const std::vector<uint8_t>& data,
                                      std::vector<uint8_t>* _aidl_return) {
  ESESTATUS status = ESESTATUS_FAILED;
  SeData cmdApdu;
  SeData rspApdu;
  memset(&cmdApdu, 0x00, sizeof(SeData));
  memset(&rspApdu, 0x00, sizeof(SeData));
  int sestatus = ISecureElement::FAILED;
  TMS_LOG_D(TAG, "%s: Enter", __func__);
  cmdApdu.len = data.size();
  if (cmdApdu.len >= MIN_APDU_LENGTH) {
    cmdApdu.pData = (uint8_t*)malloc(data.size() * sizeof(uint8_t));
    memcpy(cmdApdu.pData, data.data(), cmdApdu.len);
    status = seTransceive(&cmdApdu, &rspApdu);
  }

  std::vector<uint8_t> result;
  if (status != ESESTATUS_SUCCESS) {
    TMS_LOG_E(TAG, "%s: transmit failed!!!", __func__);
    if (!mOpenedchannelCount) {
      TMS_LOG_E(TAG, "%s: no channel opened", __func__);
      sestatus = ISecureElement::CHANNEL_NOT_AVAILABLE;
    }
    seHalResetSe();
  } else {
    sestatus = ESESTATUS_SUCCESS;
    result.resize(rspApdu.len);
    memcpy(&result[0], rspApdu.pData, rspApdu.len);
  }
  *_aidl_return = result;
  if (NULL != cmdApdu.pData) {
    free(cmdApdu.pData);
  }
  if (NULL != rspApdu.pData) {
    free(rspApdu.pData);
  }
  return sestatus == ESESTATUS_SUCCESS
             ? ndk::ScopedAStatus::ok()
             : ndk::ScopedAStatus::fromServiceSpecificError(sestatus);;
}

ScopedAStatus SecureElement::openLogicalChannel(
    const std::vector<uint8_t>& aid, int8_t p2,
    ::aidl::android::hardware::secure_element::LogicalChannelResponse*
    _aidl_return) {
  std::vector<uint8_t> manageChannelCommand = {0x00, 0x70, 0x00, 0x00, 0x01};
  OpenLogicalChannelProcessing = true;
  LogicalChannelResponse resApduBuff;
  resApduBuff.channelNumber = 0xff;
  memset(&resApduBuff, 0x00, sizeof(resApduBuff));
  TMS_LOG_D(TAG, "%s: Enter", __func__);

  bool isInited = false;
  if (!isSeInitialized()) {
    TMS_LOG_D(TAG, "%s: Enter SeInitialized", __func__);
    ESESTATUS status = seHalInit();
    if (status != ESESTATUS_SUCCESS) {
      TMS_LOG_E(TAG, "%s: seHalInit Failed!!!", __func__);
      OpenLogicalChannelProcessing = false;
      *_aidl_return = resApduBuff;
      return ScopedAStatus::fromServiceSpecificError(IOERROR);
    }
    isInited = true;
  }

  int sestatus = ISecureElement::IOERROR;
  ESESTATUS status = ESESTATUS_FAILED;
  SeData cmdApdu;
  SeData rspApdu;

  memset(&cmdApdu, 0x00, sizeof(SeData));
  memset(&rspApdu, 0x00, sizeof(SeData));

  cmdApdu.len = manageChannelCommand.size();
  cmdApdu.pData =
      (uint8_t*)malloc(manageChannelCommand.size() * sizeof(uint8_t));
  if (cmdApdu.pData != NULL) {
    memcpy(cmdApdu.pData, manageChannelCommand.data(), cmdApdu.len);
    status = seTransceive(&cmdApdu, &rspApdu);
  }
  if (status != ESESTATUS_SUCCESS) {
    /*Transceive failed*/
    sestatus = ISecureElement::IOERROR;
  } else if (rspApdu.pData[rspApdu.len - 2] == 0x90 &&
             rspApdu.pData[rspApdu.len - 1] == 0x00) {
    /*ManageChannel successful*/
    resApduBuff.channelNumber = rspApdu.pData[0];
    mOpenedchannelCount++;
    mOpenedChannels[resApduBuff.channelNumber] = true;
    sestatus = ESESTATUS_SUCCESS;
  } else if (rspApdu.pData[rspApdu.len - 2] == 0x6A &&
             rspApdu.pData[rspApdu.len - 1] == 0x81) {
    sestatus = ISecureElement::CHANNEL_NOT_AVAILABLE;
  } else if (((rspApdu.pData[rspApdu.len - 2] == 0x6E) ||
              (rspApdu.pData[rspApdu.len - 2] == 0x6D)) &&
             rspApdu.pData[rspApdu.len - 1] == 0x00) {
    sestatus = ISecureElement::UNSUPPORTED_OPERATION;
  }
  /*Free the allocations*/
  free(cmdApdu.pData);
  cmdApdu.pData = NULL;
  free(rspApdu.pData);
  rspApdu.pData = NULL;
  if (sestatus != ESESTATUS_SUCCESS) {
    /* if the SE is unresponsive, reset it */
    if (sestatus == ISecureElement::IOERROR) {
      seHalResetSe();
    } else if (isInited) {
      seHalDeInit();
    }

    /*If manageChannel is failed in any of above cases
    send the callback and return*/
    *_aidl_return = resApduBuff;
    TMS_LOG_E(TAG, "%s: Exit - manage channel failed!!", __func__);
    OpenLogicalChannelProcessing = false;
    return ScopedAStatus::fromServiceSpecificError(sestatus);
  }

  TMS_LOG_D(TAG, "%s: Sending selectApdu", __func__);
  /*Reset variables if manageChannel is success*/
  sestatus = ISecureElement::IOERROR;
  status = ESESTATUS_FAILED;

  memset(&cmdApdu, 0x00, sizeof(SeData));
  memset(&rspApdu, 0x00, sizeof(SeData));

  cmdApdu.len = (int32_t)(6 + aid.size());
  cmdApdu.pData = (uint8_t*)malloc(cmdApdu.len * sizeof(uint8_t));
  if (cmdApdu.pData != NULL) {
    uint8_t xx = 0;
    cmdApdu.pData[xx++] = resApduBuff.channelNumber;
    cmdApdu.pData[xx++] = 0xA4;        // INS
    cmdApdu.pData[xx++] = 0x04;        // P1
    cmdApdu.pData[xx++] = p2;          // P2
    cmdApdu.pData[xx++] = aid.size();  // Lc
    memcpy(&cmdApdu.pData[xx], aid.data(), aid.size());
    cmdApdu.pData[xx + aid.size()] = 0x00;  // Le
    status = seTransceive(&cmdApdu, &rspApdu);
  }

  if (status != ESESTATUS_SUCCESS) {
    /*Transceive failed*/
    sestatus = ISecureElement::IOERROR;
  } else {
    uint8_t sw1 = rspApdu.pData[rspApdu.len - 2];
    uint8_t sw2 = rspApdu.pData[rspApdu.len - 1];
    /*Return response on success, empty vector on failure*/
    /*Status is success*/
    if (sw1 == 0x90 && sw2 == 0x00) {
      /*Copy the response including status word*/
      resApduBuff.selectResponse.resize(rspApdu.len);
      memcpy(&resApduBuff.selectResponse[0], rspApdu.pData, rspApdu.len);
      sestatus = ESESTATUS_SUCCESS;
    }
    /*AID provided doesn't match any applet on the secure element*/
    else if (sw1 == 0x6A && sw2 == 0x82) {
      sestatus = ISecureElement::NO_SUCH_ELEMENT_ERROR;
    }
    /*Operation provided by the P2 parameter is not permitted by the applet.*/
    else if (sw1 == 0x6A && sw2 == 0x86) {
      sestatus = ISecureElement::UNSUPPORTED_OPERATION;
    } else {
      sestatus = ISecureElement::UNSUPPORTED_OPERATION;
    }
  }

  if (sestatus != ESESTATUS_SUCCESS) {
    /* if the SE is unresponsive, reset it */
    if (sestatus == ISecureElement::IOERROR) {
      seHalResetSe();
    } else {
      TMS_LOG_E(TAG, "%s: Select APDU failed! Close channel..", __func__);
      OpenLogicalChannelProcessing = false;
      int closeChannelStatus =
          internalCloseChannel(resApduBuff.channelNumber);
      if (closeChannelStatus != ESESTATUS_SUCCESS) {
        TMS_LOG_E(TAG, "%s: closeChannel Failed", __func__);
      } else {
        resApduBuff.channelNumber = 0xff;
      }
    }
  }
  *_aidl_return = resApduBuff;
  free(cmdApdu.pData);
  free(rspApdu.pData);
  TMS_LOG_V(TAG, "%s: Exit", __func__);
  OpenLogicalChannelProcessing = false;
  return sestatus == ESESTATUS_SUCCESS
             ? ndk::ScopedAStatus::ok()
             : ndk::ScopedAStatus::fromServiceSpecificError(sestatus);
}

ScopedAStatus SecureElement::openBasicChannel(
    const std::vector<uint8_t>& aid, int8_t p2,
    std::vector<uint8_t>* _aidl_return) {
  std::vector<uint8_t> result;
  OpenBasicChannelProcessing = true;
  TMS_LOG_D(TAG, "%s: Enter", __func__);

  if (mOpenedChannels[0]) {
    TMS_LOG_E(TAG,"openBasicChannel failed, channel already opened");
    *_aidl_return = result;
    return ScopedAStatus::fromServiceSpecificError(UNSUPPORTED_OPERATION);
  }

  bool isInited = false;
  if (!isSeInitialized()) {
    ESESTATUS status = seHalInit();
    if (status != ESESTATUS_SUCCESS) {
      TMS_LOG_E(TAG, "%s: seHalInit Failed!!!", __func__);
      *_aidl_return = result;
      OpenBasicChannelProcessing = false;
      return ScopedAStatus::fromServiceSpecificError(IOERROR);
    }
    isInited = true;
  }

  int sestatus = ISecureElement::IOERROR;
  ESESTATUS status = ESESTATUS_FAILED;
  SeData cmdApdu;
  SeData rspApdu;

  memset(&cmdApdu, 0x00, sizeof(SeData));
  memset(&rspApdu, 0x00, sizeof(SeData));

  cmdApdu.len = (int32_t)(6 + aid.size());
  cmdApdu.pData = (uint8_t*)malloc(cmdApdu.len * sizeof(uint8_t));
  if (cmdApdu.pData != NULL) {
    uint8_t xx = 0;
    cmdApdu.pData[xx++] = 0x00;        // basic channel
    cmdApdu.pData[xx++] = 0xA4;        // INS
    cmdApdu.pData[xx++] = 0x04;        // P1
    cmdApdu.pData[xx++] = p2;          // P2
    cmdApdu.pData[xx++] = aid.size();  // Lc
    memcpy(&cmdApdu.pData[xx], aid.data(), aid.size());
    cmdApdu.pData[xx + aid.size()] = 0x00;  // Le

    status = seTransceive(&cmdApdu, &rspApdu);
  }

  if (status != ESESTATUS_SUCCESS) {
    /* Transceive failed */
    sestatus = ISecureElement::IOERROR;
  } else {
    uint8_t sw1 = rspApdu.pData[rspApdu.len - 2];
    uint8_t sw2 = rspApdu.pData[rspApdu.len - 1];
    /*Return response on success, empty vector on failure*/
    /*Status is success*/
    if (((sw1 == 0x90) && (sw2 == 0x00)) || (sw1 == 0x62) || (sw1 == 0x63)) {
      /*Copy the response including status word*/
      result.resize(rspApdu.len);
      memcpy(&result[0], rspApdu.pData, rspApdu.len);
      /*Set basic channel reference if it is not set */
      if (!mOpenedChannels[0]) {
        mOpenedChannels[0] = true;
        mOpenedchannelCount++;
      }
      sestatus = ESESTATUS_SUCCESS;
    }
    /*AID provided doesn't match any applet on the secure element*/
    else if (sw1 == 0x6A && sw2 == 0x82) {
      sestatus = ISecureElement::NO_SUCH_ELEMENT_ERROR;
    }
    /*Operation provided by the P2 parameter is not permitted by the applet.*/
    else if (sw1 == 0x6A && sw2 == 0x86) {
      sestatus = ISecureElement::UNSUPPORTED_OPERATION;
    } else {
      sestatus = ISecureElement::UNSUPPORTED_OPERATION;
    }
  }

  /* if the SE is unresponsive, reset it */
  if (sestatus == ISecureElement::IOERROR) {
    seHalResetSe();
  }

  if (sestatus != ESESTATUS_SUCCESS) {
    if (mOpenedChannels[0]) {
      OpenBasicChannelProcessing = false;
      int closeChannelStatus =
          internalCloseChannel(DEFAULT_BASIC_CHANNEL);
      if (closeChannelStatus != ESESTATUS_SUCCESS) {
        TMS_LOG_E(TAG, "%s: closeChannel Failed", __func__);
      }
    } else if (isInited) {
      seHalDeInit();
    }
  }
  *_aidl_return = result;
  free(cmdApdu.pData);
  free(rspApdu.pData);
  TMS_LOG_V(TAG, "%s: Exit", __func__);
  OpenBasicChannelProcessing = false;
  return sestatus == ESESTATUS_SUCCESS
             ? ScopedAStatus::ok()
             : ScopedAStatus::fromServiceSpecificError(sestatus);
}

int SecureElement::internalCloseChannel(uint8_t channelNumber) {

  ESESTATUS status = ESESTATUS_FAILED;
  int sestatus = ISecureElement::FAILED;

  SeData cmdApdu;
  SeData rspApdu;

  TMS_LOG_D(TAG, "%s: Enter : %d", __func__, channelNumber);

  if ((channelNumber < DEFAULT_BASIC_CHANNEL) ||
      (channelNumber >= MAX_LOGICAL_CHANNELS) ||
      (mOpenedChannels[channelNumber] == false)) {
    TMS_LOG_E(TAG, "%s: invalid channel!!!", __func__);
    sestatus = ISecureElement::FAILED;
  } else if (channelNumber > DEFAULT_BASIC_CHANNEL) {
    memset(&cmdApdu, 0x00, sizeof(SeData));
    memset(&rspApdu, 0x00, sizeof(SeData));
    cmdApdu.pData = (uint8_t*)malloc(5 * sizeof(uint8_t));
    if (cmdApdu.pData != NULL) {
      uint8_t xx = 0;

      cmdApdu.pData[xx++] = channelNumber;
      cmdApdu.pData[xx++] = 0x70;           // INS
      cmdApdu.pData[xx++] = 0x80;           // P1
      cmdApdu.pData[xx++] = channelNumber;  // P2
      cmdApdu.pData[xx++] = 0x00;           // Lc
      cmdApdu.len = xx;

      status = seTransceive(&cmdApdu, &rspApdu);
    }
    if (status != ESESTATUS_SUCCESS) {
      sestatus = ISecureElement::FAILED;
    } else if ((rspApdu.pData[rspApdu.len - 2] == 0x90) &&
               (rspApdu.pData[rspApdu.len - 1] == 0x00)) {
      sestatus = ESESTATUS_SUCCESS;
    } else {
      sestatus = ISecureElement::FAILED;
    }
    free(cmdApdu.pData);
    free(rspApdu.pData);
  }

  if ((channelNumber == DEFAULT_BASIC_CHANNEL) ||
      (sestatus == ESESTATUS_SUCCESS)) {
    TMS_LOG_D(TAG, "%s: Closing channel : %d is successful ", __func__,
                channelNumber);
    mOpenedChannels[channelNumber] = false;
    if (mOpenedchannelCount > 0) {
      mOpenedchannelCount--;
    }
  }

  /*If there are no channels remaining close secureElement*/
  TMS_LOG_D(TAG, "%s: has been opened channel count: %d", __func__, mOpenedchannelCount);
  if ((mOpenedchannelCount == 0) && !OpenLogicalChannelProcessing &&
      !OpenBasicChannelProcessing) {
    sestatus = seHalDeInit();
  } else {
    sestatus = ESESTATUS_SUCCESS;
  }

  TMS_LOG_V(TAG, "%s: Exit", __func__);
  return sestatus;
}

ScopedAStatus SecureElement::closeChannel(int8_t channelNumber) {
  int sestatus = internalCloseChannel(channelNumber);
  return sestatus == ESESTATUS_SUCCESS
             ? ScopedAStatus::ok()
             : ScopedAStatus::fromServiceSpecificError(sestatus);;
}

bool SecureElement::isSeInitialized() { return SeIsInitialized(); }

ESESTATUS SecureElement::seHalInit() {
  ESESTATUS status = ESESTATUS_SUCCESS;

  TMS_LOG_D(TAG, "%s: Enter", __func__);
  status = seInit(ESE_MODE_NORMAL);
  if (status != ESESTATUS_SUCCESS) {
    TMS_LOG_E(TAG, "%s: SecureElement open failed!!!", __func__);
  }
  TMS_LOG_V(TAG, "%s: Exit", __func__);
  return status;
}

void SecureElement::seHalResetSe() {
  ESESTATUS status = ESESTATUS_SUCCESS;

  TMS_LOG_D(TAG, "%s: Enter", __func__);
  if (!isSeInitialized()) {
    ESESTATUS status = seHalInit();
    if (status != ESESTATUS_SUCCESS) {
      TMS_LOG_E(TAG, "%s: seHalInit Failed!!!", __func__);
    }
  }

  if (status == ESESTATUS_SUCCESS) {
    mCallback->onStateChange(false, "reset the SE");

    status = seReset();
    if (status != ESESTATUS_SUCCESS) {
      TMS_LOG_E(TAG, "%s: SecureElement reset failed!!", __func__);
    } else {
      for (uint8_t xx = 0; xx < MAX_LOGICAL_CHANNELS; xx++) {
        mOpenedChannels[xx] = false;
      }
      mOpenedchannelCount = 0;
      mCallback->onStateChange(true, "SE initialized");
    }
  }
  TMS_LOG_V(TAG, "%s: Exit", __func__);
}

int SecureElement::seHalDeInit() {
  TMS_LOG_D(TAG, "%s: Enter", __func__);
  ESESTATUS status = ESESTATUS_SUCCESS;
  int sestatus = ISecureElement::FAILED;
  status = seDeInit();
  if (status != ESESTATUS_SUCCESS) {
    sestatus = ISecureElement::FAILED;
  } else {
    sestatus = ESESTATUS_SUCCESS;

    for (uint8_t xx = 0; xx < MAX_LOGICAL_CHANNELS; xx++) {
      mOpenedChannels[xx] = false;
    }
    mOpenedchannelCount = 0;
  }
  TMS_LOG_V(TAG, "%s: Exit", __func__);
  return sestatus;
}

ScopedAStatus SecureElement::reset() {
  ESESTATUS status = ESESTATUS_SUCCESS;
  int sestatus = ISecureElement::FAILED;

  TMS_LOG_D(TAG, "%s: Enter", __func__);
  if (!isSeInitialized()) {
    ESESTATUS status = seHalInit();
    if (status != ESESTATUS_SUCCESS) {
      TMS_LOG_E(TAG, "%s: seHalInit Failed!!!", __func__);
    }
  }

  if (status == ESESTATUS_SUCCESS) {
    mCallback->onStateChange(false, "reset the SE");

    status = seReset();
    if (status != ESESTATUS_SUCCESS) {
      TMS_LOG_E(TAG, "%s: SecureElement reset failed!!", __func__);
    } else {
      sestatus = ESESTATUS_SUCCESS;
      for (uint8_t xx = 0; xx < MAX_LOGICAL_CHANNELS; xx++) {
        mOpenedChannels[xx] = false;
      }
      mOpenedchannelCount = 0;
      mCallback->onStateChange(true, "SE initialized");
    }
  }
  TMS_LOG_V(TAG, "%s: Exit", __func__);

  return sestatus == ESESTATUS_SUCCESS
            ? ScopedAStatus::ok()
            : ScopedAStatus::fromServiceSpecificError(sestatus);;
}

void SecureElement::trySeInitThread() {
    TMS_LOG_D(TAG, "%s: enter", __func__);
    while (mTrySeInitCnt < MAX_SE_INIT_CNT) {
        mTrySeInitCnt++;
        seHalResetSe();
        if (isSeInitialized()) {
            seHalDeInit();
            mTrySeInitCnt = 0;
            break;
        }
        usleep(500); // sleep 500ms
        TMS_LOG_D(TAG, "%s: try count = %u", __func__, mTrySeInitCnt);
    }

    pthread_mutex_lock(&mMutex);
    mIsRunning = false;
    pthread_mutex_unlock(&mMutex);
    TMS_LOG_D(TAG, "%s: exit", __func__);
}
void SecureElement::trySeInit() {
    pthread_mutex_lock(&mMutex);
    if (mIsRunning) {
        return;
    } else {
        TMS_LOG_I(TAG, "%s: start", __FUNCTION__);
        mIsRunning = true;
        std::thread initThread(&SecureElement::trySeInitThread, std::ref(*this));
        initThread.detach();
    }
    pthread_mutex_unlock(&mMutex);

}

}  // namespace secure_element
}  // namespace hardware
}  // namespace android
}  // namespace aidl
