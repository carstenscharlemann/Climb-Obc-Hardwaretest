/*
 *  obc_ssp0.c
 *
 *  Created on: 07.10.2012
 *      Author: Andi
 *
 *  Copied over from pegasus flight software on 2019-12-14
 */

#include <stdio.h>
#include <string.h>
#include <chip.h>

#include "obc_ssp.h"

// Module defines
#define SSP0_SCK_PIN 20 //ok
#define SSP0_SCK_PORT 1 //ok
#define SSP0_MISO_PIN 23 //ok
#define SSP0_MISO_PORT 1 //ok
#define SSP0_MOSI_PIN 24 //ok
#define SSP0_MOSI_PORT 1 //ok
//#define SSP0_FUNCTION_NUMBER

#define FLASH2_CS1_PIN 12 //ok
#define FLASH2_CS1_PORT 2 //ok
#define FLASH2_CS2_PIN 11 //ok
#define FLASH2_CS2_PORT 2 //ok

#define SSP1_SCK_PIN 7
#define SSP1_SCK_PORT 0
#define SSP1_MISO_PIN 8
#define SSP1_MISO_PORT 0
#define SSP1_MOSI_PIN 9
#define SSP1_MOSI_PORT 0
//#define SSP1_FUNCTION_NUMBER 2

#define FLASH1_CS1_PIN 28
#define FLASH1_CS1_PORT 4
#define FLASH1_CS2_PIN 2
#define FLASH1_CS2_PORT 2

// from RTOS
#define configMAX_LIBRARY_INTERRUPT_PRIORITY    ( 5 )
#define SSP1_INTERRUPT_PRIORITY         (configMAX_LIBRARY_INTERRUPT_PRIORITY + 3)  /* SSP1 (Flash, MPU) */
#define SSP0_INTERRUPT_PRIORITY         (SSP1_INTERRUPT_PRIORITY + 1)   /* SSP0 (Flash) - should be lower than SSP1 */

typedef struct ssp_job_s
{
	uint8_t *array_to_send;
	uint16_t bytes_to_send;
	uint16_t bytes_sent;
	uint16_t bytes_to_read;
	uint16_t bytes_read;
	uint8_t *array_to_read;
	ssp_chip_t device;
	uint8_t status;
	uint8_t dir;
} volatile ssp_job_t;

typedef struct ssp_busstatus_s
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

} volatile ssp_busstatus_t;

typedef struct ssp_jobs_s
{
	ssp_job_t job[SPI_MAX_JOBS];
	uint8_t current_job;
	uint8_t last_job_added;
	uint8_t jobs_pending;
	ssp_busstatus_t bus_status;
} volatile ssp_jobs_t;

// local/module variables
volatile bool flash1_busy;		// temp 'ersatz' für semaphor
volatile bool flash2_busy;		// temp 'ersatz' für semaphor
ssp_jobs_t ssp_jobs[2];

// Prototypes
void SSP01_IRQHandler(LPC_SSP_T *device, uint8_t busNr);
void ssp_init(LPC_SSP_T *device, uint8_t busNr, IRQn_Type irq, uint32_t irqPrio );

// Module Init
void ssp01_init(void)
{
	ssp_jobs[0].bus_status.ssp_initialized = 0;
	ssp_jobs[1].bus_status.ssp_initialized = 0;

	/* --- SSP0 pins --- */
	Chip_IOCON_PinMuxSet(LPC_IOCON, SSP0_SCK_PORT, SSP0_SCK_PIN, IOCON_FUNC3 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, SSP0_SCK_PORT, SSP0_SCK_PIN);

	Chip_IOCON_PinMuxSet(LPC_IOCON, SSP0_MISO_PORT, SSP0_MISO_PIN, IOCON_FUNC3 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, SSP0_MISO_PORT, SSP0_MISO_PIN);

	Chip_IOCON_PinMuxSet(LPC_IOCON, SSP0_MOSI_PORT, SSP0_MOSI_PIN, IOCON_FUNC3 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, SSP0_MOSI_PORT, SSP0_MOSI_PIN);

	/* --- Chip selects --- */
	Chip_IOCON_PinMuxSet(LPC_IOCON, FLASH2_CS1_PORT, FLASH2_CS1_PIN, IOCON_FUNC0 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, FLASH2_CS1_PORT, FLASH2_CS1_PIN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, FLASH2_CS1_PORT, FLASH2_CS1_PIN);
	Chip_GPIO_SetPinState(LPC_GPIO, FLASH2_CS1_PORT, FLASH2_CS1_PIN, true);

	Chip_IOCON_PinMuxSet(LPC_IOCON, FLASH2_CS2_PORT, FLASH2_CS2_PIN, IOCON_FUNC0 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, FLASH2_CS2_PORT, FLASH2_CS2_PIN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, FLASH2_CS2_PORT, FLASH2_CS2_PIN);
	Chip_GPIO_SetPinState(LPC_GPIO, FLASH2_CS2_PORT, FLASH2_CS2_PIN, true);

	/* --- SSP1 pins --- */
	Chip_IOCON_PinMuxSet(LPC_IOCON, SSP1_SCK_PORT, SSP1_SCK_PIN, IOCON_FUNC2 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, SSP1_SCK_PORT, SSP1_SCK_PIN);

	Chip_IOCON_PinMuxSet(LPC_IOCON, SSP1_MISO_PORT, SSP1_MISO_PIN, IOCON_FUNC2 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, SSP1_MISO_PORT, SSP1_MISO_PIN);

	Chip_IOCON_PinMuxSet(LPC_IOCON, SSP1_MOSI_PORT, SSP1_MOSI_PIN, IOCON_FUNC2 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, SSP1_MOSI_PORT, SSP1_MOSI_PIN);

	/* --- Chip selects --- */
	Chip_IOCON_PinMuxSet(LPC_IOCON, FLASH1_CS1_PORT, FLASH1_CS1_PIN, IOCON_FUNC0 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, FLASH1_CS1_PORT, FLASH1_CS1_PIN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, FLASH1_CS1_PORT, FLASH1_CS1_PIN);
	Chip_GPIO_SetPinState(LPC_GPIO, FLASH1_CS1_PORT, FLASH1_CS1_PIN, true);

	Chip_IOCON_PinMuxSet(LPC_IOCON, FLASH1_CS2_PORT, FLASH1_CS2_PIN, IOCON_FUNC0 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, FLASH1_CS2_PORT, FLASH1_CS2_PIN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, FLASH1_CS2_PORT, FLASH1_CS2_PIN);
	Chip_GPIO_SetPinState(LPC_GPIO, FLASH1_CS2_PORT, FLASH1_CS2_PIN, true);

	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SSP0);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SSP1);

	ssp_init(LPC_SSP0, SSP_BUS0, SSP0_IRQn, SSP0_INTERRUPT_PRIORITY);
	ssp_init(LPC_SSP1, SSP_BUS1, SSP1_IRQn, SSP1_INTERRUPT_PRIORITY);

	return;
}

// Common init routine used for both SSP buses
void ssp_init(LPC_SSP_T *device, uint8_t busNr, IRQn_Type irq, uint32_t irqPrio ) {
	/* SSP Init */
	uint32_t helper;

	/* Prevent compiler warning */
	(void) helper;

	Chip_SSP_Set_Mode(device, SSP_MODE_MASTER);
	Chip_SSP_SetFormat(device, SSP_BITS_8, SSP_FRAMEFORMAT_SPI, SSP_CLOCK_CPHA0_CPOL0);
	Chip_SSP_SetBitRate(device, 4000000);		// -> TODO ergibt 400 kHz Clock rate !???

	Chip_SSP_DisableLoopBack(device);
	Chip_SSP_Enable(device);

	while ((device->SR & SSP_STAT_RNE) != 0) /* Flush RX FIFO */
	{
		helper = device->DR;
	}

	//SSP_IntConfig(LPC_SSP0, SSP_INTCFG_RT, ENABLE);
	//SSP_IntConfig(LPC_SSP0, SSP_INTCFG_ROR, ENABLE);
	//SSP_IntConfig(LPC_SSP0, SSP_INTCFG_RX, ENABLE);
	// no function found for this one !?
	device->IMSC |= SSP_RTIM;
	device->IMSC |= SSP_RORIM;
	device->IMSC |= SSP_RXIM;

	/* Clear interrupt flags */
	device->ICR = SSP_RORIM;
	device->ICR = SSP_RTIM;

	/* Reset buffers to default values */
	ssp_jobs[busNr].current_job = 0;
	ssp_jobs[busNr].jobs_pending = 0;
	ssp_jobs[busNr].last_job_added = 0;

	NVIC_SetPriority(irq, irqPrio);
	NVIC_EnableIRQ (irq);

	ssp_jobs[busNr].bus_status.ssp_error_counter = 0;
	ssp_jobs[busNr].bus_status.ssp_initialized = 1;
}


void ssp_unselect_device(ssp_chip_t chip) {
			switch (chip)
			/* Unselect device */
			{
				case SSPx_DEV_FLASH2_1:
					Chip_GPIO_SetValue(LPC_GPIO,FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
					flash2_busy = false;
					break;

				case SSPx_DEV_FLASH2_2:
					Chip_GPIO_SetValue(LPC_GPIO,FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
					flash2_busy = false;
					break;

				case SSPx_DEV_FLASH1_1:
					Chip_GPIO_SetValue(LPC_GPIO,FLASH1_CS1_PORT, 1 << FLASH1_CS1_PIN);
					flash1_busy = false;
					break;

				case SSPx_DEV_FLASH1_2:
					Chip_GPIO_SetValue(LPC_GPIO,FLASH1_CS2_PORT, 1 << FLASH1_CS2_PIN);
					flash1_busy = false;
					break;

				default: /* Device does not exist */
					/* Release all devices */
					// jobs->bus_status.ssp_interrupt_unknown_device = 1;
					Chip_GPIO_SetValue(LPC_GPIO,FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
					Chip_GPIO_SetValue(LPC_GPIO,FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
					Chip_GPIO_SetValue(LPC_GPIO,FLASH1_CS1_PORT, 1 << FLASH1_CS1_PIN);
					Chip_GPIO_SetValue(LPC_GPIO,FLASH1_CS2_PORT, 1 << FLASH1_CS2_PIN);
					/* Release all busy flags*/
					flash1_busy = false;
					flash2_busy = false;
					break;
			}
}

bool ssp_select_device(ssp_chip_t chip) {
	switch (chip)
	/* Select device */
	{
		case SSPx_DEV_FLASH2_1:
			Chip_GPIO_ClearValue(LPC_GPIO, FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
			break;

		case SSPx_DEV_FLASH2_2:
			Chip_GPIO_ClearValue(LPC_GPIO,FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
			break;

		case SSPx_DEV_FLASH1_1:
			Chip_GPIO_ClearValue(LPC_GPIO, FLASH1_CS1_PORT, 1 << FLASH1_CS1_PIN);
			break;

		case SSPx_DEV_FLASH1_2:
			Chip_GPIO_ClearValue(LPC_GPIO,FLASH1_CS2_PORT, 1 << FLASH1_CS2_PIN);
			break;

		default: /* Device does not exist */
			// Unselect all known devices
			//jobs->bus_status.ssp_error_counter++;
			Chip_GPIO_SetValue(LPC_GPIO, FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
			Chip_GPIO_SetValue(LPC_GPIO, FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
			Chip_GPIO_SetValue(LPC_GPIO, FLASH1_CS1_PORT, 1 << FLASH1_CS1_PIN);
			Chip_GPIO_SetValue(LPC_GPIO, FLASH1_CS2_PORT, 1 << FLASH1_CS2_PIN);
			return false;
	}
	return true;
}

bool ssp_chip_select(ssp_chip_t chip, bool select) {
	bool retVal = true;
	if (select) {
		retVal = ssp_select_device(chip);
	} else {
		ssp_unselect_device(chip);
	}
	return retVal;
}


void SSP1_IRQHandler(void)
{
	SSP01_IRQHandler(LPC_SSP1, SSP_BUS1);
}

void SSP0_IRQHandler(void)
{
	SSP01_IRQHandler(LPC_SSP0, SSP_BUS0);
}

void SSP01_IRQHandler(LPC_SSP_T *device, ssp_busnr_t busNr) {
	volatile uint32_t helper;
	ssp_jobs_t *jobs = &ssp_jobs[busNr];
	uint32_t int_src = device->RIS; /* Get interrupt source */

	if (int_src == SSP_TXIM)
	{
		/* TX buffer half empty interrupt is not used but may occur */
		return;
	}

	if ((int_src & SSP_RORIM))
	{
		device->ICR = SSP_RORIM;
		jobs->bus_status.ssp_error_counter++;
		jobs->bus_status.ssp_interrupt_ror = 1;
		return;
	}

	if ((int_src & SSP_RTIM))	// Clear receive timeout
	{
		device->ICR = SSP_RTIM;
	}

	ssp_job_t *cur_job = &(jobs->job[jobs->current_job]);
	if (cur_job->dir)
	{
		/* --- TX ------------------------------------------------------------------------------------------------------------------------ */

		// Dump RX
		while ((device->SR & SSP_STAT_RNE) != 0) /* Flush RX FIFO */
		{
			helper = device->DR;
		}

		/* Fill TX FIFO */

		if ((cur_job->bytes_to_send - cur_job->bytes_sent) > 7)
		{

			helper = cur_job->bytes_sent + 7;
		}
		else
		{
			helper = cur_job->bytes_to_send;
		}

		//while (((LPC_SSP0->SR & SSP_STAT_TXFIFO_NOTFULL)) && (ssp_jobs[deviceNr].job[ssp_jobs[deviceNr].current_job].bytes_sent < helper))
		while (((device->SR & SSP_STAT_TNF)) && (cur_job->bytes_sent < helper))
		{
			device->DR = cur_job->array_to_send[cur_job->bytes_sent];
			cur_job->bytes_sent++;
		}

		//if (LPC_SSP0->SR & SSP_SR_BSY)
		if (device->SR & SSP_STAT_BSY)
			return;

		if ((cur_job->bytes_sent == cur_job->bytes_to_send))
		{
			/* TX done */
			/* Check if job includes SSP read */
			if (cur_job->bytes_to_read > 0)
			{
				/* RX init */
				cur_job->dir = 0; /* set to read */
				while ((device->SR & SSP_STAT_RNE) != 0) /* Flush RX FIFO */
				{
					helper = device->DR;
				}

				cur_job->bytes_sent = 0;

				if ((cur_job->bytes_to_read - cur_job->bytes_sent) > 7)
				{

					helper = cur_job->bytes_sent + 7;
				}
				else
				{
					helper = cur_job->bytes_to_read;
				}

				while (((device->SR & SSP_STAT_TNF)) && (cur_job->bytes_sent < helper))
				{
					device->DR = 0xFF;
					cur_job->bytes_sent++;
				}

				helper = 0;
				/* Wait for interrupt*/
			}
			else
			{
				/* transfer done */
				/* release chip select and return */
				helper = 0;
				while ((device->SR & SSP_STAT_BSY) && (helper < 100000))
				{
					/* Wait for SSP to finish transmission */
					helper++;
				}

				ssp_chip_select(cur_job->device, false);
				//ssp_unselect_device(cur_job->device);
//				/* Unselect device */
//				switch (cur_job->device)
//				{
//					case SSPx_DEV_FLASH2_1:
//						Chip_GPIO_SetPortValue(LPC_GPIO, FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
//						flash2_busy = false;
//						break;
//
//					case SSPx_DEV_FLASH2_2:
//						Chip_GPIO_SetPortValue(LPC_GPIO, FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
//						flash2_busy = false;
//						break;
//
//					case SSPx_DEV_FLASH1_1:
//						Chip_GPIO_SetPortValue(LPC_GPIO, FLASH1_CS1_PORT, 1 << FLASH1_CS1_PIN);
//						flash1_busy = false;
//						break;
//
//					case SSPx_DEV_FLASH1_2:
//						Chip_GPIO_SetPortValue(LPC_GPIO, FLASH1_CS2_PORT, 1 << FLASH1_CS2_PIN);
//						flash1_busy = false;
//						break;
//
//					default: /* Device does not exist */
//						/* Release all devices */
//						jobs->bus_status.ssp_error_counter++;
//						jobs->bus_status.ssp_interrupt_unknown_device = 1;
//						Chip_GPIO_SetPortValue(LPC_GPIO,FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
//						Chip_GPIO_SetPortValue(LPC_GPIO,FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
//						Chip_GPIO_SetPortValue(LPC_GPIO, FLASH1_CS1_PORT, 1 << FLASH1_CS1_PIN);
//						Chip_GPIO_SetPortValue(LPC_GPIO, FLASH1_CS2_PORT, 1 << FLASH1_CS2_PIN);
//						break;
//				}

				cur_job->status = SSP_JOB_STATE_DONE;
			}
		}

	}
	else
	{
		/* --- RX ------------------------------------------------------------------------------------------------------------------------ */

		/* Read from RX FIFO */

		//while ((LPC_SSP0->SR & SSP_STAT_RXFIFO_NOTEMPTY)
		while ((device->SR & SSP_STAT_RNE)
		        && (cur_job->bytes_read < cur_job->bytes_to_read))
		{
			cur_job->array_to_read[cur_job->bytes_read] = device->DR;
			cur_job->bytes_read++;
		}

		if (cur_job->bytes_read == cur_job->bytes_to_read)
		{
			/* All bytes read */

			helper = 0;
			while ((device->SR & SSP_STAT_BSY) && (helper < 100000))
			{
				/* Wait for SSP to finish transmission */
				helper++;
			}

			ssp_chip_select(cur_job->device, false);
			//ssp_unselect_device(cur_job->device);
//			switch (cur_job->device)
//			/* Unselect device */
//			{
//
//				case SSPx_DEV_FLASH2_1:
//					Chip_GPIO_SetValue(LPC_GPIO,FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
//					flash2_busy = false;
//					break;
//
//				case SSPx_DEV_FLASH2_2:
//					Chip_GPIO_SetValue(LPC_GPIO,FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
//					flash2_busy = false;
//					break;
//
//				case SSPx_DEV_FLASH1_1:
//					Chip_GPIO_SetValue(LPC_GPIO,FLASH1_CS1_PORT, 1 << FLASH1_CS1_PIN);
//					flash1_busy = false;
//					break;
//
//				case SSPx_DEV_FLASH1_2:
//					Chip_GPIO_SetValue(LPC_GPIO,FLASH1_CS2_PORT, 1 << FLASH1_CS2_PIN);
//					flash1_busy = false;
//					break;
//
//				default: /* Device does not exist */
//					/* Release all devices */
//					jobs->bus_status.ssp_interrupt_unknown_device = 1;
//					Chip_GPIO_SetValue(LPC_GPIO,FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
//					Chip_GPIO_SetValue(LPC_GPIO,FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
//					Chip_GPIO_SetValue(LPC_GPIO,FLASH1_CS1_PORT, 1 << FLASH1_CS1_PIN);
//					Chip_GPIO_SetValue(LPC_GPIO,FLASH1_CS2_PORT, 1 << FLASH1_CS2_PIN);
//					break;
//			}

			cur_job->status = SSP_JOB_STATE_DONE;
		}
		else
		{
			/* not all bytes read - send dummy data again */

			if ((cur_job->bytes_to_read - cur_job->bytes_sent) > 7)
			{

				helper = cur_job->bytes_sent + 7;
			}
			else
			{
				helper = cur_job->bytes_to_read;
			}

			while ((device->SR & SSP_STAT_TNF) && (cur_job->bytes_sent < helper))
			{
				device->DR = 0xFF;
				cur_job->bytes_sent++;
			}
		}
	}

	if (cur_job->status == SSP_JOB_STATE_DONE)
	{
		/* Job is done, increment to next job and execute if pending */

		jobs->current_job++;
		jobs->jobs_pending--;

		if (jobs->current_job == SPI_MAX_JOBS)
		{
			jobs->current_job = 0;
		}

		while ((device->SR & SSP_STAT_RNE) != 0) /* Flush RX FIFO */
		{
			helper = device->DR;
		}

		/* Check if jobs are pending */
		if (jobs->jobs_pending > 0)
		{
			if (!ssp_chip_select(cur_job->device, true)) {
				jobs->bus_status.ssp_error_counter++;
				/* Set error description */
				cur_job->status = SSP_JOB_STATE_DEVICE_ERROR;

				/* Increment to next job */
				jobs->current_job++;
				jobs->jobs_pending--;

				if (jobs->current_job == SPI_MAX_JOBS)
				{
					jobs->current_job = 0;
				}
				return;
			}
//			switch (cur_job->device)
//			/* Select device */
//			{
//				case SSPx_DEV_FLASH2_1:
//					Chip_GPIO_ClearValue(LPC_GPIO, FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
//					break;
//
//				case SSPx_DEV_FLASH2_2:
//					Chip_GPIO_ClearValue(LPC_GPIO,FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
//					break;
//
//				case SSPx_DEV_FLASH1_1:
//					Chip_GPIO_ClearValue(LPC_GPIO, FLASH1_CS1_PORT, 1 << FLASH1_CS1_PIN);
//					break;
//
//				case SSPx_DEV_FLASH1_2:
//					Chip_GPIO_ClearValue(LPC_GPIO,FLASH1_CS2_PORT, 1 << FLASH1_CS2_PIN);
//					break;
//
//				default: /* Device does not exist */
//					jobs->bus_status.ssp_error_counter++;
//					Chip_GPIO_SetValue(LPC_GPIO, FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
//					Chip_GPIO_SetValue(LPC_GPIO, FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
//					Chip_GPIO_SetValue(LPC_GPIO, FLASH1_CS1_PORT, 1 << FLASH1_CS1_PIN);
//					Chip_GPIO_SetValue(LPC_GPIO, FLASH1_CS2_PORT, 1 << FLASH1_CS2_PIN);
//
//					/* Set error description */
//					cur_job->status = SSP_JOB_STATE_DEVICE_ERROR;
//
//					/* Increment to next job */
//					jobs->current_job++;
//					jobs->jobs_pending--;
//
//					if (jobs->current_job == SPI_MAX_JOBS)
//					{
//						jobs->current_job = 0;
//					}
//
//					return;
//
//					break;
//
//			}

			cur_job->status = SSP_JOB_STATE_ACTIVE;

			/* Fill FIFO */
			if (cur_job->dir)
			{
				/* TX (+RX) */
				if ((cur_job->bytes_to_send - cur_job->bytes_sent) > 7)
				{
					helper = cur_job->bytes_sent + 7;
				}
				else
				{
					helper = cur_job->bytes_to_send;
				}

				while (((device->SR & SSP_STAT_TNF)) && (cur_job->bytes_sent < helper))
				{
					device->DR = cur_job->array_to_send[cur_job->bytes_sent];
					cur_job->bytes_sent++;
				}
			}
			else
			{
				/* RX only - send dummy data for clock output */

				cur_job->bytes_sent = 0; /* Use unused bytes_sent for counting sent dummy data bytes */

				if ((cur_job->bytes_to_read - cur_job->bytes_sent) > 7)
				{

					helper = cur_job->bytes_sent + 7;
				}
				else
				{
					helper = cur_job->bytes_to_read;
				}

				while (((device->SR & SSP_STAT_TNF)) && (cur_job->bytes_sent < helper))
				{
					device->DR = 0xFF;
					cur_job->bytes_sent++;
				}
			}
		}
	}

	//portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	return;

	/* Max. execution time: 2863 cycles */
	/* Average execution time: 626 cycles */

}

uint32_t ssp_add_job(ssp_busnr_t busNr, ssp_chip_t sensor, uint8_t *array_to_send, uint16_t bytes_to_send, uint8_t *array_to_store, uint16_t bytes_to_read,
        uint8_t **job_status)
{
	uint32_t helper;
	uint8_t position;
	ssp_jobs_t *jobs = &ssp_jobs[busNr];
	LPC_SSP_T *device;
	
	if (busNr == SSP_BUS0) {
		device = LPC_SSP0;
	} else if (busNr == SSP_BUS1) {
		device = LPC_SSP1;
	} else {
		return SSP_WRONG_BUSNR;
	}

	if (jobs->bus_status.ssp_initialized == 0)
	{
		/* SSP is not initialized - return */
		return SSP_JOB_NOT_INITIALIZED;
	}

	if (jobs->jobs_pending >= SPI_MAX_JOBS)
	{
		/* Maximum amount of jobs stored, job can't be added! */
		/* This is possibly caused by a locked interrupt -> remove all jobs and re-init SSP */
		//taskENTER_CRITICAL();
		jobs->bus_status.ssp_error_counter++;
		jobs->bus_status.ssp_buffer_overflow = 1;
		jobs->jobs_pending = 0; /* Delete jobs */
		ssp01_init(); /* Reinit SSP   make re-init per SSP nr possible here !?*/
		//taskEXIT_CRITICAL();
		return SSP_JOB_BUFFER_OVERFLOW;
	}

	// taskENTER_CRITICAL();		TODO: need for real multithreading.
	{
		position = (jobs->current_job + jobs->jobs_pending) % SPI_MAX_JOBS;

		jobs->job[position].array_to_send = array_to_send;
		jobs->job[position].bytes_to_send = bytes_to_send;
		jobs->job[position].bytes_sent = 0;
		jobs->job[position].array_to_read = array_to_store;
		jobs->job[position].bytes_to_read = bytes_to_read;
		jobs->job[position].bytes_read = 0;
		jobs->job[position].device = sensor;
		jobs->job[position].status = SSP_JOB_STATE_PENDING;

		if (bytes_to_send > 0)
		{
			/* Job contains transfer and read eventually */
			jobs->job[position].dir = 1;
		}
		else
		{
			/* Job contains readout only - transfer part is skipped */
			jobs->job[position].dir = 0;
		}

		/* Check if SPI in use */
		if (jobs->jobs_pending == 0)
		{ /* Check if jobs pending */

			/* Select device */
			if (!ssp_chip_select(jobs->job[position].device, true)) {
				jobs->bus_status.ssp_error_counter++;

				/* Set error description */
				jobs->job[position].status = SSP_JOB_STATE_DEVICE_ERROR;

				/* Increment to next job */
				jobs->current_job++;
				jobs->jobs_pending--;

				if (jobs->current_job == SPI_MAX_JOBS)
				{
					jobs->current_job = 0;
				}

				/* Return error */
				return SSP_JOB_ERROR;
			}
//			switch (jobs->job[position].device)
//			/* Select device */
//			{
//				case SSPx_DEV_FLASH2_1:
//					Chip_GPIO_ClearValue(LPC_GPIO, FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
//					break;
//
//				case SSPx_DEV_FLASH2_2:
//					Chip_GPIO_ClearValue(LPC_GPIO, FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
//					break;
//
//				case SSPx_DEV_FLASH1_1:
//					Chip_GPIO_ClearValue(LPC_GPIO, FLASH1_CS1_PORT, 1 << FLASH1_CS1_PIN);
//					break;
//
//				case SSPx_DEV_FLASH1_2:
//					Chip_GPIO_ClearValue(LPC_GPIO, FLASH1_CS2_PORT, 1 << FLASH1_CS2_PIN);
//					break;
//
//				default: /* Device does not exist */
//					/* Unselect all device */
//					Chip_GPIO_SetValue(LPC_GPIO, FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
//					Chip_GPIO_SetValue(LPC_GPIO, FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
//					Chip_GPIO_SetValue(LPC_GPIO, FLASH1_CS1_PORT, 1 << FLASH1_CS1_PIN);
//					Chip_GPIO_SetValue(LPC_GPIO, FLASH1_CS2_PORT, 1 << FLASH1_CS2_PIN);
//
//					jobs->bus_status.ssp_error_counter++;
//
//					/* Set error description */
//					jobs->job[position].status = SSP_JOB_STATE_DEVICE_ERROR;
//
//					/* Increment to next job */
//					jobs->current_job++;
//					jobs->jobs_pending--;
//
//					if (jobs->current_job == SPI_MAX_JOBS)
//					{
//						jobs->current_job = 0;
//					}
//
//					/* Return error */
//					return SSP_JOB_ERROR;
//					break;
//
//			}

			jobs->job[position].status = SSP_JOB_STATE_ACTIVE;

			while ((device->SR & SSP_STAT_RNE) != 0) /* Flush RX FIFO */
			{
				helper = device->DR;
			}

			/* Fill FIFO */

			if (jobs->job[position].dir)
			{
				/* TX (+RX) */

				if ((jobs->job[jobs->current_job].bytes_to_send - jobs->job[jobs->current_job].bytes_sent) > 7)
				{

					helper = jobs->job[jobs->current_job].bytes_sent + 7;
				}
				else
				{
					helper = jobs->job[jobs->current_job].bytes_to_send;
				}

				while (((device->SR & SSP_STAT_TNF)) && (jobs->job[position].bytes_sent < helper))
				{
					device->DR = jobs->job[position].array_to_send[jobs->job[position].bytes_sent];
					jobs->job[position].bytes_sent++;
				}
			}
			else
			{
				/* RX only - send dummy data for clock output */
				/* Use unused bytes_sent for counting sent dummy data bytes */

				if ((jobs->job[jobs->current_job].bytes_to_read - jobs->job[jobs->current_job].bytes_sent) > 7)
				{

					helper = jobs->job[jobs->current_job].bytes_sent + 7;
				}
				else
				{
					helper = jobs->job[jobs->current_job].bytes_to_read;
				}

				while (((device->SR & SSP_STAT_TNF)) && (jobs->job[position].bytes_sent < helper))
				{
					device->DR = 0xFF;
					jobs->job[position].bytes_sent++;

				}
			}
		}

		jobs->jobs_pending++;
	}

	/* Set pointer to job bus_status if necessary */
	if (job_status != NULL)
	{
		*job_status = (uint8_t *) &(jobs->job[position].status);
	}

	// taskEXIT_CRITICAL();	TODO needed for real multithreading
	return SSP_JOB_ADDED; /* Job added successfully */
}

