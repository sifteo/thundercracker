ENTRY(_start)

/*
 * Our ram is split into 3 sections:
 * - system ram, used for .data and stack of the firmware
 * - flash block cache, used to cache blocks of game code/data from external flash
 * - user ram, used as .data and stack for user applications
 */
MEMORY
{
    SYSRAM (xrw) : ORIGIN = 0x20000000, LENGTH = 28K
    CACHERAM (xrw) : ORIGIN = 0x20000000 + LENGTH(SYSRAM), LENGTH = 4K
    USERRAM (xrw) : ORIGIN = 0x20000000 + LENGTH(SYSRAM) + LENGTH(CACHERAM), LENGTH = 32K

    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 128K
}

/* Main stack starts at the top of SYSRAM */
_stack = ORIGIN(SYSRAM) + LENGTH(SYSRAM);

SECTIONS
{
    /*
     * First thing in flash must be the vector table.
     * We use GCC attributes to explicitly place it in its own section.
     */

    .vectors : {
        *(.vectors);
    } >FLASH

    /*
     * Program code in Flash
     */

    .text : {
        . = ALIGN(4);
        *(.text .text.*);
    } >FLASH

    /*
     * Read-only data, used in-place from Flash
     */

    .rodata : {
        . = ALIGN(4);
        *(.rodata .rodata.*);

        /*
         * Array of global C++ constructor pointers.
         *
         * Note: This should only have _our_ constructors in it. If you see
         *       junk like _frame_dummy in here, something's wrong with the
         *       linker configuration. We are supposed to be linking with the
         *       standard library (newlib) but NOT with the standard crt0.
         */         

        . = ALIGN(4);
        __init_array_start = .;
        KEEP (*(.init_array));
        __init_array_end = .;

        /*
         * This is our marker for where the initialized data is stored in flash below.
         * It's also used by the startup code to locate this data.
         */

        __data_src = .;

    } >FLASH

    /*
     * Initialized data. Resident in RAM, but stored in Flash.
     * During startup, we copy it to RAM.
     */

    .data : AT (__data_src) {
        . = ALIGN(4);
        __data_start = .;
        *(.data .data.*)
        __data_end = .;
    } >SYSRAM

    /*
     * Zero-initialized data. Nothing is stored in flash.
     */

    .bss : {
        . = ALIGN(4);
        __bss_start = .;
        *(.bss .bss.*);
        __bss_end = .;
    } >SYSRAM

    .blockcache : {
        . = ALIGN(4);
        __blockcache_start = .;
        *(.blockcache);
        __blockcache_end = .;
    } >CACHERAM

    .userram : {
        . = ALIGN(4);
        __userram_start = .;
        *(.userram);
        __userram_end = .;
    } >USERRAM

    /*
     * Junk we don't want.
     */

    /DISCARD/ : {
        *(.note .note.* .comment .comment.*);
        *(.ARM.*);      /* ARM EABI junk, especially exception handling tables */
    }
}

/*
 * Memory mapped I/O addresses.
 * These are extern symbols defined by hardware.h
 */

TIM2 = 0x40000000;
TIM3 = 0x40000400;
TIM4 = 0x40000800;
TIM5 = 0x40000c00;
TIM6 = 0x40001000;
TIM7 = 0x40001400;
RTC = 0x40002800;
WWDG = 0x40002c00;
IWDG = 0x40003000;
SPI2 = 0x40003800;
SPI3 = 0x40003C00;
USART2 = 0x40004400;
USART3 = 0x40004800;
UART4 = 0x40004c00;
UART5 = 0x40005000;
I2C1 = 0x40005400;
I2C2 = 0x40005800;
USB = 0x40005c00;
USBRAM = 0x40006000;
BKP = 0x40006c00;
PWR = 0x40007000;
DAC = 0x40007400;
AFIO = 0x40010000;
EXTI = 0x40010400;
GPIOA = 0x40010800;
GPIOB = 0x40010c00;
GPIOC = 0x40011000;
GPIOD = 0x40011400;
GPIOE = 0x40011800;
GPIOF = 0x40011c00;
GPIOG = 0x40012000;
ADC1 = 0x40012800;
ADC2 = 0x40012800;
TIM1 = 0x40012c00;
SPI1 = 0x40013000;
TIM8 = 0x40013400;
USART1 = 0x40013800;
ADC3 = 0x40013c00;
SDIO = 0x40018000;
DMA1 = 0x40020000;
DMA2 = 0x40020400;
RCC = 0x40021000;
FLASH = 0x40022000;
CRC = 0x40023000;
USBOTG = 0x50000000;
NVIC = 0xe000e000;
DBGMCU_CR = 0xe0042004;
