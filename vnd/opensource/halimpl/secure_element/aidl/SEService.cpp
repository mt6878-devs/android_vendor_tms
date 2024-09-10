/*
 * Copyright (c), Tsingteng MicroSystem
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

#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <log/log.h>
#ifdef CHECK_UNION_PARAM
#include <cutils/properties.h>
#endif
#include "SecureElement.h"

#include "tmslog.h"
#include "TmsEse.h"

#ifdef CHECK_UNION_PARAM
#define UNION_PAY_PROP_NAME "ro.product.cuptsm"
// union pay param format:
// "UNION_PAY_PARAM_OEM_NAME|UNION_PAY_PARAM_SE_NAME|UNION_PAY_PARAM_SERVICE_MODE|UNION_PAY_PARAM_SE_PROVIDER"
#define UNION_PAY_PARAM_SE_PROVIDER_TMS "27"
#endif

static const char TAG[] = "HalSEService";

// Generated HIDL files
using android::OK;
using android::sp;
using android::status_t;
using ::aidl::android::hardware::secure_element::SecureElement;
using ::aidl::vendor::tms::tmsese_aidl::TmsEse;

void startTmsEseAidlService()
{
    TMS_LOG_I(TAG, "start tms ese extn aidl hal service.");
    std::shared_ptr<TmsEse> tms_ese_service = ndk::SharedRefBase::make<TmsEse>();
    const std::string instance =
        std::string() + TmsEse::descriptor + "/default";
    TMS_LOG_I(TAG, "register tms ese extn aidl hal service name : %s", instance.c_str());
    binder_status_t status = AServiceManager_addService(
        tms_ese_service->asBinder().get(), instance.c_str());

    if (status != OK) {
      TMS_LOG_E(TAG, "Could not register service for TmsEse AIDL HAL Iface (%d).", status);
      return ;
    }
    TMS_LOG_I(TAG, " tms ese extn aidl hal service is ready");
    // ABinderProcess_joinThreadPool();
}

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
    exit(EXIT_FAILURE);
}
#endif

int main() {
  TMS_LOG_I(TAG, "Secure Element AIDL HAL Service is starting.");
#ifdef CHECK_UNION_PARAM
  check_union_pay_prop();
#endif
  if (!ABinderProcess_setThreadPoolMaxThreadCount(1)) {
    ALOGE("failed to set thread pool max thread count");
    return EXIT_FAILURE;
  }
  std::shared_ptr<SecureElement> se_service =
      ndk::SharedRefBase::make<SecureElement>();
  const std::string instance =
        std::string() + SecureElement::descriptor + "/" + "eSE1";
  binder_status_t status = AServiceManager_addService(
        se_service->asBinder().get(), instance.c_str());
  if (status != OK) {
    TMS_LOG_E(TAG,
      "Could not register service for Secure Element AIDL HAL Iface (%d).", status);
    return EXIT_FAILURE;
  }

  // std::thread t1(startTmsEseAidlService);
  startTmsEseAidlService();

  TMS_LOG_I(TAG, "Secure Element Service is ready");
  ABinderProcess_joinThreadPool();
  return EXIT_SUCCESS;
}
