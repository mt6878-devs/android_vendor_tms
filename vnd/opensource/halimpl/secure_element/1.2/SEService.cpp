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

#include <android/hardware/secure_element/1.2/ISecureElement.h>
#include <hidl/LegacySupport.h>
#include <log/log.h>
#ifdef CHECK_UNION_PARAM
#include <cutils/properties.h>
#endif
#include "SecureElement.h"

#include <vendor/tms/tmsese/1.0/ITmsEse.h>
#include "TmsEse.h"

#include "tmslog.h"

#ifdef CHECK_UNION_PARAM
#define UNION_PAY_PROP_NAME "ro.product.cuptsm"
// union pay param format:
// "UNION_PAY_PARAM_OEM_NAME|UNION_PAY_PARAM_SE_NAME|UNION_PAY_PARAM_SERVICE_MODE|UNION_PAY_PARAM_SE_PROVIDER"
#define UNION_PAY_PARAM_SE_PROVIDER_TMS "27"
#endif

#ifdef TMS_NFC
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "configC.h"
#endif

static const char TAG[] = "HalSEService@1.2";

// Generated HIDL files
using android::OK;
using android::sp;
using android::status_t;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::secure_element::V1_2::ISecureElement;
using android::hardware::secure_element::V1_2::implementation::SecureElement;

using vendor::tms::tmsese::V1_1::ITmsEse;
using vendor::tms::tmsese::V1_1::implementation::TmsEse;

#ifdef CHECK_UNION_PARAM
void check_union_pay_prop() {
    char prop_val[PROPERTY_VALUE_MAX] = {0};
    int ret = 0;

    ret = property_get(UNION_PAY_PROP_NAME, prop_val, "");
    if (ret > 0) { // prop len
        // prop has already been set, we cannot modify ro. prop
        // only check if SE provider id is correct
        TMS_LOG_D(TAG, "%s=%s", UNION_PAY_PROP_NAME, prop_val);
        if (ret < 2 || strncmp(UNION_PAY_PARAM_SE_PROVIDER_TMS, &prop_val[ret - 2], 2)) {
            TMS_LOG_E(TAG, "union pay param err. Plz set SE provider to 27 => TMS");
            goto fail;
        }
    } else {
        // prop is null
        TMS_LOG_E(TAG, "union pay prop(%s) is null, Plz contact UNION and TMS for generating this setting", UNION_PAY_PROP_NAME);
        goto fail;
    }
    return;
fail:
    // abort to notify the param error message
    sleep(1);
    exit(-1);
}
#endif

#ifdef TMS_NFC
bool isTmsChip(){
    int ret = -1;
    struct stat fileStat;
    const uint16_t maxLen = 260;
    char pNfcDevNode[maxLen] = {0};

    ConfigGetString(pNfcDevNode, maxLen,
                    NAME_TMS_ESE_DEV_NODE, strlen(NAME_TMS_ESE_DEV_NODE),
                    "/dev/tms_ese", strlen("/dev/tms_ese"));

    ret = stat(pNfcDevNode, &fileStat);
    if (0 == ret) {
        ALOGI("Chip is tms nfc");
        return true;
    }

    return false;
}
#endif

int main() {
  TMS_LOG_I(TAG, "Secure Element HAL Service 1.2 is starting.");
#ifdef CHECK_UNION_PARAM
  check_union_pay_prop();
#endif
#ifdef TMS_NFC
  if(!isTmsChip()) {
    ALOGE(" TMS SE service unused");
    configureRpcThreadpool(1, false /*callerWillJoin*/);
  } else {
#endif
    sp<ISecureElement> se_service = new SecureElement();
    configureRpcThreadpool(2, true /*callerWillJoin*/);
    status_t status = se_service->registerAsService("eSE1");
    if (status != OK) {
      TMS_LOG_E(TAG,
          "Could not register service for Secure Element HAL Iface (%d).", status);
      return -1;
    }
#ifdef TMS_NFC
  }
#endif

  TMS_LOG_I(TAG, " TMS eSE Extn Service 1.1 is starting.");
  sp<ITmsEse> tms_se_service = new TmsEse();
#ifdef TMS_NFC
  status_t
#endif
  status = tms_se_service->registerAsService();
  if (status != OK) {
      TMS_LOG_E(TAG,
          "Could not register service for TMS eSE Extn Iface (%d).", status);
      return -1;
  }

  TMS_LOG_I(TAG, "Secure Element Service is ready");
  joinRpcThreadpool();
  return 1;
}
