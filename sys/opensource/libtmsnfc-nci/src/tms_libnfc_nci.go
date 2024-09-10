package tms_libnfc_nci

import (
        "android/soong/android"
        "android/soong/cc"
        "fmt"
        "strconv"
)

type props struct {
    Cflags []string
    Shared_libs []string
}

func init(){
    android.RegisterModuleType("tms_libnfc_nci_default", tmsLibnfcNci)
}


func tmsLibnfcNci() android.Module {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, tmsLibnfcNciHook)
    return module
}


func tmsLibnfcNciHook(ctx android.LoadHookContext) {
    ps := &props{}

    tmsBuildCtx := ctx.Config().VendorConfig("tmsBuildCtx")
    isNfcAidlHal := tmsBuildCtx.Bool("nfcAidlHal")
    fmt.Println("tms_libnfc_nci use nfc aidl hal :", isNfcAidlHal)

    versionName := ctx.AConfig().PlatformVersionName()
    fmt.Println("tms_libnfc_nci version name :", versionName)
    version, error := strconv.Atoi(versionName)
    if error != nil {
        fmt.Println("can not convert version name")
        return
    }
    fmt.Println("tms_libnfc_nci version number:", version)

    if (isNfcAidlHal && version >= 13) {
        ps.Cflags = append(ps.Cflags, "-DTMS_NFC_AIDL")
        ps.Shared_libs = append(ps.Shared_libs, "vendor.tms.tmsnfc_aidl-V1-ndk")
    }
    ps.Shared_libs = append(ps.Shared_libs, "vendor.tms.tmsnfc@1.0")

    ctx.AppendProperties(ps)
}



