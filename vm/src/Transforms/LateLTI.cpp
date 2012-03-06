/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Constants.h"
#include "llvm/Type.h"
#include "llvm/LLVMContext.h"
#include "llvm/Instructions.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Analysis/ValueTracking.h"
using namespace llvm;

namespace llvm {
    BasicBlockPass *createLateLTIPass();
}

namespace {
    class LateLTIPass : public BasicBlockPass {
    public:
        static char ID;
        LateLTIPass()
            : BasicBlockPass(ID) {}

        virtual bool runOnBasicBlock (BasicBlock &BB);

        virtual const char *getPassName() const {
            return "Late link-time intrinsics";
        }
        
    private:
        bool runOnCall(CallSite &CS, StringRef Name);
        void transformLog(CallSite &CS, uint32_t flags = 0);
        void quoteString(const std::string &in, std::string &out);        
        void validateFormatStr(CallSite &CS, std::string &Fmt);
    };
}

char LateLTIPass::ID = 0;

BasicBlockPass *llvm::createLateLTIPass()
{
    return new LateLTIPass();
}

bool LateLTIPass::runOnBasicBlock (BasicBlock &BB)
{
    bool Changed = false;

    for (BasicBlock::iterator I = BB.begin(), E = BB.end(); I != E;) {
        CallSite CS(cast<Value>(I));
        ++I;

        if (CS) {
            Function *F = CS.getCalledFunction();
            if (F && runOnCall(CS, F->getName()))
                Changed = true;
        }
    }

    return Changed;
}

bool LateLTIPass::runOnCall(CallSite &CS, StringRef Name)
{
    if (Name == "_SYS_lti_log") {
        transformLog(CS);
        return true;
    }

    return false;
}

void LateLTIPass::transformLog(CallSite &CS, uint32_t flags)
{
    // We convert _SYS_lti_log(fmt, ...) to _SYS_log(tag, ...),
    // where tag includes such things as the function's arity,
    // and a string table offset for the actual format string data.
    
    const char *sysLog = "_SYS_25";
    const int maxArgs = 7;
    
    Instruction *I = CS.getInstruction();
    LLVMContext &Ctx = I->getContext();
    Type *i32 = Type::getInt32Ty(Ctx);
    Module *M = I->getParent()->getParent()->getParent();

    // Switch functions; call _SYS_log() instead.
    std::vector<Type*> ArgTys;
    ArgTys.push_back(i32);
    CS.setCalledFunction(M->getOrInsertFunction(sysLog,
        FunctionType::get(Type::getVoidTy(Ctx), ArgTys, true),
        AttrListPtr::get((AttributeWithIndex *)0, 0)));

    // We support a limited number of log arguments
    int numLogArgs = CS.arg_size() - 1;
    assert(numLogArgs >= 0);
    flags |= numLogArgs << 24;
    if (numLogArgs > maxArgs)
        report_fatal_error(Twine("_SYS_lti_log() with too many arguments. ")
            + Twine(numLogArgs) + Twine(" supplied, only ") + Twine(maxArgs)
            + Twine(" allowed."));

    // The format string must be constant at compile-time.
    std::string fmtStr;
    if (!GetConstantStringInfo(CS.getArgument(0), fmtStr))
        report_fatal_error("Format string for _SYS_lti_log() is not verifiably constant.");

    // Convert all arguments to i32 or float.
    for (int argNum = 1; argNum <= numLogArgs; argNum++) {
        Value *v = CS.getArgument(argNum);

        if (v->getType()->isFloatingPointTy())
            CS.setArgument(argNum, CastInst::CreateFPCast(v, Type::getFloatTy(Ctx), "", I));
        else if (v->getType() != i32)
            CS.setArgument(argNum, CastInst::CreatePointerCast(v, i32, "", I));
    }

    // Validate format string and argument types
    validateFormatStr(CS, fmtStr);

    // Create a new string in our special section
    Constant *StrConstant = ConstantArray::get(Ctx, fmtStr, true);
    GlobalVariable *GV = new GlobalVariable(*M, StrConstant->getType(),
                                            true, GlobalValue::PrivateLinkage,
                                            StrConstant, "", 0, false);
    GV->setAlignment(1);
    GV->setName("logstr");
    GV->setSection(".debug_logstr");

    // Cast from pointer to integer, then add our flags word
    Value *castV = CastInst::CreatePointerCast(GV, i32, "", I);
    Value *flagsV = BinaryOperator::CreateAdd(castV,
        ConstantInt::get(i32, flags), "", I);
    CS.setArgument(0, flagsV);
}

void LateLTIPass::quoteString(const std::string &in, std::string &out)
{
    out = "\"";

    for (unsigned i = 0, e = in.size(); i != e; ++i) {
        unsigned char C = in[i];

        if (C == '"' || C == '\\') {
            out += '\\';
            out += C;
            continue;
        }

        if (isprint((unsigned char)C)) {
            out += C;
            continue;
        }

        switch (C) {
          case '\b': out += "\\b"; break;
          case '\f': out += "\\f"; break;
          case '\n': out += "\\n"; break;
          case '\r': out += "\\r"; break;
          case '\t': out += "\\t"; break;
          default:
            out += '\\';
            out += char('0' + (C >> 6));
            out += char('0' + (C >> 3));
            out += char('0' + (C >> 0));
            break;
        }
    }

    out += "\"";
}

void LateLTIPass::validateFormatStr(CallSite &CS, std::string &Fmt)
{
    /*
     * Make sure the specified format string matches the available arguments.
     */

    std::string quotedFmt;
    quoteString(Fmt, quotedFmt);
    unsigned argCount = 0;

    for (std::string::iterator I = Fmt.begin(), E = Fmt.end(); I != E; ++I) {
        if (*I != '%')
            continue;

        bool done = false;
        do {
            char spec = *(++I);

            switch (spec) {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                case ' ':
                case '-':
                case '+':
                case '.':
                    break;

                case '%':
                    done = true;
                    break;

                case 'd':
                case 'i':
                case 'o':
                case 'u':
                case 'X':
                case 'x':
                case 'c':
                    done = true;
                    argCount++;
                    if (argCount >= CS.arg_size())
                        report_fatal_error("Not enough arguments for format string " + quotedFmt);
                    if (!CS.getArgument(argCount)->getType()->isIntegerTy())
                        report_fatal_error(Twine("Argument ")
                            + Twine(argCount) + Twine(" for format string ") + quotedFmt
                            + " is not an integer type.");
                    break;

                case 'f':
                case 'F':
                case 'e':
                case 'E':
                case 'g':
                case 'G':
                    done = true;
                    argCount++;
                    if (argCount >= CS.arg_size())
                        report_fatal_error("Not enough arguments for format string " + quotedFmt);
                    if (!CS.getArgument(argCount)->getType()->isFloatTy())
                        report_fatal_error(Twine("Argument ")
                            + Twine(argCount) + Twine(" for format string ") + quotedFmt
                            + " is not a floating point type.");
                    break;

                case 0:
                    report_fatal_error("Premature end of format string " + quotedFmt);
                    break;

                default:
                    report_fatal_error("Unsupported character '" + Twine(spec)
                        + "' in format string " + quotedFmt);
                    break;
            }
        } while (!done);
    }

    if (argCount + 1 != CS.arg_size())
        report_fatal_error("Too many arguments for format string " + quotedFmt);
}
