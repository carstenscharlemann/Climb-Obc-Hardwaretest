/*
 * cmd_loop.c
 *
 *  Created on: 22.01.2019
 *      Author: Robert
 */

#include <string.h>
#include <stdbool.h>

#include "my_board_api.h"		// The user of this common_app proj has to provide a real board impl and link against it !

enum { kMaxArgs = 64, kMaxLineChar=127 };

typedef struct cmdline {
	int argc;
	char *argv[kMaxArgs];
} cmdline_t;

void ProcessCmd(cmdline_t cmd) {
	Board_UARTPutSTR("cmd: ");
	for(int i = 0;i<cmd.argc;i++) {
		Board_UARTPutSTR(cmd.argv[i]);
	}
	Board_UARTPutSTR("\n");
}

cmdline_t ScanLine(char* line) {
	cmdline_t retVal;
	retVal.argc = 0;

	char *p2 = strtok(line, " ");
	while (p2 && retVal.argc < kMaxArgs-1)
	  {
	    retVal.argv[retVal.argc++] = p2;
	    p2 = strtok(0, " ");
	  }
	retVal.argv[retVal.argc] = 0;
	return retVal;
}

void CmdLoop(char* prefix, char* exitCmd) {
	char commandLine[kMaxLineChar + 1];
	cmdline_t cmd;
	cmd.argv[0] = 0;

	while(! ((strcmp(cmd.argv[0], exitCmd) == 0)) ) {
		int ix = 0;
		char c = 'x';

		Board_UARTPutSTR(prefix);
		// Read line chars until CR or bufffer end
		while (((c != '\n' ) && ix < 128)) {
			// wait for char
			while((c = Board_UARTGetChar()) == 0xFF);
			if (c != '\r') {
				commandLine[ix++] = c;
			}
		}
		commandLine[ix-1] = 0x00;
		cmd = ScanLine(commandLine);
		ProcessCmd(cmd);
	}
}
