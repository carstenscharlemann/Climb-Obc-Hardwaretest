/*
 * my_board_api.h
 *
 *  Created on: 22.01.2019
 *      Author: Robert
 */

#ifndef MY_BOARD_API_H_
#define MY_BOARD_API_H_

// External API Prototypes - This board HW abstractions should be used to access hardware layer independent of target board.
// Specific board implementations have to provide the code.
// -------------------------------------------------------------------------------------------------------------------------------

// called before Main(). Here we should initialize all IO lines, Timers, UARTS, .....
void MyBoard_SysInit(void);

// called from main before mainloop. Here we can use the board for initial operations (e.g. LED Status, write version to Debug Uart, ...)
void MyBoard_Init(void);

// Use to light some leds. Board Implementations assign the bits from LSB upwards to available LEDs.
void MyBoard_ShowStatusLeds(unsigned char ledbits);

// Debug UART. Board Implementation decides which UART to use here.
int Board_UARTGetChar(void);
void Board_UARTPutChar(char ch);
void Board_UARTPutSTR(char *str);


// Common Default Implementations available for all boards.
// This is code written in the my_board_api project with 'weak' implementation attribute. Writing the same
// methods in specific board implementations overrides this default code.

void MyBoard_ClockInit(void);

#endif /* MY_BOARD_API_H_ */
