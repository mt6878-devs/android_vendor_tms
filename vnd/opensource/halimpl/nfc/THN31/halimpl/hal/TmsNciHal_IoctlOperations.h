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
#include "NfcStatus.h"
#include "TmsConfig.h"
#include "TmsLog.h"
#include <hardware/nfc.h>

typedef std::map<std::string, std::string> systemProperty;

/******************************************************************************
 ** Function         tmsNciHalIoctlIf
 **
 ** Description      This function shall be called from HAL when libnfc-nci
 **                  calls tmsNciHalIoctl() to perform any IOCTL operation
 **
 ** Returns          return 0 on success and -1 on fail,
 ******************************************************************************/
int tmsNciHalIoctlIf(long arg, void *pData);

/*******************************************************************************
**
** Function         tmsNciHalGetSystemProperty
**
** Description      It shall be used to get property value of the given Key
**
** Parameters       string key
**
** Returns          It returns the property value of the key
*******************************************************************************/
string tmsNciHalGetSystemProperty(string key);

/*******************************************************************************
 **
 ** Function         tmsNciHalSetSystemProperty
 **
 ** Description      It shall be used to save/chage value to system property
 **                  based on provided key.
 **
 ** Parameters       string key, string value
 **
 ** Returns          true if success, false if fail
 *******************************************************************************/
bool tmsNciHalSetSystemProperty(string key, string value);

/*******************************************************************************
**
** Function         tmsNciHalGetTmsConfigIf
**
** Description      It shall be used to read config values from the
*libnfc-tms.conf
**
** Parameters       tmsConfigs config
**
** Returns          void
*******************************************************************************/
string tmsNciHalGetTmsConfigIf();

/*******************************************************************************
**
** Function         tmsNciHalResetEse
**
** Description      It shall be used to to reset eSE by proprietary command.
**
** Parameters       None
**
** Returns          status of eSE reset response
*******************************************************************************/
NFCSTATUS tmsNciHalResetEse(uint64_t resetType);

/******************************************************************************
** Function         tmsNciHalSetTmsTransitConfig
**
** Description      This function overwrite libnfc-tmsTransit.conf file
**                  with pTransitConfValue.
**
** Returns          bool.
**
*******************************************************************************/
bool tmsNciHalSetTmsTransitConfig(char *pTransitConfValue);

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
                      const char *pDefaultStr);

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
int propertySetIntf(const char *pPropName, const char *pValueStr);

/*******************************************************************************
 **
 ** Function:        tmsNciHalAbort()
 **
 ** Description:     This function shall be used to trigger the abort
 **
 ** Parameters       None
 **
 ** Returns:        returns 0 on success, < 0 on failure
 **
 ********************************************************************************/
bool tmsNciHalAbort();

#define PROP_VALUE_MAX 92
#define propertyGet(a, b, c) propertyGetIntf(a, b, c)
#define propertySet(a, b) propertySetIntf(a, b)
