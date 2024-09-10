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

#include <dlfcn.h>
#include <pthread.h>
#include <semaphore.h>

#include <android-base/file.h>
#include <list>
#include <vector>
#include <fstream>
#include <cstdio>
#include <iostream>
#include <unordered_map>
#include <string>

#include <cstring>

#include <cinttypes>

#if defined (USE_TMS_NFC) || defined (USE_C1)
    #include "tmsSecureString.h"
#else
    #include "securec.h"
#endif

#include "tmsCommon.h"
#include "tmsDlCommonUtils.h"
#include "eseConfig.h"
#include "tmslog.h"

#include "ITmsPhAbs.h"
using vendor::tms::ITmsPhAbs;
#ifdef TMS_REE
    #include "TmsRee.h"
    using vendor::tms::TmsRee;
#endif
volatile bool g_threadRunning = false;


using namespace std;

#ifdef TAG
#undef TAG
#endif
#define TAG g_tag

static const char g_tag[] = "TmsCosDl:DlCommonUtils";
static unordered_map<string, DL_CMD_TYPE> g_headers;

ITmsPhAbs *gpTmsPh = NULL;
int g_nfcWatchDogTime = 0;

static bool g_nfcDlDataRead = false;
static bool g_seDlDataRead = false;
static NfcVtpDownloadDataCb g_nfcVtpDlDataCb;
static VtpDownloadData g_cosPthDLData;

static void *InitMutexCond();
static pthread_mutexattr_t g_mutexAttr;
pthread_mutex_t g_mutex;
pthread_cond_t *g_gpCond = (pthread_cond_t *)InitMutexCond();
static void *InitMutexCond()
{
    pthread_mutexattr_settype(&g_mutexAttr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&g_mutex, &g_mutexAttr);
    return nullptr;
}

bool GetThreadRunning(void)
{
    return g_threadRunning;
}

void SetThreadRunning(bool running)
{
    g_threadRunning = running;
}

int GetNfcWatchDogTime(void)
{
    return g_nfcWatchDogTime;
}

void SetNfcWatchDogTime(int time)
{
    g_nfcWatchDogTime = time;
}

static bool Str2hex(string str, uint8_t *data, int dataLen);
static ESESTATUS vtpBinDownload(DownloadCmdOp *pDlCmdOp, VtpParams *pVtpParams);
static ESESTATUS vtpTxtDownload(DownloadCmdOp *pDlCmdOp, VtpParams *pVtpParams);
/**
 * ppHead   point the cmds chain head's address
 * ppTail   point the cmds chain tail's address
 * pBuffer  ponit the current parse address
 * len      parse length
 *
 * return offset of the currently parsed
 */
static int parseCmd(VtpBinDlCmdT **ppHead, VtpBinDlCmdT **ppTail,
                    char *pBuffer, int offset, int maxLen);

/**
 * pBuffer  ponit the header address, a char string
 * return cmd type
 */
static DL_CMD_TYPE parseHeader(char *pBuffer);

pthread_cond_t *GetGpCond(void)
{
    return g_gpCond;
}

void SetGpCond(pthread_cond_t *pCond)
{
    g_gpCond = pCond;
}

pthread_mutex_t *GetMutex(void)
{
    return &g_mutex;
}

static string searchFilePath(const string fileName, vector<string>& searchPth)
{
    for (string path : searchPth) {
        path.append(fileName);
        struct stat fileStat;
        if (stat(path.c_str(), &fileStat) != 0) {
            continue;
        }
        if (S_ISREG(fileStat.st_mode)) {
            return path;
        }
    }

    TMS_LOG_W(g_tag, "cannot find valid file: fileName = %s", fileName.c_str());
    return "";
}

// Compare str is end with endStr
bool IsEndWith(const char *str, int strLen, const char *endStr, int endStrLen)
{
    if (endStrLen > strLen || nullptr == str || nullptr == endStr) {
        return false;
    }

    for (int i = 1; i <= endStrLen; i++) {
        if (str[strLen - i] != endStr[endStrLen - i]) {
            TMS_LOG_I(g_tag, "%s: name[%d]=%c, endStr[%d]=%c", __FUNCTION__,
                      strLen - i, str[strLen - i],
                      endStrLen - i, endStr[endStrLen - i]);
            return false;
        }
    }

    return true;
}

static void CheckAssertCmd(VtpBinDlCmdT *pVtpDlCmd)
{
    char *cmd = pVtpDlCmd->cmd;
    uint16_t cmdLen = pVtpDlCmd->cmdLen;
    for (int i = 1; (i <= cmdLen) && ('*' == cmd[cmdLen - i]); i++) {
        cmd[cmdLen - i] = '\0';
        pVtpDlCmd->cmdLen--;
    }
}

/******************************************************************************
* Function     parseCmd
*
* Description  Parse the bin script's content based on line(split by '\n').
*              Parsed cmds will be stored to the ppHead pointed linked list.
*
* @params      ppHead - point the cmd linked list head address.
* @params      ppTail - point the cmd linked list tail address.
* @params      pBuffer - point the bin script's buffer.
* @params      offset - currently parsed byte's offset.
* @params      maxLen - pBuffer pointed buffer's size.
*
* Returns      Return the bin script's offset, it is the currently byte by parsed.
******************************************************************************/
static int parseCmd(VtpBinDlCmdT **ppHead, VtpBinDlCmdT **ppTail,
                    char *pBuffer, int offset, int maxLen)
{
    VtpBinDlCmdT *pVtpDlCmd = (VtpBinDlCmdT *)calloc(1, sizeof(VtpBinDlCmdT));
    int tagStart = offset;
    int valStart = 0;
    int valIndex = 0;
    bool isKeySplit = true;

    // '\r' may be contained in the cmd, and it will be ignored by strlen/2
    while ((offset < maxLen - 2) && ('\n' != pBuffer[offset])) {
        if (isKeySplit) {
            if ((' ' == pBuffer[offset]) || ('\r' == pBuffer[offset])) {
                if (tagStart == offset) {
                    // ignore blank spase
                    tagStart = offset + 1;
                } else {
                    isKeySplit = false;
                    pBuffer[offset] = '\0';

                    pVtpDlCmd->cmdType = parseHeader(&pBuffer[tagStart]);
                    valStart = offset + 1;
                    valIndex = valStart;
#ifdef DEBUG_FLAG
                    TMS_LOG_D(g_tag, "%s: cmdHeader[%s]", __FUNCTION__, &pBuffer[tagStart]);
#endif
                }
            }
        } else {
            if ((' ' == pBuffer[offset]) && (valStart == offset)) {
                // ignore blank space
                valStart = offset + 1;
                valIndex = valStart;
            } else if ((' ' != pBuffer[offset]) && ('\r' != pBuffer[offset])) {
                if (valIndex < offset) {
                    pBuffer[valIndex] = pBuffer[offset];
                }
                valIndex++;
            } else {
                // is a blank space, do nothing
            }
        }

        offset++;
#ifdef DEBUG_FLAG
        char iVal = pBuffer[offset];
        char iPlus1Val = pBuffer[offset + 1];
#endif
        pBuffer[offset] = pBuffer[offset] ^ pBuffer[offset + 1];
#ifdef DEBUG_FLAG
        TMS_LOG_D(g_tag, "%s: [%d]=%02X, iVal=%02X, iPlus1Val=%02X",
                  __FUNCTION__, offset, pBuffer[offset], iVal, iPlus1Val);
#endif
    }

    if (tagStart == offset) {
#ifdef DEBUG_FLAG
        TMS_LOG_D(g_tag, "%s: empty line", __FUNCTION__);
#endif
        free(pVtpDlCmd);
        pVtpDlCmd = NULL;
        return offset;
    } else {
        if (NULL == *ppHead) {
            *ppHead = pVtpDlCmd;
        }

        if (NULL != *ppTail) {
            (*ppTail)->pNext = pVtpDlCmd;
        }
        *ppTail = pVtpDlCmd;
    }

    if (offset == maxLen - 2) {  // length-2
        pBuffer[maxLen - 1] = pBuffer[maxLen - 1] ^ 0x84;
        if (('\n' != pBuffer[maxLen - 2]) &&
            ('\r' != pBuffer[maxLen - 2]) &&
            (' ' != pBuffer[maxLen - 2])) {
            pBuffer[valIndex++] = pBuffer[maxLen - 2];  // length-2
        }

        if (('\n' != pBuffer[maxLen - 1]) &&
            ('\r' != pBuffer[maxLen - 1]) &&
            (' ' != pBuffer[maxLen - 1])) {
            pBuffer[valIndex++] = pBuffer[maxLen - 1];
        }
        pBuffer[valIndex] = '\0';
    } else {
        pBuffer[valIndex] = '\0';
        if (valIndex > 0) {
            if (pBuffer[valIndex - 1] == '\r') {
                valIndex--;
                pBuffer[valIndex] = '\0';
            }
        }
    }

    pVtpDlCmd->cmd = &pBuffer[valStart];
    pVtpDlCmd->cmdLen = (uint16_t)(valIndex - valStart);
    pVtpDlCmd->pNext = NULL;
    // handle DL_CMD_NCI or DL_CMD_T1
    if (DL_CMD_RAW == pVtpDlCmd->cmdType) {
        if (strncmp(pVtpDlCmd->cmd, "5A", 2) == 0) {  // 2 length
            pVtpDlCmd->cmdType = DL_CMD_T1;
        } else {
            pVtpDlCmd->cmdType = DL_CMD_NCI;
        }
    } else if (DL_CMD_ASSERT == pVtpDlCmd->cmdType) {
        CheckAssertCmd(pVtpDlCmd);
    }

#ifdef DEBUG_FLAG
    TMS_LOG_D(g_tag, "%s: cmdType = %d, cmd = %s",
              __FUNCTION__, pVtpDlCmd->cmdType, pVtpDlCmd->cmd);
#endif

    return offset;
}

static void InitHeadersMap()
{
    if (!g_headers.empty()) {
        return;
    }

    g_headers.insert({{VTP_HEADER_F, DL_CMD_7816_4_APDU}, {VTP_HEADER_UPPER_F, DL_CMD_7816_4_APDU},
                      {VTP_HEADER_SEND, DL_CMD_7816_4_APDU}, {VTP_HEADER_UPPER_SEND, DL_CMD_7816_4_APDU},
                      {VTP_HEADER_RAW, DL_CMD_RAW}, {VTP_HEADER_UPPER_RAW, DL_CMD_RAW},
                      {VTP_HEADER_ASSERT, DL_CMD_ASSERT}, {VTP_HEADER_UPPER_ASSERT, DL_CMD_ASSERT},
                      {VTP_HEADER_WAIT, DL_CMD_SLEEP}, {VTP_HEADER_UPPER_WAIT, DL_CMD_SLEEP},
                      {VTP_HEADER_SLEEP, DL_CMD_SLEEP}, {VTP_HEADER_UPPER_SLEEP, DL_CMD_SLEEP},
                      {VTP_HEADER_V, DL_CMD_VERSION}, {VTP_HEADER_UPPER_V, DL_CMD_VERSION},
                      {VTP_HEADER_VER, DL_CMD_VERSION}, {VTP_HEADER_UPPER_VER, DL_CMD_VERSION},
                      {VTP_HEADER_SC_VER_BGN, SCRIPTS_VER_BEGIN}, {VTP_HEADER_SC_VER_END, SCRIPTS_VER_END},
                      {VTP_HEADER_T_EC1_VER_BGN, T_EC1_VER_BEGIN}, {VTP_HEADER_T_EC1_VER_END, T_EC1_VER_END},
                      {VTP_HEADER_R_EC1_VER_BGN, R_EC1_VER_BEGIN}, {VTP_HEADER_R_EC1_VER_END, R_EC1_VER_END},
                      {VTP_HEADER_T_EC2_VER_BGN, T_EC2_VER_BEGIN}, {VTP_HEADER_T_EC2_VER_END, T_EC2_VER_END},
                      {VTP_HEADER_R_EC2_VER_BGN, R_EC2_VER_BEGIN}, {VTP_HEADER_R_EC2_VER_END, R_EC2_VER_END},
                      {VTP_HEADER_SC_CMD_BGN, SCRIPTS_COMMANDS_BEGIN}, {VTP_HEADER_SC_CMD_END, SCRIPTS_COMMANDS_END},
                      {VTP_HEADER_T_EC1_BGN, T_EC1_BEGIN}, {VTP_HEADER_T_EC1_END, T_EC1_END},
                      {VTP_HEADER_R_EC1_BGN, R_EC1_BEGIN}, {VTP_HEADER_R_EC1_END, R_EC1_END},
                      {VTP_HEADER_T_EC2_BGN, T_EC2_BEGIN}, {VTP_HEADER_T_EC2_END, T_EC2_END},
                      {VTP_HEADER_R_EC2_BGN, R_EC2_BEGIN}, {VTP_HEADER_R_EC2_END, R_EC2_END},
                      {VTP_HEADER_PTH_020000_VER_BGN, PATCH_020000_VER_BEGIN},
                      {VTP_HEADER_PTH_020000_VER_END, PATCH_020000_VER_END},
                      {VTP_HEADER_PTH_020100_VER_BGN, PATCH_020100_VER_BEGIN},
                      {VTP_HEADER_PTH_020100_VER_END, PATCH_020100_VER_END},
                      {VTP_HEADER_PTH_020200_VER_BGN, PATCH_020200_VER_BEGIN},
                      {VTP_HEADER_PTH_020200_VER_END, PATCH_020200_VER_END},
                      {VTP_HEADER_PTH_020000_BGN, PATCH_020000_BEGIN},
                      {VTP_HEADER_PTH_020000_END, PATCH_020000_END},
                      {VTP_HEADER_PTH_020100_BGN, PATCH_020100_BEGIN},
                      {VTP_HEADER_PTH_020100_END, PATCH_020100_END},
                      {VTP_HEADER_PTH_020200_BGN, PATCH_020200_BEGIN},
                      {VTP_HEADER_PTH_020200_END, PATCH_020200_END}}
                    );
}

static DL_CMD_TYPE parseHeader(char *pBuffer)
{
    unordered_map<string, DL_CMD_TYPE>::iterator header = g_headers.find(string(pBuffer));
    if (g_headers.end() != header) {
        return header->second;
    } else {
        return DL_CMD_INVALID;
    }
}

static ESESTATUS tryExecCmd(const char *cmd, int cmdLen, DL_CMD_TYPE cmdType, bool *pDoChkCmdRsp,
                            ExecCmdFun execCmdFun, DownloadCmdOp *pDlCmdOp)
{
    ESESTATUS status = ESESTATUS_SUCCESS;
    if (DL_CMD_7816_4_APDU == cmdType) {
        status = execCmdFun(cmd, cmdLen, pDlCmdOp);
        if ((ESESTATUS_SUCCESS != status) ||
            ((pDlCmdOp->rspLen >= 2) && // pDlCmdOp->rspLen >= 2
             (pDlCmdOp->rsp[pDlCmdOp->rspLen - 2] != 0x90 || // rspLen - 2, sw1 0x90
              pDlCmdOp->rsp[pDlCmdOp->rspLen - 1] != 0x00))) { // rspLen - 1, sw2 0x00
            *pDoChkCmdRsp = true;
            status = ESESTATUS_SUCCESS;
        }
    } else if (DL_CMD_NCI == cmdType) {
        if ((strcmp(cmd, NCI20_RESET_CMD) == 0)
                || (strcmp(cmd, NCI20_INIT_CMD) == 0)) {
            // ignore the NCI RESET and INIT command
        } else {
            status = execCmdFun(cmd, cmdLen, pDlCmdOp);
        }
    } else if (DL_CMD_SLEEP == cmdType) {
        usleep(stod(cmd) * 1000);  // sleep cmd * 1000us
        TMS_LOG_W(g_tag, "%s: sleep %s ms", __FUNCTION__, cmd);
    } else if (DL_CMD_T1 == cmdType) {
        // Currently, ignore T1 cmd
#ifdef DEBUG_FLAG
        TMS_LOG_W(g_tag, "%s: ignore T1 cmd[%s]", __FUNCTION__, cmd);
#endif
    } else if (DL_CMD_ASSERT == cmdType) {
        if (*pDoChkCmdRsp) {
            *pDoChkCmdRsp = false;
            int len = cmdLen;
            if (1 == len) {
                if (('x' == cmd[0]) ||
                    ('X' == cmd[0])) {
                    // ignore result, continue
                } else {
                    TMS_LOG_W(g_tag, "%s: invalid assert[%s]", __FUNCTION__, cmd);
                    status = ESESTATUS_INVALID_PARAMETER;
                }
            } else {
                if ((pDlCmdOp->rspLen >= 2) && (len >= 4)) {  // rspLen >= 2 len >= 4
                    uint8_t sw1sw2[2] = {0x00};
                    if (!Cstr2hex(&cmd[len - 4], 4, sw1sw2, 2)) {  // Cstr2hex(&cmd[len - 4], 4, sw1sw2, 2)
                        TMS_LOG_E(g_tag, "%s invalid Hex format cmd: %s", __FUNCTION__, cmd);
                        // ignore check result
                    } else {
                        if (sw1sw2[0] == pDlCmdOp->rsp[pDlCmdOp->rspLen - 2]  // pDlCmdOp->rspLen - 2
                                && sw1sw2[1] == pDlCmdOp->rsp[pDlCmdOp->rspLen - 1]) {
                            // check result successfully, continue
                        } else {
                            TMS_LOG_W(g_tag, "%s: assert fail, assert[%s]", __FUNCTION__, cmd);
                            status = ESESTATUS_FATAL_ERROR;
                        }
                    }
                } else {
                    TMS_LOG_W(g_tag, "%s: assert fail, assert[%s]", __FUNCTION__, cmd);
                    status = ESESTATUS_FATAL_ERROR;
                }
            }
        } else {
#ifdef DEBUG_FLAG
            TMS_LOG_W(g_tag, "%s: ignore assert cmd[%s]", __FUNCTION__, cmd);
#endif
            // ignore assert. eg. assert 9000
        }
    } else {
        // cmdType is T1 or invalid, ignore and continue
        TMS_LOG_W(g_tag, "%s: ignore cmd[%d, %s]", __FUNCTION__, cmdType, cmd);
    }

    return status;
}

static uint64_t ParseVersion(string str)
{
    uint8_t *a = nullptr;
    uint64_t cosVer = INVALID_COS_VER;
    int len = (int)str.length();
    if (len <= 0) {
        return cosVer;
    }
    if (str.at(len - 1) == '\r') {
        len = len - 1;
    }
    size_t index = str.find("VER");
    if (index != string::npos) {
        str = str.substr(index + 3, len);  // index + 3
    } else {
        index = str.find('V');
        if (index != string::npos) {
            str = str.substr(index + 1, len);
        } else {
            TMS_LOG_E(g_tag, "VPT COS version parse fail: %s", str.c_str());
            // Invalid VPT FW version NO, do nothing
            cosVer = INVALID_COS_VER;
            goto cleanup;
        }
    }

    len = (int)str.length() / 2;  // str.length() / 2
    a = new uint8_t[len];
    if (!Str2hex(str, a, len)) {
        TMS_LOG_E(g_tag, "VPT COS version parse fail: %s", str.c_str());
        // Invalid VPT FW version NO, do nothing
        cosVer = INVALID_COS_VER;
    } else {
        cosVer = 0;
        for (int i = 0; i < len; i++) {
            cosVer = cosVer | ((static_cast<uint64_t>(a[i])) << ((len - 1 - i) * 8));  // 8 bit
        }
        TMS_LOG_I(g_tag, "%s: VTP cosVer = %010" PRIx64, __FUNCTION__, (uint64_t)cosVer);
    }

cleanup:
    delete[] a;

    return cosVer;
}

static bool Str2hex(string str, uint8_t *data, int dataLen)
{
    return Cstr2hex(str.c_str(), str.length(), data, dataLen);
}

bool Cstr2hex(const char *str, int strLen, uint8_t *data, int dataLen)
{
    static uint8_t hexBaseUpper = 'A' - 10;
    static uint8_t hexBaseLower = 'a' - 10;
    uint8_t *px = (uint8_t *)str;
    int len = strLen;
    // '\r' may be contained in str.
    if (len % 2 != 0) {  // len % 2
        len = len - 1;
    }
    UNUSED(dataLen);
    for (int j = 0; j < len - 1;) {
        uint8_t a = 0, b = 0;
        if (px[j] >= '0' && px[j] <= '9') {
            a = px[j] - '0';
        } else if (px[j] >= 'A' && px[j] <= 'F') {
            a = px[j] - hexBaseUpper;
        } else if (px[j] >= 'a' && px[j] <= 'f') {
            a = px[j] - hexBaseLower;
        } else {
            TMS_LOG_W(g_tag, "%s invalid Hex format: %c", __FUNCTION__, px[j]);
            return false;
        }

        if (px[j + 1] >= '0' && px[j + 1] <= '9') {
            b = px[j + 1] - '0';
        } else if (px[j + 1] >= 'A' && px[j + 1] <= 'F') {
            b = px[j + 1] - hexBaseUpper;
        } else if (px[j + 1] >= 'a' && px[j + 1] <= 'f') {
            b = px[j + 1] - hexBaseLower;
        } else {
            TMS_LOG_W(g_tag, "%s invalid Hex format: %c", __FUNCTION__, px[j + 1]);
            return false;
        }

        data[j / 2] = (a << 4) + b;  // data[j / 2] = (a << 4)
        j += 2;  // j+2
    }
    return true;
}

static int ReadBinSC(string fileName, int readLen, char **ppReadBuff)
{
    char *pReadBuff = nullptr;
    char realPath[PATH_MAX + 1] = {0};
    ifstream ifs;
    int len = 0;
    int count = 0;

    if (fileName.length() > PATH_MAX || realpath(fileName.c_str(), realPath) == nullptr) {
        TMS_LOG_E(g_tag, "%s: invalid path", __FUNCTION__);
        return -1;
    }
    ifs.open(realPath, ifstream::in | ifstream::binary);
    if (!ifs.is_open()) {
        TMS_LOG_E(g_tag, "%s exit, open path:%s error", __FUNCTION__, realPath);
        return -1;
    }

    ifs.seekg(0, ifs.end);
    if (0 == readLen) {
        len = ifs.tellg();
    } else {
        len = readLen;
    }
    ifs.seekg(0, ifs.beg);

    // calloc len + 1 to fixed last cmd's last char is overrided by '\0'
    // when the last line no LR and (or) CR.
    pReadBuff = (char *)calloc(1, len + 1);
    if (nullptr == pReadBuff) {
        TMS_LOG_E(g_tag, "%s: calloc failed", __FUNCTION__);
        ifs.close();
        return -1;
    }
    *ppReadBuff = pReadBuff;

    do {
        ifs.read(pReadBuff, len - count);
        count += ifs.gcount();
    } while (!ifs.eof() && count < len);
    ifs.close();

    return count;
}

/**
 * Parse script length bytes.
 * If length is 0, parse the whole script.
 */
static VtpBinDlCmdT *ParseBinSC(string fileName, int readLen, char **ppReadBuff)
{
    char *pReadBuff = nullptr;
    VtpBinDlCmdT *pHead = nullptr;
    VtpBinDlCmdT *pTail = nullptr;
    int start = 0;
    int count = ReadBinSC(fileName, readLen, ppReadBuff);
    if (count <= 0) {
        return nullptr;
    }
    pReadBuff = *ppReadBuff;
    InitHeadersMap();

    for (int i = 0; i < count - 1; i++) {
        pReadBuff[i] = pReadBuff[i + 1] ^ pReadBuff[i];
#ifdef DEBUG_FLAG
        TMS_LOG_D(g_tag, "%s: [%d]=%02X", __FUNCTION__, i, pReadBuff[i]);
#endif

        if (pReadBuff[i] == '#') {
            while (i < count  - 1) {
                i++;
                if (i < count - 1) {
                    pReadBuff[i] = pReadBuff[i + 1] ^ pReadBuff[i];
                } else {
                    pReadBuff[i] = pReadBuff[i] ^ 0x84;
                }
#ifdef DEBUG_FLAG
                TMS_LOG_D(g_tag, "%s: [%d]=%02X", __FUNCTION__, i, pReadBuff[i]);
#endif
                if (pReadBuff[i] == '\n') {
                    start = i + 1;
                    break;
                }
            }
            continue;
        } else {
            char tmpIplus1 = pReadBuff[i + 1] ^ pReadBuff[i + 2];
            if ((pReadBuff[i] == '\r') && (tmpIplus1 == '\n') && (start == i)) {
                i++;
                pReadBuff[i] = tmpIplus1;
                start = i + 1;
#ifdef DEBUG_FLAG
                TMS_LOG_D(g_tag, "%s: empty line", __FUNCTION__);
#endif
                continue;
            } else if ((pReadBuff[i] == '\n') && (start == i)) {
                start = i + 1;
#ifdef DEBUG_FLAG
                TMS_LOG_D(g_tag, "%s: empty line", __FUNCTION__);
#endif
                continue;
            } else {
                i = parseCmd(&pHead, &pTail, pReadBuff, i, count);
                start = i + 1;
            }
        }
    }
    g_headers.clear();

    return pHead;
}

static uint64_t GetCosVer(VtpBinDlCmdT *pHeadChipInfo, VtpBinDlCmdT *pHeadSC)
{
    // Check script format
    while ((nullptr != pHeadChipInfo) && (nullptr != pHeadSC)) {
#ifdef DEBUG_FLAG2
        TMS_LOG_D(g_tag, "%s: pHeadSC->cmdType = %d, ", __FUNCTION__, pHeadSC->cmdType);
        TMS_LOG_D(g_tag, "%s: pHeadChipInfo->cmdType = %d, ", __FUNCTION__, pHeadChipInfo->cmdType);
#endif
        if (pHeadChipInfo->cmdType == pHeadSC->cmdType) {
            pHeadChipInfo = pHeadChipInfo->pNext;
            pHeadSC = pHeadSC->pNext;
            if ((DL_CMD_VERSION == pHeadChipInfo->cmdType) &&
                (pHeadSC->cmdType == pHeadChipInfo->cmdType)) {
                // Point to next node, such as, T_EC1_VER_BEGIN
                pHeadChipInfo = pHeadChipInfo->pNext;
                pHeadSC = pHeadSC->pNext;
                break;
            }
        } else if (DL_CMD_VERSION == pHeadSC->cmdType) {
            // adapte old script format
            string firstLine = string("VER") + string(pHeadSC->cmd);
            return ParseVersion(firstLine);
        } else {
            TMS_LOG_E(g_tag, "%s invalid old scritp format", __FUNCTION__);
            return INVALID_COS_VER;
        }
    }

    if (nullptr == pHeadChipInfo) {
        TMS_LOG_E(g_tag, "%s chip info exception", __FUNCTION__);
        return INVALID_COS_VER;
    }

#ifdef DEBUG_FLAG2
    TMS_LOG_D(g_tag, "%s: pHeadChipInfo->cmdType = %d, ", __FUNCTION__, pHeadChipInfo->cmdType);
#endif
    while ((nullptr != pHeadSC) && (nullptr != pHeadSC->pNext)) {
#ifdef DEBUG_FLAG2
        TMS_LOG_D(g_tag, "%s: pHeadSC->cmdType = %d, ", __FUNCTION__, pHeadSC->cmdType);
#endif
        if (pHeadChipInfo->cmdType == pHeadSC->cmdType &&
            DL_CMD_VERSION == pHeadSC->pNext->cmdType) {
            string firstLine = string("VER") + string(pHeadSC->pNext->cmd);
            return ParseVersion(firstLine);
        } else {
            pHeadSC = pHeadSC->pNext;
        }
    }

    return INVALID_COS_VER;
}

static VtpBinDlCmdT *GetCmdsHead(VtpBinDlCmdT **ppHeadChipInfo, VtpBinDlCmdT *pHeadSC)
{
    VtpBinDlCmdT *pHeadChipInfo = *ppHeadChipInfo;
    // Check script format
    while ((nullptr != pHeadChipInfo) && (nullptr != pHeadSC)) {
#ifdef DEBUG_FLAG2
        TMS_LOG_D(g_tag, "%s: pHeadSC->cmdType = %d, cmd = %s", __FUNCTION__, pHeadSC->cmdType, pHeadSC->cmd);
        TMS_LOG_D(g_tag, "%s: pHeadChipInfo->cmdType = %d, ", __FUNCTION__, pHeadChipInfo->cmdType);
#endif
        if (pHeadChipInfo->cmdType == pHeadSC->cmdType) {
            pHeadChipInfo = pHeadChipInfo->pNext;
            pHeadSC = pHeadSC->pNext;
            if ((DL_CMD_VERSION == pHeadChipInfo->cmdType) &&
                (pHeadSC->cmdType == pHeadChipInfo->cmdType)) {
                // Point to next node, such as, T_EC1_BEGIN
                pHeadChipInfo = pHeadChipInfo->pNext;
                pHeadSC = pHeadSC->pNext;
                break;
            }
        } else {
            // DL_CMD_VERSION == pHeadSC->cmdType, DL script,
            // or has not DL_CMD_VERSION, rvc script.
            // adapte old script format
            return pHeadSC;
        }
    }

    if ((nullptr == pHeadChipInfo) || (nullptr == pHeadChipInfo->pNext)) {
        TMS_LOG_E(g_tag, "%s: chip info exception", __FUNCTION__);
        return nullptr;
    }

#ifdef DEBUG_FLAG2
    TMS_LOG_D(g_tag, "%s: pHeadChipInfo->cmdType = %d, ", __FUNCTION__, pHeadChipInfo->cmdType);
#endif
    bool findScCmdBgn = false;
    while ((nullptr != pHeadSC) && (nullptr != pHeadSC->pNext)) {
#ifdef DEBUG_FLAG2
        TMS_LOG_D(g_tag, "%s: pHeadSC->cmdType = %d, cmd = %s", __FUNCTION__, pHeadSC->cmdType, pHeadSC->cmd);
#endif
        // Find SCRIPTS_COMMANDS_BEGIN
        if (!findScCmdBgn && (SCRIPTS_COMMANDS_BEGIN != pHeadSC->cmdType)) {
            pHeadSC = pHeadSC->pNext;
            continue;
        } else {
            findScCmdBgn = true;
        }

        // Find DL script block
        if (pHeadChipInfo->cmdType != pHeadSC->cmdType) {
            pHeadSC = pHeadSC->pNext;
            continue;
        } else {
            pHeadSC = pHeadSC->pNext;
            *ppHeadChipInfo = pHeadChipInfo->pNext;
            return pHeadSC;
        }
    }

    return nullptr;
}

static bool AddVtpBinDlCmdList(VtpBinDlCmdT **ppHead, VtpBinDlCmdT **ppTail,
                               char *cmd, DL_CMD_TYPE cmdType)
{
    VtpBinDlCmdT *pNode = (VtpBinDlCmdT *)calloc(1, sizeof(VtpBinDlCmdT));
    if (nullptr == pNode) {
        TMS_LOG_E(g_tag, "%s calloc memory failed", __FUNCTION__);
        return false;
    } else {
        pNode->cmdType = cmdType;
        if (nullptr != cmd) {
            pNode->cmd = cmd;
            pNode->cmdLen = strlen(cmd);
        }
        pNode->pNext = nullptr;

        if (nullptr == *ppHead) {
            *ppHead = pNode;
            *ppTail = pNode;
        } else {
            (*ppTail)->pNext = pNode;
            *ppTail = pNode;
        }
        return true;
    }
}

static void DestroyVtpBinDlCmdList(VtpBinDlCmdT *pHead, bool deedFree)
{
    while (nullptr != pHead) {
        VtpBinDlCmdT *tmp = pHead;
        pHead = pHead->pNext;
        if (deedFree && (nullptr != tmp->cmd)) {
            free(tmp->cmd);
        }
        free(tmp);
    }
}

VtpBinDlCmdT *InitChipInfoVer(ChipInfo chipInfo)
{
    VtpBinDlCmdT *pHead = nullptr;
    VtpBinDlCmdT *pTail = nullptr;
    // add SCRIPTS_VER_BEGIN
    bool ret = AddVtpBinDlCmdList(&pHead, &pTail, nullptr, SCRIPTS_VER_BEGIN);
    // add SCRIPTS_VER, 010000
    ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, DL_CMD_VERSION);

    if (INVALID_FW_VER == chipInfo.cosBaseVer) {
        if ((CHIP_EC1 == chipInfo.chipType) && (CHIP_T_KEY == chipInfo.chipKeyType)) {
            // add T_EC1_VER_BEGIN
            ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, T_EC1_VER_BEGIN);
        } else if ((CHIP_EC1 == chipInfo.chipType) && (CHIP_R_KEY == chipInfo.chipKeyType)) {
            // add R_EC1_VER_BEGIN
            ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, R_EC1_VER_BEGIN);
        } else if ((CHIP_EC2 == chipInfo.chipType) && (CHIP_T_KEY == chipInfo.chipKeyType)) {
            // add T_EC2_VER_BEGIN
            ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, T_EC2_VER_BEGIN);
        } else if ((CHIP_EC2 == chipInfo.chipType) && (CHIP_R_KEY == chipInfo.chipKeyType)) {
            // add R_EC2_VER_BEGIN
            ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, R_EC2_VER_BEGIN);
        } else {
            ret = false;
            TMS_LOG_E(g_tag, "%s NFCC chip info exception", __FUNCTION__);
        }
    } else {
        if (COS_VER_MASTER_020000 == chipInfo.cosBaseVer) {
            ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, PATCH_020000_VER_BEGIN);
        } else if (COS_VER_MASTER_020100 == chipInfo.cosBaseVer) {
            ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, PATCH_020100_VER_BEGIN);
        } else if (COS_VER_MASTER_020200 == chipInfo.cosBaseVer) {
            ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, PATCH_020200_VER_BEGIN);
        } else {
            ret = false;
            TMS_LOG_E(g_tag, "%s SE chip info exception", __FUNCTION__);
        }
    }

    if (!ret) {
        DestroyVtpBinDlCmdList(pHead, false);
        pHead = nullptr;
    }
    return pHead;
}

VtpBinDlCmdT *InitChipInfoCmds(ChipInfo chipInfo)
{
    VtpBinDlCmdT *pHead = nullptr;
    VtpBinDlCmdT *pTail = nullptr;
    // add SCRIPTS_VER_BEGIN
    bool ret = AddVtpBinDlCmdList(&pHead, &pTail, nullptr, SCRIPTS_VER_BEGIN);
    // add SCRIPTS_VER, 010000
    ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, DL_CMD_VERSION);

    if (INVALID_FW_VER == chipInfo.cosBaseVer) {
        if ((CHIP_EC1 == chipInfo.chipType) && (CHIP_T_KEY == chipInfo.chipKeyType)) {
            // add T_EC1_VER_BEGIN
            ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, T_EC1_BEGIN);
            ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, T_EC1_END);
        } else if ((CHIP_EC1 == chipInfo.chipType) && (CHIP_R_KEY == chipInfo.chipKeyType)) {
            // add R_EC1_VER_BEGIN
            ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, R_EC1_BEGIN);
            ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, R_EC1_END);
        } else if ((CHIP_EC2 == chipInfo.chipType) && (CHIP_T_KEY == chipInfo.chipKeyType)) {
            // add T_EC2_VER_BEGIN
            ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, T_EC2_BEGIN);
            ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, T_EC2_END);
        } else if ((CHIP_EC2 == chipInfo.chipType) && (CHIP_R_KEY == chipInfo.chipKeyType)) {
            // add R_EC2_VER_BEGIN
            ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, R_EC2_BEGIN);
            ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, R_EC2_END);
        } else {
            ret = false;
            TMS_LOG_E(g_tag, "%s NFCC chip info exception", __FUNCTION__);
        }
    } else {
        if (COS_VER_MASTER_020000 == chipInfo.cosBaseVer) {
            ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, PATCH_020000_BEGIN);
            ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, PATCH_020000_END);
        } else if (COS_VER_MASTER_020100 == chipInfo.cosBaseVer) {
            ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, PATCH_020100_BEGIN);
            ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, PATCH_020100_END);
        } else if (COS_VER_MASTER_020200 == chipInfo.cosBaseVer) {
            ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, PATCH_020200_BEGIN);
            ret = ret && AddVtpBinDlCmdList(&pHead, &pTail, nullptr, PATCH_020200_END);
        } else {
            ret = false;
            TMS_LOG_E(g_tag, "%s SE chip info exception", __FUNCTION__);
        }
    }

    if (!ret) {
        DestroyVtpBinDlCmdList(pHead, false);
        pHead = nullptr;
    }
    return pHead;
}

bool getVaildFileNameByPth(char *fileName, int fileNameLen,
                           const char *keyName, int keyNameLen, string defaultVal,
                           vector<string>& searchPth)
{
    int err;

    UNUSED(keyNameLen);
    string cosFn = EseConfig::getString(keyName, defaultVal);
    if (cosFn.length() == 0) {
        return false;
    }

    if (!IsEndWith(cosFn.c_str(), cosFn.length(), ".bin", strlen(".bin"))) {
        cosFn += ".bin";
    }

    string cosFileName = searchFilePath(cosFn, searchPth);
    if (cosFileName.length() == 0) {
        cosFn = cosFn.substr(0, cosFn.length() - strlen(".bin"));
        cosFileName = searchFilePath(cosFn, searchPth);
        if (cosFileName.length() == 0) {
            return false;
        }
    }

    err = strncpy_s(fileName, fileNameLen, cosFileName.c_str(), cosFileName.length() + 1);
    if (EOK != err) {
        TMS_LOG_E(g_tag, "%s: strncpy_s err. ret:%d", __FUNCTION__, err);
    }
    TMS_LOG_D(g_tag, "fileName = %s, keyName = %s", fileName, keyName);
    return true;
}

bool GetVaildFileName(char *fileName, int fileNameLen,
                      const char *keyName, int keyNameLen, string defaultVal,
                      bool isNfcDir)
{
    vector<string> searchPth;
    if (isNfcDir) {
        searchPth = {"/data/vendor/nfc/",

                     "/vendor/etc/",
                     "/odm/etc/"
                    };
    } else {
        searchPth = {"/data/vendor/secure_element/",

                     "/vendor/etc/",
                     "/odm/etc/"
                    };
    }

    return getVaildFileNameByPth(fileName, fileNameLen,
                                 keyName, keyNameLen, defaultVal, searchPth);
}

/* DESCRIPTION
 * Get COS or FW version from VTP file.
 *
 * Parameters
 * fileName VTP full file name, eg. /data/vendor/nfc/PD1906_NFCC_VTP.txt.
 *
 * RETURN VALUE
 * return INVALID_COS_VER if the file cannot open, or return 0 if version number is invalid.
 * Otherwise, return a valid version number.
 */
uint64_t GetCosVerFromVtp(char *fileName, int fileNameLen, ChipInfo chipInfo)
{
    UNUSED(fileNameLen);
    uint64_t cosVer = INVALID_COS_VER;
    if (IsEndWith(fileName, strlen(fileName), ".bin", strlen(".bin"))) {
        char *pReadBuff = nullptr;
        VtpBinDlCmdT *pHead = nullptr;
        VtpBinDlCmdT *pHeadChipInfo = InitChipInfoVer(chipInfo);
        if (nullptr == pHeadChipInfo) {
            cosVer = INVALID_COS_VER;
        } else {
            pHead = ParseBinSC(string(fileName), 2000, &pReadBuff);
            if (nullptr == pHeadChipInfo) {
                cosVer = INVALID_COS_VER;
            } else {
                cosVer = GetCosVer(pHeadChipInfo, pHead);
            }
        }
        DestroyVtpBinDlCmdList(pHead, false);
        DestroyVtpBinDlCmdList(pHeadChipInfo, false);
        if (nullptr != pReadBuff) {
            free(pReadBuff);
        }
        return cosVer;
    } else {
        ifstream ifs;
        string str;
        char realPath[PATH_MAX + 1] = {0};

        if (strlen(fileName) > PATH_MAX || realpath(fileName, realPath) == nullptr) {
            TMS_LOG_E(g_tag, "%s: invalid path", __FUNCTION__);
            return INVALID_COS_VER;
        }

        ifs.open(realPath, ios::in);
        if (!ifs.is_open()) {
            TMS_LOG_E(g_tag, "%s exit, open path:%s error", __FUNCTION__, fileName);
            cosVer = INVALID_COS_VER;
        } else {
            getline(ifs, str);
            string::iterator end = std::remove(str.begin(), str.end(), ' ');
            str.erase(end, str.end());

            cosVer = ParseVersion(str);
        }
        ifs.close();
        return cosVer;
    }
}

bool chkEseSoftReset(DownloadCmdOp *pDlCmd)
{
    if ((pDlCmd->rspLen < 2) || // rspLen < 2
        (0x90 != pDlCmd->rsp[pDlCmd->rspLen - 2]) || // rspLen - 2
        (0x00 != pDlCmd->rsp[pDlCmd->rspLen - 1])) {
        return false;
    }

    return true;
}

ESESTATUS execApduCmdChkRes(const char *cmd, int cmdLen, void *arg)
{
    ESESTATUS status = ESESTATUS_INVALID_PARAMETER;
    int count = 0;
    DownloadCmdOp *pDlCmdOp = NULL;
    ITmsPhAbs *pTmsPhAbs = NULL;
    SeData cmdApdu;
    SeData rspApdu;

    if (nullptr == arg) {
        return status;
    }

    pDlCmdOp = (DownloadCmdOp *)arg;
    VtpParams *pVtpParams = (VtpParams *)pDlCmdOp->pParameters;
    if (NULL == pVtpParams) {
        return status;
    }
    pTmsPhAbs = pVtpParams->pTmsPhAbs;
    if (NULL == pTmsPhAbs) {
        return status;
    }

    rspApdu.len = 0;
    rspApdu.pData = NULL;
    cmdApdu.len = (unsigned int)cmdLen / 2;  // cmdLen / 2
    cmdApdu.pData = new uint8_t[cmdApdu.len];
    if (!Cstr2hex(cmd, cmdLen, cmdApdu.pData, cmdApdu.len)) {
        TMS_LOG_E(g_tag, "%s invalid Hex format cmd: %s", __FUNCTION__, cmd);
        goto cleanup;
    }

tryagain:
    if (!g_threadRunning) {
        TMS_LOG_W(g_tag, "%s exit thread", __FUNCTION__);
        goto cleanup;
    }
    if (rspApdu.pData != NULL) {
        free(rspApdu.pData);
        rspApdu.len = 0;
        rspApdu.pData = NULL;
    }
    status = pTmsPhAbs->transmit(&cmdApdu, &rspApdu);
    if (ESESTATUS_SUCCESS != status ||
        (rspApdu.len < 2) || // rspApdu.len < 2
        (rspApdu.pData[rspApdu.len - 2] != 0x90) || // rspApdu.len - 2
        (rspApdu.pData[rspApdu.len - 1] != 0x00)) {
        printHexPacket(g_tag, strlen(g_tag), "RECV", 4, rspApdu.pData, rspApdu.len);  // length 4
        TMS_LOG_E(g_tag, "%s: %s rsp not 9000 or status = %x error",
                  __FUNCTION__, cmd, status);
        if ((count < 2) && (rspApdu.len >= 2)) {  // rspApdu.len >= 2
            if ((rspApdu.pData[rspApdu.len - 2] == 0x65) && // rspApdu.len - 2
                (rspApdu.pData[rspApdu.len - 1] == 0x04)) {
                // Check CRC error, try again
                TMS_LOG_E(g_tag, "%s: sw1sw2=0x6504, try again", __FUNCTION__);
                count++;
                goto tryagain;
            } else if (0 == strcmp(cmd, "00F0010000") &&
                       (rspApdu.pData[rspApdu.len - 2] == 0x6F) && // rspApdu.len - 2
                       (rspApdu.pData[rspApdu.len - 1] == 0x00)) {
                // "00F0010000" is the first cmd, and SE BL will return 6F00, try again.
                count++;
                goto tryagain;
            }
        }
        status = (ESESTATUS_SUCCESS != status ? status : ESESTATUS_FAILED);
    } else {
        status = ESESTATUS_SUCCESS;
    }

    if (rspApdu.len >= 2) {  // rspApdu.len >= 2
        if ((pDlCmdOp->rsp != NULL) && NCI_RSP_LEN_MAX >= rspApdu.len) {
            int err = memcpy_s(pDlCmdOp->rsp, NCI_RSP_LEN_MAX, rspApdu.pData, rspApdu.len);
            if (err != EOK) {
                TMS_LOG_E(g_tag, "%s: memcpy_s err. ret:%d", __FUNCTION__, err);
            }
            pDlCmdOp->rspLen = (int)rspApdu.len;
        } else {
            TMS_LOG_D(g_tag, "%s pDlCmdOp->rspLen = %d", __FUNCTION__, pDlCmdOp->rspLen);
        }
    } else {
        pDlCmdOp->rspLen = 0;
    }

cleanup:
    delete[] cmdApdu.pData;
    if (rspApdu.pData != NULL) {
        free(rspApdu.pData);
    }
    return status;
}

static bool tryNewRspMem(DownloadCmdOp *pDlCmdOp, ESESTATUS &status)
{
    bool isNewRspMem = false;
    if (nullptr == pDlCmdOp->rsp) {
        pDlCmdOp->rsp = new uint8_t[NCI_RSP_LEN_MAX];
        pDlCmdOp->rspLen = NCI_RSP_LEN_MAX;
        if (nullptr == pDlCmdOp->rsp) {
            TMS_LOG_E(g_tag, "%s: new res memory failed", __FUNCTION__);
            status = ESESTATUS_MEM_EXCEPTION;
        } else {
            isNewRspMem = true;
        }
    }
    return isNewRspMem;
}

static void tryDeleteRspMem(DownloadCmdOp *pDlCmdOp)
{
    if (nullptr != pDlCmdOp->rsp) {
        delete[] pDlCmdOp->rsp;
        pDlCmdOp->rsp = nullptr;
        pDlCmdOp->rspLen = 0;
    }
}


static bool execBinCmds(VtpBinDlCmdT *pHead, DownloadCmdOp *pDlCmdOp,
                        ChipInfo *pChipInfo, VtpBinDlCmdT **ppPreCmd, ESESTATUS &status)
{
    bool res = true;
    bool doChkCmdRsp = false;
    VtpBinDlCmdT *pHeadChipInfo = nullptr;
    VtpBinDlCmdT *pTailChipInfo = nullptr;
    ITmsPhAbs *pTmsPhAbs = ((VtpParams *)pDlCmdOp->pParameters)->pTmsPhAbs;
    if (nullptr == pTmsPhAbs) {
        return status;
    }
    ExecCmdFun execCmdFun = pTmsPhAbs->getExecCmdFun();
    // status == ESESTATUS_SUCCESS
    bool isNewRspMem = tryNewRspMem(pDlCmdOp, status);
    if (ESESTATUS_SUCCESS != status) {
        return false;
    }

    if (nullptr != pChipInfo) {
        pHeadChipInfo = InitChipInfoCmds(*pChipInfo);
        pTailChipInfo = pHeadChipInfo;
        pHead = GetCmdsHead(&pTailChipInfo, pHead);
#ifdef DEBUG_FLAG2
        TMS_LOG_D(g_tag, "%s: pTailChipInfo->cmdType = %d, ", __FUNCTION__, pTailChipInfo->cmdType);
#endif
    }
    status = ESESTATUS_SUCCESS;
    VtpBinDlCmdT *pPreCmd = nullptr;
    while ((nullptr != pHead) &&
           ((nullptr == pTailChipInfo) || (pHead->cmdType != pTailChipInfo->cmdType))) {
#ifdef DEBUG_FLAG
        TMS_LOG_D(g_tag, "%s: cmdType = %d, cmdLen = %u, cmd = %s",
                  __FUNCTION__, pHead->cmdType, pHead->cmdLen, pHead->cmd);
#endif
        if (ESESTATUS_SUCCESS == status) {
            status = tryExecCmd(pHead->cmd, pHead->cmdLen, pHead->cmdType, &doChkCmdRsp,
                                execCmdFun, pDlCmdOp);
            if (DL_CMD_ASSERT != pHead->cmdType) {
                pPreCmd = pHead;
            }
            pHead = pHead->pNext;
        } else {
            *ppPreCmd = pPreCmd;
            res = false;
            break;
        }
    }
    if (isNewRspMem) {
        tryDeleteRspMem(pDlCmdOp);
    }
    DestroyVtpBinDlCmdList(pHeadChipInfo, false);
    return res;
}

ESESTATUS tryPT2SeBl(DownloadCmdOp *pDlCmdOp, VtpParams *pVtpParams)
{
    ExecCmdFun execCmdFun;
    // PT, pass-through transmission
    bool needPT2SeBl = false;
    ESESTATUS status = ESESTATUS_SUCCESS;
    ITmsPhAbs *pTmsPhAbs = pVtpParams->pTmsPhAbs;
    if (nullptr == pTmsPhAbs) {
        return ESESTATUS_INVALID_PARAMETER;
    }
    execCmdFun = pTmsPhAbs->getExecCmdFun();

    needPT2SeBl = pVtpParams->needPT2SeBl;

    if (needPT2SeBl) {
        for (int i = 0; i < 2; i++) {  // retry 2 times
            status = execCmdFun("00F0FF0200", strlen("00F0FF0200"), pDlCmdOp);
            if (ESESTATUS_SUCCESS != status) {
                if (i == 1) {
                    return ESESTATUS_SUCCESS != status ? status : ESESTATUS_FAILED;
                }
            } else {
                // reset the IFrame PCB No to 0, that is 5A00 not 5A40
                // When pass-through transmission, reset SE to SYNC and reInit ATR information
                if (pTmsPhAbs->getPhClass() == ITmsPhAbs::REE) {
#ifdef TMS_REE
                    TmsRee *pReeSE = (TmsRee *)pTmsPhAbs;
                    status = pReeSE->reeSEReset();
                    if (ESESTATUS_SUCCESS != status) {
                        return status;
                    }
#else
                    TMS_LOG_E(g_tag, "%s unsupport REE feature[seReset]", __FUNCTION__);
                    return status;
#endif // TMS_REE
                } else {
                    TMS_LOG_E(g_tag, "%s unsupport REE feature[seReset]", __FUNCTION__);
                }
                break;
            }
        }
    }
    return status;
}

static void PrintVtpDownloadData() {
    TMS_LOG_I(g_tag, "%s recorded FW DL startTimeLong = %ld", __FUNCTION__, g_nfcVtpDlDataCb.fwDLData.startTimeLong);
    TMS_LOG_I(g_tag, "%s recorded FW DL startTimeShort = %ld", __FUNCTION__, g_nfcVtpDlDataCb.fwDLData.startTimeShort);
    TMS_LOG_I(g_tag, "%s recorded FW DL shortTimeCount = %d", __FUNCTION__, g_nfcVtpDlDataCb.fwDLData.shortTimeCount);
    TMS_LOG_I(g_tag, "%s recorded FW DL longTimeCount = %d", __FUNCTION__, g_nfcVtpDlDataCb.fwDLData.longTimeCount);

    TMS_LOG_I(g_tag, "%s recorded BL DL startTimeLong = %ld", __FUNCTION__, g_nfcVtpDlDataCb.blDLData.startTimeLong);
    TMS_LOG_I(g_tag, "%s recorded BL DL startTimeShort = %ld", __FUNCTION__, g_nfcVtpDlDataCb.blDLData.startTimeShort);
    TMS_LOG_I(g_tag, "%s recorded BL DL shortTimeCount = %d", __FUNCTION__, g_nfcVtpDlDataCb.blDLData.shortTimeCount);
    TMS_LOG_I(g_tag, "%s recorded BL DL longTimeCount = %d", __FUNCTION__, g_nfcVtpDlDataCb.blDLData.longTimeCount);

    TMS_LOG_I(g_tag, "%s recorded COS patch DL startTimeLong = %ld", __FUNCTION__, g_cosPthDLData.startTimeLong);
    TMS_LOG_I(g_tag, "%s recorded COS patch DL startTimeShort = %ld", __FUNCTION__, g_cosPthDLData.startTimeShort);
    TMS_LOG_I(g_tag, "%s recorded COS patch DL shortTimeCount = %d", __FUNCTION__, g_cosPthDLData.shortTimeCount);
    TMS_LOG_I(g_tag, "%s recorded COS patch DL longTimeCount = %d", __FUNCTION__, g_cosPthDLData.longTimeCount);
}

static void ReadVtpDownloadData(char* dlName) {
    uint8_t *pBuffer = nullptr;
    uint16_t nbytes;
    string dataFile;
    bool *dataRead = nullptr;
    int fileStream;
    uint8_t retry = 3;

    TMS_LOG_D(g_tag, "%s enter.", __FUNCTION__);

    if (nullptr == dlName) {
        TMS_LOG_E(g_tag, "%s invaild param.", __FUNCTION__);
        return;
    }

    if (strcmp(dlName, VTP_DL_CPS_PTH_ROUTINE) == 0 && !g_seDlDataRead) {
        pBuffer   = (uint8_t*)&g_cosPthDLData;
        nbytes    = sizeof(g_cosPthDLData);
        dataFile  = VTP_DL_SE_DATA_STORAGE_FILE;
        dataRead = &g_seDlDataRead;
    } else if (strcmp(dlName, VTP_DL_CPS_PTH_ROUTINE) != 0 && !g_nfcDlDataRead) {
        pBuffer   = (uint8_t*)&g_nfcVtpDlDataCb;
        nbytes    = sizeof(g_nfcVtpDlDataCb);
        dataFile  = VTP_DL_NFC_DATA_STORAGE_FILE;
        dataRead = &g_nfcDlDataRead;
    } else {
        TMS_LOG_D(g_tag, "%s Data already init.", __FUNCTION__);
        return;
    }

    fileStream = open(dataFile.c_str(), O_RDONLY);
    if (fileStream >=0) {
        for (int i = 0; i < retry; i++) {
            size_t actualReadData = read(fileStream, pBuffer, nbytes);
            if (actualReadData > 0) {
                TMS_LOG_D(g_tag, "%s read file[%s] OK, data size=%zu.", __FUNCTION__, dataFile.c_str(), actualReadData);
                break;
            } else {
                TMS_LOG_E(g_tag, "%s failed to read file[%s]. retry=%d, error = %s", __FUNCTION__, dataFile.c_str(), i, strerror(errno));
                (void)memset_s(pBuffer, nbytes, 0x00, nbytes);
            }
        }
        close(fileStream);
    } else {
        TMS_LOG_E(g_tag, "%s file[%s] does not exist. error = %s.", __FUNCTION__, dataFile.c_str(), strerror(errno));
        (void)memset_s(pBuffer, nbytes, 0x00, nbytes);
    }
    *dataRead = true;
    TMS_LOG_D(g_tag, "%s exit.", __FUNCTION__, errno);
}

static void UpdateVtpDownloadData(char* dlName) {
    uint8_t *pBuffer = (uint8_t*)&g_nfcVtpDlDataCb;
    uint16_t nbytes = sizeof(g_nfcVtpDlDataCb);
    string dataFile = VTP_DL_NFC_DATA_STORAGE_FILE;

    if (nullptr == dlName) {
        TMS_LOG_E(g_tag, "%s invaild param.", __FUNCTION__);
        return;
    }

    if (strcmp(dlName, VTP_DL_CPS_PTH_ROUTINE) == 0) {
        pBuffer = (uint8_t*)&g_cosPthDLData;
        nbytes = sizeof(g_cosPthDLData);
        dataFile = VTP_DL_SE_DATA_STORAGE_FILE;
    }

    int fileStream = open(dataFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fileStream >= 0) {
        size_t actualWrittenData = write(fileStream, pBuffer, nbytes);
        if (actualWrittenData == nbytes) {
            TMS_LOG_D(g_tag, "%s write file[%s] OK, data size=%zu", __FUNCTION__, dataFile.c_str(), actualWrittenData);
        } else {
            TMS_LOG_E(g_tag, "%s failed to write file[%s]. error = %s.", __FUNCTION__, dataFile.c_str(), strerror(errno));
        }
        close(fileStream);
    } else {
        TMS_LOG_E(g_tag, "%s failed to open file[%s]. error = %s", __FUNCTION__, dataFile.c_str(), strerror(errno));
    }
}

static bool CheckAndInitVtpDlLmit(char* dlName, VtpDownloadData** actualData, VtpDownloadData* limitData) {
    bool res = false;
    if (!dlName || !actualData || !limitData) {
        TMS_LOG_E(g_tag, "%s invalid params.", __FUNCTION__);
        return res;
    }
    if (strcmp(dlName,VTP_DL_FW_ROUTINE) == 0) {
        limitData->startTimeLong  = FW_DL_LONG_TIME_LIMIT;
        limitData->startTimeShort = FW_DL_SHORT_TIME_LIMIT;
        limitData->longTimeCount  = FW_DL_LONG_TIME_COUNT_LIMIT;
        limitData->shortTimeCount = FW_DL_SHORT_TIME_COUNT_LIMIT;
        *actualData = &(g_nfcVtpDlDataCb.fwDLData);
        res = true;
    } else if (strcmp(dlName, VTP_DL_BL_ROUTINE) == 0) {
        limitData->startTimeLong  = BL_DL_LONG_TIME_LIMIT;
        limitData->startTimeShort = BL_DL_SHORT_TIME_LIMIT;
        limitData->longTimeCount  = BL_DL_LONG_TIME_COUNT_LIMIT;
        limitData->shortTimeCount = BL_DL_SHORT_TIME_COUNT_LIMIT;
        *actualData = &(g_nfcVtpDlDataCb.blDLData);
        res = true;
    } else if (strcmp(dlName, VTP_DL_CPS_PTH_ROUTINE) == 0) {
        limitData->startTimeLong  = COS_PTH_DL_LONG_TIME_LIMIT;
        limitData->startTimeShort = COS_PTH_DL_SHORT_TIME_LIMIT;
        limitData->longTimeCount  = COS_PTH_DL_LONG_TIME_COUNT_LIMIT;
        limitData->shortTimeCount = COS_PTH_DL_SHORT_TIME_COUNT_LIMIT;
        *actualData = &(g_cosPthDLData);
        res = true;
    }
    return res;
}

static bool IsVtpDownloadExcess(char* dlName) {
    time_t longTimeLmit;
    time_t shortTimeLmit;
    uint32_t longTimeCountLmit;
    uint32_t shortTimeCountLmit;
    VtpDownloadData *dlData = nullptr;
    VtpDownloadData limitData;
    time_t currentTime;

    TMS_LOG_D(g_tag, "%s enter, vtp download routine is %s.", __FUNCTION__, dlName);

    if (!g_nfcDlDataRead || !g_seDlDataRead) {
        ReadVtpDownloadData(dlName);
    }

    PrintVtpDownloadData();

    if (!CheckAndInitVtpDlLmit(dlName, &dlData, &limitData)) {
        TMS_LOG_E(g_tag, "%s Unknown vtp download routine. No limit.", __FUNCTION__);
        return false;
    } else {
        longTimeLmit       = limitData.startTimeLong;
        shortTimeLmit      = limitData.startTimeShort;
        longTimeCountLmit  = limitData.longTimeCount;
        shortTimeCountLmit = limitData.shortTimeCount;
    }

    time(&currentTime);
    TMS_LOG_I(g_tag, "%s currentTime = %ld", __FUNCTION__, currentTime);

    if (currentTime - dlData->startTimeLong > longTimeLmit) {
        dlData->startTimeLong = currentTime;
        dlData->startTimeShort = currentTime;
        dlData->shortTimeCount = 0;
        dlData->longTimeCount = 0;
        TMS_LOG_D(g_tag, "%s Long time(%ld) pass, reset all data.", __FUNCTION__, longTimeLmit);
    } else if (dlData->longTimeCount < longTimeCountLmit) {
        if (currentTime - dlData->startTimeShort > shortTimeLmit) {
            TMS_LOG_D(g_tag, "%s Short time(%ld) pass, reset data.", __FUNCTION__, shortTimeLmit);
            dlData->startTimeShort = currentTime;
            dlData->shortTimeCount = 0;
        } else if (dlData->shortTimeCount >= shortTimeCountLmit) {
            TMS_LOG_I(g_tag, "%s Download times excess limitation in short time, stop it.", __FUNCTION__);
            return true;
        }
    } else {
        TMS_LOG_I(g_tag, "%s Download times excess limitation in long time, stop it.", __FUNCTION__);
        return true;
    }

    dlData->shortTimeCount++;
    dlData->longTimeCount++;

    TMS_LOG_D(g_tag, "%s Ready to download %s, shortTimeCount = %d, longTimeCount = %d.", __FUNCTION__, dlName, dlData->shortTimeCount, dlData->longTimeCount);

    UpdateVtpDownloadData(dlName);

    return false;
}

ESESTATUS vtpDownload(void *arg)
{
    ESESTATUS status = ESESTATUS_INVALID_PARAMETER;
    DownloadCmdOp *pDlCmdOp = nullptr;
    VtpParams *pVtpParams = nullptr;
    char *fileName = nullptr;
    char *routineName = nullptr;

    if (arg == nullptr) {
        return status;
    }
    pDlCmdOp = (DownloadCmdOp *)arg;
    pVtpParams = (VtpParams *)pDlCmdOp->pParameters;
    routineName = pDlCmdOp->routineName;
    if (NULL == pVtpParams) {
        return status;
    }

    // If Erasure Protection feature is on, check if vtpDownload is over time.
    unsigned isErasureProtection = EseConfig::getUnsigned(NAME_TMS_NFCC_ERASURE_PROTECTION, 1);
    if (isErasureProtection == 1 && IsVtpDownloadExcess(routineName)) {
        TMS_LOG_E(g_tag, "%s Download too frequently!", __FUNCTION__);
        return ESESTATUS_DOWNLOAD_EXCESS;
    }

    fileName = pVtpParams->fileName;
    if (IsEndWith(fileName, strlen(fileName), ".bin", strlen(".bin"))) {
        status = vtpBinDownload(pDlCmdOp, pVtpParams);
    } else {
        status = vtpTxtDownload(pDlCmdOp, pVtpParams);
    }

    return status;
}

static ESESTATUS vtpBinDownload(DownloadCmdOp *pDlCmdOp, VtpParams *pVtpParams)
{
    ESESTATUS status = ESESTATUS_INVALID_PARAMETER;
    char *pReadBuff = nullptr;
    VtpBinDlCmdT *pHead = nullptr;
    VtpBinDlCmdT *pPreCmd = nullptr;
    ChipInfo *pChipInfo = pVtpParams->pChipInfo;

    TMS_LOG_D(g_tag, "%s enter, %s", __FUNCTION__, pVtpParams->fileName);

    status = tryPT2SeBl(pDlCmdOp, pVtpParams);
    if (ESESTATUS_SUCCESS != status) {
        return status;
    }

    pHead = ParseBinSC(string(pVtpParams->fileName), 0, &pReadBuff);
    if (nullptr == pHead) {
        TMS_LOG_E(g_tag, "%s script parse failed", __FUNCTION__);
        if (nullptr != pReadBuff) {
            free(pReadBuff);
        }
        return ESESTATUS_FAILED;
    }
    TMS_LOG_I(g_tag, "%s: Begin COS download", __FUNCTION__);

    bool ret = execBinCmds(pHead, pDlCmdOp, pChipInfo, &pPreCmd, status);
    if (!ret && (nullptr != pChipInfo) &&
        (!strncmp(pPreCmd->cmd, APDU_AUTH_HEAD, APDU_AUTH_HEAD_LEN) ||
         !strncmp(pPreCmd->cmd, APDU_AUTH_HEAD2, APDU_AUTH_HEAD_LEN2))) {
        if (CHIP_T_KEY == pChipInfo->chipKeyType) {
            pChipInfo->chipKeyType = CHIP_R_KEY;
        } else {
            pChipInfo->chipKeyType = CHIP_T_KEY;
        }
        status = ESESTATUS_SUCCESS;
        ret = execBinCmds(pHead, pDlCmdOp, pChipInfo, &pPreCmd, status);
    }

    DestroyVtpBinDlCmdList(pHead, false);
    if (nullptr != pReadBuff) {
        free(pReadBuff);
    }
    TMS_LOG_I(g_tag, "%s: COS download end, status = %d", __FUNCTION__, status);

    TMS_LOG_D(g_tag, "%s exit", __FUNCTION__);
    return status;
}

// Cannot support EC1 and EC2 at the same time.
static ESESTATUS vtpTxtDownload(DownloadCmdOp *pDlCmdOp, VtpParams *pVtpParams)
{
    ifstream ifs;
    ITmsPhAbs *pTmsPhAbs = NULL;
    ExecCmdFun execCmdFun;
    char *fileName = nullptr;
    char realPath[PATH_MAX + 1] = {0};
    // PT, pass-through transmission
    bool needPT2SeBl = false;
    ESESTATUS status = ESESTATUS_INVALID_PARAMETER;
    bool doChkCmdRsp = false;

    pTmsPhAbs = pVtpParams->pTmsPhAbs;
    if (nullptr == pTmsPhAbs) {
        return status;
    }
    execCmdFun = pTmsPhAbs->getExecCmdFun();

    fileName = pVtpParams->fileName;
    needPT2SeBl = pVtpParams->needPT2SeBl;
    TMS_LOG_D(g_tag, "%s enter, %s", __FUNCTION__, fileName);

    status = tryPT2SeBl(pDlCmdOp, pVtpParams);
    if (ESESTATUS_SUCCESS != status) {
        return status;
    }

    if (strlen(fileName) > PATH_MAX || realpath(fileName, realPath) == nullptr) {
        TMS_LOG_E(g_tag, "%s: invalid path", __FUNCTION__);
        return ESESTATUS_FAILED;
    }
    ifs.open(realPath, ios::in);
    if (!ifs.is_open()) {
        TMS_LOG_E(g_tag, "%s exit, open path:%s error", __FUNCTION__, realPath);
        return ESESTATUS_FAILED;
    } else {
        TMS_LOG_I(g_tag, "Begin COS download");
    }

    list<VtpDlCmd> test;
    list<VtpDlCmd>::iterator testiterator;
    string str;
    int startIndex = -1;
    unsigned int len = 0;
    DL_CMD_TYPE dlCmdType = DL_CMD_INVALID;
    while (getline(ifs, str)) {
        if ((str.length() == 0) || (str.length() == 1) || (str.at(0) == '#')) {
            // annotation or null line, ignore
            continue;
        } else if ((str.compare(0, VTP_HEADER_LEN1, VTP_HEADER_F) == 0)
                   || (str.compare(0, VTP_HEADER_LEN1, VTP_HEADER_UPPER_F) == 0)) {
            // eg. F 00500C00 00
            startIndex = VTP_HEADER_LEN1;
            dlCmdType = DL_CMD_7816_4_APDU;
        } else if ((str.compare(0, VTP_HEADER_LEN3, VTP_HEADER_RAW) == 0)
                   || (str.compare(0, VTP_HEADER_LEN3, VTP_HEADER_UPPER_RAW) == 0)) {
            // Eg. RAW 5AC4009E
            startIndex = VTP_HEADER_LEN3;
            // cmd type may be DL_CMD_NCI or DL_CMD_T1
            dlCmdType = DL_CMD_RAW;
        } else if ((str.compare(0, VTP_HEADER_LEN4, VTP_HEADER_SEND) == 0)
                   || (str.compare(0, VTP_HEADER_LEN4, VTP_HEADER_UPPER_SEND) == 0)) {
            // Eg. SEND
            startIndex = VTP_HEADER_LEN4;
            dlCmdType = DL_CMD_7816_4_APDU;
        } else if ((str.compare(0, VTP_HEADER_LEN4, VTP_HEADER_WAIT) == 0)
                   || (str.compare(0, VTP_HEADER_LEN4, VTP_HEADER_UPPER_WAIT) == 0)) {
            // Unit is ms. Eg. WAIT 300
            startIndex = VTP_HEADER_LEN4;
            dlCmdType = DL_CMD_SLEEP;
        } else if ((str.compare(0, VTP_HEADER_LEN5, VTP_HEADER_SLEEP) == 0)
                   || (str.compare(0, VTP_HEADER_LEN5, VTP_HEADER_UPPER_SLEEP) == 0)) {
            // Unit is ms. Eg. SLEEP 300
            startIndex = VTP_HEADER_LEN5;
            dlCmdType = DL_CMD_SLEEP;
        } else if ((str.compare(0, VTP_HEADER_LEN6, VTP_HEADER_ASSERT) == 0)
                   || (str.compare(0, VTP_HEADER_LEN6, VTP_HEADER_UPPER_ASSERT) == 0)) {
            // Eg. assert 9000
            startIndex = VTP_HEADER_LEN6;
            dlCmdType = DL_CMD_ASSERT;
        } else {
            startIndex = -1;
            dlCmdType = DL_CMD_INVALID;
#ifdef DEBUG_FLAG
            TMS_LOG_W(g_tag, "%s: invalid cmd = %s", __FUNCTION__, str.c_str());
#endif
            continue;
        }

        if (startIndex != -1) {
            string::iterator end = std::remove(str.begin(), str.end(), ' ');
            str.erase(end, str.end());
            len = str.length();
            if (str.at(len - 1) == '\r') {
                len = len - 1;
            }
            len = len - (unsigned int)startIndex;
        }

        // handle DL_CMD_NCI or DL_CMD_T1
        if (dlCmdType == DL_CMD_RAW) {
            if (str.compare(0, 2, "5A") == 0) {  // length 2
                dlCmdType = DL_CMD_T1;
            } else {
                dlCmdType = DL_CMD_NCI;
            }
        }

        VtpDlCmd dlCmd = {
            str.substr(startIndex, len),
            dlCmdType
        };
        test.push_back(dlCmd);
#ifdef DEBUG_FLAG
        TMS_LOG_D(g_tag, "%s: cmd = %s, cmdType = %d",
                  __FUNCTION__, dlCmd.cmd.c_str(), dlCmd.cmdType);
#endif
    }
    ifs.close();

    // status == ESESTATUS_SUCCESS
    bool isNewRspMem = tryNewRspMem(pDlCmdOp, status);
    if (ESESTATUS_SUCCESS != status) {
        return status;
    }

    for (testiterator = test.begin(); testiterator != test.end(); ++testiterator) {
        status = tryExecCmd(testiterator->cmd.c_str(), testiterator->cmd.length(),
                            testiterator->cmdType, &doChkCmdRsp,
                            execCmdFun, pDlCmdOp);
        if (ESESTATUS_SUCCESS != status) {
            break;
        }
    }
    TMS_LOG_I(g_tag, "%s: COS download end, status = %d", __FUNCTION__, status);

    test.clear();
    if (isNewRspMem) {
        tryDeleteRspMem(pDlCmdOp);
    }

    TMS_LOG_D(g_tag, "%s exit", __FUNCTION__);
    return status;
}

void ThreadExitHandler(int sig)
{
    TMS_LOG_E(g_tag, "exit DL[%d]", sig);
    if (SIGINT == sig) {
        if (gpTmsPh->getPhClass() == ITmsPhAbs::REE) {
            g_threadRunning = false;
#ifdef TMS_REE
            TmsRee *pReeSE = (TmsRee *)gpTmsPh;
            pReeSE->t1ReadTerminate();
            bool ret = pReeSE->ioctl(NFC_DLD_FLUSH);
            if (!ret) {
                TMS_LOG_E(g_tag, "DL flush i2c data failed, NFCC FW DL failed");
            }
#else
            TMS_LOG_E(g_tag, "%s maco unsupport REE feature", __FUNCTION__);
#endif
        } else {
            TMS_LOG_E(g_tag, "%s unsupport REE feature[t1ReadTerminate][ioctl]", __FUNCTION__);
        }
    } else {
        // do nothing
        TMS_LOG_I(g_tag, "%s signal = %d", __FUNCTION__, sig);
    }
}

bool RegisterExitSignal()
{
    struct sigaction actions;
    static sigset_t mask;
    (void)memset_s(&actions, sizeof(actions), 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    sigaddset(&mask, SIGINT);
    actions.sa_flags = 0;
    actions.sa_handler = ThreadExitHandler;
    sigaction(SIGINT, &actions, NULL);
    if (pthread_sigmask(SIG_UNBLOCK, &mask, NULL) != 0) {
        TMS_LOG_W(g_tag, "%s pthread_sigmask, errno = %d",
                  __FUNCTION__, errno);
        return false;
    } else {
        return true;
    }
}

bool startThread(DownloadCmdOp *pDlCmdOp, int timeout)
{
    pthread_t nciThread;
    gpTmsPh = ((VtpParams *)pDlCmdOp->pParameters)->pTmsPhAbs;
    int ret = pthread_create(&nciThread, NULL,
                             pDlCmdOp->routine, pDlCmdOp);
    const char *routineName = (pDlCmdOp->routineName ? pDlCmdOp->routineName : "unknown");

    if (ret != 0) {
        TMS_LOG_E(g_tag, "%s: %s status = %d",
                  __FUNCTION__, routineName, ret);
        gpTmsPh = NULL;
        return false;
    }

    struct timespec outtime;
    struct timeval now;
    gettimeofday(&now, NULL);
    outtime.tv_sec = now.tv_sec + timeout;
    outtime.tv_nsec = now.tv_usec * 1000;  // 1000us

    pthread_mutex_lock(pDlCmdOp->pMutex);
    ret = pthread_cond_timedwait(pDlCmdOp->pCond, pDlCmdOp->pMutex, &outtime);
    pthread_mutex_unlock(pDlCmdOp->pMutex);
    if (ret == ETIMEDOUT) {
        TMS_LOG_W(g_tag, "%s: %s timeout", __FUNCTION__, routineName);
        ret = pthread_kill(nciThread, SIGINT);
        if (ret != 0) {
            TMS_LOG_W(g_tag, "%s: Fail to kill %s! ret = %d", __FUNCTION__, routineName, ret);
        }
        TMS_LOG_I(g_tag, "%s: waiting %s to complete", __FUNCTION__, routineName);
        ret = pthread_join(nciThread, nullptr);
        if (ret != 0) {
            TMS_LOG_E(g_tag, "%s: pthread_join error, ret = %d", __FUNCTION__, ret);
        } else {
            TMS_LOG_I(g_tag, "%s: %s has been completed", __FUNCTION__, routineName);
        }
        gpTmsPh = NULL;
        return false;
    }

    pthread_detach(nciThread);
    gpTmsPh = NULL;
    return true;
}

void *TmsDlopen(const char *filename, int filenameLen)
{
    UNUSED(filenameLen);
    char realPath[PATH_MAX + 1] = {0};
    TMS_LOG_D(g_tag, "%s: filename = %s", __FUNCTION__, filename);
    if (strlen(filename) > PATH_MAX || realpath(filename, realPath) == nullptr) {
        TMS_LOG_E(g_tag, "%s: invalid path", __FUNCTION__);
        return NULL;
    }
    void *handle = dlopen(realPath, RTLD_LAZY);
    if (!handle) {
        TMS_LOG_E(g_tag, "%s: dlopen[%s], error[%s]", __FUNCTION__, realPath, dlerror());
        return nullptr;
    }
    // Clear any existing error
    dlerror();

    return handle;
}

bool TmsDlclose(void **handle)
{
    if (*handle == nullptr) {
        TMS_LOG_D(g_tag, "%s: has been dlclosed", __FUNCTION__);
        return true;
    }

    if (dlclose(*handle) == 0) {
        *handle = nullptr;
        return true;
    } else {
        TMS_LOG_E(g_tag, "%s: dlclose error[%s]", __FUNCTION__, dlerror());
        return false;
    }
}

void checkAndRemoveOverrideVtpFile()
{
    const char overrideFlagPath[] = "/data/vendor/nfc/override_vtp";
    const vector<string> overrideVtpFiles = {
        "/data/vendor/nfc/SEC_THN31_FW_VTP.txt",
        "/data/vendor/nfc/SEC_THN31_FW_VTP.txt.bin",
        "/data/vendor/nfc/NSEC_THN31_FW_VTP.txt",
        "/data/vendor/nfc/NSEC_THN31_FW_VTP.txt.bin",
        "/data/vendor/nfc/THN31_FW_VTP.txt",
        "/data/vendor/nfc/THN31_FW_VTP.txt.bin"};
    int ret;

    if (0 == access(overrideFlagPath, F_OK)) {
        // override flag exist. Keep override vtp file
        return;
    }

    for (const auto &vtpFile : overrideVtpFiles) {
        ret = remove(vtpFile.c_str());
        if ((ret < 0) && (ENOENT != errno)) {
            TMS_LOG_W(g_tag, "cannot remove override vtp file:%s errno:%d", vtpFile.c_str(), errno);
        } else if (0 == ret) {
            TMS_LOG_D(g_tag, "remove override vtp file:%s", vtpFile.c_str());
        }
    }
}
