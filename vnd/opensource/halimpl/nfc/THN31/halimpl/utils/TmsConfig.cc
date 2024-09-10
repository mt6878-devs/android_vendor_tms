/******************************************************************************
 *
 *  Copyright (C) 2011-2012 Broadcom Corporation
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
 *  Copyright 2013-2021 NXP
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

#include <stdio.h>
#include <sys/stat.h>
#include <list>
#include <string>
#include <vector>
#include <log/log.h>
#include <android-base/properties.h>
#include <sstream>
#include "Sparse_crc32.h"
#include <errno.h>
#include <TmsConfig.h>
#include <TmsLog.h>
#ifdef TMS_NFC //THNDEV-1963 modify merge configs for Transit.conf
#include <iomanip>
#include <regex>
#include "TmsNfcThreadMutex.h"
#endif
#if GENERIC_TARGET
    const char gAlternativeConfigPath[] = "/data/vendor/nfc/";
#else
    const char gAlternativeConfigPath[] = "";
#endif

#if 1
const char *gpTransportConfigPaths[] = {"/odm/etc/", "/vendor/etc/", "/etc/", "/data/vendor/nfc/"};
#else
const char *gpTransportConfigPaths[] = {"res/"};
#endif
const int gTransportConfigPathSize =
    (sizeof(gpTransportConfigPaths) / sizeof(gpTransportConfigPaths[0]));

#define CONFIG_NAME "libnfc-tms.conf"
#define EXTRA_CONFIG_BASE "libnfc-"
#define EXTRA_CONFIG_EXT ".conf"
#define IS_STRING_VALUE 0x80000000

typedef enum {
    CONF_FILE_TMS = 0x00,
    CONF_FILE_TMS_RF,
    CONF_FILE_TMS_TRANSIT
} TmsConfFile;

static bool isRfConfigPathModified = false;
const char gRfConfigTimestampPath[] =
    "/data/vendor/nfc/libnfc-tmsRFConfigState.bin";
const char gRrConfigTimestampPath[] =
    "/data/vendor/nfc/libnfc-tmsTransitConfigState.bin";
const char gConfigTimestampPath[] =
    "/data/vendor/nfc/libnfc-tmsConfigState.bin";
char gTmsRfConfigPath[256] =
    "/vendor/etc/libnfc-tms_RF.conf";
char gTmsConfigPath[256] =
    "/vendor/etc/libnfc-tms.conf";

#ifdef TMS_NFC
static NfcHalThreadMutex ghalConfLock;
#endif
const char gTransitConfigPath[] = "/data/vendor/nfc/libnfc-tmsTransit.conf";
void readOptionalConfig(const char *pOptional);

size_t readConfigFile(const char *pFileName, uint8_t **ppData) {
    FILE *fd = fopen(pFileName, "rb");
    if (fd == nullptr) {
        return 0;
    }

    fseek(fd, 0L, SEEK_END);
    const size_t fileSize = ftell(fd);
    rewind(fd);
    if ((long)fileSize < 0) {
        ALOGE("%s Invalid file size fileSize = %zu\n", __func__, fileSize);
        fclose(fd);
        return 0;
    }
    uint8_t *pBuffer = new uint8_t[fileSize + 1];
    if (!pBuffer) {
        fclose(fd);
        return 0;
    }
    size_t read = fread(pBuffer, fileSize, 1, fd);
    fclose(fd);

    if (read == 1) {
        pBuffer[fileSize] = '\n';
        *ppData = pBuffer;
        return fileSize + 1;
    }
    delete[] pBuffer;
    return 0;
}


using namespace ::std;

class CNfcParam : public string {
  public:
    CNfcParam();
    CNfcParam(const char *pName, const string& value);
    CNfcParam(const char *pName, unsigned long value);
    virtual ~CNfcParam();
    unsigned long numValue() const {
        return m_numValue;
    }
    const char *str_value() const {
        return m_str_value.c_str();
    }
    size_t strLen() const {
        return m_str_value.length();
    }

  private:
    string m_str_value;
    unsigned long m_numValue;
};

class CNfcConfig : public vector<const CNfcParam *> {
  public:
    virtual ~CNfcConfig();
    static CNfcConfig& getInstance();
    friend void readOptionalConfig(const char *pOptional);
    bool isModified(TmsConfFile aType);
    void resetModified(TmsConfFile aType);

    bool getValue(const char *pName, char *pValue, size_t len) const;
    bool getValue(const char *pName, unsigned long& rValue) const;
    bool getValue(const char *pName, unsigned short& rValue) const;
    bool getValue(const char *pName, char *pValue, long len, long *readlen) const;
    const CNfcParam *find(const char *pName) const;
    void readTmsTransitConfig(const char *pFileName) const;
    void readTmsRFConfig(const char *pFileName) const;
    void clean();

  private:
    CNfcConfig();
    bool readConfig(const char *pName, bool bResetContent);
    void moveFromList();
    void moveToList();
    void add(const CNfcParam *pParam);
    void dump();
    bool isAllowed(const char *pName);
    list<const CNfcParam *> m_list;
    bool mValidFile;
    uint32_t config_crc32_;
    uint32_t config_rf_crc32_;
    uint32_t config_tr_crc32_;
    string mCurrentFile;

    unsigned long state;

    inline bool Is(unsigned long f) {
        return (state & f) == f;
    }
    inline void Set(unsigned long f) {
        state |= f;
    }
    inline void Reset(unsigned long f) {
        state &= ~f;
    }
};

/*******************************************************************************
**
** Function:    isPrintable()
**
** Description: determine if 'c' is printable
**
** Returns:     1, if printable, otherwise 0
**
*******************************************************************************/
inline bool isPrintable(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || c == '/' || c == '_' || c == '-' || c == '.';
}

/*******************************************************************************
**
** Function:    isDigit()
**
** Description: determine if 'c' is numeral digit
**
** Returns:     true, if numerical digit
**
*******************************************************************************/
inline bool isDigit(char c, int base) {
    if ('0' <= c && c <= '9') {
        return true;
    }
    if (base == 16) {
        if (('A' <= c && c <= 'F') || ('a' <= c && c <= 'f')) {
            return true;
        }
    }
    return false;
}

/*******************************************************************************
**
** Function:    getDigitValue()
**
** Description: return numerical value of a decimal or hex char
**
** Returns:     numerical value if decimal or hex char, otherwise 0
**
*******************************************************************************/
inline int getDigitValue(char c, int base) {
    if ('0' <= c && c <= '9') {
        return c - '0';
    }
    if (base == 16) {
        if ('A' <= c && c <= 'F') {
            return c - 'A' + 10;
        } else if ('a' <= c && c <= 'f') {
            return c - 'a' + 10;
        }
    }
    return 0;
}

/*******************************************************************************
**
** Function:    findConfigFilePathFromTransportConfigPaths()
**
** Description: find a config file path with a given config pName from transport
**              config paths
**
** Returns:     none
**
*******************************************************************************/
bool findConfigFilePathFromTransportConfigPaths(const string& configName,
        string& filePath) {
    for (int i = 0; i < gTransportConfigPathSize; i++) {
        if (configName.empty()) {
            break;
        }
        filePath.assign(gpTransportConfigPaths[i]);
        filePath += configName;
        struct stat fileStat;
        if (stat(filePath.c_str(), &fileStat) == 0 && S_ISREG(fileStat.st_mode)) {
            return true;
        }
    }
    filePath = "";
    return false;
}

/*******************************************************************************
**
** Function:    CNfcConfig::readConfig()
**
** Description: read Config settings and parse them into a linked list
**              move the element from linked list to a array at the end
**
** Returns:     1, if there are any config data, 0 otherwise
**
*******************************************************************************/
bool CNfcConfig::readConfig(const char *pName, bool bResetContent) {
    enum {
        BEGIN_LINE = 1,
        TOKEN,
        STR_VALUE,
        NUM_VALUE,
        BEGIN_HEX,
        BEGIN_QUOTE,
        END_LINE
    };

    uint8_t *pConfig = nullptr;
    size_t configSize = readConfigFile(pName, &pConfig);
    if (pConfig == nullptr) {
        ALOGE("%s Cannot open config file %s\n", __func__, pName);
        if (bResetContent) {
            ALOGE("%s Using default value for all settings\n", __func__);
            mValidFile = false;
        }
        return false;
    }

    string token;
    string strValue;
    unsigned long numValue = 0;
    CNfcParam *pParam = NULL;
    int i = 0;
    int base = 0;
    char c;
    int bflag = 0;
    state = BEGIN_LINE;

    ALOGD("readConfig; filename is %s", pName);
    if (strcmp(pName, gTmsRfConfigPath) == 0) {
        config_rf_crc32_ = sparseCrc32(0, (const void *)pConfig, (int)configSize);
    } else if (strcmp(pName, gTransitConfigPath) == 0) {
        config_tr_crc32_ = sparseCrc32(0, (const void *)pConfig, (int)configSize);
    } else {
        config_crc32_ = sparseCrc32(0, (const void *)pConfig, (int)configSize);
    }

    mValidFile = true;
    if (size() > 0) {
        if (bResetContent) {
            clean();
        } else {
            moveToList();
        }
    }

    for (size_t offset = 0; offset != configSize; ++offset) {
        c = pConfig[offset];
        switch (state & 0xff) {
            case BEGIN_LINE:
                if (c == '#') {
                    state = END_LINE;
                } else if (isPrintable(c)) {
                    i = 0;
                    token.erase();
                    strValue.erase();
                    state = TOKEN;
                    token.push_back(c);
                }
                break;
            case TOKEN:
                if (c == '=') {
                    token.push_back('\0');
                    state = BEGIN_QUOTE;
                } else if (isPrintable(c)) {
                    token.push_back(c);
                } else {
                    state = END_LINE;
                }
                break;
            case BEGIN_QUOTE:
                if (c == '"') {
                    state = STR_VALUE;
                    base = 0;
                } else if (c == '0') {
                    state = BEGIN_HEX;
                } else if (isDigit(c, 10)) {
                    state = NUM_VALUE;
                    base = 10;
                    numValue = getDigitValue(c, base);
                    i = 0;
                } else if (c == '{') {
                    state = NUM_VALUE;
                    bflag = 1;
                    base = 16;
                    i = 0;
                    Set(IS_STRING_VALUE);
                } else {
                    state = END_LINE;
                }
                break;
            case BEGIN_HEX:
                if (c == 'x' || c == 'X') {
                    state = NUM_VALUE;
                    base = 16;
                    numValue = 0;
                    i = 0;
                    break;
                } else if (isDigit(c, 10)) {
                    state = NUM_VALUE;
                    base = 10;
                    numValue = getDigitValue(c, base);
                    break;
                } else if (c != '\n' && c != '\r') {
                    state = END_LINE;
                    break;
                }
                // fall through to numValue to handle numValue
                [[fallthrough]];
            case NUM_VALUE:
                if (isDigit(c, base)) {
                    numValue *= base;
                    numValue += getDigitValue(c, base);
                    ++i;
                } else if (bflag == 1 &&
                           (c == ' ' || c == '\r' || c == '\n' || c == '\t')) {
                    break;
                } else if (base == 16 &&
                           (c == ',' || c == ':' || c == '-' || c == ' ' || c == '}')) {
                    if (c == '}') {
                        bflag = 0;
                    }
                    if (i > 0) {
                        int n = (i + 1) / 2;
                        while (n-- > 0) {
                            numValue = numValue >> (n * 8);
                            unsigned char c = (numValue) & 0xFF;
                            strValue.push_back(c);
                        }
                    }

                    Set(IS_STRING_VALUE);
                    numValue = 0;
                    i = 0;
                } else {
                    if (c == '\n' || c == '\r') {
                        if (bflag == 0) {
                            state = BEGIN_LINE;
                        }
                    } else {
                        if (bflag == 0) {
                            state = END_LINE;
                        }
                    }
                    if (Is(IS_STRING_VALUE) && base == 16 && i > 0) {
                        int n = (i + 1) / 2;
                        while (n-- > 0) {
                            strValue.push_back(((numValue >> (n * 8)) & 0xFF));
                        }
                    }
                    if (strValue.length() > 0) {
                        pParam = new CNfcParam(token.c_str(), strValue);
                    } else {
                        pParam = new CNfcParam(token.c_str(), numValue);
                    }
                    add(pParam);
                    strValue.erase();
                    numValue = 0;
                }
                break;
            case STR_VALUE:
                if (c == '"') {
                    strValue.push_back('\0');
                    state = END_LINE;
                    pParam = new CNfcParam(token.c_str(), strValue);
                    add(pParam);
                } else if (isPrintable(c)) {
                    strValue.push_back(c);
                }
                break;
            case END_LINE:
                if (c == '\n' || c == '\r') {
                    state = BEGIN_LINE;
                }
                break;
            default:
                break;
        }
    }

    delete[] pConfig;

    moveFromList();
    return size() > 0;
}

/*******************************************************************************
**
** Function:    CNfcConfig::CNfcConfig()
**
** Description: class constructor
**
** Returns:     none
**
*******************************************************************************/
CNfcConfig::CNfcConfig()
    : mValidFile(true),
      config_crc32_(0),
      config_rf_crc32_(0),
      config_tr_crc32_(0),
      state(0) {}

/*******************************************************************************
**
** Function:    CNfcConfig::~CNfcConfig()
**
** Description: class destructor
**
** Returns:     none
**
*******************************************************************************/
CNfcConfig::~CNfcConfig() {}

/*******************************************************************************
**
** Function:    CNfcConfig::getInstance()
**
** Description: get class singleton object
**
** Returns:     none
**
*******************************************************************************/
CNfcConfig& CNfcConfig::getInstance() {
    static CNfcConfig theInstance;
#ifdef TMS_NFC
    NfcHalAutoThreadMutex a(ghalConfLock);
#endif
    if (isRfConfigPathModified) {
        theInstance.clean();
        isRfConfigPathModified = false;
    }
    if (theInstance.size() == 0 && theInstance.mValidFile) {
        string strPath;
        if (gAlternativeConfigPath[0] != '\0') {
            strPath.assign(gAlternativeConfigPath);
            strPath += CONFIG_NAME;
            theInstance.readConfig(strPath.c_str(), true);
            if (!theInstance.empty()) {
                return theInstance;
            }
        }

        if (findConfigFilePathFromTransportConfigPaths(
                    android::base::GetProperty("persist.vendor.nfc.configFile_name", ""),
                    strPath)) {
            TMSLOG_EXTNS_D("%s load %s\n", __func__,  strPath.c_str());
        } else if (findConfigFilePathFromTransportConfigPaths(
                       EXTRA_CONFIG_BASE +
                       android::base::GetProperty("ro.boot.product.hardware.sku", "") +
                       + EXTRA_CONFIG_EXT, strPath)) {
            TMSLOG_EXTNS_D("%s load %s\n", __func__,  strPath.c_str());
        } else {
            findConfigFilePathFromTransportConfigPaths(CONFIG_NAME, strPath);
            if (strcmp(strPath.c_str(), "") == 0) {
                TMSLOG_EXTNS_D("readConfig Cannot open config file %s\n", CONFIG_NAME);
            }
        }

        theInstance.readConfig(strPath.c_str(), true);
#ifdef TMS_NFC
        theInstance.readConfig(gTmsRfConfigPath, false);
        theInstance.readConfig(gTransitConfigPath, false);
#else
        theInstance.readTmsRFConfig(gTmsRfConfigPath);
        theInstance.readTmsTransitConfig(gTransitConfigPath);
#endif
    }
    return theInstance;
}

/*******************************************************************************
**
** Function:    CNfcConfig::getValue()
**
** Description: get a string value of a setting
**
** Returns:     true if setting exists
**              false if setting does not exist
**
*******************************************************************************/
bool CNfcConfig::getValue(const char *pName, char *pValue, size_t len) const {
    const CNfcParam *pParam = find(pName);
    if (pParam == NULL) {
        return false;
    }

    if (pParam->strLen() > 0) {
        memset(pValue, 0, len);
        memcpy(pValue, pParam->str_value(), pParam->strLen());
        return true;
    }
    return false;
}

bool CNfcConfig::getValue(const char *pName, char *pValue, long len,
                          long *readlen) const {
    const CNfcParam *pParam = find(pName);
    if (pParam == NULL) {
        return false;
    }

    if (pParam->strLen() > 0) {
        if (pParam->strLen() <= (unsigned long)len) {
            memset(pValue, 0, len);
            memcpy(pValue, pParam->str_value(), pParam->strLen());
            *readlen = pParam->strLen();
        } else {
            *readlen = -1;
        }

        return true;
    }
    return false;
}

/*******************************************************************************
**
** Function:    CNfcConfig::getValue()
**
** Description: get a long numerical value of a setting
**
** Returns:     true if setting exists
**              false if setting does not exist
**
*******************************************************************************/
bool CNfcConfig::getValue(const char *pName, unsigned long& rValue) const {
    const CNfcParam *pParam = find(pName);
    if (pParam == NULL) {
        return false;
    }

    if (pParam->strLen() == 0) {
        rValue = static_cast<unsigned long>(pParam->numValue());
        return true;
    }
    return false;
}

/*******************************************************************************
**
** Function:    CNfcConfig::getValue()
**
** Description: get a short numerical value of a setting
**
** Returns:     true if setting exists
**              false if setting does not exist
**
*******************************************************************************/
bool CNfcConfig::getValue(const char *pName, unsigned short& rValue) const {
    const CNfcParam *pParam = find(pName);
    if (pParam == NULL) {
        return false;
    }

    if (pParam->strLen() == 0) {
        rValue = static_cast<unsigned short>(pParam->numValue());
        return true;
    }
    return false;
}

/*******************************************************************************
**
** Function:    CNfcConfig::find()
**
** Description: search if a setting exist in the setting array
**
** Returns:     pointer to the setting object
**
*******************************************************************************/
const CNfcParam *CNfcConfig::find(const char *pName) const {
    if (size() == 0) {
        return NULL;
    }

    for (const_iterator it = begin(), itEnd = end(); it != itEnd; ++it) {
        if (**it < pName) {
            continue;
        } else if (**it == pName) {
            if ((*it)->strLen() > 0) {
                TMSLOG_EXTNS_D("%s found %s=%s\n", __func__, pName,
                               (*it)->str_value());
            } else {
                TMSLOG_EXTNS_D("%s found %s=(0x%lx)\n", __func__, pName,
                               (*it)->numValue());
            }
            return *it;
        } else {
            break;
        }
    }
    return NULL;
}

/*******************************************************************************
**
** Function:    CNfcConfig::readTmsTransitConfig()
**
** Description: read Config settings from transit conf file
**
** Returns:     none
**
*******************************************************************************/
void CNfcConfig::readTmsTransitConfig(const char *pFileName) const {
    ALOGD("readTmsTransitConfig-Enter..Reading %s", pFileName);
    CNfcConfig::getInstance().readConfig(pFileName, false);
}

/*******************************************************************************
**
** Function:    CNfcConfig::readTmsRFConfig()
**
** Description: read Config settings from RF conf file
**
** Returns:     none
**
*******************************************************************************/
void CNfcConfig::readTmsRFConfig(const char *pFileName) const {
    ALOGD("readTmsRFConfig-Enter..Reading %s", pFileName);
    CNfcConfig::getInstance().readConfig(pFileName, false);
}

/*******************************************************************************
**
** Function:    CNfcConfig::clean()
**
** Description: reset the setting array
**
** Returns:     none
**
*******************************************************************************/
void CNfcConfig::clean() {
    if (size() == 0) {
        return;
    }

    for (iterator it = begin(), itEnd = end(); it != itEnd; ++it) {
        delete *it;
    }
    clear();
}

#ifdef TMS_NFC //THNDEV-1963 modify merge configs for Transit.conf
struct RegisterEntry {
    std::vector<uint8_t> registerAddress; // register address
    uint8_t length;                       // lenth of register values
    std::vector<uint8_t> values;          // register values
};

/*******************************************************************************
**
** Function:    toHexString()
**
** Description: Convert the input byte data to a hex string format.
**
** Returns:     std::string - the formatted hex string
**
*******************************************************************************/
static std::string toHexString(const std::vector<uint8_t>& data) {
    std::stringstream ss;
    for (const auto& byte : data) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " ";
    }
    return ss.str();
}

/*******************************************************************************
**
** Function:    parseRegisterEntries()
**
** Description: Parses a vector of bytes to extract register entries.
**
** Returns:     std::vector<RegisterEntry> - List of parsed register entries
**
*******************************************************************************/
static std::vector<RegisterEntry> parseRegisterEntries(const std::vector<uint8_t>& data) {
    std::vector<RegisterEntry> entries;
    size_t idx = 4; // skip NCI head

    while (idx < data.size()) {
        RegisterEntry entry;

        // for A0 0D
        if (idx <= data.size() - 5 && data[idx] == 0xA0 && data[idx + 1] == 0x0D) {
            entry.registerAddress = {data[idx], data[idx + 1], data[idx + 2], data[idx + 3], data[idx + 4]};
            entry.length = data[idx + 2] - 2;
            if (idx + 5 + entry.length <= data.size()) {
                entry.values.insert(entry.values.end(), data.begin() + idx + 5, data.begin() + idx + 5 + entry.length);
                idx += 5 + entry.length;
            } else {
                return {}; // Return an empty vector on parsing failure
            }
        }
        // for < A0
        else if (idx <= data.size() - 2 && data[idx] < 0xA0) {
            entry.registerAddress = {data[idx]};
            entry.length = data[idx + 1];
            if (idx + 2 + entry.length <= data.size()) {
                entry.values.insert(entry.values.end(), data.begin() + idx + 2, data.begin() + idx + 2 + entry.length);
                idx += 2 + entry.length;
            } else {
                return {}; // Return an empty vector on parsing failure
            }
        }
        // for others
        else if (idx <= data.size() - 3) {
            entry.registerAddress = {data[idx], data[idx + 1]};
            entry.length = data[idx + 2];
            if (idx + 3 + entry.length <= data.size()) {
                entry.values.insert(entry.values.end(), data.begin() + idx + 3, data.begin() + idx + 3 + entry.length);
                idx += 3 + entry.length;
            } else {
                return {}; // Return an empty vector on parsing failure
            }
        } else {
            return {}; // Return an empty vector on parsing failure
        }

        entries.push_back(entry);
    }

    return entries;
}

/*******************************************************************************
**
** Function:    parseRegisterEntriesFromCNfcParam()
**
** Description: Parses a CNfcParam object to extract register entries.
**
** Returns:     std::vector<RegisterEntry> - List of parsed register entries
**
*******************************************************************************/
static std::vector<RegisterEntry> parseRegisterEntriesFromCNfcParam(const CNfcParam* param) {
    const char* strVal = param->str_value();
    std::vector<uint8_t> data(strVal, strVal + param->strLen());
    return parseRegisterEntries(data);
}

/*******************************************************************************
**
** Function:    mergeEntries()
**
** Description: Merges two sets of register entries.
**
** Returns:     std::vector<uint8_t> - merged list of register entries in byte format
**
*******************************************************************************/
static std::vector<uint8_t> mergeEntries(const CNfcParam* existingParam, const CNfcParam* newParam) {
    std::vector<RegisterEntry> oldEntries = parseRegisterEntriesFromCNfcParam(existingParam);
    std::vector<RegisterEntry> newEntries = parseRegisterEntriesFromCNfcParam(newParam);

    if (oldEntries.empty() || newEntries.empty()) {
        ALOGE("Error merging entries: Failed to parse register entries");
        return {}; // Return an empty vector on failure
    }

    for (const auto& newEntry : newEntries) {
        bool found = false;

        for (auto& oldEntry : oldEntries) {
            if (newEntry.registerAddress == oldEntry.registerAddress) {
                oldEntry.values = newEntry.values;
                found = true;
                break;
            }
        }

        if (!found) {
            oldEntries.push_back(newEntry);
        }
    }

    std::vector<uint8_t> result;
    uint8_t registerCount = 0;

    for (const auto& entry : oldEntries) {
        result.insert(result.end(), entry.registerAddress.begin(), entry.registerAddress.end());

        if (entry.registerAddress[0] != 0xA0 || entry.registerAddress[1] != 0x0D) {
            result.push_back(entry.length);
        }

        result.insert(result.end(), entry.values.begin(), entry.values.end());

        registerCount++;
    }

    uint16_t valueLength = result.size();
    uint16_t totalLength = valueLength + 1;
    std::vector<uint8_t> header = {
        0x20, 0x02,
        static_cast<uint8_t>(totalLength),
        static_cast<uint8_t>(registerCount)
    };

    header.insert(header.end(), result.begin(), result.end());

    return header;
}

/*******************************************************************************
**
** Function:    merge()
**
** Description: Merges two CNfcParam objects based on the key values and returns the merged CNfcParam.
**
** Returns:     const CNfcParam* - Pointer to the merged CNfcParam object
**
*******************************************************************************/
const CNfcParam* merge(const CNfcParam* existingParam, const CNfcParam* newParam) {
    std::string key(existingParam->c_str());
    std::regex pattern("TMS_RF_CONF_BLK_\\d+");

    if (key == "TMS_CORE_CONF" || key == "TMS_CORE_CONF_EXTN" || std::regex_match(key, pattern)) {
        ALOGD("Attempting to merge with existing parameter...");
        std::vector<uint8_t> mergedData = mergeEntries(existingParam, newParam);
        if (mergedData.empty()) {
            return newParam; // Return newParam if merging failed
        }
        std::string mergedStr(mergedData.begin(), mergedData.end());
        ALOGD("Merged parameter : %s = (HexString): %s", existingParam->c_str(), toHexString(mergedData).c_str());
        const CNfcParam* mergedParam = new CNfcParam(existingParam->c_str(), mergedStr);
        delete newParam;
        return mergedParam;
    } else {
        return newParam;
    }
}
#endif

/*******************************************************************************
**
** Function:    CNfcConfig::Add()
**
** Description: add a setting object to the list
**
** Returns:     none
**
*******************************************************************************/
void CNfcConfig::add(const CNfcParam *pParam) {
    if (m_list.size() == 0) {
        m_list.push_back(pParam);
        return;
    }
    if ((mCurrentFile.find("tmsTransit") != std::string::npos) &&
            !isAllowed(pParam->c_str())) {
        ALOGD("%s Token restricted. Returning", __func__);
        return;
    }
    for (list<const CNfcParam *>::iterator it = m_list.begin(),
            itEnd = m_list.end();
            it != itEnd; ++it) {
        if (**it < pParam->c_str()) {
            continue;
        }
        if (**it == pParam->c_str()) {
#ifdef TMS_NFC //THNDEV-1963 modify merge configs for Transit.conf
            const CNfcParam* updatedParam = merge(*it, pParam);
            m_list.insert(m_list.erase(it), updatedParam);
#else
            m_list.insert(m_list.erase(it), pParam);
#endif
        } else {
            m_list.insert(it, pParam);
        }

        return;
    }
    m_list.push_back(pParam);
}
/*******************************************************************************
**
** Function:    CNfcConfig::dump()
**
** Description: prints all elements in the list
**
** Returns:     none
**
*******************************************************************************/
void CNfcConfig::dump() {
    ALOGD("%s Enter", __func__);

    for (list<const CNfcParam *>::iterator it = m_list.begin(),
            itEnd = m_list.end();
            it != itEnd; ++it) {
        if ((*it)->strLen() > 0) {
            ALOGD("%s %s \t= %s", __func__, (*it)->c_str(), (*it)->str_value());
        } else
            ALOGD("%s %s \t= (0x%0lX)\n", __func__, (*it)->c_str(),
                  (*it)->numValue());
    }
}
/*******************************************************************************
**
** Function:    CNfcConfig::isAllowed()
**
** Description: checks if token update is allowed
**
** Returns:     true if allowed else false
**
*******************************************************************************/
bool CNfcConfig::isAllowed(const char *pName) {
    string token(pName);
    bool stat = false;
    if ((token.find("P2P_LISTEN_TECH_MASK") != std::string::npos) ||
            (token.find("HOST_LISTEN_TECH_MASK") != std::string::npos) ||
            (token.find("UICC_LISTEN_TECH_MASK") != std::string::npos) ||
            (token.find("POLLING_TECH_MASK") != std::string::npos) ||
            (token.find("TMS_RF_CONF_BLK") != std::string::npos))

    {
        stat = true;
    }
    return stat;
}
/*******************************************************************************
**
** Function:    CNfcConfig::moveFromList()
**
** Description: move the setting object from list to array
**
** Returns:     none
**
*******************************************************************************/
void CNfcConfig::moveFromList() {
    if (m_list.size() == 0) {
        return;
    }

    for (list<const CNfcParam *>::iterator it = m_list.begin(),
            itEnd = m_list.end();
            it != itEnd; ++it) {
        push_back(*it);
    }
    m_list.clear();
}

/*******************************************************************************
**
** Function:    CNfcConfig::moveToList()
**
** Description: move the setting object from array to list
**
** Returns:     none
**
*******************************************************************************/
void CNfcConfig::moveToList() {
    if (m_list.size() != 0) {
        m_list.clear();
    }

    for (iterator it = begin(), itEnd = end(); it != itEnd; ++it) {
        m_list.push_back(*it);
    }
    clear();
}
bool CNfcConfig::isModified(TmsConfFile aType) {
    FILE *fd = NULL;
    bool isModified = false;

    ALOGD("isModified enter; conf file type %d", aType);
    switch (aType) {
        case CONF_FILE_TMS:
            fd = fopen(gConfigTimestampPath, "r+");
            break;
        case CONF_FILE_TMS_RF:
            fd = fopen(gRfConfigTimestampPath, "r+");
            break;
        case CONF_FILE_TMS_TRANSIT:
            fd = fopen(gRrConfigTimestampPath, "r+");
            break;
        default:
            ALOGD("Invalid conf file type");
            return false;
    }
    if (fd == nullptr) {
        ALOGE("%s Unable to open file assume modified", __func__);
        return true;
    }

    uint32_t stored_crc32 = 0;
    if (fread(&stored_crc32, sizeof(uint32_t), 1, fd) != 1) {
        ALOGE("%s File read is not successfull errno = %d", __func__, errno);
    }

    fclose(fd);
    ALOGD("stored_crc32 is %d config_crc32_ is %d", stored_crc32, config_crc32_);

    switch (aType) {
        case CONF_FILE_TMS:
            isModified = stored_crc32 != config_crc32_;
            break;
        case CONF_FILE_TMS_RF:
            isModified = stored_crc32 != config_rf_crc32_;
            break;
        case CONF_FILE_TMS_TRANSIT:
            isModified = stored_crc32 != config_tr_crc32_;
            break;
    }
    return isModified;
}

void CNfcConfig::resetModified(TmsConfFile aType) {
    FILE *fd = NULL;

    ALOGD("resetModified enter; conf file type is %d", aType);
    switch (aType) {
        case CONF_FILE_TMS:
            fd = fopen(gConfigTimestampPath, "w+");
            break;
        case CONF_FILE_TMS_RF:
            fd = fopen(gRfConfigTimestampPath, "w+");
            break;
        case CONF_FILE_TMS_TRANSIT:
            fd = fopen(gRrConfigTimestampPath, "w+");
            break;
        default:
            ALOGD("Invalid conf file type");
            return;
    }

    if (fd == nullptr) {
        ALOGE("%s Unable to open file for writing", __func__);
        return;
    }

    switch (aType) {
        case CONF_FILE_TMS:
            fwrite(&config_crc32_, sizeof(uint32_t), 1, fd);
            break;
        case CONF_FILE_TMS_RF:
            fwrite(&config_rf_crc32_, sizeof(uint32_t), 1, fd);
            break;
        case CONF_FILE_TMS_TRANSIT:
            fwrite(&config_tr_crc32_, sizeof(uint32_t), 1, fd);
            break;
    }
    fclose(fd);
}

/*******************************************************************************
**
** Function:    CNfcParam::CNfcParam()
**
** Description: class constructor
**
** Returns:     none
**
*******************************************************************************/
CNfcParam::CNfcParam() : m_numValue(0) {}

/*******************************************************************************
**
** Function:    CNfcParam::~CNfcParam()
**
** Description: class destructor
**
** Returns:     none
**
*******************************************************************************/
CNfcParam::~CNfcParam() {}

/*******************************************************************************
**
** Function:    CNfcParam::CNfcParam()
**
** Description: class copy constructor
**
** Returns:     none
**
*******************************************************************************/
CNfcParam::CNfcParam(const char *pName, const string& value)
    : string(pName), m_str_value(value), m_numValue(0) {}

/*******************************************************************************
**
** Function:    CNfcParam::CNfcParam()
**
** Description: class copy constructor
**
** Returns:     none
**
*******************************************************************************/
CNfcParam::CNfcParam(const char *pName, unsigned long value)
    : string(pName), m_numValue(value) {}

/*******************************************************************************
**
** Function:    readOptionalConfig()
**
** Description: read Config settings from an pOptional conf file
**
** Returns:     none
**
*******************************************************************************/
void readOptionalConfig(const char *extra) {
    string strPath;
    string configName(EXTRA_CONFIG_BASE);
    configName += extra;
    configName += EXTRA_CONFIG_EXT;

    if (gAlternativeConfigPath[0] != '\0') {
        strPath.assign(gAlternativeConfigPath);
        strPath += configName;
    } else {
        findConfigFilePathFromTransportConfigPaths(configName, strPath);
    }

    CNfcConfig::getInstance().readConfig(strPath.c_str(), false);
}

/*******************************************************************************
**
** Function:    GetStrValue
**
** Description: API function for getting a string value of a setting
**
** Returns:     True if found, otherwise False.
**
*******************************************************************************/
extern "C" int getTmsStrValue(const char *pName, char *pValue,
                              unsigned long len) {
    CNfcConfig& rConfig = CNfcConfig::getInstance();

    return rConfig.getValue(pName, pValue, len);
}

/*******************************************************************************
**
** Function:    GetByteArrayValue()
**
** Description: Read byte array value from the config file.
**
** Parameters:
**              pName - pName of the config param to read.
**              pValue  - pointer to input pBuffer.
**              buffLen - input pBuffer length.
**              len - out parameter to return the number of bytes read from
**                    config file, return -1 in case buffLen is not enough.
**
** Returns:     TRUE[1] if config param pName is found in the config file, else
**              FALSE[0]
**
*******************************************************************************/
extern "C" int getTmsByteArrayValue(const char *pName, char *pValue,
                                    long buffLen, long *len) {
    CNfcConfig& rConfig = CNfcConfig::getInstance();

    return rConfig.getValue(pName, pValue, buffLen, len);
}

/*******************************************************************************
**
** Function:    GetNumValue
**
** Description: API function for getting a numerical value of a setting
**
** Returns:     true, if successful
**
*******************************************************************************/
extern "C" int getTmsNumValue(const char *pName, void *pValue,
                              unsigned long len) {
    if (!pValue) {
        return false;
    }

    CNfcConfig& rConfig = CNfcConfig::getInstance();
    const CNfcParam *pParam = rConfig.find(pName);

    if (pParam == NULL) {
        return false;
    }
    unsigned long v = pParam->numValue();
    if (v == 0 && pParam->strLen() > 0 && pParam->strLen() < 4) {
        const unsigned char *p = (const unsigned char *)pParam->str_value();
        for (size_t i = 0; i < pParam->strLen(); ++i) {
            v *= 256;
            v += *p++;
        }
    }
    switch (len) {
        case sizeof(unsigned long):
            *(static_cast<unsigned long *>(pValue)) = (unsigned long)v;
            break;
        case sizeof(unsigned short):
            *(static_cast<unsigned short *>(pValue)) = (unsigned short)v;
            break;
        case sizeof(unsigned char):
            *(static_cast<unsigned char *>(pValue)) = (unsigned char)v;
            break;
        default:
            return false;
    }
    return true;
}

/*******************************************************************************
**
** Function:    setTmsRfConfigPath
**
** Description: sets the path of the TMS RF config file
**
** Returns:     none
**
*******************************************************************************/
extern "C" void setTmsRfConfigPath(const char *pName) {
#ifdef TMS_NFC
    NfcHalAutoThreadMutex a(ghalConfLock);
#endif
    isRfConfigPathModified = true;
    memset(gTmsRfConfigPath, 0, sizeof(gTmsRfConfigPath));
    strlcpy(gTmsRfConfigPath, pName, sizeof(gTmsRfConfigPath));
    ALOGD("gTmsRfConfigPath=%s", gTmsRfConfigPath);
}

/*******************************************************************************
**
** Function:    resetConfig
**
** Description: reset settings array
**
** Returns:     none
**
*******************************************************************************/
extern "C" void resetTmsConfig()

{
    CNfcConfig& rConfig = CNfcConfig::getInstance();

    rConfig.clean();
}

/*******************************************************************************
**
** Function:    isTmsConfigModified()
**
** Description: check if config file has modified
**
** Returns:     0 if not modified, 1 otherwise.
**
*******************************************************************************/
extern "C" int isTmsConfigModified() {
    CNfcConfig& rConfig = CNfcConfig::getInstance();
    return rConfig.isModified(CONF_FILE_TMS);
}

/*******************************************************************************
**
** Function:    isTmsRFConfigModified()
**
** Description: check if config file has modified
**
** Returns:     0 if not modified, 1 otherwise.
**
*******************************************************************************/
extern "C" int isTmsRFConfigModified() {
    int retRF = 0, rettransit = 0, ret = 0;
    CNfcConfig& rConfig = CNfcConfig::getInstance();
    retRF = rConfig.isModified(CONF_FILE_TMS_RF);
    rettransit =
        rConfig.isModified(CONF_FILE_TMS_TRANSIT);
    ret = retRF | rettransit;
    ALOGD("ret RF or Transit value %d", ret);
    return ret;
}

/*******************************************************************************
**
** Function:    updateTmsConfigTimestamp()
**
** Description: update if config file has modified
**
** Returns:     0 if not modified, 1 otherwise.
**
*******************************************************************************/
extern "C" int updateTmsConfigTimestamp() {
    CNfcConfig& rConfig = CNfcConfig::getInstance();
    rConfig.resetModified(CONF_FILE_TMS);
    return 0;
}
/*******************************************************************************
**
** Function:    updateTmsConfigTimestamp()
**
** Description: update if config file has modified
**
** Returns:     0 if not modified, 1 otherwise.
**
*******************************************************************************/
extern "C" int updateTmsRfConfigTimestamp() {
    CNfcConfig& rConfig = CNfcConfig::getInstance();
    rConfig.resetModified(CONF_FILE_TMS_RF);
    rConfig.resetModified(CONF_FILE_TMS_TRANSIT);
    return 0;
}
