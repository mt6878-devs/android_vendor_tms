/*
 * Copyright (C) 2010-2019 NXP Semiconductors
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

#define LOG_TAG "TmsNfcHal"
#include <stdio.h>
#include <string.h>
#include "TmsNciHal_IoctlOperations.h"
#include <log/log.h>

/* global log level structure */
NciLogLevel_t gLogLevel;

NciLogLevel_t *getLogLevel(void)
{
    return &gLogLevel;
}

/*******************************************************************************
 *
 * Function         tmsLogSetGlobalLogLevel
 *
 * Description      Sets the global log level for all modules.
 *                  This value is set by Android property
 *nfc.tmsLog_level_global.
 *                  If value can be overridden by module log level.
 *
 * Returns          The value of global log level
 *
 ******************************************************************************/
static uint8_t tmsLogSetGlobalLogLevel(void) {
    uint8_t level = TMSLOG_DEFAULT_LOGLEVEL;
    unsigned long num = 0;
    char valueStr[PROP_VALUE_MAX] = {0};

    int len = propertyGet(PROP_NAME_TMSLOG_GLOBAL_LOGLEVEL, valueStr, "");
    if (len > 0) {
        /* let Android property override .conf variable */
        sscanf(valueStr, "%lu", &num);
        level = (unsigned char)num;
    }
    memset(&(*getLogLevel()), level, sizeof(NciLogLevel_t));
    return level;
}

/*******************************************************************************
 *
 * Function         tmsLogSetHALLogLevel
 *
 * Description      Sets the HAL layer log level.
 *
 * Returns          void
 *
 ******************************************************************************/
static void tmsLogSetHALLogLevel(uint8_t level) {
    unsigned long num = 0;
    int len;
    char valueStr[PROP_VALUE_MAX] = {0};

    if (getTmsNumValue(NAME_TMSLOG_NCIHAL_LOGLEVEL, &num, sizeof(num))) {
        (*getLogLevel()).halLogLevel =
            (level > (unsigned char)num) ? level : (unsigned char)num;
        ;
    }

    len = propertyGet(PROP_NAME_TMSLOG_NCIHAL_LOGLEVEL, valueStr, "");
    if (len > 0) {
        /* let Android property override .conf variable */
        sscanf(valueStr, "%lu", &num);
        (*getLogLevel()).halLogLevel = (unsigned char)num;
    }
}

/*******************************************************************************
 *
 * Function         tmsLogSetExtnsLogLevel
 *
 * Description      Sets the Extensions layer log level.
 *
 * Returns          void
 *
 ******************************************************************************/
static void tmsLogSetExtnsLogLevel(uint8_t level) {
    unsigned long num = 0;
    int len;
    char valueStr[PROP_VALUE_MAX] = {0};
    if (getTmsNumValue(NAME_TMSLOG_EXTNS_LOGLEVEL, &num, sizeof(num))) {
        (*getLogLevel()).extnsLogLevel =
            (level > (unsigned char)num) ? level : (unsigned char)num;
        ;
    }

    len = propertyGet(PROP_NAME_TMSLOG_EXTNS_LOGLEVEL, valueStr, "");
    if (len > 0) {
        /* let Android property override .conf variable */
        sscanf(valueStr, "%lu", &num);
        (*getLogLevel()).extnsLogLevel = (unsigned char)num;
    }
}

/*******************************************************************************
 *
 * Function         tmsLogSetTmlLogLevel
 *
 * Description      Sets the Tml layer log level.
 *
 * Returns          void
 *
 ******************************************************************************/
static void tmsLogSetTmlLogLevel(uint8_t level) {
    unsigned long num = 0;
    int len;
    char valueStr[PROP_VALUE_MAX] = {0};
    if (getTmsNumValue(NAME_TMSLOG_TML_LOGLEVEL, &num, sizeof(num))) {
        (*getLogLevel()).tmlLogLevel =
            (level > (unsigned char)num) ? level : (unsigned char)num;
        ;
    }

    len = propertyGet(PROP_NAME_TMSLOG_TML_LOGLEVEL, valueStr, "");
    if (len > 0) {
        /* let Android property override .conf variable */
        sscanf(valueStr, "%lu", &num);
        (*getLogLevel()).tmlLogLevel = (unsigned char)num;
    }
}

/*******************************************************************************
 *
 * Function         tmsLogSetNciTxLogLevel
 *
 * Description      Sets the NCI transaction layer log level.
 *
 * Returns          void
 *
 ******************************************************************************/
static void tmsLogSetNciTxLogLevel(uint8_t level) {
    unsigned long num = 0;
    int len;
    char valueStr[PROP_VALUE_MAX] = {0};
    if (getTmsNumValue(NAME_TMSLOG_NCIX_LOGLEVEL, &num, sizeof(num))) {
        (*getLogLevel()).ncixLogLevel =
            (level > (unsigned char)num) ? level : (unsigned char)num;
    }
    if (getTmsNumValue(NAME_TMSLOG_NCIR_LOGLEVEL, &num, sizeof(num))) {
        (*getLogLevel()).ncirLogLevel =
            (level > (unsigned char)num) ? level : (unsigned char)num;
        ;
    }

    len = propertyGet(PROP_NAME_TMSLOG_NCI_LOGLEVEL, valueStr, "");
    if (len > 0) {
        /* let Android property override .conf variable */
        sscanf(valueStr, "%lu", &num);
        (*getLogLevel()).ncixLogLevel = (unsigned char)num;
        (*getLogLevel()).ncirLogLevel = (unsigned char)num;
    }
}

/******************************************************************************
 * Function         tmsLogInitializeLogLevel
 *
 * Description      initialize and get log level of module from libnfc-tms.conf
 *or
 *                  Android runtime properties.
 *                  The Android property nfc.tms_global_log_level is to
 *                  define log level for all modules. Modules log level will
 *overwide global level.
 *                  The Android property will overwide the level
 *                  in libnfc-tms.conf
 *
 *                  Android property names:
 *                      nfc.tmsLog_level_global    * defines log level for all
 *modules
 *                      nfc.tmsLog_level_extns     * extensions module log
 *                      nfc.tmsLog_level_hal       * Hal module log
 *log
 *                      nfc.tmsLog_level_tml       * TML module log
 *                      nfc.tmsLog_level_nci       * NCI transaction log
 *
 *                  Log Level values:
 *                      TMSLOG_LOG_SILENT_LOGLEVEL  0        * No trace to show
 *                      TMSLOG_LOG_ERROR_LOGLEVEL   1        * Show Error trace
 *only
 *                      TMSLOG_LOG_WARN_LOGLEVEL    2        * Show Warning
 *trace and Error trace
 *                      TMSLOG_LOG_DEBUG_LOGLEVEL   3        * Show all traces
 *
 * Returns          void
 *
 ******************************************************************************/
void tmsLogInitializeLogLevel(void) {
    uint8_t level = tmsLogSetGlobalLogLevel();
    tmsLogSetHALLogLevel(level);
    tmsLogSetExtnsLogLevel(level);
    tmsLogSetTmlLogLevel(level);
    tmsLogSetNciTxLogLevel(level);

    ALOGD_IF(*getNfcDebugEnabled(),
             "%s: global =%u, Fwdnld =%u, extns =%u, \
                hal =%u, tml =%u, ncir =%u, \
                ncix =%u",
             __func__, (*getLogLevel()).globalLogLevel, (*getLogLevel()).dnldLogLevel,
             (*getLogLevel()).extnsLogLevel, (*getLogLevel()).halLogLevel,
             (*getLogLevel()).tmlLogLevel, (*getLogLevel()).ncirLogLevel,
             (*getLogLevel()).ncixLogLevel);
}
/******************************************************************************
 * Function         tmsLogEnableDisableLogLevel
 *
 * Description      This function can be called to enable/disable the log levels
 *
 *
 *                  Log Level values:
 *                      TMSLOG_LOG_SILENT_LOGLEVEL  0        * No trace to show
 *                      TMSLOG_LOG_ERROR_LOGLEVEL   1        * Show Error trace
 *only
 *                      TMSLOG_LOG_WARN_LOGLEVEL    2        * Show Warning
 *trace and Error trace
 *                      TMSLOG_LOG_DEBUG_LOGLEVEL   3        * Show all traces
 *
 * Returns          void
 *
 ******************************************************************************/
uint8_t tmsLogEnableDisableLogLevel(uint8_t enable) {
    static NciLogLevel_t prevTraceLevel = {0, 0, 0, 0, 0, 0, 0};
    static uint8_t currState = 0x01;
    static bool prevDebugEnabled = true;
    uint8_t status = NFCSTATUS_FAILED;

    if (0x01 == enable && currState != 0x01) {
        memcpy(&(*getLogLevel()), &prevTraceLevel, sizeof(NciLogLevel_t));
        *getNfcDebugEnabled() = prevDebugEnabled;
        currState = 0x01;
        status = NFCSTATUS_SUCCESS;
    } else if (0x00 == enable && currState != 0x00) {
        prevDebugEnabled = *getNfcDebugEnabled();
        memcpy(&prevTraceLevel, &(*getLogLevel()), sizeof(NciLogLevel_t));
        (*getLogLevel()).halLogLevel = 0;
        (*getLogLevel()).extnsLogLevel = 0;
        (*getLogLevel()).tmlLogLevel = 0;
        (*getLogLevel()).ncixLogLevel = 0;
        (*getLogLevel()).ncirLogLevel = 0;
        *getNfcDebugEnabled() = false;
        currState = 0x00;
        status = NFCSTATUS_SUCCESS;
    }

    return status;
}
