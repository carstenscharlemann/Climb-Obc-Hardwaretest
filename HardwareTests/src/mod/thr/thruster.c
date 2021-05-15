/*
 * thruster.c
 *
 *  Created on: 11.11.2019
 */

// ... include C std dependencies if needed ....
#include <stdio.h>			// use if needed (e.g. for printf, ...)
#include <string.h>			// use if needed (e.g. for strcpy, str... )
#include <ring_buffer.h>	// TODO: replace with own implementation (with random size ant not 2^x as this one!)

// ... include dependencies on other modules if needed ....
// #include "..\..\globals.h"   // use if needed
#include "../../layer1/UART/uart.h"
#include "../cli/cli.h"
#include "thruster.h"

// module defines
#define THR_HELLO_STR	"Hi there CHEEEEE.\n"
#define THRUSTER_UART	LPC_UART0					// This is the same in OBC and LPCX board
#define THR_TXBUFFER_SIZE 64						// the biggest command should be good here

// module prototypes (local routines not in public API)
void ThrUartIrq(LPC_USART_T *pUart);
void ThrusterSend(char *str);
void ThrusterSendCmd(int argc, char *argv[]);
void ThrusterPrintStatusCmd(int argc, char *argv[]);
void ThrusterSendHexRequest(int argc, char *argv[]);
void ThrusterSendVersionRequest(int argc, char *argv[]);
int ThrusterGetChar();

void SetReservoirTemperature(int argc, char *argv[]);
void RequestSkeleton(uint8_t value);




uint8_t REGISTER_VALUES[1]={0x00,0x00};
uint8_t ADRESS = 0xff;




#define THR_RXBUFFER_SIZE 128
char ThrRxBuffer[THR_RXBUFFER_SIZE+1];
int ThrRxIdx = 0;


// module variables
int myStateExample;
int thrTxError;

bool thrtxInProgress = false;
char prtThrTxBuffer[THR_TXBUFFER_SIZE];
RINGBUFF_T thrTxRingbuffer;



typedef struct trxRequest{
	uint8_t sender;
	uint8_t receiver;
	uint16_t len;
} trxRequest;


typedef struct ThrusterCharStruct{
	char* first_name;
	char* last_name;

}ThrusterCharStruct;


// module function implementations
// -------------------------------
// Module Init - here goes all module init code to be done before mainloop starts.
void ThrInit() {
	RingBuffer_Init(&thrTxRingbuffer,(void *)prtThrTxBuffer, 1, THR_TXBUFFER_SIZE);

	// initialize the thruster UART ....
	InitUart(THRUSTER_UART, 9600, ThrUartIrq);

	// register module commands with cli
	RegisterCommand("ss", ThrusterSendCmd);
	RegisterCommand("trStat", ThrusterPrintStatusCmd);
	RegisterCommand("hex", ThrusterSendHexRequest);
	RegisterCommand("trVersion", ThrusterSendVersionRequest);


	RegisterCommand("sett", SetReservoirTemperature);




	myStateExample = 0;
	thrTxError = 0;

	// Init communications to the thruster if needed ....
	ThrusterSendChar(THR_HELLO_STR);
}

//uint_8 mybuffer[20];
//trxRequest_t  myCurThrRequest;
//int responseByteIdx = 0;
// thats the 'mainloop' call of the module
void ThrMain() {
	//trxRequest_t  myCurThrResponse;


	//int size = sizeof(trxRequest_t);

 /*
	int32_t stat = Chip_UART_ReadLineStatus(THRUSTER_UART);
	if (stat & UART_LSR_RDR) {
		// byte received
		uint8_t byte = Chip_UART_ReadByte(THRUSTER_UART);
		char* send ="received";
		//send = (char)byte;

		ThrusterSendChar(send);


	//	....
//		last byte of responser
//		-> call some routines for reaction.
		//e.g. for READ response Put out the read darta on CLI
	}

	*/
	int ch;
	//int ThrusterGetChar()

	while ((ch = ThrusterGetChar()) != -1) {
		// make echo
		// CliPutChar((char)(ch));
		if (ch != 0x0a &&
		    ch != 0x0d) {
			ThrRxBuffer[ThrRxIdx] = (char)(ch);
			ThrRxIdx++;
		}

		if ((ThrRxIdx >= THR_RXBUFFER_SIZE) ||
			 ch == 0x0a ||
			 ch == 0x0d) 	{
			ThrRxBuffer[ThrRxIdx] = 0x00;
			printf (ThrRxBuffer); //print receiced buffer to cli uart
			//processLine();
			ThrRxIdx= 0;
		}
	}


	// do your stuff here. But remember not to make 'wait' loops or other time consuming operations!!!
	// Its called 'Cooperative multitasking' so be kind to your sibling-modules and return as fast as possible!
	if (myStateExample++ % 800000 == 0) {
		// Note printf() does not take too much time here, but keep the texts small and do not float the CLI (its slow)!
		//printf("Hello this goes to CLI UART!\n");

		//sendSomeCharToThruster();
		ThrusterCharStruct someCharStruct;
		someCharStruct.first_name ="Privet ";
		someCharStruct.last_name = " Kak dela \n";
		int len ;
		len = sizeof(someCharStruct);

		//ThrusterSendCharStruct(someCharStruct,len); // THRUSTER UART 9600



		// try the same with uint_8 structure
		//ThrusterSendChar("\n ping\n\n");

		trxRequest debug_uint8_struct;
		debug_uint8_struct.len= 15;
		debug_uint8_struct.receiver = 16;
		debug_uint8_struct.sender = 17;

		len  = sizeof(debug_uint8_struct);
		//ThrusterSendUINT_8_Struct(debug_uint8_struct,len);


		// now try to send uint_8 request
		//uint8_t valueMsg= 0x48;
		//RequestSkeleton(valueMsg);






	}

}

void ThrusterSendHexRequest(int argc, char *argv[]){

	//uint8_t requestHI[3];
	//requestHI[0]= "0x48";
	//requestHI[1]= "0x69";
	//requestHI[2]= "0x69";
	//uint8_t *requestHI = 0x4869;
	//uint8_t requestHI[5] = {0x48,0x69};
	uint8_t requestHI[4];
	requestHI[0]= 0x48;
	requestHI[1]= 0x69;
	requestHI[2]= 0x69;
	requestHI[3]= 0x00;


	int len = sizeof(requestHI);

	ThrusterSendUint8_t(requestHI,len);
	printf("\n Hex request sent to thruster \n");

}








void ThrusterSendVersionRequest(int argc, char *argv[]){


	uint8_t request[8];
	request[0]= 0x00;
	request[1]= 0xFF;
	request[2]= 0x03;
	request[3]= 0x14;
	request[4]= 0x02;
	request[5]= 0x00;
	request[6]= 0x00;
	request[7]= 0x01;


	int len = sizeof(request);

	ThrusterSendUint8_t(request,len);
	printf("\n Version request sent to thruster \n");


}



void sendSomeCharToThruster(void){
	char *tx = "Sputnik";
	char *add = "Flying \n";
	char sendBuffer[50];
	int n;
	n=sprintf (sendBuffer, "hello  %s we are %s ",tx,add);
	ThrusterSendChar(sendBuffer);

}

// An example CLI Command to trigger thruster communication
void ThrusterSendCmd(int argc, char *argv[]){
	//uint8_t tx[10] = 0x0001020305;
	//uint16_t len = 0xF0BD ;
	//int8_t x = 255;
		//tx[0] = 0x00;
		//tx[1] = 0xFF;
//.... Get Version Number READ Commadn



	char* tx = "che che";

	ThrusterSendChar(tx);
	printf("sent to thruster \n");
}

void ThrusterPrintStatusCmd(int argc, char *argv[]) {
	printf("Thruster TxErrors: %d\n", thrTxError);
}


// Send content of string to Thruster UART
void ThrusterSendUint8_t(uint8_t *bytedata, int len) {
	//Chip_GPIO_SetPinOutLow(LPC_GPIO, 3, 25);


	if (RingBuffer_InsertMult(&thrTxRingbuffer, (void*)bytedata, len) != len) {
		// Tx Buffer is to small to hold all bytes
		thrTxError++;
	}
	if (!thrtxInProgress) {
		// Trigger to send the first byte and enable the TxEmptyIRQ
		char c;
		thrtxInProgress = true;
		RingBuffer_Pop(&thrTxRingbuffer, &c);
		Chip_UART_SendByte(THRUSTER_UART, c);
		Chip_UART_IntEnable(THRUSTER_UART, UART_IER_THREINT);
	}
	//Chip_GPIO_SetPinOutHigh(LPC_GPIO, 3, 25);
}






// Send content of string to Thruster UART
void ThrusterSendChar(char *str) {
	//Chip_GPIO_SetPinOutLow(LPC_GPIO, 3, 25);
	int len = strlen(str);
	if (RingBuffer_InsertMult(&thrTxRingbuffer, (void*)str, len) != len) {
		// Tx Buffer is to small to hold all bytes
		thrTxError++;
	}
	if (!thrtxInProgress) {
		// Trigger to send the first byte and enable the TxEmptyIRQ
		char c;
		thrtxInProgress = true;
		RingBuffer_Pop(&thrTxRingbuffer, &c);
		Chip_UART_SendByte(THRUSTER_UART, c);
		Chip_UART_IntEnable(THRUSTER_UART, UART_IER_THREINT);
	}
	//Chip_GPIO_SetPinOutHigh(LPC_GPIO, 3, 25);
}



// Send content of string to Thruster UART
void ThrusterSendCharStruct(ThrusterCharStruct *bytedata, int len) {
	//Chip_GPIO_SetPinOutLow(LPC_GPIO, 3, 25);


	if (RingBuffer_InsertMult(&thrTxRingbuffer, (void*)bytedata, len) != len) {
		// Tx Buffer is to small to hold all bytes
		thrTxError++;
	}
	if (!thrtxInProgress) {
		// Trigger to send the first byte and enable the TxEmptyIRQ
		char c;
		thrtxInProgress = true;
		RingBuffer_Pop(&thrTxRingbuffer, &c);
		Chip_UART_SendByte(THRUSTER_UART, c);
		Chip_UART_IntEnable(THRUSTER_UART, UART_IER_THREINT);
	}
	//Chip_GPIO_SetPinOutHigh(LPC_GPIO, 3, 25);
}



// Send content of UINT structure to Thruster UART
void ThrusterSendUINT_8_Struct(trxRequest *bytedata, int len) {
	//Chip_GPIO_SetPinOutLow(LPC_GPIO, 3, 25);


	if (RingBuffer_InsertMult(&thrTxRingbuffer, (void*)bytedata, len) != len) {
		// Tx Buffer is to small to hold all bytes
		thrTxError++;
	}
	if (!thrtxInProgress) {
		// Trigger to send the first byte and enable the TxEmptyIRQ
		char c;
		thrtxInProgress = true;
		RingBuffer_Pop(&thrTxRingbuffer, &c);
		Chip_UART_SendByte(THRUSTER_UART, c);
		Chip_UART_IntEnable(THRUSTER_UART, UART_IER_THREINT);
	}
	//Chip_GPIO_SetPinOutHigh(LPC_GPIO, 3, 25);
}





void ThrUartIrq(LPC_USART_T *pUart){
	Chip_GPIO_SetPinOutLow(LPC_GPIO, 3, 26);
	uint32_t irqid = pUart->IIR;
	if (( irqid & UART_IIR_INTSTAT_PEND ) == 0) {
		// There is an Irq pending
		if (( irqid & (UART_IIR_INTID_RLS) ) == 0 ) {
			// This was a line status-error IRQ
//			uint32_t ls = pUart->LSR;		// clear this pending IRQ

		} else if ((irqid & (UART_IIR_INTID_RDA || UART_IIR_INTID_CTI )) != 0) {
			// This was a "Rx Fifo treshhold reached" or a "char timeout" IRQ -> Bytes are available in RX FIFO to be processsed
//			uint8_t rbr = pUart->RBR;

			// not used (yet?) in thruster module
		} else if ((irqid & (UART_IIR_INTID_THRE)) != 0) {
			// The Tx FIFO is empty (now). Lets fill it up. It can hold up to 16 (UART_TX_FIFO_SIZE) bytes.
			char c;
			int  i = 0;
			while( i++ < UART_TX_FIFO_SIZE) {
				if (RingBuffer_Pop(&thrTxRingbuffer, &c) == 1) {
					Chip_UART_SendByte(THRUSTER_UART, c);
				} else {
					// We have to stop because our tx ringbuffer is empty now.
					thrtxInProgress = false;
					Chip_UART_IntDisable(THRUSTER_UART, UART_IER_THREINT);
					break;
				}
			}
		}
	}
	Chip_GPIO_SetPinOutHigh(LPC_GPIO, 3, 26);
}


int ThrusterGetChar() {
	int32_t stat = Chip_UART_ReadLineStatus(THRUSTER_UART);
//	if (stat & UART_LSR_OE) {
//		return -2;
//	}
//	if (stat & UART_LSR_RXFE) {
//		return -3;
//	}
	if (stat & UART_LSR_RDR) {
		return (int) Chip_UART_ReadByte(THRUSTER_UART);
	}
	return -1;
}


/*
void DropeNukes(int argc, char *argv[]){

	if (argc != 2) {
		printf("Please select enemy state: \n" );
		return;
	}


	char *myptr = argv[0];

	// CLI params to binary params
	char* Country = argv[0];
	char * State  = argv[1];
	printf("%s %s selected\n",Country,State); //print to UART2
	printf("Launching ...");

	char* sendstring = "Bye Bye ";
	int size;
	size = sizeof(Country) / sizeof(Country[0]);
	strncat(sendstring, &Country, size);
	ThrusterSendChar(sendstring);


}
*/




void RequestSkeleton(uint8_t value){


	uint8_t request[3];
	request[0]= REGISTER_VALUES[0];
	request[1]= value;
	request[2]= value;

	int len = sizeof(request);

	ThrusterSendUint8_t(request,len);


}



void SetReservoirTemperature(int argc, char *argv[]){

	//char* refference_temp_char = argv[0]; // take goal argument from sent command

	uint8_t reff_t = atoi(argv[0]);
	printf(" REF TEMP SET %d \n",reff_t);

	uint8_t request[3];
	request[0]= REGISTER_VALUES[0];
	request[1]= reff_t;
	request[2]= reff_t;

	int len = sizeof(request);

	ThrusterSendUint8_t(request,len);


}
