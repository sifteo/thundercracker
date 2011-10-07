/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _CUBE_HARDWARE_H
#define _CUBE_HARDWARE_H

class CubeHardware {
 public:
    em8051 cpu;
    LCD lcd;
    
    void init();
    void exit();

 private:
    void hardwareTick();
    void graphicsTick();
    void sfrWrite(int reg);
    void sfrRead(int reg);

    SPIMaster radioSPI;
    Radio radio;
    ADC adc;
    I2C i2c;
    Flash flash;

    uint8_t lat1;
    uint8_t lat2;
    uint8_t bus;
    uint8_t prev_ctrl_port;
    uint8_t flash_drv;
    uint8_t iex3;
    uint8_t rfcken;
};

#endif
