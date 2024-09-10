/******************************************************************************
 *
 *  Copyright 2018-2020 NXP
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

#ifndef VENDOR_TMS_TMSNFC_V1_0_TMSNFC_H
#define VENDOR_TMS_TMSNFC_V1_0_TMSNFC_H

#include <vendor/tms/tmsnfc/1.0/ITmsNfc.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace vendor {
namespace tms {
namespace tmsnfc {
namespace V1_0 {
namespace implementation {

using ::android::hidl::base::V1_0::IBase;
using ::vendor::tms::tmsnfc::V1_0::ITmsNfc;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

enum {
  HAL_ESE_TMS_IOCTL_BASE = 1000,
  HAL_ESE_JUMP_TO_COS,
  HAL_ESE_COS_DL,
  HAL_NFCC_FW_DL,
  HAL_NFCC_BL_DL,
  HAL_ESE_COS_DL_I2C,
};

enum {
    FILE_TYPE_TMS = 0,
    FILE_TYPE_RF,
};

struct TmsNfc : public ITmsNfc {
    Return<void> getVendorParam(const ::android::hardware::hidl_string& key,
                                getVendorParam_cb _hidl_cb) override;
    Return<bool> setTmsTransitConfig(const ::android::hardware::hidl_string& strval) override;

    Return<bool> EseSoftReset() override;

    Return<bool> nfccFwDownload() override;

    Return<int16_t> doAction(uint64_t ioctlType) override;

    Return<bool> isConfigModified(uint8_t fileType) override;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace tmsnfc
}  // namespace tms
}  // namespace vendor

#endif  // VENDOR_TMS_TMSNFC_V1_0_TMSNFC_H
