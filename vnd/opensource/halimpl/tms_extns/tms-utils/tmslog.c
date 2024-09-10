/*
 * Copyright (c) Tsingteng MicroSystem Co., Ltd. 2021. All rights reserved.
 */
/*
 * Copyright (c) 2021 Tsingteng MicroSystem
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
 */

#if defined (USE_TMS_NFC) || defined (USE_C1)
    #include "tmsSecureString.h"
#else
    #include "securec.h"
#endif

#include <string.h>
#include <stdio.h>
#include "tmslog.h"
#include "configC.h"
#include "tmsCommon.h"

#ifdef TAG
#undef TAG
#endif
#define TAG g_tag

static const char g_tag[] = "TmsLog";

unsigned char g_halDebugLevel = DEBUG_LEVEL_D;

static bool sInitLogLevel = false;

unsigned char GetHalDebugLevel(void)
{
    return g_halDebugLevel;
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
void InitSELogLevel()
{
    unsigned char num = 0;
    if (sInitLogLevel) {
        return;
    }
    sInitLogLevel = true;

    num = (unsigned char)ConfigGetUnsigned(NAME_TMS_SE_HAL_LOGLEVEL,
                                           strlen(NAME_TMS_SE_HAL_LOGLEVEL), 0);
    if (num >= 0) {
        g_halDebugLevel = num;
    }

    TMS_LOG_D(g_tag, "%s: g_halDebugLevel = %u", __FUNCTION__, g_halDebugLevel);
}

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
                    const uint8_t *pData, uint16_t len)
{
    uint16_t lenPart = (len > 500) ? 500 : len;
    uint16_t index = 0;
    char printBuffer[lenPart * 2 + 1];  // lenPart * 2 + 1

    UNUSED(tag1Len);
    do {
        (void)memset_s(printBuffer, sizeof(printBuffer), 0, sizeof(printBuffer));
        for (uint16_t i = 0; i < lenPart; i++) {
            unsigned int maxLen = sizeof(printBuffer) - (i * 2);
            // printBuffer[i * 2], maxLen, 3
            int ret = snprintf_s(&printBuffer[i * 2], maxLen, 3, "%02X", pData[index++]);
            if (ret < 0) {
                TMS_LOG_D(tag1, "%s snprintf_s failed, ret = %d", tag2, ret);
            }
        }
        if (NULL == tag2) {
            TMS_LOG_D(tag1, "%s len = %3d, %s", tag2, lenPart, printBuffer);
        // tag2[tag2Len - 2] && tag2[tag2Len - 1]
        } else if (('T' == tag2[tag2Len - 2]) && ('x' == tag2[tag2Len - 1])) {
            TMS_LOG_D(tag1, "%s len = %3d > %s", tag2, lenPart, printBuffer);
        // tag2[tag2Len - 2] && tag2[tag2Len - 1]
        } else if (('R' == tag2[tag2Len - 2]) && ('x' == tag2[tag2Len - 1])) {
            TMS_LOG_D(tag1, "%s len = %3d < %s", tag2, lenPart, printBuffer);
        } else {
            TMS_LOG_D(tag1, "%s len = %3d, %s", tag2, lenPart, printBuffer);
        }

        len = len - lenPart;
        lenPart = (len > 500) ? 500 : len;  // (len > 500) ? 500 : len
    } while (len > 0);
}
