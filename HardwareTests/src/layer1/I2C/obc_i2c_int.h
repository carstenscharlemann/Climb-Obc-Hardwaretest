/*
 * obc_i2c_int.h
 *
 *  Copied over from Pegasus Flight Software on: 2019-11-21
 */

#ifndef OBC_I2C_INT_H
#define OBC_I2C_INT_H

#include <chip.h>
#include "obc_i2c.h"

//#define I2SCLH_SCLH			0x00000080  /* I2C SCL Duty Cycle High Reg */
//#define I2SCLL_SCLL			0x00000080  /* I2C SCL Duty Cycle Low Reg */

//#define I2C_PCLKSEL_4		0x0	/* PCLK = CCLK/4 */
#define I2C_PCLKSEL_1		0x1	/* PCLK = CCLK */
#define I2C_PCLKSEL_2		0x2 /* PCLK = CCLK/2 */
#define I2C_PCLKSEL_8		0x3 /* PCLK = CCLK/8 */

enum
{
	I2C_ERROR_NO_ERROR = 0, I2C_ERROR_RX_OVERFLOW, I2C_ERROR_BUS_ERROR, I2C_ERROR_SM_ERROR, I2C_ERROR_JOB_NOT_FINISHED
} i2c_errors_e;

//#define I2C_ERROR_RX_OVERFLOW 	0x01
//#define I2C_ERROR_BUS_ERROR   	0x02
//#define I2C_ERROR_SM_ERROR   	0x03

//typedef struct
//{
//	uint8_t tx_count;
//	uint8_t rx_count;
//	uint8_t dir;
//	uint8_t status;
//	uint8_t tx_size;
//	uint8_t rx_size;
//	uint8_t* tx_data;
//	uint8_t* rx_data;
//	uint8_t job_done;
//	uint8_t adress;
//	uint8_t error;
//	LPC_I2C_T *device;
//}volatile I2C_Data;

uint8_t i2c_add_job(I2C_Data* data);

void I2C_send();

//void init_i2c(LPC_I2C_T *I2Cx, uint32_t clockrate);

void I2C_Handler(LPC_I2C_T *I2Cx);

void I2C_stop(LPC_I2C_T *I2Cx);

void i2c_init_interrupts(void);

extern void I2C0_IRQHandler(void);
extern void I2C1_IRQHandler(void);
extern void I2C2_IRQHandler(void);

#endif
