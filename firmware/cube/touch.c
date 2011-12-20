/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * Hakim Raja <huckym@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "main.h"
#include "touch.h"
#include "hardware.h"
#include "battery.h"
#include "time.h"

#define adc_vdd13 2275 //( ((1/3)*2*(1<<12)) / 1.2 )

#define ts_threshold 500
my_idata volatile union ts_word ts_touchval={0}, ts_prev_touchval={0};
volatile union ts_word ts_adc_ref, ts_adc_sense;

volatile uint8_t ts_samples_per_scan = 1;
volatile uint8_t ts_samples = 1;
volatile __bit ts_flag_senseDone=0;
volatile __bit ts_dataReady=0;

/*
#define touch_decode() {							\
	static uint8_t samples=ts_samples_per_scan;		\
													\
	ts_touchval.val += ts_adc_sense.val;			\
	if (--samples == 0) {							\
		samples = ts_samples_per_scan;				\
		ts_dataReady = 1;							\
	}												\
}
*/

void touch_init() {
#ifdef TOUCH_STANDALONE
	//Initialize UART
	//while(CLKLFCTRL&0x08 == 0x0);
	//DEBUG_UART_INIT();
	//P1DIR &= ~BIT_6;
	//P1 &= ~BIT_6;

	//Initialize ADC
	//ADCCON2
	//diffm(7:6)	- 00(single) 01(diff AIN2) 10(diff AIN6) 11(not used)
	//cont(5)	 	- 0(single-step) 2(continuous)
	//rate(4:2)	 	- 000(2ksps) 001(4ksps) 010(8ksps) 011(16ksps) (cont)
	//pdd(4:2)		- 000(0us) 001(6us) 010(24us) 011(infinite)	   (single-step)
	//tacq(1:0)		- 00(0.75us) 01(3us) 10(12us) 11(36us)
	ADCCON2 = 0x08;	//00 0 010 00

	//ADCCON3
	//resol(7:6)	- 00(6b) 01(8b) 10(10b) 11(12b)
	//rljust(5)		- 0(left) 1(right)
	//uflow(4)		- read-only
	//oflow(3)		- read-only
	//range(2)		- read-only
	//unused(1:0)	- not-used
	ADCCON3 = 0xe0;	//11 1 0 0 0 00

	//Enable timer interrupt
	IEN_EN = 1;
	TCON_TR0 = 1;               // Timer 0 running
	IEN_TF0 = 1;                // Enable Timer 0 interrupt
#endif
}

void adc_isr(void) __interrupt(VECTOR_MISC) {

	__asm
adc_jitter:
	;mov 	a, #0xf
	;anl		a, _ADCDATL
	;djnz	ACC, .

	jb		_ts_flag_senseDone, adc_sensorDone
adc_sensorTrigger:
	; ADC is now powered-up initiate sensor measurement
	; anl 	CTRL_PORT, #~BIT_3		; debug
	setb	_ts_flag_senseDone

	; read reference value
	mov 	(_ts_adc_ref+1), _ADCDATH
	mov 	(_ts_adc_ref), _ADCDATL
	; recharge Chold
	mov 	_ADCCON1, #((1<<7) | (VDD23_ADC_CH << 2) | REF_INT_ADC)

	; wait for Chold to charge (~12 cycles)
	mov  	a, #0x1            		; 2
	djnz 	ACC, .                	; 4 (1 iters, 4 cycles each)
	nop								; 1
	nop								; 1

	; configure sensor channel as input
	orl		_MISC_DIR, #MISC_TOUCH	; 4 cycles

	; switch to sensor channel
	mov		_ADCCON1, #((1<<7) | (TOUCH_ADC_CH << 2) | REF_INT_ADC)
	; orl 	CTRL_PORT, #BIT_3		; debug

	sjmp adc_irqDone

adc_sensorDone:
	; Read sensor measurement
	; anl 	CTRL_PORT, #~BIT_3		;debug
	clr 	_ts_flag_senseDone

	; keep sensor channel grounded
	anl		MISC_PORT, #~MISC_TOUCH		; 4 cycles
	anl 	_MISC_DIR, #~MISC_TOUCH		; 4 cycles

	; read sensor value
	;mov 	(_ts_adc_sense+1), _ADCDATH
	;mov 	(_ts_adc_sense), _ADCDATL

	; accumulate sensor value
	mov		r0, #_ts_adc_sense
	mov		ar2, @r0
	inc		r0
	mov		ar3, @r0
	mov		a, _ADCDATL
	add		a, r2
	mov		r2, a
	mov		a, _ADCDATH
	addc	a, r3
	mov		r3,	a
	mov		r0, #_ts_adc_sense		; optimize this away
	mov		@r0, ar2
	inc		r0
	mov		@r0, ar3

adc_process:
	; check if done samples per scan
	; scale adc_sense and move to touchval
	; reset adc_sense and samples
	djnz	_ts_samples, adc_postProcess
	;mov 	_ts_samples, _ts_samples_per_scan
	;mov 	_ts_touchval, _ts_adc_sense
	;mov		(_ts_touchval+1), (_ts_adc_sense+1)
	;mov 	a, _ts_samples_per_scan
	mov 	_ts_samples, #0x3
	mov 	r0, #_ts_adc_sense
	mov		r1, #_ts_touchval
	mov		a, @r0
	mov 	@r1, a
	inc 	r0
	inc 	r1
	mov		a, @r0
	mov 	@r1, a
	mov		_ts_adc_sense, #0
	mov		(_ts_adc_sense+1), #0
	setb	_ts_dataReady

adc_postProcess:
	clr 	_IEN_MISC
	;setb 	_IEN_TF0
	;mov		_ADCCON1, #((1<<7) | (VDD13_ADC_CH << 2) | REF_INT_ADC)
	;clr		_global_busy_flag

adc_irqDone:

	__endasm;

#ifndef TOUCH_STANDALONE
	if (ts_dataReady) {

		ts_touchval.val = (7*ts_prev_touchval.val + ts_touchval.val) >> 3;
		//touchval = (3*prev_touchval + touchval) >> 2;
		//touchval = (prev_touchval + touchval) >> 1;
		ts_prev_touchval.val = ts_touchval.val;

		if (ts_touchval.val < ts_threshold) {
			CTRL_PORT |= BIT_3;		//debug
		} else {
			CTRL_PORT &= ~BIT_3;	//debug
		}

		ts_dataReady = 0;
	}
#endif
}

#ifdef TOUCH_STANDALONE
void timer_isr(void) __interrupt(VECTOR_TF0) {
	if (battery_adc_lock) {
		return;
	}
	//touch_decode()

	//Measure reference voltage
	//global_busy_flag = 1;
	ADCCON1 = ((1<<7) | (VDD13_ADC_CH << 2) | REF_INT_ADC);
	//P1 |= BIT_6;	//debug

	//enable ADCIRQ disable timerIRQ
	IEN_MISC = 1;
	//IEN_TF0 = 0;
}
#endif
