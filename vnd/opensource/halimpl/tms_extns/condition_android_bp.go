/*
 * Copyright (C) 2023 Tsingteng MicroSystem
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

package condition_android_bp

import (
        "android/soong/android"
        "android/soong/cc"
        "fmt"
        "strconv"
)

const TYPE_TMS = "tms"
const TYPE_C1 = "c1"

type props struct {
    Cflags []string
    Include_dirs []string
    Shared_libs []string
    Srcs []string
}

func init() {
    android.RegisterModuleType("cc_cond_se_bin", condSEBinFactory)
    android.RegisterModuleType("cc_cond_7816_lib", cond7816LibFactory)
    android.RegisterModuleType("cc_cond_tms_dl_common_lib", condTmsDlCommonLibFactory)
    android.RegisterModuleType("cc_cond_tms_dl_ree_lib", condTmsDlReeLibFactory)
    android.RegisterModuleType("cc_cond_tms_dl_tee_lib", condTmsDlTeeLibFactory)
}

func condSEBinFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, condSEBin)
    return module
}

func cond7816LibFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, cond7816Lib)
    return module
}

func condTmsDlCommonLibFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, condTmsDlCommonLib)
    return module
}

func condTmsDlReeLibFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, condTmsDlReeLib)
    return module
}

func condTmsDlTeeLibFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, condTmsDlTeeLib)
    return module
}


func condSEBin(ctx android.LoadHookContext) {
    ctx.AppendProperties(binSeGetProps(ctx))
}

func cond7816Lib(ctx android.LoadHookContext) {
    ctx.AppendProperties(lib7816GetProps(ctx))
}

func condTmsDlCommonLib(ctx android.LoadHookContext) {
    ctx.AppendProperties(libTmsDlCommonGetProps(ctx))
}

func condTmsDlReeLib(ctx android.LoadHookContext) {
    ctx.AppendProperties(libTmsDlReeGetProps(ctx))
}

func condTmsDlTeeLib(ctx android.LoadHookContext) {
    ctx.AppendProperties(libTmsDlTeeGetProps(ctx))
}

func useTmsAidlNfc(ctx android.BaseContext) (ret bool) {
    tmsBuildCtx := ctx.Config().VendorConfig("tmsBuildCtx")
    isNfcAidlHal := tmsBuildCtx.Bool("nfcAidlHal")
    fmt.Println("tms_extns use nfc aidl hal :", isNfcAidlHal)
    versionName := ctx.AConfig().PlatformVersionName()
    fmt.Println("tms_extns version name :", versionName)
    version, error := strconv.Atoi(versionName)
    if error != nil {
        fmt.Println("can not convert version name")
        return
    }
    fmt.Println("tms_extns version number:", version)

    if (isNfcAidlHal && version >= 13) {
        return true
    } else {
        return false
    }
}
func binSeGetProps(ctx android.BaseContext) (*props) {
    ps := &props{}
    tmsBuildCtx := ctx.Config().VendorConfig("tmsBuildCtx")
    nfcType := tmsBuildCtx.String("nfcType")
    fmt.Println("binSeGetProps: nfcType =", nfcType)

    if nfcType == TYPE_C1 || nfcType == TYPE_TMS {
        ps.Cflags = append(ps.Cflags, "-DUSED_COS_REE_SPI_DL")
        ps.Cflags = append(ps.Cflags, "-DUSED_COS_I2C_DL")
        if nfcType == TYPE_C1 {
            ps.Include_dirs = append(ps.Include_dirs, "hardware/nxp/nfc/SN100x/extns/impl/nxpnfc/2.0")
            //ps.Include_dirs = append(ps.Include_dirs, "hardware/nxp/secure_element/extns/impl")

            ps.Cflags = append(ps.Cflags, "-DUSE_C1")

            ps.Shared_libs = append(ps.Shared_libs, "vendor.nxp.nxpnfc@2.0")
        } else if nfcType == TYPE_TMS {
            ps.Include_dirs = append(ps.Include_dirs, "vendor/tms/vnd/opensource/halimpl/nfc/THN31/extns/impl/tmsnfc/1.0")

            ps.Cflags = append(ps.Cflags, "-DUSE_TMS_NFC")
            if useTmsAidlNfc(ctx) {
                ps.Shared_libs = append(ps.Shared_libs, "vendor.tms.tmsnfc_aidl-V1-ndk")
                ps.Cflags = append(ps.Cflags, "-DTMS_NFC_AIDL")
            } else {
                ps.Shared_libs = append(ps.Shared_libs, "vendor.tms.tmsnfc@1.0")
            }
            ps.Shared_libs = append(ps.Shared_libs, "libbinder_ndk")
            ps.Shared_libs = append(ps.Shared_libs, "vendor.tms.tmsese@1.0")
            ps.Shared_libs = append(ps.Shared_libs, "vendor.tms.tmsese@1.1")

            versionName := ctx.AConfig().PlatformVersionName()
            fmt.Println("platform version name :", versionName)
            version, error := strconv.Atoi(versionName)
            if error != nil {
                fmt.Println("can not convert version name")
                return ps
            }

            fmt.Println("platform version number:", version)
            if version >= 14 {
                ps.Shared_libs = append(ps.Shared_libs, "android.hardware.secure_element-V1-ndk")
            } else {
                fmt.Println("not support secure_element-V1-ndk")
            }
        }
    } else {
        //None C1 and TMS
    }
    return ps
}

func lib7816GetProps(ctx android.BaseContext) (*props) {
    ps := &props{}
    tmsBuildCtx := ctx.Config().VendorConfig("tmsBuildCtx")
    nfcType := tmsBuildCtx.String("nfcType")
    fmt.Println("lib7816GetProps: nfcType =", nfcType)
    if nfcType == TYPE_C1 {
        ps.Include_dirs = append(ps.Include_dirs, "hardware/nxp/nfc/SN100x/extns/impl/nxpnfc/2.0")
        //ps.Include_dirs = append(ps.Include_dirs, "hardware/nxp/nfc/SN100x/halimpl/tml")
        //ps.Include_dirs = append(ps.Include_dirs, "hardware/nxp/nfc/SN100x/halimpl/common")

        ps.Cflags = append(ps.Cflags, "-DUSE_C1")

        ps.Shared_libs = append(ps.Shared_libs, "vendor.nxp.nxpnfc@2.0")
    } else if nfcType == TYPE_TMS {
        ps.Include_dirs = append(ps.Include_dirs, "vendor/tms/vnd/opensource/halimpl/nfc/THN31/extns/impl/tmsnfc/1.0")

        ps.Cflags = append(ps.Cflags, "-DUSE_TMS_NFC")
        if useTmsAidlNfc(ctx) {
            ps.Shared_libs = append(ps.Shared_libs, "vendor.tms.tmsnfc_aidl-V1-ndk")
            ps.Cflags = append(ps.Cflags, "-DTMS_NFC_AIDL")
        } else {
            ps.Shared_libs = append(ps.Shared_libs, "vendor.tms.tmsnfc@1.0")
        }
        ps.Shared_libs = append(ps.Shared_libs, "libbinder_ndk")
    } else {
        //None C1 and TMS
        ps.Shared_libs = append(ps.Shared_libs, "libc_secshared")
    }
    return ps
}

func libTmsDlCommonGetProps(ctx android.BaseContext) (*props) {
    ps := &props{}
    tmsBuildCtx := ctx.Config().VendorConfig("tmsBuildCtx")
    nfcType := tmsBuildCtx.String("nfcType")
    fmt.Println("libTmsDlCommonGetProps: nfcType =", nfcType)

    if nfcType == TYPE_C1 || nfcType == TYPE_TMS {
        if nfcType == TYPE_C1 {
            ps.Cflags = append(ps.Cflags, "-DUSE_C1")
        } else if nfcType == TYPE_TMS {
            ps.Cflags = append(ps.Cflags, "-DUSE_TMS_NFC")
            if useTmsAidlNfc(ctx) {
                ps.Shared_libs = append(ps.Shared_libs, "vendor.tms.tmsnfc_aidl-V1-ndk")
                ps.Cflags = append(ps.Cflags, "-DTMS_NFC_AIDL")
            } else {
                ps.Shared_libs = append(ps.Shared_libs, "vendor.tms.tmsnfc@1.0")
            }
            ps.Shared_libs = append(ps.Shared_libs, "libbinder_ndk")
        }

    } else {
        //None C1 and TMS
        ps.Shared_libs = append(ps.Shared_libs, "libc_secshared")
    }
    return ps
}

func libTmsDlReeGetProps(ctx android.BaseContext) (*props) {
    ps := &props{}
    tmsBuildCtx := ctx.Config().VendorConfig("tmsBuildCtx")
    nfcType := tmsBuildCtx.String("nfcType")
    fmt.Println("libTmsDlReeGetProps: nfcType =", nfcType)

    ps.Cflags = append(ps.Cflags, "-DTMS_REE")
    if nfcType == TYPE_C1 || nfcType == TYPE_TMS {
        ps.Cflags = append(ps.Cflags, "-DUSED_COS_REE_SPI_DL")
        ps.Cflags = append(ps.Cflags, "-DUSED_COS_I2C_DL")
        //ps.Cflags = append(ps.Cflags, "-DUSED_COS_WIRED_SE_DL")
        if nfcType == TYPE_C1 {
            ps.Include_dirs = append(ps.Include_dirs, "hardware/nxp/nfc/SN100x/extns/impl/nxpnfc/2.0")

            ps.Cflags = append(ps.Cflags, "-DUSE_C1")

            ps.Shared_libs = append(ps.Shared_libs, "vendor.nxp.nxpnfc@2.0")
        } else if nfcType == TYPE_TMS {
            ps.Include_dirs = append(ps.Include_dirs, "vendor/tms/vnd/opensource/halimpl/nfc/THN31/extns/impl/tmsnfc/1.0")

            ps.Cflags = append(ps.Cflags, "-DUSE_TMS_NFC")
            if useTmsAidlNfc(ctx) {
                ps.Shared_libs = append(ps.Shared_libs, "vendor.tms.tmsnfc_aidl-V1-ndk")
                ps.Cflags = append(ps.Cflags, "-DTMS_NFC_AIDL")
            } else {
                ps.Shared_libs = append(ps.Shared_libs, "vendor.tms.tmsnfc@1.0")
            }
            ps.Shared_libs = append(ps.Shared_libs, "libbinder_ndk")
        }
    } else {
        //None C1 and TMS
        ps.Cflags = append(ps.Cflags, "-DUPDATE_NFC_FW_STATE(state)=phHWNciHal_update_nfc_fwstate(state)")
        ps.Cflags = append(ps.Cflags, "-DMACO_INCLUDE_phC1NciHal_Adaptation=#include \"phNxpNciHal_Adaptation.h\"")

        ps.Shared_libs = append(ps.Shared_libs, "libc_secshared")
    }
    return ps
}

func libTmsDlTeeGetProps(ctx android.BaseContext) (*props) {
    ps := &props{}
    tmsBuildCtx := ctx.Config().VendorConfig("tmsBuildCtx")
    nfcType := tmsBuildCtx.String("nfcType")
    fmt.Println("libTmsDlTeeGetProps: nfcType = ", nfcType, ", useVendorCa = ", tmsBuildCtx.Bool("useVerdorCa"))

    if nfcType == TYPE_C1 || nfcType == TYPE_TMS {
        if nfcType == TYPE_C1 {
            ps.Cflags = append(ps.Cflags, "-DUSE_C1")
        } else if nfcType == TYPE_TMS {
            ps.Cflags = append(ps.Cflags, "-DUSE_TMS_NFC")
        }

        ps.Cflags = append(ps.Cflags, "-DTMS_TEE_CA_TA")
        if tmsBuildCtx.Bool("useVerdorCa") {
            ps.Cflags = append(ps.Cflags, "-DCA_SO_FILE_NAME=\"/vendor/lib64/gptmsese.so\"")
        } else {
            ps.Cflags = append(ps.Cflags, "-DCA_SO_FILE_NAME=\"/system/lib64/gptmsese.so\"")
        }
    }
    return ps
}
