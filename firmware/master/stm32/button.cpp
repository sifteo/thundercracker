
#include "button.h"
#include "hardware.h"


Button pushButton(GPIOPin(&GPIOG, 8)); // MCBSTM32E eval board has USER button on PG8

//#include "audiomixer.h"
//#include "audio.gen.h"
//static _SYSAudioHandle handle = -1;
//static const Sifteo::AudioModule &mod = blueshift_8kHz;

void Button::enablePushButton()
{
    NVIC_SIFTEO.irqEnable(IVT.EXTI9_5);
    pushButton.init();
}

void Button::init()
{
    irq.setControl(GPIOPin::IN_FLOAT);
    irq.irqInit();
    irq.irqSetFallingEdge();                // TODO - tweak once we have real HW
    irq.irqEnable();
}

void Button::isr()
{
    irq.irqAcknowledge();
    // TODO - hook up
//    if (!AudioMixer::instance.isPlaying(handle)) {
//        AudioMixer::instance.play(mod, &handle);
//    }

}

IRQ_HANDLER ISR_EXTI9_5()
{
    pushButton.isr();
}
