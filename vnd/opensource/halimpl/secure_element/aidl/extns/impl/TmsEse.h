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

#ifndef VENDOR_TMS_TMSESE_V1_0_TMSESE_H
#define VENDOR_TMS_TMSESE_V1_0_TMSESE_H

#include <stdint.h>
#include <aidl/vendor/tms/tmsese_aidl/BnTmsEse.h>

namespace aidl {
namespace vendor {
namespace tms {
namespace tmsese_aidl {

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


struct TmsEse : public BnTmsEse {

  ::ndk::ScopedAStatus doAction(int64_t ioctlType, int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus sInit(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus sDeinit(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus sReset(int32_t* _aidl_return) override;
  ::ndk::ScopedAStatus transmit(const std::vector<uint8_t>& data,
    std::vector<uint8_t>* _aidl_return) override;
  ::ndk::ScopedAStatus sGetAtr(std::vector<uint8_t>* _aidl_return) override;
  ::ndk::ScopedAStatus setMtkSpiClk(bool in_enable, bool* _aidl_return) override;

private:
  bool isSeInitialized();
};

}  // namespace tmsese_aidl
}  // namespace tms
}  // namespace vendor
}  // namespace aidl


#endif  // VENDOR_TMS_TMSNFC_V1_0_TMSNFC_H
