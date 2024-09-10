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

#ifndef _TMS_SECURE_STRING_H_
#define _TMS_SECURE_STRING_H_

#if defined (USE_TMS_NFC) || defined (USE_C1)

#include <string.h>
#include <limits.h>

#ifndef UNUSED
    #define UNUSED(arg) (void)(arg)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef int errno_t;
//size_t has been defined in stddef.h and by include in string.h
//typedef unsigned int size_t;
typedef size_t rsize_t;
#define RSIZE_MAX SIZE_T_MAX

errno_t memcpy_s(void *dest, rsize_t destsz, const void *src, rsize_t count);
errno_t memset_s(void *dest, rsize_t destsz, int ch, rsize_t count);

errno_t strcpy_s(char *dest, rsize_t destsz, const char *src);
errno_t strncpy_s(char *dest, rsize_t destsz, const char *src, rsize_t count);
size_t strnlen_s(const char *src, size_t strsz);
errno_t strcat_s(char *dest, rsize_t destsz, const char *src);

//C11 spec
int c11_snprintf_s(char *buffer, rsize_t bufsz, const char *format, ...);

//Owner1 spec
//snprintf_s(&print_buffer[i*2], max_len, 3, "%02X", pData[i]);
int snprintf_s(char *buffer, rsize_t bufsz, rsize_t count, const char *format, ...);
#ifdef __cplusplus
};
#endif

#endif //defined (USE_TMS_NFC) || defined (USE_C1)
#endif //_TMS_SECURE_STRING_H_
