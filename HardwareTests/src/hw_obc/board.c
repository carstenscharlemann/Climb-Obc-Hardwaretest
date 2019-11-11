/*
 * board_impl.c
 *
 *  Created on: 02.11.2019
 *      Author: Robert
 */

#include "obc_board.h"
#include "..\mod\cli\cli.h"

#define LED_GREEN_WD_GPIO_PORT_NUM               1
#define LED_GREEN_WD_GPIO_BIT_NUM               18
#define LED_BLUE_RGB_GPIO_PORT_NUM               2
#define LED_BLUE_RGB_GPIO_BIT_NUM                6

#define BOOT_SELECT_GPIO_PORT_NUM                0
#define BOOT_SELECT_GPIO_BIT_NUM                29
#define DEBUG_SELECT_GPIO_PORT_NUM               0
#define DEBUG_SELECT_GPIO_BIT_NUM                5


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
	{0,   5, IOCON_MODE_INACT | IOCON_FUNC0},	/* Debug_SEL2 */
};

// This routine is called prior to main(). We setup all pin Functions and Clock settings here.
void ObcClimbBoardSystemInit() {
	Chip_IOCON_SetPinMuxing(LPC_IOCON, pinmuxing, sizeof(pinmuxing) / sizeof(PINMUX_GRP_T));
	Chip_SetupXtalClocking();

	/* Setup FLASH access to 4 clocks (100MHz clock) */
	Chip_SYSCTL_SetFLASHAccess(FLASHTIM_100MHZ_CPU);
}


// This routine is called from main() at startup (prior entering main loop).
void ObcClimbBoardInit() {
	/* Sets up DEBUG UART */
	//DEBUGINIT();

	/* Initializes GPIO */
	Chip_GPIO_Init(LPC_GPIO);
	Chip_IOCON_Init(LPC_IOCON);

	/* The LED Pins are configured as GPIO pin (FUNC0) during SystemInit */
	// Here we define them as OUTPUT
	Chip_GPIO_WriteDirBit(LPC_GPIO, LED_GREEN_WD_GPIO_PORT_NUM, LED_GREEN_WD_GPIO_BIT_NUM, true);
	Chip_GPIO_WriteDirBit(LPC_GPIO, LED_BLUE_RGB_GPIO_PORT_NUM, LED_BLUE_RGB_GPIO_BIT_NUM, true);

	// Die Boot bits sind inputs
	Chip_GPIO_WriteDirBit(LPC_GPIO, DEBUG_SELECT_GPIO_PORT_NUM, DEBUG_SELECT_GPIO_BIT_NUM, false);
	Chip_GPIO_WriteDirBit(LPC_GPIO, BOOT_SELECT_GPIO_PORT_NUM, BOOT_SELECT_GPIO_BIT_NUM, false);

	// UART for comand line interface init
	CliInitUart(LPC_UART2, UART2_IRQn);		// We use SP - B (same side as JTAG connector) as Debug UART.);
//	Chip_UART_Init(CLI_UART);
//	Chip_UART_SetBaud(CLI_UART, 115200);
//	Chip_UART_ConfigData(CLI_UART, UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS);
//
//	/* preemption = 1, sub-priority = 1 */
//	NVIC_SetPriority(UART2_IRQn, 1);
//	NVIC_EnableIRQ(UART2_IRQn);
//
//	/* Enable UART Transmit */
//	Chip_UART_TXEnable(CLI_UART);
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


