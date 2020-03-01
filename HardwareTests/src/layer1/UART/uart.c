#include "uart.h"

void (*irqHandler0)(LPC_USART_T *pUART) = 0;
void (*irqHandler1)(LPC_USART_T *pUART) = 0;
void (*irqHandler2)(LPC_USART_T *pUART) = 0;
void (*irqHandler3)(LPC_USART_T *pUART) = 0;

void InitUart(LPC_USART_T *pUart, int baud, void(*irqHandler)(LPC_USART_T *pUART)){
	Chip_UART_Init(pUart);
	Chip_UART_SetBaud(pUart, baud);				// Sets the next available 'un-fractioned' baud rate.
	Chip_UART_ConfigData(pUart, UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS);	//	8N1
	Chip_UART_SetupFIFOS(pUart, UART_FCR_FIFO_EN | UART_FCR_TRG_LEV1 ); 	//	RX IRQ all 4 bytes

	if (irqHandler != 0) {
		if (pUart == LPC_UART0) {
			irqHandler0 = irqHandler;
			NVIC_EnableIRQ(UART0_IRQn);
		} else if (pUart == LPC_UART1) {
			irqHandler1 = irqHandler;
			NVIC_EnableIRQ(UART1_IRQn);
		} else if (pUart == LPC_UART2) {
			irqHandler2 = irqHandler;
			NVIC_EnableIRQ(UART2_IRQn);
		} else if (pUart == LPC_UART3) {
			irqHandler3 = irqHandler;
			NVIC_EnableIRQ(UART3_IRQn);
		}
	}

	/* Enable UART Transmit */
	Chip_UART_TXEnable(pUart);
}

void UART0_IRQHandler(void) {
	if (irqHandler0 != 0) {
		irqHandler0(LPC_UART0);
	}
}

void UART1_IRQHandler(void) {
	if (irqHandler1 != 0) {
		irqHandler1(LPC_UART1);
	}
}

void UART2_IRQHandler(void) {
	if (irqHandler2 != 0) {
		irqHandler2(LPC_UART2);
	}
}

void UART3_IRQHandler(void) {
	if (irqHandler3 != 0) {
		irqHandler3(LPC_UART3);
	}
}
