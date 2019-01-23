/*
===============================================================================
 Name        : MyMbedHardwaretest.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <cr_section_macros.h>

// TODO: insert other include files here
#include "my_board_api.h"

// TODO: insert other definitions and declarations here

int main(void) {

#if defined (__USE_LPCOPEN)
    // Read clock settings and update SystemCoreClock variable
    SystemCoreClockUpdate();
#if !defined(NO_BOARD_LIB)
    // Set up and initialize all required blocks and
    // functions related to the board hardware
    Board_Init();
    // Set the LED to the state of "On"
    Board_LED_Set(0, true);
#endif
#endif


    MyBoard_Init();
    Board_UARTPutSTR("\nHello mbed (type 'm' for command shell).");


    // Force the counter to be placed into memory
    volatile static int i = 0 ;
    volatile static int l = 0 ;

    // Enter an infinite loop, just incrementing a counter
    while(1) {
    	if (Board_UARTGetChar() == 'm') {
			MyBoard_ShowStatusLeds(0xFF);
			while (Board_UARTGetChar() != '\n');	// Consume NewLine.
			CmdLoop("mbed>", "quit");
		}

        i++ ;
        if (i % 1000000 == 0) {
            l++;
            MyBoard_ShowStatusLeds((unsigned char)(l));
        }
    }
    return 0 ;
}
