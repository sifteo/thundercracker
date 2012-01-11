#include "tasks.h"
#include <sifteo.h>
#include "usb.h"
#include "audiomixer.h"

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
    Task &task = TaskList[id];
    ASSERT(task.callback != NULL);

    task.param = p;
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
        Task &task = TaskList[idx];
        task.callback(task.param);
        Atomic::ClearLZ(pendingMask, idx);
    }
}
