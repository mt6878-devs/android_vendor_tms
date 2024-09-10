/*
 * Copyright (C) 2022 Tsingteng MicroSystem
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

package tms_nfcnci

import (
    "android/soong/android"
    "android/soong/java"
    "fmt"
    "strconv"
    "math"
)

type props struct {
    Srcs        []string
    Static_libs []string
}

func init() {
    android.RegisterModuleType("tms_nfcnci_default", tmsNfcNci)
}

func tmsNfcNci() android.Module {
    module := java.DefaultsFactory()
    android.AddLoadHook(module, tmsNfcNciHook)
    return module
}

func tmsNfcNciHook(ctx android.LoadHookContext) {
    ps := &props{}

    versionName := ctx.AConfig().PlatformVersionName()
    fmt.Println("platform version name :", versionName)
    version, error := strconv.Atoi(versionName)
    if error != nil {
        fmt.Println("can not convert version name")
        version = int(math.MaxInt32) // set latest version as default.
    }

    fmt.Println("platform version number:", version)
    if version <= 13 {
        ps.Srcs = append(ps.Srcs, "src/com/tms/compat/13/TmsCompat.java")
        ps.Srcs = append(ps.Srcs, "src/com/tms/compat/13/TmsNfcAdapterStub.java")
    } else {
        ps.Srcs = append(ps.Srcs, "src/com/tms/compat/14/TmsCompat.java")
        ps.Srcs = append(ps.Srcs, "src/com/tms/compat/14/TmsNfcAdapterStub.java")
    }

    tmsBuildCtx := ctx.Config().VendorConfig("tmsBuildCtx")
    fmt.Println("tmsBuildCtx: wiredSe = ", tmsBuildCtx.String("wiredSe"))
    if tmsBuildCtx.String("wiredSe") == "true" {
        fmt.Println("compile wiredse code")
        ps.Static_libs = append(ps.Static_libs, "vendor.tms.wiredse-V1.0-java")
        ps.Srcs = append(ps.Srcs, "src/com/tms/compat/wiredse/TmsWiredSeService.java")
    } else if tmsBuildCtx.String("wiredSe") == "classic" {
        fmt.Println("NfcNci: compile classic wiredse code")
        ps.Srcs = append(ps.Srcs, "src/com/tms/compat/wiredse/TmsNfcAdapterExtrasService.java")
    }

    ctx.AppendProperties(ps)
}
