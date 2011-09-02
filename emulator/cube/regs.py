#!/usr/bin/env python

# nRF24LE1 data sheet, Table 28
regs = """
  P0 SP DPL DPH DPL1 DPH1 - -
  TCON TMOD TL0 TL1 TH0 TH1 - P3CON
  P1 - DPS P0DIR P1DIR P2DIR P3DIR P2CON
  S0CON S0BUF - - - - P0CON P1CON
  P2 PWMDC0 PWMDC1 CLKCTRL PWRDWN WUCON INTEXP MEMCON
  IEN0 IP0 S0RELL RTC2CPT01 RTC2CPT10 CLKLFCTRL OPMCON WDSV
  P3 RSTREAS PWMCON RTC2CON RTC2CMP0 RTC2CMP1 RTC2CPT00 -
  IEN1 IP1 S0RELH - SPISCON0 - SPISSTAT SPISDAT
  IRCON CCEN CCL1 CCH1 CCL2 CCH2 CCL3 CCH3
  T2CON MPAGE CRCL CRCH TL2 TH2 WUOPC1 WUOPC0
  PSW ADCCON3 ADCCON2 ADCCON1 ADCDATH ADCDATL RNGCTL RNGDAT
  ADCON W2SADR W2DAT COMPCON POFCON CCPDATIA CCPDATIB CCPDATO
  ACC W2CON1 W2CON0 - SPIRCON0 SPIRCON1 SPIRSTAT SPIRDAT
  RFCON MD0 MD1 MD2 MD3 MD4 MD5 ARCON
  B - - - - - - -
  FSR FPCR FCR - SPIMCON0 SPIMCON1 SPIMSTAT SPIMDAT
"""

sfrNames = regs.split()
assert len(sfrNames) == 128

for i, name in enumerate(sfrNames):
    if name != '-':
        print "    REG_%-12s = 0x%02X - 0x80," % (name, i + 0x80)

print

for i, name in enumerate(sfrNames):
    if name != '-':
        print "__sfr __at 0x%02X %s;" % (i + 0x80, name)

print

for i, name in enumerate(sfrNames):
    if name == '-':
        name = "SFR_%02X" % (i + 0x80)
    print '%-12s' % ('"%s",' % name),
    if (i & 3) == 3:
        print
