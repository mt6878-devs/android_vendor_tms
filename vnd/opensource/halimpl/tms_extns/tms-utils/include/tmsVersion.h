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

#ifndef TMS_7816_3_T1_TMS_VERSION_H
#define TMS_7816_3_T1_TMS_VERSION_H

#if defined (USE_TMS_NFC) || defined (USE_C1)
    #include "tmsDate.h"
#endif

#define VERSION_ANDROID 11
#define VERSION_MAJOR 1
#define VERSION_MINOR 0


#if VERSION_ANDROID > 10

#define VERSION_ANDROID_INIT \
    ((VERSION_ANDROID / 10) + '0'), \
    ((VERSION_ANDROID % 10) + '0')

#else

#define VERSION_ANDROID_INIT \
    '0', \
    (VERSION_ANDROID + '0')

#endif

#if VERSION_MAJOR > 100

#define VERSION_MAJOR_INIT \
    ((VERSION_MAJOR / 100) + '0'), \
    (((VERSION_MAJOR % 100) / 10) + '0'), \
    ((VERSION_MAJOR % 10) + '0')

#elif VERSION_MAJOR > 10

#define VERSION_MAJOR_INIT \
    ((VERSION_MAJOR / 10) + '0'), \
    ((VERSION_MAJOR % 10) + '0')

#else

#define VERSION_MAJOR_INIT \
    '0', \
    (VERSION_MAJOR + '0')

#endif

#if VERSION_MINOR > 100

#define VERSION_MINOR_INIT \
    ((VERSION_MINOR / 100) + '0'), \
    (((VERSION_MINOR % 100) / 10) + '0'), \
    ((VERSION_MINOR % 10) + '0')

#elif VERSION_MINOR > 10

#define VERSION_MINOR_INIT \
    ((VERSION_MINOR / 10) + '0'), \
    ((VERSION_MINOR % 10) + '0')

#else

#define VERSION_MINOR_INIT \
    '0', \
    (VERSION_MINOR + '0')

#endif

#define MW_VERSION { \
        VERSION_ANDROID_INIT, \
        '.', \
        VERSION_MAJOR_INIT, \
        '.', \
        VERSION_MINOR_INIT, \
        '\0' \
    }

#if defined (USE_TMS_NFC) || defined (USE_C1)
#define MW_BUILD_TIME { \
        BUILD_YEAR_CH2, BUILD_YEAR_CH3, \
        BUILD_MONTH_CH0, BUILD_MONTH_CH1, \
        BUILD_DAY_CH0, BUILD_DAY_CH1, \
        '.', \
        BUILD_HOUR_CH0, BUILD_HOUR_CH1, \
        BUILD_MIN_CH0, BUILD_MIN_CH1, \
        '\0' \
    }
#endif // defined (USE_TMS_NFC) || defined (USE_C1)

#endif