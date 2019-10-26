/*
 * globals.h
 *
 *  Created on: 26.10.2019
 *      Author: Robert
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_

#if defined(BOARD_CLIMB_OBC_EM1)
	#define LED_NR_WATCHDOG	0
	#define LED_NR_BLUE 	1
#endif

#if defined(BOARD_NXP_LPCXPRESSO_1769)
	#define LED_NR_WATCHDOG	0
	#define LED_NR_BLUE 	0

	// dummy boot mode functions
	#define GetBootmodeStr() "not available"
	#define GetBootmode() 1
#endif


#endif /* GLOBALS_H_ */
