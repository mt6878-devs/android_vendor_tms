/******************************************************************************
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

/*
 * DAL I2C port implementation for linux
 *
 * Project: Trusted NFC Linux
 *
 */
#include <errno.h>
#include <fcntl.h>
#include <hardware/nfc.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <NfccI2cTransport.h>
#include <NfcStatus.h>
#include <TmsLog.h>
#include <string.h>
#include "TmsNciHal_utils.h"

#define CRC_LEN 2
#define NORMAL_MODE_HEADER_LEN 3
#define NORMAL_MODE_LEN_OFFSET 2
#define FLUSH_BUFFER_SIZE 0xFF
TmlNfci2cFragmentation_t gFragmentationEnabled = I2C_FRAGMENATATION_DISABLED;

TmlNfci2cFragmentation_t *getFragmentationEnabled(void)
{
    return &gFragmentationEnabled;
}

/*******************************************************************************
**
** Function         i2cClose
**
** Description      Closes NFCC device
**
** Parameters       pDevHandle - device handle
**
** Returns          None
**
*******************************************************************************/
void NfccI2cTransport::i2cClose(void *pDevHandle) {
    if (NULL != pDevHandle) {
        close((int)(intptr_t)pDevHandle);
    }
    sem_destroy(&mTxRxSemaphore);
    return;
}

/*******************************************************************************
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
*******************************************************************************/
NFCSTATUS NfccI2cTransport::i2cOpenAndConfigure(pTmlNfcConfig_t pConfig,
        void **pLinkHandle) {
    int nHandle;
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    TMSLOG_TML_D("%s Opening port=%s\n", __func__, pConfig->pDevName);
    /* open port */
    nHandle = open((const char *)pConfig->pDevName, O_RDWR);
    if (nHandle < 0) {
        TMSLOG_TML_E("_i2c_open() Failed: retval %x", nHandle);
        *pLinkHandle = NULL;
        status = NFCSTATUS_INVALID_DEVICE;
    } else {
        *pLinkHandle = (void *)((intptr_t)nHandle);
        if (0 != sem_init(&mTxRxSemaphore, 0, 1)) {
            TMSLOG_TML_E("%s Failed: reason sem_init : retval %x", __func__, nHandle);
            status = NFCSTATUS_FAILED;
        }
    }
    return status;
}

/*******************************************************************************
**
** Function         flushdata
**
** Description      Reads payload of FW rsp from NFCC device into given buffer
**
** Parameters       pConfig     - hardware information
**
** Returns          True(Success)/False(Fail)
**
*******************************************************************************/
bool NfccI2cTransport::flushdata(pTmlNfcConfig_t pConfig) {
    int retRead = 0;
    int nHandle;
    uint8_t buffer[FLUSH_BUFFER_SIZE];
    TMSLOG_TML_D("%s: Enter", __func__);
    nHandle = open((const char *)pConfig->pDevName, O_RDWR | O_NONBLOCK);
    if (nHandle < 0) {
        TMSLOG_TML_E("%s: _i2c_open() Failed: retval %x", __func__, nHandle);
        return false;
    }
    do {
        retRead = read(nHandle, buffer, sizeof(buffer));
        if (retRead > 0) {
            tmsNciHalPrintPacket("RECV", buffer, retRead);
            usleep(2 * 1000);
        }
    } while (retRead > 0);
    close(nHandle);
    TMSLOG_TML_D("%s: Exit", __func__);
    return true;
}

/*******************************************************************************
**
** Function         i2cRead
**
** Description      Reads requested number of bytes from NFCC device into given
**                  pBuffer
**
** Parameters       pDevHandle       - valid device handle
**                  pBuffer          - pBuffer for read data
**                  nNbBytesToRead   - number of bytes requested to be read
**
** Returns          numRead   - number of successfully read bytes
**                  -1        - read operation failure
**
*******************************************************************************/
int NfccI2cTransport::i2cRead(void *pDevHandle, uint8_t *pBuffer,
                           int nNbBytesToRead) {
    int retRead;
    int retSelect;
    int numRead = 0;
    struct timeval tv;
    fd_set rfds;
    uint16_t totalBtyesToRead = 0;

    UNUSED_PROP(nNbBytesToRead);
    if (NULL == pDevHandle) {
        return -1;
    }

    totalBtyesToRead = NORMAL_MODE_HEADER_LEN;

    /* Read with 2 second timeOut, so that the read thread can be aborted
       when the NFCC does not respond and we need to switch to FW download
       mode. This should be done via a control socket instead. */
    FD_ZERO(&rfds);
    FD_SET((int)(intptr_t)pDevHandle, &rfds);
    tv.tv_sec = 2;
    tv.tv_usec = 1;

    retSelect =
        select((int)((intptr_t)pDevHandle + (int)1), &rfds, NULL, NULL, &tv);
    if (retSelect < 0) {
        TMSLOG_TML_D("%s errno : %x", __func__, errno);
        return -1;
    } else if (retSelect == 0) {
        TMSLOG_TML_D("%s Timeout", __func__);
        return -1;
    } else {
        retRead = read((int)(intptr_t)pDevHandle, pBuffer, totalBtyesToRead - numRead);
        if (retRead > 0 && !(pBuffer[0] == 0xFF && pBuffer[1] == 0xFF)) {
            numRead += retRead;
        } else if (retRead == 0) {
            TMSLOG_TML_E("%s [hdr]EOF", __func__);
            return -1;
        } else {
            TMSLOG_TML_E("%s [hdr] errno : %x", __func__, errno);
            if (retRead < 0) {
                TMSLOG_TML_E("%s read() failed, pBuffer[0] and pBuffer[1] are not reliable", __func__);
            } else {
                TMSLOG_TML_E(" %s pBuffer[0] = %x pBuffer[1]= %x", __func__, pBuffer[0], pBuffer[1]);
            }
            return -1;
        }

        totalBtyesToRead = NORMAL_MODE_HEADER_LEN;

        if (numRead < totalBtyesToRead) {
            retRead = read((int)(intptr_t)pDevHandle, (pBuffer + numRead), totalBtyesToRead - numRead);

            if (retRead != totalBtyesToRead - numRead) {
                TMSLOG_TML_E("%s [hdr] errno : %x", __func__, errno);
                return -1;
            } else {
                numRead += retRead;
            }
        }

        totalBtyesToRead = pBuffer[NORMAL_MODE_LEN_OFFSET] + NORMAL_MODE_HEADER_LEN;

        if ((totalBtyesToRead - numRead) != 0) {
            retRead = read((int)(intptr_t)pDevHandle, (pBuffer + numRead), totalBtyesToRead - numRead);
            if (retRead > 0) {
                numRead += retRead;
            } else if (retRead == 0) {
                TMSLOG_TML_E("%s [pyld] EOF", __func__);
                return -1;
            } else {
                TMSLOG_TML_D("_i2c_read() [hdr] received");
                tmsNciHalPrintPacket("RECV", pBuffer, NORMAL_MODE_HEADER_LEN);
                TMSLOG_TML_E("%s [pyld] errno : %x", __func__, errno);
                return -1;
            }
        } else {
            TMSLOG_TML_E("%s _>>>>> Empty packet recieved !!", __func__);
        }
    }
    return numRead;
}

/*******************************************************************************
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
*******************************************************************************/
int NfccI2cTransport::i2cWrite(void *pDevHandle, uint8_t *pBuffer,
                            int nNbBytesToWrite) {
    int ret;
    int numWrote = 0;
    int numBytes = nNbBytesToWrite;
    if (NULL == pDevHandle) {
        return -1;
    }
    if (*getFragmentationEnabled() == I2C_FRAGMENATATION_DISABLED &&
            nNbBytesToWrite > (*getTmlNfcContext())->fragmentLen) {
        TMSLOG_TML_D(
            "%s data larger than maximum I2C  size,enable I2C fragmentation",
            __func__);
        return -1;
    }
    while (numWrote < nNbBytesToWrite) {
        if (*getFragmentationEnabled() == I2C_FRAGMENTATION_ENABLED &&
                nNbBytesToWrite > (*getTmlNfcContext())->fragmentLen) {
            if (nNbBytesToWrite - numWrote > (*getTmlNfcContext())->fragmentLen) {
                numBytes = numWrote + (*getTmlNfcContext())->fragmentLen;
            } else {
                numBytes = nNbBytesToWrite;
            }
        }
        ret = write((int)(intptr_t)pDevHandle, pBuffer + numWrote, numBytes - numWrote);
        if (ret > 0) {
            numWrote += ret;
            if (*getFragmentationEnabled() == I2C_FRAGMENTATION_ENABLED &&
                    numWrote < nNbBytesToWrite) {
                usleep(500);
            }
        } else if (ret == 0) {
            TMSLOG_TML_D("%s EOF", __func__);
            return -1;
        } else {
            TMSLOG_TML_D("%s errno : %x", __func__, errno);
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            return -1;
        }
    }

    return numWrote;
}

/*******************************************************************************
**
** Function         nfccReset
**
** Description      Reset NFCC device, using VEN pin
**
** Parameters       pDevHandle     - valid device handle
**                  type          - reset level
**
** Returns           0   - reset operation success
**                  -1   - reset operation failure
**
*******************************************************************************/
int NfccI2cTransport::nfccReset(void *pDevHandle, NfccResetType type) {
    int ret = -1;
    TMSLOG_TML_D("%s, VEN type %u", __func__, type);

    if (NULL == pDevHandle) {
        return -1;
    }

    ret = ioctl((int)(intptr_t)pDevHandle, NFC_SET_PWR, type);
    if (ret < 0) {
        TMSLOG_TML_E("%s :failed errno = 0x%x", __func__, errno);
    }

    return ret;
}

/*******************************************************************************
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
*******************************************************************************/
int NfccI2cTransport::eseReset(void *pDevHandle, EseResetType type) {
    int ret = -1;
    TMSLOG_TML_D("%s, type %u", __func__, type);

    if (NULL == pDevHandle) {
        return -1;
    }
    ret = ioctl((int)(intptr_t)pDevHandle, ESE_SET_PWR, type);
    if (ret < 0) {
        TMSLOG_TML_E("%s :failed errno = 0x%x", __func__, errno);
    }
    return ret;
}

/*******************************************************************************
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
*******************************************************************************/
int NfccI2cTransport::eseGetPower(void *pDevHandle, uint32_t level) {
    return ioctl((int)(intptr_t)pDevHandle, ESE_GET_PWR, level);
}
