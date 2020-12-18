/*
 * led_controll.c
 *
 *  Created on: Nov 5, 2020
 *      Author: jevgeni
 */
#include "led_controll.h"

#include <chip.h>
#include <stdio.h>

#include "../../mod/cli/cli.h"

static uint8_t ledRedVal = 1;
static uint8_t ledBlueVal = 2;
static uint8_t ledGreenVal = 3;

void MyOwnFunction(int argc, char *argv[]){

	if (argc != 3) {
		printf("usage: ledColor <red> <blue> <green>\n" );
		printf("\nHi, will switch LEDs to have R:%d G:%d B:%d\n", ledRedVal, ledBlueVal, ledGreenVal);
		return;
	}


	char *myptr = argv[0];

	// CLI params to binary params
	ledRedVal = atoi(argv[0]);
	ledBlueVal  = atoi(argv[1]);
	ledGreenVal = atoi(argv[2]);

	printf("\nHi, will switch LEDs to have R:%d G:%d B:%d\n", ledRedVal, ledBlueVal, ledGreenVal);

	//Chip_GPIO_SetPinOutLow(LPC_GPIO, 0, 22); //turn blue on
	Chip_GPIO_SetPinToggle(LPC_GPIO, 0, 22);
}

void LedInit(){

	//configure IO bins to output
	Chip_GPIO_WriteDirBit(LPC_GPIO, 0, 22, true);
	Chip_GPIO_WriteDirBit(LPC_GPIO, 3, 25, true);
	Chip_GPIO_WriteDirBit(LPC_GPIO, 3, 26, true);

	// set all leds to off
	Chip_GPIO_SetPinOutHigh(LPC_GPIO, 3, 26);
	Chip_GPIO_SetPinOutHigh(LPC_GPIO, 0, 22);
	Chip_GPIO_SetPinOutHigh(LPC_GPIO, 3, 25);

	RegisterCommand("led", MyOwnFunction); //you register commands here

}

static int ledCounter = 0;

void LedMain(){
	ledCounter++;

	//printf("debug %d %d %d",ledRedVal,ledBlueVal,ledGreenVal);
	if (ledCounter>255) {
		ledCounter = 0;
	}


	if (ledRedVal > ledCounter) {
		Chip_GPIO_SetPinOutHigh(LPC_GPIO, 3, 26);
	} else {
		Chip_GPIO_SetPinOutLow(LPC_GPIO, 3, 26);
	}


	if (ledGreenVal > ledCounter) {
		Chip_GPIO_SetPinOutHigh(LPC_GPIO, 0, 22);
	} else {
		Chip_GPIO_SetPinOutLow(LPC_GPIO, 0, 22);
	}


	if (ledBlueVal > ledCounter) {
		Chip_GPIO_SetPinOutHigh(LPC_GPIO, 3, 25);
	} else {
		Chip_GPIO_SetPinOutLow(LPC_GPIO, 3, 25);
	}









//	if (ledCounter % 1 == 0){
//		Chip_GPIO_SetPinToggle(LPC_GPIO, 3, 25);
//	}
}
