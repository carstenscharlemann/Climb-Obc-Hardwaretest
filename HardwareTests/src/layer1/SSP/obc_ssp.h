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

/* Max SPI buffer length */
#define SPI_MAX_JOBS 16

typedef enum ssp_chip_e
{
	SSPx_DEV_FLASH1_1, SSPx_DEV_FLASH1_2, SSPx_DEV_FLASH2_1, SSPx_DEV_FLASH2_2
} ssp_chip_t;

typedef enum ssp_busnr_e
{
	SSP_BUS0 = 0, SSP_BUS1 = 1
} ssp_busnr_t;

enum
{
	SSP_JOB_STATE_DONE = 0, SSP_JOB_STATE_PENDING, SSP_JOB_STATE_ACTIVE, SSP_JOB_STATE_SSP_ERROR, SSP_JOB_STATE_DEVICE_ERROR
};

typedef enum ssp_addjob_ret_e
{ /* Return values for ssp_add_job */
	SSP_JOB_ADDED = 0, SSP_JOB_BUFFER_OVERFLOW, SSP_JOB_MALLOC_FAILED, SSP_JOB_ERROR, SSP_JOB_NOT_INITIALIZED, SSP_WRONG_BUSNR
} ssp_jobdef_ret_t;

extern volatile bool flash1_busy;		// temp 'ersatz' für semaphor
extern volatile bool flash2_busy;		// temp 'ersatz' für semaphor

void ssp01_init(void);
ssp_jobdef_ret_t ssp_add_job(ssp_busnr_t busNr, ssp_chip_t chip, uint8_t *array_to_send, uint16_t bytes_to_send, uint8_t *array_to_store, uint16_t bytes_to_read, uint8_t **job_status);

ssp_jobdef_ret_t ssp_add_job2( ssp_busnr_t busNr,
		                       ssp_chip_t chip,
							   uint8_t *array_to_send,
							   uint16_t bytes_to_send,
							   uint8_t *array_to_store,
							   uint16_t bytes_to_read,
							   uint8_t **job_status,
							   bool(*chipSelectHandler)(ssp_chip_t chip, bool select));

#endif /* STC_SPI_H_ */
