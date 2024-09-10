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

#ifdef TMS_REE

#include "TmsRee.h"
#include "tmsDlCommonUtils.h"
#include "tmsReeDlCommonUtils.h"

#include "tmslog.h"

#include "SEApi.h"

namespace vendor {
namespace tms {
static const char g_tag[] = "TmsCosDl:ReeSE";

TmsRee::TmsRee(ExecCmdFun fun)
{
    mExecCmdFun = fun;
}

TmsRee::~TmsRee()
{
}

int TmsRee::openBasicChannel()
{
    // Ignore aid
    ESESTATUS status = ESESTATUS_FAILED;
    // 1. open nfc node
    status = openT1(ESE_MODE_ESE_PTH_DL);
    if (ESESTATUS_SUCCESS != status) {
        TMS_LOG_E(g_tag, "%s: open node failed, eSE COS patch download failed",
                  __FUNCTION__);
        // open node failed, try release resource
        CloseT1();
        return -1;
    } else {
        // 2. T=1 init
        status = initT1();
        if (ESESTATUS_SUCCESS != status) {
            TMS_LOG_E(g_tag, "%s: initT1 failed, eSE COS patch download failed",
                      __FUNCTION__);
        }
    }

    if (ESESTATUS_SUCCESS == status) {
        return 0;
    } else {
        // open node failed, try release resource
        CloseT1();
        return -1;
    }
}

ESESTATUS TmsRee::transmit(SeData *pCmd, SeData *pRsp)
{
    return seTransceive(pCmd, pRsp);
}

bool TmsRee::closeChannel(int channelNum)
{
    // Ignore channelNum
    UNUSED(channelNum);
    ESESTATUS status = ESESTATUS_FAILED;
    // 1. deInitT1
    status = deInitT1();
    if (ESESTATUS_SUCCESS != status) {
        TMS_LOG_E(g_tag, "%s: deInitT1 failed", __FUNCTION__);
    }

    // 2. CloseT1
    if (!CloseT1()) {
        TMS_LOG_E(g_tag, "%s: close node failed", __FUNCTION__);
        status = ESESTATUS_FAILED;
    }

    if (ESESTATUS_SUCCESS == status) {
        return true;
    } else {
        return false;
    }
}

ESESTATUS TmsRee::reeSEReset()
{
    return seReset();
}

bool TmsRee::ioctl(long arg)
{
    return IoctlNfc(arg);
}

void TmsRee::t1ReadTerminate()
{
    DoReadTerminate();
}
}  // namespace tms
}  // namespace vendor

#endif // TMS_REE
