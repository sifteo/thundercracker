#include "tasks.h"
#include <sifteo.h>
#include "audiomixer.h"

#ifndef SIFTEO_SIMULATOR
#include "usb.h"
#endif

using namespace Sifteo;

uint32_t Tasks::pendingMask;

Tasks::Task Tasks::TaskList[MAX_TASKS] = {
    #ifdef SIFTEO_SIMULATOR
    { 0 },
    { 0 },
    #else
    { Usb::handleINData, 0 },
    { Usb::handleOUTData, 0 },
    #endif
    { AudioMixer::handleAudioOutEmpty, 0 }
};

void Tasks::init()
{
    pendingMask = 0;
}

/*
    Pend a given task handler to be run the next time we have time.
*/
void Tasks::setPending(TaskID id, void* p)
{
    ASSERT(TaskList[id].callback != NULL);
    TaskList[id].param = p;
    Atomic::SetLZ(pendingMask, id);
}

/*
    Service any tasks that are pending since our last round of work.
    Should be called in main thread context, as callbacks might typically
    take some time - it's the whole reason Tasks exists, after all.
*/
void Tasks::work()
{
    while (pendingMask) {
        unsigned idx = Intrinsic::CLZ(pendingMask);
        // clear before calling back since callback might take a while and
        // the flag might get set again in the meantime
        Atomic::ClearLZ(pendingMask, idx);
        Task &task = TaskList[idx];
        task.callback(task.param);
    }
}
