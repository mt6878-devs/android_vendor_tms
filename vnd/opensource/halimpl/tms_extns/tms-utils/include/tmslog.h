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

#ifndef TMSLOG_H
#define TMSLOG_H

#ifndef UNUSED
    #define UNUSED(arg) (void)(arg)
#endif

#include <log/log.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG_LEVEL_NONE 0x00
#define DEBUG_LEVEL_E 0x01
#define DEBUG_LEVEL_W 0x02
#define DEBUG_LEVEL_D 0x03

unsigned char GetHalDebugLevel(void);

#define TMS_LOG_V(tag, ...)                           \
{                                                     \
    if (GetHalDebugLevel() >= DEBUG_LEVEL_D)          \
        LOG_PRI(ANDROID_LOG_DEBUG, tag, __VA_ARGS__); \
}

#define TMS_LOG_D(tag, ...)                           \
{                                                     \
    if (GetHalDebugLevel() >= DEBUG_LEVEL_D)          \
        LOG_PRI(ANDROID_LOG_DEBUG, tag, __VA_ARGS__); \
}

#define TMS_LOG_I(tag, ...)                          \
{                                                    \
    if (GetHalDebugLevel() >= DEBUG_LEVEL_W)         \
        LOG_PRI(ANDROID_LOG_INFO, tag, __VA_ARGS__); \
}

#define TMS_LOG_W(tag, ...)                          \
{                                                    \
    if (GetHalDebugLevel() >= DEBUG_LEVEL_W)         \
        LOG_PRI(ANDROID_LOG_WARN, tag, __VA_ARGS__); \
}

#define TMS_LOG_E(tag, ...)                           \
{                                                     \
    if (GetHalDebugLevel() >= DEBUG_LEVEL_E)          \
        LOG_PRI(ANDROID_LOG_ERROR, tag, __VA_ARGS__); \
}

/*******************************************************************************
**
** Function:        InitSELogLevel
**
** Description:     Initialize and get global logging level from libese-tms.conf.
**                  Global log level:
**                  DEBUG_LEVEL_NONE    0, No log be printed
**                  DEBUG_LEVEL_ERROR   1, only error log be printed
**                  DEBUG_LEVEL_WARNING 2, only Warning and error log be printed
**                  DEBUG_LEVEL_DEBUG   3, all log be printed
**
*******************************************************************************/
void InitSELogLevel();

/*******************************************************************************
**
** Function         printHexPacket
**
** Description      Print packet
**
** Parameters       tag1, file g_tag name
**                  tag2, [TmsNciX, TmsNciR, TmsEseDataX, TmsEseDataR]
**                  tag2Len, tag2 length
**                  pData, will be printed buffer
**                  len, pData buffer length
** Returns          None
**
*******************************************************************************/
void printHexPacket(const char *tag1, uint8_t tag1Len,
                    const char *tag2, uint8_t tag2Len,
                    const uint8_t *pData, uint16_t len);

#ifdef __cplusplus
};
#endif

#endif // TMSLOG_H
