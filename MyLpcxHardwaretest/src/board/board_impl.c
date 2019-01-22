/*
 * board_impl.c
 *
 *  Created on: 22.01.2019
 *      Author: Robert
 */

#include "chip.h"
#include "my_board_api.h"

//
// Board specific definitions/implementations for the "OEM 13085" aka "LPCXpresso for LPC1769 CD" (Version D1 with 3 color LED)
// *****************************************************************************************************************************

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

STATIC const PINMUX_GRP_T pinmuxing[] = {
	{0,  22,  IOCON_MODE_INACT | IOCON_FUNC0},	/* Led 0 (Red)   */
	{3,  25,  IOCON_MODE_INACT | IOCON_FUNC0},	/* Led 1 (Green) */
	{3,  26,  IOCON_MODE_INACT | IOCON_FUNC0},	/* Led 2 (Blue)  */
};

void MyBoard_SysInit(void) {
	Chip_IOCON_SetPinMuxing(LPC_IOCON, pinmuxing, sizeof(pinmuxing) / sizeof(PINMUX_GRP_T));
}

void MyBoard_Init(void) {
    //  Init the LED Pins as output
	LED0_PORT.DIR |= LED0_MASK;
	LED12_PORT.DIR |= LED1_MASK | LED2_MASK;
}

void MyBoard_ShowStatusLeds(unsigned char l) {
	if (l & 0x01) {
		LED0_PORT.SET |= 1UL << LED0_GPIO_BIT_NUM;
	} else {
		LED0_PORT.CLR |= 1UL << LED0_GPIO_BIT_NUM;
	}
	if (l & 0x02) {
		LED12_PORT.SET |= 1UL << LED1_GPIO_BIT_NUM;
	} else {
		LED12_PORT.CLR |= 1UL << LED1_GPIO_BIT_NUM;
	}
	if (l & 0x04) {
		LED12_PORT.SET |= 1UL << LED2_GPIO_BIT_NUM;
	} else {
		LED12_PORT.CLR |= 1UL << LED2_GPIO_BIT_NUM;
	}
}
