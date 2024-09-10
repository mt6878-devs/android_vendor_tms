/*
 * Copyright 2019-2021 NXP
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

#include <map>
#include <set>
#include <android-base/file.h>
#include <android-base/strings.h>
#include <android-base/parseint.h>
#include "TmsNciHal_ext.h"
#include "TmsNciHal_utils.h"
#include "TmsNciHal_Adaptation.h"
#include "NfcCommon.h"
#include "TmsNciHal_IoctlOperations.h"
#include "NfcCommon.h"
#include "TmsNciHal_Adaptation.h"
#include "NfccTransportFactory.h"
#include "NfccTransport.h"

using android::base::WriteStringToFile;
using namespace ::std;
using namespace ::android::base;

#define TERMINAL_LEN 5
/* HAL_NFC_STATUS_REFUSED sent to restart NFC service */
#define HAL_NFC_STATUS_RESTART HAL_NFC_STATUS_REFUSED

/*******************************************************************************
 **
 ** Function:        propertyGetIntf()
 **
 ** Description:     Gets property value for the input property name
 **
 ** Parameters       pPropName:   Name of the property whichs value need to get
 **                  pValueStr:   output value of the property.
 **                  pDefaultStr: default value of the property if value is not
 **                              there this will be set to output value.
 **
 ** Returns:         actual length of the property value
 **
 ********************************************************************************/
int propertyGetIntf(const char *pPropName, char *pValueStr,
                      const char *pDefaultStr) {
    string paramPropName = pPropName;
    string propValue;
    string propValueDefault = pDefaultStr;
    int len = 0;

    propValue = tmsNciHalGetSystemProperty(paramPropName);
    if (propValue.length() > 0) {
        TMSLOG_NCIHAL_D("propertyGetIntf , key[%s], propValue[%s], length[%zu]",
                        pPropName, propValue.c_str(), propValue.length());
        len = propValue.length();
        strlcpy(pValueStr, propValue.c_str(), PROP_VALUE_MAX);
    } else {
        if (propValueDefault.length() > 0) {
            len = propValueDefault.length();
            strlcpy(pValueStr, propValueDefault.c_str(), PROP_VALUE_MAX);
        }
    }

    return len;
}

/*******************************************************************************
 **
 ** Function:        propertySetIntf()
 **
 ** Description:     Sets property value for the input property name
 **
 ** Parameters       pPropName:   Name of the property whichs value need to set
 **                  pValueStr:   value of the property.
 **
 ** Returns:        returns 0 on success, < 0 on failure
 **
 ********************************************************************************/
int propertySetIntf(const char *pPropName, const char *pValueStr) {
    string paramPropName = pPropName;
    string propValue = pValueStr;
    TMSLOG_NCIHAL_D("propertySetIntf, key[%s], value[%s]", pPropName, pValueStr);
    if (tmsNciHalSetSystemProperty(paramPropName, propValue)) {
        return NFCSTATUS_SUCCESS;
    } else {
        return NFCSTATUS_FAILED;
    }
}

systemProperty gTmsSystemProperty = {
    {"nfc.tmsLog_level_global", ""},
    {"nfc.tmsLog_level_extns", ""},
    {"nfc.tmsLog_level_hal", ""},
    {"nfc.tmsLog_level_nci", ""},
    {"nfc.tmsLog_level_tml", ""},
    {"nfc.fw.dfl", ""},
    {"nfc.fw.downloadmode_force", ""},
    {"nfc.debugEnabled", ""},
    {"nfc.product.support.ese", ""},
    {"nfc.product.support.uicc", ""},
    {"nfc.product.support.uicc2", ""},
    {"nfc.fw.rfreg_ver", ""},
    {"nfc.fw.rfreg_display_ver", ""},
    {"nfc.fw.dfl_areacode", ""},
    {"nfc.cover.cover_id", ""},
    {"nfc.cover.state", ""},
};

/****************************************************************
 * Local Functions
 ***************************************************************/

/******************************************************************************
 ** Function         tmsNciHalIoctlIf
 **
 ** Description      This function shall be called from HAL when libnfc-nci
 **                  calls tmsNciHalIoctl() to perform any IOCTL operation
 **
 ** Returns          return 0 on success and -1 on fail,
 ******************************************************************************/
int tmsNciHalIoctlIf(long arg, void *pData) {
    TMSLOG_NCIHAL_D("%s : enter - arg = %ld", __func__, arg);
    int ret = -1;
    UNUSED_PROP(arg);
    UNUSED_PROP(pData);

    TMSLOG_NCIHAL_D("%s : exit - ret = %d", __func__, ret);
    return ret;
}

/*******************************************************************************
 **
 ** Function         tmsNciHalGetSystemProperty
 **
 ** Description      It shall be used to get property value of the given Key
 **
 ** Parameters       string key
 **
 ** Returns          If Key is found, returns the respective property values
 **                  else returns the null/empty string
 *******************************************************************************/
string tmsNciHalGetSystemProperty(string key) {
    string propValue;
    std::map<std::string, std::string>::iterator prop;

    prop = gTmsSystemProperty.find(key);
    if (prop != gTmsSystemProperty.end()) {
        propValue = prop->second;
    } else {
        /* else Pass a null string */
    }
    return propValue;
}
/*******************************************************************************
 **
 ** Function         tmsNciHalSetSystemProperty
 **
 ** Description      It shall be used to save/change value to system property
 **                  based on provided key.
 **
 ** Parameters       string key, string value
 **
 ** Returns          true if success, false if fail
 *******************************************************************************/
bool tmsNciHalSetSystemProperty(string key, string value) {
    bool stat = true;
    if (strcmp(key.c_str(), "nfc.debugEnabled") != 0)
        TMSLOG_NCIHAL_D("%s : Enter Key = %s, value = %s", __func__, key.c_str(),
                        value.c_str());

    unsigned tmp = 0;
    if (strcmp(key.c_str(), "nfc.debugEnabled") == 0) {
        if (ParseUint(value.c_str(), &tmp)) {
            if (tmsLogEnableDisableLogLevel((uint8_t)tmp) != NFCSTATUS_SUCCESS) {
                stat = false;
            }
        } else {
            TMSLOG_NCIHAL_W("%s : Failed to parse the string to uint. "
                            "nfc.debugEnabled string : %s",
                            __func__, value.c_str());
        }
    } else if (strcmp(key.c_str(), "nfc.cmd_timeout") == 0) {
        TMSLOG_NCIHAL_E("%s : nci_timeout, sem post", __func__);
        sem_post(&((*getTmsNciHalCtrl()).syncSpiNfc));
    }
    gTmsSystemProperty[key] = value;
    return stat;
}

/*******************************************************************************
**
** Function         tmsNciHalResetEse
**
** Description      It shall be used to reset eSE by proprietary command.
**
** Parameters
**
** Returns          status of eSE reset response
*******************************************************************************/
NFCSTATUS tmsNciHalResetEse(uint64_t resetType) {
    NFCSTATUS status = NFCSTATUS_FAILED;

    if ((*getTmsNciHalCtrl()).halStatus == HAL_STATUS_CLOSE) {
        if (NFCSTATUS_SUCCESS != tmsNciHalMinOpen()) {
            return NFCSTATUS_FAILED;
        }
    }

    CONCURRENCY_LOCK();
    status = (*getTransportObj())->eseReset((*getTmlNfcContext())->pDevHandle, (EseResetType)resetType);
    CONCURRENCY_UNLOCK();
    if (status != NFCSTATUS_SUCCESS) {
        TMSLOG_NCIHAL_E("EsePowerCycle failed");
    }

    if ((*getTmsNciHalCtrl()).halStatus == HAL_STATUS_MIN_OPEN) {
        tmsNciHalClose(false);
    }

    return status;
}

/******************************************************************************
** Function         tmsNciHalAbort
**
** Description      This function shall be used to trigger the abort in libnfc
**
** Parameters       None
**
** Returns          bool.
**
*******************************************************************************/
bool tmsNciHalAbort() {
    bool ret = true;

    TMSLOG_NCIHAL_D("tmsNciHalAbort aborting. \n");
    /* When JCOP download is triggered tmsNciHalOpen is blocked, in this case only
       we need to abort the libnfc , this can be done only by check the *getNfcStackCallbackBackup()
       pointer which is assigned before the JCOP download.*/
    if (*getNfcStackCallbackBackup() != NULL) {
        (**getNfcStackCallbackBackup())(HAL_NFC_OPEN_CPLT_EVT, HAL_NFC_STATUS_RESTART);
    } else {
        ret = false;
        TMSLOG_NCIHAL_D("tmsNciHalAbort not triggered\n");
    }
    return ret;
}
