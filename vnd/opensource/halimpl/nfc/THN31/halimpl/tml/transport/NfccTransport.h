/******************************************************************************
 *
 *  Copyright 2020-2021 NXP
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

#pragma once
#include <NfcTypes.h>
#include <TmlNfc.h>

enum NfccResetType : uint32_t {
    MODE_POWER_OFF = 0x00,
    MODE_POWER_ON,
    MODE_FW_DWNLD_WITH_VEN,
    MODE_ISO_RST,
    MODE_FW_DWND_HIGH,
    MODE_POWER_RESET,
    MODE_FW_GPIO_LOW
};

enum EseResetCallSrc : uint32_t {
    SRC_SPI = 0x0,
    SRC_NFC = 0x10,
};

enum EseResetType : uint32_t {
    MODE_ESE_POWER_ON = 0,
    MODE_ESE_POWER_OFF,
    MODE_ESE_POWER_STATE,
    /*Request from eSE HAL/Service*/
    MODE_ESE_COLD_RESET,
    MODE_ESE_RESET_PROTECTION_ENABLE,
    MODE_ESE_RESET_PROTECTION_DISABLE,
    /*Request from NFC HAL/Service*/
    MODE_ESE_COLD_RESET_NFC = MODE_ESE_COLD_RESET | SRC_NFC,
    MODE_ESE_RESET_PROTECTION_ENABLE_NFC = MODE_ESE_RESET_PROTECTION_ENABLE | SRC_NFC,
    MODE_ESE_RESET_PROTECTION_DISABLE_NFC = MODE_ESE_RESET_PROTECTION_DISABLE | SRC_NFC,
};

class NfccTransport {
  public:
    /*****************************************************************************
     **
     ** Function         i2cClose
     **
     ** Description      Closes NFCC device
     **
     ** Parameters       pDevHandle - device handle
     **
     ** Returns          None
     **
     *****************************************************************************/
    virtual void i2cClose(void *pDevHandle) = 0;

    /*****************************************************************************
     **
     ** Function         i2cOpenAndConfigure
     **
     ** Description      Open and configure NFCC device and transport layer
     **
     ** Parameters       pConfig     - hardware information
     **                  pLinkHandle - device handle
     **
     ** Returns          NFC status:
     **                  NFCSTATUS_SUCCESS - open_and_configure operation success
     **                  NFCSTATUS_INVALID_DEVICE - device open operation failure
     **
     ****************************************************************************/
    virtual NFCSTATUS i2cOpenAndConfigure(pTmlNfcConfig_t pConfig,
                                       void **pLinkHandle) = 0;

    /*****************************************************************************
     **
     ** Function         i2cRead
     **
     ** Description      Reads requested number of bytes from NFCC device into
     **                 given pBuffer
     **
     ** Parameters       pDevHandle       - valid device handle
     **                  pBuffer          - pBuffer for read data
     **                  nNbBytesToRead   - number of bytes requested to be read
     **
     ** Returns          numRead   - number of successfully read bytes
     **                  -1        - read operation failure
     **
     ****************************************************************************/
    virtual int i2cRead(void *pDevHandle, uint8_t *pBuffer, int nNbBytesToRead) = 0;

    /*****************************************************************************
     **
     ** Function         i2cWrite
     **
     ** Description      Writes requested number of bytes from given pBuffer into
     **                  NFCC device
     **
     ** Parameters       pDevHandle       - valid device handle
     **                  pBuffer          - pBuffer for read data
     **                  nNbBytesToWrite  - number of bytes requested to be
     *written
     **
     ** Returns          numWrote   - number of successfully written bytes
     **                  -1         - write operation failure
     **
     *****************************************************************************/
    virtual int i2cWrite(void *pDevHandle, uint8_t *pBuffer,
                      int nNbBytesToWrite) = 0;

    /*****************************************************************************
     **
     ** Function         nfccReset
     **
     ** Description      Reset NFCC device, using VEN pin
     **
     ** Parameters       pDevHandle     - valid device handle
     **                  type          - NfccResetType
     **
     ** Returns           0   - reset operation success
     **                  -1   - reset operation failure
     **
     ****************************************************************************/
    virtual int nfccReset(void *pDevHandle, NfccResetType type);

    /*****************************************************************************
     **
     ** Function         eseReset
     **
     ** Description      Request NFCC to reset the eSE
     **
     ** Parameters       pDevHandle     - valid device handle
     **                  type          - EseResetType
     **
     ** Returns           0   - reset operation success
     **                  else - reset operation failure
     **
     ****************************************************************************/
    virtual int eseReset(void *pDevHandle, EseResetType type);

    /*****************************************************************************
     **
     ** Function         eseGetPower
     **
     ** Description      Request NFCC to reset the eSE
     **
     ** Parameters       pDevHandle     - valid device handle
     **                  level          - reset level
     **
     ** Returns           0   - reset operation success
     **                  else - reset operation failure
     **
     ****************************************************************************/
    virtual int eseGetPower(void *pDevHandle, uint32_t level);

    /*******************************************************************************
    **
    ** Function         flushdata
    **
    ** Description      Reads payload of FW rsp from NFCC device into given pBuffer
    **
    ** Parameters       pConfig     - hardware information
    **
    ** Returns          True(Success)/False(Fail)
    **
    *******************************************************************************/
    virtual bool flushdata(pTmlNfcConfig_t pConfig);

    /*****************************************************************************
     **
     ** Function         ~NfccTransport
     **
     ** Description      TransportLayer destructor
     **
     ** Parameters       none
     **
     ** Returns          None
     ****************************************************************************/
    virtual ~NfccTransport() {};
};
