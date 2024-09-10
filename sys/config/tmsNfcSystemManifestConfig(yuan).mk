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


ifeq ($(TMS_NFC_AIDL_HAL), true)
$(warning TMS CLF AIDL )
TMS_NFC_FRAMEWORK_MATRIX_FILES += vendor/$(TMS_VENDOR_DIR)/sys/config/fcm_nfc_aidl.xml
else
$(warning TMS CLF HIDL )
TMS_NFC_FRAMEWORK_MATRIX_FILES += vendor/$(TMS_VENDOR_DIR)/sys/config/fcm_nfc_hidl.xml
endif


ifeq ($(findstring $(SINGLE_CLF), flase),)
ifeq ($(TMS_ESE_AIDL_HAL), true)
$(warning TMS ESE AIDL)
TMS_NFC_FRAMEWORK_MATRIX_FILES += vendor/$(TMS_VENDOR_DIR)/sys/config/fcm_ese_aidl.xml
else
$(warning TMS ESE HIDL)
TMS_NFC_FRAMEWORK_MATRIX_FILES += vendor/$(TMS_VENDOR_DIR)/sys/config/fcm_ese_hidl.xml
endif
endif



DEVICE_PRODUCT_COMPATIBILITY_MATRIX_FILE += $(TMS_NFC_FRAMEWORK_MATRIX_FILES)
