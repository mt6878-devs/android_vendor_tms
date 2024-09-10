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

#ifndef VENDOR_TMS_TMSESE_V1_1_TMSESE_H
#define VENDOR_TMS_TMSESE_V1_1_TMSESE_H

#include <stdint.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <vendor/tms/tmsese/1.1/ITmsEse.h>

namespace vendor {
namespace tms {
namespace tmsese {
namespace V1_1 {
namespace implementation {

using ::android::hidl::base::V1_0::IBase;
using ::vendor::tms::tmsese::V1_1::ITmsEse;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

#ifndef MIN_APDU_LENGTH
#define MIN_APDU_LENGTH 0x04
#endif

enum {
  HAL_ESE_TMS_IOCTL_BASE = 1000,
  HAL_ESE_JUMP_TO_COS,
  HAL_ESE_COS_DL,         //REE SPI COS DL
  HAL_ESE_COS_PTH_DL_REE, //REE SPI COS patch DL
  HAL_ESE_COS_PTH_DL_TEE, //TEE SPI COS patch DL
};


struct TmsEse : public ITmsEse {
  Return<int16_t> doAction(uint64_t ioctlType) override;

  Return<int16_t> sInit() override;

  Return<int16_t> sDeinit() override;

  Return<int16_t> sReset() override;

  Return<void> transmit(const android::hardware::hidl_vec<uint8_t>& data,
                        transmit_cb _hidl_cb) override;

  Return<void> sGetAtr(sGetAtr_cb _hidl_cb) override;

  Return<bool> setMtkSpiClk(bool enable) override;

  Return<void> generic(uint32_t cmd,
                       const hidl_vec<uint8_t>& inData,
                       generic_cb _hidl_cb) override;

private:
  bool isSeInitialized();
};

}  // namespace implementation
}  // namespace V1_1
}  // namespace tmsese
}  // namespace tms
}  // namespace vendor

#endif  // VENDOR_TMS_TMSNFC_V1_1_TMSNFC_H
