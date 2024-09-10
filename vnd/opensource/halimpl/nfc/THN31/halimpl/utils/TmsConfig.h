/******************************************************************************
 *
 *  Copyright (C) 1999-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
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
 *  The original Work has been changed by NXP.
 *
 *  Copyright (C) 2013-2021 NXP
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

#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

int getTmsStrValue(const char *pName, char *pValue, unsigned long len);
int getTmsNumValue(const char *pName, void *pValue, unsigned long len);
int getTmsByteArrayValue(const char *pName, char *pValue, long buffLen,
                         long *len);
void resetTmsConfig(void);
int isTmsRFConfigModified();
int isTmsConfigModified();
int updateTmsConfigTimestamp();
int updateTmsRfConfigTimestamp();
void setTmsRfConfigPath(const char *pName);
size_t readConfigFile(const char *pFileName, uint8_t **pData);

#ifdef __cplusplus
};
#endif

#define NAME_TMSLOG_EXTNS_LOGLEVEL "TMSLOG_EXTNS_LOGLEVEL"
#define NAME_TMSLOG_NCIHAL_LOGLEVEL "TMSLOG_NCIHAL_LOGLEVEL"
#define NAME_TMSLOG_NCIX_LOGLEVEL "TMSLOG_NCIX_LOGLEVEL"
#define NAME_TMSLOG_NCIR_LOGLEVEL "TMSLOG_NCIR_LOGLEVEL"
#define NAME_TMSLOG_TML_LOGLEVEL "TMSLOG_TML_LOGLEVEL"
#define NAME_TMS_NFC_DEV_NODE "TMS_NFC_DEV_NODE"
#define NAME_TMS_ACT_PROP_EXTN "TMS_ACT_PROP_EXTN"
#define NAME_TMS_CORE_CONF_EXTN "TMS_CORE_CONF_EXTN"
#define NAME_TMS_CORE_CONF "TMS_CORE_CONF"
#define NAME_TMS_CORE_RF_FIELD "TMS_CORE_RF_FIELD"
#define NAME_NFC_DEBUG_ENABLED "NFC_DEBUG_ENABLED"
#define NAME_TMS_SET_CONFIG_ALWAYS "TMS_SET_CONFIG_ALWAYS"
#define NAME_TMS_CORE_PROP_SYSTEM_DEBUG "TMS_CORE_PROP_SYSTEM_DEBUG"
#define NAME_DEFAULT_ROUTE "DEFAULT_ROUTE"
#define NAME_DEFAULT_SYS_CODE_ROUTE "DEFAULT_SYS_CODE_ROUTE"
#define NAME_DEFAULT_SYS_CODE_PWR_STATE "DEFAULT_SYS_CODE_PWR_STATE"
#define NAME_OFF_HOST_ESE_PIPE_ID "OFF_HOST_ESE_PIPE_ID"
#define NAME_OFF_HOST_SIM_PIPE_ID "OFF_HOST_SIM_PIPE_ID"
#define NAME_DEFAULT_OFFHOST_ROUTE "DEFAULT_OFFHOST_ROUTE"
#define NAME_DEFAULT_NFCF_ROUTE "DEFAULT_NFCF_ROUTE"
#define NAME_ISO_DEP_MAX_TRANSCEIVE "ISO_DEP_MAX_TRANSCEIVE"
#define NAME_NFA_POLL_BAIL_OUT_MODE "NFA_POLL_BAIL_OUT_MODE"
#define NAME_DEVICE_HOST_WHITE_LIST "DEVICE_HOST_WHITE_LIST"
#define NAME_NFA_PROPRIETARY_CFG "NFA_PROPRIETARY_CFG"
#define NAME_PRESENCE_CHECK_ALGORITHM "PRESENCE_CHECK_ALGORITHM"
#define NAME_TMS_PWR_OFF_LISTEN_TECH_MASK "TMS_PWR_OFF_LISTEN_TECH_MASK"
#define NAME_OFFHOST_ROUTE_ESE "OFFHOST_ROUTE_ESE"
#define NAME_OFFHOST_ROUTE_UICC "OFFHOST_ROUTE_UICC"
#define NAME_DEFAULT_ISODEP_ROUTE "DEFAULT_ISODEP_ROUTE"
#define NAME_TMS_TRANSPORT "TMS_TRANSPORT"

/* config passthrough to nfc stack */
#define NAME_TMS_MAX_APDU_WAIT_TIME "TMS_MAX_APDU_WAIT_TIME"
#define NAME_TMS_NFCEE_PL_CFG "TMS_NFCEE_PL_CFG"
#define NAME_TMS_NFCEE_PL_VALUE "TMS_NFCEE_PL_VALUE"
#define NAME_DEFAULT_T4TNFCEE_AID_POWER_STATE "DEFAULT_T4TNFCEE_AID_POWER_STATE"
#define NAME_TMS_T4T_NFCEE_ENABLE "TMS_T4T_NFCEE_ENABLE"
#define NAME_TMS_T4T_NDEF_NFCEE_AID "TMS_T4T_NDEF_NFCEE_AID"
#define NAME_TMS_SET_ALL_ROUTE_DEFAULT "TMS_SET_ALL_ROUTE_DEFAULT"
#define NAME_TMS_DEFAULT_HCE_TECH "TMS_DEFAULT_HCE_TECH"
#endif
