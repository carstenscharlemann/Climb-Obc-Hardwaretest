/*
 * timer.c
 *
 *  Created on: 17.11.2019
 *      Author: Robert
 */

#include <chip.h>

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

}

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


// Reference code from PEG / Done with CMSISv2p00_LPC17xx Chip Abstraction ....

/*********************************************************************//**
 * Macro defines for Power Control for Peripheral Register
 **********************************************************************/
/** Power Control for Peripherals bit mask */

//#define CLKPWR_PCONP_BITMASK	0xEFEFF7DE


/*********************************************************************//**
 * @brief 		Configure power supply for each peripheral according to NewState
 * @param[in]	PPType	Type of peripheral used to enable power,
 *     					should be one of the following:
 *     			-  CLKPWR_PCONP_PCTIM0 		: Timer 0
 -  CLKPWR_PCONP_PCTIM1 		: Timer 1
 -  CLKPWR_PCONP_PCUART0  	: UART 0
 -  CLKPWR_PCONP_PCUART1   	: UART 1
 -  CLKPWR_PCONP_PCPWM1 		: PWM 1
 -  CLKPWR_PCONP_PCI2C0 		: I2C 0
 -  CLKPWR_PCONP_PCSPI   	: SPI
 -  CLKPWR_PCONP_PCRTC   	: RTC
 -  CLKPWR_PCONP_PCSSP1 		: SSP 1
 -  CLKPWR_PCONP_PCAD   		: ADC
 -  CLKPWR_PCONP_PCAN1   	: CAN 1
 -  CLKPWR_PCONP_PCAN2   	: CAN 2
 -  CLKPWR_PCONP_PCGPIO 		: GPIO
 -  CLKPWR_PCONP_PCRIT 		: RIT
 -  CLKPWR_PCONP_PCMC 		: MC
 -  CLKPWR_PCONP_PCQEI 		: QEI
 -  CLKPWR_PCONP_PCI2C1   	: I2C 1
 -  CLKPWR_PCONP_PCSSP0 		: SSP 0
 -  CLKPWR_PCONP_PCTIM2 		: Timer 2
 -  CLKPWR_PCONP_PCTIM3 		: Timer 3
 -  CLKPWR_PCONP_PCUART2  	: UART 2
 -  CLKPWR_PCONP_PCUART3   	: UART 3
 -  CLKPWR_PCONP_PCI2C2 		: I2C 2
 -  CLKPWR_PCONP_PCI2S   	: I2S
 -  CLKPWR_PCONP_PCGPDMA   	: GPDMA
 -  CLKPWR_PCONP_PCENET 		: Ethernet
 -  CLKPWR_PCONP_PCUSB   	: USB
 *
 * @param[in]	NewState	New state of Peripheral Power, should be:
 * 				- ENABLE	: Enable power for this peripheral
 * 				- DISABLE	: Disable power for this peripheral
 *
 * @return none
 **********************************************************************/
//void CLKPWR_ConfigPPWR(uint32_t PPType, FunctionalState NewState)
//{
//	if (NewState == ENABLE)
//	{
//		LPC_SC->PCONP |= PPType & CLKPWR_PCONP_BITMASK;
//	}
//	else if (NewState == DISABLE)
//	{
//		LPC_SC->PCONP &= (~PPType) & CLKPWR_PCONP_BITMASK;
//	}
//}
//
//
//RetVal timer0_init(void) /* in Hz */
//{
//    /* Init Timer 0 befor RTC init! */
//
//    CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCTIM0, ENABLE);
//
//    LPC_TIM0->PR = CLKPWR_GetPCLK(CLKPWR_PCLKSEL_TIMER0) / 1000 - 1; /* 1 kHz timer frequency */
//    LPC_TIM0->TCR = 2; /* Reset & hold timer */
//    LPC_TIM0->MR0 = 999; /* 1s */
//    LPC_TIM0->MCR |= (1 << 1); /* Reset timer on Match 0 */
//
//    /*LPC_TIM0->MCR |= (1 << 0);*//* Interrupt on Match0 compare */
//    /*NVIC_SetPriority(TIMER0_IRQn, 0);*/
//    /*NVIC_EnableIRQ(TIMER0_IRQn);*//* Enable timer0 interrupt */
//
//    timer0_start();
//    obc_status.timer0_initialized = 1;
//    return DONE;
//}
//
//void timer0_start(void)
//{
//    LPC_TIM0->TCR = 1; /* Start  timer */
//    obc_status_extended.timer0_running = 1;
//}
//
//void timer0_reset(void)
//{
//    LPC_TIM0->TCR = 2; /* Reset & hold timer */
//    obc_status_extended.timer0_running = 0;
//}
//
//void timer0_stop(void)
//{
//    LPC_TIM0->TCR = 0; /* Reset & hold timer */
//    obc_status_extended.timer0_running = 0;
//}
//
//void TIMER0_IRQHandler(void)
//{
//    if ((LPC_TIM0->IR & 1) == 1) /* 100 Hz Timer Interrupt */
//    { /* MR0 interrupt */
//        LPC_TIM0->IR |= 1; /* Clear MR0 interrupt flag */
//        /*TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT); */
//    }
//}
