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

#include <stdbool.h>
#include <utils/CallStack.h>
#include "callstack.h"

bool g_dumpStackFlag = false;

void DumpCallstack(void)
{
#ifdef DBG_LEVEL_STACK
    android::CallStack cs("test");
#endif
}

bool GetDumpStackFlag(void)
{
    return g_dumpStackFlag;
}

void SetDumpStackFlag(bool flag)
{
    g_dumpStackFlag = flag;
}
