/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _CUBE_CPU_REG_H
#define _CUBE_CPU_REG_H

namespace Cube {

#define XDATA_SIZE      1024
#define IDATA_SIZE      256
#define CODE_SIZE       16384

#define CLOCK_HZ            16000000
#define USEC_TO_CYCLES(_x)  ((int)(CLOCK_HZ * (uint64_t)(_x) / 1000000ULL))
#define NSEC_TO_CYCLES(_x)  ((int)(CLOCK_HZ * (uint64_t)(_x) / 1000000000ULL))
#define HZ_TO_CYCLES(_x)    ((int)(CLOCK_HZ / (_x)))

enum SFR_REGS
{
    REG_P0           = 0x80 - 0x80,
    REG_SP           = 0x81 - 0x80,
    REG_DPL          = 0x82 - 0x80,
    REG_DPH          = 0x83 - 0x80,
    REG_DPL1         = 0x84 - 0x80,
    REG_DPH1         = 0x85 - 0x80,
    REG_DEBUG        = 0x86 - 0x80,   // Simulator only
    REG_TCON         = 0x88 - 0x80,
    REG_TMOD         = 0x89 - 0x80,
    REG_TL0          = 0x8A - 0x80,
    REG_TL1          = 0x8B - 0x80,
    REG_TH0          = 0x8C - 0x80,
    REG_TH1          = 0x8D - 0x80,
    REG_P3CON        = 0x8F - 0x80,
    REG_P1           = 0x90 - 0x80,
    REG_DPS          = 0x92 - 0x80,
    REG_P0DIR        = 0x93 - 0x80,
    REG_P1DIR        = 0x94 - 0x80,
    REG_P2DIR        = 0x95 - 0x80,
    REG_P3DIR        = 0x96 - 0x80,
    REG_P2CON        = 0x97 - 0x80,
    REG_S0CON        = 0x98 - 0x80,
    REG_S0BUF        = 0x99 - 0x80,
    REG_P0CON        = 0x9E - 0x80,
    REG_P1CON        = 0x9F - 0x80,
    REG_P2           = 0xA0 - 0x80,
    REG_PWMDC0       = 0xA1 - 0x80,
    REG_PWMDC1       = 0xA2 - 0x80,
    REG_CLKCTRL      = 0xA3 - 0x80,
    REG_PWRDWN       = 0xA4 - 0x80,
    REG_WUCON        = 0xA5 - 0x80,
    REG_INTEXP       = 0xA6 - 0x80,
    REG_MEMCON       = 0xA7 - 0x80,
    REG_IEN0         = 0xA8 - 0x80,
    REG_IP0          = 0xA9 - 0x80,
    REG_S0RELL       = 0xAA - 0x80,
    REG_RTC2CPT01    = 0xAB - 0x80,
    REG_RTC2CPT10    = 0xAC - 0x80,
    REG_CLKLFCTRL    = 0xAD - 0x80,
    REG_OPMCON       = 0xAE - 0x80,
    REG_WDSV         = 0xAF - 0x80,
    REG_P3           = 0xB0 - 0x80,
    REG_RSTREAS      = 0xB1 - 0x80,
    REG_PWMCON       = 0xB2 - 0x80,
    REG_RTC2CON      = 0xB3 - 0x80,
    REG_RTC2CMP0     = 0xB4 - 0x80,
    REG_RTC2CMP1     = 0xB5 - 0x80,
    REG_RTC2CPT00    = 0xB6 - 0x80,
    REG_IEN1         = 0xB8 - 0x80,
    REG_IP1          = 0xB9 - 0x80,
    REG_S0RELH       = 0xBA - 0x80,
    REG_SPISCON0     = 0xBC - 0x80,
    REG_SPISSTAT     = 0xBE - 0x80,
    REG_SPISDAT      = 0xBF - 0x80,
    REG_IRCON        = 0xC0 - 0x80,
    REG_CCEN         = 0xC1 - 0x80,
    REG_CCL1         = 0xC2 - 0x80,
    REG_CCH1         = 0xC3 - 0x80,
    REG_CCL2         = 0xC4 - 0x80,
    REG_CCH2         = 0xC5 - 0x80,
    REG_CCL3         = 0xC6 - 0x80,
    REG_CCH3         = 0xC7 - 0x80,
    REG_T2CON        = 0xC8 - 0x80,
    REG_MPAGE        = 0xC9 - 0x80,
    REG_CRCL         = 0xCA - 0x80,
    REG_CRCH         = 0xCB - 0x80,
    REG_TL2          = 0xCC - 0x80,
    REG_TH2          = 0xCD - 0x80,
    REG_WUOPC1       = 0xCE - 0x80,
    REG_WUOPC0       = 0xCF - 0x80,
    REG_PSW          = 0xD0 - 0x80,
    REG_ADCCON3      = 0xD1 - 0x80,
    REG_ADCCON2      = 0xD2 - 0x80,
    REG_ADCCON1      = 0xD3 - 0x80,
    REG_ADCDATH      = 0xD4 - 0x80,
    REG_ADCDATL      = 0xD5 - 0x80,
    REG_RNGCTL       = 0xD6 - 0x80,
    REG_RNGDAT       = 0xD7 - 0x80,
    REG_ADCON        = 0xD8 - 0x80,
    REG_W2SADR       = 0xD9 - 0x80,
    REG_W2DAT        = 0xDA - 0x80,
    REG_COMPCON      = 0xDB - 0x80,
    REG_POFCON       = 0xDC - 0x80,
    REG_CCPDATIA     = 0xDD - 0x80,
    REG_CCPDATIB     = 0xDE - 0x80,
    REG_CCPDATO      = 0xDF - 0x80,
    REG_ACC          = 0xE0 - 0x80,
    REG_W2CON1       = 0xE1 - 0x80,
    REG_W2CON0       = 0xE2 - 0x80,
    REG_SPIRCON0     = 0xE4 - 0x80,
    REG_SPIRCON1     = 0xE5 - 0x80,
    REG_SPIRSTAT     = 0xE6 - 0x80,
    REG_SPIRDAT      = 0xE7 - 0x80,
    REG_RFCON        = 0xE8 - 0x80,
    REG_MD0          = 0xE9 - 0x80,
    REG_MD1          = 0xEA - 0x80,
    REG_MD2          = 0xEB - 0x80,
    REG_MD3          = 0xEC - 0x80,
    REG_MD4          = 0xED - 0x80,
    REG_MD5          = 0xEE - 0x80,
    REG_ARCON        = 0xEF - 0x80,
    REG_B            = 0xF0 - 0x80,
    REG_FSR          = 0xF8 - 0x80,
    REG_FPCR         = 0xF9 - 0x80,
    REG_FCR          = 0xFA - 0x80,
    REG_SPIMCON0     = 0xFC - 0x80,
    REG_SPIMCON1     = 0xFD - 0x80,
    REG_SPIMSTAT     = 0xFE - 0x80,
    REG_SPIMDAT      = 0xFF - 0x80,
};

enum PSW_BITS
{
    PSW_P = 0,
    PSW_UNUSED = 1,
    PSW_OV = 2,
    PSW_RS0 = 3,
    PSW_RS1 = 4,
    PSW_F0 = 5,
    PSW_AC = 6,
    PSW_C = 7
};

enum PSW_MASKS
{
    PSWMASK_P = 0x01,
    PSWMASK_UNUSED = 0x02,
    PSWMASK_OV = 0x04,
    PSWMASK_RS0 = 0x08,
    PSWMASK_RS1 = 0x10,
    PSWMASK_F0 = 0x20,
    PSWMASK_AC = 0x40,
    PSWMASK_C = 0x80
};

enum IRQ_MASKS
{
    IRQM0_IFP   = (1 << 0),
    IRQM0_TF0   = (1 << 1),
    IRQM0_PFAIL = (1 << 2),
    IRQM0_TF1   = (1 << 3),
    IRQM0_SER   = (1 << 4),
    IRQM0_TF2   = (1 << 5),
    IRQM0_EN    = (1 << 7),

    IRQM1_RFSPI = (1 << 0),
    IRQM1_RF    = (1 << 1),
    IRQM1_SPI   = (1 << 2),
    IRQM1_WUOP  = (1 << 3),
    IRQM1_MISC  = (1 << 4),
    IRQM1_TICK  = (1 << 5),
    IRQM1_TMR2EX = (1 << 7),
};

enum IRCON_MASKS
{
    IRCON_RFSPI = (1 << 0),
    IRCON_RF    = (1 << 1),
    IRCON_SPI   = (1 << 2),
    IRCON_WUOP  = (1 << 3),
    IRCON_MISC  = (1 << 4),
    IRCON_TICK  = (1 << 5),
    IRCON_TF2   = (1 << 6),
    IRCON_EXF2  = (1 << 7),
};

enum PT_MASKS
{
    PTMASK_PX0 = 0x01,
    PTMASK_PT0 = 0x02,
    PTMASK_PX1 = 0x04,
    PTMASK_PT1 = 0x08,
    PTMASK_PS  = 0x10,
    PTMASK_PT2 = 0x20,
    PTMASK_UNUSED1 = 0x40,
    PTMASK_UNUSED2 = 0x80
};

enum TCON_MASKS
{
    TCONMASK_IT0 = 0x01,
    TCONMASK_IE0 = 0x02,
    TCONMASK_IT1 = 0x04,
    TCONMASK_IE1 = 0x08,
    TCONMASK_TR0 = 0x10,
    TCONMASK_TF0 = 0x20,
    TCONMASK_TR1 = 0x40,
    TCONMASK_TF1 = 0x80
};

enum TMOD_MASKS
{
    TMODMASK_M0_0 = 0x01,
    TMODMASK_M1_0 = 0x02,
    TMODMASK_CT_0 = 0x04,
    TMODMASK_GATE_0 = 0x08,
    TMODMASK_M0_1 = 0x10,
    TMODMASK_M1_1 = 0x20,
    TMODMASK_CT_1 = 0x40,
    TMODMASK_GATE_1 = 0x80
};

enum IP_MASKS
{
    IPMASK_PX0 = 0x01,
    IPMASK_PT0 = 0x02,
    IPMASK_PX1 = 0x04,
    IPMASK_PT1 = 0x08,
    IPMASK_PS  = 0x10,
    IPMASK_PT2 = 0x20
};

// The active DPTR registers depend on the value of DPS.
#define SEL_DPL(dps)   (((dps) & 1) ? REG_DPL1 : REG_DPL)
#define SEL_DPH(dps)   (((dps) & 1) ? REG_DPH1 : REG_DPH)
#define CUR_DPL        SEL_DPL(aCPU->mSFR[REG_DPS])
#define CUR_DPH        SEL_DPH(aCPU->mSFR[REG_DPS])


};  // namespace Cube

#endif
