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

//	/* Achtung -> SSP Frequenz für read maximal 50MHz! */


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
#define FLASH_MAX_WRITE_SIZE	512

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
	FLASH_RET_WRONG_FLASHNR,

	FLASH_RET_READ_INITIALIZED					// new ,main loop state machine' returns....
} flash_ret;


typedef enum flash_status_e {
	FLASH_STAT_NOT_INITIALIZED,
	FLASH_STAT_IDLE,
	FLASH_STAT_RX_CHECKWIP,
	FLASH_STAT_RX_INPROGRESS,
	FLASH_STAT_TX_CHECKWIP,
	FLASH_STAT_TX_SETWRITEBIT,
	FLASH_STAT_TX_TRANSFER_INPROGRESS,
	FLASH_STAT_TX_WRITE_INPROGRESS,
	FLASH_STAT_TX_WRITE_INPROGRESS_DELAY,
	FLASH_STAT_TX_CLEAR_ERRORS,
	FLASH_STAT_ERROR							// TODO: what specific errors are there and what too do now ???? -> reinit SSP ???
} flash_status;

typedef struct flash_worker_s
{
	flash_status FlashStatus;
	uint8_t tx[5];
	uint8_t rx[1];
	uint8_t flash_dev;
	uint8_t *job_status;
	uint8_t *data;
	uint32_t len;
	uint32_t adr;
	uint8_t  busNr;
	int     CheckTxCounter;
	int		DelayCounter;
	void (*RxCallback)(uint8_t flashNr, uint32_t adr, uint8_t *data, uint32_t len);
	void (*TxCallback)(uint8_t rxtxResult, uint8_t flashNr, uint32_t adr, uint32_t len);
} flash_worker_t;


// prototypes
bool flash_init(uint8_t flashNr);
void FlashMainFor(uint8_t flashNr);

flash_ret flash_read(uint8_t flashNr, uint32_t adr, uint8_t *rx_data, uint32_t length, void (*callback)(uint8_t flashNr, uint32_t adr, uint8_t *data, uint32_t len));
void ReadFlashCmd(int argc, char *argv[]);
void ReadFlashFinished(uint8_t flashNr, uint32_t adr, uint8_t *data, uint32_t len);
bool ReadFlashAsync(uint8_t flashNr, uint32_t adr, uint32_t len,  void (*finishedHandler)(uint8_t flashNr, uint32_t adr, uint8_t *data, uint32_t len));

void WriteFlashCmd(int argc, char *argv[]);
void WriteFlashAsync(uint8_t flashNr, uint32_t adr, uint8_t *data, uint32_t len,  void (*finishedHandler)(uint8_t rxtxResult, uint8_t flashNr, uint32_t adr, uint32_t len));
void WriteFlashFinished(uint8_t rxtxResult, uint8_t flashNr, uint32_t adr, uint32_t len);

// local variables
flash_worker_t flashWorker[2];

void FlashInit() {
	ssp01_init();										// TODO: shouldn't each module init be called from main !?.....
	if (! flash_init(1)) {
		printf("Init Fehler für Flash1.\n");
	}
	if (! flash_init(2)) {
		printf("Init Fehler für Flash2.\n");
	}
	RegisterCommand("fRead", ReadFlashCmd);
	RegisterCommand("fWrite", WriteFlashCmd);
}

void ReadFlashCmd(int argc, char *argv[]) {
	if (argc != 3) {
		printf("uasge: cmd <mem> <adr> <len> where mem i one of 1, 2\n" );
		return;
	}

	// CLI params to binary params
	uint8_t flashNr = atoi(argv[0]);
	uint32_t adr = atoi(argv[1]);
	uint32_t len = atoi(argv[2]);
	if (len > FLASH_MAX_READ_SIZE) {
		len = FLASH_MAX_READ_SIZE;
	}

	// Binary Command
	if (! ReadFlashAsync(flashNr, adr, len,  ReadFlashFinished)) {
		printf("Not possible to initialize the flash read operation! (currently used?)\n");
	}
}

static uint8_t FlashReadData[FLASH_MAX_READ_SIZE+10];

bool ReadFlashAsync(uint8_t flashNr, uint32_t adr, uint32_t len, void (*finishedHandler)(uint8_t flashNr, uint32_t adr, uint8_t *data, uint32_t len)) {

	flash_ret ret =  flash_read(flashNr, adr, FlashReadData, len, finishedHandler);
//	if (ret == FLASH_RET_SUCCESS) {
//		finishedHandler(flashNr, adr, FlashReadData, len);
//		return true;
//	}
	return ret;
}

void ReadFlashFinished(uint8_t flashNr, uint32_t adr, uint8_t *data, uint32_t len) {
	printf("Flash %d read at %04X:\n", flashNr, adr);
	for (int i=0; i<len; i++ ) {
		printf("%02X ", ((uint8_t*)data)[i]);
		if ((i+1)%8 == 0) {
			printf("   ");
		}
	}
	printf("\n");
}

static uint8_t FlashWriteData[FLASH_MAX_WRITE_SIZE+10];
void WriteFlashCmd(int argc, char *argv[]) {
	if (argc != 4) {
		printf("uasge: cmd <flsh> <adr> <databyte> <len> where flsh is 1 or 2\n" );
		return;
	}

	// CLI params to binary params
	uint8_t flashNr = atoi(argv[0]);
	uint32_t adr = atoi(argv[1]);
	uint8_t byte = atoi(argv[2]);
	uint32_t len = atoi(argv[3]);
	if (len > FLASH_MAX_WRITE_SIZE) {
		len = FLASH_MAX_WRITE_SIZE;
	}

	for (int i=0;i<len;i++){
		FlashWriteData[i+5] = byte;		// keep first 5 bytes free for flash Write header.
	}

	// Binary Command
	WriteFlashAsync(flashNr, adr, FlashWriteData, len,  WriteFlashFinished);
}

void WriteFlashFinished(uint8_t rxtxResult, uint8_t flashNr, uint32_t adr, uint32_t len){
	if (rxtxResult == FLASH_RES_SUCCESS) {
		printf("%d bytes written to flash %d at %02X\n", len, flashNr, adr);
	} else {
		printf("Error while attemting to write to FlashNr %d: %d\n", flashNr, rxtxResult);
	}
}

void WriteFlashAsync(uint8_t flashNr, uint32_t adr, uint8_t *data, uint32_t len,  void (*finishedHandler)(uint8_t rxtxResult, uint8_t flashNr, uint32_t adr, uint32_t len)) {

	volatile bool *busyFlag;
	uint8_t busNr;
	flash_worker_t *worker;

	if (flashNr == 1) {
		//initializedFlag = &flash1_initialized;
		busyFlag = &flash1_busy;
		busNr = 1;
		worker = &flashWorker[0];
	} else if (flashNr == 2) {
		//initializedFlag = &flash2_initialized;
		busyFlag = &flash2_busy;
		busNr = 0;
		worker = &flashWorker[1];
	} else {
		finishedHandler(FLASH_RES_WRONG_FLASHNR, flashNr, adr, len);
		return;
	}

	if (worker->FlashStatus != FLASH_STAT_IDLE) {
//	if (! *initializedFlag)
//	{
		// TODO this also checks for all other busy states now  -> make own values for 'unitialized', 'Error', .... !?
		finishedHandler(FLASH_RES_BUSY, flashNr, adr, len);
	}

	if (data == NULL)
	{
		finishedHandler(FLASH_RES_DATA_PTR_INVALID, flashNr, adr, len);
	}

	if (len > FLASH_MAX_WRITE_SIZE)
	{
		finishedHandler(FLASH_RES_TX_OVERFLOW, flashNr, adr, len); // return FLASH_RET_TX_OVERFLOW;
	}

	if (adr < FLASH_DIE_SIZE)
	{
		if (flashNr == 2) {
			worker->flash_dev = SSPx_DEV_FLASH2_1;
		} else {
			worker->flash_dev = SSPx_DEV_FLASH1_1;
		}
	}
	else if (adr < FLASH_SIZE)
	{
		if (flashNr == 2) {
			worker->flash_dev = SSPx_DEV_FLASH2_2;
		} else {
			worker->flash_dev = SSPx_DEV_FLASH1_2;
		}
		adr = adr - FLASH_DIE_SIZE;
	}
	else
	{
		/* Sector address overrun  */
		finishedHandler(FLASH_RES_INVALID_ADR, flashNr, adr, len); //return FLASH_RET_INVALID_ADR;
	}

	/*--- Check WIP-Bit (Wait for previous write to complete) --- */
	worker->tx[0] = 0x05; /* 0x05 */
	worker->rx[0] = 0x00;

	*busyFlag = true;
	if (ssp_add_job(busNr, worker->flash_dev, worker->tx, 1, worker->rx, 1, NULL)) {
		/* Error while adding job */
		finishedHandler(FLASH_RES_JOB_ADD_ERROR, flashNr, adr, len); //return FLASH_RET_JOB_ADD_ERROR;
	}
	worker->data = data;
	worker->len = len;
	worker->adr = adr;
	worker->busNr = busNr;
	worker->TxCallback = finishedHandler;

	worker->FlashStatus = FLASH_STAT_TX_CHECKWIP;

}


bool flash_init(uint8_t flashNr)
{
	ssp_busnr_t busNr;
	uint8_t dev1Nr;
	uint8_t dev2Nr;
	flash_worker_t *worker;

	if (flashNr == 1) {
		busNr = SSP_BUS1;
		dev1Nr = SSPx_DEV_FLASH1_1;
		dev2Nr = SSPx_DEV_FLASH1_2;
		worker = &flashWorker[0];
	} else if (flashNr == 2) {
		busNr = SSP_BUS0;
		dev1Nr = SSPx_DEV_FLASH2_1;
		dev2Nr = SSPx_DEV_FLASH2_2;
		worker = &flashWorker[1];
	} else {
		printf("Not supported flash Number: %d", flashNr);
		return false;
	}

	worker->FlashStatus = FLASH_STAT_NOT_INITIALIZED;

	/* Init flash n and read ID register
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

	if (ssp_add_job(busNr , dev1Nr, tx, 1, rx, 1, &job_status))
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

	if (ssp_add_job(busNr, dev2Nr, tx, 1, rx, 1, &job_status))
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
		worker->FlashStatus = FLASH_STAT_ERROR;
		return false;
	}

	/* Everything ok */
	worker->FlashStatus = FLASH_STAT_IDLE;
	return true;
}


void FlashMain(void) {
	FlashMainFor(1);
	FlashMainFor(2);
}

void FlashMainFor(uint8_t flashNr) {
	volatile bool *busyFlag;
	//uint8_t busNr;
	flash_worker_t *worker;

	if (flashNr == 1) {
		busyFlag = &flash1_busy;
		//busNr = 1;
		worker = &flashWorker[0];
	} else if (flashNr == 2) {
		//initializedFlag = &flash2_initialized;
		busyFlag = &flash2_busy;
		//busNr = 0;
		worker = &flashWorker[1];
	} else {
		return;
	}

	if (*busyFlag) {
		// TODO: make TMO check(s) here !!!
	} else {
		// Der job für diesen Flash ist fertig
		switch (worker->FlashStatus) {
		case FLASH_STAT_RX_CHECKWIP: {
			// Der Job zum Lesen des Write in Progress flags ist fertig
			if (worker->rx[0] & 0x01) {
				// TODO Eigentlich sollte das hier nie passieren (Weil wir dieses Write busy beim Schreiben abwarten!),
				// aber wir könnten hier auch doch ein Zeit warten und das WIP neu lesen. ...
				worker->FlashStatus = FLASH_STAT_ERROR;
			}
			// Wir können jetzt das Lesen aktivieren
			/* Read Bytes */
			worker->tx[0] = 0x13; /* CMD fast read */
			worker->tx[1] = (worker->adr >> 24);
			worker->tx[2] = ((worker->adr & 0x00ff0000) >> 16);
			worker->tx[3] = ((worker->adr & 0x0000ff00) >> 8);
			worker->tx[4] = (worker->adr & 0x000000ff);

			*busyFlag = true;
			if (ssp_add_job(worker->busNr,worker->flash_dev, worker->tx, 5, worker->data, worker->len, &worker->job_status))
			{
				/* Error while adding job */
				worker->FlashStatus = FLASH_STAT_ERROR;
			}
			worker->FlashStatus = FLASH_STAT_RX_INPROGRESS;
			break;

		}
		case FLASH_STAT_RX_INPROGRESS: {
			// Read job is finished. Make Callback
			worker->FlashStatus = FLASH_STAT_IDLE;
			worker->RxCallback(flashNr,worker->adr, worker->data, worker->len);
			break;
		}

		case FLASH_STAT_TX_CHECKWIP: {
			// Der Job zum Lesen des Write in Progress flags ist fertig
			if (worker->rx[0] & 0x01) {
				// Fehler an Aufrufer melden und selbst in ERROR !?
				worker->FlashStatus = FLASH_STAT_ERROR;
				worker->TxCallback(FLASH_RES_WIPCHECK_ERROR,flashNr, worker->adr, worker->len);
				break;
			}

			/*--- Write Enable (WREN) --- */
			/* Set WREN bit to initiate write process */
			worker->tx[0] = 0x06; /* 0x06 WREN */

			*busyFlag = true;
			if (ssp_add_job(worker->busNr, worker->flash_dev, worker->tx, 1, NULL, 0, NULL))
			{
				worker->FlashStatus = FLASH_STAT_ERROR;
				//worker->TxCallback(FLASH_RES_JOB_ADD_ERROR,flashNr, worker->adr, worker->len);
				worker->TxCallback(FLASH_RES_JOB_ADD_ERROR,worker->CheckTxCounter, worker->adr, worker->len);

				break;
			}
			worker->FlashStatus = FLASH_STAT_TX_SETWRITEBIT;
			break;
		}

		case FLASH_STAT_TX_SETWRITEBIT: {
			/* write bit job finished */
			/*--- Write - Page Program --- */
			worker->data[0] = 0x12; /* 0x12 page program 4 byte address */
			worker->data[1] = (worker->adr >> 24);
			worker->data[2] = ((worker->adr & 0x00ff0000) >> 16);
			worker->data[3] = ((worker->adr & 0x0000ff00) >> 8);
			worker->data[4] = (worker->adr & 0x000000ff);

			*busyFlag = true;
			if (ssp_add_job(worker->busNr, worker->flash_dev, worker->data, (5 + worker->len), NULL, 0, &worker->job_status))
			{
				/* Error while adding job */
				worker->FlashStatus = FLASH_STAT_ERROR;
				worker->TxCallback(FLASH_RES_JOB_ADD_ERROR,flashNr, worker->adr, worker->len);
				break;
			}
			worker->FlashStatus = FLASH_STAT_TX_TRANSFER_INPROGRESS;
			break;
		}

		case FLASH_STAT_TX_TRANSFER_INPROGRESS: {
			/* tx data transfer job finished */
			/*--- Check WIP-Bit and Error bits --- */
			worker->tx[0] = 0x05; /* 0x05 */
			worker->CheckTxCounter = 25;				// We now wait 25 * 4 ms to get a cleared WIP bit and check for errors !?
			*busyFlag = true;
			if (ssp_add_job(worker->busNr, worker->flash_dev, worker->tx, 1, worker->rx, 1, NULL)) {
				/* Error while adding job */
				worker->FlashStatus = FLASH_STAT_ERROR;
				worker->TxCallback(FLASH_RES_JOB_ADD_ERROR,flashNr, worker->adr, worker->len);
				break;
			}
			worker->FlashStatus = FLASH_STAT_TX_WRITE_INPROGRESS;
			break;
		}


		case FLASH_STAT_TX_WRITE_INPROGRESS: {
			/* WIP & Error bits read job finished */
			if (worker->rx[0] & 0x01) {
				// Write process still ongoing
				// Delay for 4ms and repeating WIP job
				worker->DelayCounter = 1;		// at this moment with i=1 we count up to 25 mainloops. We get a Write finished after 3 mainloops each
												// one delaying the next WIP read job (takes aprx. 34 us) for aprx. 12 us only. So idle state is reached after
												// aprx. 130/140 us  ( 3 * ( 12 + 34 )! Increasing this value here would slow down polling but increase the
												// timeout we wait here ( 25*delay ).
				worker->FlashStatus = FLASH_STAT_TX_WRITE_INPROGRESS_DELAY;
			} else {
				// Write process over, check for error bits
				if (worker->rx[0] & (1 << 6)) /* PERR-Bit �berpr�fen (Bit 6 im Status Register) */
				{
					/* Error during write process */
					/*--- Clear status register --- */
					worker->tx[0] = 0x30;
					*busyFlag = true;
					if (ssp_add_job(worker->busNr, worker->flash_dev, worker->tx, 1, NULL, 0, NULL)) {
						/* Error while adding job */
						worker->FlashStatus = FLASH_STAT_ERROR;
						worker->TxCallback(FLASH_RES_JOB_ADD_ERROR,flashNr, worker->adr, worker->len);
						break;
					}
					worker->FlashStatus = FLASH_STAT_TX_CLEAR_ERRORS;
					worker->TxCallback(FLASH_RES_TX_ERROR,flashNr, worker->adr, worker->len);
				} else {
					// Everything worked out fine -> callback with Success
					worker->FlashStatus = FLASH_STAT_IDLE;
					worker->TxCallback(FLASH_RES_SUCCESS,flashNr, worker->adr, worker->len);
				}
			}
			break;
		}

		case FLASH_STAT_TX_WRITE_INPROGRESS_DELAY: {
			worker->DelayCounter--;
			if (worker->DelayCounter <= 0) {
				// its time to make a new WIP read job
				/*--- Check WIP-Bit and Error bits --- */

				worker->tx[0] = 0x05; /* 0x05 */
				worker->CheckTxCounter--;
				if (worker->CheckTxCounter <= 0) {
					worker->FlashStatus = FLASH_STAT_ERROR;
					worker->TxCallback(FLASH_RES_TX_WRITE_TOO_LONG,flashNr, worker->adr, worker->len);
				} else {
					*busyFlag = true;
					if (ssp_add_job(worker->busNr, worker->flash_dev, worker->tx, 1, worker->rx, 1, NULL)) {
						/* Error while adding job */
						worker->FlashStatus = FLASH_STAT_ERROR;
						worker->TxCallback(FLASH_RES_JOB_ADD_ERROR,flashNr, worker->adr, worker->len);
						break;
					}
					worker->FlashStatus = FLASH_STAT_TX_WRITE_INPROGRESS;
				}
			}
			break;
		}

		case FLASH_STAT_TX_CLEAR_ERRORS: {
			// Clear errors job done
			worker->FlashStatus = FLASH_STAT_IDLE;
			break;
		}

		case FLASH_STAT_IDLE:
		default:
			// nothing to do in main loop.
			break;
		} // end switch

	}
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


flash_ret flash_read(uint8_t flashNr, uint32_t adr, uint8_t *rx_data, uint32_t length, void (*callback)(uint8_t flashNr, uint32_t adr, uint8_t *data, uint32_t len))
{

	//bool *initializedFlag;
	volatile bool *busyFlag;
	uint8_t busNr;
	flash_worker_t *worker;

	if (flashNr == 1) {
		//initializedFlag = &flash1_initialized;
		busyFlag = &flash1_busy;
		busNr = 1;
		worker = &flashWorker[0];
	} else if (flashNr == 2) {
		//initializedFlag = &flash2_initialized;
		busyFlag = &flash2_busy;
		busNr = 0;
		worker = &flashWorker[1];
	} else {
		return FLASH_RET_WRONG_FLASHNR;
	}

	if (worker->FlashStatus != FLASH_STAT_IDLE) {
//	if (! *initializedFlag)
//	{
		// Flash was not initialized correctly		// TODO this also checks for all other busy states now  -> ret val ok????
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
			worker->flash_dev = SSPx_DEV_FLASH2_1;
		} else {
			worker->flash_dev = SSPx_DEV_FLASH1_1;
		}
	}
	else if (adr < FLASH_SIZE)
	{
		if (flashNr == 2) {
			worker->flash_dev = SSPx_DEV_FLASH2_2;
		} else {
			worker->flash_dev = SSPx_DEV_FLASH1_2;
		}
		adr = adr - FLASH_DIE_SIZE;
	}
	else
	{
		/* Sector address overrun  */
		return FLASH_RET_INVALID_ADR;
	}

	/*--- Check WIP-Bit (Wait for previous write to complete) --- */
	worker->tx[0] = 0x05; /* 0x05 */
	worker->rx[0] = 0x00;

	*busyFlag = true;
	if (ssp_add_job(busNr, worker->flash_dev, worker->tx, 1, worker->rx, 1, NULL)) {
		/* Error while adding job */
		return FLASH_RET_JOB_ADD_ERROR;
	}
	worker->data = rx_data;
	worker->len = length;
	worker->adr = adr;
	worker->busNr = busNr;
	worker->RxCallback = callback;

#if false
	//  Idee: Wir warten hier einmal nur kurz, falls alles ok ist, dann können wir ohne mainloop jitter weitermachen.
	//       nach dem read byte vergehen im normalfall nur ca. 12 us bis das busy flag wieder gelöscht wird.
	//       Derzeit (2019-12-25) gewinnen wir aber nur ca. 8us gegenüber dem Mainloop wait. Vieleicht ist dieser code aber bessser, wenn unser mainloop
	//       später einmal mehr code enthält und dieser jitter deshalb um einiges größer wird als die (derzeitigen ca. 10us).
	uint32_t i = 0;
	while (i++ < 500 ); // && (*busyFlag == true));	// Wir warten hier maximal ca. 60us, ob das busy flag ausgeht.

	if (i>=500) {
		// Wenn nicht, überlassen wir den nächsten Schritt unserem Main loop.
		worker->FlashStatus = FLASH_STAT_RX_CHECKWIP;
	} else {
		// Wir checken in der  Antwort, ob der Chip Lesebereit ist und setzen den Lesejob auf.
		if (worker->rx[0] & 0x01) {
			// TODO Eigentlich sollte das hier nie passieren (Weil wir dieses Write busy beim Schreiben abwarten!),
			// aber wir könnten hier auch doch ein Zeit warten und das WIP neu lesen. ...
			worker->FlashStatus = FLASH_STAT_ERROR;
		}
		// Wir können jetzt das Lesen aktivieren
		/* Read Bytes */
		worker->tx[0] = 0x13; /* CMD fast read */
		worker->tx[1] = (worker->adr >> 24);
		worker->tx[2] = ((worker->adr & 0x00ff0000) >> 16);
		worker->tx[3] = ((worker->adr & 0x0000ff00) >> 8);
		worker->tx[4] = (worker->adr & 0x000000ff);

		*busyFlag = true;
		if (ssp_add_job(worker->busNr,worker->flash_dev, worker->tx, 5, worker->data, worker->len, &worker->job_status))
		{
			/* Error while adding job */
			worker->FlashStatus = FLASH_STAT_ERROR;
		}
		worker->FlashStatus = FLASH_STAT_RX_INPROGRESS;
		// Der Main loop wartet auf das Ende dieses Jobs....
	}
#else
	// wir überlassen den nächsten Schritt unserem Main loop.
	worker->FlashStatus = FLASH_STAT_RX_CHECKWIP;
#endif
	return FLASH_RET_READ_INITIALIZED;		// Wenn es länger dauert wird das weitere (incl TMO im mainloop gesteuert)
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



