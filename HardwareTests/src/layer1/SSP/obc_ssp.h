/*
 * stc_spi.h
 *
 *  Created on: 07.10.2012
 *      Author: Andi
 */

#ifndef STC_SPI_H_
#define STC_SPI_H_

#include <stdint.h>

#include <chip.h>
//#include "LPC17xx.h"
//#include "lpc17xx_gpio.h"
//#include "lpc17xx_pinsel.h"

/* Max SPI buffer length */
#define SPI_MAX_JOBS 16

//enum
//{ /* SSP1 sensors */
//	SSP1_DEV_FLASH1_1, SSP1_DEV_FLASH1_2, SSP1_DEV_MPU
//};
//
//enum
//{
//	SSP0_DEV_FLASH2_1, SSP0_DEV_FLASH2_2
//};

enum
{
	SSPx_DEV_FLASH1_1, SSPx_DEV_FLASH1_2, SSPx_DEV_FLASH2_1, SSPx_DEV_FLASH2_2
};


enum
{
	SSP_BUS0, SSP_BUS1
};


enum
{
	SSP_JOB_STATE_DONE = 0, SSP_JOB_STATE_PENDING, SSP_JOB_STATE_ACTIVE, SSP_JOB_STATE_SSP_ERROR, SSP_JOB_STATE_DEVICE_ERROR
};

enum
{ /* Return values for ssp_add_job */
	SSP_JOB_ADDED = 0, SSP_JOB_BUFFER_OVERFLOW, SSP_JOB_MALLOC_FAILED, SSP_JOB_ERROR, SSP_JOB_NOT_INITIALIZED, SSP_WRONG_BUSNR
};

typedef struct ssp_job_s
{
	uint8_t *array_to_send;
	uint16_t bytes_to_send;
	uint16_t bytes_sent;
	uint16_t bytes_to_read;
	uint16_t bytes_read;
	uint8_t *array_to_read;
	uint8_t device;
	uint8_t status;
	uint8_t dir;
} volatile ssp_job_t;

typedef struct ssp_jobs_s
{
	ssp_job_t job[SPI_MAX_JOBS];
	uint8_t current_job;
	uint8_t last_job_added;
	uint8_t jobs_pending;
} volatile ssp_jobs_t;

typedef struct ssp_status_s
{
	/* OBC status bits block 1 - 32 Bits */
	unsigned int ssp_interrupt_ror :1; /* Bit 0 */
	unsigned int ssp_interrupt_unknown_device :1; /* Bit 1 */
	unsigned int ssp_buffer_overflow :1; /* Bit 2 */
	unsigned int ssp_frequent_errors :1; /* Bit 3 */
	unsigned int :1; /* Bit 4 */
	unsigned int :1; /* Bit 5 */
	unsigned int :1; /* Bit 6 */
	unsigned int :1; /* Bit 7 */
	unsigned int :1; /* Bit 8 */
	unsigned int :1; /* Bit 9 */
	unsigned int :1; /* Bit 10 */
	unsigned int :1; /* Bit 11 */
	unsigned int :1; /* Bit 12 */
	unsigned int :1; /* Bit 13 */
	unsigned int :1; /* Bit 14 */
	unsigned int :1; /* Bit 15 */
	unsigned int :1; /* Bit 16 */
	unsigned int :1; /* Bit 17 */
	unsigned int :1; /* Bit 18 */
	unsigned int :1; /* Bit 19 */
	unsigned int :1; /* Bit 20 */
	unsigned int :1; /* Bit 21 */
	unsigned int :1; /* Bit 22 */
	unsigned int :1; /* Bit 23 */
	unsigned int :1; /* Bit 24 */
	unsigned int :1; /* Bit 25 */
	unsigned int :1; /* Bit 26 */
	unsigned int :1; /* Bit 27 */
	unsigned int :1; /* Bit 28 */
	unsigned int :1; /* Bit 29 */
	unsigned int :1; /* Bit 30 */
	unsigned int ssp_initialized:1;    /* Bit 31 */

	uint8_t ssp_error_counter;
}
volatile ssp_status_t;

extern volatile bool flash1_busy;		// temp 'ersatz' für semaphor
extern volatile bool flash2_busy;		// temp 'ersatz' für semaphor

void ssp01_init(void);
//uint32_t ssp1_add_job(uint8_t sensor, uint8_t *array_to_send, uint16_t bytes_to_send, uint8_t *array_to_store, uint16_t bytes_to_read,
//		uint8_t **job_status);

uint32_t ssp_add_job(uint8_t busNr, uint8_t sensor, uint8_t *array_to_send, uint16_t bytes_to_send, uint8_t *array_to_store, uint16_t bytes_to_read,
		uint8_t **job_status);

#endif /* STC_SPI_H_ */
