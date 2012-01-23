//===- ARMSchedule.td - ARM Scheduling Definitions ---------*- tablegen -*-===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Instruction Itinerary classes used for ARM
//
def IIC_iALUx      : InstrItinClass;
def IIC_iALUi      : InstrItinClass;
def IIC_iALUr      : InstrItinClass;
def IIC_iALUsi     : InstrItinClass;
def IIC_iALUsir    : InstrItinClass;
def IIC_iALUsr     : InstrItinClass;
def IIC_iBITi      : InstrItinClass;
def IIC_iBITr      : InstrItinClass;
def IIC_iBITsi     : InstrItinClass;
def IIC_iBITsr     : InstrItinClass;
def IIC_iUNAr      : InstrItinClass;
def IIC_iUNAsi     : InstrItinClass;
def IIC_iEXTr      : InstrItinClass;
def IIC_iEXTAr     : InstrItinClass;
def IIC_iEXTAsr    : InstrItinClass;
def IIC_iCMPi      : InstrItinClass;
def IIC_iCMPr      : InstrItinClass;
def IIC_iCMPsi     : InstrItinClass;
def IIC_iCMPsr     : InstrItinClass;
def IIC_iTSTi      : InstrItinClass;
def IIC_iTSTr      : InstrItinClass;
def IIC_iTSTsi     : InstrItinClass;
def IIC_iTSTsr     : InstrItinClass;
def IIC_iMOVi      : InstrItinClass;
def IIC_iMOVr      : InstrItinClass;
def IIC_iMOVsi     : InstrItinClass;
def IIC_iMOVsr     : InstrItinClass;
def IIC_iMOVix2    : InstrItinClass;
def IIC_iMOVix2addpc : InstrItinClass;
def IIC_iMOVix2ld  : InstrItinClass;
def IIC_iMVNi      : InstrItinClass;
def IIC_iMVNr      : InstrItinClass;
def IIC_iMVNsi     : InstrItinClass;
def IIC_iMVNsr     : InstrItinClass;
def IIC_iCMOVi     : InstrItinClass;
def IIC_iCMOVr     : InstrItinClass;
def IIC_iCMOVsi    : InstrItinClass;
def IIC_iCMOVsr    : InstrItinClass;
def IIC_iCMOVix2   : InstrItinClass;
def IIC_iMUL16     : InstrItinClass;
def IIC_iMAC16     : InstrItinClass;
def IIC_iMUL32     : InstrItinClass;
def IIC_iMAC32     : InstrItinClass;
def IIC_iMUL64     : InstrItinClass;
def IIC_iMAC64     : InstrItinClass;
def IIC_iLoad_i    : InstrItinClass;
def IIC_iLoad_r    : InstrItinClass;
def IIC_iLoad_si   : InstrItinClass;
def IIC_iLoad_iu   : InstrItinClass;
def IIC_iLoad_ru   : InstrItinClass;
def IIC_iLoad_siu  : InstrItinClass;
def IIC_iLoad_bh_i   : InstrItinClass;
def IIC_iLoad_bh_r   : InstrItinClass;
def IIC_iLoad_bh_si  : InstrItinClass;
def IIC_iLoad_bh_iu  : InstrItinClass;
def IIC_iLoad_bh_ru  : InstrItinClass;
def IIC_iLoad_bh_siu : InstrItinClass;
def IIC_iLoad_d_i  : InstrItinClass;
def IIC_iLoad_d_r  : InstrItinClass;
def IIC_iLoad_d_ru : InstrItinClass;
def IIC_iLoad_m    : InstrItinClass<0>;  // micro-coded
def IIC_iLoad_mu   : InstrItinClass<0>;  // micro-coded
def IIC_iLoad_mBr  : InstrItinClass<0>;  // micro-coded
def IIC_iPop       : InstrItinClass<0>;  // micro-coded
def IIC_iPop_Br    : InstrItinClass<0>;  // micro-coded
def IIC_iLoadiALU  : InstrItinClass;
def IIC_iStore_i   : InstrItinClass;
def IIC_iStore_r   : InstrItinClass;
def IIC_iStore_si  : InstrItinClass;
def IIC_iStore_iu  : InstrItinClass;
def IIC_iStore_ru  : InstrItinClass;
def IIC_iStore_siu : InstrItinClass;
def IIC_iStore_bh_i   : InstrItinClass;
def IIC_iStore_bh_r   : InstrItinClass;
def IIC_iStore_bh_si  : InstrItinClass;
def IIC_iStore_bh_iu  : InstrItinClass;
def IIC_iStore_bh_ru  : InstrItinClass;
def IIC_iStore_bh_siu : InstrItinClass;
def IIC_iStore_d_i   : InstrItinClass;
def IIC_iStore_d_r   : InstrItinClass;
def IIC_iStore_d_ru  : InstrItinClass;
def IIC_iStore_m   : InstrItinClass<0>;  // micro-coded
def IIC_iStore_mu  : InstrItinClass<0>;  // micro-coded
def IIC_Br         : InstrItinClass;

//===----------------------------------------------------------------------===//
// Processor instruction itineraries.

def GenericItineraries : ProcessorItineraries<[], [], []>;
