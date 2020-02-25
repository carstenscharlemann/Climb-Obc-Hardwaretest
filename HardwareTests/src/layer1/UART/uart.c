#include "uart.h"


void InitUart(LPC_USART_T *pUart, LPC175X_6X_IRQn_Type irqType, int baud){
	Chip_UART_Init(pUart);
	Chip_UART_SetBaud(pUart, baud);
	Chip_UART_ConfigData(pUart, UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS);

	/* preemption = 1, sub-priority = 1 */
	//	NVIC_SetPriority(UART2_IRQn, 1);
	NVIC_EnableIRQ(irqType);
	//
	/* Enable UART Transmit */
	Chip_UART_TXEnable(pUart);
}
