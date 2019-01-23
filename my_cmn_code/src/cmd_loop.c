/*
 * cmd_loop.c
 *
 *  Created on: 22.01.2019
 *      Author: Robert
 */

#include <string.h>
#include <stdbool.h>

#include "my_board_api.h"		// The user of this cmn_code project has to provide a real board implementation and link against it!
#include "cmd_loop.h"

extern void ClockCmd(int argc, char** argv);

static const commands_t commands[] = {
		{ "clk", ClockCmd  }
};

void ClockCmd(int argc, char** argv) {
	Board_UARTPutSTR("Clock Command called.\n");
}


void ProcessCmd(cmdline_t cmd) {
	if (cmd.argc > 0) {
		int arrayLength = sizeof(commands) / sizeof(commands_t);
		for (int ix = 0; ix < arrayLength; ix++ ) {
			if (strcmp(cmd.argv[0], commands[ix].cmdStr) == 0) {
				// Call the Command function
				commands[ix].cmdPtr(cmd.argc, cmd.argv);
				return;
			}
		}
		Board_UARTPutSTR("unknown cmd:");
		for(int i = 0;i<cmd.argc;i++) {
			Board_UARTPutChar(' ');
			Board_UARTPutSTR(cmd.argv[i]);
		}
		Board_UARTPutSTR("\n");
	}


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
