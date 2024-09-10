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

#ifndef PHTMSSPILIB_COSDL_COMMON_UTILS_H
#define PHTMSSPILIB_COSDL_COMMON_UTILS_H

#if defined (USE_TMS_NFC) || defined (USE_C1)
    #define PROP_KEY_SECOS_DL_STATE "vendor.tms.nfc.secos.download"
#else
    #define PROP_KEY_SECOS_DL_STATE "nfc.vendor.tms.nfc.secos.download"
#endif

// 7816-3-T1/include/common.h
#include "tmsCommon.h"
#include <time.h>

#include "ITmsPhAbs.h"
using vendor::tms::ITmsPhAbs;

#include <stdint.h>
#include <pthread.h>

#include <string>
using std::string;

#include <vector>
using std::vector;


#ifndef UNUSED
    #define UNUSED(arg) (void)(arg)
#endif

// Header 3 + payload 255 ( + 1 for T1 protocol)
#define NCI_RSP_LEN_MAX 259
#define NCI20_RESET_CMD "20000100"
#define NCI20_RESET_CMD_LEN 8
#define NCI20_INIT_CMD "2001020000"
#define NCI20_INIT_CMD_LEN 10

#define APDU_AUTH_HEAD "00F20000C601"
#define APDU_AUTH_HEAD_LEN 12
#define APDU_AUTH_HEAD2 "00F200000000C601"
#define APDU_AUTH_HEAD_LEN2 16


#define VTP_HEADER_LEN1 1
#define VTP_HEADER_F "f"
#define VTP_HEADER_UPPER_F "F"
#define VTP_HEADER_V "v"
#define VTP_HEADER_UPPER_V "V"

#define VTP_HEADER_LEN3 3
#define VTP_HEADER_RAW "raw"
#define VTP_HEADER_UPPER_RAW "RAW"
#define VTP_HEADER_VER "ver"
#define VTP_HEADER_UPPER_VER "VER"

#define VTP_HEADER_LEN4 4
#define VTP_HEADER_SEND "send"
#define VTP_HEADER_UPPER_SEND "SEND"
#define VTP_HEADER_WAIT "wait"
#define VTP_HEADER_UPPER_WAIT "WAIT"

#define VTP_HEADER_LEN5 5
#define VTP_HEADER_SLEEP "sleep"
#define VTP_HEADER_UPPER_SLEEP "SLEEP"

#define VTP_HEADER_LEN6 6
#define VTP_HEADER_ASSERT "assert"
#define VTP_HEADER_UPPER_ASSERT "ASSERT"

#define VTP_HEADER_LEN9 9
#define VTP_HEADER_T_EC1_END "T_EC1_END"
#define VTP_HEADER_T_EC2_END "T_EC2_END"
#define VTP_HEADER_R_EC1_END "R_EC1_END"
#define VTP_HEADER_R_EC2_END "R_EC2_END"

#define VTP_HEADER_LEN11 11
#define VTP_HEADER_T_EC1_BGN "T_EC1_BEGIN"
#define VTP_HEADER_T_EC2_BGN "T_EC2_BEGIN"
#define VTP_HEADER_R_EC1_BGN "R_EC1_BEGIN"
#define VTP_HEADER_R_EC2_BGN "R_EC2_BEGIN"

#define VTP_HEADER_LEN13 13
#define VTP_HEADER_T_EC1_VER_END "T_EC1_VER_END"
#define VTP_HEADER_T_EC2_VER_END "T_EC2_VER_END"
#define VTP_HEADER_R_EC1_VER_END "R_EC1_VER_END"
#define VTP_HEADER_R_EC2_VER_END "R_EC2_VER_END"

#define VTP_HEADER_LEN15 15
#define VTP_HEADER_SC_VER_END "SCRIPTS_VER_END"
// TAG VER BEGIN
#define VTP_HEADER_T_EC1_VER_BGN "T_EC1_VER_BEGIN"
#define VTP_HEADER_T_EC2_VER_BGN "T_EC2_VER_BEGIN"
#define VTP_HEADER_R_EC1_VER_BGN "R_EC1_VER_BEGIN"
#define VTP_HEADER_R_EC2_VER_BGN "R_EC2_VER_BEGIN"

#define VTP_HEADER_LEN16 16
#define VTP_HEADER_PTH_020000_END "PATCH_020000_END"
#define VTP_HEADER_PTH_020100_END "PATCH_020100_END"
#define VTP_HEADER_PTH_020200_END "PATCH_020200_END"

#define VTP_HEADER_LEN17 17
#define VTP_HEADER_SC_VER_BGN "SCRIPTS_VER_BEGIN"

#define VTP_HEADER_LEN18 18
#define VTP_HEADER_PTH_020000_BGN "PATCH_020000_BEGIN"
#define VTP_HEADER_PTH_020100_BGN "PATCH_020100_BEGIN"
#define VTP_HEADER_PTH_020200_BGN "PATCH_020200_BEGIN"

#define VTP_HEADER_LEN20 20
#define VTP_HEADER_SC_CMD_END "SCRIPTS_COMMANDS_END"
#define VTP_HEADER_PTH_020000_VER_END "PATCH_020000_VER_END"
#define VTP_HEADER_PTH_020100_VER_END "PATCH_020100_VER_END"
#define VTP_HEADER_PTH_020200_VER_END "PATCH_020200_VER_END"

#define VTP_HEADER_LEN22 22
#define VTP_HEADER_SC_CMD_BGN "SCRIPTS_COMMANDS_BEGIN"
#define VTP_HEADER_PTH_020000_VER_BGN "PATCH_020000_VER_BEGIN"
#define VTP_HEADER_PTH_020100_VER_BGN "PATCH_020100_VER_BEGIN"
#define VTP_HEADER_PTH_020200_VER_BGN "PATCH_020200_VER_BEGIN"


#define FILE_NAME_LEN 256

#define COS_VER_MASTER_020000 0X020000
#define COS_VER_MASTER_020100 0X020100
#define COS_VER_MASTER_020200 0X020200

#define COS_VER_MASTER_MASK 0x0000FFFFFF
#define COS_VER_PATCH_MASK 0xFFFF000000

#define INVALID_COS_VER (0xFFFFFFFFFFFFFFFF)
#define INVALID_FW_VER (0xFFFFFFFF)

#define INVALID_CHIP_TYPE (0xFF)
#define CHIP_EC1 (0x10)
#define CHIP_EC2 (0x20)

#define INVALID_CHIP_KEY_TYPE (0xFF)
#define CHIP_T_KEY (0x00)
#define CHIP_R_KEY (0x80)

#define VTP_DL_FW_ROUTINE "DOWNLOAD_FW_ROUTINE"
#define VTP_DL_BL_ROUTINE "DOWNLOAD_BL_ROUTINE"
#define VTP_DL_CPS_PTH_ROUTINE "DOWNLOAD_COS_PTH_ROUTINE"

#define VTP_DL_NFC_DATA_STORAGE_FILE "/data/vendor/nfc/tmsNfcVtpDataStorage.bin"
#define VTP_DL_SE_DATA_STORAGE_FILE "/data/vendor/secure_element/tmsSeVtpDataStorage.bin"

#define FW_DL_LONG_TIME_LIMIT (10 * 24 * 60 * 60)  // 10 days, uint second
#define FW_DL_SHORT_TIME_LIMIT (5 * 60)            // 5 minutes, uint second
#define FW_DL_LONG_TIME_COUNT_LIMIT 100
#define FW_DL_SHORT_TIME_COUNT_LIMIT 20

#define BL_DL_LONG_TIME_LIMIT (10 * 24 * 60 * 60)  // 10 days, uint second
#define BL_DL_SHORT_TIME_LIMIT (5 * 60)            // 5 minutes, uint second
#define BL_DL_LONG_TIME_COUNT_LIMIT 100
#define BL_DL_SHORT_TIME_COUNT_LIMIT 20

#define COS_PTH_DL_LONG_TIME_LIMIT (10 * 24 * 60 * 60)  // 10 days, uint second
#define COS_PTH_DL_SHORT_TIME_LIMIT (10 * 60)           // 10 minutes, uint second
#define COS_PTH_DL_LONG_TIME_COUNT_LIMIT 100
#define COS_PTH_DL_SHORT_TIME_COUNT_LIMIT 10

typedef enum {
    NFC_DLD_PWR = 9,
    NFC_DLD_PWR_VEN_ON,
    NFC_DLD_PWR_VEN_OFF,
    NFC_DLD_PWR_DL_ON,
    NFC_DLD_PWR_DL_OFF,
    NFC_DLD_FLUSH,
} NFC_DLD_PWR_MODE;

typedef enum {
    DL_CMD_INVALID = 0,
    DL_CMD_RAW = 1,
    DL_CMD_T1 = 2,
    DL_CMD_NCI = 3,
    DL_CMD_7816_4_APDU = 4,
    DL_CMD_ASSERT = 5,
    DL_CMD_SLEEP = 6,
    DL_CMD_VERSION = 7,

    SCRIPTS_VER_BEGIN = 8,
    SCRIPTS_VER_END = 9,

    T_EC1_VER_BEGIN = 10,
    T_EC1_VER_END = 11,

    R_EC1_VER_BEGIN = 12,
    R_EC1_VER_END = 13,

    T_EC2_VER_BEGIN = 14,
    T_EC2_VER_END = 15,
    R_EC2_VER_BEGIN = 16,
    R_EC2_VER_END = 17,

    SCRIPTS_COMMANDS_BEGIN = 18,
    SCRIPTS_COMMANDS_END = 19,

    T_EC1_BEGIN = 20,
    T_EC1_END = 21,

    R_EC1_BEGIN = 22,
    R_EC1_END = 23,

    T_EC2_BEGIN = 24,
    T_EC2_END = 25,
    R_EC2_BEGIN = 26,
    R_EC2_END = 27,

    // COS patch version
    PATCH_020000_VER_BEGIN = 28,
    PATCH_020000_VER_END = 29,

    PATCH_020100_VER_BEGIN = 30,
    PATCH_020100_VER_END = 31,

    PATCH_020200_VER_BEGIN = 32,
    PATCH_020200_VER_END = 33,

    // COS patch
    PATCH_020000_BEGIN = 34,
    PATCH_020000_END = 35,

    PATCH_020100_BEGIN = 36,
    PATCH_020100_END = 37,

    PATCH_020200_BEGIN = 38,
    PATCH_020200_END = 39,
} DL_CMD_TYPE;

typedef struct {
    string cmd;
    DL_CMD_TYPE cmdType;
} VtpDlCmd;

typedef struct VtpBinDlCmd {
    char *cmd;
    uint16_t cmdLen;
    DL_CMD_TYPE cmdType;
    struct VtpBinDlCmd *pNext;
} VtpBinDlCmdT;

// CosVersion response's length is 14 bytes
typedef struct {
    uint8_t chipCla;      // Internal chip class
    uint8_t operatorCla;  // operator class, NFC SE is 0x04
    uint8_t appCla;       // Application class, NFC SE is 0x02
    uint8_t platformCla;  // Platform class, NFC SE is 0x4E

    uint8_t cosCla;       // COS class, java or native, NFC SE is 0x4A

    uint8_t cosDate[4];   // COS create date, yyyyMMdd, such as, 20220113
    uint8_t patchVer[2];  // patch version
    uint8_t baseVer[3];    // COS version, such as, 020000
} CosVersion;

typedef struct {
    char *routineName = NULL;
    uint8_t *cmd = NULL;
    uint8_t *rsp = NULL;
    uint8_t *rspChk = NULL;
    int cmdLen = 0;
    int rspLen = 0;
    int rspChkLen = 0;
    void *pResStatus = NULL;
    void *(*routine)(void *);
    pthread_cond_t *pCond = NULL;
    pthread_mutex_t *pMutex = NULL;
    void *pParameters = NULL;
} DownloadCmdOp;

typedef struct {
    uint32_t fwVer = INVALID_FW_VER;
    uint32_t blVer = INVALID_FW_VER;
} NfccFwBlVer;

typedef struct {
    uint32_t fwVer = INVALID_FW_VER;
    uint32_t nfccBlVer = INVALID_FW_VER;
    uint32_t cosBaseVer = INVALID_FW_VER;
    uint32_t seBlVer = INVALID_FW_VER;
    uint8_t chipType = INVALID_CHIP_TYPE;
    uint8_t chipKeyType = INVALID_CHIP_KEY_TYPE;
} ChipInfo;

typedef struct {
    uint64_t cosVer = INVALID_FW_VER;
    VtpBinDlCmdT *pChipInfo;
} VerInfo;

typedef struct {
    char *fileName = NULL;

    // PT, pass-through transmission
    bool needPT2SeBl = false;
    bool isNfccBlState = false;
    bool isSearchNfcDir = false;
    ITmsPhAbs *pTmsPhAbs = NULL;
    ChipInfo *pChipInfo = NULL;
} VtpParams;

typedef struct {
    time_t startTimeLong;
    time_t startTimeShort;
    uint8_t longTimeCount;
    uint8_t shortTimeCount;
} VtpDownloadData;

typedef struct {
    VtpDownloadData fwDLData;
    VtpDownloadData blDLData;
} NfcVtpDownloadDataCb;

#ifdef __cplusplus
extern "C" {
#endif

bool Cstr2hex(const char *str, int strLen, uint8_t *data, int dataLen);

bool GetVaildFileName(char *fileName, int fileNameLen,
                      const char *keyName, int keyNameLen, string defaultVal,
                      bool isNfcDir);

uint64_t GetCosVerFromVtp(char *fileName, int fileNameLen, ChipInfo chipInfo);
bool chkEseSoftReset(DownloadCmdOp *pDlCmd);
ESESTATUS execApduCmdChkRes(const char *cmd, int cmdLen, void *arg);
ESESTATUS vtpDownload(void *arg);

void ThreadExitHandler(int sig);
bool RegisterExitSignal();

bool startThread(DownloadCmdOp *pDlCmdOp, int timeout);

void *TmsDlopen(const char *filename, int filenameLen);
bool TmsDlclose(void **handle);
pthread_mutex_t *GetMutex(void);
pthread_cond_t *GetGpCond(void);
void SetGpCond(pthread_cond_t *pCond);
bool GetThreadRunning(void);
void SetThreadRunning(bool running);
int GetNfcWatchDogTime(void);
void SetNfcWatchDogTime(int time);
void checkAndRemoveOverrideVtpFile(void);
#ifdef __cplusplus
};
#endif

#endif // PHTMSSPILIB_COSDL_COMMON_UTILS_H
