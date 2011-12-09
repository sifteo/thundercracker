
#include "usart.h"

// NOTE - these divisors must reflect the startup values configured in setup.cpp
#define APB2RATE (72000000 / 2)
#define APB1RATE (72000000 / 4)

void Usart::init(int rate, StopBits bits)
{
    if (uart == &USART1) {
        RCC_SIFTEO.APB2ENR |= (1 << 14);
    }
    else if (uart == &USART2) {
        RCC_SIFTEO.APB1ENR |= (1 << 17);
    }
    else if (uart == &USART3) {
        RCC_SIFTEO.APB1ENR |= (1 << 18);
    }
    else if (uart == &UART4) {
        RCC_SIFTEO.APB1ENR |= (1 << 19);
    }
    else if (uart == &UART5) {
        RCC_SIFTEO.APB1ENR |= (1 << 20);
    }

    if (uart == &USART1)
        uart->BRR = APB2RATE / rate;
    else
        uart->BRR = APB1RATE / rate;

    uart->CR1 = (1 << 13) |     // UE - enable
                (0 << 12) |     // M - word length, 8 bits
                (0 << 8) |      // PEIE - interrupt on parity error
                (0 << 5) |      // RXNEIE - interrupt on ORE (overrun) or RXNE (rx register not empty)
                (1 << 3) |      // TE - transmitter enable
                (1 << 2);       // RE - receiver enable
    uart->CR2 = (1 << 14) |     // LINEN - LIN mode enable
                (bits << 12);   // STOP - bits
    // TODO - support interrupts/DMA
    uart->SR = 0;
    (void)uart->SR;  // SR reset step 1.
    (void)uart->DR;  // SR reset step 2.
}

void Usart::deinit()
{
    if (uart == &USART1) {
        RCC_SIFTEO.APB2ENR &= ~(1 << 14);
    }
    else if (uart == &USART2) {
        RCC_SIFTEO.APB1ENR &= ~(1 << 17);
    }
    else if (uart == &USART3) {
        RCC_SIFTEO.APB1ENR &= ~(1 << 18);
    }
    else if (uart == &UART4) {
        RCC_SIFTEO.APB1ENR &= ~(1 << 19);
    }
    else if (uart == &UART5) {
        RCC_SIFTEO.APB1ENR &= ~(1 << 20);
    }
}

/*
 * Just synchronous for now, to serve as a stupid data dump stream.
 */
void Usart::write(const char *buf, int size)
{
    while (size--) {
        while (!(uart->SR & (1 << 7))); // wait for empty data register
        uart->DR = *buf++;
    }
}

void Usart::write(const char *buf)
{
    while (*buf) {
        while (!(uart->SR & (1 << 7))); // wait for empty data register
        uart->DR = *buf++;
    }
}

/*
 * Just synchronous for now, to serve as a stupid data dump stream.
 */
void Usart::read(char *buf, int size)
{
    while (size--) {
        while (!(uart->SR & (1 << 5))); // wait for data register to be not-not empty
        *buf++ = uart->DR;
    }
}

void Usart::put(char c)
{
    while (!(uart->SR & (1 << 7))); // wait for empty data register
    uart->DR = c;
}

char Usart::get()
{
    while (!(uart->SR & (1 << 5))); // wait for data register to be not-not empty
    return uart->DR;
}
