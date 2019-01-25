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

// Use to light some leds. Board Implementations assign the bits from LSB upwards to available LEDs.
void MyBoard_ShowStatusLeds(unsigned char ledbits);

// Debug UART. Board Implementation decides which UART to use here.
int Board_UARTGetChar(void);
void Board_UARTPutChar(char ch);
void Board_UARTPutSTR(char *str);

#endif /* MY_BOARD_API_H_ */
