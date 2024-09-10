/*
 * Copyright (C) 2022 Tsingteng MicroSystem
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
#include <base/logging.h>
#include <stdio.h>

#define DUMP_PACKET(tag, pData, plen) { \
    uint16_t data_len = plen; \
    uint16_t lenPart = (data_len > 500) ? 500 : data_len; \
    uint16_t index = 0; \
    char printBuffer[lenPart * 2 + 1]; \
    do { \
        (void)memset(printBuffer, 0, sizeof(printBuffer)); \
        for (uint16_t i = 0; i < lenPart; i++) { \
            int ret = snprintf(&printBuffer[i * 2], 3, "%02X", pData[index++]); \
            if (ret < 0) { \
                DLOG_IF(INFO, nfc_debug_enabled) \
                    << StringPrintf("%s %s: snprintf failed, ret = %d",__func__, tag, ret); \
            } \
        } \
        DLOG_IF(INFO, nfc_debug_enabled) \
            << StringPrintf("%s %s: len = %3d, %s",__func__, tag, lenPart, printBuffer); \
        data_len = data_len - lenPart; \
        lenPart = (data_len > 500) ? 500 : data_len; \
    } while (data_len > 0); \
}