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

#ifndef TMS_7816_3_T1_CALL_STACK_H
#define TMS_7816_3_T1_CALL_STACK_H

#ifdef __cplusplus
extern "C" {
#endif

// Only for testing [start]
extern void DumpCallstack();
// Only for testing [end]

bool GetDumpStackFlag(void);
void SetDumpStackFlag(bool flag);

#ifdef __cplusplus
}
#endif

#endif