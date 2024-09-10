/*
 * Copyright (c) Tsingteng MicroSystem Co., Ltd. 2021. All rights reserved.
 */
/*
 * Copyright (c) 2021-2022 Tsingteng MicroSystem
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

#ifndef VENDOR_TMS_TMS_TEE_H
#define VENDOR_TMS_TMS_TEE_H

#ifdef TMS_TEE

#include <stdint.h>
#include "ITmsPhAbs.h"

#define PROP_AID "544D43524F4F5401"

#ifdef TMS_TEE_CA_TA
#define FUN_OPEN_BASIC_CHANNEL "openBasicChannel"
#define FUN_TRANSMIT "transmit"
#define FUN_CLOSE_CHANNEL "closeChannel"
typedef int (*TeeTransmit)(uint8_t *pApduCmd, uint16_t apduCmdLen,
                           uint8_t **ppApduRsp, uint16_t *pApduRspLen);
typedef int (*TeeCloseChannel)(const int channelNum);

#else

typedef int (*TeeTransmit)(uint8_t *pApduCmd, uint16_t apduCmdLen,
                           uint8_t *pApduRsp, uint16_t *pApduRspLen);
typedef int (*TeeCloseChannel)();
#endif

typedef int (*TeeOpenBasicChannel)(uint8_t *pAid, uint16_t aidLen);

namespace vendor
{
namespace tms
{

class TmsTee : public ITmsPhAbs
{
public:
    TmsTee(ExecCmdFun fun);
    PhClass getPhClass() override
    {
        return TEE;
    }
    int openBasicChannel() override;
    ESESTATUS transmit(SeData *pCmd, SeData *pRsp) override;
    bool closeChannel(int channelNum) override;
};

}  // namespace tms
}  // namespace vendor

#endif //TMS_TEE
#endif //VENDOR_TMS_TMS_TEE_H

