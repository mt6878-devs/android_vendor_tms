# Copyright (C) 2022 Tsingteng MicroSystem
#
# All rights are reserved. Reproduction in whole or in part is
# prohibited without the written consent of the copyright owner.
#
# Tsingteng reserves the right to make changes without notice at any time.
#
# Tsingteng makes no warranty, expressed, implied or statutory, including but
# not limited to any implied warranty of merchantability or fitness for any
# particular purpose, or that the use will not infringe any third party patent,
# copyright or trademark. Tsingteng must not be liable for any loss or damage
# arising from its use.

TMS_NFC_HOST := $(TARGET_PRODUCT)
TMS_NFC_HW := thn31
TMS_NFC_PLATFORM := thn31
TMS_VENDOR_DIR := tms
USE_TMS_NFC := true
USE_NFC_TYPE := tms
USE_VENDOR_CA := true
SINGLE_CLF := true
#WIRED_SE := classic
#TMS_NFC_AIDL_HAL := true
#TMS_ESE_AIDL_HAL := true

SOONG_CONFIG_NAMESPACES += tmsBuildCtx
SOONG_CONFIG_tmsBuildCtx += wiredSe
SOONG_CONFIG_tmsBuildCtx += nfcType
SOONG_CONFIG_tmsBuildCtx += useVerdorCa
#SOONG_CONFIG_tmsBuildCtx += nfcAidlHal
#SOONG_CONFIG_tmsBuildCtx += seAidlHal

ifneq ($(findstring $(WIRED_SE), true classic),)
$(warning WIRED_SE is $(WIRED_SE), setting tmsBuildCtx.wiredSe classic)
SOONG_CONFIG_tmsBuildCtx_wiredSe := classic
else
SOONG_CONFIG_tmsBuildCtx_wiredSe := no
endif

$(warning USE_NFC_TYPE is $(USE_NFC_TYPE), setting tmsBuildCtx.nfcType)
ifneq ($(findstring $(USE_NFC_TYPE), tms),)
SOONG_CONFIG_tmsBuildCtx_nfcType := tms
else ifneq ($(findstring $(USE_NFC_TYPE), c1),)
SOONG_CONFIG_tmsBuildCtx_nfcType := c1
else
SOONG_CONFIG_tmsBuildCtx_nfcType := other
endif

ifeq ($(USE_VENDOR_CA), true)
$(warning USE_VENDOR_CA is true, setting tmsBuildCtx.useVerdorCa true)
SOONG_CONFIG_tmsBuildCtx_useVerdorCa := true
else
SOONG_CONFIG_tmsBuildCtx_useVerdorCa := false
endif

ifneq ($(findstring $(TMS_NFC_AIDL_HAL), true),)
$(warning NFC_AIDL_HAL is $(TMS_NFC_AIDL_HAL), setting tmsBuildCtx.nfcAidlHal true)
SOONG_CONFIG_tmsBuildCtx_nfcAidlHal := true
else
SOONG_CONFIG_tmsBuildCtx_nfcAidlHal := false
endif

ifneq ($(findstring $(TMS_ESE_AIDL_HAL), true),)
$(warning ESE_AIDL_HAL is $(TMS_ESE_AIDL_HAL), setting tmsBuildCtx.seAidlHal true)
SOONG_CONFIG_tmsBuildCtx_seAidlHal := true
else
SOONG_CONFIG_tmsBuildCtx_seAidlHal := false
endif

