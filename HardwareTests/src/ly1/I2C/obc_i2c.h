/*
 * obc_i2c_int.h
 *
 *  Copied over from Pegasus Flight Software on: 2019-11-21
 */


#ifndef OBC_I2C_H
#define OBC_I2C_H

#define I2C_0_CLOCKRATE 25   	/* in kHz */
#define I2C_1_CLOCKRATE 50    	/* I2C onboard, in kHz */
#define I2C_2_CLOCKRATE	25		/* in kHz */

/* Onboard I2C adresses */

//#define I2C_ADR_ONBOARD_MPU 		0x68
//#define I2C_ADR_ONBOARD_MPU_MAG 	0x0C
//#define I2C_ADR_ONBOARD_TMP100 		0x48
//#define I2C_ADR_ONBOARD_MAG  		0x1E
//#define I2C_ADR_ONBOARD_MSP			0x24
#define I2C_ADR_EEPROM1				0x57
#define I2C_ADR_EEPROM2				0x53
#define I2C_ADR_EEPROM3				0x51

/* Panel's I2C addresses */
//#define I2C_ADR_SPA 				0x31	// X+
//#define I2C_ADR_SPB 				0x28	// Y-
//#define I2C_ADR_SPC 				0x31	// X-
//#define I2C_ADR_SPD 				0x28	// Y+
//#define I2C_ADR_BP					0x47	// SP-B/I2C2
//#define I2C_ADR_SA					0x32	// SP-B/I2C2

//#define I2C_ADR_EPS 	 		    0x55 	/* Address of EPS (CC1 & CC2)*/

//#define I2C_ADR_BP_MAG  			0x1E
//#define I2C_ADR_BP_MAG_BOOM 		0x1E	/* Different I2C-bus than BP_MAG */

#endif /* OBC_I2C_H */

