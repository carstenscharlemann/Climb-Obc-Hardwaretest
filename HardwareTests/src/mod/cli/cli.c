/*
 * cli.c
 *
 *  Created on: 02.11.2019
 *      Author: Robert
 */
#include "..\..\globals.h"

#define BUFFER_SIZE 64


void processLine();

int ptrIdx = 0;
char buffer[BUFFER_SIZE];

void CliInit() {

}


void CliMain(){
	int anz;
	// The UART has 16 byte Input buffer
	// read all available bytes in this main loop call.
	while ((anz = ClimbCliUARTGetChar()) != -1) {

	// one byte per main loop
	//if  ((anz = ClimbCliUARTGetChar()) != -1) {
		// make echo
		//ClimbCliUARTPutChar(anz);
		//ClimbCliUARTPutChar(anz + 1);
		if (anz != 0x0a &&
			anz != 0x0d) {
			buffer[ptrIdx++] = (char)(anz);
		}

		if ((ptrIdx >= BUFFER_SIZE) ||
			anz == 0x0a ||
			anz == 0x0d) 	{
			buffer[ptrIdx++] = 0x00;
			processLine();
			ptrIdx = 0;
		}
	}
}

void processLine() {
	printf("\nReceived: %s", &buffer);
}
