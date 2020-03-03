/*
 * obc_ttc.c
 *
 *  Created on: 02.03.2020
 */

#include <string.h>

#include "..\..\layer1\UART\uart.h"
#include "..\cli\cli.h"

#include "obc_ttc.h"

#define TTC_TXBUFFER_SIZE	64			// This must always be power of 2 !!!!!
#define TTC_A_UART			LPC_UART3	//
#define TTC_C_UART			LPC_UART1	//

bool ttcaTxInProgress = false;
RINGBUFF_T ttcaTxRingbuffer;
char ttcaTxBuffer[TTC_TXBUFFER_SIZE];

bool ttccTxInProgress = false;
RINGBUFF_T ttccTxRingbuffer;
char ttccTxBuffer[TTC_TXBUFFER_SIZE];

void TtcUartIrq(LPC_USART_T *pUart);
void TtcSendCmd(int argc, char *argv[]);
void TtcSendBytes(LPC_USART_T *pUart, uint8_t *pBytes, int len);

void TtcInit() {
	RingBuffer_Init(&ttcaTxRingbuffer,(void *)ttcaTxBuffer, 1, TTC_TXBUFFER_SIZE);
	RingBuffer_Init(&ttccTxRingbuffer,(void *)ttccTxBuffer, 1, TTC_TXBUFFER_SIZE);

	InitUart(TTC_A_UART, 9600, TtcUartIrq);
	InitUart(TTC_C_UART, 9600, TtcUartIrq);

	Chip_UART_IntEnable(TTC_A_UART, UART_IER_RLSINT | UART_IER_RBRINT);

	RegisterCommand("ttcSend", TtcSendCmd);
}

void TtcMain() {

}

void TtcUartIrq(LPC_USART_T *pUart){
	Chip_GPIO_SetPinOutLow(LPC_GPIO, 3, 26);

	RINGBUFF_T *pTxRingBuffer;
	bool *progressFlag;
	if (pUart == TTC_A_UART) {
		pTxRingBuffer = &ttcaTxRingbuffer;
		progressFlag = &ttcaTxInProgress;
	} else if (pUart == TTC_C_UART) {
		pTxRingBuffer = &ttccTxRingbuffer;
		progressFlag = &ttccTxInProgress;
	} else {
		// ???????
		return;
	}

	uint32_t irqid = pUart->IIR;
	uint8_t irqSource = (irqid & 0x0E)>>1;
	int error = 0;
	int i = 0;


	if (( irqid & UART_IIR_INTSTAT_PEND ) == 0) {
		// There is an Irq pending
		if ( irqSource == 0x03 ) {
			// This was a line status-error IRQ
			uint32_t ls = pUart->LSR;		// clear this pending IRQ
			if (( ls & ( UART_LSR_BI | UART_LSR_FE | UART_LSR_OE | UART_LSR_PE)) > 0 ) {
				error++;
			}
		} else if ( (irqSource == 0x02) || (irqSource == 0x06)) {
			// This was a "Rx Fifo treshhold reached" or a "char timeout" IRQ -> Bytes are available in RX FIFO to be processsed
			uint8_t buffer[20];

			uint32_t status;
			while (1) {
				status = pUart->LSR;
				if (( status & ( UART_LSR_BI | UART_LSR_FE | UART_LSR_OE | UART_LSR_PE)) > 0 ) {
					error++;
				}
				if ((status & UART_LSR_RDR) > 0) {
					buffer[i++] = pUart->RBR;
				} else {
					break;
				}
			}
		} else if (irqSource == 0x01) {
			// The Tx FIFO is empty (now). Lets fill it up. It can hold up to 16 (UART_TX_FIFO_SIZE) bytes.
			char c;
			int  i = 0;
			while( i++ < UART_TX_FIFO_SIZE) {
				if (RingBuffer_Pop(pTxRingBuffer, &c) == 1) {
					Chip_UART_SendByte(pUart, c);
				} else {
					// We have to stop because our tx ringbuffer is empty now.
					*progressFlag = false;
					Chip_UART_IntDisable(pUart, UART_IER_THREINT);
					break;
				}
			}
		}
		i = i + 10;
	}
	Chip_GPIO_SetPinOutHigh(LPC_GPIO, 3, 26);
}

void TtcSendCmd(int argc, char *argv[]){
	LPC_USART_T *pUart = TTC_A_UART;
	char *sendString = "ABCDEFG";
	if (argc > 1) {
		if (strcmp(argv[0],"C") == 0) {
			pUart = TTC_C_UART;
		}
		sendString = argv[1];
	} else if (argc == 1) {
		sendString = argv[0];
	}

	TtcSendBytes(pUart, (uint8_t *)sendString, strlen(sendString));
}

void TtcSendBytes(LPC_USART_T *pUart, uint8_t *pBytes, int len) {
	RINGBUFF_T *pRingBuffer;
	bool *progressFlag;
	if (pUart == TTC_A_UART) {
		pRingBuffer = &ttcaTxRingbuffer;
		progressFlag = &ttcaTxInProgress;
	} else if (pUart == TTC_C_UART) {
		pRingBuffer = &ttccTxRingbuffer;
		progressFlag = &ttccTxInProgress;
	} else {
		// ???????
		return;
	}

	if (RingBuffer_InsertMult(pRingBuffer, (void*)pBytes, len) != len) {
			// Tx Buffer is to small to hold all bytes
			//thrTxError++;
	}
	if (! (*progressFlag)) {
		// Trigger to send the first byte and enable the TxEmptyIRQ
		char c;
		*progressFlag = true;
		RingBuffer_Pop(pRingBuffer, &c);
		Chip_UART_SendByte(pUart, c);
		Chip_UART_IntEnable(pUart, UART_IER_THREINT);
	}
}
