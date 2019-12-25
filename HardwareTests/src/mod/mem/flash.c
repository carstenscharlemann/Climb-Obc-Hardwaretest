/*
 * flash.c
 *
 *  Created on: 20.12.2019
 *
 */
/* 2x S25FL512
 * 1 Sector = 256kByte
 * Sector erase time = 520ms
 * Page Programming Time = 340us
 * Page size = 512Byte
 * One time programmable memory 1024Byte
 * Program and erase suspend possible
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "../../layer1/SSP/obc_ssp.h"

#include "flash.h"
#include "../cli/cli.h"
#include "../tim/timer.h"

/* Whole flash */
#define FLASH_SIZE 			134217728 	/* Bytes (2 * 2^26)*/
#define FLASH_PAGE_SIZE		((uint32_t) 512)			/* Bytes */
#define FLASH_SECTOR_SIZE	262144		/* Bytes (2^18) */
#define FLASH_PAGE_NUMBER 	262144
#define FLASH_SECTOR_NUMBER	512

/* Single DIE only */
#define FLASH_DIE_SIZE			67108864	/* Bytes (2^26)*/
#define FLASH_DIE_SECTOR_NUMBER 256
#define FLASH_DIE_PAGE_NUMBER 	131072


#define FLASH_MAX_READ_SIZE		512


typedef enum flash_ret_e
{
	FLASH_RET_SUCCESS = 0,
	FLASH_RET_INIT_ERROR,
	FLASH_RET_SEMAPHORE_TIMEOUT,
	FLASH_RET_WRITE_STILL_ACTIVE,
	FLASH_RET_WRITE_ERROR,
	FLASH_RET_ERASE_ERROR,
	FLASH_RET_TX_OVERFLOW,
	FLASH_RET_DATA_PTR_INVALID,
	FLASH_RET_RX_LEN_OVERFLOW,
	FLASH_RET_INVALID_ADR,
	FLASH_RET_INVALID_SECTOR,
	FLASH_RET_SEMAPHORE_CREATE_ERROR,
	FLASH_RET_JOB_ADD_ERROR,
	FLASH_RET_WRONG_FLASHNR
} flash_ret;


// prototypes
bool flash_init(void);
flash_ret flash_read(uint8_t flashNr, uint32_t adr, uint8_t *rx_data, uint32_t length);
void ReadFlashPageCmd(int argc, char *argv[]);
void ReadFlashFinished(uint8_t flashNr, uint16_t adr, uint8_t *data, uint16_t len);
bool ReadFlashPageAsync(uint8_t flashNr, uint16_t adr, uint16_t len,  void (*finishedHandler)(uint8_t flashNr, uint16_t adr, uint8_t *data, uint16_t len));

// local variables
bool flash1_initialized = false;
bool flash2_initialized = false;


//SemaphoreHandle_t flash2_semaphore;


void FlashInit() {
	flash1_initialized = false;
	flash2_initialized = false;

	if (! flash_init()) {
		printf("Init Fehler für Flash.\n");
		return;
	}
	RegisterCommand("fRead", ReadFlashPageCmd);
}


void ReadFlashPageCmd(int argc, char *argv[]) {
	if (argc != 3) {
		printf("uasge: cmd <mem> <adr> <len> where mem i one of 1, 2\n" );
		return;
	}

	// CLI params to binary params
	uint8_t flashNr = atoi(argv[0]);
	uint16_t adr = atoi(argv[1]);
	uint16_t len = atoi(argv[2]);
	if (len > FLASH_MAX_READ_SIZE) {
		len = FLASH_MAX_READ_SIZE;
	}

	// Binary Command
	if (! ReadFlashPageAsync(flashNr, adr, len,  ReadFlashFinished)) {
		printf("Not possible to initialize the flash read operation! (currently used?)\n");
	}
}

static uint8_t FlashReadData[FLASH_MAX_READ_SIZE+10];

bool ReadFlashPageAsync(uint8_t flashNr, uint16_t adr, uint16_t len, void (*finishedHandler)(uint8_t flashNr, uint16_t adr, uint8_t *data, uint16_t len)) {

	flash_ret ret =  flash_read(flashNr, adr, FlashReadData, len);
	if (ret == FLASH_RET_SUCCESS) {
		finishedHandler(flashNr, adr, FlashReadData, len);
		return true;
	}
	return false;

}

void ReadFlashFinished(uint8_t flashNr, uint16_t adr, uint8_t *data, uint16_t len) {
	printf("Flash %d read at %04X:\n", flashNr, adr);
	for (int i=0; i<len; i++ ) {
		printf("%02X ", ((uint8_t*)data)[i]);
		if ((i+1)%8 == 0) {
			printf("   ");
		}
	}
	printf("\n");
}

bool flash_init(void)
{
	ssp01_init();

	/* Init flash 1 and read ID register
	 * Parameters: none
	 * Return value: 0 in case of success, != 0 in case of error
	 */
	uint8_t tx[1];
	uint8_t rx[1];
	uint8_t *job_status = NULL;
	volatile uint32_t helper;
	/* Read flash ID register */
	tx[0] = 0x9F; /* 0x9F */
	rx[0] = 0x00;

	if (ssp_add_job(SSP_BUS1 , SSPx_DEV_FLASH1_1, tx, 1, rx, 1, &job_status))
	{
		/* Error while adding job */
		//return FLASH_RET_JOB_ADD_ERROR;
		return false;
	}
	helper = 0;
	while ((*job_status != SSP_JOB_STATE_DONE) && (helper < 1000000))
	{
		/* Wait for job to finish */
		helper++;
	}

	if (rx[0] != 0x01)
	{
		/* Error - Flash could not be accessed */
		//obc_status.flash2_initialized = 0;
		//return FLASH_RET_INIT_ERROR;
		return false;
	}

	/* Read flash ID register */
	tx[0] = 0x9F; /* 0x9F */
	rx[0] = 0x00;

	if (ssp_add_job(SSP_BUS1, SSPx_DEV_FLASH1_2, tx, 1, rx, 1, &job_status))
	{
		/* Error while adding job */
		//return FLASH_RET_JOB_ADD_ERROR;
		return false;
	}

	helper = 0;
	while ((*job_status != SSP_JOB_STATE_DONE) && (helper < 1000000))
	{
		/* Wait for job to finish */
		helper++;
	}

	if (rx[0] != 0x01)
	{
		/* Error - Flash could not be accessed */
//		obc_status.flash2_initialized = 0;
//		return FLASH_RET_INIT_ERROR;
		return false;
	}

	/* Everything ok */
	flash1_initialized = true;



	/* Read flash ID register */
	tx[0] = 0x9F; /* 0x9F */
	rx[0] = 0x00;

	if (ssp_add_job(SSP_BUS0 , SSPx_DEV_FLASH2_1, tx, 1, rx, 1, &job_status))
	{
		/* Error while adding job */
		//return FLASH_RET_JOB_ADD_ERROR;
		return false;
	}
	helper = 0;
	while ((*job_status != SSP_JOB_STATE_DONE) && (helper < 1000000))
	{
		/* Wait for job to finish */
		helper++;
	}

	if (rx[0] != 0x01)
	{
		/* Error - Flash could not be accessed */
		//obc_status.flash2_initialized = 0;
		//return FLASH_RET_INIT_ERROR;
		return false;
	}

	/* Read flash ID register */
	tx[0] = 0x9F; /* 0x9F */
	rx[0] = 0x00;

	if (ssp_add_job(SSP_BUS0, SSPx_DEV_FLASH2_2, tx, 1, rx, 1, &job_status))
	{
		/* Error while adding job */
		//return FLASH_RET_JOB_ADD_ERROR;
		return false;
	}

	helper = 0;
	while ((*job_status != SSP_JOB_STATE_DONE) && (helper < 1000000))
	{
		/* Wait for job to finish */
		helper++;
	}

	if (rx[0] != 0x01)
	{
		/* Error - Flash could not be accessed */
//		obc_status.flash2_initialized = 0;
//		return FLASH_RET_INIT_ERROR;
		return false;
	}

	/* Everything ok */
	flash2_initialized = true;

//#if EXTENDED_DEBUG_MESSAGES
//	debug_transmit("OBC: Flash 2 initialied\n",0);
//#endif
	return true;
}
//
//flash_ret flash2_write(uint32_t adr, uint8_t *tx_data, uint32_t len)
//{
//	/*
//	 *
//	 * Parameters:
//	 *
//	 *
//	 */
//	uint8_t rx[1];
//	uint8_t tx[1];
//	uint8_t flash_dev = SSP0_DEV_FLASH2_1;
//	uint8_t *job_status = NULL;
//	volatile int i;
//
//	if (obc_status.flash2_initialized == 0)
//	{
//		// Flash was not initialized correctly
//		return FLASH_RET_INIT_ERROR;
//	}
//
//	if (len > 512)
//	{
//		return FLASH_RET_TX_OVERFLOW;
//	}
//
//	if (tx_data == NULL)
//	{
//		return FLASH_RET_DATA_PTR_INVALID;
//	}
//
//	/* Address decoding */
//
//	if (adr < FLASH_DIE_SIZE)
//	{
//		/* Flash2 die 1 */
//		flash_dev = SSP0_DEV_FLASH2_1;
//	}
//	else if (adr < FLASH_SIZE)
//	{
//		/* Flash2 die 2 */
//		flash_dev = SSP0_DEV_FLASH2_2;
//		adr = adr - FLASH_DIE_SIZE;
//	}
//	else
//	{
//		/* Page number out of range  */
//		return FLASH_RET_INVALID_ADR;
//	}
//
//	/*--- Check WIP-Bit (Wait for previous write to complete) --- */
//	tx[0] = 0x05; /* 0x05 */
//	rx[0] = 0x00;
//
//	xSemaphoreTake(flash2_semaphore, (TickType_t) 0); /* Free semaphore */
//
//	if (ssp0_add_job(flash_dev, tx, 1, rx, 1, &job_status))
//	{
//		/* Error while adding job */
//		return FLASH_RET_JOB_ADD_ERROR;
//	}
//
//	if (xSemaphoreTake(flash2_semaphore, (TickType_t) 250) == pdFALSE)
//	{
//		/* Semaphore was not given in the specified time interval - rx data is not valid */
//		return FLASH_RET_SEMAPHORE_TIMEOUT;
//	}
//
//	if (rx[0] & 0x01)
//	{
//		return FLASH_RET_WRITE_STILL_ACTIVE;
//	}
//
//	/*--- Write Enable (WREN) --- */
//	/* Set WREN bit to initiate write process */
//	tx[0] = 0x06; /* 0x06 WREN */
//	if (ssp0_add_job(flash_dev, tx, 1, NULL, 0, NULL))
//	{
//		/* Error while adding job */
//		return FLASH_RET_JOB_ADD_ERROR;
//	}
//	xSemaphoreTake(flash2_semaphore, (TickType_t) 10);
//
//	/*--- Write - Page Program --- */
//	tx_data[0] = 0x12; /* 0x12 page program 4 byte address */
//	tx_data[1] = (adr >> 24);
//	tx_data[2] = ((adr & 0x00ff0000) >> 16);
//	tx_data[3] = ((adr & 0x0000ff00) >> 8);
//	tx_data[4] = (adr & 0x000000ff);
//
//	if (ssp0_add_job(flash_dev, tx_data, (5 + len), NULL, 0, &job_status))
//	{
//		/* Error while adding job */
//		return FLASH_RET_JOB_ADD_ERROR;
//	}
//
//	/*--- Check WIP-Bit and Error bits --- */
//	tx[0] = 0x05; /* 0x05 */
//
//	xSemaphoreTake(flash2_semaphore, (TickType_t) 10);
//	i = 0;
//	while ((*job_status != SSP_JOB_STATE_DONE) && (i < 1000))
//	{
//		xSemaphoreTake(flash2_semaphore, (TickType_t) 1);
//		/* Wait for job to finish */
//		i++;
//	}
//
//	i = 0;
//	do
//	{
//		//vTaskDelay(WAIT_MS(4));
//
//		if (ssp0_add_job(flash_dev, tx, 1, rx, 1, NULL))
//		{
//			/* Error while adding job */
//			return FLASH_RET_JOB_ADD_ERROR;
//		}
//		i++;
//		/* Wait for job to be executed and check result afterwards */
//		xSemaphoreTake(flash2_semaphore, (TickType_t) 10);
//
//	} while ((rx[0] & 0x01) && (i < 25));
//
//	if (rx[0] & 0x01)
//	{
//		return FLASH_RET_WRITE_STILL_ACTIVE;
//		/* write process takes unusually long */
//	}
//
//	if (rx[0] & (1 << 6)) /* PERR-Bit �berpr�fen (Bit 6 im Status Register) */
//	{
//		/* Error during write process */
//
//		/*--- Clear status register --- */
//		tx[0] = 0x30;
//		if (ssp0_add_job(flash_dev, tx, 1, NULL, 0, NULL))
//		{
//			/* Error while adding job */
//			return FLASH_RET_JOB_ADD_ERROR;
//		}
//
//		return FLASH_RET_WRITE_ERROR;
//	}
//
//	return FLASH_RET_SUCCESS;
//}
//


flash_ret flash_read(uint8_t flashNr, uint32_t adr, uint8_t *rx_data, uint32_t length)
{
	uint8_t tx[5];
	uint8_t rx[1];
	uint8_t flash_dev;
	uint8_t *job_status;
	/* Achtung -> SSP Frequenz für read maximal 50MHz! */

	bool *initializedFlag;
	volatile bool *busyFlag;
	uint8_t busNr;

	if (flashNr == 1) {
		initializedFlag = &flash1_initialized;
		busyFlag = &flash1_busy;
		busNr = 1;
	} else if (flashNr == 2) {
		initializedFlag = &flash2_initialized;
		busyFlag = &flash2_busy;
		busNr = 0;
	} else {
		return FLASH_RET_WRONG_FLASHNR;
	}

	if (! *initializedFlag)
	{
		// Flash was not initialized correctly
		return FLASH_RET_INIT_ERROR;
	}

	if (rx_data == NULL)
	{
		return FLASH_RET_DATA_PTR_INVALID;
	}

	if (length > FLASH_DIE_SIZE)
	{
		return FLASH_RET_RX_LEN_OVERFLOW;
	}

	if (adr < FLASH_DIE_SIZE)
	{
		if (flashNr == 2) {
			flash_dev = SSPx_DEV_FLASH2_1;
		} else {
			flash_dev = SSPx_DEV_FLASH1_1;
		}
	}
	else if (adr < FLASH_SIZE)
	{
		if (flashNr == 2) {
			flash_dev = SSPx_DEV_FLASH2_2;
		} else {
			flash_dev = SSPx_DEV_FLASH1_2;
		}
		adr = adr - FLASH_DIE_SIZE;
	}
	else
	{
		/* Sector address overrun  */
		return FLASH_RET_INVALID_ADR;
	}

	/*--- Check WIP-Bit (Wait for previous write to complete) --- */
	tx[0] = 0x05; /* 0x05 */
	rx[0] = 0x00;


	//xSemaphoreTake(flash2_semaphore, (TickType_t) 0); /* Free semaphore */
	if (! *busyFlag) {
		*busyFlag = true;

		if (ssp_add_job(busNr, flash_dev, tx, 1, rx, 1, NULL)) {
				/* Error while adding job */
				return FLASH_RET_JOB_ADD_ERROR;
		}

		// Wir wartebn bis zu 250 ms !????! bis das busy ausgeht -> TODO make mainloop state engine / make timing between jobs and flash protocoll....
		if (!TimWaitForFalseMs(busyFlag, 250)) {
			return FLASH_RET_SEMAPHORE_TIMEOUT;
		}

		if (rx[0] & 0x01)
		{
			return FLASH_RET_WRITE_STILL_ACTIVE;
		}

		/* Read Bytes */
		tx[0] = 0x13; /* CMD fast read */
		tx[1] = (adr >> 24);
		tx[2] = ((adr & 0x00ff0000) >> 16);
		tx[3] = ((adr & 0x0000ff00) >> 8);
		tx[4] = (adr & 0x000000ff);

		//xSemaphoreTake(flash2_semaphore, (TickType_t) 0); /* Free semaphore */
		*busyFlag = true;
		if (ssp_add_job(busNr, flash_dev, tx, 5, rx_data, length, &job_status))
		{
			/* Error while adding job */
			return FLASH_RET_JOB_ADD_ERROR;
		}

//		if (xSemaphoreTake(flash2_semaphore, (TickType_t) 500) == pdFALSE)
//		{
//			/* Semaphore was not given in the specified time intervall - rx data is not valid */
//			return FLASH_RET_SEMAPHORE_TIMEOUT;
//		}
		if (!TimWaitForFalseMs(busyFlag, (uint8_t)500)) {
			return FLASH_RET_SEMAPHORE_TIMEOUT;
		}

		if (*job_status != SSP_JOB_STATE_DONE)
		{
			return FLASH_RET_SEMAPHORE_TIMEOUT;
		}

		return FLASH_RET_SUCCESS;
	}

	return FLASH_RET_SEMAPHORE_CREATE_ERROR;		// TODO: heißt eigentlich 'flash2 ist gerade busy....
}
//
//flash_ret flash2_sektor_erase(uint32_t adr)
//{
//	volatile uint32_t i;
//	uint8_t tx[5];
//	uint8_t rx[1];
//	uint8_t flash_dev;
//	uint8_t *job_status = NULL;
//
//	if (obc_status.flash2_initialized == 0)
//	{
//		// Flash was not initialized correctly
//		return FLASH_RET_INIT_ERROR;
//	}
//
//	if (adr < FLASH_DIE_SIZE)
//	{
//		flash_dev = SSP0_DEV_FLASH2_1;
//	}
//	else if (adr < FLASH_SIZE)
//	{
//		flash_dev = SSP0_DEV_FLASH2_2;
//		adr = adr - FLASH_DIE_SIZE;
//	}
//	else
//	{
//		/* Sector number does not exist */
//		return FLASH_RET_INVALID_SECTOR;
//	}
//
//	/*--- Check WIP-Bit (Wait for previous write to complete) --- */
//	tx[0] = 0x05; /* 0x05 */
//	rx[0] = 0x00;
//
//	if (ssp0_add_job(flash_dev, tx, 1, rx, 1, NULL))
//	{
//		/* Error while adding job */
//		return FLASH_RET_JOB_ADD_ERROR;
//	}
//
//	if (xSemaphoreTake(flash2_semaphore, (TickType_t) 100) == pdFALSE)
//	{
//		/* Semaphore was not given in the specified time intervall - rx data is not valid */
//		return FLASH_RET_SEMAPHORE_TIMEOUT;
//	}
//
//	if (rx[0] & 0x01)
//	{
//		return FLASH_RET_WRITE_STILL_ACTIVE;
//	}
//
//	/*--- Write Enable (WREN) --- */
//	/* Set WREN bit to initiate write process */
//	tx[0] = 0x06; /* 0x06 WREN */
//	if (ssp0_add_job(flash_dev, tx, 1, NULL, 0, NULL))
//	{
//		/* Error while adding job */
//		return FLASH_RET_JOB_ADD_ERROR;
//	}
//	xSemaphoreTake(flash2_semaphore, (TickType_t) 10);
//
//	/*--- Write - Page Program --- */
//	tx[0] = 0xDC; /* 0xDC sector erase, 4 byte address */
//	tx[1] = (adr >> 24);
//	tx[2] = ((adr & 0x00ff0000) >> 16);
//	tx[3] = ((adr & 0x0000ff00) >> 8);
//	tx[4] = (adr & 0x000000ff);
//
//	if (ssp0_add_job(flash_dev, tx, 5, NULL, 0, &job_status))
//	{
//		/* Error while adding job */
//		return FLASH_RET_JOB_ADD_ERROR;
//	}
//
//	xSemaphoreTake(flash2_semaphore, (TickType_t) 80);
//	i = 0;
//	while ((*job_status != SSP_JOB_STATE_DONE) && (i < 500))
//	{
//		xSemaphoreTake(flash2_semaphore, (TickType_t) 5);
//		/* Wait for job to finish */
//		i++;
//	}
//
//	/*--- Check WIP-Bit and Error bits --- */
//	tx[0] = 0x05; /* 0x05 */
//
//	i = 0;
//
//	do
//	{
//		vTaskDelay(WAIT_MS(50)); /* Timeout size? */
//		if (ssp0_add_job(flash_dev, tx, 1, rx, 1, &job_status))
//		{
//			/* Error while adding job */
//			return FLASH_RET_JOB_ADD_ERROR;
//		}
//
//		i++;
//		/* Wait for job to be executed and check result afterwards */
//		xSemaphoreTake(flash2_semaphore, (TickType_t) 80);
//
//	} while (((rx[0] & 0x01)) && (i < 20));
//
//	if (rx[0] & 0x01)
//	{
//		return FLASH_RET_WRITE_STILL_ACTIVE;
//		/* write process takes unusually long */
//	}
//
//	if (rx[0] & 0x20) /* EERR-Bit �berpr�fen (Bit 5 im Status Register) */
//	{
//		/* Error during write process */
//
//		/*--- Clear status register --- */
//		tx[0] = 0x30;
//		if (ssp0_add_job(flash_dev, tx, 1, NULL, 0, NULL))
//		{
//			/* Error while adding job */
//			return FLASH_RET_JOB_ADD_ERROR;
//		}
//
//		return FLASH_RET_ERASE_ERROR;
//	}
//
//	return FLASH_RET_SUCCESS;
//}



