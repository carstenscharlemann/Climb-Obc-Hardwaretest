/*
 * board_impl.c
 *
 *  Created on: 22.01.2019
 *      Author: Robert
 */

#include <stdio.h>
#include "chip.h"

#include "my_board_api.h"		// The API to be implemented here

//
// Board specific definitions/implementations for the "OEM 13085" aka "LPCXpresso for LPC1769 CD" (revision D1 with 3 color LED)
// *****************************************************************************************************************************

#define DEBUG_UART 						LPC_UART0

#define LED0_PORT			          LPC_GPIO[0]
#define LED0_GPIO_PORT_NUM                      0
#define LED0_GPIO_BIT_NUM                      22
#define LED12_PORT			          LPC_GPIO[3]
#define LED12_GPIO_PORT_NUM                     3
#define LED1_GPIO_BIT_NUM                      25
#define LED2_GPIO_BIT_NUM                      26

#define LED0_MASK	   (1UL << LED0_GPIO_BIT_NUM)
#define LED1_MASK	   (1UL << LED1_GPIO_BIT_NUM)
#define LED2_MASK	   (1UL << LED2_GPIO_BIT_NUM)

// Configure Pins to have FUNC0 - FUNC3. This selects function from list in Pin Diagram ( e.g. P0(2)/TCD0/AD0(7) for Pin 98 )
STATIC const PINMUX_GRP_T pinmuxing[] = {
	// port, bit, additional f() | f()
		{ 0, 0, IOCON_MODE_INACT | IOCON_FUNC1 }, /* TXD3 */
		{ 0, 1, IOCON_MODE_INACT | IOCON_FUNC1 }, /* RXD3 */
		{ 0, 2, IOCON_MODE_INACT | IOCON_FUNC1 }, /* TXD0 */
		{ 0, 3, IOCON_MODE_INACT | IOCON_FUNC1 }, /* RXD0 */
		{ 0, 22, IOCON_MODE_INACT | IOCON_FUNC0 }, /* P0(22) Led 0 (Red)   */
		{ 3, 25, IOCON_MODE_INACT | IOCON_FUNC0 }, /* P3(25) Led 1 (Green) */
		{ 3, 26, IOCON_MODE_INACT | IOCON_FUNC0 }, /* P3(26) Led 2 (Blue)  */
};

// Called before main()
void MyBoard_SysInit(void) {
	Chip_IOCON_SetPinMuxing(LPC_IOCON, pinmuxing, sizeof(pinmuxing) / sizeof(PINMUX_GRP_T));
}

// Called from main()
void MyBoard_Init(void) {
	//  Init the LED Pins as output
	LED0_PORT.DIR |= LED0_MASK;
	LED12_PORT.DIR |= LED1_MASK | LED2_MASK;

	// Init the Debug Uart. This is connected to a VCOM via CMSIS DAP of the Development board.
	Chip_UART_Init(DEBUG_UART);
	Chip_UART_SetBaud(DEBUG_UART, 115200);
	Chip_UART_ConfigData(DEBUG_UART, UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS);
	Chip_UART_TXEnable(DEBUG_UART);
}


void MyBoard_ShowStatusLeds(unsigned char l) {
	if (l & 0x01) {
		LED0_PORT.CLR |= 1UL << LED0_GPIO_BIT_NUM;
	} else {
		LED0_PORT.SET |= 1UL << LED0_GPIO_BIT_NUM;
	}
	if (l & 0x02) {
		LED12_PORT.CLR |= 1UL << LED1_GPIO_BIT_NUM;
	} else {
		LED12_PORT.SET |= 1UL << LED1_GPIO_BIT_NUM;
	}
	if (l & 0x04) {
		LED12_PORT.CLR |= 1UL << LED2_GPIO_BIT_NUM;
	} else {
		LED12_PORT.SET |= 1UL << LED2_GPIO_BIT_NUM;
	}
}

/* Sends a character on the UART */
void Board_UARTPutChar(char ch) {
	while ((Chip_UART_ReadLineStatus(DEBUG_UART) & UART_LSR_THRE) == 0) {
	}
	Chip_UART_SendByte(DEBUG_UART, (uint8_t) ch);
}

/* Gets a character from the UART, returns EOF if no character is ready */
int Board_UARTGetChar(void) {
	if (Chip_UART_ReadLineStatus(DEBUG_UART) & UART_LSR_RDR) {
		return (int) Chip_UART_ReadByte(DEBUG_UART);
	}
	return EOF;
}

void Board_UARTPutSTR(char* str) {
	while (*str != '\0') {
		Board_UARTPutChar(*str++);
	}
}
