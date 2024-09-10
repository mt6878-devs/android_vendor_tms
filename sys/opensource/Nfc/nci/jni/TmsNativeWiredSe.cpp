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

#include "TmsSecureElement.h"
#include "nfa_ee_api.h"

using android::base::StringPrintf;
extern bool nfc_debug_enabled;

namespace tms {
    static const char* gTmsNativeWiredSeClassName =
        "com/tms/nfc/dhimpl/TmsNativeWiredSe";

    static const tNFA_HANDLE ee_handle_eSE = 0x4C0;
    static const uint8_t host_id_eSE = 0xC0;

    static jint nativeWiredSe_doNativeOpenWiredSeConnection(JNIEnv * e, jobject o) {
        bool status = false;
        DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);

        // power on eSE
        status = SecureElement::getInstance().setPowerAndLinkState(ee_handle_eSE, SecureElement::POWER_AND_LINK_ALWAYS_ON);
        if (!status) {
            LOG(ERROR) << StringPrintf("%s: set eSE PL failed", __func__);
            goto exit;
        }

        // send apdu abort to create wired SE connection
        status = SecureElement::getInstance().abortApdu(ee_handle_eSE, host_id_eSE);
        if (!status) {
            LOG(ERROR) << StringPrintf("%s: send apdu abort failed. ret:%d", __func__, status);
            goto exit;
        }
exit:
        return status;
    }

    static void nativeWiredSe_doNativeCloseWiredSeConnection(JNIEnv * e, jobject o) {
        bool status = false;
        DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);
        status = SecureElement::getInstance().setPowerAndLinkState(ee_handle_eSE, SecureElement::POWER_ALWAYS_ON);
        if (!status) {
            LOG(ERROR) << StringPrintf("%s: set power and link failed.", __func__);
        }

        status = SecureElement::getInstance().endApdu(ee_handle_eSE, host_id_eSE);
        if (!status) {
            LOG(ERROR) << StringPrintf("%s: end of apdu transfer failed.", __func__);
        }
    }

    static jbyteArray nativeWiredSe_doNativeWiredSeTransceive(JNIEnv * e, jobject o, jbyteArray cApdu) {
        DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);
        jbyte *capdu_byte = (jbyte *)e->GetByteArrayElements(cApdu, 0);
        jsize capdu_len = e->GetArrayLength(cApdu);
        std::vector<uint8_t> capdu_vec(capdu_byte, capdu_byte + capdu_len);

        std::vector<uint8_t> rapdu_vec = SecureElement::getInstance().sendApdu(ee_handle_eSE, host_id_eSE, capdu_vec);

        jbyteArray result = e->NewByteArray(rapdu_vec.size());
        if ((nullptr != result) && (rapdu_vec.size() > 0)) {
            e->SetByteArrayRegion(result, 0, rapdu_vec.size(), (jbyte *)&rapdu_vec[0]);
        }
        return result;
    }

    static jbyteArray nativeWiredSe_doNativeWiredSeGetAtr(JNIEnv * e, jobject o) {
        DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);
        std::vector<uint8_t> atr = SecureElement::getInstance().getAtr(host_id_eSE);
        jbyteArray result = e->NewByteArray(atr.size());
        if ((nullptr != result) && (atr.size() > 0)) {
            e->SetByteArrayRegion(result, 0, atr.size(), (jbyte *)&atr[0]);
        }
        return result;
    }

    static JNINativeMethod gMethods[] = {
        {"doNativeOpenWiredSeConnection", "()I", (void*) nativeWiredSe_doNativeOpenWiredSeConnection},
        {"doNativeCloseWiredSeConnection", "()V", (void*) nativeWiredSe_doNativeCloseWiredSeConnection},
        {"doNativeWiredSeTransceive", "([B)[B", (void*) nativeWiredSe_doNativeWiredSeTransceive},
        {"doNativeWiredSeGetAtr", "()[B", (void*) nativeWiredSe_doNativeWiredSeGetAtr},
    };

    int register_com_tms_nfc_TmsNativeWiredSe(JNIEnv* e) {
        DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s: enter", __func__);
        return jniRegisterNativeMethods(e, gTmsNativeWiredSeClassName, gMethods,
                                          NELEM(gMethods));
    }

} // namespace tms

