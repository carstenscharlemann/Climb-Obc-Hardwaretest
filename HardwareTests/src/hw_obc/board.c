/*
 * board_impl.c
 *
 * This is the Board Abstraction Layer for the OBC EM1 Hardware.
 *
 *  Created on: 02.11.2019
 *      Author: Robert
 */

#include <string.h>

#include "obc_board.h"
#include "..\mod\cli\cli.h"
#include "..\layer1\I2C\obc_i2c.h"

#define LED_GREEN_WD_GPIO_PORT_NUM               1
#define LED_GREEN_WD_GPIO_BIT_NUM               18
#define LED_BLUE_RGB_GPIO_PORT_NUM               2
#define LED_BLUE_RGB_GPIO_BIT_NUM                6

#define BOOT_SELECT_GPIO_PORT_NUM                0
#define BOOT_SELECT_GPIO_BIT_NUM                29

#define DEBUG_SELECT_GPIO_PORT_NUM               0
#define DEBUG_SELECT_GPIO_BIT_NUM                5


void SwitchVccFfgDCmd(int argc, char *argv[]);
/* Pin muxing configuration */
STATIC const PINMUX_GRP_T pinmuxing[] = {
	// UARTS
	{0,  2,   IOCON_MODE_INACT | IOCON_FUNC1},	/* TXD0 - UART SP-D	Y+ */
	{0,  3,   IOCON_MODE_INACT | IOCON_FUNC1},	/* RXD0 - UART SP-D Y+ */
	{2,  0,   IOCON_MODE_INACT | IOCON_FUNC2},	/* TXD1 - UART SP-C X- */
	{2,  1,   IOCON_MODE_INACT | IOCON_FUNC2},	/* RXD1 - UART SP-C X- */
	{2,  8,   IOCON_MODE_INACT | IOCON_FUNC2},	/* TXD2 - UART SP-B Y- */
	{2,  9,   IOCON_MODE_INACT | IOCON_FUNC2},	/* RXD2 - UART SP-B Y- */
	{0,  0,   IOCON_MODE_INACT | IOCON_FUNC2},	/* TXD3 - UART SP-A X+ */
	{0,  1,   IOCON_MODE_INACT | IOCON_FUNC2},	/* RXD3 - UART SP-A X+ */

	// LEDS
	{2,  6,  IOCON_MODE_INACT | IOCON_FUNC0},	/* Led 4 Blue - also RGB LED Pin */
	{1, 18,  IOCON_MODE_INACT | IOCON_FUNC0},	/* Led 1 green - Watchdog Feed   */

	// GPIOs
	{0,  29, IOCON_MODE_INACT | IOCON_FUNC0},	/* BL_SEL1    */
	{0,  30, IOCON_MODE_INACT | IOCON_FUNC0},	/* ...    */

	{0,   5, IOCON_MODE_INACT | IOCON_FUNC0},	/* Debug_SEL2 */

	{1, 27, IOCON_MODE_INACT | IOCON_FUNC1},	/* CLOCKOUT */

	{0, 27, IOCON_MODE_INACT | IOCON_FUNC1},	/* I2C0 SDA */
	{0, 28, IOCON_MODE_INACT | IOCON_FUNC1},	/* I2C0 SCL */

	{0, 19, IOCON_MODE_INACT | IOCON_FUNC3},	/* I2C1 SDA */
	{0, 20, IOCON_MODE_INACT | IOCON_FUNC3},	/* I2C1 SCL */

	{0, 10, IOCON_MODE_INACT | IOCON_FUNC2},	/* I2C2 SDA */
	{0, 11, IOCON_MODE_INACT | IOCON_FUNC2},	/* I2C2 SCL */

	{0,  26,  IOCON_MODE_INACT | IOCON_FUNC0},	/* FG-VCC Enable */

};


//
// Public API Implementation
//

// This routine is called prior to main(). We setup all pin Functions and Clock settings here.
void ObcClimbBoardSystemInit() {
	Chip_IOCON_SetPinMuxing(LPC_IOCON, pinmuxing, sizeof(pinmuxing) / sizeof(PINMUX_GRP_T));
	Chip_SetupXtalClocking();		// Asumes 12Mhz Quarz -> PLL0 frq=384Mhz -> CPU frq=96MHz

	/* Setup FLASH access to 4 clocks (100MHz clock) */
	Chip_SYSCTL_SetFLASHAccess(FLASHTIM_100MHZ_CPU);
}

// This routine is called from main() at startup (prior entering main loop).
void ObcClimbBoardInit() {
	/* Initializes GPIO */
	Chip_GPIO_Init(LPC_GPIO);

	/* Initialize IO Dirs */
	// TODO: make a nice loop here (like/or combine as the function selects on all IOS...)
	/* The LED Pins are configured as GPIO pin (FUNC0) during SystemInit */
	// Here we define them as OUTPUT
	Chip_GPIO_WriteDirBit(LPC_GPIO, LED_GREEN_WD_GPIO_PORT_NUM, LED_GREEN_WD_GPIO_BIT_NUM, true);
	Chip_GPIO_WriteDirBit(LPC_GPIO, LED_BLUE_RGB_GPIO_PORT_NUM, LED_BLUE_RGB_GPIO_BIT_NUM, true);
	Chip_GPIO_WriteDirBit(LPC_GPIO, 0, 26, true);


	// Die Boot bits sind inputs
	Chip_GPIO_WriteDirBit(LPC_GPIO, DEBUG_SELECT_GPIO_PORT_NUM, DEBUG_SELECT_GPIO_BIT_NUM, false);
	Chip_GPIO_WriteDirBit(LPC_GPIO, BOOT_SELECT_GPIO_PORT_NUM, BOOT_SELECT_GPIO_BIT_NUM, false);
	Chip_GPIO_WriteDirBit(LPC_GPIO, 0, 30, false);

	// UART for comand line interface init
	CliInitUart(LPC_UART2, UART2_IRQn);		// We use SP - B (same side as JTAG connector) as Debug UART.);

	// Init I2c bus for Onboard devices (3xEEProm, 1xTemp, 1x FRAM)
	InitOnboardI2C(ONBOARD_I2C);

	RegisterCommand("vcc",SwitchVccFfgDCmd);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 26, false);
}

void SwitchVccFfgDCmd(int argc, char *argv[]){
	bool myswitch = false;
	if (argc > 0) {
		myswitch = true;
	}
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 26, myswitch);
}

void UART2_IRQHandler(void) {
	CliUartIRQHandler(LPC_UART2);
}

void ObcLedToggle(uint8_t ledNr) {
	if (ledNr == 0) {
		ObcLedSet(ledNr, !ObcLedTest(ledNr));
	}
}


/* Sets the state of a board LED to on or off */
void ObcLedSet(uint8_t ledNr, bool On)
{
	/* There is only one LED */
	if (ledNr == 0) {
		Chip_GPIO_WritePortBit(LPC_GPIO, LED_BLUE_RGB_GPIO_PORT_NUM, LED_BLUE_RGB_GPIO_BIT_NUM, On);
	}
}


/* Returns the current state of a board LED */
bool ObcLedTest(uint8_t ledNr)
{
	bool state = false;
	if (ledNr == 0) {
		state = Chip_GPIO_ReadPortBit(LPC_GPIO, LED_BLUE_RGB_GPIO_PORT_NUM, LED_BLUE_RGB_GPIO_BIT_NUM);
	}
	return state;
}


bootmode_t ObcGetBootmode(){
	bool boot = Chip_GPIO_ReadPortBit(LPC_GPIO, BOOT_SELECT_GPIO_PORT_NUM, BOOT_SELECT_GPIO_BIT_NUM);
	if (Chip_GPIO_ReadPortBit(LPC_GPIO, DEBUG_SELECT_GPIO_PORT_NUM, DEBUG_SELECT_GPIO_BIT_NUM)) {
		 if (boot) {
			return DebugEven;
		 } else {
			 return DebugOdd;
		 }
	} else {
		if (boot) {
			return Even;
		 } else {
			 return Odd;
		 }
	}
}

char* ObcGetBootmodeStr() {
	switch (ObcGetBootmode()) {
		case Odd:
			return "Odd";
		case Even:
			return "Even";
		case DebugOdd:
			return "DebugOdd";
		case DebugEven:
			return "DebugEven";
	}
	return "Unknown";
}

uint8_t ObcGetI2CAddrForMemoryDeviceName(char* name) {
    if (strcmp(name,C_MEM_EEPROM1_NAME) == 0) {
    	return I2C_ADR_EEPROM1;
    } else if (strcmp(name,C_MEM_EEPROM2_NAME) == 0) {
    	return I2C_ADR_EEPROM2;
    } else if (strcmp(name,C_MEM_EEPROM3_NAME) == 0) {
    	return I2C_ADR_EEPROM3;
    } else if (strcmp(name,C_MEM_FRAM_NAME) == 0) {
    	return I2C_ADR_FRAM;
    }

    return 0;
}

