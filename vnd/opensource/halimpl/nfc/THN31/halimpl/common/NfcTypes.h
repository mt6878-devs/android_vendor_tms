/*
 * Copyright (C) 2010-2020 NXP Semiconductors
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

#ifndef PHNFCTYPES_H
#define PHNFCTYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "TmsFeatures.h"

#ifndef true
    #define true (0x01) /* Logical True Value */
#endif
#ifndef TRUE
    #define TRUE (0x01) /* Logical True Value */
#endif
#ifndef false
    #define false (0x00) /* Logical False Value */
#endif
#ifndef FALSE
    #define FALSE (0x00) /* Logical False Value */
#endif
typedef uint8_t bool_t;     /* boolean data type */
typedef uint16_t NFCSTATUS; /* Return values */

/*
 * Deferred message. This message type will be posted to the client application
 * thread
 * to notify that a deferred call must be invoked.
 */
#define PH_LIBNFC_DEFERREDCALL_MSG (0x311)

/*
 * Deferred call declaration.
 * This type of API is called from ClientApplication ( main thread) to notify
 * specific callback.
 */
typedef void (*pNciHalDeferredCallback_t)(void *);

/*
 * Deferred parameter declaration.
 * This type of data is passed as parameter from ClientApplication (main thread)
 * to the
 * callback.
 */
typedef void *pNciHalDeferredParameter_t;

/*
 * Possible Hardware Configuration exposed to upper layer.
 * Typically this should be at least the communication link (Ex:"COM1","COM2")
 * the controller is connected to.
 */
typedef struct NciHalSConfig {
    uint8_t *pLogFile; /* Log File Name*/
    /* The client ID (thread ID or message queue ID) */
    intptr_t clientId;
} NciHalSConfig_t, *pNciHalSConfig_t;

/*
 * NFC Message structure contains message specific details like
 * message type, message specific data block details, etc.
 */
typedef struct NciHalMessage {
    uint32_t msgType; /* Type of the message to be posted*/
    void *pMsgData;    /* Pointer to message specific data block in case any*/
    uint32_t size;     /* Size of the datablock*/
} NciHalMessage_t, *pNciHalMessage_t;

/*
 * Deferred message specific info declaration.
 * This type of information is packed as message data when
 * PH_LIBNFC_DEFERREDCALL_MSG
 * type message is posted to message handler thread.
 */
typedef struct NciHalDeferredCall {
    pNciHalDeferredCallback_t pCallback;   /* pointer to Deferred callback */
    pNciHalDeferredParameter_t pParameter; /* pointer to Deferred parameter */
} NciHalDeferredCall_t;

/*
 *  Enumerated MIFARE Commands
 */

#define UNUSED_PROP(X) (void)(X);

/* PHNFCTYPES_H */
#endif
