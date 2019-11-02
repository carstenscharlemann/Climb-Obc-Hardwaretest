/*
 * obc_board.h
 *
 *  Created on: 02.11.2019
 *      Author: Robert
 */

#ifndef HW_OBC_OBC_BOARD_H_
#define HW_OBC_OBC_BOARD_H_

#include "chip.h"

// Module API (all as Aliases pointing to implementation)
#define ClimbBoardInit ObcClimbBoardInit
#define ClimbBoardSystemInit ObcClimbBoardSystemInit

// Module Implementation Prototypes
void ObcClimbBoardInit();
void ObcClimbBoardSystemInit();

#endif /* HW_OBC_OBC_BOARD_H_ */
