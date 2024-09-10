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

#ifndef TMS_VERSION_H
#define TMS_VERSION_H

#include "tms_date.h"

#define MW_VERSION "13.23Q3.00"

#define MW_BUILD_TIME {\
    BUILD_YEAR_CH2, BUILD_YEAR_CH3,\
    BUILD_MONTH_CH0, BUILD_MONTH_CH1,\
    BUILD_DAY_CH0, BUILD_DAY_CH1,\
    '.',\
    BUILD_HOUR_CH0, BUILD_HOUR_CH1,\
    BUILD_MIN_CH0, BUILD_MIN_CH1,\
    '\0'\
}
#endif
