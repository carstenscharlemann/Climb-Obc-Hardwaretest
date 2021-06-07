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
#include "../crc/obc_checksums.h"

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
void SetHeaterMode(int argc, char *argv[]);
void SetHeaterVoltage(int argc, char *argv[]);
void SetHeaterCurrent(int argc, char *argv[]);
void SetHeaterPower(int argc, char *argv[]);
void ReadHeaterCurrent(int argc, char *argv[]);

/*
#define TR_HEATER_CURRENT_REFF 0x41
#define  SetHeaterCurrentRef(x)   Set_Int_Reg_Value(TR_HEATER_CURRENT_REFF, x)

*/
#define TR_HEATER_CURRENT 0x43

//uint8_t CONVERSION[108] = {[0 ... 107]=1};
uint16_t CONVERSION[108] = {0,1,2,3,4,5,6,7,8,9,10,
		11,12,13,14,15,16,17,18,19,20,
		21,22,23,24,25,26,27,28,29,30,
		31,32,33,34,35,36,37,38,39,40,
		41,42,43,44,45,46,48,48,49,50,
		51,52,53,54,55,56,57,58,59,1,
		1000,62,63,64,10000,66,67,68,1000,70,
		71,72,73,74,75,76,77,78,79,80,
		81,82,83,84,85,86,87,88,89,90,
		91,92,93,94,95,96,100,98,99,100,
		101,102,103,104,105,106,107
		};


//uint8_t REGISTER_VALUES[1]={0x61,0x00};
uint8_t REGISTER_VALUES[108]={0,1,2,3,4,5,6,7,8,9,10,
		11,12,13,14,15,16,17,18,19,20,
		21,22,23,24,25,26,27,28,29,30,
		31,32,33,34,35,36,37,38,39,40,
		41,42,43,44,45,46,48,48,49,50,
		51,52,53,54,55,56,57,58,59,60,
		61,62,63,64,65,66,67,68,69,70,
		71,72,73,74,75,76,77,78,79,80,
		81,82,83,84,85,86,87,88,89,90,
		91,92,93,94,95,96,97,98,99,100,
		101,102,103,104,105,106,107
		};
// 0 - Reseirvoir Temp Ref (LSB)
uint8_t SENDER_ADRESS = 0x00;
uint8_t DEVICE = 0xff;

uint8_t MSGTYPE[7]={0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
// 0 - OK
// 1 - ERROR
// 2- READ
// 3 - WRITE
// 4 - DATA
// 5 - RESET
//6 - UPDATE
//7  -CONFIG



//#define THR_RXBUFFER_SIZE 128
#define THR_RXBUFFER_SIZE 7 // WAIT TO RECEIVE 6 bytes from thruster
char ThrRxBuffer[THR_RXBUFFER_SIZE+1];
int ThrRxIdx = 0;
int tr_ExpectedReceiveBuffer = 1; //7

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
	RegisterCommand("heatermode", SetHeaterMode);
	RegisterCommand("setheaterv", SetHeaterVoltage);
	RegisterCommand("setheaterc", SetHeaterCurrent);
	RegisterCommand("setheaterp", SetHeaterPower);
	RegisterCommand("readheaterc", ReadHeaterCurrent);


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

		//if ((ThrRxIdx >= THR_RXBUFFER_SIZE) ||
		if ((ThrRxIdx >= tr_ExpectedReceiveBuffer) ||

			 ch == 0x0a ||
			 ch == 0x0d) 	{
			ThrRxBuffer[ThrRxIdx] = 0x00;
			//printf (" DATA PACKAGE RECEIVED---- \n");
			//printf (ThrRxBuffer); //print receiced buffer to cli uart
			printf("\n ------------------------\n");
			//printf("\n RECEIVED BYTE 0 %d \n",(uint8_t)ThrRxBuffer[0]);
			//printf("\n RECEIVED BYTE 1 %d \n",(uint8_t)ThrRxBuffer[1]);
			//printf("\n RECEIVED BYTE 2 %d \n",(uint8_t)ThrRxBuffer[2]);
			//printf("\n RECEIVED BYTE 3 %d \n",(uint8_t)ThrRxBuffer[3]);
			//printf("\n RECEIVED BYTE 4 %d \n",(uint8_t)ThrRxBuffer[4]);
			//printf("\n RECEIVED BYTE 5 %d \n",(uint8_t)ThrRxBuffer[5]);

			//printf("RECEIVED HEX 0 %x \n",ThrRxBuffer[0] & 0xff);
			//printf("RECEIVED HEX 1 %x \n",ThrRxBuffer[1] & 0xff);
			//printf("RECEIVED HEX 2 %x \n",ThrRxBuffer[2] & 0xff);
			//printf("RECEIVED HEX 3 %x \n",ThrRxBuffer[3] & 0xff);
			//printf("RECEIVED HEX 4 %x \n",ThrRxBuffer[4] & 0xff);
			//printf("RECEIVED HEX 5 %x \n",ThrRxBuffer[5] & 0xff);

			printf("%x %x %x %x %x %x %x \n",ThrRxBuffer[0] & 0xff,ThrRxBuffer[1] & 0xff,ThrRxBuffer[2] & 0xff,ThrRxBuffer[3] & 0xff,ThrRxBuffer[4] & 0xff,ThrRxBuffer[5] & 0xff,ThrRxBuffer[6] & 0xff);


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

/* Compute CRC8 (binary String)
uint8_t CRC8(uint8_t* str, size_t length)
{
    uint8_t checksum = 0;

    for (; length--; c_CRC8(*str++, &checksum))
        ;

    return checksum;
}

 */
void SetReservoirTemperature(int argc, char *argv[]){


	uint16_t reff_t = atoi(argv[0]);
	printf(" REF TEMP SET %d \n",reff_t);
	// APPLY CONVERSION MULTIPLIER CONCEPT
	//uint16_t conversion_mult = 100;
	//reff_t = reff_t*conversion_mult;
	reff_t = reff_t*CONVERSION[97];
	printf(" CONVERSION  =  %d \n",CONVERSION[97]);

	uint8_t request[9];
	request[0] = SENDER_ADRESS;
	request[1] = DEVICE;
	request[2] = MSGTYPE[3]; // WRITE -3
	//checksumm
	request[3] = 0x00;



	// payload length two bytes 03 00 | two bytes for
	// payload length = register map + value
	request[4] = 0x03; // LENGTH of payload HARDCODED (3)
	request[5] = 0x00; //hardcoded

	request[6]= REGISTER_VALUES[97]; // RESEIRVOUR TEMPERATURE - index 97



	// CONVERT uint16_t value into array of two uint8_t bytes
	uint8_t conversionArray[2];
	conversionArray[0] = reff_t & 0xff;
	conversionArray[1] = (reff_t >> 8) & 0xff;

	request[7]= conversionArray[0];
	request[8] = conversionArray[1];




	int len = sizeof(request);


	request[3] = CRC8(request,len);

	ThrusterSendUint8_t(request,len);


}





void SetHeaterMode(int argc, char *argv[]){

	//  HEATER MODE 1 or 0
	uint16_t heater_mode = atoi(argv[0]);

	if (heater_mode ==1 || heater_mode ==0){

		// APPLY CONVERSION MULTIPLIER CONCEPT
		//uint16_t conversion_mult = 1;
		//heater_mode = heater_mode*conversion_mult;
		heater_mode = heater_mode * CONVERSION[60];
		printf(" CONVERSION  =  %d \n",CONVERSION[60]);

		uint8_t request[8];
		request[0] = SENDER_ADRESS;
		request[1] = DEVICE;
		request[2] = MSGTYPE[3]; // WRITE -3
		request[3] = 0x00; //byte for checksum initially set to 0
		request[4] = 0x02; // LENGTH of payload HARDCODED (2)       || payload length = register map + value
		request[5] = 0x00; //hardcoded
		request[6]= REGISTER_VALUES[60]; // HEATER MODE - 60
		request[7]= heater_mode;

		int len = sizeof(request);
		request[3] = CRC8(request,len); // after actual checksum is calculated = byte is filled
		ThrusterSendUint8_t(request,len);

	}
	else {
		printf("HEATER MODE WRONG INPUT %d \n",heater_mode);

		return;
	}


}






void SetHeaterVoltage(int argc, char *argv[]){


	uint16_t voltage = atoi(argv[0]);
	printf(" SET VOLTAGE  =  %d \n",voltage);
	// APPLY CONVERSION MULTIPLIER CONCEPT
	//uint16_t conversion_mult = 1000;
	//voltage = voltage*conversion_mult;
	voltage = voltage * CONVERSION[61];
	printf(" CONVERSION  =  %d \n",CONVERSION[61]);

	uint8_t request[9];
	request[0] = SENDER_ADRESS;
	request[1] = DEVICE;
	request[2] = MSGTYPE[3]; // WRITE -3
	request[3] = 0x00; //checksumm
	request[4] = 0x03; // LENGTH of payload HARDCODED (3)
	request[5] = 0x00; //hardcoded
	request[6]= REGISTER_VALUES[61]; // RESEIRVOUR TEMPERATURE - index 97



	// CONVERT uint16_t value into array of two uint8_t bytes
	//uint8_t conversionArray[2];
	//conversionArray[0] = voltage & 0xff;
	//conversionArray[1] = (voltage >> 8) & 0xff;

	request[7]= voltage & 0xff;
	request[8] = (voltage >> 8) & 0xff;

	int len = sizeof(request);


	request[3] = CRC8(request,len);

	ThrusterSendUint8_t(request,len);


}






void SetHeaterCurrent(int argc, char *argv[]){
	// INPUT RANGE 0-3[A]  !!!!

	// PARSE DOUBLE FROM ARGV
	double input;
	sscanf(argv[0], "%lf", &input);

	printf(" \n ARGV  =  %s \n",argv[0]);
	printf(" \n ORIGINAL INPUT  =  %f \n",input);
	//double conversion_factor = (double)CONVERSION[65];

	//APPLY CONVERSION MULTIPLIER
	input = input * (double)CONVERSION[65];
	//input = input / 0.0001;
	printf(" \n CONVERTED INPUT  =  %f \n",input);
	printf(" \n MULTIPLIER  =  %f \n",(double)CONVERSION[65]);


	// CONVERT INPUT INTO UINT16
	uint16_t value = (uint16_t) input;
	printf("\n SET CURRENT  =  %d \n",value);


	uint8_t request[9];
	request[0] = SENDER_ADRESS;
	request[1] = DEVICE;
	request[2] = MSGTYPE[3]; // WRITE -3
	request[3] = 0x00; //checksumm
	request[4] = 0x03; // LENGTH of payload HARDCODED (3)
	request[5] = 0x00; //hardcoded
	request[6]= REGISTER_VALUES[65]; // RESEIRVOUR TEMPERATURE - index 65
	request[7]= value & 0xff;
	request[8] = (value >> 8) & 0xff;

	int len = sizeof(request);


	request[3] = CRC8(request,len);
	tr_ExpectedReceiveBuffer = 6;// change expected receive buffer accordingly

	ThrusterSendUint8_t(request,len);


}


void SetHeaterPower(int argc, char *argv[]){


	uint16_t value = atoi(argv[0]);
	printf(" SET POWER  =  %d \n",value);
	// APPLY CONVERSION MULTIPLIER CONCEPT
	//uint16_t conversion_mult = 1000;
	//uint16_t conversion_mult = CONVERSION[69];
	value = value * CONVERSION[69];

	printf(" CONVERSION  =  %d \n",CONVERSION[69]);

	uint8_t request[9];
	request[0] = SENDER_ADRESS;
	request[1] = DEVICE;
	request[2] = MSGTYPE[3]; // WRITE -3
	request[3] = 0x00; //checksumm
	request[4] = 0x03; // LENGTH of payload HARDCODED (3)
	request[5] = 0x00; //hardcoded
	request[6]= REGISTER_VALUES[69]; // RESEIRVOUR TEMPERATURE - index 69
	request[7]= value & 0xff;
	request[8] = (value >> 8) & 0xff;

	int len = sizeof(request);


	request[3] = CRC8(request,len);

	ThrusterSendUint8_t(request,len);


}





void ReadHeaterCurrent(int argc, char *argv[]){

		uint8_t request[8];
		request[0] = SENDER_ADRESS;
		request[1] = DEVICE;
		request[2] = MSGTYPE[2]; // READ -3
		request[3] = 0x00; //checksumm
		request[4] = 0x02; // LENGTH of payload REGISTER and length
		request[5] = 0x00; //hardcoded
		request[6]= TR_HEATER_CURRENT; // value from DEFINE register
		request[7]= 2;// number of bytes to read
		//request[8] = (value >> 8) & 0xff;

		int len = sizeof(request);


		request[3] = CRC8(request,len);

		ThrusterSendUint8_t(request,len);



}














/*
void SetDEBUGCURRENT_reff(int argc, char *argv[]){


	uint16_t value = atoi(argv[0]);
	printf(" SET current  =  %d \n",value);
	SetHeaterCurrentRef(value);


}



void  Set_Int_Reg_Value(uint8_t reg_index,uint16_t value){



		value = value * CONVERSION[reg_index];

		printf(" CONVERSION  =  %d \n",CONVERSION[reg_index]);

		uint8_t request[9];
		request[0] = SENDER_ADRESS;
		request[1] = DEVICE;
		request[2] = MSGTYPE[3]; // WRITE -3
		request[3] = 0x00; //checksumm
		request[4] = 0x03; // LENGTH of payload HARDCODED (3)
		request[5] = 0x00; //hardcoded
		request[6]= reg_index; // RESEIRVOUR TEMPERATURE - index 69
		request[7]= value & 0xff;
		request[8] = (value >> 8) & 0xff;

		int len = sizeof(request);


		request[3] = CRC8(request,len);

		ThrusterSendUint8_t(request,len);


}
*/
