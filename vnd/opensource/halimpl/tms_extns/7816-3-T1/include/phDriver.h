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

#ifndef TMS_7816_3_T1_PH_DRIVER_H
#define TMS_7816_3_T1_PH_DRIVER_H

#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

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
int PhOpen(char *devName, int devNameLen, int oFlag);

/*******************************************************************************
 *
 * @Function         spiClose
 *
 * @Description      Close the physical, SPI or I2C, device driver.
 *
 * @Returns          void
 *
 ******************************************************************************/
void PhClose();

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
int PhRead(uint8_t *rxBuff, unsigned int bytesToRead);

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
int PhWrite(uint8_t *txBuff, unsigned int txBuffLen);

#endif // TMS_7816_3_T1_PH_DRIVER_H
