/*
 * Copyright (c) Tsingteng MicroSystem Co., Ltd. 2021. All rights reserved.
 */
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "phDriver.h"
#include "tmslog.h"
#include "common.h"
#include "SEApi.h"

static const char g_tag[] = "phDriver";

static int g_gsDeviceFd;


/*******************************************************************************
 *
 * @Function         PhOpen
 *
 * @Description      Open the physical, SPI or I2C, device driver.
 *
 * @Parameters       devName - SPI or I2C device node name, such as, /dev/thn31spi.
 *
 * @Returns          the file descriptor if everything is ok, -1 otherwise.
 *
 ******************************************************************************/
int PhOpen(char *devName, int devNameLen, int oFlag)
{
    if (devNameLen <= 0) {
        TMS_LOG_E(g_tag, "%s: devNameLen[%d]", __FUNCTION__, devNameLen);
        return -1;
    }
    g_gsDeviceFd = open(devName, oFlag);
    TMS_LOG_D(g_tag, " spiDeviceId: %d", g_gsDeviceFd);
    TMS_LOG_D(g_tag, " PH_WRITE_TIMEOUT: %d", GetEseCtx()->maxWriteRetryCnt);
    if (g_gsDeviceFd < 0) {
        return -1;
    }

    return g_gsDeviceFd;
}

/*******************************************************************************
 *
 * @Function         spiClose
 *
 * @Description      Close the physical, SPI or I2C, device driver.
 *
 * @Returns          void
 *
 ******************************************************************************/
void PhClose()
{
    if (g_gsDeviceFd > 0) {
        close(g_gsDeviceFd);
    }
    g_gsDeviceFd = -1;
}

/*******************************************************************************
 *
 * @Function         PhRead
 *
 * @Description      Reads bytesToRead bytes from the physical interface, SPI or I2C.
 *
 * @Parameters       rxBuff    - Buffer to store recieved datas.
 *                   bytesToRead - Expected number of bytes to be read.
 *
 * @Returns          The amount of bytes read from the slave, -1 if something failed.
 *
*******************************************************************************/
int PhRead(uint8_t *rxBuff, unsigned int bytesToRead)
{
    int count = -1;
    count = (int)read(g_gsDeviceFd, rxBuff, bytesToRead);
    return count;
}

/*******************************************************************************
 *
 * @Function         PhWrite
 *
 * @Description      Write txBufferLength bytes to the physical interface, SPI or I2C.
 *
 * @Parameters       txBuff       - Buffer to transmit.
 *                   txBuffLen - Number of bytes to be written.
 *
 * @Returns          The amount of bytes written to the slave, -1 if something failed.
 *
*******************************************************************************/
int PhWrite(uint8_t *txBuff, unsigned int txBuffLen)
{
    int ret = -1;
    int numWrote = 0;
    unsigned int retryCount = 0;

    while ((unsigned int)numWrote < txBuffLen) {
        ret = write(g_gsDeviceFd, txBuff, txBuffLen);
        if (ret > 0) {
            numWrote += ret;
        } else if (ret == 0) {
            TMS_LOG_E(g_tag, "write() EOF");
            return -1;
        } else {
            TMS_LOG_E(g_tag, "write() errno : %d", errno);
            if ((errno == EIO || errno == EINTR || errno == EAGAIN) &&
                    (retryCount < GetEseCtx()->maxWriteRetryCnt)) {
                retryCount++;

                /* 1000us delay to give ESE wake up delay */
                usleep(1000);
                TMS_LOG_E(g_tag, "write() failed. Going to retry, counter:%u !", retryCount);
                continue;
            }

            return -1;
        }
    }

    return numWrote;
}
