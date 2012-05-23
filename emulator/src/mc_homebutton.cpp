#include "homebutton.h"
#include <glfw.h>
#include "tasks.h"
#include "macros.h"

namespace HomeButton
{

void init()
{
    // no-op in simulator
}

void onChange()
{
    if (isPressed()) {
        Tasks::setPending(Tasks::HomeButton);
    }
}

bool isPressed()
{
    return glfwGetKey('B') == GLFW_PRESS;
}

/*
 * Called from within Tasks::work to handle a button event on the main loop.
 */
void task(void *p)
{
    // do things here
}

} // namespace Button
