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

#ifndef TMS_7816_3_T1_T1_H
#define TMS_7816_3_T1_T1_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "tmsCommon.h"

#define CRC_PRESET 0xFFFF
#define CRC_POLYNOMIAL 0x8408

#define T1_DEFAULT_IFS 0xFE

// Node address device to card
#define NAD_D2C 0x5A
// Node address card to device
#define NAD_C2DT 0xA5


// T1 prologue field length and offset [start]
#define T1_HEADER_LEN 0x03
// ext frame's length is 2 bytes, refer to "GPC APDU Transport over SPI / I2C Version 1.0"
// The LEN and CRC fields shall have their Most Significant Byte sent first (i.e. big-endian order).
#define T1_EXT_HEADER_LEN 0x04
// T1 protocol frame LRC length is 1 byte
#define T1_LRC_LEN 0x01
// T1 protocol frame CRC length is 2 bytes
#define T1_CRC_LEN 0x02

#define T1_NAD_OFFSET 0x00
#define T1_PCB_OFFSET 0x01
#define T1_FRAME_LEN_OFFSET 0x02
#define T1_FRAME_LEN_OFFSET2 0x03
// T1 prologue field length and offset [end]


// T1 flags bit for masking [start]
#define T1_CHAINING_MASK 0x20
#define T1_S_BLOCK_REQ_MASK 0xC0
#define T1_S_BLOCK_RSP_MASK 0xE0
#define T1_I_BLOCK_SEQ_NO_MASK 0x40
#define T1_R_BLOCK_SEQ_NO_MASK 0x10

#define T1_S_BLOCK_SUB_TYPE_MASK 0x3F
// T1 flags bit for masking [end]

// reference to ATR definition field: capbilities
#define T1_EXT_HEADER_LEN_MASK 0x04

typedef enum {
    LRC = 0x00,  // Longitudinal redundancy code
    CRC = 0x01,  // Cyclic redundancy code
    CHK_INVALID,
} ChkCodeTypes;

/* ATRInfo: ISO7816 ATR Information bytes
 *
 * This structure holds ATR information bytes, contains ATR length.
 *
 */
typedef struct {
    uint8_t len;          // ATR length in bytes
    uint8_t vendorID[5];  // VendorID according to ISO7816-5
    uint8_t dllIC;       // Data Link Layer - Interface Character
    uint8_t bgt[2];       // Minimum guard time in milliseconds for
    // T=1 blocks sent in opposite directions.

    uint8_t bwt[2];       // Maximum allowed command processing
    // time in milliseconds before card has sent either
    // command response or S(WTX) requesting processing time extension

    uint8_t maxFreq[2];   // Max supported  clock frequency in kHz
    uint8_t checksum;     // Checksum (0 = LRC / 1 = CRC)
    uint8_t defaultIFSC;  // Default IFS size
    // Recommended value is 0x0102(258), APDU(256) + sw1sw2(2)

    uint8_t numChannels;  // Number of logical connections supported
    uint8_t maxIFSC[2];   // Maximum size of IFS supported
    uint8_t capbilities[2]; // Bitmap to indicate various features supported by SE
    // Bit-1: SE Data Available Line supported.
    // Bit-2: SE Data available polarity. 1 - Data available GPIO will be pulled HIGH when SE response is ready
    // Bit 3: SE chip reset S-blk command supported
    // Bit-4: Extended frame length feature supported
    // Bit-5: Support for more than one logical channel
    // Bit 6 to 16: Reserved for future use
} AtrInfo;

typedef struct {
    uint8_t lsb :  1; // PCB: lsb
    uint8_t bit2 : 1; // PCB: bit2
    uint8_t bit3 : 1; // PCB: bit3
    uint8_t bit4 : 1; // PCB: bit4
    uint8_t bit5 : 1; // PCB: bit5
    uint8_t bit6 : 1; // PCB: bit6
    uint8_t bit7 : 1; // PCB: bit7
    uint8_t msb : 1;  // PCB: msb
} T1PcbBits;

/** T1 Frame types **/
typedef enum {
    FRAME_TYPE_INVALID_MIN,  // Frame type: INVALID
    IFRAME,  // Frame type: I-frame
    RFRAME,  // Frame type: R-frame
    SFRAME,  // Frame type: S-frame
    FRAME_TYPE_INVALID_MAX,  // Frame type: INVALID
} T1FrameTypes;

/** T1 S-Frame sub-types **/
typedef enum {
    RESYNC_REQ = 0x00, // Re-synchronisation request between host and ESE
    RESYNC_RSP = 0x20, // Re-synchronisation response between host and ESE
    IFS_REQ = 0x01,     // IFSC size request
    IFS_RSP = 0x21,     // IFSC size response
    ABORT_REQ = 0x02,   // Abort request
    ABORT_RSP = 0x22,   // Abort response
    WTX_REQ = 0x03,     // WTX request
    WTX_RSP = 0x23,     // WTX response
    CIP_REQ = 0x04,     // Interface reset request(Communication Interface Parameters request)
    CIP_RSP = 0x24,     // Interface reset response(Communication Interface Parameters response)
    PROP_END_APDU_REQ = 0x05, // Proprietary Enf of APDU request
    PROP_END_APDU_RSP = 0x25, // Proprietary Enf of APDU response
    HARD_RESET_REQ = 0x06, // Chip reset request
    HARD_RESET_RSP = 0x26, // Chip reset request
    ATR_REQ = 0x07,  // ATR request
    ATR_RSP = 0x27,  // ATR response
    SFRAME_TYPE_INVALID, // Invalid request
} SFrameTypes;

/** T1 R-Frame sub-types **/
typedef enum  {
    RACK = 0x00,  // R-frame Acknowledgement frame indicator, an error-free acknowledgement
    RNACK_PARITY_ERROR = 0x01, // R-frame Negative-Acknowledgement frame indicator,
    //         a redundancy code error or a character parity error.
    RNACK_OTHER_ERROR = 0x02,  // R-frame Negative-Acknowledgement frame indicator, other errors.
    RNACK_INVALID_ERROR = 0xFF, // Invalid R-block request
} RFrameTypes;

typedef enum {
    STATE_IDEL = 0,
    I_BLK,
    R_ACK,
    R_PARITY_ERR,
    R_OTHER_ERR,
    S_RESYNC_REQ,
    S_IFS_REQ,
    S_IFS_RSP,
    S_ABORT_REQ,
    S_ABORT_RSP,
    S_WTX_REQ,
    S_WTX_RSP,
    S_CIP_REQ,
    S_PROP_END_APDU_REQ,
    S_ATR_REQ,
} T1TransceiveState;

/** T1 protocol process params **/
typedef struct {
    // T1 chaining feature, false I-block is not chained to the next I-block.
    bool isDeviceChaining;
    bool isCardChaining;

    // T=1 protocol retry rules.
    // t1SendSFrame, t1SendIFrame or t1SendRFrame returns success status.
    uint8_t recoveryCnt;
    // CIP cmd retry count
    uint8_t cipRecoveryCnt;

    // t1SendSFrame, t1SendIFrame or t1SendRFrame returns error status.
    uint8_t blkRetryCnt;

    uint8_t wtxCnt;
    // Rule 3 - If the card requires more than BWT to process the previously received I-block,
    //          it transmits S(WTX request) where INF conveys one byte encoding an integer
    //          multiplier of the BWT value. The interface device shall acknowledge by
    //          S(WTX response) with the same INF
    uint8_t wtxInfo;

    T1FrameTypes txFrameType;
    SFrameTypes txSubSFrameType; // Only used for S-block
    RFrameTypes txSubRFrameType; // Only used for R-block

    // For recovery
    T1FrameTypes lastRxFrameType;
    SFrameTypes lastRxSubSFrameType; // Only used for S-block
    RFrameTypes lastRxSubRFrameType; // Only used for R-block
    T1PcbBits lastRxPcbBits;

    uint8_t *pDataTx;     // Only I-block used
    uint16_t txLen;       // Only I-block used
    uint8_t *pDataTxPart; // Only I-block used
    uint16_t txLenPart; // Only I-block used
    uint16_t txDataOffset; // Only I-block used
} T1Params;

// Make sure include common.h here, if move to the front of T1Params, compile failed.
#include "common.h"

typedef struct SERecvBuffList {
    /* buffer to be used to store the received payload */
    SeData data;
    /* pointer to the next node present in lined list */
    struct SERecvBuffList *pNext;
} T1RecvBuffListT;

/******************************************************************************
 * @Function     t1TranscieveApduPart
 *
 * @Description  Send an APDU command and receive its response.
 *
 * @params       pCmdApdu - 7816-4 APDU that shall be sent
 *               cmdLen - Length of the apduCmd to be sent.
 *
 * @Returns          On success return ESESTATUS_SUCCESS or else ESESTATUS error.
 *
 ******************************************************************************/
ESESTATUS t1TransceiveApdu(
    uint8_t *pCmdApdu, uint16_t cmdLen);

/******************************************************************************
 * @Function     t1CipReq
 *
 * @Description  Send CIP request(S-Block). CIP, Communication Interface Parameters,
 *               definition by GPC APDU Transport over SPI/I2C Version 1.0.
 *               TMS definition: response is ATR, refer to AtrInfo struct.
 *               Note1:T1 request for ATR, the frame's INFO length should be 1 byte,
 *                     do not support 2 bytes length.
 *                     So, CIP and ATR request frame's INFO LEN is 1 byte.
 *                     And epiLogue field should use LRC (1 byte).
 *               Note2:T1 response for ATR, the frame's INFO length should be 1 byte,
 *                     do not support 2 bytes length.
 *                     So, CIP and ATR response frame's INFO LEN is 1 byte.
 *                     And epiLogue field should use LRC (1 byte).
 *
 * @Returns      On success return ESESTATUS_SUCCESS or else ESESTATUS error.
 *
******************************************************************************/
ESESTATUS t1CIPReq();

/******************************************************************************
 * @Function     t1ATRReq
 *
 * @Description  Send ATR request(S-Block). Refer to AtrInfo struct.
 *               Note1:T1 request for ATR, the frame's INFO length should be 1 byte,
 *                     do not support 2 bytes length.
 *                     So, CIP and ATR request frame's INFO LEN is 1 byte.
 *                     And epiLogue field should use LRC (1 byte).
 *               Note2:T1 response for ATR, the frame's INFO length should be 1 byte,
 *                     do not support 2 bytes length.
 *                     So, CIP and ATR response frame's INFO LEN is 1 byte.
 *                     And epiLogue field should use LRC (1 byte).
 *
 * @Returns      On success return ESESTATUS_SUCCESS or else ESESTATUS error.
 *
******************************************************************************/
ESESTATUS t1ATRReq();

/******************************************************************************
 * Function     t1IFSDeviceReq
 *
 * Description  Send IFSD request(S-Block)
 *
 * Returns      On success return ESESTATUS_SUCCESS or else ESESTATUS error.
 *
 ******************************************************************************/
ESESTATUS t1IFSDeviceReq();

/******************************************************************************
 * Function     t1PropEndApduReq
 *
 * Description  Send PROP END APDU request(S-Block)
 *
 * Returns      On success return ESESTATUS_SUCCESS or else ESESTATUS error.
 *
 ******************************************************************************/
ESESTATUS t1PropEndApduReq();

/******************************************************************************
 * Function     t1RecvDataGet
 *
 * Description  This function get the len and received data.
 *
 * @params       ppData - a pointer address to the received data.
 *               dataLen - a pointer to the length of the received data.
 *
 * Returns      On success return ESESTATUS_SUCCESS or else ESESTATUS error.
 *
 ******************************************************************************/
ESESTATUS t1RecvDataGet(uint8_t **ppData, uint16_t *dataLen);

AtrInfo *GetAtr(void);

#endif // TMS_7816_3_T1_T1_H