/*
 * main.c
 *
 *  Created on: 02.11.2019
 *      Author: Robert
 */
#include <stdio.h>

#include "..\globals.h"
#include "main.h"

// include Modules used here
#include "cli\cli.h"
#include "thr\thruster.h"
#include "tim\timer.h"

// Call all Module Inits
void MainInit() {
	printf("Hello %s HardwareTest. Bootmode: %s [%d]\n", BOARD_SHORT, ClimbGetBootmodeStr(), ClimbGetBootmode());
	TimInit();
	CliInit();
	ThrInit();
}

// Poll all Modules from Main loop
void MainMain() {
	// Call module mains with 'fast - requirement'
	CliMain();
	ThrMain();
	bool tick = TimMain();

	if (tick) {
		ClimbLedToggle(0);
		// Call module mains with 'tick - requirement'

	}

}

