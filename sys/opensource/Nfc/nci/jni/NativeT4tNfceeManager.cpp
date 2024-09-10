/******************************************************************************
 *
 *  The Original Work was created and released publicly in 2019 by NXP Semiconductors.
 *  Copyright (C) 2019 NXP Semiconductors
 *
 *  The Original Work was revised and released publicly in 2023 by Tsingteng Microsystem.
 *  Copyright (C) 2023 Tsingteng MicroSystem
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
 *  Copyright 2019 NXP
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
#ifdef TMS_NFC
#include <android-base/stringprintf.h>
#include <base/logging.h>
#include <nativehelper/ScopedPrimitiveArray.h>
#include "JavaClassConstants.h"
#include "NativeT4tNfcee.h"
#include "NfcJniUtil.h"
extern bool nfc_debug_enabled;
using android::base::StringPrintf;

namespace tms {
static const char* gNativeT4tNfceeClassName =
    "com/android/nfc/dhimpl/NativeT4tNfceeManager";

/*******************************************************************************
**
** Function:        t4tClearData
**
** Description:     This API will set all the T4T NFCEE NDEF data to zero.
**                  This API can be called regardless of NDEF file lock state.
**
** Returns:         boolean : Return the Success or fail of the operation.
**                  Return "True" when operation is successful. else "False"
**
*******************************************************************************/
jint t4tNfceeManager_doClearNdefT4tData(JNIEnv* e, jobject o) {
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);

  return t4tNfcEe.t4tClearData(e, o);
}
/*******************************************************************************
 **
 ** Function:        nfcManager_doWriteT4tData
 **
 ** Description:     Write the data into the T4T file of the specific file ID
 **
 ** Returns:         Return the size of data written
 **                  Return negative number of error code
 **
 *******************************************************************************/
jint t4tNfceeManager_doWriteT4tData(JNIEnv* e, jobject o, jbyteArray fileId,
                                    jbyteArray data, jint length) {
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);

  return t4tNfcEe.t4tWriteData(e, o, fileId, data, length);
}
/*******************************************************************************
**
** Function:        nfcManager_doReadT4tData
**
** Description:     Read the data from the T4T file of the specific file ID.
**
** Returns:         byte[] : all the data previously written to the specific
**                  file ID.
**                  Return one byte '0xFF' if the data was never written to the
**                  specific file ID,
**                  Return null if reading fails.
**
*******************************************************************************/
jbyteArray t4tNfceeManager_doReadT4tData(JNIEnv* e, jobject o,
                                         jbyteArray fileId) {
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);
  return t4tNfcEe.t4tReadData(e, o, fileId);
}
/*******************************************************************************
**
** Function:        t4tNfceeManager_enableT4tNfcee
**
** Description:     This function enable/disable t4t nfcee.
**
** Parameter:       jboolean enable: decide to enable or disable.
**
** Returns:         "TRUE" if value is successfully retrieved
**
*******************************************************************************/
jboolean t4tNfceeManager_enableT4tNfcee(JNIEnv* e, jobject o, jboolean enable) {
  return t4tNfcEe.enableT4tNfcee(e, o, enable);
}
/*******************************************************************************
**
** Function:        t4tNfceeManager_sendT4tRawApdu
**
** Description:     This function sends raw apdu to t4t nfcee.
**
** Parameter:       jbyteArray apdu: apdu command to be sent.
**                  jint apduLen: length of apdu command to be sent.
**
** Returns:         byte[]: response apdu if successfully retrieved.
**
*******************************************************************************/
jbyteArray t4tNfceeManager_sendT4tRawApdu(JNIEnv* e, jobject o, jbyteArray apdu, jint apduLen) {
  return t4tNfcEe.t4tSendRawApdu(e, o, apdu, apduLen);
}
/*****************************************************************************
 **
 ** Description:     JNI functions
 **
 *****************************************************************************/
static JNINativeMethod gMethods[] = {
    {"doWriteT4tData", "([B[BI)I", (void*)t4tNfceeManager_doWriteT4tData},
    {"doReadT4tData", "([B)[B", (void*)t4tNfceeManager_doReadT4tData},
    {"doClearNdefT4tData", "()Z", (void*)t4tNfceeManager_doClearNdefT4tData},
    {"enableT4tNfcee", "(Z)Z", (void*)t4tNfceeManager_enableT4tNfcee},
    {"sendT4tRawApdu", "([BI)[B", (void*)t4tNfceeManager_sendT4tRawApdu},
};

/*******************************************************************************
 **
 ** Function:        register_com_android_nfc_NativeT4tNfcee
 **
 ** Description:     Regisgter JNI functions with Java Virtual Machine.
 **                  e: Environment of JVM.
 **
 ** Returns:         Status of registration.
 **
 *******************************************************************************/
int register_com_android_nfc_NativeT4tNfcee(JNIEnv* e) {
  return jniRegisterNativeMethods(e, gNativeT4tNfceeClassName, gMethods,
                                  NELEM(gMethods));
}
}  // namespace tms
#endif
