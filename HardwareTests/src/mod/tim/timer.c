/*
 * timer.c
 *
 *  Created on: 17.11.2019
 *      Author: Robert
 */

#include <chip.h>
#include <stdio.h>		// for printf()
#include <string.h>		// for strcmp()
#include <stdlib.h>		// for atoi()

#include "..\cli\cli.h"

#include "timer.h"


// Define the timer to use here n=0..3
#define MAINLOOP_TIMER			LPC_TIMER0
#define MAINLOOP_TIMER_IRQ 		TIMER0_IRQn
#define MAINLOOP_TIMER_PCLK		SYSCTL_PCLK_TIMER0

// The matchnum is a 'channel' for different interrupts and timings done with one timer
// possible matchnum is 0..3 (if you change this channel here you needs to restart (power cycle) the debugger used !?
//                            otherwise no new interrupts are triggered!? )
#define INT_MATCHNUM_FOR_TICK	1


// Prototypes
//
void TimIrqHandler(LPC_TIMER_T *mlTimer);
void TimOutputClockCmd(int argc, char *argv[]);

// Variables
//
static bool ticked = false;

// Implementations
//
void TimInit() {

	/* Enable timer clock */
	Chip_TIMER_Init(MAINLOOP_TIMER);

	/* Timer setup for match and interrupt all TIM_MAIN_TICK_MS */
	Chip_TIMER_Reset(MAINLOOP_TIMER);
	Chip_TIMER_MatchEnableInt(MAINLOOP_TIMER, INT_MATCHNUM_FOR_TICK);
	// The needed match counter is calculated by PeripherialClock frequency * [tick time in ms] / 1000;
	// PCLK defines a Divider per Periphery. Default after reset: "divided by 4" (this is in our case: counter has 96/4 = 24Mhz)
	Chip_TIMER_SetMatch( MAINLOOP_TIMER, INT_MATCHNUM_FOR_TICK,
			             (Chip_Clock_GetPeripheralClockRate(MAINLOOP_TIMER_PCLK) / 1000) * TIM_MAIN_TICK_MS );
	Chip_TIMER_ResetOnMatchEnable(MAINLOOP_TIMER, INT_MATCHNUM_FOR_TICK);
	Chip_TIMER_Enable(MAINLOOP_TIMER);

	/* Enable timer interrupt */
	NVIC_ClearPendingIRQ(MAINLOOP_TIMER_IRQ);
	NVIC_EnableIRQ(MAINLOOP_TIMER_IRQ);

	// We use clockout but disable after reset.
	Chip_Clock_DisableCLKOUT();


	// Register module Commands
	RegisterCommand("clkOut", TimOutputClockCmd);
}

// This 'overwrites' the weak definition of this IRQ in cr_startup_lpc175x_6x.c
void TIMER0_IRQHandler(void)
{
	TimIrqHandler(MAINLOOP_TIMER);
}

void TimIrqHandler(LPC_TIMER_T *mlTimer) {
	if (Chip_TIMER_MatchPending(mlTimer, INT_MATCHNUM_FOR_TICK)) {
		Chip_TIMER_ClearMatch(mlTimer, INT_MATCHNUM_FOR_TICK);
		ticked = true;
	}
}

bool TimMain(){
	if (ticked == true) {
		// Signal a single tick happened to Mainloop caller.
		ticked = false;
		return true;
	}
	return false;
}

// Command to switch one of the internal Clock signals to the output PIN43 P1[27] 'CLKOUT'
// In OBC there is the 'Probe CLKO' pad connected. In LPCX this is on 'C16' - 'PAD16'
//
// Cmd Syntax: <cmd> ['OFF'|'CPU'|'OSC'|'IRC'|'USB'|'RTC'] [1...16]
// The optional divider is set to divide by 10 if omitted.
// If no parameter given 'CPU' is default
void TimOutputClockCmd(int argc, char *argv[]) {
	bool on = true;
	CHIP_SYSCTL_CLKOUTSRC_T src = SYSCTL_CLKOUTSRC_CPU;
	uint32_t div = 10;

	if (argc > 0 ) {
		if (strcmp(argv[0],"CPU") == 0 ) {
				src = SYSCTL_CLKOUTSRC_CPU;
		} else if (strcmp(argv[0],"OSC") == 0 ) {
			src = SYSCTL_CLKOUTSRC_MAINOSC;
		} else if (strcmp(argv[0],"IRC") == 0 ) {
			src = SYSCTL_CLKOUTSRC_IRC;
		} else if (strcmp(argv[0],"USB") == 0 ) {
			src = SYSCTL_CLKOUTSRC_USB;
		} else if (strcmp(argv[0],"RTC") == 0 ) {
			src = SYSCTL_CLKOUTSRC_RTC;
		} else if (strcmp(argv[0],"OFF") == 0 ) {
			on = false;
		}
	}
	if (argc > 1 ) {
		div = atoi(argv[1]);
		if (div < 1) {
			div = 1;
		}
		if (div > 16) {
			div = 16;
		}
	}
	Chip_Clock_DisableCLKOUT();
	if (on) {
		Chip_Clock_SetCLKOUTSource(src, div);
		Chip_Clock_EnableCLKOUT();
		printf("CLKOUT (P1.27) enabled. Divider: %d \n", div);
	} else {
		printf("CLKOUT (P1.27) disabled\n");
	}
}


