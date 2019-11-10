/*
 * board.c
 *
 *  Created on: 02.11.2019
 *      Author: Robert
 */

#include "lpcx_board.h"
#include "..\mod\cli\cli.h"

#define LED0_GPIO_PORT_NUM	0
#define LED0_GPIO_BIT_NUM   22

//#define CLI_UART 			LPC_UART3


STATIC const PINMUX_GRP_T pinmuxing[] = {
	{0,  0,   IOCON_MODE_INACT | IOCON_FUNC2},	/* TXD3 */
	{0,  1,   IOCON_MODE_INACT | IOCON_FUNC2},	/* RXD3 */
	{0,  4,   IOCON_MODE_INACT | IOCON_FUNC2},	/* CAN-RD2 */
	{0,  5,   IOCON_MODE_INACT | IOCON_FUNC2},	/* CAN-TD2 */
	{LED0_GPIO_PORT_NUM, LED0_GPIO_BIT_NUM ,  IOCON_MODE_INACT | IOCON_FUNC0},	/* Led 0 */
	{0,  23,  IOCON_MODE_INACT | IOCON_FUNC1},	/* ADC 0 */
	{0,  26,  IOCON_MODE_INACT | IOCON_FUNC2},	/* DAC */

	/* ENET */
	{0x1, 0,  IOCON_MODE_INACT | IOCON_FUNC1},	/* ENET_TXD0 */
	{0x1, 1,  IOCON_MODE_INACT | IOCON_FUNC1},	/* ENET_TXD1 */
	{0x1, 4,  IOCON_MODE_INACT | IOCON_FUNC1},	/* ENET_TX_EN */
	{0x1, 8,  IOCON_MODE_INACT | IOCON_FUNC1},	/* ENET_CRS */
	{0x1, 9,  IOCON_MODE_INACT | IOCON_FUNC1},	/* ENET_RXD0 */
	{0x1, 10, IOCON_MODE_INACT | IOCON_FUNC1},	/* ENET_RXD1 */
	{0x1, 14, IOCON_MODE_INACT | IOCON_FUNC1},	/* ENET_RX_ER */
	{0x1, 15, IOCON_MODE_INACT | IOCON_FUNC1},	/* ENET_REF_CLK */
	{0x1, 16, IOCON_MODE_INACT | IOCON_FUNC1},	/* ENET_MDC */
	{0x1, 17, IOCON_MODE_INACT | IOCON_FUNC1},	/* ENET_MDIO */
	{0x1, 27, IOCON_MODE_INACT | IOCON_FUNC1},	/* CLKOUT */

	/* Joystick buttons. */
	{2, 3,  IOCON_MODE_INACT | IOCON_FUNC0},	/* JOYSTICK_UP */
	{0, 15, IOCON_MODE_INACT | IOCON_FUNC0},	/* JOYSTICK_DOWN */
	{2, 4,  IOCON_MODE_INACT | IOCON_FUNC0},	/* JOYSTICK_LEFT */
	{0, 16, IOCON_MODE_INACT | IOCON_FUNC0},	/* JOYSTICK_RIGHT */
	{0, 17, IOCON_MODE_INACT | IOCON_FUNC0},	/* JOYSTICK_PRESS */

};

//
// Public API Implementation
//

// This routine is called prior to main(). We setup all pin Functions and Clock settings here.
void LpcxClimbBoardSystemInit() {
	Chip_IOCON_SetPinMuxing(LPC_IOCON, pinmuxing, sizeof(pinmuxing) / sizeof(PINMUX_GRP_T));
	Chip_SetupXtalClocking();

	/* Setup FLASH access to 4 clocks (100MHz clock) */
	Chip_SYSCTL_SetFLASHAccess(FLASHTIM_100MHZ_CPU);
}


// This routine is called from main() at startup (prior entering main loop).
void LpcxClimbBoardInit() {
	/* Sets up DEBUG UART */
	//DEBUGINIT();

	/* Initializes GPIO */
	Chip_GPIO_Init(LPC_GPIO);
	Chip_IOCON_Init(LPC_IOCON);

	/* Initialize IO Dirs */
	/* Pin PIO0_22 is configured as GPIO pin during SystemInit */
	/* Set the PIO_22 as output */
	Chip_GPIO_WriteDirBit(LPC_GPIO, LED0_GPIO_PORT_NUM, LED0_GPIO_BIT_NUM, true);

	// UART for comand line interface init
	CliInitUart(LPC_UART3, UART3_IRQn);		// We use SP - B (same side as JTAG connector) as Debug UART.);
//	// UART Init
//	Chip_UART_Init(CLI_UART);
//	Chip_UART_SetBaud(CLI_UART, 115200);
//	Chip_UART_ConfigData(CLI_UART, UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS);
//
//	/* Enable UART Transmit */
//	Chip_UART_TXEnable(CLI_UART);
}

void UART3_IRQHandler(void) {
	CliUartIRQHandler(LPC_UART3);
}


void LpcxLedToggle(uint8_t ledNr) {
	if (ledNr == 0) {
		LpcxLedSet(ledNr, !LpcxLedTest(ledNr));
	}
}


/* Sets the state of a board LED to on or off */
void LpcxLedSet(uint8_t ledNr, bool On)
{
	/* There is only one LED */
	if (ledNr == 0) {
		Chip_GPIO_WritePortBit(LPC_GPIO, LED0_GPIO_PORT_NUM, LED0_GPIO_BIT_NUM, On);
	}
}


/* Returns the current state of a board LED */
bool LpcxLedTest(uint8_t ledNr)
{
	bool state = false;
	if (ledNr == 0) {
		state = Chip_GPIO_ReadPortBit(LPC_GPIO, LED0_GPIO_PORT_NUM, LED0_GPIO_BIT_NUM);
	}
	return state;
}

/* Sends a character on the UART */
//void LpcxCliUARTPutChar(char ch)
//{
//	while ((Chip_UART_ReadLineStatus(CLI_UART) & UART_LSR_THRE) == 0) {}
//	Chip_UART_SendByte(CLI_UART, (uint8_t) ch);
//}
//
///* Gets a character from the UART, returns EOF if no character is ready */
//int LpcxCliUARTGetChar(void)
//{
//	if (Chip_UART_ReadLineStatus(CLI_UART) & UART_LSR_RDR) {
//		return (int) Chip_UART_ReadByte(CLI_UART);
//	}
//	return -1;
//}