/*
 * cmd_loop.h
 *
 *  Created on: 22.01.2019
 *      Author: Robert
 */

#ifndef CMD_LOOP_H_
#define CMD_LOOP_H_


enum { kMaxArgs = 64, kMaxLineChar=127 };

typedef struct cmdline {
	int argc;
	char *argv[kMaxArgs];
} cmdline_t;

typedef struct commands {
	char* cmdStr;
	void (*cmdPtr) (int argc, char** argv);
} commands_t;


extern void CmdLoop(char* prefix, char* exitCmd );


#endif /* CMD_LOOP_H_ */
