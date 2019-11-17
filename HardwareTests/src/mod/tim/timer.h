/*
 * timer.h
 *
 *  Created on: 17.11.2019
 *      Author: Robert
 */

#ifndef MOD_TIM_TIMER_H_
#define MOD_TIM_TIMER_H_

#include <stdbool.h>

void TimInit();										// Module Init called once prior mainloop
bool TimMain();										// Module routine participating each mainloop.


#endif /* MOD_TIM_TIMER_H_ */
