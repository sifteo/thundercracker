#include "homebutton.h"
#include <glfw.h>
#include "tasks.h"
#include "macros.h"

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
        Tasks::trigger(Tasks::HomeButton);
    }
}

bool isPressed()
{
    return state;
}


} // namespace HomeButton

