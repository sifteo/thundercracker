//===- ARMInstrFormats.td - ARM Instruction Formats ----------*- tablegen -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
//
// ARM Instruction Format Definitions.
//

// Format specifies the encoding used by the instruction.  This is part of the
// ad-hoc solution used to emit machine instruction encodings by our machine
// code emitter.
class Format<bits<6> val> {
  bits<6> Value = val;
}

def Pseudo        : Format<0>;
def MulFrm        : Format<1>;
def BrFrm         : Format<2>;
def BrMiscFrm     : Format<3>;

def DPFrm         : Format<4>;
def DPSoRegRegFrm    : Format<5>;

def LdFrm         : Format<6>;
def StFrm         : Format<7>;
def LdMiscFrm     : Format<8>;
def StMiscFrm     : Format<9>;
def LdStMulFrm    : Format<10>;

def LdStExFrm     : Format<11>;

def ArithMiscFrm  : Format<12>;
def SatFrm        : Format<13>;
def ExtFrm        : Format<14>;

def VFPUnaryFrm   : Format<15>;
def VFPBinaryFrm  : Format<16>;
def VFPConv1Frm   : Format<17>;
def VFPConv2Frm   : Format<18>;
def VFPConv3Frm   : Format<19>;
def VFPConv4Frm   : Format<20>;
def VFPConv5Frm   : Format<21>;
def VFPLdStFrm    : Format<22>;
def VFPLdStMulFrm : Format<23>;
def VFPMiscFrm    : Format<24>;

def ThumbFrm      : Format<25>;
def MiscFrm       : Format<26>;

def NGetLnFrm     : Format<27>;
def NSetLnFrm     : Format<28>;
def NDupFrm       : Format<29>;
def NLdStFrm      : Format<30>;
def N1RegModImmFrm: Format<31>;
def N2RegFrm      : Format<32>;
def NVCVTFrm      : Format<33>;
def NVDupLnFrm    : Format<34>;
def N2RegVShLFrm  : Format<35>;
def N2RegVShRFrm  : Format<36>;
def N3RegFrm      : Format<37>;
def N3RegVShFrm   : Format<38>;
def NVExtFrm      : Format<39>;
def NVMulSLFrm    : Format<40>;
def NVTBLFrm      : Format<41>;
def DPSoRegImmFrm  : Format<42>;

// Misc flags.

// The instruction has an Rn register operand.
// UnaryDP - Indicates this is a unary data processing instruction, i.e.
// it doesn't have a Rn operand.
class UnaryDP    { bit isUnaryDataProc = 1; }

// Xform16Bit - Indicates this Thumb2 instruction may be transformed into
// a 16-bit Thumb instruction if certain conditions are met.
class Xform16Bit { bit canXformTo16Bit = 1; }

//===----------------------------------------------------------------------===//
// ARM Instruction flags.  These need to match ARMBaseInstrInfo.h.
//

// FIXME: Once the JIT is MC-ized, these can go away.
// Addressing mode.
class AddrMode<bits<5> val> {
  bits<5> Value = val;
}
def AddrModeNone    : AddrMode<0>;
def AddrMode1       : AddrMode<1>;
def AddrMode2       : AddrMode<2>;
def AddrMode3       : AddrMode<3>;
def AddrMode4       : AddrMode<4>;
def AddrMode5       : AddrMode<5>;
def AddrMode6       : AddrMode<6>;
def AddrModeT1_1    : AddrMode<7>;
def AddrModeT1_2    : AddrMode<8>;
def AddrModeT1_4    : AddrMode<9>;
def AddrModeT1_s    : AddrMode<10>;
def AddrModeT2_i12  : AddrMode<11>;
def AddrModeT2_i8   : AddrMode<12>;
def AddrModeT2_so   : AddrMode<13>;
def AddrModeT2_pc   : AddrMode<14>;
def AddrModeT2_i8s4 : AddrMode<15>;
def AddrMode_i12    : AddrMode<16>;

// Load / store index mode.
class IndexMode<bits<2> val> {
  bits<2> Value = val;
}
def IndexModeNone : IndexMode<0>;
def IndexModePre  : IndexMode<1>;
def IndexModePost : IndexMode<2>;
def IndexModeUpd  : IndexMode<3>;

// Instruction execution domain.
class Domain<bits<3> val> {
  bits<3> Value = val;
}
def GenericDomain : Domain<0>;
def VFPDomain     : Domain<1>; // Instructions in VFP domain only
def NeonDomain    : Domain<2>; // Instructions in Neon domain only
def VFPNeonDomain : Domain<3>; // Instructions in both VFP & Neon domains
def VFPNeonA8Domain : Domain<5>; // Instructions in VFP & Neon under A8

//===----------------------------------------------------------------------===//
// ARM special operands.
//

// ARM imod and iflag operands, used only by the CPS instruction.
def imod_op : Operand<i32> {
  let PrintMethod = "printCPSIMod";
}

def ProcIFlagsOperand : AsmOperandClass {
  let Name = "ProcIFlags";
  let ParserMethod = "parseProcIFlagsOperand";
}
def iflags_op : Operand<i32> {
  let PrintMethod = "printCPSIFlag";
  let ParserMatchClass = ProcIFlagsOperand;
}

// ARM Predicate operand. Default to 14 = always (AL). Second part is CC
// register whose default is 0 (no register).
def CondCodeOperand : AsmOperandClass { let Name = "CondCode"; }
def pred : PredicateOperand<OtherVT, (ops i32imm, i32imm),
                                     (ops (i32 14), (i32 zero_reg))> {
  let PrintMethod = "printPredicateOperand";
  let ParserMatchClass = CondCodeOperand;
  let DecoderMethod = "DecodePredicateOperand";
}

// Conditional code result for instructions whose 's' bit is set, e.g. subs.
def CCOutOperand : AsmOperandClass { let Name = "CCOut"; }
def cc_out : OptionalDefOperand<OtherVT, (ops CCR), (ops (i32 zero_reg))> {
  let EncoderMethod = "getCCOutOpValue";
  let PrintMethod = "printSBitModifierOperand";
  let ParserMatchClass = CCOutOperand;
  let DecoderMethod = "DecodeCCOutOperand";
}

// Same as cc_out except it defaults to setting CPSR.
def s_cc_out : OptionalDefOperand<OtherVT, (ops CCR), (ops (i32 CPSR))> {
  let EncoderMethod = "getCCOutOpValue";
  let PrintMethod = "printSBitModifierOperand";
  let ParserMatchClass = CCOutOperand;
  let DecoderMethod = "DecodeCCOutOperand";
}

// ARM special operands for disassembly only.
//
def SetEndAsmOperand : AsmOperandClass {
  let Name = "SetEndImm";
  let ParserMethod = "parseSetEndImm";
}
def setend_op : Operand<i32> {
  let PrintMethod = "printSetendOperand";
  let ParserMatchClass = SetEndAsmOperand;
}

def MSRMaskOperand : AsmOperandClass {
  let Name = "MSRMask";
  let ParserMethod = "parseMSRMaskOperand";
}
def msr_mask : Operand<i32> {
  let PrintMethod = "printMSRMaskOperand";
  let DecoderMethod = "DecodeMSRMask";
  let ParserMatchClass = MSRMaskOperand;
}

// Shift Right Immediate - A shift right immediate is encoded differently from
// other shift immediates. The imm6 field is encoded like so:
//
//    Offset    Encoding
//     8        imm6<5:3> = '001', 8 - <imm> is encoded in imm6<2:0>
//     16       imm6<5:4> = '01', 16 - <imm> is encoded in imm6<3:0>
//     32       imm6<5> = '1', 32 - <imm> is encoded in imm6<4:0>
//     64       64 - <imm> is encoded in imm6<5:0>
def shr_imm8  : Operand<i32> {
  let EncoderMethod = "getShiftRight8Imm";
  let DecoderMethod = "DecodeShiftRight8Imm";
}
def shr_imm16 : Operand<i32> {
  let EncoderMethod = "getShiftRight16Imm";
  let DecoderMethod = "DecodeShiftRight16Imm";
}
def shr_imm32 : Operand<i32> {
  let EncoderMethod = "getShiftRight32Imm";
  let DecoderMethod = "DecodeShiftRight32Imm";
}
def shr_imm64 : Operand<i32> {
  let EncoderMethod = "getShiftRight64Imm";
  let DecoderMethod = "DecodeShiftRight64Imm";
}

//===----------------------------------------------------------------------===//
// ARM Instruction templates.
//


class InstTemplate<AddrMode am, int sz, IndexMode im,
                   Format f, Domain d, string cstr, InstrItinClass itin>
  : Instruction {
  let Namespace = "ARM";

  AddrMode AM = am;
  int Size = sz;
  IndexMode IM = im;
  bits<2> IndexModeBits = IM.Value;
  Format F = f;
  bits<6> Form = F.Value;
  Domain D = d;
  bit isUnaryDataProc = 0;
  bit canXformTo16Bit = 0;
  // The instruction is a 16-bit flag setting Thumb instruction. Used
  // by the parser to determine whether to require the 'S' suffix on the
  // mnemonic (when not in an IT block) or preclude it (when in an IT block).
  bit thumbArithFlagSetting = 0;

  // If this is a pseudo instruction, mark it isCodeGenOnly.
  let isCodeGenOnly = !eq(!cast<string>(f), "Pseudo");

  // The layout of TSFlags should be kept in sync with ARMBaseInfo.h.
  let TSFlags{4-0}   = AM.Value;
  let TSFlags{6-5}   = IndexModeBits;
  let TSFlags{12-7} = Form;
  let TSFlags{13}    = isUnaryDataProc;
  let TSFlags{14}    = canXformTo16Bit;
  let TSFlags{17-15} = D.Value;
  let TSFlags{18}    = thumbArithFlagSetting;

  let Constraints = cstr;
  let Itinerary = itin;
}

class Encoding {
  field bits<32> Inst;
}

class InstARM<AddrMode am, int sz, IndexMode im,
              Format f, Domain d, string cstr, InstrItinClass itin>
  : InstTemplate<am, sz, im, f, d, cstr, itin>, Encoding {
  let DecoderNamespace = "ARM";
}

// This Encoding-less class is used by Thumb1 to specify the encoding bits later
// on by adding flavors to specific instructions.
class InstThumb<AddrMode am, int sz, IndexMode im,
                Format f, Domain d, string cstr, InstrItinClass itin>
  : InstTemplate<am, sz, im, f, d, cstr, itin> {
  let DecoderNamespace = "Thumb";
}

class PseudoInst<dag oops, dag iops, InstrItinClass itin, list<dag> pattern>
  : InstTemplate<AddrModeNone, 0, IndexModeNone, Pseudo,
                 GenericDomain, "", itin> {
  let OutOperandList = oops;
  let InOperandList = iops;
  let Pattern = pattern;
  let isCodeGenOnly = 1;
  let isPseudo = 1;
}

class tPseudoInst<dag oops, dag iops, int sz, InstrItinClass itin,
                    list<dag> pattern>
  : PseudoInst<oops, iops, itin, pattern> {
  let Size = sz;
}


//===----------------------------------------------------------------------===//
// Thumb Instruction Format Definitions.
//

class ThumbI<dag oops, dag iops, AddrMode am, int sz,
             InstrItinClass itin, string asm, string cstr, list<dag> pattern>
  : InstThumb<am, sz, IndexModeNone, ThumbFrm, GenericDomain, cstr, itin> {
  let OutOperandList = oops;
  let InOperandList = iops;
  let AsmString = asm;
  let Pattern = pattern;
}

// TI - Thumb instruction.
class TI<dag oops, dag iops, InstrItinClass itin, string asm, list<dag> pattern>
  : ThumbI<oops, iops, AddrModeNone, 2, itin, asm, "", pattern>;

// Two-address instructions
class TIt<dag oops, dag iops, InstrItinClass itin, string asm,
          list<dag> pattern>
  : ThumbI<oops, iops, AddrModeNone, 2, itin, asm, "$lhs = $dst",
           pattern>;

// tBL, tBX 32-bit instructions
class TIx2<bits<5> opcod1, bits<2> opcod2, bit opcod3,
           dag oops, dag iops, InstrItinClass itin, string asm,
           list<dag> pattern>
    : ThumbI<oops, iops, AddrModeNone, 4, itin, asm, "", pattern>,
      Encoding {
  let Inst{31-27} = opcod1;
  let Inst{15-14} = opcod2;
  let Inst{12}    = opcod3;
}

// BR_JT instructions
class TJTI<dag oops, dag iops, InstrItinClass itin, string asm,
           list<dag> pattern>
  : ThumbI<oops, iops, AddrModeNone, 0, itin, asm, "", pattern>;

// Thumb1 only
class Thumb1I<dag oops, dag iops, AddrMode am, int sz,
              InstrItinClass itin, string asm, string cstr, list<dag> pattern>
  : InstThumb<am, sz, IndexModeNone, ThumbFrm, GenericDomain, cstr, itin> {
  let OutOperandList = oops;
  let InOperandList = iops;
  let AsmString = asm;
  let Pattern = pattern;
}

class T1I<dag oops, dag iops, InstrItinClass itin,
          string asm, list<dag> pattern>
  : Thumb1I<oops, iops, AddrModeNone, 2, itin, asm, "", pattern>;
class T1Ix2<dag oops, dag iops, InstrItinClass itin,
            string asm, list<dag> pattern>
  : Thumb1I<oops, iops, AddrModeNone, 4, itin, asm, "", pattern>;

// Two-address instructions
class T1It<dag oops, dag iops, InstrItinClass itin,
           string asm, string cstr, list<dag> pattern>
  : Thumb1I<oops, iops, AddrModeNone, 2, itin,
            asm, cstr, pattern>;

// Thumb1 instruction that can either be predicated or set CPSR.
class Thumb1sI<dag oops, dag iops, AddrMode am, int sz,
               InstrItinClass itin,
               string opc, string asm, string cstr, list<dag> pattern>
  : InstThumb<am, sz, IndexModeNone, ThumbFrm, GenericDomain, cstr, itin> {
  let OutOperandList = !con(oops, (outs s_cc_out:$s));
  let InOperandList = !con(iops, (ins pred:$p));
  let AsmString = !strconcat(opc, "${s}${p}", asm);
  let Pattern = pattern;
  let thumbArithFlagSetting = 1;
  let DecoderNamespace = "ThumbSBit";
}

class T1sI<dag oops, dag iops, InstrItinClass itin,
           string opc, string asm, list<dag> pattern>
  : Thumb1sI<oops, iops, AddrModeNone, 2, itin, opc, asm, "", pattern>;

// Two-address instructions
class T1sIt<dag oops, dag iops, InstrItinClass itin,
            string opc, string asm, list<dag> pattern>
  : Thumb1sI<oops, iops, AddrModeNone, 2, itin, opc, asm,
             "$Rn = $Rdn", pattern>;

// Thumb1 instruction that can be predicated.
class Thumb1pI<dag oops, dag iops, AddrMode am, int sz,
               InstrItinClass itin,
               string opc, string asm, string cstr, list<dag> pattern>
  : InstThumb<am, sz, IndexModeNone, ThumbFrm, GenericDomain, cstr, itin> {
  let OutOperandList = oops;
  let InOperandList = !con(iops, (ins pred:$p));
  let AsmString = !strconcat(opc, "${p}", asm);
  let Pattern = pattern;
}

class T1pI<dag oops, dag iops, InstrItinClass itin,
           string opc, string asm, list<dag> pattern>
  : Thumb1pI<oops, iops, AddrModeNone, 2, itin, opc, asm, "", pattern>;

// Two-address instructions
class T1pIt<dag oops, dag iops, InstrItinClass itin,
            string opc, string asm, list<dag> pattern>
  : Thumb1pI<oops, iops, AddrModeNone, 2, itin, opc, asm,
             "$Rn = $Rdn", pattern>;

class T1pIs<dag oops, dag iops,
            InstrItinClass itin, string opc, string asm, list<dag> pattern>
  : Thumb1pI<oops, iops, AddrModeT1_s, 2, itin, opc, asm, "", pattern>;

class Encoding16 : Encoding {
  let Inst{31-16} = 0x0000;
}

// A6.2 16-bit Thumb instruction encoding
class T1Encoding<bits<6> opcode> : Encoding16 {
  let Inst{15-10} = opcode;
}

// A6.2.1 Shift (immediate), add, subtract, move, and compare encoding.
class T1General<bits<5> opcode> : Encoding16 {
  let Inst{15-14} = 0b00;
  let Inst{13-9} = opcode;
}

// A6.2.2 Data-processing encoding.
class T1DataProcessing<bits<4> opcode> : Encoding16 {
  let Inst{15-10} = 0b010000;
  let Inst{9-6} = opcode;
}

// A6.2.3 Special data instructions and branch and exchange encoding.
class T1Special<bits<4> opcode> : Encoding16 {
  let Inst{15-10} = 0b010001;
  let Inst{9-6}   = opcode;
}

// A6.2.4 Load/store single data item encoding.
class T1LoadStore<bits<4> opA, bits<3> opB> : Encoding16 {
  let Inst{15-12} = opA;
  let Inst{11-9}  = opB;
}
class T1LdStSP<bits<3> opB>   : T1LoadStore<0b1001, opB>; // SP relative

class T1BranchCond<bits<4> opcode> : Encoding16 {
  let Inst{15-12} = opcode;
}

// Helper classes to encode Thumb1 loads and stores. For immediates, the
// following bits are used for "opA" (see A6.2.4):
//
//   0b0110 => Immediate, 4 bytes
//   0b1000 => Immediate, 2 bytes
//   0b0111 => Immediate, 1 byte
class T1pILdStEncode<bits<3> opcode, dag oops, dag iops, AddrMode am,
                     InstrItinClass itin, string opc, string asm,
                     list<dag> pattern>
  : Thumb1pI<oops, iops, am, 2, itin, opc, asm, "", pattern>,
    T1LoadStore<0b0101, opcode> {
  bits<3> Rt;
  bits<8> addr;
  let Inst{8-6} = addr{5-3};    // Rm
  let Inst{5-3} = addr{2-0};    // Rn
  let Inst{2-0} = Rt;
}
class T1pILdStEncodeImm<bits<4> opA, bit opB, dag oops, dag iops, AddrMode am,
                        InstrItinClass itin, string opc, string asm,
                        list<dag> pattern>
  : Thumb1pI<oops, iops, am, 2, itin, opc, asm, "", pattern>,
    T1LoadStore<opA, {opB,?,?}> {
  bits<3> Rt;
  bits<8> addr;
  let Inst{10-6} = addr{7-3};   // imm5
  let Inst{5-3}  = addr{2-0};   // Rn
  let Inst{2-0}  = Rt;
}

// A6.2.5 Miscellaneous 16-bit instructions encoding.
class T1Misc<bits<7> opcode> : Encoding16 {
  let Inst{15-12} = 0b1011;
  let Inst{11-5} = opcode;
}

