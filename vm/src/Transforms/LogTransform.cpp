/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * This file implements LogTransform(), which converts a _SYS_lti_log()
 * call to any number of _SYS_log() calls.
 *
 * We parse the log's format string, and build zero or more new format strings
 * which get included in _SYS_log() calls. The string literals are included
 * in a new log-specific string table section, and information about the log
 * is packed along with a string table offset into a single 32-bit parameter.
 */

#include "Target/SVMRuntime.inc"
#include "Support/ErrorReporter.h"
#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Constants.h"
#include "llvm/Type.h"
#include "llvm/LLVMContext.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Analysis/ValueTracking.h"
#include <string.h>
#include <stdio.h>

using namespace llvm;


namespace {

    class LogBuilder {
    public:
        LogBuilder(Instruction *I, uint32_t flags)
            : I(I), Ctx(I->getContext()), i32(Type::getInt32Ty(Ctx)),
            M(I->getParent()->getParent()->getParent()), defaultFlags(flags)
        {
            // Create a reference to _SYS_log()
            std::vector<Type*> ArgTys;
            ArgTys.push_back(i32);
            sysLogFn = M->getOrInsertFunction(SVMRT_log,
                FunctionType::get(Type::getVoidTy(Ctx), ArgTys, true),
                AttrListPtr::get((AttributeWithIndex *)0, 0));
        }

        void parse(std::string::iterator &fmtBegin, std::string::iterator fmtEnd,
            CallSite::arg_iterator &argBegin, CallSite::arg_iterator argEnd)
        {
            do {
                if (*fmtBegin != '%') {
                    // Literal character
                    fmt += *fmtBegin;
                    ++fmtBegin;
                    continue;
                }

                handleSpecifier(fmtBegin, fmtEnd, argBegin, argEnd);

            } while (fmtBegin != fmtEnd);

            if (argBegin != argEnd)
                reportError("Too many parameters");

            flush();
        }

    private:
        static const unsigned MAX_ARGS = 7;
        
        Instruction *I;
        LLVMContext &Ctx;
        Type *i32;
        Module *M;
        uint32_t defaultFlags;

        std::string fmt;
        SmallVector<Value *, 8> args;
        Value *sysLogFn;

        void reportError(Twine description)
        {
            report_fatal_error(I, "In log format string: " + description);
        }

        Value *createFlagsWord(uint32_t type, uint32_t arity, uint32_t param = 0)
        {
            uint32_t flags = defaultFlags | (arity << 24) | (type << 27) | param;
            return ConstantInt::get(i32, flags);
        }

        Value *createLogString()
        {
            // Create a new string in our special section
            Constant *StrConstant = ConstantArray::get(Ctx, fmt, true);
            GlobalVariable *GV = new GlobalVariable(*M, StrConstant->getType(),
                                                    true, GlobalValue::PrivateLinkage,
                                                    StrConstant, "", 0, false);
            GV->setAlignment(1);
            GV->setName("logstr");
            GV->setSection(".debug_logstr");

            // Cast from pointer to integer, then add our flags word
            Value *castV = CastInst::CreatePointerCast(GV, i32, "", I);
            Value *flagsV = createFlagsWord(0, args.size());
            return BinaryOperator::CreateAdd(castV, flagsV, "", I);
        }
        
        void flush()
        {
            if (!fmt.empty()) {
                // Insert a call to _SYS_log(), using the current fmt and args,
                // and clear the buffer of format data and arguments.
                args.insert(args.begin(), createLogString());
                CallInst::Create(sysLogFn, args, "", I);
                fmt = "";
                args.clear();
            }
            assert(args.empty());
        }

        void appendEscaped(const std::string &str)
        {
            // Append a string to the current format string, escaping all '%' chars.
            for (std::string::const_iterator I = str.begin(), E = str.end(); I != E; ++I) {
                char c = *I;
                if (c == '%')
                    fmt += '%';
                fmt += c;
            }
        }

        template <typename T>
        void appendScalar(std::string fmt, T value)
        {
            // Append a simple formatted integer or floating point scalar value.
            char buf[256];
            snprintf(buf, sizeof buf - 1, fmt.c_str(), value);
            buf[sizeof buf - 1] = '\0';
            appendEscaped(buf);
        }

        ConstantFP *sloppyConstFloat(Value *V)
        {
            // Try to convert 'V' to a constant float value, ignoring fptrunc/fpext casts.
            for (;;) {
                CastInst *CI = dyn_cast<CastInst>(V);
                if (!CI)
                    break;

                switch (CI->getOpcode()) {

                    case Instruction::FPExt:
                    case Instruction::FPTrunc:
                        V = CI->getOperand(0);
                        break;

                    default:
                        return 0;
                }
            }

            return dyn_cast<ConstantFP>(V);
        }

        void handleSpecifier(std::string::iterator &fmtBegin, std::string::iterator fmtEnd,
            CallSite::arg_iterator &argBegin, CallSite::arg_iterator argEnd)
        {
            int width = 0;
            bool dot = false;
            std::string::iterator S = fmtBegin;
            ++S;

            while (S != fmtEnd) {
                char c = *S;
                ++S;
                switch (c) {

                // Prefix characters; doesn't affect data type.
                // Most of these pass straight through into the format string,
                // but we do keep track of integer width.

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
                    if (!dot)
                        width = (width * 10) + (c - '0');
                    break;

                case '.':
                    dot = true;
                    break;

                case ' ':
                case '-':
                case '+':
                    break;

                // Literal '%'.
                case '%':
                    fmt.append(fmtBegin, S);
                    fmtBegin = S;
                    return;

                // Integer formatting
                case 'd':
                case 'i':
                case 'o':
                case 'u':
                case 'X':
                case 'x':
                case 'p':
                case 'c': {
                    if (argBegin == argEnd)
                        reportError("Too few arguments");

                    Value *v = argBegin->get();
                    if (!v->getType()->isIntegerTy() && !v->getType()->isPointerTy())
                        reportError("Expected integer or pointer argument for %" + Twine(c));
                    if (v->getType() != i32)
                        v = CastInst::CreatePointerCast(v, i32, "", I);

                    // Is the value known at compile-time?
                    ConstantInt *CV = dyn_cast<ConstantInt>(v);
                    if (CV) {
                        // Format this value now.
                        appendScalar(std::string(fmtBegin, S), (int)CV->getSExtValue());                
                    } else {
                        // Add to the format string
                        if (args.size() >= MAX_ARGS)
                            flush();
                        args.push_back(v);
                        fmt.append(fmtBegin, S);
                    }

                    fmtBegin = S;
                    ++argBegin;
                    return;
                }

                // Nonstandard integer specifiers (No compile-time resolution)
                case 'b':
                case 'P':
                case 'C': {
                    if (argBegin == argEnd)
                        reportError("Too few arguments");

                    Value *v = argBegin->get();
                    if (!v->getType()->isIntegerTy() && !v->getType()->isPointerTy())
                        reportError("Expected integer or pointer argument for %" + Twine(c));
                    if (v->getType() != i32)
                        v = CastInst::CreatePointerCast(v, i32, "", I);

                    if (args.size() >= MAX_ARGS)
                        flush();
                    args.push_back(v);
                    fmt.append(fmtBegin, S);

                    fmtBegin = S;
                    ++argBegin;
                    return;
                }

                // Floating point formatting
                case 'f':
                case 'F':
                case 'e':
                case 'E':
                case 'g':
                case 'G': {
                    if (argBegin == argEnd)
                        reportError("Too few arguments");

                    Value *v = argBegin->get();
                    if (!v->getType()->isFloatingPointTy())
                        reportError("Expected floating point argument for %" + Twine(c));
                    if (v->getType() != i32)
                        v = CastInst::CreateFPCast(v, Type::getFloatTy(Ctx), "", I);

                    // Is the value known at compile-time?
                    ConstantFP *CV = sloppyConstFloat(v);
                    if (CV) {
                        // Format this value now.
                        appendScalar(std::string(fmtBegin, S), CV->getValueAPF().convertToDouble());                
                    } else {
                        // Add to the format string
                        if (args.size() >= MAX_ARGS)
                            flush();
                        args.push_back(v);
                        fmt.append(fmtBegin, S);
                    }

                    fmtBegin = S;
                    ++argBegin;
                    return;
                }

                // String pointer formatting
                case 's': {
                    if (argBegin == argEnd)
                        reportError("Too few arguments");

                    Value *v = argBegin->get();
                    if (!v->getType()->isPointerTy())
                        reportError("Expected pointer argument for %" + Twine(c));

                    // Is this string constant at compile-time?
                    std::string vStr;
                    if (GetConstantStringInfo(v, vStr)) {
                        // Yes, fold it into the format string after escaping it.
                        appendEscaped(vStr);

                    } else {
                        // No, output a dynamic string logging call
                        flush();
                        args.push_back(createFlagsWord(1, 1));
                        args.push_back(CastInst::CreatePointerCast(v, i32, "", I));
                        CallInst::Create(sysLogFn, args, "", I);
                        args.clear();
                    }

                    fmtBegin = S;
                    ++argBegin;
                    return;
                }

                // Fixed-width buffer hex dump
                case 'h': {
                    if (argBegin == argEnd)
                        reportError("Too few arguments");

                    Value *v = argBegin->get();
                    if (!v->getType()->isPointerTy())
                        reportError("Expected pointer argument for %" + Twine(c));

                    if (width == 0)
                        reportError("The 'h' specifier (hex dump) requires a width. For example: %16h");  

                    flush();
                    args.push_back(createFlagsWord(2, 1, width));
                    args.push_back(CastInst::CreatePointerCast(v, i32, "", I));
                    CallInst::Create(sysLogFn, args, "", I);
                    args.clear();

                    fmtBegin = S;
                    ++argBegin;
                    return;
                }

                default:
                    reportError("Unsupported character '" + Twine(c) + "'");
                    break;
                }
            }

            reportError("Unexpected end of string");
        }
    };
    
}

namespace llvm {

    void LogTransform(CallSite &CS, uint32_t flags = 0)
    {
        Instruction *I = CS.getInstruction();
        LogBuilder LB(I, flags);

        // The format string must be constant at compile-time.
        std::string fmtStr;
        assert(!CS.arg_empty());
        if (!GetConstantStringInfo(CS.getArgument(0), fmtStr))
            report_fatal_error(I, "Format string for _SYS_lti_log() is not verifiably constant.");

        // Iteratively extract log entries from the original _SYS_log() call
        std::string::iterator fmtI = fmtStr.begin(), fmtE = fmtStr.end();
        CallSite::arg_iterator argI = CS.arg_begin(), argE = CS.arg_end();
        ++argI;

        LB.parse(fmtI, fmtE, argI, argE);

        // Remove the original _SYS_lti_log() call
        I->eraseFromParent();
    }

};
