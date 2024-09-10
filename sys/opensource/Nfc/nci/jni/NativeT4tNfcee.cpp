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
 *  Copyright 2019-2021 NXP
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
#include "NativeT4tNfcee.h"
#include <android-base/stringprintf.h>
#include <base/logging.h>
#include <nativehelper/ScopedPrimitiveArray.h>
// #include "MposManager.h"
#include "NfcJniUtil.h"
// #include "nci_defs_extns.h"
#include "nfa_nfcee_api.h"
#include "nfa_nfcee_int.h"
#include "nfc_config.h"

using android::base::StringPrintf;
extern bool nfc_debug_enabled;

/*Considering NCI response timeout which is 2s, Timeout set 100ms more*/
#define T4TNFCEE_TIMEOUT 2100
#define T4TOP_TIMEOUT 200
#define FILE_ID_LEN 0x02

extern bool gActivated;
namespace android {
extern bool isDiscoveryStarted();
extern void startRfDiscovery(bool isStart);
extern bool nfcManager_isNfcActive();
extern int nfcManager_doPartialInitialize(JNIEnv* e, jobject o, jint mode);
extern int nfcManager_doPartialDeInitialize(JNIEnv*, jobject);
}  // namespace android

namespace tms {
extern tNFA_STATUS TmsNfc_Write_Cmd_Common(uint8_t retlen, uint8_t* buffer);
}

NativeT4tNfcee NativeT4tNfcee::sNativeT4tNfceeInstance;
bool NativeT4tNfcee::sIsNfcOffTriggered = false;

NativeT4tNfcee::NativeT4tNfcee() { mBusy = false; memset (&mReadData, 0x00, sizeof(tNFA_RX_DATA)); mT4tOpStatus = NFA_STATUS_FAILED; }

/*****************************************************************************
**
** Function:        getInstance
**
** Description:     Get the NativeT4tNfcee singleton object.
**
** Returns:         NativeT4tNfcee object.
**
*******************************************************************************/
NativeT4tNfcee& NativeT4tNfcee::getInstance() {
  return sNativeT4tNfceeInstance;
}

/*******************************************************************************
**
** Function:        initialize
**
** Description:     Initialize all member variables.
**
** Returns:         None.
**
*******************************************************************************/
void NativeT4tNfcee::initialize(void) {
  sIsNfcOffTriggered = false;
  mBusy = false;
}

/*****************************************************************************
**
** Function:        onNfccShutdown
**
** Description:     This api shall be called in NFC OFF case.
**
** Returns:         none.
**
*******************************************************************************/
void NativeT4tNfcee::onNfccShutdown() {
  sIsNfcOffTriggered = true;
  if(mBusy) {
    /* Unblock JNI APIs */
    {
      SyncEventGuard g(mT4tNfcOffEvent);
      if (mT4tNfcOffEvent.wait(T4TOP_TIMEOUT) == false) {
        SyncEventGuard ga(mT4tNfcEeRWEvent);
        mT4tNfcEeRWEvent.notifyOne();
      }
    }
    /* Try to close the connection with t4t nfcee, discard the status */
    (void)closeConnection();
    resetBusy();
  }
}
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
jboolean NativeT4tNfcee::t4tClearData(JNIEnv* e, jobject o) {
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s:Enter: ", __func__);

  /*Local variable Initalization*/
  uint8_t pFileId[] = {0xE1, 0x04};
  jbyteArray fileIdArray = e->NewByteArray(sizeof(pFileId));
  e->SetByteArrayRegion(fileIdArray, 0, sizeof(pFileId), (jbyte*)pFileId);
  bool clear_status = false;

  /*Validate Precondition*/
  T4TNFCEE_STATUS_t t4tNfceeStatus =
      validatePreCondition(OP_CLEAR, fileIdArray);

  switch (t4tNfceeStatus) {
    case STATUS_SUCCESS:
      /*NFC is ON*/
      clear_status = performT4tClearData(pFileId);
      break;
    case ERROR_NFC_NOT_ON:
      /*NFC is OFF*/
      DLOG_IF(ERROR, nfc_debug_enabled) << StringPrintf(
          "%s:Exit: NFC is OFF. Returnig status : %d", __func__, clear_status);
      break;
    default:
      DLOG_IF(ERROR, nfc_debug_enabled) << StringPrintf(
          "%s:Exit: Returnig status : %d", __func__, clear_status);
      break;
  }
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s:Exit: ", __func__);
  return clear_status;
}
/*******************************************************************************
**
** Function:        performT4tClearData
**
** Description:     This api clear the T4T Nfcee data
**
** Returns:         boolean : Return the Success or fail of the operation.
**                  Return "True" when operation is successful. else "False"
**
*******************************************************************************/
jboolean NativeT4tNfcee::performT4tClearData(uint8_t* fileId) {
  bool t4tClearReturn = false;
  tNFA_STATUS status = NFA_STATUS_FAILED;

  /*Open connection and stop discovery*/
  if (setup() != NFA_STATUS_OK) return t4tClearReturn;

  /*Clear Ndef data*/
  SyncEventGuard g(mT4tNfcEeClrDataEvent);
  status = NFA_T4tNfcEeClear(fileId);
  if (status == NFA_STATUS_OK) {
    if (mT4tNfcEeClrDataEvent.wait(T4TNFCEE_TIMEOUT) == false)
      t4tClearReturn = false;
    else {
      if (mT4tOpStatus == NFA_STATUS_OK) {
        t4tClearReturn = true;
      }
    }
  }

  /*Close connection and start discovery*/
  cleanup();
  return t4tClearReturn;
}
/*******************************************************************************
**
** Function:        t4tWriteData
**
** Description:     Write the data into the T4T file of the specific file ID
**
** Returns:         Return the size of data written
**                  Return negative number of error code
**
*******************************************************************************/
jint NativeT4tNfcee::t4tWriteData(JNIEnv* e, jobject object, jbyteArray fileId,
                                  jbyteArray data, int length) {
  tNFA_STATUS status = NFA_STATUS_FAILED;

  T4TNFCEE_STATUS_t t4tNfceeStatus =
      validatePreCondition(OP_WRITE, fileId, data);
  if (t4tNfceeStatus != STATUS_SUCCESS) return t4tNfceeStatus;

  ScopedByteArrayRO bytes(e, fileId);
  if (bytes.size() < FILE_ID_LEN) {
    DLOG_IF(ERROR, nfc_debug_enabled)
        << StringPrintf("%s:Wrong File Id", __func__);
    return ERROR_INVALID_FILE_ID;
  }

  ScopedByteArrayRO bytesData(e, data);
  if (bytesData.size() == 0x00) {
    DLOG_IF(ERROR, nfc_debug_enabled)
        << StringPrintf("%s:Empty Data", __func__);
    return ERROR_EMPTY_PAYLOAD;
  }

  if ((int)bytesData.size() != length) {
    DLOG_IF(ERROR, nfc_debug_enabled)
        << StringPrintf("%s:Invalid Length", __func__);
    return ERROR_INVALID_LENGTH;
  }

  if (setup() != NFA_STATUS_OK) return ERROR_CONNECTION_FAILED;

  uint8_t* pFileId = NULL;
  pFileId = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(&bytes[0]));

  uint8_t* pData = NULL;
  pData = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(&bytesData[0]));

  jint t4tWriteReturn = STATUS_FAILED;
  {
    SyncEventGuard g(mT4tNfcEeRWEvent);
    status = NFA_T4tNfcEeWrite(pFileId, pData, bytesData.size());
    if (status == NFA_STATUS_OK) {
      if (mT4tNfcEeRWEvent.wait(T4TNFCEE_TIMEOUT) == false)
        t4tWriteReturn = STATUS_FAILED;
      else {
        if (mT4tOpStatus == NFA_STATUS_OK) {
          /*if status is success then return length of data written*/
          t4tWriteReturn = mReadData.len;
        } else if (mT4tOpStatus == NFA_STATUS_REJECTED) {
          t4tWriteReturn = ERROR_NDEF_VALIDATION_FAILED;
        } else if (mT4tOpStatus == NFA_T4T_STATUS_INVALID_FILE_ID){
          t4tWriteReturn = ERROR_INVALID_FILE_ID;
        } else if (mT4tOpStatus == NFA_STATUS_READ_ONLY) {
          t4tWriteReturn = ERROR_WRITE_PERMISSION;
        } else {
          t4tWriteReturn = STATUS_FAILED;
        }
      }
    }
  }

  /*Close connection and start discovery*/
  cleanup();
  DLOG_IF(ERROR, nfc_debug_enabled) << StringPrintf(
      "%s:Exit: Returnig status : %d", __func__, t4tWriteReturn);
  return t4tWriteReturn;
}

/*******************************************************************************
**
** Function:        t4tReadData
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
jbyteArray NativeT4tNfcee::t4tReadData(JNIEnv* e, jobject object,
                                       jbyteArray fileId) {
  tNFA_STATUS status = NFA_STATUS_FAILED;

  T4TNFCEE_STATUS_t t4tNfceeStatus = validatePreCondition(OP_READ, fileId);
  if (t4tNfceeStatus != STATUS_SUCCESS) return NULL;

  ScopedByteArrayRO bytes(e, fileId);
  ScopedLocalRef<jbyteArray> result(e, NULL);
  if (bytes.size() < FILE_ID_LEN) {
    DLOG_IF(ERROR, nfc_debug_enabled)
        << StringPrintf("%s:Wrong File Id", __func__);
    return NULL;
  }

  if (setup() != NFA_STATUS_OK) return NULL;

  uint8_t* pFileId = NULL;
  pFileId = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(&bytes[0]));

  { /*syncEvent code section*/
    SyncEventGuard g(mT4tNfcEeRWEvent);
    sRxDataBuffer.clear();
    status = NFA_T4tNfcEeRead(pFileId);
    if ((status != NFA_STATUS_OK) ||
        (mT4tNfcEeRWEvent.wait(T4TNFCEE_TIMEOUT) == false)) {
      DLOG_IF(ERROR, nfc_debug_enabled)
          << StringPrintf("%s:Read Failed, status = 0x%X", __func__, status);
      cleanup();
      return NULL;
    }
  }

  if (sRxDataBuffer.size() > 0) {
    result.reset(e->NewByteArray(sRxDataBuffer.size()));
    if (result.get() != NULL) {
      e->SetByteArrayRegion(result.get(), 0, sRxDataBuffer.size(),
            (const jbyte*)sRxDataBuffer.data());
    } else {
      char data[1] = {0xFF};
      result.reset(e->NewByteArray(0x01));
      e->SetByteArrayRegion(result.get(), 0, 0x01, (jbyte*)data);
      LOG(ERROR) << StringPrintf("%s: Failed to allocate java byte array",
               __func__);
    }
    sRxDataBuffer.clear();
  } else if (mT4tOpStatus == NFA_T4T_STATUS_INVALID_FILE_ID){
    char data[1] = {0xFF};
    result.reset(e->NewByteArray(0x01));
    e->SetByteArrayRegion(result.get(), 0, 0x01, (jbyte*)data);
  }
  /*Close connection and start discovery*/
  cleanup();
  return result.release();
}

/*******************************************************************************
**
** Function:        openConnection
**
** Description:     Open T4T Nfcee Connection
**
** Returns:         Status
**
*******************************************************************************/
tNFA_STATUS NativeT4tNfcee::openConnection() {
  tNFA_STATUS status = NFA_STATUS_FAILED;
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: Enter", __func__);
  SyncEventGuard g(mT4tNfcEeEvent);
  status = NFA_T4tNfcEeOpenConnection();
  if (status == NFA_STATUS_OK) {
    if (mT4tNfcEeEvent.wait(T4TNFCEE_TIMEOUT) == false)
      status = NFA_STATUS_FAILED;
    else
      status = mT4tNfcEeEventStat;
  }
  DLOG_IF(INFO, nfc_debug_enabled)
      << StringPrintf("%s: Exit status = 0x%02x", __func__, status);
  return status;
}

/*******************************************************************************
**
** Function:        closeConnection
**
** Description:     Close T4T Nfcee Connection
**
** Returns:         Status
**
*******************************************************************************/
tNFA_STATUS NativeT4tNfcee::closeConnection() {
  tNFA_STATUS status = NFA_STATUS_FAILED;
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: Enter", __func__);
  {
    SyncEventGuard g(mT4tNfcEeEvent);
    status = NFA_T4tNfcEeCloseConnection();
    if (status == NFA_STATUS_OK) {
      if (mT4tNfcEeEvent.wait(T4TNFCEE_TIMEOUT) == false)
        status = NFA_STATUS_FAILED;
      else
        status = mT4tNfcEeEventStat;
    }
  }

  DLOG_IF(INFO, nfc_debug_enabled)
      << StringPrintf("%s: Exit status = 0x%02x", __func__, status);
  return status;
}

/*******************************************************************************
**
** Function:        setup
**
** Description:     stops Discovery and opens T4TNFCEE connection
**
** Returns:         Status
**
*******************************************************************************/
tNFA_STATUS NativeT4tNfcee::setup(void) {
  tNFA_STATUS status = NFA_STATUS_FAILED;
  setBusy();
  if (android::isDiscoveryStarted()) {
    android::startRfDiscovery(false);
  }

  status = openConnection();
  if (status != NFA_STATUS_OK) {
    DLOG_IF(ERROR, nfc_debug_enabled) << StringPrintf(
        "%s: openConnection Failed, status = 0x%X", __func__, status);
    if (!android::isDiscoveryStarted()) android::startRfDiscovery(true);
    resetBusy();
  }
  return status;
}
/*******************************************************************************
**
** Function:        cleanup
**
** Description:     closes connection and starts discovery
**
** Returns:         Status
**
*******************************************************************************/
void NativeT4tNfcee::cleanup(void) {

  if(sIsNfcOffTriggered) {
    SyncEventGuard g(mT4tNfcOffEvent);
    mT4tNfcOffEvent.notifyOne();
    DLOG_IF(ERROR, nfc_debug_enabled) << StringPrintf("%s: Nfc Off triggered", __func__);
    return;
  }
  if (closeConnection() != NFA_STATUS_OK) {
    DLOG_IF(ERROR, nfc_debug_enabled) << StringPrintf("%s: closeConnection Failed", __func__);
  }
  if (!android::isDiscoveryStarted()) {
    android::startRfDiscovery(true);
  }
  resetBusy();
}

/*******************************************************************************
**
** Function:        validatePreCondition
**
** Description:     Runs precondition checks for requested operation
**
** Returns:         Status
**
*******************************************************************************/
T4TNFCEE_STATUS_t NativeT4tNfcee::validatePreCondition(T4TNFCEE_OPERATIONS_t op,
                                                       jbyteArray fileId,
                                                       jbyteArray data) {
  T4TNFCEE_STATUS_t t4tNfceeStatus = STATUS_SUCCESS;
  if (!android::nfcManager_isNfcActive()) {
    t4tNfceeStatus = ERROR_NFC_NOT_ON;
  } else if (sIsNfcOffTriggered) {
    t4tNfceeStatus = ERROR_NFC_OFF_TRIGGERED;
  } else if (gActivated) {
    t4tNfceeStatus = ERROR_RF_ACTIVATED;
  } else if (fileId == NULL && op != OP_SEND_APDU) {
    DLOG_IF(ERROR, nfc_debug_enabled)
        << StringPrintf("%s:Invalid File Id", __func__);
    t4tNfceeStatus = ERROR_INVALID_FILE_ID;
  }

  switch (op) {
    case OP_READ:
      break;
    case OP_WRITE:
    case OP_SEND_APDU:
      if (data == NULL) {
        DLOG_IF(ERROR, nfc_debug_enabled)
            << StringPrintf("%s:Empty data", __func__);
        t4tNfceeStatus = ERROR_EMPTY_PAYLOAD;
      }
      break;
    case OP_CLEAR:
    [[fallthrough]];
    default:
      break;
  }
  return t4tNfceeStatus;
}

/*******************************************************************************
**
** Function:        t4tReadComplete
**
** Description:     Updates read data to the waiting READ API
**
** Returns:         none
**
*******************************************************************************/
void NativeT4tNfcee::t4tReadComplete(tNFA_STATUS status, tNFA_RX_DATA data) {
  mT4tOpStatus = status;
  if (status == NFA_STATUS_OK) {
    if(data.len > 0) {
      sRxDataBuffer.append(data.p_data, data.len);
      DLOG_IF(INFO, nfc_debug_enabled)
        << StringPrintf("%s: Read Data len new: %d ", __func__, data.len);
    }
  }
  SyncEventGuard g(mT4tNfcEeRWEvent);
  mT4tNfcEeRWEvent.notifyOne();
}

/*******************************************************************************
 **
 ** Function:        t4tWriteComplete
 **
 ** Description:     Returns write complete information
 **
 ** Returns:         none
 **
 *******************************************************************************/
void NativeT4tNfcee::t4tWriteComplete(tNFA_STATUS status, tNFA_RX_DATA data) {
  mReadData.len = 0x00;
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: Enter", __func__);
  if (status == NFA_STATUS_OK) mReadData.len = data.len;
  mT4tOpStatus = status;
  SyncEventGuard g(mT4tNfcEeRWEvent);
  mT4tNfcEeRWEvent.notifyOne();
}
/*******************************************************************************
 **
 ** Function:        t4tClearComplete
 **
 ** Description:     Update T4T clear data status, waiting T4tClearData API.
 **
 ** Returns:         none
 **
 *******************************************************************************/
void NativeT4tNfcee::t4tClearComplete(tNFA_STATUS status) {
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: Enter", __func__);
  mT4tOpStatus = status;
  SyncEventGuard g(mT4tNfcEeClrDataEvent);
  mT4tNfcEeClrDataEvent.notifyOne();
}
/*******************************************************************************
 **
 ** Function:        t4tSendApduComplete
 **
 ** Description:     Returns send apdu complete information
 **
 ** Returns:         none
 **
 *******************************************************************************/
void NativeT4tNfcee::t4tSendApduComplete(tNFA_STATUS status, tNFA_RX_DATA data) {
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: Enter", __func__);
  mT4tOpStatus = status;
  if (status == NFA_STATUS_OK) {
    if(data.len > 0) {
      sRxDataBuffer.append(data.p_data, data.len);
      DLOG_IF(INFO, nfc_debug_enabled)
        << StringPrintf("%s: Response Data len: %d ", __func__, data.len);
    }
  }
  SyncEventGuard g(mT4tNfcEeRWEvent);
  mT4tNfcEeRWEvent.notifyOne();
}
/*******************************************************************************
**
** Function:        t4tNfceeEventHandler
**
** Description:     Handles callback events received from lower layer
**
** Returns:         none
**
*******************************************************************************/
void NativeT4tNfcee::eventHandler(uint8_t event,
                                  tNFA_CONN_EVT_DATA* eventData) {
  switch (event) {
    case NFA_T4TNFCEE_EVT:
      DLOG_IF(INFO, nfc_debug_enabled)
          << StringPrintf("%s: NFA_T4TNFCEE_EVT", __func__);
      {
        SyncEventGuard guard(mT4tNfcEeEvent);
        mT4tNfcEeEventStat = eventData->status;
        mT4tNfcEeEvent.notifyOne();
      }
      break;

    case NFA_T4TNFCEE_READ_CPLT_EVT:
      DLOG_IF(INFO, nfc_debug_enabled)
          << StringPrintf("%s: NFA_T4TNFCEE_READ_CPLT_EVT", __func__);
      t4tReadComplete(eventData->status, eventData->data);
      break;

    case NFA_T4TNFCEE_WRITE_CPLT_EVT:
      DLOG_IF(INFO, nfc_debug_enabled)
          << StringPrintf("%s: NFA_T4TNFCEE_WRITE_CPLT_EVT", __func__);
      t4tWriteComplete(eventData->status, eventData->data);
      break;

    case NFA_T4TNFCEE_CLEAR_CPLT_EVT:
      DLOG_IF(INFO, nfc_debug_enabled)
          << StringPrintf("%s: NFA_T4TNFCEE_CLEAR_CPLT_EVT", __func__);
      t4tClearComplete(eventData->status);
      break;

    case NFA_T4TNFCEE_SEND_APDU_CPLT_EVT:
      DLOG_IF(INFO, nfc_debug_enabled)
          << StringPrintf("%s: NFA_T4TNFCEE_SEND_APDU_CPLT_EVT", __func__);
      t4tSendApduComplete(eventData->status, eventData->data);
      break;

    default:
      DLOG_IF(INFO, nfc_debug_enabled)
          << StringPrintf("%s: unknown Event", __func__);
      break;
  }
}

/*******************************************************************************
**
** Function:        enableT4tNfcee
**
** Description:     This function enable/disable t4t nfcee.
**
** Parameter:       jboolean enable: decide to enable or disable.
**
** Returns:         "TRUE" if value is successfully retrieved
**
*******************************************************************************/
bool NativeT4tNfcee::enableT4tNfcee(JNIEnv* e, jobject o, jboolean enable) {
  tNFA_STATUS status = NFA_STATUS_FAILED;
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);

  if (!android::nfcManager_isNfcActive()) {
    DLOG_IF(ERROR, nfc_debug_enabled) << StringPrintf("%s: nfc is not active!", __func__);
    return false;
  }

  // disable when t4t nfcee is working, clean first.
  if(!enable && mBusy) {
    /* Unblock JNI APIs */
    {
      SyncEventGuard g(mT4tNfcOffEvent);
      if (mT4tNfcOffEvent.wait(T4TOP_TIMEOUT) == false) {
        SyncEventGuard ga(mT4tNfcEeRWEvent);
        mT4tNfcEeRWEvent.notifyOne();
      }
    }
    /* Try to close the connection with t4t nfcee, discard the status */
    (void)closeConnection();
    resetBusy();
  }

  std::vector<uint8_t> enableCmd = {0x20,
                                    0x02,
                                    0x05,
                                    0x01,
                                    0xA0,
                                    TMS_NFC_CLPARAM_ID_T4T_NFCEE,
                                    TMS_PARAM_LEN_T4T_NFCEE};

  enableCmd.push_back(enable ? 0x01 : 0x00);

  if (!enable) {
    status = NFA_EeModeSet(T4TNFCEE_TARGET_HANDLE, NFC_MODE_DEACTIVATE);
  }
  if (status != NFA_STATUS_OK) return false;

  status = tms::TmsNfc_Write_Cmd_Common(enableCmd.size(), &enableCmd[0]);
  if (status != NFA_STATUS_OK) return false;

  if (enable) {
    status = NFA_EeModeSet(T4TNFCEE_TARGET_HANDLE, NFC_MODE_ACTIVATE);
  }
  if (status != NFA_STATUS_OK) return false;

  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: Exit", __func__);

  return true;
}
/*******************************************************************************
**
** Function:        sendT4tRawApdu
**
** Description:     This function sends raw apdu to t4t nfcee.
**
** Parameter:       jbyteArray apdu: apdu command to be sent.
**                  jint apduLen: length of apdu command to be sent.
**
** Returns:         byte[]: response apdu if successfully retrieved.
**
*******************************************************************************/
jbyteArray NativeT4tNfcee::t4tSendRawApdu(JNIEnv* e, jobject o, jbyteArray apdu, jint apduLen) {
  tNFA_STATUS status = NFA_STATUS_FAILED;
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);

  T4TNFCEE_STATUS_t t4tNfceeStatus =
      validatePreCondition(OP_SEND_APDU, NULL, apdu);
  if (t4tNfceeStatus != STATUS_SUCCESS) return NULL;

  ScopedLocalRef<jbyteArray> result(e, NULL);
  ScopedByteArrayRO bytesData(e, apdu);
  if (bytesData.size() == 0x00) {
    DLOG_IF(ERROR, nfc_debug_enabled)
        << StringPrintf("%s:Empty apdu", __func__);
    return NULL;
  }

  if ((int)bytesData.size() != apduLen) {
    DLOG_IF(ERROR, nfc_debug_enabled)
        << StringPrintf("%s:Invalid apduLen", __func__);
    return NULL;
  }

  if (setup() != NFA_STATUS_OK) return NULL;

  uint8_t* pApdu = NULL;
  pApdu = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(&bytesData[0]));

  { /*syncEvent code section*/
    SyncEventGuard g(mT4tNfcEeRWEvent);
    sRxDataBuffer.clear();
    status = NFA_T4tNfcEeSendApdu(pApdu, bytesData.size());
    if ((status != NFA_STATUS_OK) ||
        (mT4tNfcEeRWEvent.wait(T4TNFCEE_TIMEOUT) == false)) {
      DLOG_IF(ERROR, nfc_debug_enabled)
          << StringPrintf("%s:Send Apdu Failed, status = 0x%X", __func__, status);
      cleanup();
      return NULL;
    }
  }

  if (sRxDataBuffer.size() > 0) {
    result.reset(e->NewByteArray(sRxDataBuffer.size()));
    if (result.get() != NULL) {
      e->SetByteArrayRegion(result.get(), 0, sRxDataBuffer.size(),
            (const jbyte*)sRxDataBuffer.data());
    } else {
      char data[1] = {0xFF};
      result.reset(e->NewByteArray(0x01));
      e->SetByteArrayRegion(result.get(), 0, 0x01, (jbyte*)data);
      LOG(ERROR) << StringPrintf("%s: Failed to allocate java byte array",
               __func__);
    }
    sRxDataBuffer.clear();
  }
  /*Close connection and start discovery*/
  cleanup();
  return result.release();
}
/*******************************************************************************
**
** Function:        isNdefWritePermission
**
** Description:     Read from config file for write permission
**
** Parameter:       NULL
**
** Returns:         Return T4T NDEF write permission status.
**                  Return "True" when T4T write permission allow to change.
**                  Otherwise, "False" shall be returned.
**
*******************************************************************************/
bool NativeT4tNfcee::isNdefWritePermission() {
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);
  unsigned long num = 0x00;
  bool isNdefWriteAccess = false;
  if (NfcConfig::hasKey(NAME_TMS_T4T_NFCEE_ENABLE))
    num = NfcConfig::getUnsigned(NAME_TMS_T4T_NFCEE_ENABLE);

  if ((num & MASK_T4T_FEATURE_BIT) && (num & (1 << MASK_PROP_NDEF_FILE_BIT)))
    isNdefWriteAccess = true;

  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: Exit 0x%lx", __func__, num);
  return isNdefWriteAccess;
}
/*******************************************************************************
 **
 ** Function:        isT4tNfceeBusy
 **
 ** Description:     Returns True if T4tNfcee operation is ongoing else false
 **
 ** Returns:         true/false
 **
 *******************************************************************************/
bool NativeT4tNfcee::isT4tNfceeBusy(void) { return mBusy; }

/*******************************************************************************
 **
 ** Function:        setBusy
 **
 ** Description:     Sets busy flag indicating T4T operation is ongoing
 **
 ** Returns:         none
 **
 *******************************************************************************/
void NativeT4tNfcee::setBusy() { mBusy = true; }

/*******************************************************************************
 **
 ** Function:        resetBusy
 **
 ** Description:     Resets busy flag indicating T4T operation is completed
 **
 ** Returns:         none
 **
 *******************************************************************************/
void NativeT4tNfcee::resetBusy() { mBusy = false; }
/*******************************************************************************
**
** Function:        getT4TNfceeAid
**
** Description:     Get the T4T Nfcee AID.
**
** Returns:         T4T AID: vector<uint8_t>
**
*******************************************************************************/
vector<uint8_t> NativeT4tNfcee::getT4TNfceeAid() {
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s:enter", __func__);

  std::vector<uint8_t> t4tNfceeAidBuf{0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01};

  if (NfcConfig::hasKey(NAME_TMS_T4T_NDEF_NFCEE_AID)) {
    t4tNfceeAidBuf = NfcConfig::getBytes(NAME_TMS_T4T_NDEF_NFCEE_AID);
  }

  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s:Exit", __func__);

  return t4tNfceeAidBuf;
}

/*******************************************************************************
**
** Function:        isFwSupportNonStdT4TAid
**
** Description:     Check FW supports Non-standard AID or not.
**
** Returns:         true: FW support NON-STD AID
**                  false: FW not support NON-STD AID
**
*******************************************************************************/
bool NativeT4tNfcee::isFwSupportNonStdT4TAid() {
  jboolean isFwSupport = false;
  LOG(INFO) << StringPrintf(
      "nfcManager_isFwSupportNonStdT4TAid Enter isFwSupport = %d", isFwSupport);
  return isFwSupport;
}
/*******************************************************************************
**
** Function:        checkAndUpdateT4TAid
**
** Description:     Check and update T4T Ndef Nfcee AID.
**
** Returns:         void
**
*******************************************************************************/
void NativeT4tNfcee::checkAndUpdateT4TAid(uint8_t* t4tNdefAid,
                                          uint8_t* t4tNdefAidLen) {
  if (!isFwSupportNonStdT4TAid()) {
    uint8_t stdT4tAid[] = {0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01};
    *t4tNdefAidLen = sizeof(stdT4tAid);
    memcpy(t4tNdefAid, stdT4tAid, *t4tNdefAidLen);
  } else {
    vector<uint8_t> t4tNfceeAidBuf = getT4TNfceeAid();
    uint8_t* t4tAidBuf = t4tNfceeAidBuf.data();
    *t4tNdefAidLen = t4tNfceeAidBuf.size();
    memcpy(t4tNdefAid, t4tAidBuf, *t4tNdefAidLen);
  }
}
#endif
