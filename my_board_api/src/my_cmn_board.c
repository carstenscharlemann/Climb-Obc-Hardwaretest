/*
 * my_cmn_board.c
 *
 *  Created on: 26.01.2019
 *      Author: Robert
 */

#include <stdlib.h>
#include "chip.h"
#include "my_board_api.h"

__attribute__ ((weak)) void MyBoard_ClockInit(void) {
	Chip_SetupXtalClocking();
	/* Setup FLASH access to 4 clocks (100MHz clock) */
	Chip_SYSCTL_SetFLASHAccess(FLASHTIM_100MHZ_CPU);
}
