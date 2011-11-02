/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Hardware platform definitions
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _HARDWARE_H
#define _HARDWARE_H

/*
 * GPIO Ports
 */

#define ADDR_PORT       P0
#define ADDR_DIR        P0DIR
#define ADDR_CON        P0CON

#define MISC_PORT       P1
#define MISC_DIR        P1DIR
#define MISC_CON        P1CON
#define _MISC_DIR       _P1DIR
#define _MISC_CON       _P1CON

#define BUS_PORT        P2
#define BUS_DIR         P2DIR
#define BUS_CON         P2CON

#define CTRL_PORT       P3
#define CTRL_DIR        P3DIR
#define CTRL_CON        P3CON

__sbit __at 0xA0 CTRL_LCD_TE;      // XXX: Hardware not ready for TE yet

#define MISC_NB_OUT1    (1 << 0)
#define MISC_NB_OUT2    (1 << 1)
#define MISC_I2C_SCL    (1 << 2)
#define MISC_I2C_SDA    (1 << 3)
#define MISC_TOUCH      (1 << 4)   // AIN12 and GPINT2
#define MISC_NB_OUT3    (1 << 5)
#define MISC_NB_IN      (1 << 6)   // T1 input
#define MISC_NB_OUT4    (1 << 7)

#define TOUCH_ADC_CH    12

#define MISC_I2C        (MISC_I2C_SCL | MISC_I2C_SDA)
#define MISC_NB_OUT     (MISC_NB_OUT1 | MISC_NB_OUT2 | MISC_NB_OUT3 | MISC_NB_OUT4)

#define MISC_NB_MASK0   (MISC_NB_OUT1 | MISC_NB_OUT2)
#define MISC_NB_MASK1   (MISC_NB_OUT1 | MISC_NB_OUT3)

#define MISC_DIR_VALUE  (~MISC_I2C_SCL)
#define MISC_IDLE       (MISC_I2C_SCL | MISC_I2C_SDA)

#define CTRL_LCD_DCX    (1 << 0)
#define CTRL_FLASH_LAT1 (1 << 1)
#define CTRL_FLASH_LAT2 (1 << 2)
#define CTRL_3V3_EN     (1 << 3)
#define CTRL_BACKLIGHT  (1 << 4)
#define CTRL_FLASH_WE   (1 << 5)
#define CTRL_FLASH_OE   (1 << 6)

#define CTRL_DIR_VALUE  (~(CTRL_LCD_DCX | CTRL_FLASH_LAT1 | CTRL_FLASH_LAT2 | \
                           CTRL_FLASH_WE | CTRL_FLASH_OE | CTRL_BACKLIGHT | \
                           CTRL_3V3_EN ))

#define CTRL_IDLE       (CTRL_BACKLIGHT | CTRL_FLASH_WE | CTRL_FLASH_OE | CTRL_LCD_DCX)
#define CTRL_FLASH_CMD  (CTRL_BACKLIGHT | CTRL_FLASH_OE | CTRL_LCD_DCX)
#define CTRL_LCD_CMD    (CTRL_BACKLIGHT | CTRL_FLASH_WE | CTRL_FLASH_OE)
#define CTRL_FLASH_OUT  (CTRL_BACKLIGHT | CTRL_FLASH_WE | CTRL_LCD_DCX)

/*
 * Debug UART (P1.0, 38400 baud)
 */

#define DEBUG_UART_INIT() {                     \
        IEN_RI0_TI0 = 0;                        \
        P1 |= 1;                                \
        P1DIR &= 0xFE;                          \
        S0CON_TI0 = 1;                          \
        S0CON_SM0 = 0;                          \
        S0CON_SM1 = 1;                          \
        PCON |= 0x80;                           \
        ADCON |= 0x80;                          \
        S0RELL = 0xf3;                          \
        S0RELH = 0x03;                          \
    }

#define DEBUG_UART(_b) {                        \
        while (!S0CON_TI0);                     \
        S0CON_TI0 = 0;                          \
        S0BUF = (_b);                           \
    }

#define ASM_DEBUG_UART(_b)                      __endasm; \
    __asm jnb   _S0CON_TI0, (.)                 __endasm; \
    __asm clr   _S0CON_TI0                      __endasm; \
    __asm mov   _S0BUF, _b                      __endasm; \
    __asm

/*
 * nRF24L01 Radio
 */

#define RF_PAYLOAD_MAX          32

/* SPI Commands */
#define RF_CMD_R_REGISTER       0x00
#define RF_CMD_W_REGISTER       0x20
#define RF_CMD_R_RX_PAYLOAD     0x61
#define RF_CMD_W_TX_PAYLOAD     0xA0
#define RF_CMD_FLUSH_TX         0xE1
#define RF_CMD_FLUSH_RX         0xE2
#define RF_CMD_REUSE_TX_PL      0xE3
#define RF_CMD_R_RX_PL_WID      0x60
#define RF_CMD_W_ACK_PAYLD      0xA8
#define RF_CMD_W_TX_PAYLD_NOACK 0xB0
#define RF_CMD_NOP              0xFF

/* Registers */
#define RF_REG_CONFIG           0x00
#define RF_REG_EN_AA            0x01
#define RF_REG_EN_RXADDR        0x02
#define RF_REG_SETUP_AW         0x03
#define RF_REG_SETUP_RETR       0x04
#define RF_REG_RF_CH            0x05
#define RF_REG_RF_SETUP         0x06
#define RF_REG_STATUS           0x07
#define RF_REG_OBSERVE_TX       0x08
#define RF_REG_RPD              0x09
#define RF_REG_RX_ADDR_P0       0x0A
#define RF_REG_RX_ADDR_P1       0x0B
#define RF_REG_RX_ADDR_P2       0x0C
#define RF_REG_RX_ADDR_P3       0x0D
#define RF_REG_RX_ADDR_P4       0x0E
#define RF_REG_RX_ADDR_P5       0x0F
#define RF_REG_TX_ADDR          0x10
#define RF_REG_RX_PW_P0         0x11
#define RF_REG_RX_PW_P1         0x12
#define RF_REG_RX_PW_P2         0x13
#define RF_REG_RX_PW_P3         0x14
#define RF_REG_RX_PW_P4         0x15
#define RF_REG_RX_PW_P5         0x16
#define RF_REG_FIFO_STATUS      0x17
#define RF_REG_DYNPD            0x1C
#define RF_REG_FEATURE          0x1D

/* STATUS bits */
#define RF_STATUS_TX_FULL       0x01
#define RF_STATUS_RX_P_MASK     0X0E
#define RF_STATUS_MAX_RT        0x10
#define RF_STATUS_TX_DS         0x20
#define RF_STATUS_RX_DR         0x40

/* FIFO_STATUS bits */
#define RF_FIFO_RX_EMPTY        0x01
#define RF_FIFO_RX_FULL         0x02
#define RF_FIFO_TX_EMPTY        0x10
#define RF_FIFO_TX_FULL         0x20
#define RF_FIFO_TX_REUSE        0x40

/*
 * CPU instruction macros
 */

#define rl(x)   (((x) << 1) | ((x) >> 7))
#define rr(x)   (((x) >> 1) | ((x) << 7))
#define swap(x) (((x) >> 4) | ((x) << 4))

// Global interrupt enable. We never disable interrupts globally.
#define sti()   { IEN_EN = 1; }

/*
 * CPU Special Function Registers
 */

__sfr16 __at 0x8382 DPTR;

__sfr __at 0x80 P0;
__sfr __at 0x81 SP;
__sfr __at 0x82 DPL;
__sfr __at 0x83 DPH;
__sfr __at 0x84 DPL1;
__sfr __at 0x85 DPH1;
__sfr __at 0x86 DEBUG_REG;   // Simulator only
__sfr __at 0x87 PCON;
__sfr __at 0x88 TCON;
__sfr __at 0x89 TMOD;
__sfr __at 0x8A TL0;
__sfr __at 0x8B TL1;
__sfr __at 0x8C TH0;
__sfr __at 0x8D TH1;
__sfr __at 0x8F volatile P3CON;
__sfr __at 0x90 P1;
__sfr __at 0x92 DPS;
__sfr __at 0x93 P0DIR;
__sfr __at 0x94 P1DIR;
__sfr __at 0x95 P2DIR;
__sfr __at 0x96 P3DIR;
__sfr __at 0x97 volatile P2CON;
__sfr __at 0x98 S0CON;
__sfr __at 0x99 S0BUF;
__sfr __at 0x9E volatile P0CON;
__sfr __at 0x9F volatile P1CON;
__sfr __at 0xA0 P2;
__sfr __at 0xA1 PWMDC0;
__sfr __at 0xA2 PWMDC1;
__sfr __at 0xA3 CLKCTRL;
__sfr __at 0xA4 PWRDWN;
__sfr __at 0xA5 WUCON;
__sfr __at 0xA6 INTEXP;
__sfr __at 0xA7 MEMCON;
__sfr __at 0xA8 IEN0;
__sfr __at 0xA9 IP0;
__sfr __at 0xAA S0RELL;
__sfr __at 0xAB RTC2CPT01;
__sfr __at 0xAC RTC2CPT10;
__sfr __at 0xAD CLKLFCTRL;
__sfr __at 0xAE OPMCON;
__sfr __at 0xAF WDSV;
__sfr __at 0xB0 P3;
__sfr __at 0xB1 RSTREAS;
__sfr __at 0xB2 PWMCON;
__sfr __at 0xB3 RTC2CON;
__sfr __at 0xB4 RTC2CMP0;
__sfr __at 0xB5 RTC2CMP1;
__sfr __at 0xB6 RTC2CPT00;
__sfr __at 0xB8 IEN1;
__sfr __at 0xB9 IP1;
__sfr __at 0xBA S0RELH;
__sfr __at 0xBC SPISCON0;
__sfr __at 0xBE SPISSTAT;
__sfr __at 0xBF SPISDAT;
__sfr __at 0xC0 IRCON;
__sfr __at 0xC1 CCEN;
__sfr __at 0xC2 CCL1;
__sfr __at 0xC3 CCH1;
__sfr __at 0xC4 CCL2;
__sfr __at 0xC5 CCH2;
__sfr __at 0xC6 CCL3;
__sfr __at 0xC7 CCH3;
__sfr __at 0xC8 T2CON;
__sfr __at 0xC9 MPAGE;
__sfr __at 0xCA CRCL;
__sfr __at 0xCB CRCH;
__sfr __at 0xCC TL2;
__sfr __at 0xCD TH2;
__sfr __at 0xCE WUOPC1;
__sfr __at 0xCF WUOPC0;
__sfr __at 0xD0 PSW;
__sfr __at 0xD1 ADCCON3;
__sfr __at 0xD2 ADCCON2;
__sfr __at 0xD3 ADCCON1;
__sfr __at 0xD4 ADCDATH;
__sfr __at 0xD5 ADCDATL;
__sfr __at 0xD6 RNGCTL;
__sfr __at 0xD7 RNGDAT;
__sfr __at 0xD8 ADCON;
__sfr __at 0xD9 W2SADR;
__sfr __at 0xDA volatile W2DAT;
__sfr __at 0xDB COMPCON;
__sfr __at 0xDC POFCON;
__sfr __at 0xDD CCPDATIA;
__sfr __at 0xDE CCPDATIB;
__sfr __at 0xDF CCPDATO;
__sfr __at 0xE0 ACC;
__sfr __at 0xE1 W2CON1;
__sfr __at 0xE2 volatile W2CON0;
__sfr __at 0xE4 SPIRCON0;
__sfr __at 0xE5 SPIRCON1;
__sfr __at 0xE6 SPIRSTAT;
__sfr __at 0xE7 volatile SPIRDAT;
__sfr __at 0xE8 RFCON;
__sfr __at 0xE9 MD0;
__sfr __at 0xEA MD1;
__sfr __at 0xEB MD2;
__sfr __at 0xEC MD3;
__sfr __at 0xED MD4;
__sfr __at 0xEE MD5;
__sfr __at 0xEF ARCON;
__sfr __at 0xF0 B;
__sfr __at 0xF8 FSR;
__sfr __at 0xF9 FPCR;
__sfr __at 0xFA FCR;
__sfr __at 0xFC SPIMCON0;
__sfr __at 0xFD SPIMCON1;
__sfr __at 0xFE SPIMSTAT;
__sfr __at 0xFF SPIMDAT;

// S0CON bits
__sbit __at 0x98 S0CON_RI0;
__sbit __at 0x99 S0CON_TI0;
__sbit __at 0x9a S0CON_RB80;
__sbit __at 0x9b S0CON_TB80;
__sbit __at 0x9c S0CON_REN0;
__sbit __at 0x9d S0CON_SM20;
__sbit __at 0x9e S0CON_SM1;
__sbit __at 0x9f S0CON_SM0;

// T2CON bits
__sbit __at 0xc8 T2CON_T2I0;
__sbit __at 0xc9 T2CON_T2I1;
__sbit __at 0xca T2CON_T2CM;
__sbit __at 0xcb T2CON_T2R0;
__sbit __at 0xcc T2CON_T2R1;
__sbit __at 0xcd T2CON_I2FR;
__sbit __at 0xce T2CON_I3FR;
__sbit __at 0xcf T2CON_T2PS;

// TCON bits
__sbit __at 0x88 TCON_IT0;
__sbit __at 0x89 TCON_IE0;
__sbit __at 0x8a TCON_IT1;
__sbit __at 0x8b TCON_IE1;
__sbit __at 0x8c TCON_TR0;
__sbit __at 0x8d TCON_TF0;
__sbit __at 0x8e TCON_TR1;
__sbit __at 0x8f TCON_TF1;

// IEN0 bits
__sbit __at 0xA8 IEN_IFP;
__sbit __at 0xA9 IEN_TF0;
__sbit __at 0xAA IEN_POF;
__sbit __at 0xAB IEN_TF1;
__sbit __at 0xAC IEN_RI0_TI0;
__sbit __at 0xAD IEN_TF2_EXF2;
__sbit __at 0xAF IEN_EN;

// IEN1 bits
__sbit __at 0xB8 IEN_RFSPI;
__sbit __at 0xB9 IEN_RF;
__sbit __at 0xBA IEN_SPI_I2C;
__sbit __at 0xBB IEN_WUOP;
__sbit __at 0xBC IEN_MISC;
__sbit __at 0xBD IEN_TICK;
__sbit __at 0xBF IEN_EXF2;

// IRCON bits
__sbit __at 0xC0 IR_RFSPI;
__sbit __at 0xC1 IR_RF;
__sbit __at 0xC2 IR_SPI;
__sbit __at 0xC3 IR_WUP0;
__sbit __at 0xC4 IR_MISC;
__sbit __at 0xC5 IR_TICK;
__sbit __at 0xC6 IR_TF2;
__sbit __at 0xC7 IR_EXF2;

// RFCON bits
__sbit __at 0xE8 RF_CE;
__sbit __at 0xE9 RF_CSN;
__sbit __at 0xEA RF_CKEN;

// Interrupt vector numbers
#define VECTOR_IFP      0
#define VECTOR_TF0      1
#define VECTOR_PFAIL    2
#define VECTOR_TF1      3
#define VECTOR_SER      4
#define VECTOR_TF2      5
#define VECTOR_RFSPI    8
#define VECTOR_RF       9
#define VECTOR_SPI_I2C  10
#define VECTOR_WUOP     11
#define VECTOR_MISC     12
#define VECTOR_TICK     13

// SPI Master status bits, used in CON1 and STATUS
#define SPI_RX_FULL     0x08
#define SPI_RX_READY    0x04
#define SPI_TX_EMPTY    0x02
#define SPI_TX_READY    0x01

// W2CON0 bits
#define W2CON0_START    0x10
#define W2CON0_STOP     0x20

// W2CON1 bits
#define W2CON1_ACK      0x02
#define W2CON1_READY    0x01


#endif // __HARDWARE_H
