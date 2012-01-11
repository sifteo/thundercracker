#ifndef TASKS_H
#define TASKS_H

#include <stdint.h>

class Tasks
{
public:
    static const uint8_t MAX_TASKS = 32;

    enum TaskID {
        UsbIN,
        UsbOUT
    };

    static void init();
    static void work();
    static void setPending(TaskID id, void* p);

private:
    typedef void (*TaskCallback)(void *);

    static uint32_t pending;
    struct Task {
        TaskCallback callback;
        void *param;
    };

    static Task TaskList[MAX_TASKS];
};

#endif // TASKS_H
