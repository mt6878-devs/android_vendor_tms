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

#include <android-base/stringprintf.h>
#include <base/logging.h>
#include <log/log.h>
#include <nativehelper/ScopedLocalRef.h>
#include <nativehelper/ScopedPrimitiveArray.h>
#include <nativehelper/ScopedUtfChars.h>
#include <nativehelper/JNIHelp.h>
#include "NfcJniUtil.h"
#include "nfc_config.h"
#include <string>

extern bool nfc_debug_enabled;

using android::base::StringPrintf;
//using android::NfcConfig;

namespace tms { /* namespace tms start*/

const char* gTmsNativeNfcConfigClassName =
    "com/tms/nfc/dhimpl/TmsNativeNfcConfig";

static jboolean hasKey(JNIEnv* e, jobject o, jstring key) {
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);
    if (key == NULL) {
        return false;
    }
    const char* chars = e->GetStringUTFChars(key, NULL);
    if (chars == NULL) {
        return false;
    }
    std::string keyString(chars);
    e->ReleaseStringUTFChars(key, chars);
    return NfcConfig::hasKey(keyString);
}

static jstring getString(JNIEnv* e, jobject o, jstring key, jstring defaultValue) {
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);
    if (key == NULL) {
        return defaultValue;
    }
    const char* chars = e->GetStringUTFChars(key, NULL);
    if (chars == NULL) {
        return defaultValue;
    }
    std::string keyString(chars);
    e->ReleaseStringUTFChars(key, chars);
    if (!NfcConfig::hasKey(keyString)) {
        return defaultValue;
    }
    std::string value = NfcConfig::getString(keyString);
    return e->NewStringUTF(value.c_str());
}

static jint getInt(JNIEnv* e, jobject o, jstring key, jint defaultValue) {
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);
    if (key == NULL) {
        return defaultValue;
    }
    const char* chars = e->GetStringUTFChars(key, NULL);
    if (chars == NULL) {
        return defaultValue;
    }
    std::string keyString(chars);
    e->ReleaseStringUTFChars(key, chars);
    if (!NfcConfig::hasKey(keyString)) {
        return defaultValue;
    }
    unsigned value = NfcConfig::getUnsigned(keyString);
    return value;
}

static jbyteArray getBytes(JNIEnv* e, jobject o, jstring key) {
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);
    if (key == NULL) {
        return NULL;
    }
    const char* chars = e->GetStringUTFChars(key, NULL);
    if (chars == NULL) {
        return NULL;
    }
    std::string keyString(chars);
    e->ReleaseStringUTFChars(key, chars);
    if (!NfcConfig::hasKey(keyString)) {
        return NULL;
    }
    std::vector<uint8_t> value = NfcConfig::getBytes(keyString);
    int size = value.size();
    jbyteArray jarray = e->NewByteArray(size);
    e->SetByteArrayRegion(jarray, 0, size, (const jbyte *)&value[0]);
    return jarray;
}

/*****************************************************************************
**
** JNI functions for TMS
**
*****************************************************************************/
static JNINativeMethod gMethods[] = {
    {"hasKey", "(Ljava/lang/String;)Z", (void*) hasKey},
    {"getString", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", (void*)getString},
    {"getInt", "(Ljava/lang/String;I)I", (void*)getInt},
    {"getBytes", "(Ljava/lang/String;)[B", (void*)getBytes},
};

int register_com_tms_nfc_TmsNativeNfcConfig(JNIEnv* e) {
    DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);
    return jniRegisterNativeMethods(e, gTmsNativeNfcConfigClassName, gMethods,
                                      NELEM(gMethods));
}

} /* namespace tms end*/