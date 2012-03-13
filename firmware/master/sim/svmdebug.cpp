/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <string.h>
#include "svm.h"
#include "svmdebug.h"
#include "svmruntime.h"
#include "elfdefs.h"
#include "flash.h"
using namespace Svm;

#ifndef PRIxPTR // for mingw
#define PRIxPTR "x"
#endif

static const unsigned MAX_DEBUG_SECTIONS = 32;

struct DebugSectionInfo {
    Elf::SectionHeader header;
    FlashRange data;
    char name[64];
};

static DebugSectionInfo debugSections[MAX_DEBUG_SECTIONS];

static DebugSectionInfo *findDebugSection(const char *name)
{
    for (unsigned i = 0; i < MAX_DEBUG_SECTIONS; i++) {
        DebugSectionInfo *SI = &debugSections[i];
        if (!strcmp(SI->name, name))
            return SI;
    }
    return NULL;
}

static const char* faultStr(FaultCode code)
{
    switch (code) {
    case F_STACK_OVERFLOW:      return "Stack allocation failure";
    case F_BAD_STACK:           return "Validation-time stack address error";
    case F_BAD_CODE_ADDRESS:    return "Branch-time code address error";
    case F_BAD_SYSCALL:         return "Unsupported syscall number";
    case F_LOAD_ADDRESS:        return "Runtime load address error";
    case F_STORE_ADDRESS:       return "Runtime store address error";
    case F_LOAD_ALIGNMENT:      return "Runtime load alignment error";
    case F_STORE_ALIGNMENT:     return "Runtime store alignment error";
    case F_CODE_FETCH:          return "Runtime code fetch error";
    case F_CODE_ALIGNMENT:      return "Runtime code alignment error";
    case F_CPU_SIM:             return "Unhandled ARM instruction in sim (validator bug)";
    case F_RESERVED_SVC:        return "Reserved SVC encoding";
    case F_RESERVED_ADDROP:     return "Reserved ADDROP encoding";
    case F_ABORT:               return "User call to _SYS_abort()";
    case F_LONG_STACK_LOAD:     return "Bad address in long stack LDR addrop";
    case F_LONG_STACK_STORE:    return "Bad address in long stack STR addrop";
    case F_PRELOAD_ADDRESS:     return "Bad address for async preload";
    case F_RETURN_FRAME:        return "Bad saved FP value detected during ret()";
    case F_LOG_FETCH:           return "Memory fault while fetching _SYS_log() data";
    default:                    return "unknown error";
    }
}
    
void SvmDebug::fault(FaultCode code)
{    
    LOG(("***\n"
         "*** VM FAULT code %d (%s)\n"
         "***\n"
         "***   PC: va=%08x pa=%p%s\n"
         "***   SP: va=%08x pa=%p%s\n"
         "***  GPR: %08x %08x %08x %08x\n"
         "***       %08x %08x %08x %08x\n"
         "***\n",
         code, faultStr(code),
 
         (unsigned)SvmRuntime::reconstructCodeAddr(SvmCpu::reg(REG_PC)),
         reinterpret_cast<void*>(SvmCpu::reg(REG_PC)),
         SvmMemory::isAddrValid(SvmCpu::reg(REG_PC)) ? "" : " (INVALID)",

         (unsigned)SvmMemory::physToVirtRAM(
             reinterpret_cast<SvmMemory::PhysAddr>(SvmCpu::reg(REG_SP))),
         reinterpret_cast<void*>(SvmCpu::reg(REG_SP)),
         SvmMemory::isAddrValid(SvmCpu::reg(REG_SP)) ? "" : " (INVALID)",

         (unsigned) SvmCpu::reg(0),
         (unsigned) SvmCpu::reg(1),
         (unsigned) SvmCpu::reg(2),
         (unsigned) SvmCpu::reg(3),
         (unsigned) SvmCpu::reg(4),
         (unsigned) SvmCpu::reg(5),
         (unsigned) SvmCpu::reg(6),
         (unsigned) SvmCpu::reg(7)
    ));

    SvmRuntime::exit();
}

static void formatLog(char *out, size_t outSize, char *fmt,
    uint32_t *args, size_t argCount)
{
    // This is a simple printf()-like formatter, used to format log entries.
    // It differs from printf() in two important ways:
    //
    //   - We enforce a subset of functionality which is both safe and
    //     does not require runtime memory access. This means only int, float,
    //     and character arguments are permitted.
    //
    //   - Unlike C's printf(), our floating point arguments are single
    //     precision, and always stored in a single 32-bit argument slot.
    //
    // We don't worry too much about nice error reporting here, since the
    // format string should have already been validated by slinky. We only
    // perform the minimum checks necessary to ensure runtime safety.

    char *outEnd = out + outSize - 1;
    *outEnd = '\0';

    while (out < outEnd) {
        char *segment = fmt;
        char c = *(fmt++);

        if (c != '%') {
            *(out++) = c;
            if (c)
                continue;
            break;
        }

        bool done = false;
        bool dot = false;
        bool leadingZero = false;
        int width = 0;
        do {
            char spec = *(fmt++);
            char saved = *fmt;
            *fmt = '\0';  // NUL terminate the current segment
            
            switch (spec) {
                
                // Prefix characters
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
                    if (width == 0 && spec == '0')
                        leadingZero = true;
                    width = (width * 10) + (spec - '0');
                    break;

                case ' ':
                case '-':
                case '+':
                    break;

                case '.':
                    dot = true;
                    break;

                // Literal '%'
                case '%':
                    *(out++) = '%';
                    done = true;
                    break;

                // Pointers, formatted as 32-bit hex values
                case 'p':
                    done = true;
                    ASSERT(argCount && "Too few arguments in format string");
                    out += snprintf(out, outEnd - out, "0x%08x", *args);
                    argCount--;
                    args++;
                    break;

                // Binary integers
                case 'b':
                    done = true;
                    ASSERT(argCount && "Too few arguments in format string");
                    for (int i = 31; i >= 0 && out != outEnd; --i) {
                        unsigned bits = (*args) >> i;

                        if (bits != 0) {
                            // There's a '1' at or to the left of the current bit
                            out += snprintf(out, outEnd - out, "%d", bits & 1);
                        } else if (i < width) {
                            // Pad with blanks or zeroes
                            *(out++) = leadingZero ? '0' : ' ';
                        }
                    }
                    argCount--;
                    args++;
                    break;

                // Integer parameters, passed to printf()
                case 'd':
                case 'i':
                case 'o':
                case 'u':
                case 'X':
                case 'x':
                case 'c':
                    done = true;
                    ASSERT(argCount && "Too few arguments in format string");
                    out += snprintf(out, outEnd - out, segment, *args);
                    argCount--;
                    args++;
                    break;

                // 32-bit float parameters, passed to printf()
                case 'f':
                case 'F':
                case 'e':
                case 'E':
                case 'g':
                case 'G':
                    done = true;
                    ASSERT(argCount && "Too few arguments in format string");
                    out += snprintf(out, outEnd - out, segment,
                        (double) reinterpret_cast<float&>(*args));
                    argCount--;
                    args++;
                    break;

                // 'C' specifier: Four characters packed into a 32-bit int.
                // Stops when it hits a NUL.
                case 'C':
                    done = true;
                    for (unsigned i = 0; outEnd != out && i < 4; i++) {
                        char c = (*args) >> (i * 8);
                        if (!c)
                            break;
                        *(out++) = c;
                    }
                    argCount--;
                    args++;
                    break;

                case 0:
                    ASSERT(0 && "Premature end of format string");
                    break;

                default:
                    ASSERT(0 && "Unsupported character in format string");
                    return;
            }
            
            *fmt = saved;   // Restore next character
        } while (!done);
    }
}

uint32_t *SvmDebug::logReserve(SvmLogTag tag)
{
    // On real hardware, we'll be writing directly into a USB ring buffer.
    // On simulation, we can just stow the parameters in a temporary global
    // buffer, and decode them to stdout immediately.

    static uint32_t buffer[7];
    return buffer;
}

void SvmDebug::logCommit(SvmLogTag tag, uint32_t *buffer, uint32_t bytes)
{
    // On real hardware, log decoding would be deferred to the host machine.
    // In simulation, we can decode right away. Note that we use the raw Flash
    // interface instead of going through the cache, since we don't want
    // debug log decoding to affect caching behavior.

    char outBuffer[1024];

    switch (tag.getType()) {
        
        // Stow all arguments, plus the log tag. The post-processor
        // will do some printf()-like formatting on the stored arguments.
        case _SYS_LOGTYPE_FMT: {
            DebugSectionInfo *strTab = findDebugSection(".debug_logstr");
            if (!strTab) {
                LOG(("SVMLOG: No symbol table found. Raw data:\n"
                     "\t[%08x] %08x %08x %08x %08x %08x %08x %08x\n",
                     tag.getValue(), buffer[0], buffer[1], buffer[2],
                     buffer[3], buffer[4], buffer[5], buffer[6]));
                return;
            }

            char fmt[1024];
            FlashRange rFmt = strTab->data.split(tag.getParam(), sizeof fmt);
            Flash::read(rFmt.getAddress(), (uint8_t*)fmt, rFmt.getSize());

            formatLog(outBuffer, sizeof outBuffer, fmt, buffer, tag.getArity());
            LOG(("%s", outBuffer));
            return;
        }
        
        // Print a string from the log buffer
        case _SYS_LOGTYPE_STRING: {
            memcpy(outBuffer, buffer, bytes);
            outBuffer[bytes] = '\0';
            LOG(("%s", outBuffer));
            return;
        }

        // Fixed width hex-dump
        case _SYS_LOGTYPE_HEXDUMP: {
            uint32_t size = MIN(SvmDebug::LOG_BUFFER_BYTES, tag.getParam());
            for (unsigned i = 0; i != size; i++)
                LOG(("%02x ", ((uint8_t*)buffer)[i]));
            return;
        }
        
        default:
            ASSERT(0 && "Decoding unknown log type. (syscall layer should have caught this!)");
    }
}

void SvmDebug::setSymbolSourceELF(const FlashRange &elf)
{
    /*
     * The ELF header has already been validated for runtime purposes
     * by ElfUtil. Now we want to pull out the section header table,
     * so that we can read debug symbols at runtime.
     *
     * For simplicity's sake, this function extracts all necessary
     * ELF header information into a static array of sections,
     * the global debugSections[].
     */

    memset(debugSections, 0, sizeof debugSections);

    FlashBlockRef ref;
    FlashBlock::get(ref, elf.getAddress());
    Elf::FileHeader *header = reinterpret_cast<Elf::FileHeader*>(ref->getData());

    // First pass, copy out all the headers.
    for (unsigned i = 0; i < header->e_shnum && i < MAX_DEBUG_SECTIONS; i++) {
        DebugSectionInfo &SI = debugSections[i];
        Elf::SectionHeader *pHdr = &SI.header;
        FlashRange rHdr = elf.split(header->e_shoff + i * header->e_shentsize, sizeof *pHdr);
        ASSERT(rHdr.getSize() == sizeof *pHdr);
        Flash::read(rHdr.getAddress(), (uint8_t*)pHdr, rHdr.getSize());
        SI.data = elf.split(pHdr->sh_offset, pHdr->sh_size);
    }

    // Assign section names
    if (header->e_shstrndx < header->e_shnum && header->e_shstrndx < MAX_DEBUG_SECTIONS) {
        FlashRange strTab = debugSections[header->e_shstrndx].data;
        for (unsigned i = 0; i < header->e_shnum && i < MAX_DEBUG_SECTIONS; i++) {
            DebugSectionInfo &SI = debugSections[i];
            FlashRange rStr = strTab.split(SI.header.sh_name, sizeof SI.name);
            Flash::read(rStr.getAddress(), (uint8_t*)SI.name, rStr.getSize());
            SI.name[sizeof SI.name - 1] = 0;
        }
    }
}
