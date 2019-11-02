/*
===============================================================================
 Name        : HardwareTests.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#include "globals.h"

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#if HW_USED == LPCX_BOARD
	#include "hw_lpcx/lpcx_board.h"
#elif HW_USED == OBC_BOARD
	#include "hw_obc/obc_board.h"
#endif

#include "retarget.h"
#include <cr_section_macros.h>

// TODO: insert other include files here

// TODO: insert other definitions and declarations here

int main(void) {

#if defined (__USE_LPCOPEN)
#if defined (HW_USED)
	ClimbBoardInit();
#else
	// The original LpcOpen way of Chip inizializing if no board is defined. Not sure if this Clock Update is needed ...???...
    // Read clock settings and update SystemCoreClock variable
    SystemCoreClockUpdate();
#endif
#if !defined(NO_BOARD_LIB)
    // Set up and initialize all required blocks and
    // functions related to the board hardware
    Board_Init();
    // Set the LED to the state of "On"
    Board_LED_Set(0, true);
#endif
#endif

    // TODO: insert code here
    printf("Hello Climb HardwareTest.");
    // Force the counter to be placed into memory
    volatile static int i = 0 ;
    // Enter an infinite loop, just incrementing a counter
    while(1) {
    	i++ ;
		 if (i % 1000000 == 0) {
			//Board_LED_Toggle(LED_GREEN_WD);
			ClimbLedToggle(0);
			//Board_LED_Set(LED_GREEN_WD, false);	// For ever
			printf(".");
		}

    }
    return 0 ;
}
