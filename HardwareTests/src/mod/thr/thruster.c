/*
 * thruster.c
 *
 *  Created on: 11.11.2019
 *      Author:
 */

#include <stdio.h>			// use if needed (e.g. for printf, ...)
// #include <string.h>			// use if needed (e.g. for strcpy, str... )

// #include "..\..\globals.h"   // use if needed
#include "thruster.h"
#include "..\..\layer1\UART\uart.h"
// ... include dependencies on other modules if needed ....

// module defines
#define THR_HELLO_STR	"Hi there. "

// module prototypes
void exampleFeedback(int par1);

// module variables
int myStateExample;

// module function implementations

// here goes all module init code  to be done before mainloop starts.
void ThrInit() {
	// initialize the thruster UART ....
	// maybe register commands with the cli !?
	InitUart(LPC_UART0, UART0_IRQn, 9600);
	Chip_UART_SendBlocking(LPC_UART0, "Hello Thrust", 12);
	// example code
	myStateExample = 0;
}

// thats the 'mainloop' call of the module
void ThrMain() {
	// do your stuff here. But remember not to make 'wait' loops or other time consuming operations!!!
	// Its called 'Cooperative multitasking' so be kind to your sibling-modules and return as fast as possible!

	// Example code
	myStateExample++;

	if (myStateExample % 100000 == 0) {
		// Note printf should not take too much time (once CLI module is really ready), but keep the texts small!
//		printf(THR_HELLO_STR);
//		printf("ThrMain() called %d times.\n", myStateExample);
	}

}
