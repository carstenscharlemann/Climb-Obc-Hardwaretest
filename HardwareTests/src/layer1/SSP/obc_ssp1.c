/*
 *  obc_ssp1.c
 *  Copied over from pegasus flight software on 2019-12-14
 */
//
//#include <stdio.h>
//#include <string.h>
//
//#include "FreeRTOS.h"
//#include "task.h"
//#include "queue.h"
//#include "semphr.h"
//
///* FreeRTOS+IO includes. */
//#include "FreeRTOS_IO.h"
//
//#include <lpc17xx_spi.h>
//#include <lpc17xx_ssp.h>
//#include "lpc17xx_gpio.h"
//
//#include <layer1/inc/obc_ssp.h>
//#include "layer2/inc/obc_flash.h"
//
//ssp_jobs_t ssp1_jobs;
//
//void ssp1_init(void)
//{
//    /* SSP Init */
//    PINSEL_CFG_Type xPinConfig;
//    SSP_CFG_Type sspInitialization;
//    uint32_t helper;
//
//    obc_status.ssp1_initialized = 0;
//
//    /* Prevent compiler warning */
//    (void) helper;
//
//    /* --- SSP1 pins --- */
//    (xPinConfig).Funcnum = SSP1_FUNCTION_NUMBER;
//    (xPinConfig).OpenDrain = 0;
//    (xPinConfig).Pinmode = PINSEL_PINMODE_TRISTATE;
//    (xPinConfig).Portnum = SSP1_SCK_PORT;
//    (xPinConfig).Pinnum = SSP1_SCK_PIN;
//    PINSEL_ConfigPin(&xPinConfig);
//
//    (xPinConfig).Pinnum = SSP1_MISO_PIN;
//    (xPinConfig).Portnum = SSP1_MISO_PORT;
//    PINSEL_ConfigPin(&xPinConfig);
//
//    (xPinConfig).Pinnum = SSP1_MOSI_PIN;
//    (xPinConfig).Portnum = SSP1_MOSI_PORT;
//    PINSEL_ConfigPin(&xPinConfig);
//
//    /* --- Chip selects --- */
//    (xPinConfig).Funcnum = 0;
//    (xPinConfig).Portnum = FLASH1_CS1_PORT;
//    (xPinConfig).Pinnum = FLASH1_CS1_PIN;
//    PINSEL_ConfigPin(&(xPinConfig));
//    GPIO_SetDir(FLASH1_CS1_PORT, (1 << FLASH1_CS1_PIN), 1);
//    GPIO_SetValue(FLASH1_CS1_PORT, 1 << FLASH1_CS1_PIN);
//
//    (xPinConfig).Portnum = FLASH1_CS2_PORT;
//    (xPinConfig).Pinnum = FLASH1_CS2_PIN;
//    PINSEL_ConfigPin(&(xPinConfig));
//    GPIO_SetDir(FLASH1_CS2_PORT, (1 << FLASH1_CS2_PIN), 1);
//    GPIO_SetValue(FLASH1_CS2_PORT, 1 << FLASH1_CS2_PIN);
//
//    (xPinConfig).Portnum = SD_CS_PORT;
//    (xPinConfig).Pinnum = SD_CS_PIN;
//    PINSEL_ConfigPin(&(xPinConfig));
//    GPIO_SetDir(SD_CS_PORT, (1 << SD_CS_PIN), 1);
//    GPIO_SetValue(SD_CS_PORT, 1 << SD_CS_PIN);
//
//    (xPinConfig).Portnum = MPU_CS_PORT;
//    (xPinConfig).Pinnum = MPU_CS_PIN;
//    PINSEL_ConfigPin(&(xPinConfig));
//    GPIO_SetDir(MPU_CS_PORT, (1 << MPU_CS_PIN), 1);
//    GPIO_SetValue(MPU_CS_PORT, 1 << MPU_CS_PIN);
//
//    SSP_ConfigStructInit(&sspInitialization);
//
//    sspInitialization.CPHA = SSP_CPHA_FIRST;
//    sspInitialization.CPOL = SSP_CPOL_HI;
//    sspInitialization.ClockRate = 2000000;
//    sspInitialization.FrameFormat = SSP_FRAME_SPI;
//    sspInitialization.Databit = SSP_DATABIT_8;
//    sspInitialization.Mode = SSP_MASTER_MODE;
//
//    SSP_Init(LPC_SSP1, &sspInitialization);
//
//    SSP_LoopBackCmd(LPC_SSP1, DISABLE);
//
//    SSP_Cmd(LPC_SSP1, ENABLE);
//
//    while ((LPC_SSP1->SR & SSP_SR_RNE) != 0) /* Flush RX FIFO */
//    {
//        helper = LPC_SSP1->DR;
//    }
//
//    SSP_IntConfig(LPC_SSP1, SSP_INTCFG_RT, ENABLE);
//    SSP_IntConfig(LPC_SSP1, SSP_INTCFG_ROR, ENABLE);
//    SSP_IntConfig(LPC_SSP1, SSP_INTCFG_RX, ENABLE);
//
//    /* Clear interrupt flags */
//    LPC_SSP1->ICR = SSP_INTCLR_ROR;
//    LPC_SSP1->ICR = SSP_INTCLR_RT;
//
//    /* Reset buffers to default values */
//    ssp1_jobs.current_job = 0;
//    ssp1_jobs.jobs_pending = 0;
//    ssp1_jobs.last_job_added = 0;
//
//    NVIC_SetPriority(SSP1_IRQn, SSP1_INTERRUPT_PRIORITY);
//    NVIC_EnableIRQ (SSP1_IRQn);
//
//    obc_error_counters.ssp1_error_counter = 0;
//    obc_status.ssp1_initialized = 1;
//    return;
//}
//
//void SSP1_IRQHandler(void)
//{
//    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//    uint32_t int_src = LPC_SSP1->RIS; /* Get interrupt source */
//    volatile uint32_t helper;
//
//    if (int_src == SSP_MIS_TX)
//    {
//        /* TX buffer half empty interrupt is not used but may occur */
//        return;
//    }
//
//    if ((int_src & SSP_INTSTAT_ROR))
//    {
//        LPC_SSP1->ICR = SSP_INTCLR_ROR;
//        obc_error_counters.ssp1_error_counter++;
//        obc_status_extended.ssp1_interrupt_ror = 1;
//        return;
//    }
//
//    if ((int_src & SSP_INTSTAT_RT))	// Clear receive timeout
//    {
//        LPC_SSP1->ICR = SSP_INTCLR_RT;
//    }
//
//    if (ssp1_jobs.job[ssp1_jobs.current_job].dir)
//    {
//        /* --- TX ------------------------------------------------------------------------------------------------------------------------ */
//
//        // Dump RX
//        while ((LPC_SSP1->SR & SSP_SR_RNE) != 0) /* Flush RX FIFO */
//        {
//            helper = LPC_SSP1->DR;
//        }
//
//        /* Fill TX FIFO */
//
//        if ((ssp1_jobs.job[ssp1_jobs.current_job].bytes_to_send - ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent) > 7)
//        {
//
//            helper = ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent + 7;
//        }
//        else
//        {
//            helper = ssp1_jobs.job[ssp1_jobs.current_job].bytes_to_send;
//        }
//
//        while (((LPC_SSP1->SR & SSP_STAT_TXFIFO_NOTFULL)) && (ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent < helper))
//        {
//            LPC_SSP1->DR = ssp1_jobs.job[ssp1_jobs.current_job].array_to_send[ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent];
//            ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent++;
//        }
//
//        if (LPC_SSP1->SR & SSP_SR_BSY)
//            return;
//
//        if ((ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent == ssp1_jobs.job[ssp1_jobs.current_job].bytes_to_send))
//        {
//            /* TX done */
//            /* Check if job includes SSP read */
//            if (ssp1_jobs.job[ssp1_jobs.current_job].bytes_to_read > 0)
//            {
//                /* RX init */
//                ssp1_jobs.job[ssp1_jobs.current_job].dir = 0; /* set to read */
//                while ((LPC_SSP1->SR & SSP_SR_RNE) != 0) /* Flush RX FIFO */
//                {
//                    helper = LPC_SSP1->DR;
//                }
//
//                ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent = 0;
//
//                if ((ssp1_jobs.job[ssp1_jobs.current_job].bytes_to_read - ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent) > 7)
//                {
//
//                    helper = ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent + 7;
//                }
//                else
//                {
//                    helper = ssp1_jobs.job[ssp1_jobs.current_job].bytes_to_read;
//                }
//
//                while (((LPC_SSP1->SR & SSP_STAT_TXFIFO_NOTFULL)) && (ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent < helper))
//                {
//                    LPC_SSP1->DR = 0xFF;
//                    ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent++;
//                }
//
//                helper = 0;
//                /* Wait for interrupt*/
//            }
//            else
//            {
//                /* transfer done */
//                /* release chip select and return */
//
//                helper = 0;
//                while ((LPC_SSP1->SR & SSP_SR_BSY) && (helper < 100000))
//                {
//                    /* Wait for SSP to finish transmission */
//                    helper++;
//                }
//
//                switch (ssp1_jobs.job[ssp1_jobs.current_job].device)
//                    /* Unselect device */
//                {
//
//                case SSP1_DEV_FLASH1_1:
//                    GPIO_SetValue(FLASH1_CS1_PORT, 1 << FLASH1_CS1_PIN);
//                    xSemaphoreGiveFromISR(flash1_semaphore, &xHigherPriorityTaskWoken);
//                    break;
//
//                case SSP1_DEV_FLASH1_2:
//                    GPIO_SetValue(FLASH1_CS2_PORT, 1 << FLASH1_CS2_PIN);
//                    xSemaphoreGiveFromISR(flash1_semaphore, &xHigherPriorityTaskWoken);
//                    break;
//
//                case SSP1_DEV_MPU:
//                    GPIO_SetValue(MPU_CS_PORT, 1 << MPU_CS_PIN);
//                    break;
//                default: /* Device does not exist */
//                    /* Release all devices */
//                    obc_status_extended.ssp1_interrupt_unknown_device = 1;
//                    obc_error_counters.ssp1_error_counter++;
//                    GPIO_SetValue(FLASH1_CS2_PORT, 1 << FLASH1_CS2_PIN);
//                    GPIO_SetValue(FLASH1_CS1_PORT, 1 << FLASH1_CS1_PIN);
//                    GPIO_SetValue(MPU_CS_PORT, 1 << MPU_CS_PIN);
//                    break;
//                }
//
//                ssp1_jobs.job[ssp1_jobs.current_job].status = SSP_JOB_STATE_DONE;
//            }
//        }
//
//    }
//    else
//    {
//        /* --- RX ------------------------------------------------------------------------------------------------------------------------ */
//
//        /* Read from RX FIFO */
//
//        while ((LPC_SSP1->SR & SSP_STAT_RXFIFO_NOTEMPTY)
//                && (ssp1_jobs.job[ssp1_jobs.current_job].bytes_read < ssp1_jobs.job[ssp1_jobs.current_job].bytes_to_read))
//        {
//            ssp1_jobs.job[ssp1_jobs.current_job].array_to_read[ssp1_jobs.job[ssp1_jobs.current_job].bytes_read] = LPC_SSP1->DR;
//            ssp1_jobs.job[ssp1_jobs.current_job].bytes_read++;
//        }
//
//        if (ssp1_jobs.job[ssp1_jobs.current_job].bytes_read == ssp1_jobs.job[ssp1_jobs.current_job].bytes_to_read)
//        {
//            /* All bytes read */
//
//            helper = 0;
//            while ((LPC_SSP1->SR & SSP_SR_BSY) && (helper < 100000))
//            {
//                /* Wait for SSP to finish transmission */
//                helper++;
//            }
//
//            switch (ssp1_jobs.job[ssp1_jobs.current_job].device)
//                /* Unselect device */
//            {
//
//            case SSP1_DEV_FLASH1_1:
//                GPIO_SetValue(FLASH1_CS1_PORT, 1 << FLASH1_CS1_PIN);
//                xSemaphoreGiveFromISR(flash1_semaphore, &xHigherPriorityTaskWoken);
//                break;
//
//            case SSP1_DEV_FLASH1_2:
//                GPIO_SetValue(FLASH1_CS2_PORT, 1 << FLASH1_CS2_PIN);
//                xSemaphoreGiveFromISR(flash1_semaphore, &xHigherPriorityTaskWoken);
//                break;
//
//            case SSP1_DEV_MPU:
//                GPIO_SetValue(MPU_CS_PORT, 1 << MPU_CS_PIN);
//                break;
//
//            default: /* Device does not exist */
//                /* Release all devices */
//                obc_error_counters.ssp1_error_counter++;
//                obc_status_extended.ssp1_interrupt_unknown_device = 1;
//                GPIO_SetValue(FLASH1_CS2_PORT, 1 << FLASH1_CS2_PIN);
//                GPIO_SetValue(FLASH1_CS1_PORT, 1 << FLASH1_CS1_PIN);
//                GPIO_SetValue(MPU_CS_PORT, 1 << MPU_CS_PIN);
//
//                break;
//            }
//
//            ssp1_jobs.job[ssp1_jobs.current_job].status = SSP_JOB_STATE_DONE;
//        }
//        else
//        {
//            /* not all bytes read - send dummy data again */
//
//            if ((ssp1_jobs.job[ssp1_jobs.current_job].bytes_to_read - ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent) > 7)
//            {
//
//                helper = ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent + 7;
//            }
//            else
//            {
//                helper = ssp1_jobs.job[ssp1_jobs.current_job].bytes_to_read;
//            }
//
//            while ((LPC_SSP1->SR & SSP_STAT_TXFIFO_NOTFULL) && (ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent < helper))
//            {
//                LPC_SSP1->DR = 0xFF;
//                ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent++;
//            }
//        }
//    }
//
//    if (ssp1_jobs.job[ssp1_jobs.current_job].status == SSP_JOB_STATE_DONE)
//    {
//        /* Job is done, increment to next job and execute if pending */
//
//        ssp1_jobs.current_job++;
//        ssp1_jobs.jobs_pending--;
//
//        if (ssp1_jobs.current_job == SPI_MAX_JOBS)
//        {
//            ssp1_jobs.current_job = 0;
//        }
//
//        while ((LPC_SSP1->SR & SSP_SR_RNE) != 0) /* Flush RX FIFO */
//        {
//            helper = LPC_SSP1->DR;
//        }
//
//        /* Check if jobs are pending */
//        if (ssp1_jobs.jobs_pending > 0)
//        {
//            switch (ssp1_jobs.job[ssp1_jobs.current_job].device)
//                /* Select device */
//            {
//            case SSP1_DEV_FLASH1_1:
//                GPIO_ClearValue(FLASH1_CS1_PORT, 1 << FLASH1_CS1_PIN);
//                break;
//
//            case SSP1_DEV_FLASH1_2:
//                GPIO_ClearValue(FLASH1_CS2_PORT, 1 << FLASH1_CS2_PIN);
//                break;
//
//            case SSP1_DEV_MPU:
//                GPIO_ClearValue(MPU_CS_PORT, 1 << MPU_CS_PIN);
//                break;
//
//            default: /* Device does not exist */
//                obc_error_counters.ssp1_error_counter++;
//                GPIO_SetValue(FLASH1_CS1_PORT, 1 << FLASH1_CS1_PIN);
//                GPIO_SetValue(FLASH1_CS2_PORT, 1 << FLASH1_CS2_PIN);
//                GPIO_SetValue(MPU_CS_PORT, 1 << MPU_CS_PIN);
//
//                /* Set error description */
//                ssp1_jobs.job[ssp1_jobs.current_job].status = SSP_JOB_STATE_DEVICE_ERROR;
//
//                /* Increment to next job */
//                ssp1_jobs.current_job++;
//                ssp1_jobs.jobs_pending--;
//
//                if (ssp1_jobs.current_job == SPI_MAX_JOBS)
//                {
//                    ssp1_jobs.current_job = 0;
//                }
//
//                return;
//
//                break;
//
//            }
//
//            ssp1_jobs.job[ssp1_jobs.current_job].status = SSP_JOB_STATE_ACTIVE;
//
//            /* Fill FIFO */
//            if (ssp1_jobs.job[ssp1_jobs.current_job].dir)
//            {
//                /* TX (+RX) */
//                if ((ssp1_jobs.job[ssp1_jobs.current_job].bytes_to_send - ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent) > 7)
//                {
//                    helper = ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent + 7;
//                }
//                else
//                {
//                    helper = ssp1_jobs.job[ssp1_jobs.current_job].bytes_to_send;
//                }
//
//                while (((LPC_SSP1->SR & SSP_STAT_TXFIFO_NOTFULL)) && (ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent < helper))
//                {
//                    LPC_SSP1->DR = ssp1_jobs.job[ssp1_jobs.current_job].array_to_send[ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent];
//                    ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent++;
//                }
//            }
//            else
//            {
//                /* RX only - send dummy data for clock output */
//
//                ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent = 0; /* Use unused bytes_sent for counting sent dummy data bytes */
//
//                if ((ssp1_jobs.job[ssp1_jobs.current_job].bytes_to_read - ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent) > 7)
//                {
//
//                    helper = ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent + 7;
//                }
//                else
//                {
//                    helper = ssp1_jobs.job[ssp1_jobs.current_job].bytes_to_read;
//                }
//
//                while (((LPC_SSP1->SR & SSP_STAT_TXFIFO_NOTFULL)) && (ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent < helper))
//                {
//                    LPC_SSP1->DR = 0xFF;
//                    ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent++;
//                }
//            }
//        }
//    }
//
//    /* If lHigherPriorityTaskWoken is now equal to pdTRUE, then a context
//     switch should be performed before the interrupt exists.  That ensures the
//     unblocked (higher priority) task is returned to immediately. */
//    //portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
//    return;
//
//    /* Max. execution time: 2863 cycles */
//    /* Average execution time: 626 cycles */
//
//}
//
//uint32_t ssp1_add_job(uint8_t sensor, uint8_t *array_to_send, uint16_t bytes_to_send, uint8_t *array_to_store, uint16_t bytes_to_read,
//                      uint8_t **job_status)
//{
//    uint32_t helper;
//    uint8_t position;
//
//    if (obc_status.ssp1_initialized == 0)
//    {
//        /* SSP is not initialized - return */
//        return SSP_JOB_NOT_INITIALIZED;
//    }
//
//    if (ssp1_jobs.jobs_pending >= SPI_MAX_JOBS)
//    {
//        /* Maximum amount of jobs stored, job can't be added! */
//        /* This is possibly caused by a locked interrupt -> remove all jobs and re-init SSP */
//        taskENTER_CRITICAL();
//        obc_error_counters.ssp1_error_counter++;
//        obc_status_extended.ssp1_buffer_overflow = 1;
//        ssp1_jobs.jobs_pending = 0; /* Delete jobs */
//        ssp1_init(); /* Reinit SSP */
//        taskEXIT_CRITICAL();
//        return SSP_JOB_BUFFER_OVERFLOW;
//    }
//
//    taskENTER_CRITICAL();
//    {
//        position = (ssp1_jobs.current_job + ssp1_jobs.jobs_pending) % SPI_MAX_JOBS;
//
//        ssp1_jobs.job[position].array_to_send = array_to_send;
//        ssp1_jobs.job[position].bytes_to_send = bytes_to_send;
//        ssp1_jobs.job[position].bytes_sent = 0;
//        ssp1_jobs.job[position].array_to_read = array_to_store;
//        ssp1_jobs.job[position].bytes_to_read = bytes_to_read;
//        ssp1_jobs.job[position].bytes_read = 0;
//        ssp1_jobs.job[position].device = sensor;
//        ssp1_jobs.job[position].status = SSP_JOB_STATE_PENDING;
//
//        if (bytes_to_send > 0)
//        {
//            /* Job contains transfer and read eventually */
//            ssp1_jobs.job[position].dir = 1;
//        }
//        else
//        {
//            /* Job contains readout only - transfer part is skipped */
//            ssp1_jobs.job[position].dir = 0;
//        }
//
//        /* Check if SPI in use */
//        if (ssp1_jobs.jobs_pending == 0)
//        { /* Check if jobs pending */
//            switch (ssp1_jobs.job[position].device)
//                /* Select device */
//            {
//            case SSP1_DEV_FLASH1_1:
//                GPIO_ClearValue(FLASH1_CS1_PORT, 1 << FLASH1_CS1_PIN);
//                break;
//
//            case SSP1_DEV_FLASH1_2:
//                GPIO_ClearValue(FLASH1_CS2_PORT, 1 << FLASH1_CS2_PIN);
//                break;
//
//            case SSP1_DEV_MPU:
//                GPIO_ClearValue(MPU_CS_PORT, 1 << MPU_CS_PIN);
//                break;
//
//            default: /* Device does not exist */
//                /* Unselect all device */
//                GPIO_SetValue(FLASH1_CS1_PORT, 1 << FLASH1_CS1_PIN);
//                GPIO_SetValue(FLASH1_CS2_PORT, 1 << FLASH1_CS2_PIN);
//                GPIO_SetValue(MPU_CS_PORT, 1 << MPU_CS_PIN);
//
//                /* Set error description */
//                obc_error_counters.ssp1_error_counter++;
//                ssp1_jobs.job[position].status = SSP_JOB_STATE_DEVICE_ERROR;
//
//                /* Increment to next job */
//                ssp1_jobs.current_job++;
//                ssp1_jobs.jobs_pending--;
//
//                if (ssp1_jobs.current_job == SPI_MAX_JOBS)
//                {
//                    ssp1_jobs.current_job = 0;
//                }
//
//                /* Return error */
//                taskEXIT_CRITICAL();
//                return SSP_JOB_ERROR;
//            }
//
//            ssp1_jobs.job[position].status = SSP_JOB_STATE_ACTIVE;
//
//            while ((LPC_SSP1->SR & SSP_SR_RNE) != 0) /* Flush RX FIFO */
//            {
//                helper = LPC_SSP1->DR;
//            }
//
//            /* Fill FIFO */
//
//            if (ssp1_jobs.job[position].dir)
//            {
//                /* TX (+RX) */
//
//                if ((ssp1_jobs.job[ssp1_jobs.current_job].bytes_to_send - ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent) > 7)
//                {
//
//                    helper = ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent + 7;
//                }
//                else
//                {
//                    helper = ssp1_jobs.job[ssp1_jobs.current_job].bytes_to_send;
//                }
//
//                while (((LPC_SSP1->SR & SSP_STAT_TXFIFO_NOTFULL)) && (ssp1_jobs.job[position].bytes_sent < helper))
//                {
//                    LPC_SSP1->DR = ssp1_jobs.job[position].array_to_send[ssp1_jobs.job[position].bytes_sent];
//                    ssp1_jobs.job[position].bytes_sent++;
//                }
//            }
//            else
//            {
//                /* RX only - send dummy data for clock output */
//                /* Use unused bytes_sent for counting sent dummy data bytes */
//
//                if ((ssp1_jobs.job[ssp1_jobs.current_job].bytes_to_read - ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent) > 7)
//                {
//
//                    helper = ssp1_jobs.job[ssp1_jobs.current_job].bytes_sent + 7;
//                }
//                else
//                {
//                    helper = ssp1_jobs.job[ssp1_jobs.current_job].bytes_to_read;
//                }
//
//                while (((LPC_SSP1->SR & SSP_STAT_TXFIFO_NOTFULL)) && (ssp1_jobs.job[position].bytes_sent < helper))
//                {
//                    LPC_SSP1->DR = 0xFF;
//                    ssp1_jobs.job[position].bytes_sent++;
//
//                }
//            }
//        }
//
//        ssp1_jobs.jobs_pending++;
//    }
//
//    /* Set pointer to job status if necessary */
//    if (job_status != NULL)
//    {
//        *job_status = (uint8_t *) &(ssp1_jobs.job[position].status);
//    }
//
//    taskEXIT_CRITICAL();
//    return SSP_JOB_ADDED; /* Job added successfully */
//}
//
