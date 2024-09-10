/******************************************************************************
 *
 *  Copyright 2018 NXP
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

#define LOG_TAG "SecureElement1.0"
#include <tmslog.h>

#include <stdlib.h>
#include <string.h>
#include "SecureElement.h"

static const char TAG[] = "SecureElement1.0";

extern bool ese_debug_enabled;
static bool OpenLogicalChannelProcessing = false;
static bool OpenBasicChannelProcessing = false;

namespace android {
namespace hardware {
namespace secure_element {
namespace V1_0 {
namespace implementation {

sp<V1_0::ISecureElementHalCallback> SecureElement::mCallbackV1_0 = nullptr;

SecureElement::SecureElement()
    : mOpenedchannelCount(0), mOpenedChannels{false, false, false, false} {}

Return<void> SecureElement::init(
    const sp<
        ::android::hardware::secure_element::V1_0::ISecureElementHalCallback>&
        clientCallback) {
  ESESTATUS status = ESESTATUS_SUCCESS;
  TMS_LOG_D(TAG, "%s: Enter", __func__);
  if (clientCallback == nullptr) {
    return Void();
  } else {
    mCallbackV1_0 = clientCallback;
    if (!mCallbackV1_0->linkToDeath(this, 0 /*cookie*/)) {
      TMS_LOG_E(TAG, "%s: Failed to register death notification", __func__);
    }
  }

  if (isSeInitialized()) {
    clientCallback->onStateChange(true);
    return Void();
  }

  status = seHalInit();
  if (status != ESESTATUS_SUCCESS) {
    clientCallback->onStateChange(false);
    return Void();
  } else {
    clientCallback->onStateChange(true);
    return Void();
  }
}

Return<void> SecureElement::getAtr(getAtr_cb _hidl_cb) {
  TMS_LOG_D(TAG, "%s: Enter", __func__);
  hidl_vec<uint8_t> response;
  SeData atr;
  memset(&atr, 0x00, sizeof(SeData));
  ESESTATUS status = seGetATR(&atr);
  if ((ESESTATUS_SUCCESS == status)
      && (atr.pData != nullptr) && (atr.len != 0)) {
    response.resize(atr.len);
    memcpy(&response[0], atr.pData, atr.len);
  }
  if (NULL != atr.pData) {
    free(atr.pData);
  }
  _hidl_cb(response);
  return Void();
}

Return<bool> SecureElement::isCardPresent() { return true; }

Return<void> SecureElement::transmit(const hidl_vec<uint8_t>& data,
                                     transmit_cb _hidl_cb) {
  ESESTATUS status = ESESTATUS_FAILED;
  SeData cmdApdu;
  SeData rspApdu;
  memset(&cmdApdu, 0x00, sizeof(SeData));
  memset(&rspApdu, 0x00, sizeof(SeData));

  TMS_LOG_D(TAG, "%s: Enter", __func__);
  cmdApdu.len = data.size();
  if (cmdApdu.len >= MIN_APDU_LENGTH) {
    cmdApdu.pData = (uint8_t*)malloc(data.size() * sizeof(uint8_t));
    memcpy(cmdApdu.pData, data.data(), cmdApdu.len);
    status = seTransceive(&cmdApdu, &rspApdu);
  }

  hidl_vec<uint8_t> result;
  if (status != ESESTATUS_SUCCESS) {
    TMS_LOG_E(TAG, "%s: transmit failed!!!", __func__);
    seHalResetSe();
  } else {
    result.resize(rspApdu.len);
    memcpy(&result[0], rspApdu.pData, rspApdu.len);
  }
  _hidl_cb(result);
  free(cmdApdu.pData);
  free(rspApdu.pData);
  return Void();
}

Return<void> SecureElement::openLogicalChannel(const hidl_vec<uint8_t>& aid,
                                               uint8_t p2,
                                               openLogicalChannel_cb _hidl_cb) {
  hidl_vec<uint8_t> manageChannelCommand = {0x00, 0x70, 0x00, 0x00, 0x01};
  OpenLogicalChannelProcessing = true;
  LogicalChannelResponse resApduBuff;
  resApduBuff.channelNumber = 0xff;
  memset(&resApduBuff, 0x00, sizeof(resApduBuff));
  TMS_LOG_D(TAG, "%s: Enter", __func__);

  if (!isSeInitialized()) {
    TMS_LOG_D(TAG, "%s: Enter SeInitialized", __func__);
    ESESTATUS status = seHalInit();
    if (status != ESESTATUS_SUCCESS) {
      TMS_LOG_E(TAG, "%s: seHalInit Failed!!!", __func__);
      _hidl_cb(resApduBuff, SecureElementStatus::IOERROR);
      OpenLogicalChannelProcessing = false;
      return Void();
    }
  }

  SecureElementStatus sestatus = SecureElementStatus::IOERROR;
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
    sestatus = SecureElementStatus::IOERROR;
  } else if (rspApdu.pData[rspApdu.len - 2] == 0x90 &&
             rspApdu.pData[rspApdu.len - 1] == 0x00) {
    /*ManageChannel successful*/
    resApduBuff.channelNumber = rspApdu.pData[0];
    mOpenedchannelCount++;
    mOpenedChannels[resApduBuff.channelNumber] = true;
    sestatus = SecureElementStatus::SUCCESS;
  } else if (rspApdu.pData[rspApdu.len - 2] == 0x6A &&
             rspApdu.pData[rspApdu.len - 1] == 0x81) {
    sestatus = SecureElementStatus::CHANNEL_NOT_AVAILABLE;
  } else if (((rspApdu.pData[rspApdu.len - 2] == 0x6E) ||
              (rspApdu.pData[rspApdu.len - 2] == 0x6D)) &&
             rspApdu.pData[rspApdu.len - 1] == 0x00) {
    sestatus = SecureElementStatus::UNSUPPORTED_OPERATION;
  }
  /*Free the allocations*/
  free(cmdApdu.pData);
  cmdApdu.pData = NULL;
  free(rspApdu.pData);
  rspApdu.pData = NULL;
  if (sestatus != SecureElementStatus::SUCCESS) {
    /* if the SE is unresponsive, reset it */
    if (sestatus == SecureElementStatus::IOERROR) {
      seHalResetSe();
    }

    /*If manageChannel is failed in any of above cases
    send the callback and return*/
    _hidl_cb(resApduBuff, sestatus);
    TMS_LOG_E(TAG, "%s: Exit - manage channel failed!!", __func__);
    OpenLogicalChannelProcessing = false;
    return Void();
  }

  TMS_LOG_D(TAG, "%s: Sending selectApdu", __func__);
  /*Reset variables if manageChannel is success*/
  sestatus = SecureElementStatus::IOERROR;
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
    sestatus = SecureElementStatus::IOERROR;
  } else {
    uint8_t sw1 = rspApdu.pData[rspApdu.len - 2];
    uint8_t sw2 = rspApdu.pData[rspApdu.len - 1];
    /*Return response on success, empty vector on failure*/
    /*Status is success*/
    if (sw1 == 0x90 && sw2 == 0x00) {
      /*Copy the response including status word*/
      resApduBuff.selectResponse.resize(rspApdu.len);
      memcpy(&resApduBuff.selectResponse[0], rspApdu.pData, rspApdu.len);
      sestatus = SecureElementStatus::SUCCESS;
    }
    /*AID provided doesn't match any applet on the secure element*/
    else if (sw1 == 0x6A && sw2 == 0x82) {
      sestatus = SecureElementStatus::NO_SUCH_ELEMENT_ERROR;
    }
    /*Operation provided by the P2 parameter is not permitted by the applet.*/
    else if (sw1 == 0x6A && sw2 == 0x86) {
      sestatus = SecureElementStatus::UNSUPPORTED_OPERATION;
    }
  }

  if (sestatus != SecureElementStatus::SUCCESS) {
    /* if the SE is unresponsive, reset it */
    if (sestatus == SecureElementStatus::IOERROR) {
      seHalResetSe();
    } else {
      TMS_LOG_E(TAG, "%s: Select APDU failed! Close channel..", __func__);
      SecureElementStatus closeChannelStatus =
          closeChannel(resApduBuff.channelNumber);
      if (closeChannelStatus != SecureElementStatus::SUCCESS) {
        TMS_LOG_E(TAG, "%s: closeChannel Failed", __func__);
      } else {
        resApduBuff.channelNumber = 0xff;
      }
    }
  }
  _hidl_cb(resApduBuff, sestatus);
  free(cmdApdu.pData);
  free(rspApdu.pData);
  TMS_LOG_V(TAG, "%s: Exit", __func__);
  OpenLogicalChannelProcessing = false;
  return Void();
}

Return<void> SecureElement::openBasicChannel(const hidl_vec<uint8_t>& aid,
                                             uint8_t p2,
                                             openBasicChannel_cb _hidl_cb) {
  hidl_vec<uint8_t> result;
  OpenBasicChannelProcessing = true;
  TMS_LOG_D(TAG, "%s: Enter", __func__);

  if (!isSeInitialized()) {
    ESESTATUS status = seHalInit();
    if (status != ESESTATUS_SUCCESS) {
      TMS_LOG_E(TAG, "%s: seHalInit Failed!!!", __func__);
      _hidl_cb(result, SecureElementStatus::IOERROR);
      OpenBasicChannelProcessing = false;
      return Void();
    }
  }

  SecureElementStatus sestatus = SecureElementStatus::IOERROR;
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
    sestatus = SecureElementStatus::IOERROR;
  } else {
    uint8_t sw1 = rspApdu.pData[rspApdu.len - 2];
    uint8_t sw2 = rspApdu.pData[rspApdu.len - 1];
    /*Return response on success, empty vector on failure*/
    /*Status is success*/
    if ((sw1 == 0x90) && (sw2 == 0x00)) {
      /*Copy the response including status word*/
      result.resize(rspApdu.len);
      memcpy(&result[0], rspApdu.pData, rspApdu.len);
      /*Set basic channel reference if it is not set */
      if (!mOpenedChannels[0]) {
        mOpenedChannels[0] = true;
        mOpenedchannelCount++;
      }
      sestatus = SecureElementStatus::SUCCESS;
    }
    /*AID provided doesn't match any applet on the secure element*/
    else if (sw1 == 0x6A && sw2 == 0x82) {
      sestatus = SecureElementStatus::NO_SUCH_ELEMENT_ERROR;
    }
    /*Operation provided by the P2 parameter is not permitted by the applet.*/
    else if (sw1 == 0x6A && sw2 == 0x86) {
      sestatus = SecureElementStatus::UNSUPPORTED_OPERATION;
    }
  }

  /* if the SE is unresponsive, reset it */
  if (sestatus == SecureElementStatus::IOERROR) {
    seHalResetSe();
  }

  if ((sestatus != SecureElementStatus::SUCCESS) && mOpenedChannels[0]) {
    SecureElementStatus closeChannelStatus =
        closeChannel(DEFAULT_BASIC_CHANNEL);
    if (closeChannelStatus != SecureElementStatus::SUCCESS) {
      TMS_LOG_E(TAG, "%s: closeChannel Failed", __func__);
    }
  }
  _hidl_cb(result, sestatus);
  free(cmdApdu.pData);
  free(rspApdu.pData);
  TMS_LOG_V(TAG, "%s: Exit", __func__);
  OpenBasicChannelProcessing = false;
  return Void();
}

Return<::android::hardware::secure_element::V1_0::SecureElementStatus>
SecureElement::closeChannel(uint8_t channelNumber) {
  ESESTATUS status = ESESTATUS_FAILED;
  SecureElementStatus sestatus = SecureElementStatus::FAILED;

  SeData cmdApdu;
  SeData rspApdu;

  TMS_LOG_D(TAG, "%s: Enter : %d", __func__, channelNumber);

  if ((channelNumber < DEFAULT_BASIC_CHANNEL) ||
      (channelNumber >= MAX_LOGICAL_CHANNELS) ||
      (mOpenedChannels[channelNumber] == false)) {
    TMS_LOG_E(TAG, "%s: invalid channel!!!", __func__);
    sestatus = SecureElementStatus::FAILED;
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
      sestatus = SecureElementStatus::FAILED;
    } else if ((rspApdu.pData[rspApdu.len - 2] == 0x90) &&
               (rspApdu.pData[rspApdu.len - 1] == 0x00)) {
      sestatus = SecureElementStatus::SUCCESS;
    } else {
      sestatus = SecureElementStatus::FAILED;
    }
    free(cmdApdu.pData);
    free(rspApdu.pData);
  }

  if ((channelNumber == DEFAULT_BASIC_CHANNEL) ||
      (sestatus == SecureElementStatus::SUCCESS)) {
    TMS_LOG_D(TAG, "%s: Closing channel : %d is successful ", __func__,
                channelNumber);
    mOpenedChannels[channelNumber] = false;
    mOpenedchannelCount--;
    /*If there are no channels remaining close secureElement*/
    if ((mOpenedchannelCount == 0) && !OpenLogicalChannelProcessing &&
        !OpenBasicChannelProcessing) {
      sestatus = seHalDeInit();
    } else {
      sestatus = SecureElementStatus::SUCCESS;
    }
  }

  TMS_LOG_V(TAG, "%s: Exit", __func__);
  return sestatus;
}

void SecureElement::serviceDied(uint64_t /*cookie*/, const wp<IBase>& /*who*/) {
  TMS_LOG_E(TAG, "%s: SecureElement serviceDied!!!", __func__);
  SecureElementStatus sestatus = seHalDeInit();
  if (sestatus != SecureElementStatus::SUCCESS) {
    TMS_LOG_E(TAG, "%s: seHalDeInit Failed!!!", __func__);
  }
  if (mCallbackV1_0 != nullptr) {
    mCallbackV1_0->unlinkToDeath(this);
  }
}

bool SecureElement::isSeInitialized() { return SeIsOpened(); }

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
    mCallbackV1_0->onStateChange(false);

    status = seReset();
    if (status != ESESTATUS_SUCCESS) {
      TMS_LOG_E(TAG, "%s: SecureElement reset failed!!", __func__);
    } else {
      for (uint8_t xx = 0; xx < MAX_LOGICAL_CHANNELS; xx++) {
        mOpenedChannels[xx] = false;
      }
      mOpenedchannelCount = 0;
      mCallbackV1_0->onStateChange(true);
    }
  }
  TMS_LOG_V(TAG, "%s: Exit", __func__);
}

Return<::android::hardware::secure_element::V1_0::SecureElementStatus>
SecureElement::seHalDeInit() {
  TMS_LOG_D(TAG, "%s: Enter", __func__);
  ESESTATUS status = ESESTATUS_SUCCESS;
  SecureElementStatus sestatus = SecureElementStatus::FAILED;
  status = seDeInit();
  if (status != ESESTATUS_SUCCESS) {
    sestatus = SecureElementStatus::FAILED;
  } else {
    sestatus = SecureElementStatus::SUCCESS;

    for (uint8_t xx = 0; xx < MAX_LOGICAL_CHANNELS; xx++) {
      mOpenedChannels[xx] = false;
    }
    mOpenedchannelCount = 0;
  }
  TMS_LOG_V(TAG, "%s: Exit", __func__);
  return sestatus;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace secure_element
}  // namespace hardware
}  // namespace android
