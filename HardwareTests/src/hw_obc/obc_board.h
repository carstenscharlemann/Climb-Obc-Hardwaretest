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
#define ClimbGetBootmode()		ObcGetBootmode()
#define ClimbGetBootmodeStr()	ObcGetBootmodeStr()

#define ClimbLedToggle(x)		ObcLedToggle(x)
#define ClimbLedSet(x,y)		ObcLedSet(x,y)
#define ClimbLedTest(x)			ObcLedTest(x)

// definitions only available in OBC version. For other boards Alias for ObcGetBootmode() gives a pure number (int).
typedef enum {DebugEven, DebugOdd, Even, Odd} bootmode_t;

// Module Implementation Prototypes
void ObcClimbBoardInit();
void ObcClimbBoardSystemInit();
bootmode_t ObcGetBootmode();
char* ObcGetBootmodeStr();

void ObcLedToggle(uint8_t ledNr);
void ObcLedSet(uint8_t ledNr,  bool On);
bool ObcLedTest(uint8_t ledNr);

#endif /* HW_OBC_OBC_BOARD_H_ */
