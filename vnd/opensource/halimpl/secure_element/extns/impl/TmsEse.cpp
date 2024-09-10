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

namespace vendor {
namespace tms {
namespace tmsese {
namespace V1_1 {
namespace implementation {

static const char TAG[] = "TmsEse";

Return<int16_t> TmsEse::doAction(uint64_t ioctlType) {
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
  return (int16_t)status;
}

Return<int16_t> TmsEse::sInit() {
  ESESTATUS status = ESESTATUS_SUCCESS;
  TMS_LOG_D(TAG, "%s: Enter", __func__);
  status = seInit(ESE_MODE_NORMAL);
  if (status != ESESTATUS_SUCCESS) {
    TMS_LOG_E(TAG, "%s: Init failed!!!", __func__);
  }
  TMS_LOG_V(TAG, "%s: Exit", __func__);

  return (int16_t)status;
}

Return<int16_t> TmsEse::sDeinit() {
  ESESTATUS status = ESESTATUS_SUCCESS;
  status = seDeInit();
  if (status != ESESTATUS_SUCCESS) {
    TMS_LOG_E(TAG, "%s: Deinit failed!!!", __func__);
  }

  TMS_LOG_V(TAG, "%s: Exit", __func__);
  return (int16_t)status;
}

Return<int16_t> TmsEse::sReset() {
  ESESTATUS status = ESESTATUS_SUCCESS;
  status = seReset();
  if (status != ESESTATUS_SUCCESS) {
    TMS_LOG_E(TAG, "%s: Reset failed!!!", __func__);
  }

  TMS_LOG_V(TAG, "%s: Exit", __func__);
  return (int16_t)status;
}

Return<void> TmsEse::transmit(const android::hardware::hidl_vec<uint8_t>& data,
                              transmit_cb _hidl_cb) {
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

  android::hardware::hidl_vec<uint8_t> result;
  if (status != ESESTATUS_SUCCESS) {
    TMS_LOG_E(TAG, "%s: transmit failed!!!", __func__);
  } else {
    result.resize(rspApdu.len);
    memcpy(&result[0], rspApdu.pData, rspApdu.len);
  }
  _hidl_cb(result);
  free(cmdApdu.pData);
  free(rspApdu.pData);
  return Void();
}

bool TmsEse::isSeInitialized() { return SeIsInitialized(); }

Return<void> TmsEse::sGetAtr(sGetAtr_cb _hidl_cb) {
  TMS_LOG_D(TAG, "%s: Enter", __func__);
  hidl_vec<uint8_t> response;
  SeData atr;
  bool isInited = false;

  if (!isSeInitialized()) {
    TMS_LOG_D(TAG, "%s: Enter SeInitialized", __func__);
    int16_t result = sInit();
    ESESTATUS status = (ESESTATUS)result;
    if (status != ESESTATUS_SUCCESS) {
      TMS_LOG_E(TAG, "%s: sInit Failed!!!", __func__);
      _hidl_cb(response);
      return Void();
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
    sDeinit();
  }

  if (NULL != atr.pData) {
    free(atr.pData);
  }
  _hidl_cb(response);
  return Void();
}

Return<bool> TmsEse::setMtkSpiClk(bool enable) {
#ifdef MTK_TRUSTONIC_TEE
  static uint32_t externSpiCnt = 0;
  bool result = false;
  TMS_LOG_D(TAG, "%s: Enter, enable = %d, externSpiCnt = %d", __func__, enable, externSpiCnt);
  if (externSpiCnt == 0 && !enable) {
    TMS_LOG_D(TAG, "%s: stop releasing spi clk", __func__, enable, externSpiCnt);
    return true;
  }
  result = enable? requestSpiClk(): releaseSpiClk();
  if (result) {
    enable? externSpiCnt++:externSpiCnt--;
  }
  return result;
#else
  return enable? enable: !enable;
#endif
}

Return<void> TmsEse::generic(uint32_t cmd,
                             const hidl_vec<uint8_t>& inData,
                             generic_cb _hidl_cb) {
  hidl_vec<uint8_t> outData;
  int status = ESESTATUS_SUCCESS;
  (void)(inData);

  TMS_LOG_I(TAG, "%s: enter cmd=%u", __func__, cmd);
  _hidl_cb(outData, status);
  return Void();
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace tmsese
}  // namespace tms
}  // namespace vendor
