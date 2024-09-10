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
#pragma once
#include "SyncEvent.h"
#include <map>
#include <vector>
#include "nfa_api.h"
#include "nfa_ee_api.h"
#include "nfa_hci_api.h"

namespace tms {
class SecureElement {
public:
    // power and link config
    static const uint8_t NFCC_DECIDES = 0x00;
    static const uint8_t POWER_ALWAYS_ON = 0x01;
    static const uint8_t LINK_ALWAYS_ACTIVE = 0x02;
    static const uint8_t POWER_AND_LINK_ALWAYS_ON = 0x03;

    // mode set mode
    static const uint8_t NFCEE_OFF = 0x00;
    static const uint8_t NFCEE_ON = 0x01;

    static SecureElement& getInstance();
    void initialize();
    void deinitialize();
    bool modeSet(tNFA_HANDLE ee_handle, uint8_t mode);
    bool setPowerAndLinkState(tNFA_HANDLE ee_handle, uint8_t state);
    bool abortApdu(tNFA_HANDLE ee_handle, uint8_t host_id);
    std::vector<uint8_t> sendApdu(tNFA_HANDLE ee_handle, uint8_t host_id, const std::vector<uint8_t> &c_apdu);
    std::vector<uint8_t> getAtr(uint8_t host_id);
    bool endApdu(tNFA_HANDLE ee_handle, uint8_t host_id);
    void abortWaits();

    // for unknown reason, directly register call to nfc stack will cause crash
    // so call this callback in RoutingManager::nfaEeCallback instead
    static void nfaEeCallback(tNFA_EE_EVT event, tNFA_EE_CBACK_DATA* eventData);
private:
    uint32_t maxApduWaitTime;

    bool eeModeSetStatus;
    bool eeSetPwrAndLinkStatus;
    std::map<uint8_t, std::vector<uint8_t>> atrs;
    std::vector<uint8_t> *rsp_buf;

    SyncEvent mEeSetModeEvent;
    SyncEvent mEePwrAndLinkEvent;
    SyncEvent mHciRegisterEvent;
    SyncEvent mHciDeregisterEvent;
    SyncEvent mHciAbortApduEvent;
    SyncEvent mHciRcvApduEvent;

    static void nfaHciCallback(tNFA_HCI_EVT event, tNFA_HCI_EVT_DATA* eventData);
};
} // namespace tms
