
#include "button.h"
#include "board.h"

GPIOPin Button::homeButton = BTN_HOME_GPIO;

void Button::init()
{
    homeButton.setControl(GPIOPin::IN_FLOAT);
    homeButton.irqInit();
    homeButton.irqSetFallingEdge();                // TODO - tweak once we have real HW
    homeButton.irqEnable();
}

void Button::isr()
{
    homeButton.irqAcknowledge();
    // TODO - hook up
//    if (!AudioMixer::instance.isPlaying(handle)) {
//        AudioMixer::instance.play(mod, &handle);
//    }

}

IRQ_HANDLER ISR_EXTI0()
{
    Button::isr();
}
