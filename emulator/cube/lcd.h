/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _LCD_H
#define _LCD_H

class LCD {
 public:
    static const unsigned LCD_WIDTH  = 128;
    static const unsigned LCD_HEIGHT = 128;

    static const unsigned FB_SIZE = 0x8000;
    static const unsigned FB_MASK = 0x3FFF;
    static const unsigned FB_ROW_SHIFT = 7;

    struct lcd_pins {
        /* Configured for an 8-bit parallel bus, in 80-system mode */
        
        uint8_t   csx;        // IN, active-low
        uint8_t   dcx;        // IN, low=cmd high=data
        uint8_t   wrx;        // IN, rising edge
        uint8_t   rdx;        // IN, rising edge
        uint8_t   data_in;    // IN
    };

    /* 16-bit RGB 5-6-5 format */
    uint16_t fb_mem[FB_SIZE];

    void init() {
        memset(this, 0, sizeof *this);

        // Framebuffer contents undefined. Simulate that...
        uint32_t i;
        for (i = 0; i < FB_SIZE; i++)
            fb_mem[i] = 31337 * (1+i);
        
        xe = LCD_WIDTH - 1;
        ye = LCD_HEIGHT - 1;
        colmod = COLMOD_18;
    }

    void cycle(struct lcd_pins *pins) {
        /*
         * Make lots of assumptions...
         *
         * This is pretending to be an SPFD5414-like controller, with the following settings:
         *
         *   - 8-bit parallel interface, in 80-series mode
         *   - 16-bit color depth, RGB-565 (3AH = 05)
         */
        
        if (!pins->csx && pins->wrx && !prev_wrx) {
            if (pins->dcx) {
                /* Data write strobe */
                lcd_data(pins->data_in);
            } else {
                /* Command write strobe */
                lcd_cmd(pins->data_in);
            }
        }

        prev_wrx = pins->wrx;
    }

    uint32_t getWriteCount() {
        uint32_t cnt = write_count;
        write_count = 0;
        return cnt;
    }

    bool isVisible() {  
        return mode_awake && mode_display_on;
    }

    void pulseTE() {
        if (mode_te) {
            // This runs on the GUI thread, use a lock-free timer.
            te_timer_head += USEC_TO_CYCLES(TE_WIDTH_US);
        }
    }

    int tick() {
        /*
         * We have a separate entry point for ticking the TE timer,
         * since it really does need to happen every tick rather than
         * just when there's a graphics pin state change.
         */
        
        if (te_timer_head != te_timer_tail) {
            te_timer_tail++;
            return 1;
        }
        return 0;
    }

 private:
    void firstPixel() {
        // Return to start row/column
        row = ys;
        col = xs;
    }

    void writePixel(uint16_t pixel) {
        unsigned vRow, vCol, addr;
        uint8_t madctr = madctr ^ model.madctr_xor;

        // Logical to physical address translation
        vRow = (madctr & MADCTR_MY) ? (LCD_HEIGHT - 1 - row) : row;
        vCol = (madctr & MADCTR_MX) ? (LCD_WIDTH - 1 - col) : col;
        vRow += model.row_adj;
        vCol += model.col_adj;

        addr = (madctr & MADCTR_MV) 
            ? (vRow + (vCol << FB_ROW_SHIFT))
            : (vCol + (vRow << FB_ROW_SHIFT));
    
        fb_mem[addr & FB_MASK] = pixel;
        
        if (++col > xe) {
            col = xs;
            if (++row > ye)
                row = ys;
        }
    }

    void writeByte(uint8_t b) {
        pixel_bytes[cmd_bytecount++] = b;

        switch (colmod) {

        case COLMOD_12:
            if (cmd_bytecount == 3) {
                uint8_t r1 = pixel_bytes[0] >> 4;
                uint8_t g1 = pixel_bytes[0] & 0x0F;
                uint8_t b1 = pixel_bytes[1] >> 4;

                uint8_t r2 = pixel_bytes[1] & 0x0F;
                uint8_t g2 = pixel_bytes[2] >> 4;
                uint8_t b2 = pixel_bytes[2] & 0x0F;

                cmd_bytecount = 0;

                writePixel( (r1 << 12) | ((r1 >> 3) << 11) |
                            (g1 << 7) | ((g1 >> 2) << 5) |
                            (b1 << 1) | (b1 >> 3) );
                
                writePixel( (r2 << 12) | ((r2 >> 3) << 11) |
                            (g2 << 7) | ((g2 >> 2) << 5) |
                            (b2 << 1) | (b2 >> 3) );
            }
            break;

        case COLMOD_16:
            if (cmd_bytecount == 2) {
                cmd_bytecount = 0;
                writePixel( (pixel_bytes[0] << 8) |
                            pixel_bytes[1] );
            }
            break;

        case COLMOD_18:
            if (cmd_bytecount == 3) {
                uint8_t r = pixel_bytes[0] >> 3;
                uint8_t g = pixel_bytes[1] >> 2;
                uint8_t b = pixel_bytes[2] >> 3;

                cmd_bytecount = 0;
                writePixel( (r << 11) | (g << 5) | b );
            }
            break;

        default:
            cmd_bytecount = 0;
            break;
        }
    }

    void command(uint8_t op) {
        current_cmd = op;
        cmd_bytecount = 0;

        switch (op) {
        
        case CMD_RAMWR:
            firstPixel();
            write_count++;
            break;

        case CMD_SWRESET:
            init();
            break;

        case CMD_SLPIN:
            mode_awake = 0;
            break;
            
        case CMD_SLPOUT:
            mode_awake = 1;
            break;

        case CMD_DISPOFF:
            mode_display_on = 0;
            break;
        
        case CMD_DISPON:
            mode_display_on = 1;
            break;

        case CMD_TEOFF:
            mode_te = 0;
            break;

        case CMD_TEON:
            mode_te = 1;
            break;

            /*
             * Assume this firmware is expecting a Truly  Undo its model-specific tweaks.
             */
        case CMD_MAGIC_TRULY:
            model.madctr_xor = MADCTR_MX | MADCTR_MY;
            model.row_adj = -32;
            model.col_adj = 0;
            break;

        }
    }

    void data(uint8_t byte) {
        switch (current_cmd) {

        case CMD_CASET:
            switch (cmd_bytecount++) {
            case 1:  xs = clamp(byte, 0, 0x83);
            case 3:  xe = clamp(byte, 0, 0x83);
            }
            break;

        case CMD_RASET:
            switch (cmd_bytecount++) {
            case 1:  ys = clamp(byte, 0, 0xa1);
            case 3:  ye = clamp(byte, 0, 0xa1);
            }
            break;
            
        case CMD_MADCTR:
            madctr = byte;
            break;

        case CMD_COLMOD:
            colmod = byte;
            break;
            
        case CMD_RAMWR:
            lcd_write_byte(byte);
            break;
        }
    }
    
    static uint8_t clamp(uint8_t val, uint8_t min, uint8_t max) {
        if (val < min) val = min;
        if (val > max) val = max;
        return val;
    }

    static const uint8_t CMD_NOP      = 0x00;
    static const uint8_t CMD_SWRESET  = 0x01;
    static const uint8_t CMD_SLPIN    = 0x10;
    static const uint8_t CMD_SLPOUT   = 0x11;
    static const uint8_t CMD_DISPOFF  = 0x28;
    static const uint8_t CMD_DISPON   = 0x29;
    static const uint8_t CMD_CASET    = 0x2A;
    static const uint8_t CMD_RASET    = 0x2B;
    static const uint8_t CMD_RAMWR    = 0x2C;
    static const uint8_t CMD_TEOFF    = 0x34;
    static const uint8_t CMD_TEON     = 0x35;
    static const uint8_t CMD_MADCTR   = 0x36;
    static const uint8_t CMD_COLMOD   = 0x3A;

    static const uint8_t COLMOD_12    = 3;
    static const uint8_t COLMOD_16    = 5;
    static const uint8_t COLMOD_18    = 6;

    static const uint8_t MADCTR_MY    = 0x80;
    static const uint8_t MADCTR_MX    = 0x40;
    static const uint8_t MADCTR_MV    = 0x20;
    static const uint8_t MADCTR_ML    = 0x10;   // Not implemented
    static const uint8_t MADCTR_RGB   = 0x08;   // Not implemented

    // Vendor-specific commands that we use to detect an LCD model
    static const uint8_t CMD_MAGIC_TRULY   = 0xC4;

    // Width of emulated TE pulses
    static const unsigned TE_WIDTH_US = 1000;

    uint32_t write_count;
    uint32_t te_timer_head;
    uint32_t te_timer_tail;

    /* Hardware interface */
    uint8_t prev_wrx;

    /*
     * LCD Controller State
     */
 
    uint8_t current_cmd;
    uint8_t cmd_bytecount;
    uint8_t pixel_bytes[3];

    unsigned xs, xe, ys, ye;
    unsigned row, col;

    uint8_t madctr;
    uint8_t colmod;
    uint8_t mode_awake;
    uint8_t mode_display_on;
    uint8_t mode_te;

    /*
     * Model-specific emulation characteristics.
     *
     * These really just undo some of the model-specific tweaks
     * that the firmware performs. The emulation isn't particularly
     * accurate, but it's close enough for our purposes right now.
     */

    struct {
        uint8_t madctr_xor;
        int8_t row_adj;
        int8_t col_adj;
    } model;
};

#endif
