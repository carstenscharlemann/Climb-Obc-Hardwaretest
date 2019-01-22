/*
 * board_impl.c
 *
 *  Created on: 22.01.2019
 *      Author: Robert
 */

#include "chip.h"
#include "my_board_api.h"



#define LED_PORT			          LPC_GPIO[1]
#define LED_GPIO_PORT_NUM						1
#define LED0_GPIO_BIT_NUM                      18
#define LED1_GPIO_BIT_NUM                      20
#define LED2_GPIO_BIT_NUM                      21
#define LED3_GPIO_BIT_NUM                      23

#define LED0_MASK	(1UL << LED0_GPIO_BIT_NUM)
#define LED1_MASK	(1UL << LED1_GPIO_BIT_NUM)
#define LED2_MASK	(1UL << LED2_GPIO_BIT_NUM)
#define LED3_MASK	(1UL << LED3_GPIO_BIT_NUM)


STATIC const PINMUX_GRP_T pinmuxing[] = {
	{1,  18,  IOCON_MODE_INACT | IOCON_FUNC0},	/* Led 1 */
	{1,  20,  IOCON_MODE_INACT | IOCON_FUNC0},	/* Led 2 */
	{1,  21,  IOCON_MODE_INACT | IOCON_FUNC0},	/* Led 3 */
	{1,  23,  IOCON_MODE_INACT | IOCON_FUNC0},	/* Led 4 */
};


void MyBoard_SysInit(void) {
	Chip_IOCON_SetPinMuxing(LPC_IOCON, pinmuxing, sizeof(pinmuxing) / sizeof(PINMUX_GRP_T));
}

void MyBoard_Init(void) {
    //  Init the LED Pin as output
	LED_PORT.DIR |= LED0_MASK | LED1_MASK | LED2_MASK | LED3_MASK;
}

void MyBoard_ShowStatusLeds(unsigned char l) {
	if (l & 0x01) {
		LED_PORT.SET |= 1UL << LED0_GPIO_BIT_NUM;
	} else {
		LED_PORT.CLR |= 1UL << LED0_GPIO_BIT_NUM;
	}
	if (l & 0x02) {
		LED_PORT.SET |= 1UL << LED1_GPIO_BIT_NUM;
	} else {
		LED_PORT.CLR |= 1UL << LED1_GPIO_BIT_NUM;
	}
	if (l & 0x04) {
		LED_PORT.SET |= 1UL << LED2_GPIO_BIT_NUM;
	} else {
		LED_PORT.CLR |= 1UL << LED2_GPIO_BIT_NUM;
	}
	if (l & 0x08) {
		LED_PORT.SET |= 1UL << LED3_GPIO_BIT_NUM;
	} else {
		LED_PORT.CLR |= 1UL << LED3_GPIO_BIT_NUM;
	}
}
