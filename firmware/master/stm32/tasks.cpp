#include "tasks.h"
#include <sifteo/machine.h>
#include "usb.h"

using namespace Sifteo;

uint32_t Tasks::pending;

Tasks::Task Tasks::TaskList[MAX_TASKS] = {
    { Usb::handleINData, 0 },
    { Usb::handleOUTData, 0 },
};

void Tasks::init()
{
    pending = 0;
}

/*
    Pend a given task handler to be run the next time we have time.
*/
void Tasks::setPending(TaskID id, void* p)
{
    Task &task = TaskList[id];
    if (task.callback) {
        task.param = p;
        Atomic::SetLZ(pending, id);
    }
}

/*
    Service any tasks that are pending since our last round of work.
    Should be called in main thread context, as callbacks might typically
    take some time - it's the whole reason Tasks exists, after all.
*/
void Tasks::work()
{
    while (pending) {
        unsigned tsk = Intrinsic::CLZ(pending);
        TaskList[tsk].callback(TaskList[tsk].param);
        Atomic::And(pending, ~Intrinsic::LZ(tsk));
    }
}
