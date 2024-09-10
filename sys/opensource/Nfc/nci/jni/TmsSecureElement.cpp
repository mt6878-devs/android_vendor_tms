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

#include "TmsSecureElement.h"
#include <android-base/stringprintf.h>
#include <base/logging.h>
#include <log/log.h>
#include <nativehelper/ScopedLocalRef.h>
#include <nativehelper/ScopedPrimitiveArray.h>
#include <nativehelper/ScopedUtfChars.h>
#include <nativehelper/JNIHelp.h>
#include "NfcJniUtil.h"
#include "nfc_config.h"

#define DEFAULT_MAX_APDU_WAIT_TIME (30000)
#define APP_NAME "TmsSE"
using android::base::StringPrintf;
extern bool nfc_debug_enabled;

namespace tms {

SecureElement& SecureElement::getInstance() {
    static SecureElement mSecureElement;
    return mSecureElement;
}

void SecureElement::initialize() {
    static const char fn[] = "SecureElement::initialize";

    maxApduWaitTime = NfcConfig::getUnsigned(NAME_TMS_MAX_APDU_WAIT_TIME, DEFAULT_MAX_APDU_WAIT_TIME);
    DLOG_IF(INFO, nfc_debug_enabled) << fn << ": maxApduWaitTime:" << maxApduWaitTime;
    rsp_buf = nullptr;

    DLOG_IF(INFO, nfc_debug_enabled) << fn << " enter";
    {
        SyncEventGuard guard(mHciRegisterEvent);
        DLOG_IF(INFO, nfc_debug_enabled) << fn << ": hci register";
        tNFA_STATUS nfaStat = NFA_HciRegister(const_cast<char *>(APP_NAME), nfaHciCallback, true);
        if (nfaStat != NFA_STATUS_OK) {
          LOG(ERROR) << fn << StringPrintf(": hci register failed. error=0x%X", nfaStat);
          return;
        }
        mHciRegisterEvent.wait();
    }
}

void SecureElement::deinitialize() {
    static const char fn[] = "SecureElement::deinitialize";

    DLOG_IF(INFO, nfc_debug_enabled) << fn << " enter";
    {
        SyncEventGuard guard(mHciDeregisterEvent);
        DLOG_IF(INFO, nfc_debug_enabled) << fn << ": hci deregister";
        tNFA_STATUS nfaStat = NFA_HciDeregister(const_cast<char *>(APP_NAME));
        if (nfaStat != NFA_STATUS_OK) {
          LOG(ERROR) << fn << StringPrintf(": hci deregister failed. error=0x%X", nfaStat);
          return;
        }
        mHciDeregisterEvent.wait();
    }
    atrs.clear();
    rsp_buf = nullptr;
}

bool SecureElement::modeSet(tNFA_HANDLE ee_handle, uint8_t mode) {
    static const char fn[] = "SecureElement::modeSet";
    tNFA_STATUS status = NFA_STATUS_FAILED;

    DLOG_IF(INFO, nfc_debug_enabled) << fn << StringPrintf(": enter. mode:%d", mode);
    SyncEventGuard guard(mEeSetModeEvent);
    eeModeSetStatus = false;
    status = NFA_EeModeSet(ee_handle, mode);
    if (NFA_STATUS_OK == status) {
        mEeSetModeEvent.wait();
    }
    DLOG_IF(INFO, nfc_debug_enabled) << fn << StringPrintf(": exit. status:%d modeset status:%d",
        status, eeModeSetStatus);
    return eeModeSetStatus;
}

bool SecureElement::setPowerAndLinkState(tNFA_HANDLE ee_handle, uint8_t state) {
    static const char fn[] = "SecureElement::setPowerAndLinkState";
    tNFA_STATUS status = NFA_STATUS_FAILED;

    DLOG_IF(INFO, nfc_debug_enabled) << fn << StringPrintf(": enter. state:%d", state);
    SyncEventGuard guard(mEePwrAndLinkEvent);
    eeSetPwrAndLinkStatus = false;
    status = NFA_EePowerAndLinkCtrl(ee_handle, state);
    if (NFA_STATUS_OK == status) {
        mEePwrAndLinkEvent.wait();
    }
    DLOG_IF(INFO, nfc_debug_enabled) << fn << StringPrintf(": exit. status:%d PL status:%d",
        status, eeSetPwrAndLinkStatus);
    return eeSetPwrAndLinkStatus;
}

bool SecureElement::abortApdu(tNFA_HANDLE ee_handle, uint8_t host_id) {
    static const char fn[] = "SecureElement::abortApdu";
    tNFA_STATUS status = NFA_STATUS_FAILED;

    DLOG_IF(INFO, nfc_debug_enabled) << fn << StringPrintf(": enter. ee_handle:%04X host_id:%02X",
        ee_handle, host_id);
    SyncEventGuard guard(mHciAbortApduEvent);
    status = NFA_HciAbortApdu(ee_handle, host_id, maxApduWaitTime);
    if (NFA_STATUS_OK == status) {
        mHciAbortApduEvent.wait();
    }
    DLOG_IF(INFO, nfc_debug_enabled) << fn << StringPrintf(": exit. status:%d atrEmpty:%d", status, atrs.empty());
    return ((NFA_STATUS_OK == status) && !atrs.empty());
}

std::vector<uint8_t> SecureElement::sendApdu(tNFA_HANDLE ee_handle, uint8_t host_id, const std::vector<uint8_t> &c_apdu) {
    static const char fn[] = "SecureElement::sendApdu";
    tNFA_STATUS status = NFA_STATUS_FAILED;
    std::vector<uint8_t> result;

    DLOG_IF(INFO, nfc_debug_enabled) << fn << StringPrintf(": enter. ee_handle:%04X host_id:%02X len:%d",
        ee_handle, host_id, static_cast<int>(c_apdu.size()));

    SyncEventGuard guard(mHciRcvApduEvent);
    rsp_buf = &result;
    status = NFA_HciSendApdu(ee_handle, host_id, (uint8_t *)&c_apdu[0], c_apdu.size(), maxApduWaitTime);
    if (NFA_STATUS_OK == status) {
        mHciRcvApduEvent.wait();
    }
    rsp_buf = nullptr;
    DLOG_IF(INFO, nfc_debug_enabled) << fn << StringPrintf(": exit status:%d", status);
    return result;
}

std::vector<uint8_t> SecureElement::getAtr(uint8_t host_id) {
    for (auto &it : atrs) {
        if (it.first == host_id) {
            return it.second;
        }
    }

    LOG(ERROR) << "no atr of " << host_id;
    std::vector<uint8_t> v;
    return v;
}

bool SecureElement::endApdu(tNFA_HANDLE ee_handle, uint8_t host_id) {
    static const char fn[] = "SecureElement::endApdu";
    tNFA_STATUS status = NFA_STATUS_FAILED;

    DLOG_IF(INFO, nfc_debug_enabled) << fn << StringPrintf(": enter. ee_handle:%04X host_id:%02X",
        ee_handle, host_id);

    status = NFA_HciEndApdu(ee_handle, host_id);
    DLOG_IF(INFO, nfc_debug_enabled) << fn << StringPrintf(": exit status:%d", status);
    return (NFA_STATUS_OK == status);
}

void SecureElement::nfaEeCallback(tNFA_EE_EVT event, tNFA_EE_CBACK_DATA* eventData) {
    static const char fn[] = "SecureElement::nfaEeCallback";

    SecureElement& secureElement = SecureElement::getInstance();

    DLOG_IF(INFO, nfc_debug_enabled) << fn << StringPrintf(": event:%d", event);

    if (nullptr == eventData) {
        LOG(ERROR) << fn << ": empty eventData";
        return;
    }
    switch (event) {
        case NFA_EE_MODE_SET_EVT: {
            SyncEventGuard guard(secureElement.mEeSetModeEvent);
            secureElement.eeModeSetStatus = (eventData->mode_set.status == NFA_STATUS_OK);
            secureElement.mEeSetModeEvent.notifyOne();
            } break;

        case NFA_EE_PWR_AND_LINK_CTRL_EVT: {
            SyncEventGuard guard(secureElement.mEePwrAndLinkEvent);
            secureElement.eeSetPwrAndLinkStatus = (eventData->status == NFA_STATUS_OK);
            secureElement.mEePwrAndLinkEvent.notifyOne();
            } break;
    }
}

void SecureElement::nfaHciCallback(tNFA_HCI_EVT event, tNFA_HCI_EVT_DATA* eventData) {
    static const char fn[] = "SecureElement::nfaHciCallback";

    SecureElement& secureElement = SecureElement::getInstance();

    DLOG_IF(INFO, nfc_debug_enabled) << fn << StringPrintf(": event:%d", event);

    if (nullptr == eventData) {
        LOG(ERROR) << fn << ": empty eventData";
        return;
    }

    switch (event) {
        case NFA_HCI_REGISTER_EVT: {
            SyncEventGuard guard(secureElement.mHciRegisterEvent);
            secureElement.mHciRegisterEvent.notifyOne();
            } break;

        case NFA_HCI_DEREGISTER_EVT: {
            SyncEventGuard guard(secureElement.mHciDeregisterEvent);
            secureElement.mHciDeregisterEvent.notifyOne();
            } break;

        case NFA_HCI_ABORT_APDU_EVT: {
            SyncEventGuard guard(secureElement.mHciAbortApduEvent);
            if (NFA_STATUS_OK == eventData->abort_apdu.status) {
                std::vector<uint8_t> atr;
                atr.insert(atr.end(), eventData->abort_apdu.p_atr,
                    eventData->abort_apdu.p_atr + eventData->abort_apdu.atr_len);
                secureElement.atrs[eventData->abort_apdu.host_id] = atr;
            } else {
                secureElement.atrs.clear();
            }
            secureElement.mHciAbortApduEvent.notifyOne();
            } break;

        case NFA_HCI_R_APDU_EVT: {
            SyncEventGuard guard(secureElement.mHciRcvApduEvent);
            if (NFA_STATUS_OK == eventData->rcv_apdu.status) {
                // copy out response
                if ((nullptr != secureElement.rsp_buf) && (nullptr != eventData->rcv_apdu.p_rsp)) {
                    secureElement.rsp_buf->resize(eventData->rcv_apdu.rsp_len);
                    memcpy(secureElement.rsp_buf->data(), eventData->rcv_apdu.p_rsp, eventData->rcv_apdu.rsp_len);
                }
            } else {
                if (nullptr != secureElement.rsp_buf) {
                    secureElement.rsp_buf->clear();
                }
            }
            secureElement.mHciRcvApduEvent.notifyOne();
            } break;
    }
}

void SecureElement::abortWaits() {
   DLOG_IF(INFO, nfc_debug_enabled) << StringPrintf("%s", __func__);
   {
     SyncEventGuard guard(mHciRegisterEvent);
     mHciRegisterEvent.notifyOne();
   }
   {
     SyncEventGuard guard(mHciDeregisterEvent);
     mHciDeregisterEvent.notifyOne();
   }
   {
     SyncEventGuard guard(mEeSetModeEvent);
     mEeSetModeEvent.notifyOne();
   }
   {
     SyncEventGuard guard(mEePwrAndLinkEvent);
     mEePwrAndLinkEvent.notifyOne();
   }
   {
     SyncEventGuard guard(mHciAbortApduEvent);
     mHciAbortApduEvent.notifyOne();
   }
   {
     SyncEventGuard guard(mHciRcvApduEvent);
     mHciRcvApduEvent.notifyOne();
   }
}

} // namespace tms
