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

USE_TMS_NFC := true
USE_NFC_TYPE := tms
USE_VENDOR_CA := true

SOONG_CONFIG_NAMESPACES += tmsBuildCtx
SOONG_CONFIG_tmsBuildCtx += wiredSe
SOONG_CONFIG_tmsBuildCtx += nfcType
SOONG_CONFIG_tmsBuildCtx += useVerdorCa

ifneq ($(findstring $(WIRED_SE), true classic),)
SOONG_CONFIG_tmsBuildCtx_wiredSe := classic
else
SOONG_CONFIG_tmsBuildCtx_wiredSe := no
endif

ifneq ($(findstring $(USE_NFC_TYPE), tms),)
SOONG_CONFIG_tmsBuildCtx_nfcType := tms
else ifneq ($(findstring $(USE_NFC_TYPE), c1),)
SOONG_CONFIG_tmsBuildCtx_nfcType := c1
else
SOONG_CONFIG_tmsBuildCtx_nfcType := other
endif

ifeq ($(USE_VENDOR_CA), true)
SOONG_CONFIG_tmsBuildCtx_useVerdorCa := true
else
SOONG_CONFIG_tmsBuildCtx_useVerdorCa := false
endif

ifneq ($(findstring $(TMS_NFC_AIDL_HAL), true),)
SOONG_CONFIG_tmsBuildCtx_nfcAidlHal := true
else
SOONG_CONFIG_tmsBuildCtx_nfcAidlHal := false
endif

ifneq ($(findstring $(TMS_ESE_AIDL_HAL), true),)
SOONG_CONFIG_tmsBuildCtx_seAidlHal := true
else
SOONG_CONFIG_tmsBuildCtx_seAidlHal := false
endif
