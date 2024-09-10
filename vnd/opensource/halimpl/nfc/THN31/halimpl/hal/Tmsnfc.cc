/******************************************************************************
 * Copyright (c) 2021-2022 Tsingteng MicroSystem
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
 *****************************************************************************/

#include <log/log.h>

#include <map>
#include <set>
#include <android-base/file.h>
#include <android-base/strings.h>
#include <android-base/parseint.h>

#include "tmsDlNciUtils.h"
#include "tmsNfccDl.h"
#include "tmsCosI2cDl.h"
#include "TmsNciHal_IoctlOperations.h"
#include <TmlNfc.h>

using android::base::WriteStringToFile;
using namespace ::std;
using namespace ::android::base;

extern systemProperty gTmsSystemProperty;
const char default_tms_config_path[] = "/vendor/etc/libnfc-tms.conf";
std::set<string> gNciConfigs = {NAME_TMS_MAX_APDU_WAIT_TIME,
                                NAME_TMS_NFCEE_PL_CFG,
                                NAME_TMS_NFCEE_PL_VALUE,
                                NAME_DEFAULT_T4TNFCEE_AID_POWER_STATE,
                                NAME_TMS_T4T_NFCEE_ENABLE,
                                NAME_TMS_T4T_NDEF_NFCEE_AID,
                                NAME_TMS_SET_ALL_ROUTE_DEFAULT,
                                NAME_TMS_DEFAULT_HCE_TECH};

/*******************************************************************************
**
** Function         phTmsNciHal_CheckKeyNeeded
**
** Description      Check if the config needed for libnfc as per gNciConfigs
*                   list
**
** Parameters       string config
**
** Returns          bool(true/false)
*******************************************************************************/
static bool phTmsNciHal_CheckKeyNeeded(string key) {
    return ((gNciConfigs.find(key) != gNciConfigs.end()) ? true : false);
}

/*******************************************************************************
**
** Function         phTmsNciHal_parseBytesString
**
** Description      Parse bytes from string
**
** Parameters       string config
**
** Returns          Resultant string
*******************************************************************************/
static string phTmsNciHal_parseBytesString(string in) {
    size_t pos;
    in.erase(remove(in.begin(), in.end(), ' '), in.end());
    pos = in.find(",");
    while (pos != string::npos) {
        in = in.replace(pos, 1, ":");
        pos = in.find(",", pos);
    }
    return in;
}

/*******************************************************************************
**
** Function         phTmsNciHal_parseValueFromString
**
** Description      Parse value determine data type of config option
**
** Parameters       string config
**
** Returns          bool(true/false)
*******************************************************************************/
static bool phTmsNciHal_parseValueFromString(string& in) {
    unsigned tmp = 0;
    bool stat = false;
    if (in.length() >= 1) {
        switch (in[0]) {
            case '"':
                if (in[in.length() - 1] == '"' && in.length() > 2) {
                    stat = true;
                }
                break;
            case '{':
                if (in[in.length() - 1] == '}' && in.length() >= 3) {
                    in = phTmsNciHal_parseBytesString(in);
                    stat = true;
                }
                break;
            default:
                if (ParseUint(in.c_str(), &tmp)) {
                    stat = true;
                }
                break;
        }
    } else {
        ALOGE("Tmsnfc::%s : Invalid config string ", __func__);
    }
    return stat;
}

/*******************************************************************************
**
** Function         phTmsNciHal_extractConfig
**
** Description      It parses complete config file and extracts only
*                   enabled options ignores comments etc.
**
** Parameters       string config
**
** Returns          Resultant string
*******************************************************************************/
static string phTmsNciHal_extractConfig(string& config) {
    stringstream ss(config);
    string line;
    string result;

    while (getline(ss, line)) {
        line = Trim(line);
        if (line.empty()) {
            continue;
        }
        if (line.at(0) == '#') {
            continue;
        }
        if (line.at(0) == 0) {
            continue;
        }

        auto search = line.find('=');
        if (search == string::npos) {
            continue;
        }

        string key(Trim(line.substr(0, search)));
        if (!phTmsNciHal_CheckKeyNeeded(key)) {
            continue;
        }
        string value_string(Trim(line.substr(search + 1, string::npos)));

        if (!phTmsNciHal_parseValueFromString(value_string)) {
            continue;
        }
        ALOGD("Tmsnfc::%s : passthrough [%s]=%s conf to nfc stack", __func__, key.c_str(), value_string.c_str());
        line = key + "=" + value_string + "\n";
        result += line;
    }

    return result;
}

/*******************************************************************************
**
** Function         phTmsNciHal_getTmsConfig
**
** Description      It shall be used to read config values from the
*libnfc-tms.conf
**
** Parameters       tmsConfigs config
**
** Returns          void
*******************************************************************************/
string phTmsNciHal_getTmsConfigIf() {
    std::string config;
    uint8_t *p_config = nullptr;
    size_t config_size = readConfigFile(default_tms_config_path, &p_config);
    if (config_size) {
        config.assign((char *)p_config, config_size);
        delete[] p_config;
        config = phTmsNciHal_extractConfig(config);
    }
    return config;
}

/*******************************************************************************
 **
 ** Function         phTmsNciHalGetSystemProperty
 **
 ** Description      It shall be used to get property value of the given Key
 **
 ** Parameters       string key
 **
 ** Returns          If Key is found, returns the respective property values
 **                  else returns the null/empty string
 *******************************************************************************/
string phTmsNciHalGetSystemProperty(string key) {
    string propValue;
    std::map<std::string, std::string>::iterator prop;

    if (0 == key.compare("libnfc-tms.conf")) {
        return phTmsNciHal_getTmsConfigIf();
    }

    prop = gTmsSystemProperty.find(key);
    if (prop != gTmsSystemProperty.end()) {
        propValue = prop->second;
    } else {
        /* else Pass a null string */
    }
    return propValue;
}

/******************************************************************************
* Function         phTmsNciHalSetTmsTransitConfig
*
* Description      This function overwrite libnfc-tmsTransit.conf file
*                  with transitConfValue.
*
* Returns          bool.
*
******************************************************************************/
bool phTmsNciHalSetTmsTransitConfig(char *transitConfValue) {
    bool status = true;
    ALOGD("Tmsnfc::%s Enter", __FUNCTION__);
    std::string transitConfFileName = "/data/vendor/nfc/libnfc-tmsTransit.conf";
    long transitConfValueLen = strlen(transitConfValue) + 1;

    if (transitConfValueLen > 1) {
        if (!WriteStringToFile(transitConfValue, transitConfFileName)) {
            ALOGE("Tmsnfc::%s WriteStringToFile: Failed", __FUNCTION__);
            status = false;
        }
    } else {
        if (!WriteStringToFile("", transitConfFileName)) {
            ALOGE("Tmsnfc::%s WriteStringToFile: Write blank Failed", __FUNCTION__);
            status = false;
        }
        if (remove(transitConfFileName.c_str())) {
            ALOGE("Tmsnfc::%s Unable to remove file", __FUNCTION__);
            status = false;
        }
    }
    ALOGD("Tmsnfc::%s Exit", __FUNCTION__);
    return status;
}

/*******************************************************************************
**
** Function         phTmsNciHalEseSoftReset
**
** Description      It shall be used to reset eSE by proprietary command.
**
** Returns          status of eSE reset result
*******************************************************************************/
bool phTmsNciHalEseSoftReset() {
    bool ret = false;
    TmlNfcContext_t* pTmlNfcContext = *getTmlNfcContext();
    if (nullptr != pTmlNfcContext) {
    ret = EseSoftReset(pTmlNfcContext, &pTmlNfcContext->rxSemaphore,
                       (uint8_t *)&pTmlNfcContext->readInfo.bEnable,
                       &pTmlNfcContext->readInfo.bThreadBusy,
                       (uint8_t *)&pTmlNfcContext->writeInfo.bEnable,
                       &pTmlNfcContext->writeInfo.bThreadBusy);
    } else {
      ALOGE("Tmsnfc::%s gpphTmlNfc_Context is NULL, nfc is not enabled", __FUNCTION__);
      ret = EseSoftReset(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    }
    if (!ret) {
      ALOGE("Tmsnfc::%s failed", __FUNCTION__);
    }

    return ret;
}

/*******************************************************************************
**
** Function         phTmsNciHalNfccFwDownload
**
** Description      It shall be used to download nfcc fw.
**
** Returns          status of fw download result.
*******************************************************************************/
bool phTmsNciHalNfccFwDownload() {
    bool ret = NfccFwDownload() == 0? true : false;
    if (!ret) {
      ALOGE("Tmsnfc::%s failed", __FUNCTION__);
    }

    return ret;
}

/*******************************************************************************
**
** Function         doEseCosDownloadI2C
**
** Description      It shall be used to download ese cos by i2c.
**
** Returns          status of fw download result.
*******************************************************************************/
int16_t doEseCosDownloadI2C() {
    int16_t ret = eseCosDownloadI2C();
    return ret;
}

/*******************************************************************************
**
** Function         doNfccFwDownload
**
** Description      It shall be used to download nfcc fw.
**
** Returns          status of fw download result.
*******************************************************************************/
int16_t doNfccFwDownload() {
    int16_t ret = NfccFwDownload();
    return ret;
}

/*******************************************************************************
**
** Function         doNfccBlDownload
**
** Description      It shall be used to download nfcc BL.
**
** Returns          status of fw download result.
*******************************************************************************/
int16_t doNfccBlDownload() {
    int16_t ret = NfccBlDownload();
    return ret;
}
