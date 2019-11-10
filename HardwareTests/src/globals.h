/*
 * globals.h
 *
 *  Created on: 02.11.2019
 *      Author: Robert
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_

// Available board abstractions
#define OBC_BOARD	1
#define LPCX_BOARD	2

// Switch the board used here, and only here!
#define HW_USED	LPCX_BOARD
//#define HW_USED OBC_BOARD


#if HW_USED == LPCX_BOARD
	#include "hw_lpcx/lpcx_board.h"
#elif HW_USED == OBC_BOARD
	#include "hw_obc/obc_board.h"
#endif


#endif /* GLOBALS_H_ */
