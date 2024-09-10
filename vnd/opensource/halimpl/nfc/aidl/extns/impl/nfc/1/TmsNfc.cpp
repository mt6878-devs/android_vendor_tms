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

#include <log/log.h>

#include "TmsNfc.h"
#include "TmsNciHal_IoctlOperations.h"
#include "Tmsnfc.h"
#include "tmsCommon.h"

namespace aidl {
namespace vendor {
namespace tms {
namespace tmsnfc_aidl {


::ndk::ScopedAStatus TmsNfc::getVendorParam(const std::string& in_key, std::string* _aidl_return) {
    ALOGD("TmsNfc::getVendorParam Entry");
    *_aidl_return = phTmsNciHalGetSystemProperty(in_key);
    ALOGD("TmsNfc::getVendorParam Exit");
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus TmsNfc::setTmsTransitConfig(const std::string& in_transitConfValue, bool* _aidl_return) {

    ALOGD("TmsNfc::setTmsTransitConfig Entry");
    *_aidl_return = phTmsNciHalSetTmsTransitConfig((char *)in_transitConfValue.c_str());
    ALOGD("TmsNfc::setTmsTransitConfig Exit");

    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus TmsNfc::EseSoftReset(bool* _aidl_return) {
    ALOGD("TmsNfc::%s enter", __FUNCTION__);
    *_aidl_return = phTmsNciHalEseSoftReset();
    ALOGD("TmsNfc::%s Exit", __FUNCTION__);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus TmsNfc::nfccFwDownload(bool* _aidl_return) {
    ALOGD("TmsNfc::%s enter", __FUNCTION__);
    *_aidl_return = phTmsNciHalNfccFwDownload();
    ALOGD("TmsNfc::%s Exit", __FUNCTION__);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus TmsNfc::doAction(int64_t in_ioctlType, int32_t* _aidl_return) {
    ESESTATUS status = ESESTATUS_SUCCESS;

    switch (in_ioctlType) {
        case HAL_ESE_COS_DL_I2C: {
            status = (ESESTATUS)doEseCosDownloadI2C();
            break;
        }

        case HAL_NFCC_FW_DL: {
            status = (ESESTATUS)doNfccFwDownload();
            break;
        }

        case HAL_NFCC_BL_DL: {
            status = (ESESTATUS)doNfccBlDownload();
            break;
        }

        default: {
            ALOGD("TmsNfc::%s : invalid ioctlType = %d",
                __FUNCTION__, (int)in_ioctlType);
            status = ESESTATUS_INVALID_PARAMETER;
        }
    }
    *_aidl_return = (char16_t)status;
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus TmsNfc::isConfigModified(int8_t in_fileType, bool* _aidl_return) {
    bool ret = false;

    switch (in_fileType) {
        case FILE_TYPE_TMS: {
            ret = isTmsConfigModified();
            break;
        }

        case FILE_TYPE_RF: {
            ret = isTmsRFConfigModified();
            break;
        }

        default: {
            ALOGD("TmsNfc::%s: unknown file type:%d",
                __func__, in_fileType);
            ret = false;
        }
    }

    *_aidl_return = ret;
    return ndk::ScopedAStatus::ok();

}

}  // namespace nfc
}  // namespace tms
}  // namespace vendor
}  // namespace aidl
