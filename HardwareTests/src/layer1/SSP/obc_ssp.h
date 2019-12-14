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

enum
{ /* SSP1 sensors */
	SSP1_DEV_FLASH1_1, SSP1_DEV_FLASH1_2, SSP1_DEV_MPU
};

enum
{
	SSP0_DEV_FLASH2_1, SSP0_DEV_FLASH2_2
};

enum
{
	SSP_JOB_STATE_DONE = 0, SSP_JOB_STATE_PENDING, SSP_JOB_STATE_ACTIVE, SSP_JOB_STATE_SSP_ERROR, SSP_JOB_STATE_DEVICE_ERROR
};

enum
{ /* Return values for ssp_add_job */
	SSP_JOB_ADDED = 0, SSP_JOB_BUFFER_OVERFLOW, SSP_JOB_MALLOC_FAILED, SSP_JOB_ERROR, SSP_JOB_NOT_INITIALIZED
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

typedef struct tmp_status_s
{
	/* OBC status bits block 1 - 32 Bits */
	unsigned int ssp0_interrupt_ror :1; /* Bit 0 */
	unsigned int ssp1_interrupt_ror :1; /* Bit 1 */
	unsigned int ssp0_interrupt_unknown_device :1; /* Bit 2 */
	unsigned int ssp1_interrupt_unknown_device :1; /* Bit 3 */
	unsigned int ssp0_buffer_overflow :1; /* Bit 4 */
	unsigned int ssp1_buffer_overflow :1; /* Bit 5 */
	unsigned int ssp0_frequent_errors :1; /* Bit 6 */
	unsigned int ssp1_frequent_errors :1; /* Bit 7 */
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
	unsigned int flash_test_executed :1; /* Bit 23 */
	unsigned int flash_test_successful :1; /* Bit 24 */
	unsigned int flash_up_test_successful :1; /* Bit 25 */
	unsigned int :1; /* Bit 26 */
	unsigned int :1; /* Bit 27 */
	unsigned int task_sensors_running :1; /* Bit 28 */
	unsigned int task_maintenance_running :1; /* Bit 29 */
	unsigned int statemachine_initialized :1; /* Bit 30 */
	unsigned int rtc_synchronized :1; /* Bit 31 */

	/* OBC status bits block 2 - 32 Bits */
	unsigned int i2c0_initialized :1; /* Bit 0 */
	unsigned int i2c1_initialized :1; /* Bit 1 */
	unsigned int i2c2_initialized :1; /* Bit 2 */
	unsigned int ssp0_initialized :1; /* Bit 3 */
	unsigned int ssp1_initialized :1; /* Bit 4 */
	unsigned int supply_switches_initialized :1; /* Bit 5 */
	unsigned int i2c_switches_initialized :1; /* Bit 6 */
	unsigned int rtc_initialized :1; /* Bit 7 */
	unsigned int adc_initialized :1; /* Bit 8 */
	unsigned int uart_gps_initialized :1; /* Bit 9 */
	unsigned int uart_ttc2_initialized :1; /* Bit 10 */
	unsigned int uart_mnlp_initialized :1; /* Bit 11 */
	unsigned int uart_ttc1_initialized :1; /* Bit 12 */
	unsigned int timer0_initialized :1; /* Bit 13 */
	unsigned int watchdog_initialized :1; /* Bit 14 */
	unsigned int timer1_initialized :1; /* Bit 15 */
	unsigned int eps_cc1_operational :1; /* Bit 16 */
	unsigned int eps_cc2_operational :1; /* Bit 17 */
	unsigned int eeprom1_initialized :1; /* Bit 18 */
	unsigned int eeprom2_initialized :1; /* Bit 19 */
	unsigned int eeprom3_initialized :1; /* Bit 20 */
	unsigned int mag_bp_initialized :1; /* Bit 21 */
	unsigned int mag_bp_boom_initialized :1; /* Bit 22 */
	unsigned int gyro1_initialized :1; /* Bit 23 */
	unsigned int gyro2_initialized :1; /* Bit 24 */
	unsigned int msp_initialized :1; /* Bit 25 */
	unsigned int onboard_mag_initialized :1; /* Bit 26 */
	unsigned int onboard_tmp100_initialized :1; /* Bit 27 */
	unsigned int mpu_initialized :1; /* Bit 28 */
	unsigned int gps_active :1; /* Bit 29 */
	unsigned int flash1_initialized :1; /* Bit 30 */
	unsigned int flash2_initialized :1; /* Bit 31 */

	/* OBC status bits block 3 - 32 Bits */
	unsigned int spa_initialized :1; /* Bit 0 */
	unsigned int spb_initialized :1; /* Bit 1 */
	unsigned int spc_initialized :1; /* Bit 2 */
	unsigned int spd_initialized :1; /* Bit 3 */
	unsigned int sa_initialized :1; /* Bit 4 *//* Science adapter */
	unsigned int bp_initialized :1; /* Bit 5 */
	unsigned int :1; /* Bit 6 */
	unsigned int gps_initialized :1; /* Bit 7 */
	unsigned int ttc1_initialized :1; /* Bit 8 */
	unsigned int ttc2_initialized :1; /* Bit 9 */
	unsigned int science_module_initialized :1; /* Bit 10 */
	unsigned int spa_vcc_on :1; /* Bit 11 */
	unsigned int spb_vcc_on :1; /* Bit 12 */
	unsigned int spc_vcc_on :1; /* Bit 13 */
	unsigned int spd_vcc_on :1; /* Bit 14 */
	unsigned int bp1_vcc_on :1; /* Bit 15 */
	unsigned int bp2_vcc_on :1; /* Bit 16 */
	unsigned int sa_vcc_on :1; /* Bit 17 */
	unsigned int i2c_sw_a_on :1; /* Bit 18 */
	unsigned int i2c_sw_b_on :1; /* Bit 19 */
	unsigned int i2c_sw_c_on :1; /* Bit 20 */
	unsigned int i2c_sw_d_on :1; /* Bit 21 */
	unsigned int onboard_mag_powersafe :1; /* Bit 22 */
	unsigned int gyro_powesafe :1; /* Bit 23 */
	unsigned int mpu_powersafe :1; /* Bit 24 */
	unsigned int tmp100_powersafe :1; /* Bit 25 */
	unsigned int mag_bp_powersave :1; /* Bit 26 */
	unsigned int mag_bp_boom_powersave :1; /* Bit 27 */
	unsigned int mnlp_5v_enabled :1; /* Bit 28 */
	unsigned int power_source :1; /* Bit 29 */
	unsigned int :1; /* Bit 30 */
	unsigned int :1; /* Bit 31 */

	/* Status block 4 */
	uint8_t error_code;
	uint8_t error_code_previous;
	uint8_t error_code_before_reset;
	uint8_t error_code_msp;

	uint8_t ssp0_error_counter;
	uint8_t ssp1_error_counter;
}
volatile tmp_status_t;



void ssp1_init(void);
void SSP1_IRQHandler(void);
uint32_t ssp1_add_job(uint8_t sensor, uint8_t *array_to_send, uint16_t bytes_to_send, uint8_t *array_to_store, uint16_t bytes_to_read,
		uint8_t **job_status);

void ssp0_init(void);
void SSP0_IRQHandler(void);
uint32_t ssp0_add_job(uint8_t sensor, uint8_t *array_to_send, uint16_t bytes_to_send, uint8_t *array_to_store, uint16_t bytes_to_read,
		uint8_t **job_status);

#endif /* STC_SPI_H_ */
