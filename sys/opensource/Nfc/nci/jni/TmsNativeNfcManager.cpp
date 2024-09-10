/*
 * Copyright (C) 2021 Tsingteng MicroSystem
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

#include <android-base/stringprintf.h>
#include <base/logging.h>
#include <log/log.h>
#include <nativehelper/ScopedLocalRef.h>
#include <nativehelper/ScopedPrimitiveArray.h>
#include <nativehelper/ScopedUtfChars.h>
#include <nativehelper/JNIHelp.h>
#include "NfcJniUtil.h"
#include "NfcTag.h"
#include "RoutingManager.h"
#include "tms_version.h"
#include "nfc_config.h"

extern bool nfc_debug_enabled;
extern SyncEvent gIsReconfiguringDiscovery;

using android::base::StringPrintf;
#define ONE_SECOND_MS 1000
#define TRIGGER_TYPE_M1_RDM 0x01
#define OFFSET_M1_RDM_TRIGGER_TYPE 3
#define OFFSET_M1_RDM_AUTH_STATE 4
#define LENGTH_M1_RDM_NTF 5
extern bool gActivated;
SyncEvent sNfaTransitConfigEvent;  // event for NFA_SetTransitConfig()
SyncEvent sTmsCommonWriteEvt;
tNFA_STATUS sWriteStatus;

namespace android {
extern void startRfDiscovery(bool isStart);
extern bool isDiscoveryStarted();
extern bool sSeRfActive;
extern bool nfcManager_isNfcActive();
}

using android::startRfDiscovery;
using android::isDiscoveryStarted;
using android::nfcManager_isNfcActive;

void nfcManager_abortWaits() {
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);
    {
        SyncEventGuard guard(sTmsCommonWriteEvt);
        sTmsCommonWriteEvt.notifyOne();
    }
    {
        SyncEventGuard guard(sNfaTransitConfigEvent);
        sNfaTransitConfigEvent.notifyOne();
    }
}

namespace tms { /* namespace tms start*/

#define EVT_M1_AUTH_RESULT 0x30

#define EVT_LEVEL_L1 0x01
#define EVT_LEVEL_L2 0x02
#define EVT_LEVEL_L3 0x04

#define EVT_VALUE_L1 0x35
#define EVT_VALUE_L2 0x36
#define EVT_VALUE_L3 0x41

jmethodID gCachedTmsNfcManagerNotifyRawPtmCommandCallback;
jmethodID gCachedTmsNfcManagerNotifyM1RawDataAuthCallback;
jmethodID gCachedTmsNfcManagerNotifyLxDebugUploadCallback;
jmethodID gCachedTmsNfcManagerNotifyNfceeAidSelectCallback;
bool gIsM1RawDataEnabled = false;
static SyncEvent sSendRawVsCommandEvent;
static uint8_t sRawVsResponseData[260];
static int sRawVsResponseLen = -1;

static uint8_t sLxDebugEventLevel = 0;

static struct nfc_jni_native_data* sCachedNat = NULL;

const char* gTmsNativeNfcManagerClassName =
    "com/tms/nfc/dhimpl/TmsNativeNfcManager";

static void nfcManager_doSkipTagSelect(JNIEnv* e, jobject o,jint proto) {
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: proto=0x%02x", __func__, proto);
    NfcTag::getInstance().setTagSkipProtocol(proto);
}

static void nfcManager_doSetRfListenMask(JNIEnv* e, jobject o,jint listenMask, jboolean restartRfDiscovery) {
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: listenMask=%d, restartRfDiscovery=%d", __func__, listenMask, restartRfDiscovery);
    tNFA_STATUS status = NFA_STATUS_FAILED;

    if (!nfcManager_isNfcActive()) {
      LOG(ERROR) << StringPrintf("%s: nfc is not enabled", __func__);
      return;
    }

    gIsReconfiguringDiscovery.start();
    status = NFA_SetTmsRfListenMask(listenMask);
    if (status != NFA_STATUS_OK) {
        DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: NFA_SetTmsRfListenMask error: %d", __func__, status);
        gIsReconfiguringDiscovery.end();
        return;
    }

    if (restartRfDiscovery) {
        if (isDiscoveryStarted()) {
            startRfDiscovery(false);
        }
        startRfDiscovery(true);
    }
    gIsReconfiguringDiscovery.end();
}

static int nfcManager_doGetRfListenMask(JNIEnv* e, jobject o) {
    uint8_t listenMask = NFA_GetTmsRfListenMask();
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: listenMask=%d", __func__, listenMask);
    return listenMask;
}

/*******************************************************************************
 **
 ** Function:        tmsNfcManager_setUserDefaultRoutesPref
 ** Description:     Set default routes as set by user through
 *NfcSettingsAdapter
 **                  APIs.
 **
 **                  e: JVM environment.
 **                  o: Java object.
 **
 *******************************************************************************/
static void tmsNfcManager_setUserDefaultRoutesPref(
    JNIEnv* e, jobject o, jint mifareRoute, jint isoDepRoute, jint felicaRoute,
    jint abTechRoute, jint scRoute, jint aidRoute) {
  RoutingManager& routingManager = RoutingManager::getInstance();
  routingManager.setUserDefaultRoutesPref(mifareRoute, isoDepRoute, felicaRoute,
                                          abTechRoute, scRoute, aidRoute);
}

static void nfaNciRawCommandCallback(uint8_t event, uint16_t param_len, uint8_t* p_param) {
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter, event=%d len= %d", __func__, event, param_len);

  SyncEventGuard g(sSendRawVsCommandEvent);

  sRawVsResponseLen = param_len;
  memcpy(sRawVsResponseData, p_param, param_len);

  sSendRawVsCommandEvent.notifyOne();

  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: exit", __func__);
}

static jbyteArray nfcManager_doSendRawNciCommand(JNIEnv* e, jobject o, jbyteArray data, jboolean restartRfDiscovery) {
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter, restartRfDiscovery = %d", __func__, restartRfDiscovery);

  if (!nfcManager_isNfcActive()) {
    LOG(ERROR) << StringPrintf("%s: nfc is not enabled", __func__);
    return NULL;
  }

  bool wasDiscoveryEnabled = false;
  if (restartRfDiscovery) {
    SyncEventGuard g(gIsReconfiguringDiscovery);
    bool isRfEnabled = isDiscoveryStarted();
    if (isRfEnabled) {
      DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: stop rf discovery first", __func__);
      startRfDiscovery(false);
      wasDiscoveryEnabled = true;
    }
  }

  SyncEventGuard guard(sSendRawVsCommandEvent);

  sRawVsResponseLen = -1;

  ScopedLocalRef<jbyteArray> dataJavaArray(e, NULL);
  ScopedByteArrayRO bytes(e, data);
  uint8_t* buf = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(&bytes[0]));
  size_t bufLen = bytes.size();
  int ret = NFA_SendRawVsCommand(bufLen, buf, nfaNciRawCommandCallback);

  if (ret != NFA_STATUS_OK) {
    LOG(ERROR) << StringPrintf("%s: send raw nci data failed, ret=%d", __func__, ret);
    goto restart_rf_discovery;
  }

  if (!sSendRawVsCommandEvent.wait(2000) || sRawVsResponseLen <= 0) {
    // timeout or no rsp
    LOG(ERROR) << StringPrintf("%s: get rsp failed, sRawVsResponseLen=%d", __func__, sRawVsResponseLen);
    goto restart_rf_discovery;
  }

  dataJavaArray.reset(e->NewByteArray(sRawVsResponseLen));
  CHECK(dataJavaArray.get());
  e->SetByteArrayRegion((jbyteArray)dataJavaArray.get(), 0, sRawVsResponseLen, (jbyte*)sRawVsResponseData);
  CHECK(!e->ExceptionCheck());

restart_rf_discovery:
  if (wasDiscoveryEnabled) {
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: start rf discovery", __func__);
    startRfDiscovery(true);
  }

  return dataJavaArray.release();
}

static void TmsResponse_Cb(uint8_t event, uint16_t param_len,
                           uint8_t* p_param) {
  (void)event;
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf(
      "TmsResponse_Cb Received length data = 0x%x status = 0x%x", param_len,
      p_param[3]);
  if (p_param != NULL) {
    if (p_param[3] == 0x00) {
      sWriteStatus = NFA_STATUS_OK;
    } else {
      sWriteStatus = NFA_STATUS_FAILED;
    }
    SyncEventGuard guard(sTmsCommonWriteEvt);
    sTmsCommonWriteEvt.notifyOne();
  }
}


/*******************************************************************************
 **
 ** Function:        TmsNfc_Write_Cmd()
 **
 ** Description:     Writes the command to NFCC
 **
 ** Returns:         success/failure
 **
 *******************************************************************************/
tNFA_STATUS TmsNfc_Write_Cmd_Common(uint8_t retlen, uint8_t* buffer) {
  tNFA_STATUS status = NFA_STATUS_FAILED;
  sWriteStatus = NFA_STATUS_FAILED;
  SyncEventGuard guard(sTmsCommonWriteEvt);
  status = NFA_SendRawVsCommand(retlen, buffer, TmsResponse_Cb);
  if (status == NFA_STATUS_OK) {
    DLOG_IF(INFO, nfc_debug_enabled)
        << StringPrintf("%s: Success NFA_SendRawVsCommand", __func__);
    sTmsCommonWriteEvt.wait(); /* wait for callback */
  } else {
    LOG(ERROR) << StringPrintf("%s: Failed NFA_SendRawVsCommand", __func__);
  }
  status = sWriteStatus;
  return status;
}

static void nfaPtmRawCommandCallback(uint8_t event, uint16_t param_len, uint8_t* p_param) {
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter, event=%d len= %d", __func__, event, param_len);

  if (sCachedNat == NULL) {
    LOG(ERROR) << StringPrintf("%s: jni env is null", __func__);
    return;
  }

  JNIEnv* e = NULL;
  ScopedAttach attach(sCachedNat->vm, &e);
  if (e == NULL) {
    LOG(ERROR) << StringPrintf("%s: jni env is null", __func__);
    return;
  }

  ScopedLocalRef<jobject> dataJavaArray(e, e->NewByteArray(param_len));
  CHECK(dataJavaArray.get());
  e->SetByteArrayRegion((jbyteArray)dataJavaArray.get(), 0, param_len, (jbyte*)p_param);
  CHECK(!e->ExceptionCheck());

  e->CallVoidMethod(sCachedNat->manager,
                    gCachedTmsNfcManagerNotifyRawPtmCommandCallback,
                    event, param_len, dataJavaArray.get());
  if (e->ExceptionCheck()) {
    e->ExceptionClear();
    LOG(ERROR) << StringPrintf("%s: fail notify nfc service", __func__);
  }

  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: exit", __func__);
}

static bool nfcManager_doSendRawPtmCommand(JNIEnv* e, jobject o, jbyteArray data) {
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);
  // if (data == nullptr) {
  //   DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: data is null, wait next rsp", __func__);
  //   return NFA_WaitVsResponse(nfaNciRawCommandCallback) == NFA_STATUS_OK;
  // }
  if (!nfcManager_isNfcActive()) {
    LOG(ERROR) << StringPrintf("%s: nfc is not enabled", __func__);
    return false;
  }

  ScopedByteArrayRO bytes(e, data);
  uint8_t* buf = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(&bytes[0]));
  size_t bufLen = bytes.size();
  int ret = NFA_SendRawPtmCommand(bufLen, buf);
  if (ret != NFA_STATUS_OK) {
    LOG(ERROR) << StringPrintf("%s: send raw nci data failed, ret=%d", __func__, ret);
    return false;
  }
  return true;
}

static void nfcManager_doSetPassthroughMode(JNIEnv* e, jobject o, jint mode) {
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter, mode = %d", __func__, mode);
  NFA_SetPassthroughMode(mode, (mode == 0)? nullptr : nfaPtmRawCommandCallback);
}

static jboolean nfcManager_initTmsNativeStruc(JNIEnv* e, jobject o) {
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);

  nfc_jni_native_data* nat =
      (nfc_jni_native_data*)malloc(sizeof(struct nfc_jni_native_data));
  if (nat == NULL) {
    LOG(ERROR) << StringPrintf("%s: fail allocate native data", __func__);
    return JNI_FALSE;
  }

  memset(nat, 0, sizeof(*nat));
  e->GetJavaVM(&(nat->vm));
  nat->env_version = e->GetVersion();
  nat->manager = e->NewGlobalRef(o);

  ScopedLocalRef<jclass> cls(e, e->GetObjectClass(o));
  jfieldID f = e->GetFieldID(cls.get(), "mNative", "J");
  e->SetLongField(o, f, (jlong)nat);

  sCachedNat = nat;

  /* Initialize native cached references */
  gCachedTmsNfcManagerNotifyRawPtmCommandCallback = 
      e->GetMethodID(cls.get(), "notifyRawPtmCommandCallback",
                     "(II[B)V");
  gCachedTmsNfcManagerNotifyM1RawDataAuthCallback =
        e->GetMethodID(cls.get(), "notifyM1RawDataAuthCallback",
                       "(I)V");
  gCachedTmsNfcManagerNotifyLxDebugUploadCallback =
        e->GetMethodID(cls.get(), "notifyLxDebugUpload",
                       "(I[B)V");
  gCachedTmsNfcManagerNotifyNfceeAidSelectCallback =
        e->GetMethodID(cls.get(), "notifyNfceeAidSelect",
                       "(I[B)V");
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: exit", __func__);
  return JNI_TRUE;
}

static jstring nfcManager_doGetMwVersion(JNIEnv* e, jobject o) {
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);
  char mw_version[] = MW_VERSION;
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: NCI MW Version: %s", __func__, mw_version);
  return e->NewStringUTF(mw_version);
}

static jstring nfcManager_doGetMwBuildTime(JNIEnv* e, jobject o) {
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);
  char mw_build_time[] = MW_BUILD_TIME;
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: NCI MW Build Time: %s", __func__, mw_build_time);
  return e->NewStringUTF(mw_build_time);
}
/*******************************************************************************
**
** Function:        ConvertJavaStrToStdString
**
** Description:     Convert Jstring to string
**                  e: JVM environment.
**                  o: Java object.
**                  s: Jstring.
**
** Returns:         std::string
**
*******************************************************************************/
std::string ConvertJavaStrToStdString(JNIEnv* env, jstring s) {
  if (!s) return "";

  const jclass strClass = env->GetObjectClass(s);
  const jmethodID getBytes =
      env->GetMethodID(strClass, "getBytes", "(Ljava/lang/String;)[B");
  const jbyteArray strJbytes = (jbyteArray)env->CallObjectMethod(
      s, getBytes, env->NewStringUTF("UTF-8"));

  size_t length = (size_t)env->GetArrayLength(strJbytes);
  jbyte* pBytes = env->GetByteArrayElements(strJbytes, NULL);

  std::string ret = std::string((char*)pBytes, length);
  env->ReleaseByteArrayElements(strJbytes, pBytes, JNI_ABORT);

  env->DeleteLocalRef(strJbytes);
  env->DeleteLocalRef(strClass);
  return ret;
}

/*******************************************************************************
**
** Function:        nfcManager_isNfccBusy
**
** Description:     Check If NFCC is busy
**
** Returns:         True if NFCC is busy.
**
*******************************************************************************/
static bool nfcManager_isNfccBusy(JNIEnv*, jobject) {
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: ENTER", __func__);
    bool statBusy = false;
   if (android::sSeRfActive || gActivated ) {
      LOG(ERROR) << StringPrintf("%s:FAIL  RF session ongoing", __func__);
      statBusy = true;
    }
    return statBusy;
}

static int nfcManager_setTransitConfig(JNIEnv * e, jobject o,
                                         jstring config) {
    (void)e;
    (void)o;
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);
    std::string transitConfig = ConvertJavaStrToStdString(e, config);
    SyncEventGuard guard(sNfaTransitConfigEvent);
    int stat = NFA_SetTransitConfig(transitConfig);
    if (stat != NFA_STATUS_OK) {
      LOG(ERROR) << StringPrintf("%s: NFA_SetTransitConfig failed", __func__);
    } else {
      if(sNfaTransitConfigEvent.wait(10 * ONE_SECOND_MS) == false) {
        LOG(ERROR) << StringPrintf("Nfa transitConfig Event has terminated");
      }
    }
    return stat;
}

/*******************************************************************************
**
** Function:        nfcManager_clearRoutingEntry
**
** Description:     Set the routing entry in routing table
**                  e: JVM environment.
**                  o: Java object.
**                  type:technology/protocol/aid clear routing
**
*******************************************************************************/
static void nfcManager_setEmptyAidRoute (JNIEnv*, jobject, jint route, jint power)
{
    RoutingManager::getInstance().setEmptyAidEntry(route, power);
    return;
}

static jbyteArray nfcManager_doGetManufSpecInfo(JNIEnv * e, jobject o) {
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);
    tNFC_NFCC_MANUF_SPEC_INFO infos = NFC_GetManufSpecInfo();
    ScopedLocalRef<jobject> dataJavaArray(e, e->NewByteArray(infos.len));
    CHECK(dataJavaArray.get());
    e->SetByteArrayRegion((jbyteArray)dataJavaArray.get(), 0, infos.len, (jbyte*)infos.infos);
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: exit", __func__);
    return (jbyteArray)dataJavaArray.release();
}

static void nfaM1RawDataCallback(tNFC_VS_EVT event, uint16_t data_len, uint8_t* p_data) {
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter, event=%d, len=%d", __func__, event, data_len);
    if (event != (NCI_NTF_BIT | EVT_M1_AUTH_RESULT)) {
        DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: not m1 auth ntf", __func__);
        return;
    }
    if (data_len != LENGTH_M1_RDM_NTF) {
        DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: not validate ntf", __func__);
        return;
    }
    uint8_t trigger_type = *(p_data + OFFSET_M1_RDM_TRIGGER_TYPE);
    if (trigger_type != TRIGGER_TYPE_M1_RDM) {
        DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: unknown trigger type: %d", __func__, trigger_type);
        return;
    }
    int authStatus = *(p_data + OFFSET_M1_RDM_AUTH_STATE);
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: auth_status=%d", __func__, authStatus);
    JNIEnv* e = NULL;
    ScopedAttach attach(sCachedNat->vm, &e);
    if (e == NULL) {
        LOG(ERROR) << StringPrintf("%s: jni env is null", __func__);
        return;
    }
    e->CallVoidMethod(sCachedNat->manager,
                    gCachedTmsNfcManagerNotifyM1RawDataAuthCallback,
                    authStatus);
}

static void nfcManager_doSetM1RawDataModeEnable(JNIEnv* e, jobject o, jboolean isEnabled) {
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter, isEnable = %d", __func__, isEnabled);
    gIsM1RawDataEnabled = isEnabled;
    NFC_RegVSCback(isEnabled, nfaM1RawDataCallback);
}

/*******************************************************************************
**
** Function:        nfcManager_doGetActiveSecureElementList
**
** Description:     Get NFCEEs in active state.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         None
**
*******************************************************************************/
static jintArray nfcManager_doGetActiveSecureElementList(JNIEnv* e, jobject o) {
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);
  uint8_t nfcee_num = NFA_EE_MAX_EE_SUPPORTED;
  tNFA_EE_INFO nfcee_info[NFA_EE_MAX_EE_SUPPORTED];
  int count = 0;
  jint seId = 0;
  tNFA_STATUS status = NFA_EeGetInfo(&nfcee_num, nfcee_info);
  if (status != NFA_STATUS_OK || nfcee_num < 0) {
    LOG(ERROR) << StringPrintf("%s: get ee info failed! status = %d, ee num = %d", __func__, status, nfcee_num);
    return nullptr;
  }
  jintArray list = e->NewIntArray (nfcee_num);
  for(int i = 0; i < nfcee_num; i++) {
    if (nfcee_info[i].ee_status == NFC_NFCEE_STATUS_ACTIVE) {
      seId = nfcee_info[i].ee_handle & ~NFA_HANDLE_GROUP_EE;
      switch (seId) {
        case 0xC0:
        case 0x80:
        case 0x81:
        case 0x10:
          DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: got active ee: 0x%x", __func__, seId);
          break;
        default:
          LOG(ERROR) << StringPrintf("%s: invaild ee: 0x%x", __func__, seId);
          break;
      }
    }
    e->SetIntArrayRegion (list, count++, 1, &seId);
  }
  return list;
}

/*******************************************************************************
**
** Function:        nfcManager_doGetT4TNfceePowerState
**
** Description:     Get the T4T Nfcee power state supported.
**                  e: JVM environment.
**                  o: Java object.
**
** Returns:         None
**
*******************************************************************************/
static jint nfcManager_doGetT4TNfceePowerState(JNIEnv* e, jobject o) {
  RoutingManager& routingManager = RoutingManager::getInstance();
  int defaultPowerState = ~(routingManager.PWR_SWTCH_OFF_MASK |
          routingManager.PWR_BATT_OFF_MASK);

  return NfcConfig::getUnsigned(NAME_DEFAULT_T4TNFCEE_AID_POWER_STATE,
          defaultPowerState);
}

static void nfaLxDebugCallback(tNFC_VS_EVT event, uint16_t data_len, uint8_t* p_data) {
  uint8_t oid = event & (~NCI_NTF_BIT);
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter,event=%d oid=%d, len=%d, sLxDebugEventLevel=0x%02X", __func__, event, oid, data_len, sLxDebugEventLevel);

  bool is_lxdebug_event = false;
  if ((sLxDebugEventLevel & EVT_LEVEL_L1) && oid == EVT_VALUE_L1) {
    is_lxdebug_event = true;
  } else if ((sLxDebugEventLevel & EVT_LEVEL_L2) && oid == EVT_VALUE_L2) {
    is_lxdebug_event = true;
  } else if ((sLxDebugEventLevel & EVT_LEVEL_L3) && oid == EVT_VALUE_L3) {
    is_lxdebug_event = true;
  }

  if (!is_lxdebug_event) {
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: ignore this event", __func__);
    return;
  }

  if (sCachedNat == NULL) {
    LOG(ERROR) << StringPrintf("%s: jni env is null", __func__);
    return;
  }

  JNIEnv* e = NULL;
  ScopedAttach attach(sCachedNat->vm, &e);
  if (e == NULL) {
    LOG(ERROR) << StringPrintf("%s: jni env is null", __func__);
    return;
  }
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: lx debug", __func__);
  ScopedLocalRef<jobject> dataJavaArray(e, e->NewByteArray(data_len));
  CHECK(dataJavaArray.get());
  e->SetByteArrayRegion((jbyteArray)dataJavaArray.get(), 0, data_len, (jbyte*)p_data);
  CHECK(!e->ExceptionCheck());

  e->CallVoidMethod(sCachedNat->manager,
                    gCachedTmsNfcManagerNotifyLxDebugUploadCallback,
                    oid, dataJavaArray.get());
  if (e->ExceptionCheck()) {
    e->ExceptionClear();
    LOG(ERROR) << StringPrintf("%s: fail notify nfc service", __func__);
  }

}

static void nfcManager_doSetLxDebugUploadEnabled(JNIEnv* e, jobject o, jboolean isEnabled, jint level) {
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter, isEnabled=%d, level=0x%02X, sLxDebugEventLevel=0x%02X", __func__, isEnabled, level, sLxDebugEventLevel);
  level &= EVT_LEVEL_L1 | EVT_LEVEL_L2 | EVT_LEVEL_L3;
  if (isEnabled) {
    sLxDebugEventLevel |= level & 0xFF;
  } else {
    sLxDebugEventLevel &= ~(level & 0xFF);
  }
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: sLxDebugEventLevel=0x%02X", __func__, sLxDebugEventLevel);
  if (sLxDebugEventLevel != 0) {
    NFC_RegVSCback(true, nfaLxDebugCallback);
    LOG(INFO) << StringPrintf("%s: register lxdebug callback", __func__);
  } else {
    NFC_RegVSCback(false, nfaLxDebugCallback);
    LOG(INFO) << StringPrintf("%s: unregister lxdebug callback", __func__);
  }
}

static jboolean nfcManager_setRfConfigs(JNIEnv* e, jobject o, jobjectArray configs, jboolean stopWhenFailed) {
  jint size = e->GetArrayLength(configs);
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter, stopWhenFailed=%d, config size=%d", __func__, stopWhenFailed, size);
  if (size == 0) {
    LOG(ERROR) << StringPrintf("%s: empty configs, do noting", __func__);
    return true;
  }
  bool wasDiscoveryEnabled = false;
  SyncEventGuard g(gIsReconfiguringDiscovery);
  bool isRfEnabled = isDiscoveryStarted();
  if (isRfEnabled) {
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: stop rf discovery first", __func__);
    startRfDiscovery(false);
    wasDiscoveryEnabled = true;
  }

  bool isSetConfigSuccess = false;

  for (int i = 0; i < size; i++) {
    isSetConfigSuccess = false;
    jbyteArray config = static_cast<jbyteArray>(e->GetObjectArrayElement(configs, i));
    jbyteArray respBytes = nfcManager_doSendRawNciCommand(e, o, config, false);

    if (respBytes != NULL) {
      int respLength = e->GetArrayLength(respBytes);
      uint8_t* resp = (uint8_t*)e->GetByteArrayElements(respBytes, NULL);

      isSetConfigSuccess = (respLength == 5 && resp[3] == 0);

      e->ReleaseByteArrayElements(respBytes, (jbyte*)resp, JNI_ABORT);
    }

    if (!isSetConfigSuccess) {
      LOG(ERROR) << StringPrintf("%s: set config failed at: %d", __func__, i);
      if (stopWhenFailed) {
        break;
      }
    }
  }

  if (wasDiscoveryEnabled) {
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: start rf discovery", __func__);
    startRfDiscovery(true);
  }
  return isSetConfigSuccess;
}

void onNfceeActionEvent(tNFA_EE_ACTION& action) {
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);
  if (sCachedNat == NULL) {
    LOG(ERROR) << StringPrintf("%s: jni env is null", __func__);
    return;
  }

  JNIEnv* e = NULL;
  ScopedAttach attach(sCachedNat->vm, &e);
  if (e == NULL) {
    LOG(ERROR) << StringPrintf("%s: jni env is null", __func__);
    return;
  }
  if (action.trigger == NFC_EE_TRIG_SELECT) {
    int aid_len = action.param.aid.len_aid;

    ScopedLocalRef<jobject> dataJavaArray(e, e->NewByteArray(aid_len));
    CHECK(dataJavaArray.get());
    e->SetByteArrayRegion((jbyteArray)dataJavaArray.get(), 0, aid_len, (jbyte*) &action.param.aid.aid[0]);
    CHECK(!e->ExceptionCheck());

    e->CallVoidMethod(sCachedNat->manager,
                      gCachedTmsNfcManagerNotifyNfceeAidSelectCallback,
                      action.ee_handle & 0xFF, dataJavaArray.get());
    if (e->ExceptionCheck()) {
      e->ExceptionClear();
      LOG(ERROR) << StringPrintf("%s: fail notify nfc service", __func__);
    }

  }
  DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: exit", __func__);
}

/*****************************************************************************
**
** JNI functions for TMS
**
*****************************************************************************/
static JNINativeMethod gMethods[] = {
    {"initializeTmsNativeStructure", "()Z", (void*) nfcManager_initTmsNativeStruc},
    {"doSkipTagSelect", "(I)V", (void*)nfcManager_doSkipTagSelect},
    {"doSetRfListenMask", "(IZ)V", (void*)nfcManager_doSetRfListenMask},
    {"doGetRfListenMask", "()I", (void*)nfcManager_doGetRfListenMask},
    {"setUserDefaultRoutesPref", "(IIIIII)V", (void*)tmsNfcManager_setUserDefaultRoutesPref},
    {"doSendRawNciCommand", "([BZ)[B", (void*)nfcManager_doSendRawNciCommand},
    {"doGetMwVersion", "()Ljava/lang/String;", (void*)nfcManager_doGetMwVersion},
    {"doGetMwBuildTime", "()Ljava/lang/String;", (void*)nfcManager_doGetMwBuildTime},
    {"isNfccBusy", "()Z", (void*)nfcManager_isNfccBusy},
    {"setTransitConfig", "(Ljava/lang/String;)I",
                  (void*)nfcManager_setTransitConfig},
    {"doSendRawPtmCommand", "([B)Z", (void*)nfcManager_doSendRawPtmCommand},
    {"doSetPassthroughMode", "(I)V", (void*)nfcManager_doSetPassthroughMode},
    {"setEmptyAidRoute", "(II)V", (void*)nfcManager_setEmptyAidRoute},
    {"doGetManufSpecInfo", "()[B", (void*)nfcManager_doGetManufSpecInfo},
    {"doSetM1RawDataModeEnable", "(Z)V", (void*)nfcManager_doSetM1RawDataModeEnable},
    {"doGetActiveSecureElementList", "()[I", (void*)nfcManager_doGetActiveSecureElementList},
    {"doGetT4TNfceePowerState", "()I", (void*) nfcManager_doGetT4TNfceePowerState},
    {"doSetLxDebugUploadEnabled", "(ZI)V", (void*) nfcManager_doSetLxDebugUploadEnabled},
    {"setRfConfigs", "([[BZ)Z", (void*) nfcManager_setRfConfigs},
};

int register_com_tms_nfc_TmsNativeNfcManager(JNIEnv* e) {
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);
    return jniRegisterNativeMethods(e, gTmsNativeNfcManagerClassName, gMethods,
                                      NELEM(gMethods));
}


} /* namespace tms end*/
