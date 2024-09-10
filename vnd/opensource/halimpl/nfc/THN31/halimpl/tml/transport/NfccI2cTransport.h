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
#include <NfccTransport.h>

#define NFC_MAGIC 0xE9
/*
 * NFCC power control via ioctl
 * NFC_SET_PWR(0): power off
 * NFC_SET_PWR(1): power on
 * NFC_SET_PWR(2): reset and power on with firmware download enabled
 */
#define NFC_SET_PWR _IOW(NFC_MAGIC, 0x01, long)
/*
 * 1. SPI Request NFCC to enable ESE power, only in param
 *   Only for SPI
 *   level 1 = Enable power
 *   level 0 = Disable power
 * 2. NFC Request the eSE cold reset, only with MODE_ESE_COLD_RESET
 */
#define ESE_SET_PWR _IOW(NFC_MAGIC, 0x02, long)

/*
 * SPI or DWP can call this ioctl to get the current
 * power state of ESE
 */
#define ESE_GET_PWR _IOR(NFC_MAGIC, 0x03, long)

class NfccI2cTransport : public NfccTransport {
  private:
    sem_t mTxRxSemaphore;


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
    void i2cClose(void *pDevHandle);

    /*****************************************************************************
     **
     ** Function         i2cOpenAndConfigure
     **
     ** Description      Open and configure NFCC device
     **
     ** Parameters       pConfig     - hardware information
     **                  pLinkHandle - device handle
     **
     ** Returns          NFC status:
     **                  NFCSTATUS_SUCCESS - open_and_configure operation success
     **                  NFCSTATUS_INVALID_DEVICE - device open operation failure
     **
     ****************************************************************************/
    NFCSTATUS i2cOpenAndConfigure(pTmlNfcConfig_t pConfig, void **pLinkHandle);

    /*****************************************************************************
     **
     ** Function         i2cRead
     **
     ** Description      Reads requested number of bytes from NFCC device into
     *given
     **                  pBuffer
     **
     ** Parameters       pDevHandle       - valid device handle
     **                  pBuffer          - pBuffer for read data
     **                  nNbBytesToRead   - number of bytes requested to be read
     **
     ** Returns          numRead   - number of successfully read bytes
     **                  -1        - read operation failure
     **
     ****************************************************************************/
    int i2cRead(void *pDevHandle, uint8_t *pBuffer, int nNbBytesToRead);

    /*****************************************************************************
    **
    ** Function         i2cWrite
    **
    ** Description      Writes requested number of bytes from given pBuffer into
    **                  NFCC device
    **
    ** Parameters       pDevHandle       - valid device handle
    **                  pBuffer          - pBuffer for read data
    **                  nNbBytesToWrite  - number of bytes requested to be written
    **
    ** Returns          numWrote   - number of successfully written bytes
    **                  -1         - write operation failure
    **
    *****************************************************************************/
    int i2cWrite(void *pDevHandle, uint8_t *pBuffer, int nNbBytesToWrite);

    /*****************************************************************************
     **
     ** Function         nfccReset
     **
     ** Description      Reset NFCC device, using VEN pin
     **
     ** Parameters       pDevHandle     - valid device handle
     **                  level          - reset level
     **
     ** Returns           0   - reset operation success
     **                  -1   - reset operation failure
     **
     ****************************************************************************/
    int nfccReset(void *pDevHandle, NfccResetType type);

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
    int eseReset(void *pDevHandle, EseResetType type);

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
    int eseGetPower(void *pDevHandle, uint32_t level);

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
    bool flushdata(pTmlNfcConfig_t pConfig);
};
