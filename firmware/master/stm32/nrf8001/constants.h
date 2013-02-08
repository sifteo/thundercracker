/*
 * Constants defined by the nRF8001 data sheet.
 */

#ifndef _NRF8001_CONSTANTS_H
#define _NRF8001_CONSTANTS_H

namespace Op {
    enum NRF8001Opcodes {

        // System commands
        Test                    = 0x01,
        Echo                    = 0x02,
        DtmCommand              = 0x03,
        Sleep                   = 0x04,
        Wakeup                  = 0x05,
        Setup                   = 0x06,
        ReadDynamicData         = 0x07,
        WriteDynamicData        = 0x08,
        GetDeviceVersion        = 0x09,
        GetDeviceAddress        = 0x0a,
        GetBatteryLevel         = 0x0b,
        GetTemperature          = 0x0c,
        RadioReset              = 0x0e,
        Connect                 = 0x0f,
        Bond                    = 0x10,
        Disconnect              = 0x11,
        SetTxPower              = 0x12,
        ChangeTimingRequest     = 0x13,
        OpenRemotePipe          = 0x14,
        SetAppILatency          = 0x19,
        SetKey                  = 0x1a,
        OpenAdvPipe             = 0x1b,
        Broadcast               = 0x1c,
        BondSecurityRequest     = 0x1d,
        DirectedConnect         = 0x1e,
        CloseRemotePipe         = 0x1f,

        // Data commands
        SetLocalData            = 0x0d,
        SendData                = 0x15,
        SendDataAck             = 0x16,
        RequestData             = 0x17,
        SendDataNack            = 0x18,

        // System events
        DeviceStartedEvent      = 0x81,
        EchoEvent               = 0x82,
        HardwareErrorEvent      = 0x83,
        CommandResponseEvent    = 0x84,
        ConnectedEvent          = 0x85,
        DisconnectedEvent       = 0x86,
        BondStatusEvent         = 0x87,
        PipeStatusEvent         = 0x88,
        TimingEvent             = 0x89,
        DisplayKeyEvent         = 0x8e,
        KeyRequestEvent         = 0x8f,

        // Data events
        DataCreditEvent         = 0x8a,
        PipeErrorEvent          = 0x8d,
        DataReceivedEvent       = 0x8c,
        DataAckEvent            = 0x8b,
    };
}

namespace OperatingMode {
    enum NRF8001OperatingMode {
        Awake   = 0x00,
        Test    = 0x01,
        Setup   = 0x02,
        Standby = 0x03,
    };
}

#endif // _NRF8001_CONSTANTS_H
