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

$(warning tmsNfcVendorConfig.mk SINGLE_CLF is $(SINGLE_CLF))

#	Need to add NFC FW to CONFIG
#THN31FWC1U := true
THN31FGB1N := true
#THN31SWC2U := true

TMS_NFC_PRODUCT_PACKAGES := \
    nfc_nci.tms \
    tms-utils \
    7816-3-T1 \
    tms-cos-dl-common \
    tms-cos-dl-ree 


ifeq ($(shell test $(PLATFORM_VERSION) -ge 13; echo $$?),0)
ifeq ($(TMS_NFC_AIDL_HAL), true)
$(warning "add aidl nfc service on aosp 13 and later")
TMS_NFC_PRODUCT_PACKAGES += android.hardware.nfc-service-tms
else
$(warning "add hidl nfc service on aosp 13 and later")
TMS_NFC_PRODUCT_PACKAGES += android.hardware.nfc@1.2-service-tms
endif
else
$(warning "add hidl nfc service")
TMS_NFC_PRODUCT_PACKAGES += android.hardware.nfc@1.2-service-tms
endif


ifeq ($(shell test $(PLATFORM_VERSION) -ge 14; echo $$?),0)
ifeq ($(TMS_ESE_AIDL_HAL), true)
$(warning "add aidl ese service on aosp 14 and later")
TMS_ESE_PRODUCT_PACKAGES += android.hardware.secure_element-service-tms
else
$(warning "add hidl ese service on aosp 14 and later")
TMS_ESE_PRODUCT_PACKAGES += android.hardware.secure_element@1.2-service-tms
endif
else
$(warning "add hidl ese service")
TMS_ESE_PRODUCT_PACKAGES += android.hardware.secure_element@1.2-service-tms
endif

ifeq ($(THN31FWC1U), true)
$(warning THN31FWC1U is true)
PRODUCT_COPY_FILES += \
    vendor/$(TMS_VENDOR_DIR)/vnd/config/hw/thn31/sample/libnfc-tms_RF.conf:vendor/etc/libnfc-tms_RF.conf \
    vendor/tms/vnd/config/hw/nfc_fw/THN31_FW_C2_15_2E.hex_0xC0.bin:vendor/etc/SEC_THN31_FW_VTP.txt.bin
endif

ifeq ($(THN31FGB1N), true)
$(warning THN31FGB1N is true)
PRODUCT_COPY_FILES += \
    vendor/$(TMS_VENDOR_DIR)/vnd/config/hw/thn31/sample/libnfc-tms_RF_GB1.conf:vendor/etc/libnfc-tms_RF_GB1.conf \
	vendor/tms/vnd/config/hw/nfc_fw/THN31_FW_D2_12_2C_CRC-20240524.ts.bin:vendor/etc/SEC_THN31_FW_VTP.txt.bin
endif

ifeq ($(THN31SWC2U), true)
$(warning THN31SWC2U is true)
PRODUCT_COPY_FILES += \
    vendor/$(TMS_VENDOR_DIR)/vnd/config/hw/thn31/sample/libnfc-tms_RF_EC2.conf:vendor/etc/libnfc-tms_RF_EC2.conf \
	vendor/tms/vnd/config/hw/nfc_fw/FW_D1_15.bin:vendor/etc/SEC_THN31_FW_VTP.txt.bin
endif

TMS_NFC_CONFIG_FILES := \
    vendor/$(TMS_VENDOR_DIR)/vnd/config/hw/thn31/sample/libnfc-tms.conf:vendor/etc/libnfc-tms.conf \
    vendor/$(TMS_VENDOR_DIR)/vnd/config/hw/thn31/sample/libese-tms.conf:vendor/etc/libese-tms.conf \
    vendor/$(TMS_VENDOR_DIR)/vnd/config/hw/thn31/sample/hal_uuid_map_config.xml:vendor/etc/hal_uuid_map_config.xml 


TMS_NFC_INIT_FILE := \
     vendor/$(TMS_VENDOR_DIR)/vnd/config/hw/thn31/sample/init.$(TMS_NFC_PLATFORM).nfc.rc:vendor/etc/init/init.$(TMS_NFC_HOST).nfc.rc \

TMS_ESE_INIT_FILE := \
    vendor/$(TMS_VENDOR_DIR)/vnd/config/hw/thn31/sample/init.$(TMS_NFC_PLATFORM).se.rc:vendor/etc/init/init.$(TMS_NFC_HOST).se.rc \

#If this project need apply CUP card to eSE, must set the flowing property and ensure that it's correct.
#The flowing is only an example, not correct.
#UNION_PAY_PROP := ro.product.cuptsm=TSINGTENG|ESE|01|27

ifeq ($(USE_TMS_NFC), true)
PRODUCT_COPY_FILES += \
    $(TMS_NFC_CONFIG_FILES) \
    $(TMS_NFC_INIT_FILE) \
    $(TMS_ESE_INIT_FILE)

PRODUCT_PACKAGES += \
    $(TMS_NFC_PRODUCT_PACKAGES)


ifneq ($(SINGLE_CLF), true)
$(warning  tmsNfcVendorConfig.mk SINGLE_CLF is false, handle eSE)
PRODUCT_PACKAGES += \
    $(TMS_ESE_PRODUCT_PACKAGES) \
    $(TMS_WEAVER_PRODUCT_PACKAGES) \
    $(TMS_STRONGBOX_PRODUCT_PACKAGES)

PRODUCT_PROPERTY_OVERRIDES += \
    $(UNION_PAY_PROP)
endif

endif
