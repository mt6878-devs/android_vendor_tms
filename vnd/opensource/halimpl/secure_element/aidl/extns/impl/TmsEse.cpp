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

#include "TmsEse.h"
#include "SEApi.h"
#include "tmslog.h"
#include "tmsCosReeSpiDl.h"
#include "tmsCosTeeSpiDl.h"

#ifdef MTK_TRUSTONIC_TEE
#include "tms_spi_control.h"
#endif

namespace aidl {
namespace vendor {
namespace tms {
namespace tmsese_aidl {


static const char TAG[] = "TmsEse";

::ndk::ScopedAStatus TmsEse::doAction(int64_t ioctlType, int32_t* _aidl_return) {
  ESESTATUS status = ESESTATUS_SUCCESS;

  switch (ioctlType) {
    case HAL_ESE_COS_DL: {
      status = (ESESTATUS)eseCosReeDl();
      break;
    }
    case HAL_ESE_COS_PTH_DL_REE: {
      status = (ESESTATUS)eseCosReeDlPth();
      break;
    }
    case HAL_ESE_COS_PTH_DL_TEE: {
      status = (ESESTATUS)eseCosTeeDlPth(false);
      break;
    }

    default: {
      TMS_LOG_W(TAG, "%s : invalid ioctlType = %d",
            __FUNCTION__, (int)ioctlType);
      status = ESESTATUS_INVALID_PARAMETER;
    }
  }
  *_aidl_return = (int16_t)status;
  return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus TmsEse::sInit(int32_t* _aidl_return) {
  ESESTATUS status = ESESTATUS_SUCCESS;
  TMS_LOG_D(TAG, "%s: Enter", __func__);
  status = seInit(ESE_MODE_NORMAL);
  if (status != ESESTATUS_SUCCESS) {
    TMS_LOG_E(TAG, "%s: Init failed!!!", __func__);
  }
  TMS_LOG_V(TAG, "%s: Exit", __func__);

  *_aidl_return = (int16_t)status;
  return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus TmsEse::sDeinit(int32_t* _aidl_return) {
  ESESTATUS status = ESESTATUS_SUCCESS;
  status = seDeInit();
  if (status != ESESTATUS_SUCCESS) {
    TMS_LOG_E(TAG, "%s: Deinit failed!!!", __func__);
  }

  TMS_LOG_V(TAG, "%s: Exit", __func__);
  *_aidl_return = (int16_t)status;
  return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus TmsEse::sReset(int32_t* _aidl_return) {
  ESESTATUS status = ESESTATUS_SUCCESS;
  status = seReset();
  if (status != ESESTATUS_SUCCESS) {
    TMS_LOG_E(TAG, "%s: Reset failed!!!", __func__);
  }

  TMS_LOG_V(TAG, "%s: Exit", __func__);
  *_aidl_return = (int16_t)status;
  return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus TmsEse::transmit(const std::vector<uint8_t>& data,
    std::vector<uint8_t>* _aidl_return) {
  ESESTATUS status = ESESTATUS_SUCCESS;
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

  std::vector<uint8_t> result;
  if (status != ESESTATUS_SUCCESS) {
    TMS_LOG_E(TAG, "%s: transmit failed!!!", __func__);
  } else {
    result.resize(rspApdu.len);
    memcpy(&result[0], rspApdu.pData, rspApdu.len);
  }
  *_aidl_return = result;
  free(cmdApdu.pData);
  free(rspApdu.pData);
  return ndk::ScopedAStatus::ok();
}

bool TmsEse::isSeInitialized() { return SeIsInitialized(); }

::ndk::ScopedAStatus TmsEse::sGetAtr(std::vector<uint8_t>* _aidl_return){
  TMS_LOG_D(TAG, "%s: Enter", __func__);
  std::vector<uint8_t> response;
  SeData atr;
  bool isInited = false;

  if (!isSeInitialized()) {
    TMS_LOG_D(TAG, "%s: Enter SeInitialized", __func__);
    ESESTATUS status = seInit(ESE_MODE_NORMAL);
    if (status != ESESTATUS_SUCCESS) {
      TMS_LOG_E(TAG, "%s: sInit Failed!!!", __func__);
      *_aidl_return = response;
      return ndk::ScopedAStatus::ok();
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
    status = seDeInit(); {
      if (status != ESESTATUS_SUCCESS) {
        TMS_LOG_E(TAG, "%s: Deinit failed!!!", __func__);
      }
    }
  }

  if (NULL != atr.pData) {
    free(atr.pData);
  }
  *_aidl_return = response;
  return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus TmsEse::setMtkSpiClk(bool enable, bool* _aidl_return) {

  TMS_LOG_D(TAG, "%s: Enter, enable = %d", __func__, enable);
#ifdef MTK_TRUSTONIC_TEE
  static uint32_t externSpiCnt = 0;
  bool result = false;
  TMS_LOG_D(TAG, "%s: externSpiCnt = %d", __func__, externSpiCnt);
  if (externSpiCnt == 0 && !enable) {
    TMS_LOG_D(TAG, "%s: stop releasing spi clk", __func__, enable, externSpiCnt);
    return true;
  }
  result = enable? requestSpiClk(): releaseSpiClk();
  if (result) {
    enable? externSpiCnt++:externSpiCnt--;
  }
  *_aidl_return = result;
#else
  *_aidl_return = true;
#endif
  return ndk::ScopedAStatus::ok();
}

}  // namespace tmsese_aidl
}  // namespace tms
}  // namespace vendor
}  // namespace aidl
