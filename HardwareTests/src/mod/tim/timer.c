/*
 * timer.c
 *
 *  Created on: 17.11.2019
 *      Author: Robert
 */

#include "timer.h"

static int i = 0;

void TimInit() {

}

bool TimMain(){
	if (i++ % 10000 == 0) {
		return true;
	}
	return false;
}
