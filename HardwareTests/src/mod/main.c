/*
 * main.c
 *
 *  Created on: 02.11.2019
 *      Author: Robert
 */
#include <stdio.h>

#include "../globals.h"
#include "main.h"

// include Modules used here
#include "cli/cli.h"
#include "thr/thruster.h"
#include "tim/timer.h"
#include "tim/obc_rtc.h"
#include "mem/eeprom.h"
#include "mem/flash.h"
#include "mem/mram.h"

#ifdef RADIATION_TEST
#include "rad/radiation_test.h"
#endif



// Call all Module Inits
void MainInit() {
	printf("Hello %s HardwareTest. Bootmode: %s [%d]\n", BOARD_SHORT, ClimbGetBootmodeStr(), ClimbGetBootmode());
	TimInit();
	RtcInit();
	ThrInit();
	EepromInit();
	FlashInit();
	MramInit();
	CliInit();
#ifdef RADIATION_TEST
	RadTstInit();
#endif
}

// Poll all Modules from Main loop
void MainMain() {
	// Call module mains with 'fast - requirement'
	CliMain();
	ThrMain();
	FlashMain();
	MramMain();
	bool tick = TimMain();
	if (tick) {
		ClimbLedToggle(0);
		// Call module mains with 'tick - requirement'
		EepromMain();
		RtcMain();			// At this moment we only track day changes here so Tick time is enough.
#ifdef RADIATION_TEST
		RadTstInit();
#endif
	}

//  Test timer delay function....
//	static uint32_t counter = 0;
//	if ((counter++ % 100000) == 0) {
//		ClimbLedToggle(0);
//		TimBlockMs(10);
//		ClimbLedToggle(0);
//	}


}

