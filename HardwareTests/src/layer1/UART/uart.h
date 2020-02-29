/*
 * uart.h
 *
 *  Created on: 25.02.2020
 *      Author: wolfg
 */

#ifndef LAYER1_UART_UART_H_
#define LAYER1_UART_UART_H_

#include <chip.h>
void InitUart(LPC_USART_T *pUart, LPC175X_6X_IRQn_Type irqType, int baud);

#endif /* LAYER1_UART_UART_H_ */
