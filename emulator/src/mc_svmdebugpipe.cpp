/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <string.h>
#include "lua_script.h"
#include "svm.h"
#include "svmdebugpipe.h"
#include "svmmemory.h"
#include "svmruntime.h"
#include "elfdefs.h"
#include "mc_elfdebuginfo.h"
#include "mc_gdbserver.h"
#include "mc_logdecoder.h"
#include "lua_runtime.h"
#include "tinythread.h"
#include "tasks.h"
using namespace Svm;

static ELFDebugInfo gELFDebugInfo;
static LogDecoder gLogDecoder;
static LuaScript *gLuaScript;


static struct DebuggerMailbox {
    tthread::mutex m;
    tthread::condition_variable cond;

    uint32_t cmdWords;
    uint32_t replyWords;
    uint32_t cmd[Debugger::MAX_CMD_WORDS];
    uint32_t reply[Debugger::MAX_REPLY_WORDS];
} gDebuggerMailbox;


bool SvmDebugPipe::fault(FaultCode code)
{
    /*
     * Does Lua code want to handle the fault?
     */

    if (LuaRuntime::onFault(code))
        return true;

    /*
     * Nope. Pass it through, but log it along the way.
     */

    uint32_t pcVA = SvmRuntime::reconstructCodeAddr(SvmCpu::reg(REG_PC));
    std::string pcName = formatAddress(pcVA);

    LOG(("***\n"
         "*** (>\")> --[ Uh oh!  A VM Fault! ]\n"
         "***\n"
         "***   Code %x (%s)\n"
         "***\n"
         "***   PC: va=%08x pa=%p%s at %s\n"
         "***   SP: va=%08x pa=%p%s\n"
         "***  GPR: %08x %08x %08x %08x\n"
         "***       %08x %08x %08x %08x\n"
         "***\n"
         "*** Stack Backtrace:\n"
         "***\n",
         code, faultString(code),
 
         pcVA,
         reinterpret_cast<void*>(SvmCpu::reg(REG_PC)),
         SvmMemory::isAddrValid(SvmCpu::reg(REG_PC)) ? "" : " (INVALID)",
         pcName.c_str(),

         (unsigned)SvmMemory::physToVirtRAM(SvmCpu::reg(REG_SP)),
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

    SvmMemory::VirtAddr fpVA = SvmCpu::reg(REG_FP);
    SvmMemory::PhysAddr fpPA;
    while (SvmMemory::mapRAM(fpVA, sizeof(CallFrame), fpPA)) {
        CallFrame *frame = reinterpret_cast<CallFrame*>(fpPA);
        std::string symbolName = formatAddress(frame->pc);

        LOG(("***  [%08x] pc=%08x  %s\n",
            (unsigned)SvmMemory::physToVirtRAM(fpPA),
            frame->pc, symbolName.c_str()));

        fpVA = frame->fp;
    }

    LOG(("***\n"));

    return false;
}

uint32_t *SvmDebugPipe::logReserve(SvmLogTag tag)
{
    // On real hardware, we'll be writing directly into a USB ring buffer.
    // On simulation, we can just stow the parameters in a temporary global
    // buffer, and decode them to stdout immediately.

    static uint32_t buffer[7];
    return buffer;
}

void SvmDebugPipe::logCommit(SvmLogTag tag, uint32_t *buffer, uint32_t bytes)
{
    // On real hardware, log decoding would be deferred to the host machine.
    // In simulation, we can decode right away. Note that we use the raw Flash
    // interface instead of going through the cache, since we don't want
    // debug log decoding to affect caching behavior.

    uint32_t decodedSize = gLogDecoder.decode(gELFDebugInfo, tag, buffer);
    ASSERT(decodedSize == bytes);
}

std::string SvmDebugPipe::formatAddress(uint32_t address)
{
    return gELFDebugInfo.formatAddress(address);
}

std::string SvmDebugPipe::formatAddress(void *address)
{
    return gELFDebugInfo.formatAddress(SvmMemory::physToVirtRAM((uint8_t*)address));
}

bool SvmDebugPipe::debuggerMsgAccept(SvmDebugPipe::DebuggerMsg &msg)
{
    /*
     * First half of handling a debugger message: If a message
     * is available, this returns a pointer to the buffer memory
     * for both the message and its reply. The caller retains ownership
     * of this buffer until calling debuggerMsgFinish().
     *
     * This call must be non-blocking.
     */

    DebuggerMailbox &mbox = gDebuggerMailbox;
    mbox.m.lock();
    if (mbox.cmdWords == 0) {
        mbox.m.unlock();
        return false;
    }

    msg.cmd = mbox.cmd;
    msg.reply = mbox.reply;
    msg.cmdWords = mbox.cmdWords;
    msg.replyWords = 0;

    // Remove the message from our mbox
    mbox.cmdWords = 0;

    // Leave mbox.m locked.
    return true;
}

void SvmDebugPipe::debuggerMsgFinish(SvmDebugPipe::DebuggerMsg &msg)
{
    /*
     * Finish handling a debugger message. Must be paired with any
     * successful call to debuggerMsgAccept.
     *
     * This will signal the debuggerMsgCallback() to wake up, which
     * will eventually free the messagebox by setting cmdWords=0.
     */

    DebuggerMailbox &mbox = gDebuggerMailbox;
    mbox.replyWords = msg.replyWords;
    mbox.cond.notify_all();
    mbox.m.unlock();
}

static uint32_t debuggerMsgCallback(const uint32_t *cmd,
    uint32_t cmdWords, uint32_t *reply)
{
    /*
     * Called by the GDBServer thread to synchronously deliver a message
     * and wait for a reply. This posts a new message to the mailbox, and
     * waits on a condvar until the reply has appeared.
     */

    // Zero-byte replies are vaild; use a special EMPTY token.
    const uint32_t EMPTY = (uint32_t) -1;

    DebuggerMailbox &mbox = gDebuggerMailbox;
    tthread::lock_guard<tthread::mutex> guard(mbox.m);

    // Post the command
    mbox.replyWords = EMPTY;
    mbox.cmdWords = cmdWords;
    memcpy(mbox.cmd, cmd, cmdWords * sizeof(uint32_t));

    // Wake up the debugger's event loop
    Tasks::trigger(Tasks::Debugger);

    // Wait for a reply
    do {
        mbox.cond.wait(mbox.m);
    } while (mbox.replyWords == EMPTY);

    // Free the buffer
    mbox.cmdWords = 0;

    // Return the reply
    uint32_t replyWords = mbox.replyWords;
    memcpy(reply, mbox.reply, replyWords * sizeof(uint32_t));
    return replyWords;
}

void SvmDebugPipe::setSymbolSource(const Elf::Program &program)
{
    gELFDebugInfo.init(program);
    GDBServer::setDebugInfo(&gELFDebugInfo);
    GDBServer::setMessageCallback(debuggerMsgCallback);
}

static void luaHandler(const char *str, void*)
{
    ASSERT(gLuaScript);
    if (gLuaScript->runString(str))
        SvmRuntime::fault(F_SCRIPT_EXCEPTION);
}

void SvmDebugPipe::init()
{
    delete gLuaScript;
    gLuaScript = new LuaScript(*SystemMC::getSystem());

    LogDecoder::ScriptHandler lua = { luaHandler };
    gLogDecoder.init();
    gLogDecoder.setScriptHandler(_SYS_SCRIPT_LUA, lua);
}
