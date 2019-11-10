/*
 * cli.c
 *
 *  Created on: 02.11.2019
 *      Author: Robert
 */
#include <stdio.h>
#include "cli.h"

#define CLI_PROMPT "\ncl>"

//
// local module variables
// ----------------------
LPC_USART_T *cliUart;						// pointer to UART used for CLI

// The Rx line buffer - used with polling from mainloop
#define CLI_RXBUFFER_SIZE 64
char cliRxBuffer[CLI_RXBUFFER_SIZE];
int cliRxPtrIdx = 0;

// The Tx 'ringbuffer' used for TX with interrupt routine
#define CLI_TXBUFFER_SIZE 128
bool prtTxInProgress = false;
int prtBufferRead = 0;
int prtBufferWrite = 0;
char prtBuffer[CLI_TXBUFFER_SIZE];

// module statistics
int ignoredTxChars = 0;
int bufferErrors   = 0;
int linesProcessed = 0;

//
// local module prototypes
// ----------------------
void processLine();

//
// Module function implementations
// -------------------------------

// This is called from BoardInit. So the specific Uart to be used is decided there!
void CliInitUart(LPC_USART_T *pUart, LPC175X_6X_IRQn_Type irqType){
	cliUart = pUart;
	Chip_UART_Init(pUart);
	Chip_UART_SetBaud(pUart, 115200);
	Chip_UART_ConfigData(pUart, UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS);

	/* preemption = 1, sub-priority = 1 */
	//	NVIC_SetPriority(UART2_IRQn, 1);
	NVIC_EnableIRQ(irqType);
	//
	/* Enable UART Transmit */
	Chip_UART_TXEnable(pUart);
}

// The UART Interrupt handler. We only use 'TX empty' interrupt to get out the next byte from our tx 'ringbuffer'
// This method must be wrapped from the real UART interrupt (there is one for each of the 4 UARTx
// We do this in board.c. There we have to call the CliUartIni and make the correct ISR point to here.
void CliUartIRQHandler(LPC_USART_T *pUART) {
	if (pUART->IER & UART_IER_THREINT) {
		prtBufferRead++;
		if (prtBufferRead == CLI_TXBUFFER_SIZE) {
			prtBufferRead = 0;
		}

		if (prtBufferRead != prtBufferWrite) {
			// We still have bytes to catch up with the written buffer content. Send out next one.
			Chip_UART_SendByte(pUART, prtBuffer[prtBufferRead]);
		} else {
			// Nothing left to send.  Disable transmit interrupt.
			Chip_UART_IntDisable(pUART, UART_IER_THREINT);
			prtTxInProgress = false;
		}

	}
}

// Sends a character on the UART without wasting to much time.
// This is also used by all redlib io to stdout (printf,....)
// We put the char in our TX ringbuffer and initialize sending if needed.
void CliPutChar(char ch) {
	if (prtTxInProgress) {
		// We just put the char into buffer. Tx is already running and will be re-triggered by tx interrupt.
		if (prtBufferRead != prtBufferWrite) {
			prtBuffer[prtBufferWrite++]=ch;
			if (prtBufferWrite >= CLI_TXBUFFER_SIZE) {
				prtBufferWrite = 0;
			}
		} else {
			// Thats bad. Seems buffer is overrun (or prtTxInProgress is 'lying')
			// ... TODO ... log event here !?
			ignoredTxChars++;
		}
	} else {
		// We trigger a new Tx block.
		if (prtBufferRead != prtBufferWrite) {
			// Thats strange. somebody left the buffer 'unsent' -> lets reset
			prtBufferRead = 0;
			prtBufferWrite = 0;
			// TODO .. log event here
			bufferErrors++;
		}
		prtBuffer[prtBufferWrite++]=ch;
		if (prtBufferWrite >= CLI_TXBUFFER_SIZE) {
			prtBufferWrite = 0;
		}

		// We trigger sending without checking of Line Status here because we trust our own variables and
		// want to avoid clearing the possible Rx Overrun error by reading the status register !?
		// I am not sure the 'non occurence' of the Rx Overrun in my first version (without tx interrupt) was caused by this
		// but its my only explanation i had for what i was seeing then. ( Mainloop was delayed by tx waiting for LineStatus
		// -> output was cut off after 2 lines and skipped to the end of my RX - Teststring containing a lot more characters
		// than 2 64 byte lines. Breakpoint for Rx Overrun was never hit !!???
		prtTxInProgress = true;
		Chip_UART_IntEnable(cliUart, UART_IER_THREINT);
		Chip_UART_SendByte(cliUart, (uint8_t) ch);
	}
}

// Used locally and by redlib for stdio readline... (not tested yet)
int CliGetChar() {
	int32_t stat = Chip_UART_ReadLineStatus(cliUart);
//	if (stat & UART_LSR_OE) {
//		return -2;
//	}
//	if (stat & UART_LSR_RXFE) {
//		return -3;
//	}
	if (stat & UART_LSR_RDR) {
		return (int) Chip_UART_ReadByte(cliUart);
	}
	return -1;
}

// This module init from main module. Remark: The Uart initialization is done in CliInitUart() called by board init.
void CliInit() {
	printf(CLI_PROMPT);
}

// This is module main loop entry. Do not use (too much) time here!!!
void CliMain(){
	int ch;

	// The UART has 16 byte Input buffer
	// read all available bytes in this main loop call.
	while ((ch = CliGetChar()) != -1) {
		// make echo
		// CliPutChar((char)(ch));
		if (ch != 0x0a &&
		    ch != 0x0d) {
			cliRxBuffer[cliRxPtrIdx++] = (char)(ch);
		}

		if ((cliRxPtrIdx >= CLI_RXBUFFER_SIZE) ||
			 ch == 0x0a ||
			 ch == 0x0d) 	{
			cliRxBuffer[cliRxPtrIdx] = 0x00;
			processLine();
			cliRxPtrIdx = 0;
			printf(CLI_PROMPT);
		}
	}
}


void processLine() {
	linesProcessed++;
	printf("\nRe:%s",cliRxBuffer);
}
