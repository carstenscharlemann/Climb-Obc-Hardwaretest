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

//#include <lpc17xx_spi.h>
//#include <lpc17xx_ssp.h>
#include "obc_ssp.h"
//#include "lpc17xx_gpio.h"

//#include "FreeRTOS.h"
//#include "task.h"
//#include "queue.h"
//#include "semphr.h"

/* FreeRTOS+IO includes. */
//#include "FreeRTOS_IO.h"

//#include "layer2/inc/obc_flash.h"

#define SSP0_DEBUG 0

ssp_jobs_t ssp0_jobs;

tmp_status_t obc_status;

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


// from RTOS
#define configMAX_LIBRARY_INTERRUPT_PRIORITY    ( 5 )
#define SSP1_INTERRUPT_PRIORITY         (configMAX_LIBRARY_INTERRUPT_PRIORITY + 3)  /* SSP1 (Flash, MPU) */
#define SSP0_INTERRUPT_PRIORITY         (SSP1_INTERRUPT_PRIORITY + 1)   /* SSP0 (Flash) - should be lower than SSP1 */

//typedef long BaseType_t;
//#define pdFALSE			( ( BaseType_t ) 0 )

volatile bool flash2_busy;		// temp 'ersatz' für semaphor

void ssp0_init(void)
{
	/* SSP Init */

//	PINSEL_CFG_Type xPinConfig;
//	SSP_CFG_Type sspInitialization;
	uint32_t helper;

	/* Prevent compiler warning */
	(void) helper;

	obc_status.ssp0_initialized = 0;

	/* --- SSP0 pins --- */
	//(xPinConfig).Funcnum = SSP0_FUNCTION_NUMBER;
	//(xPinConfig).OpenDrain = 0;
	//(xPinConfig).Pinmode = PINSEL_PINMODE_TRISTATE;
	//(xPinConfig).Portnum = SSP0_SCK_PORT;
	//(xPinConfig).Pinnum = SSP0_SCK_PIN;
	//PINSEL_ConfigPin(&xPinConfig);
	Chip_IOCON_PinMuxSet(LPC_IOCON, SSP0_SCK_PORT, SSP0_SCK_PIN, IOCON_FUNC3 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, SSP0_SCK_PORT, SSP0_SCK_PIN);


//	(xPinConfig).Pinnum = SSP0_MISO_PIN;
//	(xPinConfig).Portnum = SSP0_MISO_PORT;
//	PINSEL_ConfigPin(&xPinConfig);
	Chip_IOCON_PinMuxSet(LPC_IOCON, SSP0_MISO_PORT, SSP0_MISO_PIN, IOCON_FUNC3 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, SSP0_MISO_PORT, SSP0_MISO_PIN);


//	(xPinConfig).Pinnum = SSP0_MOSI_PIN;
//	(xPinConfig).Portnum = SSP0_MOSI_PORT;
//	PINSEL_ConfigPin(&xPinConfig);
	Chip_IOCON_PinMuxSet(LPC_IOCON, SSP0_MOSI_PORT, SSP0_MOSI_PIN, IOCON_FUNC3 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, SSP0_MOSI_PORT, SSP0_MOSI_PIN);


	/* --- Chip selects --- */
	//(xPinConfig).Funcnum = 0;
	//(xPinConfig).Portnum = FLASH2_CS1_PORT;
	//(xPinConfig).Pinnum = FLASH2_CS1_PIN;
	//PINSEL_ConfigPin(&(xPinConfig));
	Chip_IOCON_PinMuxSet(LPC_IOCON, FLASH2_CS1_PORT, FLASH2_CS1_PIN, IOCON_FUNC0 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, FLASH2_CS1_PORT, FLASH2_CS1_PIN);

	//GPIO_SetDir(FLASH2_CS1_PORT, (1 << FLASH2_CS1_PIN), 1);
	//GPIO_SetValue(FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);

	Chip_GPIO_SetPinDIROutput(LPC_GPIO, FLASH2_CS1_PORT, FLASH2_CS1_PIN);
	Chip_GPIO_SetPinState(LPC_GPIO, FLASH2_CS1_PORT, FLASH2_CS1_PIN, true);


	//(xPinConfig).Portnum = FLASH2_CS2_PORT;
	//(xPinConfig).Pinnum = FLASH2_CS2_PIN;
	//PINSEL_ConfigPin(&(xPinConfig));
	Chip_IOCON_PinMuxSet(LPC_IOCON, FLASH2_CS2_PORT, FLASH2_CS2_PIN, IOCON_FUNC0 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, FLASH2_CS2_PORT, FLASH2_CS2_PIN);

	//GPIO_SetDir(FLASH2_CS2_PORT, (1 << FLASH2_CS2_PIN), 1);
	//GPIO_SetValue(FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, FLASH2_CS2_PORT, FLASH2_CS2_PIN);
	Chip_GPIO_SetPinState(LPC_GPIO, FLASH2_CS2_PORT, FLASH2_CS2_PIN, true);


//	SSP_CFG_Type sspInitialization;
//	SSP_ConfigStructInit(&sspInitialization);			-> code in LPC17xx_spi.c macht default Werte:
//															SSP_InitStruct->CPHA = SSP_CPHA_FIRST;
//															SSP_InitStruct->CPOL = SSP_CPOL_HI;
//															SSP_InitStruct->ClockRate = 1000000;
//															SSP_InitStruct->Databit = SSP_DATABIT_8;
//															SSP_InitStruct->Mode = SSP_MASTER_MODE;
//															SSP_InitStruct->FrameFormat = SSP_FRAME_SPI;

//														PEG Werte:
//	sspInitialization.CPHA = SSP_CPHA_FIRST;			((uint32_t)(0))
//	sspInitialization.CPOL = SSP_CPOL_HI;				((uint32_t)(0))
//	sspInitialization.ClockRate = 2000000;
//	sspInitialization.FrameFormat = SSP_FRAME_SPI;		((uint32_t)(0<<4))			??? einen 0er um 4 bit nach links ???
//	sspInitialization.Databit = SSP_DATABIT_8;			((uint32_t)((8-1)&0xF))
//	sspInitialization.Mode = SSP_MASTER_MODE;			((uint32_t)(0))
//
//	SSP_Init(LPC_SSP0, &sspInitialization);				-> code in LPC17xx_spi
//															void SSP_Init(LPC_SSP_TypeDef *SSPx, SSP_CFG_Type *SSP_ConfigStruct)
//															{
//																...
//																	/* Set up clock and power for SSP0 module */
//																	CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCSSP0, ENABLE);
//																...
//
//																tmp = ((SSP_ConfigStruct->CPHA) | (SSP_ConfigStruct->CPOL) | (SSP_ConfigStruct->FrameFormat) | (SSP_ConfigStruct->Databit)) & SSP_CR0_BITMASK;
//																SSPx->CR0 = tmp;
//
//																tmp = SSP_ConfigStruct->Mode & SSP_CR1_BITMASK;
//																SSPx->CR1 = tmp;
//
//																// Set clock rate for SSP peripheral
//																setSSPclock(SSPx, SSP_ConfigStruct->ClockRate);		-> rel koplizierte routine um passendes asetiing zu finden (lt. kommentar ist Clockrate hier in Hz.)
//															}
//
//
//
	// Init from LPC SSP example:
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SSP0);

	Chip_SSP_Set_Mode(LPC_SSP0, SSP_MODE_MASTER);
	Chip_SSP_SetFormat(LPC_SSP0, SSP_BITS_8, SSP_FRAMEFORMAT_SPI, SSP_CLOCK_CPHA0_CPOL0);
	Chip_SSP_SetBitRate(LPC_SSP0, 2000000);

	//SSP_LoopBackCmd(LPC_SSP0, DISABLE);
	Chip_SSP_DisableLoopBack(LPC_SSP0);

	//SSP_Cmd(LPC_SSP0, ENABLE);
	Chip_SSP_Enable(LPC_SSP0);

	while ((LPC_SSP0->SR & SSP_STAT_RNE) != 0) /* Flush RX FIFO */
	{
		helper = LPC_SSP0->DR;
	}

	//SSP_IntConfig(LPC_SSP0, SSP_INTCFG_RT, ENABLE);
	//SSP_IntConfig(LPC_SSP0, SSP_INTCFG_ROR, ENABLE);
	//SSP_IntConfig(LPC_SSP0, SSP_INTCFG_RX, ENABLE);

	// no function found for this one !?
	LPC_SSP0->IMSC |= SSP_RTIM;
	LPC_SSP0->IMSC |= SSP_RORIM;
	LPC_SSP0->IMSC |= SSP_RXIM;

	/* Clear interrupt flags */
	//LPC_SSP0->ICR = SSP_INTCLR_ROR;
	//LPC_SSP0->ICR = SSP_INTCLR_RT;
	LPC_SSP0->ICR = SSP_RORIM;
	LPC_SSP0->ICR = SSP_RTIM;

	/* Reset buffers to default values */
	ssp0_jobs.current_job = 0;
	ssp0_jobs.jobs_pending = 0;
	ssp0_jobs.last_job_added = 0;

	NVIC_SetPriority(SSP0_IRQn, SSP0_INTERRUPT_PRIORITY);
	NVIC_EnableIRQ (SSP0_IRQn);

	obc_status.ssp0_error_counter = 0;
	obc_status.ssp0_initialized = 1;
	return;
}

void SSP0_IRQHandler(void)
{
	//BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	uint32_t int_src = LPC_SSP0->RIS; /* Get interrupt source */
	volatile uint32_t helper;

	if (int_src == SSP_TXIM)
	{
		/* TX buffer half empty interrupt is not used but may occur */
		return;
	}

	if ((int_src & SSP_RORIM))
	{
		LPC_SSP0->ICR = SSP_RORIM;
		obc_status.ssp0_error_counter++;
		obc_status.ssp0_interrupt_ror = 1;
		return;
	}

	if ((int_src & SSP_RTIM))	// Clear receive timeout
	{
		LPC_SSP0->ICR = SSP_RTIM;
	}

	if (ssp0_jobs.job[ssp0_jobs.current_job].dir)
	{
		/* --- TX ------------------------------------------------------------------------------------------------------------------------ */

		// Dump RX
		while ((LPC_SSP0->SR & SSP_STAT_RNE) != 0) /* Flush RX FIFO */
		{
			helper = LPC_SSP0->DR;
		}

		/* Fill TX FIFO */

		if ((ssp0_jobs.job[ssp0_jobs.current_job].bytes_to_send - ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent) > 7)
		{

			helper = ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent + 7;
		}
		else
		{
			helper = ssp0_jobs.job[ssp0_jobs.current_job].bytes_to_send;
		}

		//while (((LPC_SSP0->SR & SSP_STAT_TXFIFO_NOTFULL)) && (ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent < helper))
		while (((LPC_SSP0->SR & SSP_STAT_TNF)) && (ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent < helper))
		{
			LPC_SSP0->DR = ssp0_jobs.job[ssp0_jobs.current_job].array_to_send[ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent];
			ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent++;
		}

		//if (LPC_SSP0->SR & SSP_SR_BSY)
		if (LPC_SSP0->SR & SSP_STAT_BSY)
			return;

		if ((ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent == ssp0_jobs.job[ssp0_jobs.current_job].bytes_to_send))
		{
			/* TX done */
			/* Check if job includes SSP read */
			if (ssp0_jobs.job[ssp0_jobs.current_job].bytes_to_read > 0)
			{
				/* RX init */
				ssp0_jobs.job[ssp0_jobs.current_job].dir = 0; /* set to read */
				while ((LPC_SSP0->SR & SSP_STAT_RNE) != 0) /* Flush RX FIFO */
				{
					helper = LPC_SSP0->DR;
				}

				ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent = 0;

				if ((ssp0_jobs.job[ssp0_jobs.current_job].bytes_to_read - ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent) > 7)
				{

					helper = ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent + 7;
				}
				else
				{
					helper = ssp0_jobs.job[ssp0_jobs.current_job].bytes_to_read;
				}

				while (((LPC_SSP0->SR & SSP_STAT_TNF)) && (ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent < helper))
				{
					LPC_SSP0->DR = 0xFF;
					ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent++;
				}

				helper = 0;
				/* Wait for interrupt*/
			}
			else
			{
				/* transfer done */
				/* release chip select and return */
				helper = 0;
				while ((LPC_SSP0->SR & SSP_STAT_BSY) && (helper < 100000))
				{
					/* Wait for SSP to finish transmission */
					helper++;
				}
				switch (ssp0_jobs.job[ssp0_jobs.current_job].device)
				/* Unselect device */
				{

					case SSP0_DEV_FLASH2_1:
						//GPIO_SetValue(FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
						Chip_GPIO_SetPortValue(LPC_GPIO, FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
						//xSemaphoreGiveFromISR(flash2_semaphore, &xHigherPriorityTaskWoken);
						// TOdo ....
						flash2_busy = false;
						break;

					case SSP0_DEV_FLASH2_2:
						//GPIO_SetValue(FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
						Chip_GPIO_SetPortValue(LPC_GPIO, FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
						//xSemaphoreGiveFromISR(flash2_semaphore, &xHigherPriorityTaskWoken);
						flash2_busy = false;
						break;
					default: /* Device does not exist */
						/* Release all devices */
						obc_status.ssp0_error_counter++;
						obc_status.ssp0_interrupt_unknown_device = 1;
//						GPIO_SetValue(FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
//						GPIO_SetValue(FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
						Chip_GPIO_SetPortValue(LPC_GPIO,FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
						Chip_GPIO_SetPortValue(LPC_GPIO,FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
						break;
				}

				ssp0_jobs.job[ssp0_jobs.current_job].status = SSP_JOB_STATE_DONE;
			}
		}

	}
	else
	{
		/* --- RX ------------------------------------------------------------------------------------------------------------------------ */

		/* Read from RX FIFO */

		//while ((LPC_SSP0->SR & SSP_STAT_RXFIFO_NOTEMPTY)
		while ((LPC_SSP0->SR & SSP_STAT_RNE)
		        && (ssp0_jobs.job[ssp0_jobs.current_job].bytes_read < ssp0_jobs.job[ssp0_jobs.current_job].bytes_to_read))
		{
			ssp0_jobs.job[ssp0_jobs.current_job].array_to_read[ssp0_jobs.job[ssp0_jobs.current_job].bytes_read] = LPC_SSP0->DR;
			ssp0_jobs.job[ssp0_jobs.current_job].bytes_read++;
		}

		if (ssp0_jobs.job[ssp0_jobs.current_job].bytes_read == ssp0_jobs.job[ssp0_jobs.current_job].bytes_to_read)
		{
			/* All bytes read */

			helper = 0;
			while ((LPC_SSP0->SR & SSP_STAT_BSY) && (helper < 100000))
			{
				/* Wait for SSP to finish transmission */
				helper++;
			}

			switch (ssp0_jobs.job[ssp0_jobs.current_job].device)
			/* Unselect device */
			{

				case SSP0_DEV_FLASH2_1:
					Chip_GPIO_SetValue(LPC_GPIO,FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
					//xSemaphoreGiveFromISR(flash2_semaphore, &xHigherPriorityTaskWoken);
					flash2_busy = false;
					break;

				case SSP0_DEV_FLASH2_2:
					Chip_GPIO_SetValue(LPC_GPIO,FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
					//xSemaphoreGiveFromISR(flash2_semaphore, &xHigherPriorityTaskWoken);
					flash2_busy = false;
					break;
				default: /* Device does not exist */
					/* Release all devices */
					obc_status.ssp0_interrupt_unknown_device = 1;
					Chip_GPIO_SetValue(LPC_GPIO,FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
					Chip_GPIO_SetValue(LPC_GPIO,FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
					break;
			}

			ssp0_jobs.job[ssp0_jobs.current_job].status = SSP_JOB_STATE_DONE;
		}
		else
		{
			/* not all bytes read - send dummy data again */

			if ((ssp0_jobs.job[ssp0_jobs.current_job].bytes_to_read - ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent) > 7)
			{

				helper = ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent + 7;
			}
			else
			{
				helper = ssp0_jobs.job[ssp0_jobs.current_job].bytes_to_read;
			}

			while ((LPC_SSP0->SR & SSP_STAT_TNF) && (ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent < helper))
			{
				LPC_SSP0->DR = 0xFF;
				ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent++;
			}
		}
	}

	if (ssp0_jobs.job[ssp0_jobs.current_job].status == SSP_JOB_STATE_DONE)
	{
		/* Job is done, increment to next job and execute if pending */

		ssp0_jobs.current_job++;
		ssp0_jobs.jobs_pending--;

		if (ssp0_jobs.current_job == SPI_MAX_JOBS)
		{
			ssp0_jobs.current_job = 0;
		}

		while ((LPC_SSP0->SR & SSP_STAT_RNE) != 0) /* Flush RX FIFO */
		{
			helper = LPC_SSP0->DR;
		}

		/* Check if jobs are pending */
		if (ssp0_jobs.jobs_pending > 0)
		{
			switch (ssp0_jobs.job[ssp0_jobs.current_job].device)
			/* Select device */
			{
				case SSP0_DEV_FLASH2_1:
					//GPIO_ClearValue(FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
					Chip_GPIO_ClearValue(LPC_GPIO, FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
					break;

				case SSP0_DEV_FLASH2_2:
					//GPIO_ClearValue(FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
					Chip_GPIO_ClearValue(LPC_GPIO,FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
					break;
				default: /* Device does not exist */
					obc_status.ssp0_error_counter++;
//					GPIO_SetValue(FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
//					GPIO_SetValue(FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
					Chip_GPIO_SetValue(LPC_GPIO, FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
					Chip_GPIO_SetValue(LPC_GPIO, FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);

					/* Set error description */
					ssp0_jobs.job[ssp0_jobs.current_job].status = SSP_JOB_STATE_DEVICE_ERROR;

					/* Increment to next job */
					ssp0_jobs.current_job++;
					ssp0_jobs.jobs_pending--;

					if (ssp0_jobs.current_job == SPI_MAX_JOBS)
					{
						ssp0_jobs.current_job = 0;
					}

					return;

					break;

			}

			ssp0_jobs.job[ssp0_jobs.current_job].status = SSP_JOB_STATE_ACTIVE;

			/* Fill FIFO */
			if (ssp0_jobs.job[ssp0_jobs.current_job].dir)
			{
				/* TX (+RX) */
				if ((ssp0_jobs.job[ssp0_jobs.current_job].bytes_to_send - ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent) > 7)
				{
					helper = ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent + 7;
				}
				else
				{
					helper = ssp0_jobs.job[ssp0_jobs.current_job].bytes_to_send;
				}

				while (((LPC_SSP0->SR & SSP_STAT_TNF)) && (ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent < helper))
				{
					LPC_SSP0->DR = ssp0_jobs.job[ssp0_jobs.current_job].array_to_send[ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent];
					ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent++;
				}
			}
			else
			{
				/* RX only - send dummy data for clock output */

				ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent = 0; /* Use unused bytes_sent for counting sent dummy data bytes */

				if ((ssp0_jobs.job[ssp0_jobs.current_job].bytes_to_read - ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent) > 7)
				{

					helper = ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent + 7;
				}
				else
				{
					helper = ssp0_jobs.job[ssp0_jobs.current_job].bytes_to_read;
				}

				while (((LPC_SSP0->SR & SSP_STAT_TNF)) && (ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent < helper))
				{
					LPC_SSP0->DR = 0xFF;
					ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent++;
				}
			}
		}
	}

	//portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	return;

	/* Max. execution time: 2863 cycles */
	/* Average execution time: 626 cycles */

}

uint32_t ssp0_add_job(uint8_t sensor, uint8_t *array_to_send, uint16_t bytes_to_send, uint8_t *array_to_store, uint16_t bytes_to_read,
        uint8_t **job_status)
{
	uint32_t helper;
	uint8_t position;

	if (obc_status.ssp0_initialized == 0)
	{
		/* SSP is not initialized - return */
		return SSP_JOB_NOT_INITIALIZED;
	}

	if (ssp0_jobs.jobs_pending >= SPI_MAX_JOBS)
	{
		/* Maximum amount of jobs stored, job can't be added! */
		/* This is possibly caused by a locked interrupt -> remove all jobs and re-init SSP */
		//taskENTER_CRITICAL();
		obc_status.ssp0_error_counter++;
		obc_status.ssp0_buffer_overflow = 1;
		ssp0_jobs.jobs_pending = 0; /* Delete jobs */
		ssp0_init(); /* Reinit SSP */
		//taskEXIT_CRITICAL();
		return SSP_JOB_BUFFER_OVERFLOW;
	}

	// taskENTER_CRITICAL();		TODO: need for real multithreading.
	{
		position = (ssp0_jobs.current_job + ssp0_jobs.jobs_pending) % SPI_MAX_JOBS;

		ssp0_jobs.job[position].array_to_send = array_to_send;
		ssp0_jobs.job[position].bytes_to_send = bytes_to_send;
		ssp0_jobs.job[position].bytes_sent = 0;
		ssp0_jobs.job[position].array_to_read = array_to_store;
		ssp0_jobs.job[position].bytes_to_read = bytes_to_read;
		ssp0_jobs.job[position].bytes_read = 0;
		ssp0_jobs.job[position].device = sensor;
		ssp0_jobs.job[position].status = SSP_JOB_STATE_PENDING;

		if (bytes_to_send > 0)
		{
			/* Job contains transfer and read eventually */
			ssp0_jobs.job[position].dir = 1;
		}
		else
		{
			/* Job contains readout only - transfer part is skipped */
			ssp0_jobs.job[position].dir = 0;
		}

		/* Check if SPI in use */
		if (ssp0_jobs.jobs_pending == 0)
		{ /* Check if jobs pending */
			switch (ssp0_jobs.job[position].device)
			/* Select device */
			{
				case SSP0_DEV_FLASH2_1:
					Chip_GPIO_ClearValue(LPC_GPIO, FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
					break;

				case SSP0_DEV_FLASH2_2:
					Chip_GPIO_ClearValue(LPC_GPIO, FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
					break;

				default: /* Device does not exist */
					/* Unselect all device */
					Chip_GPIO_SetValue(LPC_GPIO, FLASH2_CS1_PORT, 1 << FLASH2_CS1_PIN);
					Chip_GPIO_SetValue(LPC_GPIO, FLASH2_CS2_PORT, 1 << FLASH2_CS2_PIN);
					obc_status.ssp0_error_counter++;

					/* Set error description */
					ssp0_jobs.job[position].status = SSP_JOB_STATE_DEVICE_ERROR;

					/* Increment to next job */
					ssp0_jobs.current_job++;
					ssp0_jobs.jobs_pending--;

					if (ssp0_jobs.current_job == SPI_MAX_JOBS)
					{
						ssp0_jobs.current_job = 0;
					}

					/* Return error */
					return SSP_JOB_ERROR;
					break;

			}

			ssp0_jobs.job[position].status = SSP_JOB_STATE_ACTIVE;

			while ((LPC_SSP0->SR & SSP_STAT_RNE) != 0) /* Flush RX FIFO */
			{
				helper = LPC_SSP0->DR;
			}

			/* Fill FIFO */

			if (ssp0_jobs.job[position].dir)
			{
				/* TX (+RX) */

				if ((ssp0_jobs.job[ssp0_jobs.current_job].bytes_to_send - ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent) > 7)
				{

					helper = ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent + 7;
				}
				else
				{
					helper = ssp0_jobs.job[ssp0_jobs.current_job].bytes_to_send;
				}

				while (((LPC_SSP0->SR & SSP_STAT_TNF)) && (ssp0_jobs.job[position].bytes_sent < helper))
				{
					LPC_SSP0->DR = ssp0_jobs.job[position].array_to_send[ssp0_jobs.job[position].bytes_sent];
					ssp0_jobs.job[position].bytes_sent++;
				}
			}
			else
			{
				/* RX only - send dummy data for clock output */
				/* Use unused bytes_sent for counting sent dummy data bytes */

				if ((ssp0_jobs.job[ssp0_jobs.current_job].bytes_to_read - ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent) > 7)
				{

					helper = ssp0_jobs.job[ssp0_jobs.current_job].bytes_sent + 7;
				}
				else
				{
					helper = ssp0_jobs.job[ssp0_jobs.current_job].bytes_to_read;
				}

				while (((LPC_SSP0->SR & SSP_STAT_TNF)) && (ssp0_jobs.job[position].bytes_sent < helper))
				{
					LPC_SSP0->DR = 0xFF;
					ssp0_jobs.job[position].bytes_sent++;

				}
			}
		}

		ssp0_jobs.jobs_pending++;
	}

	/* Set pointer to job status if necessary */
	if (job_status != NULL)
	{
		*job_status = (uint8_t *) &(ssp0_jobs.job[position].status);
	}

	// taskEXIT_CRITICAL();	TODO needed for real multithreading
	return SSP_JOB_ADDED; /* Job added successfully */
}

