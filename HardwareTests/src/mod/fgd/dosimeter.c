/*
 * dosimeter.c
 *
 *  Created on: 11.01.2020
 */

#include <Chip.h>
#include <stdio.h>

#include "..\..\layer1\SPI\spi.h"

bool dosimeter_init();
void InitDosimeterCmd(int argc, char *argv[]);

void FgdInit() {
	spi_init();

	RegisterCommand("dInit", InitDosimeterCmd);
	//dosimeter_init();

}

void FgdMain() {}

void InitDosimeterCmd(int argc, char *argv[]){
	if (dosimeter_init()) {
		printf("Dosimeter init: true\n");
	} else {
		printf("Dosimeter init: false\n");
	}
}

bool dosimeter_init()
{
	/* Init flash n and read ID register
	 * Parameters: none
	 * Return value: 0 in case of success, != 0 in case of error
	 */
	uint8_t tx[1];
	uint8_t rx[1];

	volatile uint32_t helper;
	/* Read flash ID register */
	tx[0] = 0x13 | 0x80 ; /* Read  */
	rx[0] = 0x00;

	if (spi_add_job(1, tx[0],1, rx))
	{
		/* Error while adding job */
		return false;
	}
	helper = 0;
	while (spi_getJobsPending() > 0)
	{
		/* Wait for job to finish */
		helper++;
	}

	if (rx[0] != 0x01)
	{
		/* Error - Dosimeter could not be accessed */
		return false;
	}
	return true;
}

