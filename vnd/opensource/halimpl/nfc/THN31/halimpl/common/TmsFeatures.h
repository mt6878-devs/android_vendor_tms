/******************************************************************************
 *
 *  Copyright 2018-2021 NXP
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

#include <stdint.h>
#include <string>
#ifndef TMS_FEATURES_H
#define TMS_FEATURES_H

/*Including T4T NFCEE by incrementing 1*/
#define NFA_EE_MAX_EE_SUPPORTED 5

#define JCOP_VER_4_0    4

using namespace std;
typedef enum {
    NFCC_DWNLD_WITH_VEN_RESET,
    NFCC_DWNLD_WITH_NCI_CMD
} NfccDnldType;

typedef enum {
    DEFAULT_CHIP_TYPE = 0x00,
    thn31
} NfcChipType;

typedef struct {
    /*Flags common to all chip types*/
    uint8_t _TMS_NFCC_EMPTY_DATA_PACKET                     : 1;
    uint8_t _GEMALTO_SE_SUPPORT                             : 1;
    uint8_t _NFCC_I2C_READ_WRITE_IMPROVEMENT                : 1;
    uint8_t _NFCC_MIFARE_TIANJIN                            : 1;
    uint8_t _NFCC_MW_RCVRY_BLK_FW_DNLD                      : 1;
    uint8_t _NFC_TMS_STAT_DUAL_UICC_EXT_SWITCH              : 1;
    uint8_t _NFC_TMS_STAT_DUAL_UICC_WO_EXT_SWITCH           : 1;
    uint8_t _NFCC_FW_WA                                     : 1;
    uint8_t _NFCC_FORCE_NCI1_0_INIT                         : 1;
    uint8_t _NFCC_ROUTING_BLOCK_BIT                         : 1;
    uint8_t _NFCC_SPI_FW_DOWNLOAD_SYNC                      : 1;
    uint8_t _HW_ANTENNA_LOOP4_SELF_TEST                     : 1;
    uint8_t _NFCEE_REMOVED_NTF_RECOVERY                     : 1;
    uint8_t _NFCC_FORCE_FW_DOWNLOAD                         : 1;
    uint8_t _UICC_CREATE_CONNECTIVITY_PIPE                  : 1;
    uint8_t _NFCC_AID_MATCHING_PLATFORM_CONFIG              : 1;
    uint8_t _NFCC_ROUTING_BLOCK_BIT_PROP                    : 1;
    uint8_t _TMS_NFC_UICC_ETSI12                            : 1;
    uint8_t _NFA_EE_MAX_EE_SUPPORTED                        : 3;
    uint8_t _NFCC_DWNLD_MODE                                : 1;
} NfccFeatureList;

typedef struct {
    uint8_t _ESE_EXCLUSIVE_WIRED_MODE                    : 2;
    uint8_t _ESE_WIRED_MODE_RESUME                       : 2;
    uint8_t _ESE_WIRED_MODE_TIMEOUT                      : 2;
    uint8_t _ESE_UICC_DUAL_MODE                          : 2;
    uint8_t _ESE_APDU_GATE_RESET                         : 2;
    uint8_t _ESE_WIRED_MODE_DISABLE_DISCOVERY            : 1;
    uint8_t _LEGACY_APDU_GATE                            : 1;
    uint8_t _TRIPLE_MODE_PROTECTION                      : 1;
    uint8_t _ESE_FELICA_CLT                              : 1;
    uint8_t _WIRED_MODE_STANDBY_PROP                     : 1;
    uint8_t _WIRED_MODE_STANDBY                          : 1;
    uint8_t _ESE_DUAL_MODE_PRIO_SCHEME                   : 2;
    uint8_t _ESE_FORCE_ENABLE                            : 1;
    uint8_t _ESE_RESET_METHOD                            : 1;
    uint8_t _EXCLUDE_NV_MEM_DEPENDENCY                   : 1;
    uint8_t _ESE_ETSI_READER_ENABLE                      : 1;
    uint8_t _ESE_SVDD_SYNC                               : 1;
    uint8_t _NFCC_ESE_UICC_CONCURRENT_ACCESS_PROTECTION  : 1;
    uint8_t _ESE_JCOP_DWNLD_PROTECTION                   : 1;
    uint8_t _UICC_HANDLE_CLEAR_ALL_PIPES                 : 1;
    uint8_t _GP_CONTINOUS_PROCESSING                     : 1;
    uint8_t _ESE_DWP_SPI_SYNC_ENABLE                     : 1;
    uint8_t _ESE_ETSI12_PROP_INIT                        : 1;
    uint8_t _ESE_WIRED_MODE_PRIO                         : 1;
    uint8_t _ESE_UICC_EXCLUSIVE_WIRED_MODE               : 1;
    uint8_t _ESE_POWER_MODE                              : 1;
    uint8_t _ESE_P73_ISO_RST                             : 1;
    uint8_t _BLOCK_PROPRIETARY_APDU_GATE                 : 1;
    uint8_t _JCOP_WA_ENABLE                              : 1;
    uint8_t _TMS_LDR_SVC_VER_2                           : 1;
    uint8_t _TMS_ESE_VER                                 : 3;
    uint8_t _TMS_ESE_JCOP_OSU_UAI_ENABLED                : 1;
    uint8_t  _NCI_NFCEE_PWR_LINK_CMD                     : 1;
} EseFeatureList;
typedef struct {
    uint8_t _NFCC_RESET_RSP_LEN;
} PlatformFeatureList;

typedef struct {
    uint8_t _NCI_INTERFACE_UICC_DIRECT;
    uint8_t _NCI_INTERFACE_ESE_DIRECT;
    uint8_t _NCI_PWR_LINK_PARAM_CMD_SIZE;
    uint8_t _NCI_EE_PWR_LINK_ALWAYS_ON;
    uint8_t _NFA_EE_MAX_AID_ENTRIES;
    uint8_t _NFC_TMS_AID_MAX_SIZE_DYN : 1;
} NfcMwFeatureList;

typedef struct {
    uint8_t nfcTmsEse : 1;
    NfcChipType chipType;
    std::string _FW_LIB_PATH;
    std::string _PLATFORM_LIB_PATH;
    std::string _PKU_LIB_PATH;
    std::string _FW_BIN_PATH;
    uint16_t _PHDNLDNFC_USERDATA_EEPROM_OFFSET;
    uint16_t _PHDNLDNFC_USERDATA_EEPROM_LEN;
    NfccFeatureList nfccFL;
    EseFeatureList eseFL;
    PlatformFeatureList platformFL;
    NfcMwFeatureList nfcMwFL;
} NfcFeatureList;

NfcFeatureList *getNfcFL(void);

#define CONFIGURE_FEATURELIST(chipType) {                                   \
        (*getNfcFL()).chipType = chipType;                                          \
        if ((chipType == thn31)) {                                     \
            (*getNfcFL()).nfcTmsEse = true;                                         \
            CONFIGURE_FEATURELIST_NFCC_WITH_ESE(chipType)                   \
        } \
        else {                                                              \
            (*getNfcFL()).nfcTmsEse = false;                                        \
            CONFIGURE_FEATURELIST_NFCC(chipType)                            \
        }                                                                   \
        \
        \
    }

#define CONFIGURE_FEATURELIST_NFCC_WITH_ESE(chipType) {                     \
        (*getNfcFL()).nfccFL._TMS_NFCC_EMPTY_DATA_PACKET = true;                    \
        (*getNfcFL()).nfccFL._GEMALTO_SE_SUPPORT = true;                            \
        \
        \
        (*getNfcFL()).eseFL._ESE_EXCLUSIVE_WIRED_MODE = 1;                          \
        (*getNfcFL()).eseFL._ESE_WIRED_MODE_RESUME = 2;                             \
        (*getNfcFL()).eseFL._ESE_APDU_GATE_RESET = 2;                               \
        (*getNfcFL()).eseFL._TMS_ESE_VER = JCOP_VER_4_0;                            \
        (*getNfcFL()).eseFL._TMS_LDR_SVC_VER_2 = true;                              \
        (*getNfcFL()).eseFL._TMS_ESE_JCOP_OSU_UAI_ENABLED = false;                  \
        \
        \
        (*getNfcFL()).eseFL._NCI_NFCEE_PWR_LINK_CMD = false;                        \
        (*getNfcFL()).nfcMwFL._NFC_TMS_AID_MAX_SIZE_DYN = true;                     \
        if (chipType == thn31) {                                           \
            CONFIGURE_FEATURELIST_NFCC(thn31)                              \
            (*getNfcFL()).nfccFL._NFCC_SPI_FW_DOWNLOAD_SYNC = true;                 \
            (*getNfcFL()).nfccFL._NFA_EE_MAX_EE_SUPPORTED = 5;                      \
            \
            \
            (*getNfcFL()).eseFL._NCI_NFCEE_PWR_LINK_CMD = true;                     \
            (*getNfcFL()).eseFL._TMS_ESE_JCOP_OSU_UAI_ENABLED = true;               \
            (*getNfcFL()).eseFL._ESE_FELICA_CLT = true;                             \
            (*getNfcFL()).eseFL._ESE_DUAL_MODE_PRIO_SCHEME =                        \
            (*getNfcFL()).eseFL._ESE_UICC_DUAL_MODE;                                \
            (*getNfcFL()).eseFL._ESE_RESET_METHOD = true;                           \
            (*getNfcFL()).eseFL._ESE_POWER_MODE = false;                            \
            (*getNfcFL()).eseFL._ESE_P73_ISO_RST = true;                            \
            (*getNfcFL()).eseFL._WIRED_MODE_STANDBY = false;                        \
            (*getNfcFL()).eseFL._ESE_ETSI_READER_ENABLE = true;                     \
            (*getNfcFL()).eseFL._ESE_SVDD_SYNC = false;                             \
            (*getNfcFL()).eseFL._ESE_JCOP_DWNLD_PROTECTION = true;                  \
            (*getNfcFL()).eseFL._UICC_HANDLE_CLEAR_ALL_PIPES = true;                \
            (*getNfcFL()).eseFL._GP_CONTINOUS_PROCESSING = false;                   \
            (*getNfcFL()).eseFL._ESE_DWP_SPI_SYNC_ENABLE = false;                   \
            (*getNfcFL()).eseFL._ESE_ETSI12_PROP_INIT = false;                      \
            (*getNfcFL()).eseFL._BLOCK_PROPRIETARY_APDU_GATE = false;               \
            (*getNfcFL()).eseFL._LEGACY_APDU_GATE = true;                           \
        }                                                                   \
    }

#define CONFIGURE_FEATURELIST_NFCC(chipType) {                              \
        (*getNfcFL()).eseFL._ESE_WIRED_MODE_TIMEOUT = 3;                            \
        (*getNfcFL()).eseFL._ESE_UICC_DUAL_MODE = 0;                                \
        (*getNfcFL()).eseFL._ESE_WIRED_MODE_DISABLE_DISCOVERY = false;              \
        (*getNfcFL()).eseFL._LEGACY_APDU_GATE = false;                              \
        (*getNfcFL()).eseFL._TRIPLE_MODE_PROTECTION = false;                        \
        (*getNfcFL()).eseFL._ESE_FELICA_CLT = false;                                \
        (*getNfcFL()).eseFL._WIRED_MODE_STANDBY_PROP = false;                       \
        (*getNfcFL()).eseFL._WIRED_MODE_STANDBY = false;                            \
        (*getNfcFL()).eseFL._ESE_DUAL_MODE_PRIO_SCHEME =                            \
        (*getNfcFL()).eseFL._ESE_WIRED_MODE_TIMEOUT;                                \
        (*getNfcFL()).eseFL._ESE_FORCE_ENABLE = false;                              \
        (*getNfcFL()).eseFL._ESE_RESET_METHOD = false;                              \
        (*getNfcFL()).eseFL._ESE_ETSI_READER_ENABLE = false;                        \
        (*getNfcFL()).eseFL._ESE_SVDD_SYNC = false;                                 \
        (*getNfcFL()).eseFL._NFCC_ESE_UICC_CONCURRENT_ACCESS_PROTECTION = false;    \
        (*getNfcFL()).eseFL._ESE_JCOP_DWNLD_PROTECTION = false;                     \
        (*getNfcFL()).eseFL._UICC_HANDLE_CLEAR_ALL_PIPES = false;                   \
        (*getNfcFL()).eseFL._GP_CONTINOUS_PROCESSING = false;                       \
        (*getNfcFL()).eseFL._ESE_DWP_SPI_SYNC_ENABLE = false;                       \
        (*getNfcFL()).eseFL._ESE_ETSI12_PROP_INIT = false;                          \
        (*getNfcFL()).eseFL._ESE_WIRED_MODE_PRIO = false;                           \
        (*getNfcFL()).eseFL._ESE_UICC_EXCLUSIVE_WIRED_MODE = false;                 \
        (*getNfcFL()).eseFL._ESE_POWER_MODE = false;                                \
        (*getNfcFL()).eseFL._ESE_P73_ISO_RST = false;                               \
        (*getNfcFL()).eseFL._BLOCK_PROPRIETARY_APDU_GATE = false;                   \
        (*getNfcFL()).eseFL._JCOP_WA_ENABLE = true;                                 \
        (*getNfcFL()).eseFL._EXCLUDE_NV_MEM_DEPENDENCY = false;                     \
        (*getNfcFL()).nfccFL._TMS_NFC_UICC_ETSI12 = false;                          \
        (*getNfcFL()).nfccFL._NFCC_SPI_FW_DOWNLOAD_SYNC = false;                    \
        \
        \
        (*getNfcFL()).platformFL._NFCC_RESET_RSP_LEN = 0;                           \
        \
        \
        (*getNfcFL()).nfcMwFL._NCI_INTERFACE_UICC_DIRECT = 0x00;                    \
        (*getNfcFL()).nfcMwFL._NCI_INTERFACE_ESE_DIRECT = 0x00;                     \
        (*getNfcFL()).nfcMwFL._NCI_PWR_LINK_PARAM_CMD_SIZE = 0x02;                  \
        (*getNfcFL()).nfcMwFL._NCI_EE_PWR_LINK_ALWAYS_ON = 0x01;                    \
        (*getNfcFL())._PHDNLDNFC_USERDATA_EEPROM_OFFSET = 0x023CU;          \
        (*getNfcFL())._PHDNLDNFC_USERDATA_EEPROM_LEN = 0x0C80U;             \
        (*getNfcFL()).nfccFL._NFCC_DWNLD_MODE = NFCC_DWNLD_WITH_VEN_RESET;          \
        \
        \
        if (chipType == thn31)                                             \
        {                                                                   \
            (*getNfcFL()).nfccFL._NFCC_DWNLD_MODE = NFCC_DWNLD_WITH_NCI_CMD;        \
            (*getNfcFL()).nfccFL._NFCC_I2C_READ_WRITE_IMPROVEMENT = true;           \
            (*getNfcFL()).nfccFL._NFCC_MIFARE_TIANJIN = false;                      \
            (*getNfcFL()).nfccFL._NFCC_MW_RCVRY_BLK_FW_DNLD = true;                 \
            (*getNfcFL()).nfccFL._NFC_TMS_STAT_DUAL_UICC_EXT_SWITCH = false;        \
            (*getNfcFL()).nfccFL._NFC_TMS_STAT_DUAL_UICC_WO_EXT_SWITCH = false;     \
            (*getNfcFL()).nfccFL._NFCC_FW_WA = true;                                \
            (*getNfcFL()).nfccFL._NFCC_FORCE_NCI1_0_INIT = false;                   \
            (*getNfcFL()).nfccFL._NFCC_SPI_FW_DOWNLOAD_SYNC = true;                 \
            (*getNfcFL()).nfccFL._HW_ANTENNA_LOOP4_SELF_TEST = false;               \
            (*getNfcFL()).nfccFL._NFCEE_REMOVED_NTF_RECOVERY = true;                \
            (*getNfcFL()).nfccFL._NFCC_FORCE_FW_DOWNLOAD = true;                    \
            (*getNfcFL()).nfccFL._UICC_CREATE_CONNECTIVITY_PIPE = true;             \
            (*getNfcFL()).nfccFL._TMS_NFC_UICC_ETSI12 = false;                      \
            (*getNfcFL()).nfccFL._NFA_EE_MAX_EE_SUPPORTED = 3;                      \
            (*getNfcFL()).nfccFL._NFCC_ROUTING_BLOCK_BIT_PROP = false;              \
            (*getNfcFL()).nfccFL._NFCC_AID_MATCHING_PLATFORM_CONFIG = false;        \
            \
            \
            (*getNfcFL()).eseFL._ESE_ETSI12_PROP_INIT = true;                       \
            (*getNfcFL()).eseFL._EXCLUDE_NV_MEM_DEPENDENCY = true;                  \
            \
            \
            (*getNfcFL()).platformFL._NFCC_RESET_RSP_LEN = 0x10U;                   \
            \
            \
            (*getNfcFL()).nfcMwFL._NCI_INTERFACE_UICC_DIRECT = 0x82;                \
            (*getNfcFL()).nfcMwFL._NCI_INTERFACE_ESE_DIRECT = 0x83;                 \
            \
            \
        }                                                                   \
        else if(chipType == DEFAULT_CHIP_TYPE) {                            \
            (*getNfcFL()).nfccFL._NFCC_FORCE_FW_DOWNLOAD = true;                   \
        }                                                                  \
    }
#endif
