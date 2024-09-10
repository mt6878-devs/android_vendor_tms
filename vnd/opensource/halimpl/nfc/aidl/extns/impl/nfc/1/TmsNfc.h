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

#ifndef VENDOR_TMS_NFC_V1_TMSNFC_H
#define VENDOR_TMS_NFC_V1_TMSNFC_H

#include <aidl/vendor/tms/tmsnfc_aidl/BnTmsNfc.h>

namespace aidl {
namespace vendor {
namespace tms {
namespace tmsnfc_aidl {


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

struct TmsNfc : public BnTmsNfc {
    ::ndk::ScopedAStatus EseSoftReset(bool* _aidl_return) override;
    ::ndk::ScopedAStatus doAction(int64_t in_ioctlType, int32_t* _aidl_return) override;
    ::ndk::ScopedAStatus getVendorParam(const std::string& in_key, std::string* _aidl_return) override;
    ::ndk::ScopedAStatus isConfigModified(int8_t in_fileType, bool* _aidl_return) override;
    ::ndk::ScopedAStatus nfccFwDownload(bool* _aidl_return) override;
    ::ndk::ScopedAStatus setTmsTransitConfig(const std::string& in_transitConfValue, bool* _aidl_return) override;
};

}  // namespace nfc
}  // namespace tms
}  // namespace vendor
}  // namespace aidl


#endif  // VENDOR_TMS_TMS_V1_TMSNFC_H
