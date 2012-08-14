#include "homebutton.h"
#include <glfw.h>
#include "tasks.h"
#include "macros.h"
#include "pause.h"

namespace HomeButton
{

static uint32_t state;


void init()
{
    state = 0;
}

void setPressed(bool value)
{
    if (state != value) {
        state = value;
        HomeButton::update();
        Pause::taskWork.atomicMark(Pause::ButtonPress);
        Tasks::trigger(Tasks::Pause);
    }
}

bool hwIsPressed()
{
    return state;
}

} // namespace HomeButton
