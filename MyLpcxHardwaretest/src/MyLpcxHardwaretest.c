/*
 ===============================================================================
 Name        : MyLpcxHardwaretest.c
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
#include "cmd_loop.h"


/**
 * @brief	RTC interrupt handler
 * @return	Nothing
 */
void RTC_IRQHandler(void)
{
	uint32_t sec;

	/* Toggle heart beat LED for each second field change interrupt */
	if (Chip_RTC_GetIntPending(LPC_RTC, RTC_INT_COUNTER_INCREASE)) {
		/* Clear pending interrupt */
		Chip_RTC_ClearIntPending(LPC_RTC, RTC_INT_COUNTER_INCREASE);
		//On0 = (bool) !On0;
		//Board_LED_Set(0, On0);
	}

	/* display timestamp every 5 seconds in the background */
	sec = Chip_RTC_GetTime(LPC_RTC, RTC_TIMETYPE_SECOND);
	if (!(sec % 5)) {
		//fIntervalReached = true;	/* set flag for background */
	}

	/* Check for alarm match */
	if (Chip_RTC_GetIntPending(LPC_RTC, RTC_INT_ALARM)) {
		/* Clear pending interrupt */
		Chip_RTC_ClearIntPending(LPC_RTC, RTC_INT_ALARM);
		//fAlarmTimeMatched = true;	/* set alarm handler flag */
	}
}


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

	Chip_RTC_Init(LPC_RTC);

	/* Set the RTC to generate an interrupt on each second */
	Chip_RTC_CntIncrIntConfig(LPC_RTC, RTC_AMR_CIIR_IMSEC, ENABLE);

	/* Enable matching for alarm for second, minute, hour fields only */
//	Chip_RTC_AlarmIntConfig(LPC_RTC, RTC_AMR_CIIR_IMSEC | RTC_AMR_CIIR_IMMIN | RTC_AMR_CIIR_IMHOUR, ENABLE);

	/* Clear interrupt pending */
	Chip_RTC_ClearIntPending(LPC_RTC, RTC_INT_COUNTER_INCREASE | RTC_INT_ALARM);

	/* Enable RTC interrupt in NVIC */
	NVIC_EnableIRQ((IRQn_Type) RTC_IRQn);

	/* Enable RTC (starts increase the tick counter and second counter register) */
	Chip_RTC_Enable(LPC_RTC, ENABLE);




	Board_UARTPutSTR("\nHello LPCX (type 'm' for command shell).");

	// Force the counter to be placed into memory
	volatile static int i = 0;
	volatile static int l = 0;

	// Enter an infinite loop, just incrementing a counter
	while (1) {
		if (Board_UARTGetChar() == 'm') {
			MyBoard_ShowStatusLeds(0xFF);
			while (Board_UARTGetChar() != '\n');	// Consume NewLine.
			CmdLoop("LPCX>", "exit");
		}

		i++;
		if (i % 1000000 == 0) {
			l++;
			MyBoard_ShowStatusLeds((unsigned char) (l));
		}
	}
	return 0;
}
