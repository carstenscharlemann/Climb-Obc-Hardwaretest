/*
 * clock_commands.c
 *
 *  Created on: 23.01.2019
 *      Author: Robert
 */

#include <stdlib.h>

#include "chip.h"

#include "my_board_api.h"
#include "clock_commands.h"


cmdresult_t ClockCmd(int argc, char** argv) {
	Board_UARTPutSTR("Clock Command called.\n");

	CHIP_SYSCTL_CCLKSRC_T ct = Chip_Clock_GetCPUClockSource();
	uint32_t clk = Chip_Clock_GetSystemClockRate();

	if (ct == SYSCTL_CCLKSRC_MAINPLL) {
		Board_UARTPutSTR("MainPll: ");
	} else {
		Board_UARTPutSTR("other: ");
	}

	char vstring [33];
	itoa (clk,vstring,10); // convert to decimal
	Board_UARTPutSTR(vstring);
	Board_UARTPutSTR(" Hz\n");

	return cmdOk;
}

