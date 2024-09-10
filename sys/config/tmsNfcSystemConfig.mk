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

include vendor/tms/common/config/tmsNfcGlobalConfig.mk

$(warning tmsNfcSystemConfig.mk SINGLE_CLF is $(SINGLE_CLF))

TMS_NFC_PRODUCT_HW_FEATURES := \
    frameworks/native/data/etc/android.hardware.nfc.hce.xml:system/etc/permissions/android.hardware.nfc.hce.xml \
    frameworks/native/data/etc/android.hardware.nfc.hcef.xml:system/etc/permissions/android.hardware.nfc.hcef.xml \
    frameworks/native/data/etc/android.hardware.nfc.uicc.xml:system/etc/permissions/android.hardware.nfc.uicc.xml \
    frameworks/native/data/etc/android.hardware.nfc.xml:system/etc/permissions/android.hardware.nfc.xml

TMS_ESE_PRODUCT_HW_FEATURES := \
    frameworks/native/data/etc/android.hardware.nfc.ese.xml:system/etc/permissions/android.hardware.nfc.ese.xml \
    frameworks/native/data/etc/android.hardware.se.omapi.ese.xml:system/etc/permissions/android.hardware.se.omapi.ese.xml

AOSP_NFC_CONFIG_FILES := \
    vendor/$(TMS_VENDOR_DIR)/sys/config/libnfc-nci.conf:system/etc/libnfc-nci.conf

TMS_NFC_PRODUCT_PACKAGES := \
    Tag \
    NfcNci_tms \
    com.tms.nfc \
    com.tms.nfc.xml \
    libtmsnfc-nci \
    libtmsnfc_nci_jni \
    com.tms.cosdl \
    com.tms.cosdl.xml

# app for certification only, should not distribute into product
TMS_DTA_PACKAGES := \
    TMSDTA

# app for test only, should not distribute into product
TMS_NFC_TEST_APK := \
    NfcDev \
    NfcTester \
    MifareReader \
    NfcLogCapture \
    SeBridge \
    Taginfo \
    TmsWallet \
    OmaDev \
    TmsLogView \
    TmsApi

ifeq ($(USE_TMS_NFC), true)
PRODUCT_COPY_FILES += \
    $(TMS_NFC_PRODUCT_HW_FEATURES) \
    $(AOSP_NFC_CONFIG_FILES)

PRODUCT_PACKAGES += \
    $(TMS_NFC_PRODUCT_PACKAGES) \
    $(TMS_DTA_PACKAGES) \
    $(TMS_NFC_TEST_APK)

ifneq ($(SINGLE_CLF), true)
$(warning  tmsNfcSystemConfig.mk SINGLE_CLF is false, handle eSE)
PRODUCT_COPY_FILES += \
    $(TMS_ESE_PRODUCT_HW_FEATURES)

endif
endif
