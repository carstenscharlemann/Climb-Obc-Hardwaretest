/*
 * timer.h
 *
 *  Created on: 17.11.2019
 *      Author: Robert
 */

#ifndef MOD_TIM_TIMER_H_
#define MOD_TIM_TIMER_H_

#define TIM_MAIN_TICK_MS	(250)		// This is the 'feature define' for choosing your main loop tick time in milliseconds.

#include <stdbool.h>

void TimInit();						    // Module Init called once prior mainloop
bool TimMain();							// Module routine participating each mainloop.


#endif /* MOD_TIM_TIMER_H_ */
