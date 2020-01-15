/*
 * dosimeter.c
 *
 *  Created on: 11.01.2020
 */

#include <Chip.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "..\..\layer1\SPI\spi.h"
#include "..\cli\cli.h"

#define FLOGA_VCC_ENABLE_PORT	0
#define FLOGA_VCC_ENABLE_PIN   26

#define FLOGA_FAULT_PORT		1
#define FLOGA_FAULT_PIN		    9

#define FLOGA_IRQ_PORT			1
#define FLOGA_IRQ_PIN		   10

#define FLOGA_CS_PORT			1
#define FLOGA_CS_PIN			4

typedef struct flog_registers_s
{
    uint8_t  temp;
    int32_t  sensorValue;
    int32_t  refValue;
    bool     newSensValue;
	bool     newRefValue;
	bool     sensOverflow;
	bool     refOverflow;
	uint8_t  reCharges;
	bool	 reChargeInProgress;
	bool	 reChargeRequested;
}
flog_registers_t;

typedef enum flog_status_e {
	FLOG_STAT_IDLE,
	FLOG_STAT_RX_JOB,
	FLOG_STAT_TXCONFIG1_JOB,
	FLOG_STAT_TXCONFIG2_JOB,
	FLOG_STAT_TXCONFIG3_JOB,


	FLOG_STAT_MEASSURE_POWERON,
	FLOG_STAT_MEASSURE_CFG1,
	FLOG_STAT_MEASSURE_CFG2,
	FLOG_STAT_MEASSURE_CFGFINISHED,
	FLOG_STAT_MEASSURE_READDELAY,
	FLOG_STAT_MEASSURE_READ,

} flog_status_t;


uint8_t 		RxBuffer[20];
uint8_t 		TxBuffer[20];
bool   			jobFinished;

flog_status_t	status;
uint8_t         valuesToMeassure;
uint8_t			delayTicks;
bool 			configStep1Only = false;

void ReadAllCmd(int argc, char *argv[]);
void ConfigDeviceCmd(int argc, char *argv[]);
void Switch18VCmd(int argc, char *argv[]);

bool ReadAllRegisters();
bool ConfigDevice1();
void InitializeAndMakeMessurement(uint8_t cnt);
void LogMeasurementResult();

void ChipSelectFlog(bool select);

void ReadAllCmd(int argc, char *argv[]){
	if (!ReadAllRegisters()) {
		printf("Error adding SPI Job!\n");
	}
}

void ConfigDeviceCmd(int argc, char *argv[]){
	configStep1Only = false;
	if (argc > 0 && strcmp(argv[0],"FSTO") == 0) {
		configStep1Only = true;
	}

	if (!ConfigDevice1()) {
		printf("Error adding SPI Job 1!\n");
	}
}

void Switch18VCmd(int argc, char *argv[]){
	if (argc == 1) {
		if (strcmp(argv[0],"ON") == 0  || strcmp(argv[0],"on") == 0 ) {
			Chip_GPIO_SetPinState(LPC_GPIO, FLOGA_VCC_ENABLE_PORT, FLOGA_VCC_ENABLE_PIN, false);
			printf("18V switched on!\n");
		} else {
			Chip_GPIO_SetPinState(LPC_GPIO, FLOGA_VCC_ENABLE_PORT, FLOGA_VCC_ENABLE_PIN, true);
			printf("18V switched off!\n");
		}
	}
}
void MakeMeasurementCmd(int argc, char *argv[]){
	uint8_t probeCount = 20;
	if (argc > 0) {
		probeCount = atoi(argv[0]);
	}
	InitializeAndMakeMessurement(probeCount);
}

void FgdInit() {
	spi_init();

	//	/* --- Chip selects IOs --- */
	Chip_IOCON_PinMuxSet(LPC_IOCON, FLOGA_CS_PORT, FLOGA_CS_PIN, IOCON_FUNC0 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, FLOGA_CS_PORT, FLOGA_CS_PIN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, FLOGA_CS_PORT, FLOGA_CS_PIN);
	Chip_GPIO_SetPinState(LPC_GPIO, FLOGA_CS_PORT, FLOGA_CS_PIN, true);						// init as 'not selected'

	/* FLOGA_EN is output and switches on the 18V Charge Pump (low active) */
	Chip_IOCON_PinMuxSet(LPC_IOCON, FLOGA_VCC_ENABLE_PORT, FLOGA_VCC_ENABLE_PIN, IOCON_FUNC0 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, FLOGA_VCC_ENABLE_PORT, FLOGA_VCC_ENABLE_PIN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, FLOGA_VCC_ENABLE_PORT, FLOGA_VCC_ENABLE_PIN);
	Chip_GPIO_SetPinState(LPC_GPIO, FLOGA_VCC_ENABLE_PORT, FLOGA_VCC_ENABLE_PIN, true);		// Init as off

	/* FLOGA_FAULT, FLOGA_IRQ is input */
	Chip_IOCON_PinMuxSet(LPC_IOCON, FLOGA_FAULT_PORT, FLOGA_FAULT_PIN, IOCON_FUNC0 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, FLOGA_FAULT_PORT, FLOGA_FAULT_PIN);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, FLOGA_FAULT_PORT, FLOGA_FAULT_PIN);

	Chip_IOCON_PinMuxSet(LPC_IOCON, FLOGA_IRQ_PORT, FLOGA_IRQ_PIN, IOCON_FUNC0 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, FLOGA_IRQ_PORT, FLOGA_IRQ_PIN);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, FLOGA_IRQ_PORT, FLOGA_IRQ_PIN);

	status = FLOG_STAT_IDLE;
	jobFinished = false;
	RegisterCommand("flogVcc", Switch18VCmd);
	RegisterCommand("flogRead", ReadAllCmd);
	RegisterCommand("flogConfig", ConfigDeviceCmd);
//	RegisterCommand("flogMeasure", MakeMeasurementCmd);
}

// This is the callback used by layer1 to activate/deactivate the chip select line for the Floating gate dosimeter
void ChipSelectFlog(bool select) {
	Chip_GPIO_SetPinState(LPC_GPIO, FLOGA_CS_PORT, FLOGA_CS_PIN, !select);
	if (!select) {
		// Job is finished
		jobFinished = true;
	}
}

// Mainloop call every TIM_MAIN_TICK_MS milliseconds.
void FgdMain() {
	if ((status != FLOG_STAT_IDLE) && (jobFinished)) {
		jobFinished = false;
		// The job has just finished its work. Lets go on with the state machine
		if (status == FLOG_STAT_RX_JOB) {
			LogMeasurementResult();
			status = FLOG_STAT_IDLE;
//			printf("flogRegRaw ; ");
//			for(int i=0; i<20;i++) {
//				printf("%02X ",RxBuffer[i]);
//			}
//			printf("\n");
//
//			flog_registers_t reg;
//			reg.newRefValue = RxBuffer[5] & 0x08;
//			reg.refOverflow = RxBuffer[5] & 0x04;
//			reg.newSensValue =  RxBuffer[8] & 0x08;
//			reg.sensOverflow =  RxBuffer[8] & 0x04;
//
//			reg.refValue =  (((uint32_t)RxBuffer[5]) & 0x03) << 16 | ((uint32_t)RxBuffer[4])<<8 | ((uint32_t)RxBuffer[3]);
//			reg.sensorValue =  (((uint32_t)RxBuffer[8]) & 0x03) << 16 | ((uint32_t)RxBuffer[7])<<8 | ((uint32_t)RxBuffer[6]);
//
//			printf("new %d Ref: %ld (ovf: %d) ; new %d Sens: %ld (ovf: %d) \n",reg.newRefValue, reg.refValue, reg.refOverflow, reg.newSensValue, reg.sensorValue, reg.sensOverflow);
		} else if (status == FLOG_STAT_TXCONFIG1_JOB) {
			if (configStep1Only) {
				status = FLOG_STAT_IDLE;
				printf("Config finished early.\n");
			} else {
				// Next step in device config is to set the TARGET byte (see Datasheet Page 8 - C.4 ) We skip C.2 - C.3 as remarked in datasheet Note.
				TxBuffer[0] = 0x09 | 0x40 ; /* Write register  */

				TxBuffer[1] = 0x2a;			// This target value was measured at  device 'PRIMUS' on 14.1.2020 (ref Values were between 44128 and 44170 (dez) -> bit 17:10 -> 0x2B (44032 ...45055)
											// after some playing around the ref value came down to 44000 . -> so I changed the bit 10 to 0 -> 0x2a;
				if (spi_add_job(ChipSelectFlog, TxBuffer, 2, NULL, 0)) {
					status = FLOG_STAT_TXCONFIG2_JOB;
				} else {
					status = FLOG_STAT_IDLE;
					printf("Error adding SPI Job 2!\n");
				}
			}
		} else if (status == FLOG_STAT_TXCONFIG2_JOB) {
			// Next step in device config is to enable the recharching system
			TxBuffer[0] = 0x0d | 0x40 ; /* Write register  */

			TxBuffer[1] = 0x41;			// ECH: 1 ... recharge allowed, CHMOD: 01 .. automatic recharging mode
			if (spi_add_job( ChipSelectFlog, TxBuffer, 2, NULL, 0)) {
				status = FLOG_STAT_TXCONFIG3_JOB;
			} else {
				status = FLOG_STAT_IDLE;
				printf("Error adding SPI Job 3!\n");
			}
		} else if (status == FLOG_STAT_TXCONFIG3_JOB) {
			status = FLOG_STAT_IDLE;
			printf("Config finished.\n");
		}
	}

}
//	if (delayTicks > 0) {
//		delayTicks--;
//		return;
//	}
//	switch (status) {
//	case FLOG_STAT_MEASSURE_POWERON:
//		// check if power has failed.
//		if (!Chip_GPIO_GetPinState(LPC_GPIO, FLOGA_FAULT_PORT, FLOGA_FAULT_PIN)) {
//			printf("18V power fault!\n");
//			// Clock off, VCC disable
//			Chip_Clock_DisableCLKOUT();
//			Chip_GPIO_SetPinState(LPC_GPIO, FLOGA_VCC_ENABLE_PORT, FLOGA_VCC_ENABLE_PIN, true);
//			status = FLOG_STAT_IDLE;
//			return;
//		}
//		// now we can initialize the FGD Chip.
//		TxBuffer[0] = 0x0B | 0x40 ; /* Write registers from adr 0x0B */
//		TxBuffer[1] = 0x40;			// REF: 100 .. Referenz freq set to middle value (50kHz?), WINDOW: 11 .. 1 Second measurement window
//		TxBuffer[2] = 0x79;			// POWR: 111 .. normal operation, SENS: 001 .. High Sensitivity
//		TxBuffer[3] = 0x00;         // ECH: 0 .. recharge not allowed, CHMODE: 00 .. recharging disabled
//		TxBuffer[4] = 0x32;         // MNREV: 0 .. no measurement IRQs , NIRQOC: 1 .. open collector, ENGATE: 0 .. window counts clk pulses
//		if (!spi_add_job( ChipSelectFlog, TxBuffer, 5, NULL, 0)) {
//			printf("SPI error 1!\n");
//			status = FLOG_STAT_IDLE;
//			return ;
//		}
//		status = FLOG_STAT_MEASSURE_CFG1;
//		delayTicks = 5;
//		break;
//
//	case FLOG_STAT_MEASSURE_CFG1:
//		if (!jobFinished) {
//			return;				// Wait until job finished
//		}
//		jobFinished = false;	// Clear the flag.
//		// next step: write TARGET(7:0)
//		TxBuffer[0] = 0x09 | 0x40 ; 	/* Write register  */
//		TxBuffer[1] = 0x2a;				// This target value was measured at device 'PRIMUS' on 14.1.2020 (ref Values were between 44128 and 44170 (dez) -> bit 17:10 -> 0x2B (44032 ...45055)
//			                            // after some playing around the ref value came down to 44000 . -> so I changed the bit 10 to 0 -> 0x2a;
//		if (!spi_add_job( ChipSelectFlog, TxBuffer, 2, NULL, 0)) {
//			printf("SPI error 2!\n");
//			status = FLOG_STAT_IDLE;
//			return ;
//		}
//		status = FLOG_STAT_MEASSURE_CFG2;
//		delayTicks = 5;
//		break;
//
//	case FLOG_STAT_MEASSURE_CFG2:
//		if (!jobFinished) {
//			return;				// Wait until job finished
//		}
//		jobFinished = false;	// Clear the flag.
//		// next step: enable the re-charging system
//		TxBuffer[0] = 0x0d | 0x40 ; /* Write register  */
//		TxBuffer[1] = 0x41;			// ECH: 1 ... recharge allowed, CHMOD: 01 .. automatic recharging mode
//		if (!spi_add_job( ChipSelectFlog, TxBuffer, 2, NULL, 0)) {
//			printf("SPI error 3!\n");
//			status = FLOG_STAT_IDLE;
//			return ;
//		}
//		status = FLOG_STAT_MEASSURE_CFGFINISHED;
//		delayTicks = 5;
//		break;
//
//	case FLOG_STAT_MEASSURE_CFGFINISHED:
//		if (!jobFinished) {
//			return;				// Wait until job finished
//		}
//		jobFinished = false;	// Clear the flag.
//		delayTicks = 20; 		// Lets wait
//		status = FLOG_STAT_MEASSURE_READDELAY;
//		break;
//
//	case FLOG_STAT_MEASSURE_READDELAY:
//		/* Now read all flash egister */
//		TxBuffer[0] = 0x00 | 0x80 ; /* Read registers from adr 0x00*/
//		if (!spi_add_job( ChipSelectFlog, TxBuffer, 1, RxBuffer, 20)) {
//			printf("SPI error 4!\n");
//			status = FLOG_STAT_IDLE;
//			return ;
//		}
//		status = FLOG_STAT_MEASSURE_READ;
//		break;
//
//	case FLOG_STAT_MEASSURE_READ:
//		if (!jobFinished) {
//			return;				// Wait until job finished
//		}
//		jobFinished = false;	// Clear the flag.
//		// Data was read. Make measurement Line
//		LogMeasurementResult();
//
//		valuesToMeassure--;
//		if (valuesToMeassure > 0) {
//			delayTicks = 20;
//			status = FLOG_STAT_MEASSURE_READDELAY;
//		} else {
//			// Power down the chip and goto idle mode
//			status = FLOG_STAT_IDLE;
//		}
//		break;
//
//	case FLOG_STAT_IDLE:
//	default:
//		break;
//	}
//}

void LogMeasurementResult() {
	// Log the raw data first
	printf("FGD-raw ; ");
	for(int i=0; i<20;i++) {
		printf("%02X ", RxBuffer[i]);
	}
	printf("\n");

	flog_registers_t reg;
	reg.refValue = -1;
	reg.sensorValue = -1;
	reg.newRefValue = RxBuffer[5] & 0x08;
	reg.refOverflow = RxBuffer[5] & 0x04;
	reg.newSensValue =  RxBuffer[8] & 0x08;
	reg.sensOverflow =  RxBuffer[8] & 0x04;
	reg.reCharges = RxBuffer[1] & 0x0F;
	reg.reChargeInProgress = RxBuffer[1] & 0x80;
	reg.reChargeRequested = RxBuffer[1] & 0x60;

	if (!reg.reChargeInProgress) {
		if (reg.newRefValue && !reg.refOverflow) {
			reg.refValue =  (((uint32_t)RxBuffer[5]) & 0x03) << 16 | ((uint32_t)RxBuffer[4])<<8 | ((uint32_t)RxBuffer[3]);
		}

		if (reg.newSensValue && !reg.sensOverflow) {
			reg.sensorValue =  (((uint32_t)RxBuffer[8]) & 0x03) << 16 | ((uint32_t)RxBuffer[7])<<8 | ((uint32_t)RxBuffer[6]);
		}
	}

	printf("FGD-measurement ; ");
	if (reg.reChargeInProgress) {
		printf("no data (recharging)\n");
	} else {
		printf("ref; %ld ; %s ; sensor; %ld ; %s \n", reg.refValue, (reg.refOverflow?"OV":""), reg.sensorValue, (reg.sensOverflow?"OV":"") );
	}
}




void InitializeAndMakeMessurement(uint8_t probeCount) {
	// First thing to do is Switch on Power and Clk to the FGD chip.
	valuesToMeassure = probeCount;
	Chip_Clock_SetCLKOUTSource(SYSCTL_CLKOUTSRC_RTC, 1);
	Chip_GPIO_SetPinState(LPC_GPIO, FLOGA_VCC_ENABLE_PORT, FLOGA_VCC_ENABLE_PIN, false);

	status = FLOG_STAT_MEASSURE_POWERON;
	delayTicks = 10;
}

bool ReadAllRegisters() {
	if (status == FLOG_STAT_IDLE) {
		/* Read flash ID register */
		TxBuffer[0] = 0x00 | 0x80 ; /* Read registers from adr 0x00*/

		// read all 20 registers at once
		if (spi_add_job( ChipSelectFlog, TxBuffer,1, RxBuffer, 20)) {
			status = FLOG_STAT_RX_JOB;
			return true;
		}
	}
	return false;	//error adding job
}

bool ConfigDevice1() {
	if (status == FLOG_STAT_IDLE) {
		// Step 1 in device config is to configure Mode (see Datasheet Page 8 - B.) and to disconnect the recharging system. (Page 8 - C.1)
		/* Write Config bytes 0x0B to 0x0E */
		TxBuffer[0] = 0x0B | 0x40 ; /* Write registers from adr 0x0B */

		TxBuffer[1] = 0x40;			// REF: 100 .. Referenz freq set to middle value (50kHz?), WINDOW: 11 .. 1 Second measurement window
		TxBuffer[2] = 0x79;			// POWR: 111 .. normal operation, SENS: 001 .. High Sensitivity
		TxBuffer[3] = 0x00;         // ECH: 0 .. recharge not allowed, CHMODE: 00 .. recharging disabled
		TxBuffer[4] = 0x32;         // MNREV: 0 .. no measurement IRQs , NIRQOC: 1 .. open collector, ENGATE: 0 .. window counts clk pulses

		if (spi_add_job( ChipSelectFlog, TxBuffer, 5, RxBuffer, 0)) {
			status = FLOG_STAT_TXCONFIG1_JOB;
			return true;
		}
	}
	return false;	//error adding job
}

