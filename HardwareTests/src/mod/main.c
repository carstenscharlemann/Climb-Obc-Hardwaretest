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
#include "mem/eeprom.h"
#include "mem/flash.h"
#include "mem/mram.h"


// Call all Module Inits
void MainInit() {
	printf("Hello %s HardwareTest. Bootmode: %s [%d]\n", BOARD_SHORT, ClimbGetBootmodeStr(), ClimbGetBootmode());
	TimInit();
	CliInit();
	ThrInit();
	EepromInit();
	FlashInit();
	MramInit();
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
	}

//  Test timer delay function....
//	static uint32_t counter = 0;
//	if ((counter++ % 100000) == 0) {
//		ClimbLedToggle(0);
//		TimBlockMs(10);
//		ClimbLedToggle(0);
//	}


}

