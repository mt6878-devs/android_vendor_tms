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

/******************************************************************************
 * Implement based on C11
 *****************************************************************************/

#if defined (USE_TMS_NFC) || defined (USE_C1)

#include <stdarg.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include "tmsCommon.h"
#include "tmsSecureString.h"
#include "tmslog.h"

static const char g_tag[] = "TmsSecureString";

errno_t memcpy_s(void *dest, rsize_t destsz, const void *src, rsize_t count)
{
    if (NULL == dest || NULL == src) {
        if (NULL != dest) {
            (void)memset_s(dest, destsz, 0, destsz);
        }
        TMS_LOG_E(g_tag, "%s: dest or src is NULL", __FUNCTION__);
        return EINVAL;
    }
    if (count > RSIZE_MAX || destsz > RSIZE_MAX) {
        (void)memset_s(dest, destsz, 0, destsz);
        TMS_LOG_E(g_tag, "%s: destsz[%zd] or count[%zd] larger than RSIZE_MAX[%zd]",
                  __FUNCTION__, destsz, count, RSIZE_MAX);
        return EOVERFLOW;
    }
    if (count > destsz) {
        (void)memset_s(dest, destsz, 0, destsz);
        TMS_LOG_E(g_tag, "%s: count[%zd] larger than destsz[%zd]",
                  __FUNCTION__, count, destsz);
        return EOVERFLOW;
    }
    if ((dest > src && ((uint8_t *)dest - (uint8_t *)src < count))
            || (dest < src && ((uint8_t *)src - (uint8_t *)dest < count))) {
        (void)memset_s(dest, destsz, 0, destsz);
        TMS_LOG_E(g_tag, "%s: memory override, count[%zd] dest[%p] src[%p]",
                  __FUNCTION__, count, dest, src);
        return EOVERFLOW;
    }

    memcpy(dest, src, count);
    return 0;
}

errno_t memset_s(void *dest, rsize_t destsz, int ch, rsize_t count)
{
    if (NULL == dest) {
        TMS_LOG_E(g_tag, "%s: dest is NULL", __FUNCTION__);
        return EINVAL;
    }
    if (count > RSIZE_MAX || destsz > RSIZE_MAX) {
        memset(dest, 0, destsz);
        TMS_LOG_E(g_tag, "%s: destsz[%zd] or count[%zd] larger than RSIZE_MAX[%zd]",
                  __FUNCTION__, destsz, count, RSIZE_MAX);
        return EOVERFLOW;
    }
    if (count > destsz) {
        memset(dest, 0, destsz);
        TMS_LOG_E(g_tag, "%s: count[%zd] larger than destsz[%zd]",
                  __FUNCTION__, count, destsz);
        return EOVERFLOW;
    }

    memset(dest, ch, count);
    return 0;
}


errno_t strcpy_s(char *dest, rsize_t destsz, const char *src)
{
    if (NULL == dest || NULL == src) {
        if (NULL != dest) {
            dest[0] = '\0';
        }
        TMS_LOG_E(g_tag, "%s: dest or src is NULL", __FUNCTION__);
        return EINVAL;
    }
    if (destsz > RSIZE_MAX || 0 == destsz) {
        if (destsz > 0) {
            dest[0] = '\0';
        }
        TMS_LOG_E(g_tag, "%s: destsz[%zd] is 0 or larger than RSIZE_MAX[%zd]",
                  __FUNCTION__, destsz, RSIZE_MAX);
        return EINVAL;
    }
    rsize_t srcsz = strnlen_s(src, destsz);
    if (destsz + 1 < srcsz) {
        dest[0] = '\0';
        TMS_LOG_E(g_tag, "%s: srcsz[%zd] larger than destsz[%zd]",
                  __FUNCTION__, srcsz, destsz);
        return EOVERFLOW;
    }
    if ((dest > src && (dest - src < srcsz - 1))
            || (dest < src && (src - dest < srcsz - 1))) {
        dest[0] = '\0';
        TMS_LOG_E(g_tag, "%s: memory override, srcsz[%zd] dest[%p] src[%p]",
                  __FUNCTION__, srcsz, dest, src);
        return EOVERFLOW;
    }
    strcpy(dest, src);
    return 0;
}

errno_t strncpy_s(char *dest, rsize_t destsz, const char *src, rsize_t count)
{
    UNUSED(destsz);
    strncpy(dest, src, count);
    return 0;
}

size_t strnlen_s(const char *src, size_t strsz)
{
    UNUSED(strsz);
    return strlen(src);
}

errno_t strcat_s(char *dest, rsize_t destsz, const char *src)
{
    UNUSED(destsz);
    strcat(dest, src);
    return 0;
}


int c11_snprintf_s(char *buffer, rsize_t bufsz, const char *format, ...)
{
    int count = -1;
    va_list ap;
    va_start(ap, format);
    count = vsnprintf(buffer, bufsz, format, ap);
    va_end(ap);
    return count;
}

int snprintf_s(char *buffer, rsize_t bufsz, rsize_t count, const char *format, ...)
{
    int cnt = -1;
    UNUSED(bufsz);
    va_list ap;
    va_start(ap, format);
    cnt = vsnprintf(buffer, count, format, ap);
    va_end(ap);
    return cnt;
}

#endif
