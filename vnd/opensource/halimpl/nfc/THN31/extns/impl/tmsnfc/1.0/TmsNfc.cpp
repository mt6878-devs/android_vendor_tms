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

namespace vendor {
namespace tms {
namespace tmsnfc {
namespace V1_0 {
namespace implementation {

Return<void> TmsNfc::getVendorParam(const ::android::hardware::hidl_string& key,
                                    getVendorParam_cb _hidl_cb) {
    std::string val = phTmsNciHalGetSystemProperty(key);
    _hidl_cb(val);
    return Void();
}

Return<bool> TmsNfc::setTmsTransitConfig(const ::android::hardware::hidl_string& strval) {
    bool status = true;
    ALOGD("TmsNfc::setTmsTransitConfig Entry");

    status = phTmsNciHalSetTmsTransitConfig((char *)strval.c_str());

    ALOGD("TmsNfc::setTmsTransitConfig Exit");
    return status;
}

Return<bool> TmsNfc::EseSoftReset() {
    bool ret = false;
    ALOGD("TmsNfc::%s enter", __FUNCTION__);
    ret = phTmsNciHalEseSoftReset();
    ALOGD("TmsNfc::%s Exit", __FUNCTION__);
    return ret;
}

Return<bool> TmsNfc::nfccFwDownload() {
    bool ret = false;
    ALOGD("TmsNfc::%s enter", __FUNCTION__);
    ret = phTmsNciHalNfccFwDownload();
    ALOGD("TmsNfc::%s Exit", __FUNCTION__);
    return ret;
}

Return<int16_t> TmsNfc::doAction(uint64_t ioctlType) {
  ESESTATUS status = ESESTATUS_SUCCESS;

  switch (ioctlType) {
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
            __FUNCTION__, (int)ioctlType);
      status = ESESTATUS_INVALID_PARAMETER;
    }
  }
  return (int16_t)status;
}

Return<bool> TmsNfc::isConfigModified(uint8_t fileType) {
    bool ret = false;

    switch (fileType) {
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
                __func__, fileType);
            ret = false;
        }
    }

    return ret;
}


}  // namespace implementation
}  // namespace V1_0
}  // namespace Tmsnfc
}  // namespace Tms
}  // namespace vendor
