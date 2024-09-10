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

$(warning tmsNfcVendorSepolicyConfig.mk SINGLE_CLF is $(SINGLE_CLF))
TMS_NFC_SEPOLICY_DIRS := \
    vendor/$(TMS_VENDOR_DIR)/vnd/config/sepolicy \
    vendor/$(TMS_VENDOR_DIR)/vnd/config/sepolicy/nfc \
    vendor/$(TMS_VENDOR_DIR)/vnd/config/sepolicy/se


TMS_SINGLE_CLF_SEPOLICY_DIRS := \
    vendor/$(TMS_VENDOR_DIR)/vnd/config/sepolicy_no_ese \
    vendor/$(TMS_VENDOR_DIR)/vnd/config/sepolicy_no_ese/nfc

TMS_NFC_SEPOLICY_COMPAT_FOR_14 := \
    vendor/$(TMS_VENDOR_DIR)/vnd/config/sepolicy_compat/14 \
    vendor/$(TMS_VENDOR_DIR)/vnd/config/nfc/sepolicy_compat/14/se \
    vendor/$(TMS_VENDOR_DIR)/vnd/config/sepolicy_compat/14/nfc

TMS_NFC_SEPOLICY_COMPAT_FOR_13 := \
    vendor/$(TMS_VENDOR_DIR)/vnd/config/sepolicy_compat/13 \
    vendor/$(TMS_VENDOR_DIR)/vnd/config/sepolicy_compat/13/se \
    vendor/$(TMS_VENDOR_DIR)/vnd/config/sepolicy_compat/13/nfc

TMS_NFC_SEPOLICY_COMPAT_FOR_11 := \
    vendor/$(TMS_VENDOR_DIR)/vnd/config/sepolicy_compat/11 \
    vendor/$(TMS_VENDOR_DIR)/vnd/config/sepolicy_compat/11/se

TMS_NFC_SEPOLICY_COMPAT_FOR_10 := \
    vendor/$(TMS_VENDOR_DIR)/vnd/config/sepolicy_compat/10 \
    vendor/$(TMS_VENDOR_DIR)/vnd/config/sepolicy_compat/10/se

ifeq ($(USE_TMS_NFC), true)

#check if PLATFORM_VERSION is greater or equal to 14
ifeq ($(shell test $(PLATFORM_VERSION) -ge 14; echo $$?),0)
$(warning "add 14 sepolicy")
BOARD_SEPOLICY_DIRS += $(TMS_NFC_SEPOLICY_COMPAT_FOR_14)
else ifeq ($(shell test $(PLATFORM_VERSION) -ge 13; echo $$?),0)
#check if PLATFORM_VERSION is greater than or equal to 13
$(warning "add 13 sepolicy")
BOARD_SEPOLICY_DIRS += $(TMS_NFC_SEPOLICY_COMPAT_FOR_13)
else ifeq ($(shell test $(PLATFORM_VERSION) -ge 11; echo $$?),0)
$(warning "add 11 sepolicy")
BOARD_SEPOLICY_DIRS += $(TMS_NFC_SEPOLICY_COMPAT_FOR_11)
else
$(warning "add 10 sepolicy")
BOARD_SEPOLICY_DIRS += $(TMS_NFC_SEPOLICY_COMPAT_FOR_10)
endif

ifneq ($(SINGLE_CLF), true)
$(warning  tmsNfcVendorSepolicyConfig.mk Using the sepolicy)

BOARD_SEPOLICY_DIRS += $(TMS_NFC_SEPOLICY_DIRS)

else
$(warning  tmsNfcVendorSepolicyConfig.mk Using the sepolicy_no_ese )
BOARD_SEPOLICY_DIRS += $(TMS_SINGLE_CLF_SEPOLICY_DIRS)
endif

endif
