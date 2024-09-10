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

#ifndef VENDOR_TMS_TMS_REE_H
#define VENDOR_TMS_TMS_REE_H

#ifdef TMS_REE

#include <stdint.h>
#include "ITmsPhAbs.h"

namespace vendor {
namespace tms {
class TmsRee : public ITmsPhAbs {
public:
    TmsRee(ExecCmdFun fun);
    ~TmsRee();
    PhClass getPhClass() override
    {
        return REE;
    }

    int openBasicChannel() override;
    ESESTATUS transmit(SeData *pCmd, SeData *pRsp) override;
    bool closeChannel(int channelNum) override;

    ESESTATUS reeSEReset();
    bool ioctl(long arg);
    void t1ReadTerminate();
};
}  // namespace tms
}  // namespace vendor

#endif // TMS_REE
#endif // VENDOR_TMS_TMS_REE_H

