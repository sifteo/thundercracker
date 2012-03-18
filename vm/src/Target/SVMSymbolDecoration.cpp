/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMSymbolDecoration.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Module.h"
#include "llvm/Constant.h"
#include "llvm/GlobalValue.h"
#include "llvm/GlobalAlias.h"
#include "llvm/Type.h"
#include "llvm/DerivedTypes.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCValue.h"
#include "llvm/MC/MCExpr.h"
using namespace llvm;

// Prefixes that are intended for use in source code
const char SVMDecorations::SYS[] = "_SYS_";
        
// For internal use
const char SVMDecorations::CALL[] = "_call$";
const char SVMDecorations::TCALL[] = "_tcall$";
const char SVMDecorations::LB[] = "_lb$";
const char SVMDecorations::OFFSET[] = "_o$";
const char SVMDecorations::META[] = "_meta$";
const char SVMDecorations::INIT[] = "_init$";
const char SVMDecorations::SEPARATOR[] = "$";

Constant *SVMDecorations::Apply(Module *M, const GlobalValue *Value, Twine Prefix)
{
    /*
     * Get or create a Constant that represents a decorated version of
     * the given GlobalValue. The decorated symbol is 'Prefix' concatenated
     * with the original symbol value.
     */

    GlobalValue *GV = (GlobalValue*) Value;
    Twine Name = Prefix + GV->getName();
    GlobalAlias *GA = M->getNamedAlias(Name.str());

    if (!GA)
        GA = new GlobalAlias(GV->getType(), GlobalValue::ExternalLinkage, Name, GV, M);

    return GA;
}

Constant *SVMDecorations::ApplyOffset(Module *M, const GlobalValue *Value, int32_t offset)
{
    if (offset)
        return Apply(M, Value, Twine(OFFSET) + Twine(offset) + Twine(SEPARATOR));
    else
        return (Constant*) Value;
}

StringRef SVMDecorations::Decode(StringRef Name)
{
    // Internal prefixes are always stripped
    isLongBranch = testAndStripPrefix(Name, LB);
    isTailCall = testAndStripPrefix(Name, TCALL);
    isCall = isTailCall || testAndStripPrefix(Name, CALL);
    
    // Offset prefix includes a number and a separator
    testAndStripNumberedPrefix(Name, OFFSET, offset);

    // XXX: Not sure why, but Clang is prepending this junk to __asm__ symbols
    testAndStripPrefix(Name, "\x01");
    testAndStripPrefix(Name, "_01_");

    /*
     * At this point, we have the full user-specified name of either
     * a normal or special symbol. This is always the value we return,
     * even if further results are decoded into the SVMDecorations struct.
     */
    StringRef Result = Name;

    // Numeric syscalls can be written in any base, using C-style numbers
    isSys = testAndStripPrefix(Name, SYS) && !Name.getAsInteger(0, sysNumber);

    // Numeric metadata keys
    isMeta = testAndStripNumberedPrefix(Name, META, metaKey);

    return Result;
}

bool SVMDecorations::testAndStripPrefix(StringRef &Name, StringRef Prefix)
{
    if (Name.startswith(Prefix)) {
        Name = Name.substr(Prefix.size());
        return true;
    }
    return false;
}

bool SVMDecorations::testAndStripNumberedPrefix(StringRef &Name,
    StringRef Prefix, int32_t &num)
{
    // Prefix + Number + Separator

    num = 0;
    if (testAndStripPrefix(Name, Prefix)) {
        size_t len = Name.find(SEPARATOR[0]);
        if (len != Name.npos && !Name.substr(0, len).getAsInteger(0, num)) {
            Name = Name.substr(len+1);
            return true;
        }
    }
    
    return false;
}
