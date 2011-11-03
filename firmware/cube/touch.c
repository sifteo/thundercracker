/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * Hakim Raja <huckym@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include <stdio.h>
#include "touch.h"
#include "hardware.h"
#include "time.h"

void putchar(char c)
{
  DEBUG_UART(c);
}

#define adc_threshold 0

void adc_isr(void) __interrupt(VECTOR_MISC) {
	static int16_t f1=0;
	static int16_t f2=0;
	static int16_t x[2]={0,0}, y[2]={0,0};
	static uint8_t firstPass=1;
	static adc_ref;
	uint16_t adc_sense=0;
	int16_t r, adc_op;

	if(firstPass) {
		firstPass = 0;
		adc_ref = (ADCDATH<<8) | (ADCDATL);

		//charge Chold to reference again
		//wait for Tacq (0.75us = 12cycles)
		//ADCCON1
		//pwrup(7)		-0(power-down) 1(power-up)
		//busy(6)		-read-only
		//chsel(5:2)	-0000-1101(AIN0-AIN13) 1110(1/3*VDD) 1111(2/3*VDD)
		//refsel(1:0)	-00(INT) 01(VDD) 10(AIN3) 11(AIN9)

		ADCCON1 = 0xbc; //1 0 1111 00

		P1DIR &= ~BIT_4;	//4cycles
		P1 &= ~BIT_4;			//3cycles
		P1DIR |= BIT_4;		//4cycles
		__asm
		nop					//1cycle
		__endasm;

		//Switch to sensor channel and wait for interrupt again
		//ADCCON1 = 0x84;	//1 0 0001 00 channel-1
		//ADCCON1 = 0x8c;	//1 0 0011 00 channel-3
		ADCCON1 = 0xb0; //1 0 1100 00 channel-12

		return;
	}

	//read adc value
	adc_sense = (ADCDATH<<8) | (ADCDATL);

	//powerdown ADC
	ADCCON1 = 0x38;	//0 0 1110 00

	//No filter
	adc_op = adc_ref-adc_sense;

/*
	//HPF
	x[1] = x[0];
	x[0] = adc_sense;
	y[1] = y[0];
	y[0] = (3*y[0])>>2;
	if (x[0] >= x[1]) {
		y[0] += (3*(x[0]-x[1])>>1)>>1;
	} else {
		y[0] += (3*(x[1]-x[0])>>1)>>1;
	}
*/


	//LPF
	x[0] = adc_op;
	y[1] = y[0];
	y[0] = (y[1]+x[0])>>1;
	//y[0] = (x[0]+y[1]+y[1]<<1)>>2;
	//y[0] = (x[0]>>3)+(7*y[1])>>3;
	//y[0] = (x[0]/6) + ((5*y[0])/6);
	adc_op = y[0];

/*
	//Beth's 2 LPF
	f1 += adc_sense - f1>>7;
	f2 += adc_sense - f2>>4;
	r = f1>>3 - f2;
	if(r<0) r=0;
	adc_op = r;
*/

	//printf_tiny("%d\n\r", adc_op);

	//Reset flag
	firstPass = 1;

	//enable timerIRQ disable adcIRQ
	IEN_TF0 = 1;
	IEN_MISC = 0;
}

void timer_isr(void) __interrupt(VECTOR_TF0) {
	//Measure reference voltage
	ADCCON1 = 0xbd; //1 0 1110 00

	//enable ADCIRQ disable timerIRQ
	IEN_MISC = 1;
	IEN_TF0 = 0;
}

void adc_init() {
	//ADCCON2
	//diffm(7:6)	- 00(single) 01(diff AIN2) 10(diff AIN6) 11(not used)
	//cont(5)	 	- 0(single-step) 2(continuous)
	//rate(4:2)	 	- 000(2ksps) 001(4ksps) 010(8ksps) 011(16ksps) (cont)
	//pdd(4:2)		- 000(0us) 001(6us) 010(24us) 011(infinite)	   (single-step)
	//tacq(1:0)		- 00(0.75us) 01(3us) 10(12us) 11(36us)
	ADCCON2 = 0x0c;	//00 0 011 00

	//ADCCON3
	//resol(7:6)	- 00(6b) 01(8b) 10(10b) 11(12b)
	//rljust(5)		- 0(left) 1(right)
	//uflow(4)		- read-only
	//oflow(3)		- read-only
	//range(2)		- read-only
	//unused(1:0)	- not-used
	ADCCON3 = 0xe0;	//11 1 0 0 0 0
}


void touch_init() {
	//Initialize UART
	//while(CLKLFCTRL&0x08 == 0x0);
	//DEBUG_UART_INIT();

	//Initialize ADC
	adc_init();

	//Enable timer interrupt
	TCON_TR0 = 1;               // Timer 0 running
	IEN_TF0 = 1;                // Enable Timer 0 interrupt
}

