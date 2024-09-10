/*
 * Copyright (C) 2010-2014 NXP Semiconductors
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

#if !defined(TMSLOG__H_INCLUDED)
#define TMSLOG__H_INCLUDED
#include <log/log.h>
#include <NfcStatus.h>
#include <TmsNciHal_Adaptation.h>

typedef struct NciLogLevel {
    uint8_t globalLogLevel;
    uint8_t extnsLogLevel;
    uint8_t halLogLevel;
    uint8_t dnldLogLevel;
    uint8_t tmlLogLevel;
    uint8_t ncixLogLevel;
    uint8_t ncirLogLevel;
} NciLogLevel_t;

NciLogLevel_t *getLogLevel(void);

/* ####################### Set the log module name in .conf file
 * ########################## */
#define NAME_TMSLOG_EXTNS_LOGLEVEL "TMSLOG_EXTNS_LOGLEVEL"
#define NAME_TMSLOG_NCIHAL_LOGLEVEL "TMSLOG_NCIHAL_LOGLEVEL"
#define NAME_TMSLOG_NCIX_LOGLEVEL "TMSLOG_NCIX_LOGLEVEL"
#define NAME_TMSLOG_NCIR_LOGLEVEL "TMSLOG_NCIR_LOGLEVEL"
#define NAME_TMSLOG_TML_LOGLEVEL "TMSLOG_TML_LOGLEVEL"

/* ####################### Set the log module name by Android property
 * ########################## */
#define PROP_NAME_TMSLOG_GLOBAL_LOGLEVEL "nfc.tmsLog_level_global"
#define PROP_NAME_TMSLOG_EXTNS_LOGLEVEL "nfc.tmsLog_level_extns"
#define PROP_NAME_TMSLOG_NCIHAL_LOGLEVEL "nfc.tmsLog_level_hal"
#define PROP_NAME_TMSLOG_NCI_LOGLEVEL "nfc.tmsLog_level_nci"
#define PROP_NAME_TMSLOG_TML_LOGLEVEL "nfc.tmsLog_level_tml"

/* ####################### Set the logging level for EVERY COMPONENT here
 * ######################## :START: */
#define TMSLOG_LOG_SILENT_LOGLEVEL 0x00
#define TMSLOG_LOG_ERROR_LOGLEVEL 0x01
#define TMSLOG_LOG_WARN_LOGLEVEL 0x02
#define TMSLOG_LOG_DEBUG_LOGLEVEL 0x03
/* ####################### Set the default logging level for EVERY COMPONENT
 * here ########################## :END: */

/* The Default log level for all the modules. */
#define TMSLOG_DEFAULT_LOGLEVEL TMSLOG_LOG_ERROR_LOGLEVEL
#define TMSLOG_ITEM_EXTNS "TmsExtns"
#define TMSLOG_ITEM_NCIHAL "TmsHal"
#define TMSLOG_ITEM_NCIX "TmsNciX"
#define TMSLOG_ITEM_NCIR "TmsNciR"
#define TMSLOG_ITEM_TML "TmsTml"

/* Logging APIs used by TmsExtns module */
#define TMSLOG_EXTNS_D(...)                                       \
    {                                                               \
        if ((*getNfcDebugEnabled()) ||                                   \
                ((*getLogLevel()).extnsLogLevel >= TMSLOG_LOG_DEBUG_LOGLEVEL))  \
            LOG_PRI(ANDROID_LOG_DEBUG, TMSLOG_ITEM_EXTNS, __VA_ARGS__); \
    }
#define TMSLOG_EXTNS_W(...)                                      \
    {                                                              \
        if ((*getNfcDebugEnabled()) ||                                  \
                ((*getLogLevel()).extnsLogLevel >= TMSLOG_LOG_WARN_LOGLEVEL))  \
            LOG_PRI(ANDROID_LOG_WARN, TMSLOG_ITEM_EXTNS, __VA_ARGS__); \
    }
#define TMSLOG_EXTNS_E(...)                                       \
    {                                                               \
        if ((*getLogLevel()).extnsLogLevel >= TMSLOG_LOG_ERROR_LOGLEVEL)  \
            LOG_PRI(ANDROID_LOG_ERROR, TMSLOG_ITEM_EXTNS, __VA_ARGS__); \
    }

/* Logging APIs used by TmsNciHal module */
#define TMSLOG_NCIHAL_D(...)                                       \
    {                                                                \
        if ((*getNfcDebugEnabled()) ||                                     \
                ((*getLogLevel()).halLogLevel >= TMSLOG_LOG_DEBUG_LOGLEVEL))     \
            LOG_PRI(ANDROID_LOG_DEBUG, TMSLOG_ITEM_NCIHAL, __VA_ARGS__); \
    }
#define TMSLOG_NCIHAL_W(...)                                      \
    {                                                               \
        if ((*getNfcDebugEnabled()) ||                                    \
                ((*getLogLevel()).halLogLevel >= TMSLOG_LOG_WARN_LOGLEVEL))     \
            LOG_PRI(ANDROID_LOG_WARN, TMSLOG_ITEM_NCIHAL, __VA_ARGS__); \
    }
#define TMSLOG_NCIHAL_E(...)                                       \
    {                                                                \
        if ((*getLogLevel()).halLogLevel >= TMSLOG_LOG_ERROR_LOGLEVEL)     \
            LOG_PRI(ANDROID_LOG_ERROR, TMSLOG_ITEM_NCIHAL, __VA_ARGS__); \
    }

/* Logging APIs used by TmsNciX module */
#define TMSLOG_NCIX_D(...)                                       \
    {                                                              \
        if ((*getNfcDebugEnabled()) ||                                   \
                ((*getLogLevel()).ncixLogLevel >= TMSLOG_LOG_DEBUG_LOGLEVEL))  \
            LOG_PRI(ANDROID_LOG_DEBUG, TMSLOG_ITEM_NCIX, __VA_ARGS__); \
    }
#define TMSLOG_NCIX_W(...)                                      \
    {                                                             \
        if ((*getNfcDebugEnabled()) ||                                 \
                ((*getLogLevel()).ncixLogLevel >= TMSLOG_LOG_WARN_LOGLEVEL))  \
            LOG_PRI(ANDROID_LOG_WARN, TMSLOG_ITEM_NCIX, __VA_ARGS__); \
    }
#define TMSLOG_NCIX_E(...)                                       \
    {                                                              \
        if ((*getLogLevel()).ncixLogLevel >= TMSLOG_LOG_ERROR_LOGLEVEL)  \
            LOG_PRI(ANDROID_LOG_ERROR, TMSLOG_ITEM_NCIX, __VA_ARGS__); \
    }

/* Logging APIs used by TmsNciR module */
#define TMSLOG_NCIR_D(...)                                       \
    {                                                              \
        if ((*getNfcDebugEnabled()) ||                                  \
                ((*getLogLevel()).ncirLogLevel >= TMSLOG_LOG_DEBUG_LOGLEVEL))  \
            LOG_PRI(ANDROID_LOG_DEBUG, TMSLOG_ITEM_NCIR, __VA_ARGS__); \
    }
#define TMSLOG_NCIR_W(...)                                      \
    {                                                             \
        if ((*getNfcDebugEnabled()) ||                                 \
                ((*getLogLevel()).ncirLogLevel >= TMSLOG_LOG_WARN_LOGLEVEL))  \
            LOG_PRI(ANDROID_LOG_WARN, TMSLOG_ITEM_NCIR, __VA_ARGS__); \
    }
#define TMSLOG_NCIR_E(...)                                       \
    {                                                              \
        if ((*getLogLevel()).ncirLogLevel >= TMSLOG_LOG_ERROR_LOGLEVEL)  \
            LOG_PRI(ANDROID_LOG_ERROR, TMSLOG_ITEM_NCIR, __VA_ARGS__); \
    }

/* Logging APIs used by TmsTml module */
#define TMSLOG_TML_D(...)                                       \
    {                                                             \
        if ((*getNfcDebugEnabled()) ||                                 \
                ((*getLogLevel()).tmlLogLevel >= TMSLOG_LOG_DEBUG_LOGLEVEL))  \
            LOG_PRI(ANDROID_LOG_DEBUG, TMSLOG_ITEM_TML, __VA_ARGS__); \
    }
#define TMSLOG_TML_W(...)                                      \
    {                                                            \
        if ((*getNfcDebugEnabled()) ||                                \
                ((*getLogLevel()).tmlLogLevel >= TMSLOG_LOG_WARN_LOGLEVEL))  \
            LOG_PRI(ANDROID_LOG_WARN, TMSLOG_ITEM_TML, __VA_ARGS__); \
    }
#define TMSLOG_TML_E(...)                                       \
    {                                                             \
        if ((*getLogLevel()).tmlLogLevel >= TMSLOG_LOG_ERROR_LOGLEVEL)  \
            LOG_PRI(ANDROID_LOG_ERROR, TMSLOG_ITEM_TML, __VA_ARGS__); \
    }

void tmsLogInitializeLogLevel(void);
uint8_t tmsLogEnableDisableLogLevel(uint8_t enable);

#endif /* TMSLOG__H_INCLUDED */
