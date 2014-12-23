/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * Micah Elizabeth Scott <micah@misc.name>
 * Copyright <c> 2011 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
 *
 *  -DLCD_MODEL_GIANTPLUS_ILI9163C
 *  -DLCD_MODEL_TRULY_ST7735
 *  -DLCD_MODEL_TIANMA_ST7715
 *  -DLCD_MODEL_TIANMA_HX8353
 *
 * ... or accept the current default (in cube_hardware.h).
 */

#if !( defined(LCD_MODEL_GIANTPLUS_ILI9163C) || \
       defined(LCD_MODEL_TRULY_ST7735)       || \
       defined(LCD_MODEL_TIANMA_ST7715)      || \
       defined(LCD_MODEL_TIANMA_HX8353)      || \
       defined(LCD_MODEL_SANTEK_ST7735R)     || \
       defined(LCD_MODEL_WnW_RM68116)           )

    #error No lcd model selected
#endif

/********************************************************************/

#define SHORT_DELAY             (0x7F + 1)    // ~6 ms
#define LONG_DELAY              (0x7F + 20)   // ~120 ms

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

// MADCTR flags that correspond with VRAM flags (for rotation)
#define LCD_MADCTR_VRAM         (LCD_MADCTR_MY | LCD_MADCTR_MX | LCD_MADCTR_MV)

/*
 * MADCTR flags that we set by default (XOR'ed with VRAM flags). This
 * is where we compensate for panel-specific orientation or RGB/BGR
 * differences.
 */

#ifdef LCD_MODEL_GIANTPLUS_ILI9163C
#define LCD_MADCTR_NORMAL       (LCD_MADCTR_RGB)
#endif

#ifdef LCD_MODEL_TRULY_ST7735
#define LCD_MADCTR_NORMAL       (LCD_MADCTR_RGB | LCD_MADCTR_MY | LCD_MADCTR_MX)
#endif

#ifdef LCD_MODEL_TIANMA_ST7715
#define LCD_MADCTR_NORMAL       (LCD_MADCTR_RGB | LCD_MADCTR_MY | LCD_MADCTR_MX)
#endif

#ifdef LCD_MODEL_TIANMA_HX8353
#define LCD_MADCTR_NORMAL       (LCD_MADCTR_RGB | LCD_MADCTR_MY | LCD_MADCTR_MX)
#endif

#ifdef LCD_MODEL_SANTEK_ST7735R
#define LCD_MADCTR_NORMAL       (LCD_MADCTR_RGB | LCD_MADCTR_MY | LCD_MADCTR_MX)
#endif

#ifdef LCD_MODEL_WnW_RM68116
#define LCD_MADCTR_NORMAL       (LCD_MADCTR_RGB | LCD_MADCTR_MY | LCD_MADCTR_MX)
#endif

/*
 * Some LCDs are configured with larger GRAMs than the panel size,
 * We need to specify the margins and handle rotations appropriately
 */

#ifdef LCD_MODEL_TRULY_ST7735
#define HAVE_GRAM_PANEL_MISMATCH
#define LCD_X_LEFT_MARGIN       0
#define LCD_X_RIGHT_MARGIN      0
#define LCD_Y_TOP_MARGIN        32
#define LCD_Y_BOTTOM_MARGIN     0
#endif

#ifdef LCD_MODEL_TIANMA_ST7715
#define HAVE_GRAM_PANEL_MISMATCH
#define LCD_X_LEFT_MARGIN       2
#define LCD_X_RIGHT_MARGIN      2
#define LCD_Y_TOP_MARGIN        3
#define LCD_Y_BOTTOM_MARGIN     1
#endif

#ifdef LCD_MODEL_SANTEK_ST7735R
#define HAVE_GRAM_PANEL_MISMATCH
#define LCD_X_LEFT_MARGIN       2
#define LCD_X_RIGHT_MARGIN      2
#define LCD_Y_TOP_MARGIN        33
#define LCD_Y_BOTTOM_MARGIN     1
#endif

/*
 * Static initialization table
 */

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
     * Truly display, with ST7735R-G4 Controller.
     *
     * Based on some totally magical random numbers that Truly gave
     * us, in their TFT128128-102-E application note.
     *
     * Note that the Truly LCD absolutely requires a hardware reset
     * pulse before we can initialize it. Due to our limited GPIOs,
     * we combine this with the LCD backlight power control. So, we
     * are required to turn the backlight on before this init code.
     */

#ifdef LCD_MODEL_TRULY_ST7735

    LONG_DELAY,

    1, LCD_CMD_SWRESET,
    1, LCD_CMD_SLPOUT,

    LONG_DELAY,

    4, LCD_CMD_FRCONTROL, 0x08, 0x2c, 0x2b,
    4, LCD_CMD_FRCONTROL_IDLE, 0x00, 0x01, 0x01,
    7, LCD_CMD_FRCONTROL_PAR, 0x00, 0x01, 0x01, 0x00, 0x01, 0x01,

    2, LCD_CMD_INVCTRL, 0x03,
    4, LCD_CMD_POWER_CTRL1, 0xa2, 0x02, 0x84,
    2, LCD_CMD_POWER_CTRL2, 0x05,
    3, LCD_CMD_POWER_CTRL3, 0x0a, 0x00,
    3, LCD_CMD_POWER_CTRL4, 0x8a, 0x2a,
    3, LCD_CMD_POWER_CTRL5, 0x8a, 0xee,
    2, LCD_CMD_VCOM_CTRL1, 0x0e,
    2, LCD_CMD_VCOM_OFFSET, 0x10,
    
    17, LCD_CMD_POS_GAMMA,
    0x0f, 0x2b, 0x00, 0x08, 0x1b, 0x21, 0x20, 0x22,
    0x1f, 0x1b, 0x23, 0x37, 0x00, 0x07, 0x02, 0x10,

    17, LCD_CMD_NEG_GAMMA,
    0x0f, 0x1b, 0x0f, 0x17, 0x33, 0x2c, 0x29, 0x2e,
    0x30, 0x30, 0x39, 0x3f, 0x00, 0x07, 0x03, 0x10,

#endif // LCD_MODEL_TRULY_ST7735

    /**************************************************************
     * Tianma display, with ST7715R Controller.
     *
     * Based on the sample init sequence provided by Tianma.
     */

#ifdef LCD_MODEL_TIANMA_ST7715

    1, LCD_CMD_SWRESET,
    1, LCD_CMD_SLPOUT,
    
    LONG_DELAY,

    4, LCD_CMD_FRCONTROL, 0x02, 0x23, 0x22,
    4, LCD_CMD_FRCONTROL_IDLE, 0x02, 0x23, 0x22,
    7, LCD_CMD_FRCONTROL_PAR, 0x02, 0x23, 0x22, 0x02, 0x23, 0x22,
    
    2, LCD_CMD_INVCTRL, 0x07,

    4, LCD_CMD_POWER_CTRL1, 0xa3, 0x02, 0x84,
    2, LCD_CMD_POWER_CTRL2, 0xc5,
    3, LCD_CMD_POWER_CTRL3, 0x0a, 0x00,
    3, LCD_CMD_POWER_CTRL4, 0x8a, 0x2a,
    3, LCD_CMD_POWER_CTRL5, 0x8a, 0xee,
    2, LCD_CMD_VCOM_CTRL1, 0x07,

    // Undocumented...
    2, 0xf0, 0x01,
    2, 0xf6, 0x00,

    17, LCD_CMD_POS_GAMMA,
    0x0f, 0x2b, 0x00, 0x08, 0x1b, 0x21, 0x20, 0x22,
    0x1f, 0x1b, 0x23, 0x37, 0x00, 0x07, 0x02, 0x10,

    17, LCD_CMD_NEG_GAMMA,
    0x0f, 0x1b, 0x0f, 0x17, 0x33, 0x2c, 0x29, 0x2e,
    0x30, 0x30, 0x39, 0x3f, 0x00, 0x07, 0x03, 0x10,

#endif // LCD_MODEL_TIANMA_ST7715

    /**************************************************************
     * Tianma display, with HX8353-D Controller.
     *
     * Based on the sample init sequence provided by Tianma.
     */

#ifdef LCD_MODEL_TIANMA_HX8353

    1, LCD_CMD_SWRESET,
    1, LCD_CMD_SLPOUT,

    LONG_DELAY,

    // Magic command to enable HX8353 "extended" commands.
    // We also use this for model detection in siftulator.
    4, 0xb9, 0xff, 0x83, 0x53,          // SETEXC

    /*
     * Gamma, power, and oscillator init.
     *
     * XXX: This is the magic init sequence we got from Tianma, but it
     *      seems to exhibit some annoying flickering/interlacing
     *      artifacts. It's a little better than the default config
     *      in terms of color quality, though, so we're still using
     *      this... though I suspect we can optimize it.
     */
    2, 0xc6, 0x31,                      // UADJ   (60Hz frame rate)
    3, 0xb1, 0x00, 0x00,                // SETPWCTR
    3, 0xbf, 0x04, 0x38,                // SETPTBA
    5, 0xc0, 0x50, 0x08, 0x0c, 0xca,    // SETSTBA
    2, 0xcc, 0x00,                      // SETPANEL
    5, 0xe3, 0x08, 0x00, 0x04, 0x10,    // EQ
    4, 0xb6, 0x94, 0x78, 0x64,          // VCOM

    20, 0xe0,                           // Gamma
    0x00, 0x74, 0x71, 0x0a, 0xff, 0x01, 0x07, 0x0f,
    0x06, 0x01, 0x60, 0x30, 0x77, 0x0d, 0xf0, 0x0e,
    0x0a, 0x08, 0x0f,

#endif // LCD_MODEL_TIANMA_HX8353

    /**************************************************************
     * Santek display, with ST7735R Controller.
     *
     * Based on the sample init sequence provided by Santek.
     */

#ifdef LCD_MODEL_SANTEK_ST7735R
    // This delay is a MUST for proper reset sequence
    // ~120ms suffices although santek specifies ~240ms
    LONG_DELAY,
    //LONG_DELAY,

    1, LCD_CMD_SWRESET,
    1, LCD_CMD_SLPOUT,

    LONG_DELAY,

    4, LCD_CMD_FRCONTROL, 0x01, 0x2c, 0x2d,
    4, LCD_CMD_FRCONTROL_IDLE, 0x01, 0x2c, 0x2d,
    7, LCD_CMD_FRCONTROL_PAR, 0x01, 0x2c, 0x2d, 0x01, 0x2c, 0x2d,
    //4, LCD_CMD_FRCONTROL, 0x01, 0x08, 0x05,
    //4, LCD_CMD_FRCONTROL_IDLE, 0x01, 0x08, 0x05,
    //7, LCD_CMD_FRCONTROL_PAR, 0x01, 0x08, 0x05, 0x01, 0x08, 0x05,

    2, LCD_CMD_INVCTRL, 0x07,
    //2, LCD_CMD_INVCTRL, 0x00,

    4, LCD_CMD_POWER_CTRL1, 0xa2, 0x02, 0x84,
    2, LCD_CMD_POWER_CTRL2, 0xc5,
    3, LCD_CMD_POWER_CTRL3, 0x0a, 0x00,
    3, LCD_CMD_POWER_CTRL4, 0x8a, 0x2a,
    3, LCD_CMD_POWER_CTRL5, 0x8a, 0xee,

    2, LCD_CMD_VCOM_CTRL1, 0x0e,

    //1, LCD_CMD_INVOFF,

    17, LCD_CMD_POS_GAMMA,
    0x0f, 0x1a, 0x0f, 0x18, 0x2f, 0x28, 0x20, 0x22,
    0x1f, 0x1b, 0x23, 0x37, 0x00, 0x07, 0x02, 0x10,

    17, LCD_CMD_NEG_GAMMA,
    0x0f, 0x1b, 0x0f, 0x17, 0x33, 0x2c, 0x29, 0x2e,
    0x30, 0x30, 0x39, 0x3f, 0x00, 0x07, 0x03, 0x10,

    //Mystery commands per santek initialization code
    2, 0xf0, 0x01,
    2, 0xf6, 0x00,

#endif // LCD_MODEL_SANTEK_ST7735R

    /**************************************************************
     * W&W display, with RM68116 Controller.
     *
     * Based on the sample init sequence provided by W&W.
     */

#ifdef LCD_MODEL_WnW_RM68116

    1, LCD_CMD_SWRESET,
    1, LCD_CMD_SLPOUT,

    LONG_DELAY,

    3, LCD_CMD_POWER_CTRL1, 0xd3, 0x13,
    2, LCD_CMD_INVCTRL, 0x03,

    //Mystery command
    2, 0xf8, 0x01,

    17, LCD_CMD_POS_GAMMA,
    0x00, 0x01, 0x05, 0x29, 0x27, 0x1f, 0x07, 0x0e,
    0x05, 0x04, 0x05, 0x08, 0x06, 0x0c, 0x04, 0x07,

    17, LCD_CMD_NEG_GAMMA,
    0x00, 0x01, 0x05, 0x29, 0x27, 0x1f, 0x07, 0x0e,
    0x05, 0x04, 0x05, 0x08, 0x06, 0x0c, 0x04, 0x07,

    //Mystery command
    5, 0xfe, 0x09, 0xb0, 0x10, 0x48,

    4, LCD_CMD_FRCONTROL, 0x0f, 0x00, 0x04,

    //Mystery command
    5, 0xfd, 0x10, 0xdf, 0x60, 0xd0,
    3, 0xf4, 0x00, 0x0c,

    3, LCD_CMD_POWER_CTRL3, 0x02, 0x84,

    //Mystery command
    2, 0xf8, 0x00,

#endif // LCD_MODEL_WnW_RM68116

    /**************************************************************
     * Portable initialization
     */

    2, LCD_CMD_TEON, 0x00,
    2, LCD_CMD_NORMAL, 0x00,

    // Use 16-bit color mode
    2, LCD_CMD_COLMOD, LCD_COLMOD_16,

    0,
};
