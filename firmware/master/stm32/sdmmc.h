#ifndef SDMMC_H
#define SDMMC_H

namespace SDMMC {

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
    CmdSendStatus           = 13,
    CmdSetBlocklen          = 16,
    CmdReadSingleBlock      = 17,
    CmdReadMultiBlock       = 18,
    CmdSetBlockCount        = 23,
    CmdWriteBlock           = 24,
    CmdWriteMultiBlock      = 25,
    CmdEraseRWBlockStart    = 32,
    CmdEraseRWBlockEnd      = 33,
    CmdErase                = 38,
    CmdAppOpCond            = 41,
    CmdLockUnlock           = 42,
    CmdAppCmd               = 55,
    CmdReadOCR              = 58
};

// TODO: CSD record offsets

} // namespace SDMMC

#endif // SDMMC_H
