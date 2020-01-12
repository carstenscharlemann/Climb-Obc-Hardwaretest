/*
 * dosimeter.c
 *
 *  Created on: 11.01.2020
 */

#include <Chip.h>
#include <stdio.h>
#include <string.h>

#include "..\..\layer1\SPI\spi.h"
#include "..\cli\cli.h"

#define FLOGA_VCC_ENABLE_PORT	0
#define FLOGA_VCC_ENABLE_PIN   26

#define FLOGA_FAULT_PORT		1
#define FLOGA_FAULT_PIN		    9

#define FLOGA_IRQ_PORT			1
#define FLOGA_IRQ_PIN		   10

#define FLOGA_CS_PORT			1
#define FLOGA_CS_PIN			4


bool dosimeter_init();
void chipSelectFlog(bool select);
void InitDosimeterCmd(int argc, char *argv[]);
void Switch18VCmd(int argc, char *argv[]);

void FgdInit() {
	spi_init();

	//	/* --- Chip selects IOs --- */
	Chip_IOCON_PinMuxSet(LPC_IOCON, FLOGA_CS_PORT, FLOGA_CS_PIN, IOCON_FUNC0 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, FLOGA_CS_PORT, FLOGA_CS_PIN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, FLOGA_CS_PORT, FLOGA_CS_PIN);
	Chip_GPIO_SetPinState(LPC_GPIO, FLOGA_CS_PORT, FLOGA_CS_PIN, true);						// init as 'not selected'


	/* FLOGA_EN is output and switches on the 18V Charge Pump (low active) */
	Chip_IOCON_PinMuxSet(LPC_IOCON, FLOGA_VCC_ENABLE_PORT, FLOGA_VCC_ENABLE_PIN, IOCON_FUNC0 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, FLOGA_VCC_ENABLE_PORT, FLOGA_VCC_ENABLE_PIN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, FLOGA_VCC_ENABLE_PORT, FLOGA_VCC_ENABLE_PIN);
	Chip_GPIO_SetPinState(LPC_GPIO, FLOGA_VCC_ENABLE_PORT, FLOGA_VCC_ENABLE_PIN, true);		// Init as off

	/* FLOGA_FAULT, FLOGA_IRQ is input */
	Chip_IOCON_PinMuxSet(LPC_IOCON, FLOGA_FAULT_PORT, FLOGA_FAULT_PIN, IOCON_FUNC0 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, FLOGA_FAULT_PORT, FLOGA_FAULT_PIN);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, FLOGA_FAULT_PORT, FLOGA_FAULT_PIN);

	Chip_IOCON_PinMuxSet(LPC_IOCON, FLOGA_IRQ_PORT, FLOGA_IRQ_PIN, IOCON_FUNC0 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, FLOGA_IRQ_PORT, FLOGA_IRQ_PIN);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, FLOGA_IRQ_PORT, FLOGA_IRQ_PIN);


	RegisterCommand("flogVcc", Switch18VCmd);
	RegisterCommand("dInit", InitDosimeterCmd);
	//dosimeter_init();

}

void FgdMain() {}

void chipSelectFlog(bool select) {
	Chip_GPIO_SetPinState(LPC_GPIO, FLOGA_CS_PORT, FLOGA_CS_PIN, !select);
}

void InitDosimeterCmd(int argc, char *argv[]){
	if (dosimeter_init()) {
		printf("Dosimeter init: true\n");
	} else {
		printf("Dosimeter init: false\n");
	}
}

void Switch18VCmd(int argc, char *argv[]){
	if (argc == 1) {
		if (strcmp(argv[0],"ON") == 0  || strcmp(argv[0],"on") == 0 ) {
			Chip_GPIO_SetPinState(LPC_GPIO, FLOGA_VCC_ENABLE_PORT, FLOGA_VCC_ENABLE_PIN, false);
		} else {
			Chip_GPIO_SetPinState(LPC_GPIO, FLOGA_VCC_ENABLE_PORT, FLOGA_VCC_ENABLE_PIN, true);
		}
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

	if (spi_add_job(chipSelectFlog, tx[0],1, rx))
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

