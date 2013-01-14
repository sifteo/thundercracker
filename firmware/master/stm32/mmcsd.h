#ifndef MMCSD_H
#define MMCSD_H

#include "macros.h"

namespace MMCSD {

static const unsigned BlockSize     = 512;

static const uint32_t R1ErrorMask   = 0xFDFFE008;

static const uint32_t Cmd8Pattern   = 0x000001AA;

enum Status {
    StatusIdle              = 0,
    StatusReady             = 1,
    StatusIdent             = 2,
    StatusStandby           = 3,
    StatusTransmit          = 4,
    StatusData              = 5,
    StatusRecv              = 6,
    StatusProgram           = 7,
    StatusDisconnect        = 8
};

enum Command {
    CmdGoIdleState          = 0,
    CmdInit                 = 1,
    CmdAllSendCID           = 2,
    CmdSendRelativeAddr     = 3,
    CmdSetBusWidth          = 6,
    CmdSelDeselCard         = 7,
    CmdSendIfCond           = 8,
    CmdSendCSD              = 9,
    CmdSendCID              = 10,
    CmdStopTransmission     = 12,
    CmdSendStatus           = 0x80 | 13,
    CmdSetBlocklen          = 16,
    CmdReadSingleBlock      = 17,
    CmdReadMultiBlock       = 18,
    CmdSetBlockCount        = 0x80 | 23,
    CmdWriteBlock           = 24,
    CmdWriteMultiBlock      = 25,
    CmdEraseRWBlockStart    = 32,
    CmdEraseRWBlockEnd      = 33,
    CmdErase                = 38,
    CmdAppOpCond            = 0x80 | 41,
    CmdLockUnlock           = 42,
    CmdAppCmd               = 55,
    CmdReadOCR              = 58
};

enum R1Bits {
    R1Success               = 0,
    R1IdleState             = 1 << 0,
    R1EraseReset            = 1 << 1,
    R1IllegalCmd            = 1 << 2,
    R1CmdCRCErr             = 1 << 3,
    R1EraseSequenceErr      = 1 << 4,
    R1AddressErr            = 1 << 5,
    R1ParamErr              = 1 << 6
};

enum DataCmdResponse {
    DataAccepted            = 0x05,
    DataRejectedCRC         = 0x0B,
    DataRejectedWrite       = 0x0D,
    DataWriteMultiple       = 0XFC,
    DataStopTxfer           = 0XFD,
    DataStartBlock          = 0xFE
};


struct R7 {

    uint8_t bytes[5];

    ALWAYS_INLINE uint8_t r1() const {
        return bytes[0];
    }

    ALWAYS_INLINE uint8_t cmdVersion() const {
        return bytes[1];
    }
};

/*
 * CSD version 2.0
 */

#define MMCSD_CSD_20_CRC_SLICE                  7,1
#define MMCSD_CSD_20_FILE_FORMAT_SLICE          11,10
#define MMCSD_CSD_20_TMP_WRITE_PROTECT_SLICE    12,12
#define MMCSD_CSD_20_PERM_WRITE_PROTECT_SLICE   13,13
#define MMCSD_CSD_20_COPY_SLICE                 14,14
#define MMCSD_CSD_20_FILE_FORMAT_GRP_SLICE      15,15
#define MMCSD_CSD_20_WRITE_BL_PARTIAL_SLICE     21,21
#define MMCSD_CSD_20_WRITE_BL_LEN_SLICE         25,12
#define MMCSD_CSD_20_R2W_FACTOR_SLICE           28,26
#define MMCSD_CSD_20_WP_GRP_ENABLE_SLICE        31,31
#define MMCSD_CSD_20_WP_GRP_SIZE_SLICE          38,32
#define MMCSD_CSD_20_ERASE_SECTOR_SIZE_SLICE    45,39
#define MMCSD_CSD_20_ERASE_BLK_EN_SLICE         46,46
#define MMCSD_CSD_20_C_SIZE_SLICE               69,48
#define MMCSD_CSD_20_DSR_IMP_SLICE              76,76
#define MMCSD_CSD_20_READ_BLK_MISALIGN_SLICE    77,77
#define MMCSD_CSD_20_WRITE_BLK_MISALIGN_SLICE   78,78
#define MMCSD_CSD_20_READ_BL_PARTIAL_SLICE      79,79
#define MMCSD_CSD_20_READ_BL_LEN_SLICE          83,80
#define MMCSD_CSD_20_CCC_SLICE                  95,84
#define MMCSD_CSD_20_TRANS_SPEED_SLICE          103,96
#define MMCSD_CSD_20_NSAC_SLICE                 111,104
#define MMCSD_CSD_20_TAAC_SLICE                 119,112
#define MMCSD_CSD_20_STRUCTURE_SLICE            127,126


/*
 * CSD version 1.0
 */

#define MMCSD_CSD_10_CRC_SLICE                  MMCSD_CSD_20_CRC_SLICE
#define MMCSD_CSD_10_FILE_FORMAT_SLICE          MMCSD_CSD_20_FILE_FORMAT_SLICE
#define MMCSD_CSD_10_TMP_WRITE_PROTECT_SLICE    MMCSD_CSD_20_TMP_WRITE_PROTECT_SLICE
#define MMCSD_CSD_10_PERM_WRITE_PROTECT_SLICE   MMCSD_CSD_20_PERM_WRITE_PROTECT_SLICE
#define MMCSD_CSD_10_COPY_SLICE                 MMCSD_CSD_20_COPY_SLICE
#define MMCSD_CSD_10_FILE_FORMAT_GRP_SLICE      MMCSD_CSD_20_FILE_FORMAT_GRP_SLICE
#define MMCSD_CSD_10_WRITE_BL_PARTIAL_SLICE     MMCSD_CSD_20_WRITE_BL_PARTIAL_SLICE
#define MMCSD_CSD_10_WRITE_BL_LEN_SLICE         MMCSD_CSD_20_WRITE_BL_LEN_SLICE
#define MMCSD_CSD_10_R2W_FACTOR_SLICE           MMCSD_CSD_20_R2W_FACTOR_SLICE
#define MMCSD_CSD_10_WP_GRP_ENABLE_SLICE        MMCSD_CSD_20_WP_GRP_ENABLE_SLICE
#define MMCSD_CSD_10_WP_GRP_SIZE_SLICE          MMCSD_CSD_20_WP_GRP_SIZE_SLICE
#define MMCSD_CSD_10_ERASE_SECTOR_SIZE_SLICE    MMCSD_CSD_20_ERASE_SECTOR_SIZE_SLICE
#define MMCSD_CSD_10_ERASE_BLK_EN_SLICE         MMCSD_CSD_20_ERASE_BLK_EN_SLICE
#define MMCSD_CSD_10_C_SIZE_MULT_SLICE          49,47
#define MMCSD_CSD_10_VDD_W_CURR_MAX_SLICE       52,50
#define MMCSD_CSD_10_VDD_W_CURR_MIN_SLICE       55,53
#define MMCSD_CSD_10_VDD_R_CURR_MAX_SLICE       58,56
#define MMCSD_CSD_10_VDD_R_CURR_MIX_SLICE       61,59
#define MMCSD_CSD_10_C_SIZE_SLICE               73,62
#define MMCSD_CSD_10_DSR_IMP_SLICE              MMCSD_CSD_20_DSR_IMP_SLICE
#define MMCSD_CSD_10_READ_BLK_MISALIGN_SLICE    MMCSD_CSD_20_READ_BLK_MISALIGN_SLICE
#define MMCSD_CSD_10_WRITE_BLK_MISALIGN_SLICE   MMCSD_CSD_20_WRITE_BLK_MISALIGN_SLICE
#define MMCSD_CSD_10_READ_BL_PARTIAL_SLICE      MMCSD_CSD_20_READ_BL_PARTIAL_SLICE
#define MMCSD_CSD_10_READ_BL_LEN_SLICE          83, 80
#define MMCSD_CSD_10_CCC_SLICE                  MMCSD_CSD_20_CCC_SLICE
#define MMCSD_CSD_10_TRANS_SPEED_SLICE          MMCSD_CSD_20_TRANS_SPEED_SLICE
#define MMCSD_CSD_10_NSAC_SLICE                 MMCSD_CSD_20_NSAC_SLICE
#define MMCSD_CSD_10_TAAC_SLICE                 MMCSD_CSD_20_TAAC_SLICE
#define MMCSD_CSD_10_STRUCTURE_SLICE            MMCSD_CSD_20_STRUCTURE_SLICE

} // namespace MMCSD

#endif // MMCSD_H
