/*
 * Copyright (c) Tsingteng MicroSystem Co., Ltd. 2021. All rights reserved.
 */
/******************************************************************************
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
 *****************************************************************************/

#ifndef VENDOR_TMS_ITMS_PHYSICAL_LAYER_H
#define VENDOR_TMS_ITMS_PHYSICAL_LAYER_H

#include <stdint.h>
#include "tmsCommon.h"

#include <string>
using std::string;
typedef ESESTATUS(*ExecCmdFun)(const char *str, int strLen, void *arg);

namespace vendor {
namespace tms {
class ITmsPhAbs {
  public:
    enum PhClass {
        REE = 1,
        TEE = 2,
        WIRED = 3
    };

    virtual PhClass getPhClass() = 0;
    virtual int openBasicChannel() = 0;
    virtual ESESTATUS transmit(SeData *pCmd, SeData *pRsp) = 0;
    virtual bool closeChannel(int channelNum) = 0;

    ExecCmdFun getExecCmdFun() const
    {
        return mExecCmdFun;
    }

    virtual ~ITmsPhAbs()
    {
        // Do not delete this virtual destructor.
        // Avoid sub-class destructor cannot called when delete base-class pointer.
    }

  protected:
    ExecCmdFun mExecCmdFun;
};
}  // namespace tms
}  // namespace vendor

#endif  // VENDOR_TMS_ITMS_PHYSICAL_LAYER_H
