/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "ui_assets.h"

extern const uint16_t v01_LogoWhiteOnBlue_data[];
extern const uint16_t v01_MenuBackground_data[];
extern const uint16_t v01_IconQuit_data[];
extern const uint16_t v01_IconBack_data[];
extern const uint16_t v01_IconResume_data[];
extern const uint16_t v01_ShutdownBackground_data[];

extern const uint16_t v02_LogoWhiteOnBlue_data[];
extern const uint16_t v02_MenuBackground_data[];
extern const uint16_t v02_IconQuit_data[];
extern const uint16_t v02_IconBack_data[];
extern const uint16_t v02_IconResume_data[];
extern const uint16_t v02_ShutdownBackground_data[];
extern const uint16_t v02_BigDigits_data[];
extern const uint16_t v02_LowBattBase_data[];
extern const uint16_t v02_FaultMessage_data[];
extern const uint16_t v02_CubeRangePause_data[];


void UIAssets::init(unsigned version)
{
    switch (version) {

        case 0x01:
            logoWhiteOnBlue     = v01_LogoWhiteOnBlue_data;
            menuBackground      = v01_MenuBackground_data;
            iconQuit            = v01_IconQuit_data;
            iconBack            = v01_IconBack_data;
            iconResume          = v01_IconResume_data;
            shutdownBackground  = v01_ShutdownBackground_data;
            bigDigits           = v02_BigDigits_data;       // XXX
            faultMessage        = v02_FaultMessage_data;    // XXX
            iconCubeRange       = v02_CubeRangePause_data;  // XXX
            iconLowBattBase     = v02_LowBattBase_data;     // XXX
            menuHeight          = 9;
            menuTextPalette     = 11;
            iconSize            = 5;
            iconSpacing         = 8;
            shutdownHeight      = 10;
            shutdownY1          = 1;
            shutdownY2          = 8;
            break;

        default:
        case 0x02:
            logoWhiteOnBlue     = v02_LogoWhiteOnBlue_data;
            menuBackground      = v02_MenuBackground_data;
            iconQuit            = v02_IconQuit_data;
            iconBack            = v02_IconBack_data;
            iconResume          = v02_IconResume_data;
            shutdownBackground  = v02_MenuBackground_data;
            bigDigits           = v02_BigDigits_data;
            iconLowBattBase     = v02_LowBattBase_data;
            faultMessage        = v02_FaultMessage_data;
            iconCubeRange       = v02_CubeRangePause_data;
            menuHeight          = 11;
            menuTextPalette     = 8;
            iconSize            = 8;
            iconSpacing         = 9;
            shutdownHeight      = 11;
            shutdownY1          = 1;
            shutdownY2          = 9;
            break;

    }
}
