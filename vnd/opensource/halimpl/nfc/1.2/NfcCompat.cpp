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


#include "NfcCompat.h"
#include "TmsConfig.h"
#define TMS_MAX_CONFIG_STRING_LEN 260

namespace android {
namespace hardware {
namespace nfc {
namespace V1_2 {
namespace implementation {

/******************************************************************************
 * Function         tmsNciHalGetVendorConfig
 *
 * Description      This function can be used by HAL to inform
 *                 to update vendor configuration parametres
 *
 * Returns          void.
 *
 ******************************************************************************/
void NfcCompat::tmsNciHalGetVendorConfig(android::hardware::nfc::V1_1::NfcConfig& config) {
    unsigned long num = 0;
    std::array<uint8_t, TMS_MAX_CONFIG_STRING_LEN> pBuffer;
    pBuffer.fill(0);
    long retLen = 0;
    memset(&config, 0x00, sizeof(android::hardware::nfc::V1_1::NfcConfig));
    if (getTmsNumValue(NAME_NFA_POLL_BAIL_OUT_MODE, &num, sizeof(num))) {
        config.nfaPollBailOutMode = (bool)num;
    }
    if (getTmsNumValue(NAME_ISO_DEP_MAX_TRANSCEIVE, &num, sizeof(num))) {
        config.maxIsoDepTransceiveLength = (uint32_t)num;
    }
    if (getTmsNumValue(NAME_DEFAULT_OFFHOST_ROUTE, &num, sizeof(num))) {
        config.defaultOffHostRoute = (uint8_t)num;
    }
    if (getTmsNumValue(NAME_DEFAULT_NFCF_ROUTE, &num, sizeof(num))) {
        config.defaultOffHostRouteFelica = (uint8_t)num;
    }
    if (getTmsNumValue(NAME_DEFAULT_SYS_CODE_ROUTE, &num, sizeof(num))) {
        config.defaultSystemCodeRoute = (uint8_t)num;
    }
    if (getTmsNumValue(NAME_DEFAULT_SYS_CODE_PWR_STATE, &num, sizeof(num))) {
        config.defaultSystemCodePowerState = num;
    }
    if (getTmsNumValue(NAME_DEFAULT_ROUTE, &num, sizeof(num))) {
        config.defaultRoute = (uint8_t)num;
    }
    if (getTmsByteArrayValue(NAME_DEVICE_HOST_WHITE_LIST, (char *)pBuffer.data(), pBuffer.size(), &retLen)) {
        config.hostWhitelist.resize(retLen);
        for (long i = 0; i < retLen; i++) {
            config.hostWhitelist[i] = pBuffer[i];
        }
    }
    if (getTmsNumValue(NAME_OFF_HOST_ESE_PIPE_ID, &num, sizeof(num))) {
        config.offHostESEPipeId = (uint8_t)num;
    }
    if (getTmsNumValue(NAME_OFF_HOST_SIM_PIPE_ID, &num, sizeof(num))) {
        config.offHostSIMPipeId = (uint8_t)num;
    }
    if ((getTmsByteArrayValue(NAME_NFA_PROPRIETARY_CFG, (char *)pBuffer.data(), pBuffer.size(), &retLen))
            && (retLen == 9)) {
        config.nfaProprietaryCfg.protocol18092Active = (uint8_t) pBuffer[0];
        config.nfaProprietaryCfg.protocolBPrime = (uint8_t) pBuffer[1];
        config.nfaProprietaryCfg.protocolDual = (uint8_t) pBuffer[2];
        config.nfaProprietaryCfg.protocol15693 = (uint8_t) pBuffer[3];
        config.nfaProprietaryCfg.protocolKovio = (uint8_t) pBuffer[4];
        config.nfaProprietaryCfg.protocolMifare = (uint8_t) pBuffer[5];
        config.nfaProprietaryCfg.discoveryPollKovio = (uint8_t) pBuffer[6];
        config.nfaProprietaryCfg.discoveryPollBPrime = (uint8_t) pBuffer[7];
        config.nfaProprietaryCfg.discoveryListenBPrime = (uint8_t) pBuffer[8];
    } else {
        memset(&config.nfaProprietaryCfg, 0xFF, sizeof(ProtocolDiscoveryConfig));
    }
    if ((getTmsNumValue(NAME_PRESENCE_CHECK_ALGORITHM, &num, sizeof(num))) && (num <= 2)) {
        config.presenceCheckAlgorithm = (PresenceCheckAlgorithm)num;
    }
}


/******************************************************************************
 * Function         tmsNciHalGetVendorConfig_1_2
 *
 * Description      This function can be used by HAL to inform
 *                 to update vendor configuration parametres
 *
 * Returns          void.
 *
 ******************************************************************************/

void NfcCompat::tmsNciHalGetVendorConfig_1_2(android::hardware::nfc::V1_2::NfcConfig& config) {
    unsigned long num = 0;
    std::array<uint8_t, TMS_MAX_CONFIG_STRING_LEN> pBuffer;
    pBuffer.fill(0);
    long retLen = 0;
    memset(&config, 0x00, sizeof(android::hardware::nfc::V1_2::NfcConfig));
    tmsNciHalGetVendorConfig(config.v1_1);

    if (getTmsByteArrayValue(NAME_OFFHOST_ROUTE_UICC, (char *)pBuffer.data(), pBuffer.size(), &retLen)) {
        config.offHostRouteUicc.resize(retLen);
        for (long i = 0; i < retLen; i++) {
            config.offHostRouteUicc[i] = pBuffer[i];
        }
    }

    if (getTmsByteArrayValue(NAME_OFFHOST_ROUTE_ESE, (char *)pBuffer.data(), pBuffer.size(), &retLen)) {
        config.offHostRouteEse.resize(retLen);
        for (long i = 0; i < retLen; i++) {
            config.offHostRouteEse[i] = pBuffer[i];
        }
    }

    if (getTmsNumValue(NAME_DEFAULT_ISODEP_ROUTE, &num, sizeof(num))) {
        config.defaultIsoDepRoute = num;
    }

}

}  // namespace implementation
}  // namespace V1_2
}  // namespace nfc
}  // namespace hardware
}  // namespace android
