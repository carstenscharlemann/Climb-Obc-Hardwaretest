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


static int i = 0;

// Call all Module Inits
void MainInit() {
	printf("Hello Robert HardwareTest Bootmode: %s [%d]\n", ClimbGetBootmodeStr(), ClimbGetBootmode());
	CliInit();
}

// Poll all Modules from Main loop
void MainMain() {
	CliMain();
	i++ ;
	if (i % 100000 == 0) {
		ClimbLedToggle(0);
		//printf(".");
	}
}

