/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * This file implements the highly model-specific portions of the LCD
 * driver. Much of the LCD driver will run without changes on a wide
 * family of LCD controllers, but the initialization code and some of
 * the specific commands are model-specific.
 *
 * Note that this isn't a "real" header file. It has definitions
 * that are ONLY for inclusion into lcd.c.
 */

/********************************************************************
 * Pick one LCD model to build for...
 */

#define LCD_MODEL_GIANTPLUS_ILI9163C
//#define LCD_MODEL_TRULY_ST7735

/********************************************************************/

/*
 * This is a superset of commands defined by all the LCDs we support.
 * Check the individual controller data sheets when using anything new.
 */

#define LCD_CMD_NOP             0x00
#define LCD_CMD_SWRESET         0x01
#define LCD_CMD_SLPIN           0x10
#define LCD_CMD_SLPOUT          0x11
#define LCD_CMD_PARTIAL         0x12
#define LCD_CMD_NORMAL          0x13
#define LCD_CMD_INVOFF          0x20
#define LCD_CMD_INVON           0x21
#define LCD_CMD_GAMMA_SET       0x26
#define LCD_CMD_DISPOFF         0x28
#define LCD_CMD_DISPON          0x29
#define LCD_CMD_CASET           0x2A
#define LCD_CMD_RASET           0x2B
#define LCD_CMD_RAMWR           0x2C
#define LCD_CMD_COLOR_LUT       0x2D
#define LCD_CMD_PARTIAL_AREA    0x30
#define LCD_CMD_TEOFF           0x34
#define LCD_CMD_TEON            0x35
#define LCD_CMD_MADCTR          0x36
#define LCD_CMD_IDLEOFF         0x38
#define LCD_CMD_IDLEON          0x39
#define LCD_CMD_COLMOD          0x3A
#define LCD_CMD_FRCONTROL       0xB1
#define LCD_CMD_FRCONTROL_IDLE  0xB2
#define LCD_CMD_FRCONTROL_PAR   0xB3
#define LCD_CMD_INVCTRL         0xB4
#define LCD_CMD_PORCH           0xB5
#define LCD_CMD_FNSET           0xB6
#define LCD_CMD_SRCDRV          0xB7
#define LCD_CMD_GATEDRV         0xB8
#define LCD_CMD_POWER_CTRL1     0xC0
#define LCD_CMD_POWER_CTRL2     0xC1
#define LCD_CMD_POWER_CTRL3     0xC2
#define LCD_CMD_POWER_CTRL4     0xC3
#define LCD_CMD_POWER_CTRL5     0xC4
#define LCD_CMD_VCOM_CTRL1      0xC5
#define LCD_CMD_VCOM_CTRL2      0xC6
#define LCD_CMD_VCOM_OFFSET     0xC7
#define LCD_CMD_POS_GAMMA       0xE0
#define LCD_CMD_NEG_GAMMA       0xE1
#define LCD_CMD_UNDOCUMENTED_EC 0xEC
#define LCD_CMD_GAM_R_SEL       0xF2
#define LCD_CMD_POWER_CTRL6     0xFC

#define LCD_COLMOD_12           3
#define LCD_COLMOD_16           5
#define LCD_COLMOD_18           6

#define LCD_MADCTR_MY           0x80
#define LCD_MADCTR_MX           0x40
#define LCD_MADCTR_MV           0x20
#define LCD_MADCTR_ML           0x10
#define LCD_MADCTR_RGB          0x08
#define LCD_MADCTR_MH           0x04

// MADCTR flags that we always set
#define LCD_MADCTR_NORMAL       (LCD_MADCTR_RGB)

// MADCTR flags that correspond with VRAM flags (for rotation)
#define LCD_MADCTR_VRAM         (LCD_MADCTR_MY | LCD_MADCTR_MX | LCD_MADCTR_MV)

#define SHORT_DELAY             0x80    // 1ms
#define LONG_DELAY              0xFF    // 128ms


static const __code uint8_t lcd_setup_table[] =
{
    /**************************************************************
     * GiantPlus display, with ILI9163C Controller.
     *
     * This init code is based on the code we used in the Gen 1 cube
     * firmware, with some additional tweaks and cleanups.
     */

#ifdef LCD_MODEL_GIANTPLUS_ILI9163C

    2, LCD_CMD_SWRESET, 0x00,

    SHORT_DELAY,

    2, LCD_CMD_UNDOCUMENTED_EC, 0x1b,

    2, LCD_CMD_SLPOUT, 0x00,

    SHORT_DELAY,

    3, LCD_CMD_FRCONTROL, 0x0c, 0x03,

    3, LCD_CMD_FNSET, 0x07, 0x02,

    3, LCD_CMD_POWER_CTRL1, 0x08, 0x00,
    2, LCD_CMD_POWER_CTRL2, 0x03,
    3, LCD_CMD_POWER_CTRL3, 0x03, 0x03,

    3, LCD_CMD_VCOM_CTRL1, 0x4a, 0x3e,
    2, LCD_CMD_VCOM_OFFSET, 0x40,

    2, LCD_CMD_GAMMA_SET, 0x01,
    2, LCD_CMD_GAM_R_SEL, 0x01,

    16, LCD_CMD_POS_GAMMA,
    0x3f, 0x2a, 0x27, 0x2d, 0x26, 0x0c, 0x53, 0xf4,
    0x3f, 0x16, 0x1d, 0x15, 0x0f, 0x06, 0x00,
    
    16, LCD_CMD_NEG_GAMMA,
    0x00, 0x15, 0x18, 0x11, 0x19, 0x13, 0x2b, 0x63,
    0x40, 0x09, 0x22, 0x2a, 0x30, 0x39, 0x3f,

#endif // LCD_MODEL_GIANTPLUS_ILI9163C

    /**************************************************************
     * Truly display, with ST7735 Controller.
     *
     * Based on the init sequence used by the adafruit ST7735 library:
     * https://github.com/adafruit/ST7735-Library/
     *
     * Note that the Truly LCD absolutely requires a hardware reset
     * pulse before we can initialize it. Due to our limited GPIOs,
     * we combine this with the LCD backlight power control. So, we
     * are required to turn the backlight on before this init code.
     *
     * XXX: Not working yet
     */

#ifdef LCD_MODEL_TRULY_ST7735

    1, LCD_CMD_SWRESET,

    LONG_DELAY,

    1, LCD_CMD_SLPOUT,
    SHORT_DELAY,

    LONG_DELAY,

    2, LCD_CMD_COLMOD, 0x05,
    SHORT_DELAY,

    4, LCD_CMD_FRCONTROL, 0x00, 0x06, 0x03,
    SHORT_DELAY,

    2, LCD_CMD_MADCTR, 0xc8,
    SHORT_DELAY,

    3, LCD_CMD_FNSET, 0x15, 0x02,
    SHORT_DELAY,
    2, LCD_CMD_INVCTRL, 0x00,
    SHORT_DELAY,
    
    3, LCD_CMD_POWER_CTRL1, 0x02, 0x70,
    SHORT_DELAY,
    2, LCD_CMD_POWER_CTRL2, 0x05,
    SHORT_DELAY,
    3, LCD_CMD_POWER_CTRL3, 0x01, 0x02,
    SHORT_DELAY,
    3, LCD_CMD_VCOM_CTRL1, 0x3c, 0x38,
    SHORT_DELAY,
    3, LCD_CMD_POWER_CTRL6, 0x11, 0x15,
    SHORT_DELAY,
    
    17, LCD_CMD_POS_GAMMA,
    0x09, 0x16, 0x09, 0x20, 0x21, 0x1b, 0x13, 0x19,
    0x17, 0x15, 0x1e, 0x2b, 0x04, 0x05, 0x02, 0x0e,
    SHORT_DELAY,
    
    17, LCD_CMD_NEG_GAMMA,
    0x0b, 0x14, 0x08, 0x1e, 0x22, 0x1d, 0x18, 0x1e,
    0x1b, 0x1a, 0x24, 0x2b, 0x06, 0x06, 0x02, 0x0f,
    SHORT_DELAY,

    5, LCD_CMD_CASET, 0x00, 0x02, 0x00, 0x81,
    SHORT_DELAY,
    5, LCD_CMD_RASET, 0x00, 0x02, 0x00, 0x81,
    SHORT_DELAY,

    LONG_DELAY,

#endif // LCD_MODEL_TRULY_ST7735

    /**************************************************************
     * Portable initialization
     */

    2, LCD_CMD_TEON, 0x00,
    2, LCD_CMD_NORMAL, 0x00,

    // RASET happens at every frame, but CASET should happen once
    5, LCD_CMD_CASET, 0x00, 0x00, 0x00, 0x7f,

    // Use 16-bit color mode
    2, LCD_CMD_COLMOD, LCD_COLMOD_16,

    0,
};
