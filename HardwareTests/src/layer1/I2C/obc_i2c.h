/*
 * obc_i2c_int.h
 *
 *  Copied over from Pegasus Flight Software on: 2019-11-21
 */


#ifndef OBC_I2C_H
#define OBC_I2C_H

//#define I2C_0_CLOCKRATE 25   	/* in kHz */
//#define I2C_1_CLOCKRATE 50    	/* I2C onboard, in kHz */
//#define I2C_2_CLOCKRATE	25		/* in kHz */

///* Onboard I2C adresses */
//#define I2C_ADR_EEPROM1				0x57
//#define I2C_ADR_EEPROM2				0x53
//#define I2C_ADR_EEPROM3				0x51
//
//#define I2C_ADR_TEMP				0x90
//#define I2C_ADR_FRAM				0x50

//enum
//{
//	I2C_ERROR_NO_ERROR = 0, I2C_ERROR_RX_OVERFLOW, I2C_ERROR_BUS_ERROR, I2C_ERROR_SM_ERROR, I2C_ERROR_JOB_NOT_FINISHED
//} i2c_errors_e;
//

typedef struct
{
	uint8_t tx_count;
	uint8_t rx_count;
	uint8_t dir;
	uint8_t status;
	uint8_t tx_size;
	uint8_t rx_size;
	uint8_t* tx_data;
	uint8_t* rx_data;
	uint8_t job_done;
	uint8_t adress;
	uint8_t error;
	LPC_I2C_T *device;
} volatile I2C_Data;


void init_i2c(LPC_I2C_T *I2Cx, uint32_t clockrate);

// Choose the Onboard I2C. Its one of the 3 available (on OBC: I2C1 on LPCX: I2C0)
#define ONBOARD_CLOCKRATE 50    	/* I2C onboard, in kHz */
#define InitOnboardI2C(I2Cx) 		init_i2c(I2Cx, ONBOARD_CLOCKRATE)

#endif /* OBC_I2C_H */

