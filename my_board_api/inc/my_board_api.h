/*
 * my_board_api.h
 *
 *  Created on: 22.01.2019
 *      Author: Robert
 */

#ifndef MY_BOARD_API_H_
#define MY_BOARD_API_H_

// called before Main(). Here we should initialize all IO lines.....
void MyBoard_SysInit(void);

// called from main before mainloop. Here we can use the board for initial operations (e.g. write version to Debug Uart, ...)
void MyBoard_Init(void);


// Use to light some leds. Board Implementations assign the bits from LSB upwards to available LEDs
void MyBoard_ShowStatusLeds(unsigned char ledbits);


/**
 * @brief	Get a single character from the UART, required for scanf input
 * @return	EOF if not character was received, or character value
 */
int Board_UARTGetChar(void);

/**
 * @brief	Prints a string to the UART
 * @param	str	: Terminated string to output
 * @return	None
 */
void Board_UARTPutSTR(char *str);



#endif /* MY_BOARD_API_H_ */
