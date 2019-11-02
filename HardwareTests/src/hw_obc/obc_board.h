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
#define ClimbBoardInit 			ObcClimbBoardInit
#define ClimbBoardSystemInit 	ObcClimbBoardSystemInit

#define ClimbLedToggle(x)		ObcLedToggle(x)
#define ClimbLedSet(x,y)		ObcLedSet(x,y)
#define ClimbLedTest(x)			ObcLedTest(x)

#define ClimbCliUARTPutChar(x) 	ObcCliUARTPutChar(x)
#define ClimbCliUARTGetChar 	ObcCliUARTGetChar


// Module Implementation Prototypes
void ObcClimbBoardInit();
void ObcClimbBoardSystemInit();

void ObcLedToggle(uint8_t ledNr);
void ObcLedSet(uint8_t ledNr,  bool On);
bool ObcLedTest(uint8_t ledNr);

void ObcCliUARTPutChar(char c);
int  ObcCliUARTGetChar();

#endif /* HW_OBC_OBC_BOARD_H_ */
