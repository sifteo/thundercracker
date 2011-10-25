/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "graphics_sprite.h"


void vm_bg0_spr_line()
{
    /*
     * XXX: Very slow unoptimized implementation. Using this to develop tests...
     */

    uint8_t bg0_addr = x_bg0_first_addr;
    uint8_t bg0_wrap = x_bg0_wrap;
    uint8_t x = 128;
    
    DPTR = y_bg0_map;
    
    do {
        struct x_sprite_t __idata *s = x_spr;
        uint8_t ns = y_spr_active;

        // Sprites

        do {
            if (!(s->mask & s->pos)) {
                ADDR_PORT = s->lat1;
                CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;
                    
                ADDR_PORT = s->lat2;
                CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT2;
                    
                ADDR_PORT = s->line_addr + ((s->pos & 7) << 2);
        
                if (BUS_PORT != _SYS_CHROMA_KEY)
                    goto pixel_done;

                s->pos++;

                if (!(7 & s->pos)) {
                    s->lat1 += 2;
                    if (!s->lat1)       
                        s->lat2 += 2;
                }

            } else {
                s->pos++;
            }

            s++;
        } while (--ns);
    
        // BG0
        __asm ADDR_FROM_DPTR(_DPL) __endasm;
        ADDR_PORT = y_bg0_addr_l + bg0_addr;
        
        pixel_done:
        ADDR_INC4();
        
        if (!(bg0_addr = (bg0_addr + 4) & 0x1F)) {
            DPTR_INC2();
            BG0_WRAP_CHECK();
        }
 
        if (ns)
            do {
                if (!(s->mask & s->pos)) {
                    s->pos++;

                    if (!(7 & s->pos)) {
                        s->lat1 += 2;
                        if (!s->lat1)       
                            s->lat2 += 2;
                    }
                } else {
                    s->pos++;
                }
                
                s++;
            } while (--ns);

    } while (--x);
}

void vm_bg0_spr_bg1_line()
{
    /*
     * XXX: Very slow unoptimized implementation. Using this to develop tests...
     */

    uint8_t bg0_addr = x_bg0_first_addr;
    uint8_t bg1_addr = x_bg1_first_addr;
    uint8_t bg0_wrap = x_bg0_wrap;
    uint8_t x = 128;
    
    DPTR = y_bg0_map;
    
    do {
        struct x_sprite_t __idata *s = x_spr;
        uint8_t ns = y_spr_active;

        // BG1
        if (MD0 & 1) {
            __asm
                inc     _DPS
                ADDR_FROM_DPTR(_DPL1)
                dec     _DPS
            __endasm ;
            ADDR_PORT = y_bg1_addr_l + bg1_addr;            
            if (BUS_PORT != _SYS_CHROMA_KEY)
                goto pixel_done;
        }
       
        // Sprites

        do {
            if (!(s->mask & s->pos)) {
                ADDR_PORT = s->lat1;
                CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT1;
                    
                ADDR_PORT = s->lat2;
                CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT2;
                    
                ADDR_PORT = s->line_addr + ((s->pos & 7) << 2);
        
                if (BUS_PORT != _SYS_CHROMA_KEY)
                    goto pixel_done;

                s->pos++;

                if (!(7 & s->pos)) {
                    s->lat1 += 2;
                    if (!s->lat1)       
                        s->lat2 += 2;
                }

            } else {
                s->pos++;
            }

            s++;
        } while (--ns);
    
        // BG0
        __asm ADDR_FROM_DPTR(_DPL) __endasm;
        ADDR_PORT = y_bg0_addr_l + bg0_addr;
        
        pixel_done:
        ADDR_INC4();
        
        if (!(bg0_addr = (bg0_addr + 4) & 0x1F)) {
            DPTR_INC2();
            BG0_WRAP_CHECK();
        }
        
        if (!(bg1_addr = (bg1_addr + 4) & 0x1F)) {
            if (MD0 & 1) {
                __asm
                    inc     _DPS
                    inc     dptr
                    inc     dptr
                    dec     _DPS
                    anl     _DPH1, #3
                __endasm ;
            }
            MD0 = MD0;  // Dummy write to reset MDU
            ARCON = 0x21;
        }

        if (ns)
            do {
                if (!(s->mask & s->pos)) {
                    s->pos++;

                    if (!(7 & s->pos)) {
                        s->lat1 += 2;
                        if (!s->lat1)       
                            s->lat2 += 2;
                    }
                } else {
                    s->pos++;
                }
                
                s++;
            } while (--ns);

    } while (--x);
}


/*************************** TO REWORK ********************************************************/
 
#if 0

static void line_bg_spr0(void)
{
    uint8_t x = 15;
    uint8_t bg_wrap = x_bg_wrap;
    register uint8_t spr0_mask = lvram.sprites[0].mask_x;
    register uint8_t spr0_x = lvram.sprites[0].x + x_bg_first_w;
    uint8_t spr0_pixel_addr = (spr0_x - 1) << 2;

    DPTR = y_bg_map;

    // First partial or full tile
    ADDR_FROM_DPTR_INC();
    MAP_WRAP_CHECK();
    ADDR_PORT = y_bg_addr_l + x_bg_first_addr;
    PIXEL_BURST(x_bg_first_w);

    // There are always 15 full tiles on-screen
    do {
        if ((spr0_x & spr0_mask) && ((spr0_x + 7) & spr0_mask)) {
            // All 8 pixels are non-sprite

            ADDR_FROM_DPTR_INC();
            MAP_WRAP_CHECK();
            ADDR_PORT = y_bg_addr_l;
            addr_inc32();
            spr0_x += 8;
            spr0_pixel_addr += 32;

        } else {
            // A mixture of sprite and tile pixels.

#define SPR0_OPAQUE(i)                          \
        test_##i:                               \
            if (BUS_PORT == CHROMA_KEY)         \
                goto transparent_##i;           \
            ADDR_INC4();                        \

#define SPR0_TRANSPARENT_TAIL(i)                \
        transparent_##i:                        \
            ADDR_FROM_DPTR();                   \
            ADDR_PORT = y_bg_addr_l + (i*4);    \
            ADDR_INC4();                        \

#define SPR0_TRANSPARENT(i, j)                  \
            SPR0_TRANSPARENT_TAIL(i);           \
            ADDR_FROM_SPRITE(0);                \
            ADDR_PORT = spr0_pixel_addr + (j*4);\
            goto test_##j;                      \

#define SPR0_END()                              \
            spr0_x += 8;                        \
            spr0_pixel_addr += 32;              \
            DPTR_INC2();                        \
            MAP_WRAP_CHECK();                   \
            continue;                           \

            // Fast path: All opaque pixels in a row.

            // XXX: The assembly generated by sdcc for this loop is okayish, but
            //      still rather bad. There are still a lot of gains left to be had
            //      by using inline assembly here.

            ADDR_FROM_SPRITE(0);
            ADDR_PORT = spr0_pixel_addr;
            SPR0_OPAQUE(0);
            SPR0_OPAQUE(1);
            SPR0_OPAQUE(2);
            SPR0_OPAQUE(3);
            SPR0_OPAQUE(4);
            SPR0_OPAQUE(5);
            SPR0_OPAQUE(6);
            SPR0_OPAQUE(7);
            SPR0_END();

            // Transparent pixel jump targets

            SPR0_TRANSPARENT(0, 1);
            SPR0_TRANSPARENT(1, 2);
            SPR0_TRANSPARENT(2, 3);
            SPR0_TRANSPARENT(3, 4);
            SPR0_TRANSPARENT(4, 5);
            SPR0_TRANSPARENT(5, 6);
            SPR0_TRANSPARENT(6, 7);
            SPR0_TRANSPARENT_TAIL(7);
            SPR0_END();
        }

    } while (--x);

    // Might be one more partial tile
    if (x_bg_last_w) {
        ADDR_FROM_DPTR_INC();
        MAP_WRAP_CHECK();
        ADDR_PORT = y_bg_addr_l;
        PIXEL_BURST(x_bg_last_w);
    }

    do {
        uint8_t active_sprites = 0;

        /*
         * Per-line sprite accounting. Update all Y coordinates, and
         * see which sprites are active. (Unrolled loop here, to allow
         * calculating masks and array addresses at compile-time.)
         */

#define SPRITE_Y_ACCT(i)                                                \
        if (!(++lvram.sprites[i].y & lvram.sprites[i].mask_y)) {        \
            active_sprites |= 1 << i;                                   \
            lvram.sprites[i].addr_l += 2;                               \
        }                                                               \

        SPRITE_Y_ACCT(0);
        SPRITE_Y_ACCT(1);

        /*
         * Choose a scanline renderer
         */

        switch (active_sprites) {
        case 0x00:      line_bg(); break;
        case 0x01:      line_bg_spr0(); break;
        }
#endif
