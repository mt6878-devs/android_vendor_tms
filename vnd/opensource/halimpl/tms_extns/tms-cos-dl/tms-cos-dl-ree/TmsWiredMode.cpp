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

#ifdef TMS_WIRED_MODE

#include "TmsWiredMode.h"
#include "tmsDlCommonUtils.h"
#include "tmslog.h"

namespace vendor
{
namespace tms
{

static const char g_tag[] = "TmsCosDl:WiredMode";

int TmsWiredMode::openBasicChannel()
{
    UNUSED(g_tag);
    return -1;
}

ESESTATUS TmsWiredMode::transmit(SeData *pCmd, SeData *pRsp)
{
    UNUSED(pCmd);
    UNUSED(pRsp);
    return ESESTATUS_FAILED;
}

bool TmsWiredMode::closeChannel(int channelNum)
{
    //Ignore channelNum
    ESESTATUS status = ESESTATUS_FAILED;
    UNUSED(channelNum);

    if (ESESTATUS_SUCCESS == status) {
        return true;
    } else {
        return false;
    }
}

}  // namespace tms
}  // namespace vendor

#endif //TMS_WIRED_MODE