/*
 * obc_i2c_int.c
 *
 *  Copied over from Pegasus Flight Software on: 2019-11-21
 */

#include <stdio.h>
#include <string.h>

#include "chip.h"

#include "obc_i2c_int.h"
#include "obc_i2c_rb.h"
#include "obc_i2c.h"

//#include "FreeRTOS.h"
//#include "task.h"
//#include "queue.h"
//#include "semphr.h"

//#include "main.h"

#define DEBUG_MODE 0

I2C_RB I2C_buffer[3];
I2C_Data* I2C_active[3];

uint8_t active_job_done[3];

uint32_t PCLK[3];


typedef struct obc_status_s
{
	/* OBC status bits block 1 - 32 Bits */
	unsigned int crystal_oszillator_used :1; /* Bit 0 */
	unsigned int reset_by_eps_watchdog :1; /* Bit 1 */
	unsigned int last_reset_source1 :1; /* Bit 2 *//* (source1 source2) POR: 0b00, EXTR: 0b01, WDTR: 0b10; BODR: 0b11 */
	unsigned int last_reset_source2 :1; /* Bit 3 */
	unsigned int eps_cc_used :1; /* Bit 4 *//* 0..CC1, 1..CC2 */
	unsigned int rbf_inserted :1; /* Bit 5 *//* Remove before flight pin is present */
	unsigned int obc_powersave :1; /* Bit 6 */
	unsigned int obc_3v3_spa_enabled :1; /* Bit 7 */
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

}
volatile obc_status_t;
typedef struct obc_error_counters_s
{
	/* OBC error and overflow counters */
	/* --- Block 1 --- */
	uint8_t i2c0_error_counter;
	uint8_t i2c1_error_counter;
	uint8_t i2c2_error_counter;
	uint8_t adc_error_counter;

	/* --- Block 2 --- */
	uint8_t uart_gps_error_counter;
	uint8_t uart_mnlp_error_counter;
	uint8_t uart_ttc1_error_counter;
	uint8_t uart_ttc2_error_counter;

	/* --- Block 3 --- */
	uint8_t ssp0_error_counter;
	uint8_t ssp1_error_counter;
	uint8_t _unused;
	uint8_t _unused1;

	/* --- Block 4 --- */
	unsigned int i2c0_reinit_counter :2;
	unsigned int i2c1_reinit_counter :2;
	unsigned int i2c2_reinit_counter :2;
	unsigned int ssp0_reinit_counter :2;

	unsigned int ssp1_reinit_counter :2;
	unsigned int uart_gps_reinit_counter :2;
	unsigned int uart_mnlp_reinit_counter :2;
	unsigned int uart_ttc1_reinit_counter :2;

	unsigned int uart_ttc2_reinit_counter :2;
	unsigned int _unused2 :6;

	uint8_t _unused_3;

}
volatile obc_error_counters_t;

obc_status_t obc_status;
obc_error_counters_t obc_error_counters;


void I2C0_IRQHandler(void)
{
    I2C_Handler (LPC_I2C0);
}

void I2C1_IRQHandler(void)
{
    I2C_Handler (LPC_I2C1);
}

void I2C2_IRQHandler(void)
{
    I2C_Handler (LPC_I2C2);
}

//
//void I2CInit(LPC_I2C_T* i2cPtr, uint32_t clockrate, LPC175X_6X_IRQn_Type irq, int idx) {
//	int SCL;
//
//	active_job_done[0] = 1;
//
//	//LPC_SC->PCONP |= (1 << 7);
//	LPC_SYSCTL->PCONP |= (1 << SYSCTL_CLOCK_I2C0);		// TODO: move to board init
//	LPC_SYSCTL->PCONP |= (1 << SYSCTL_CLOCK_I2C1);		// TODO: move to board init
//	LPC_SYSCTL->PCONP |= (1 << SYSCTL_CLOCK_I2C2);		// TODO: move to board init
//
//	/* set PIO0.27 and PIO0.28 to I2C0 SDA and SCL */
//	/* function to 01 on both SDA and SCL. */
//	//LPC_PINCON->PINSEL1 &= ~((0x03 << 22) | (0x03 << 24));
//	//LPC_PINCON->PINSEL1 |= ((0x01 << 22) | (0x01 << 24));
//	// -> moved to board init PINMUX_GRP_T pinmuxing[]
//	//			{0, 27, IOCON_MODE_INACT | IOCON_FUNC1},	/* I2C0 SDA no Pull Up/down */
//	//			{0, 28, IOCON_MODE_INACT | IOCON_FUNC1},	/* I2C0 SCL  no Pull Up/down */
//
//
//	/*--- Clear flags ---*/
//	//LPC_I2C0->I2CONCLR = I2C_I2CONCLR_AAC | I2C_I2CONCLR_SIC | I2C_I2CONCLR_STAC | I2C_I2CONCLR_I2ENC;
//	LPC_I2C0->CONCLR = I2C_I2CONCLR_AAC | I2C_I2CONCLR_SIC | I2C_I2CONCLR_STAC | I2C_I2CONCLR_I2ENC;
//
//	/*--- Reset registers ---*/
//	/*#if FAST_MODE_PLUS
//	 LPC_PINCON->I2CPADCFG |= ((0x1<<0)|(0x1<<2));
//	 LPC_I2C0->SCLL   = I2SCLL_HS_SCLL;
//	 LPC_I2C0->SCLH   = I2SCLH_HS_SCLH;
//	 #else
//	 LPC_PINCON->I2CPADCFG &= ~((0x1<<0)|(0x1<<2));
//	 LPC_I2C0->I2SCLL   = I2SCLL_SCLL;
//	 LPC_I2C0->I2SCLH   = I2SCLH_SCLH;
//	 #endif*/
//	SCL = PCLK[0] / (2 * clockrate * 1000); /*duty cycle wenn low und high gleich lang ist. (Siehe user manual seite 448) */
//
//	if (SCL < 4)
//		SCL = 4; /*SCL darf nicht kleiner als 4 werden (siehe user manual) */
//
//	//LPC_I2C0->I2SCLL = SCL;
//	//LPC_I2C0->I2SCLH = SCL;
//	LPC_I2C0->SCLL = SCL;
//	LPC_I2C0->SCLH = SCL;
//
//	/* Install interrupt handler */
//	NVIC_EnableIRQ (I2C0_IRQn);
//	NVIC_SetPriority(I2C0_IRQn, I2C0_INTERRUPT_PRIORITY);
//	//LPC_I2C0->I2CONSET = I2C_I2CONSET_I2EN;
//	LPC_I2C0->CONSET = I2C_I2CONSET_I2EN;
//
//obc_error_counters.i2c0_error_counter = 0;
//obc_status.i2c0_initialized = 1;
//	return;
//}

/*****************************************************************************
 ** Function name:		I2CInit
 **
 ** Descriptions:		Initialize I2C controller as a master
 **
 ** parameters:			None
 ** Returned value:		None
 **
 *****************************************************************************/
void I2C0Init(uint32_t clockrate)
{
    int SCL;

    active_job_done[0] = 1;

    //LPC_SC->PCONP |= (1 << 7);
    LPC_SYSCTL->PCONP |= (1 << SYSCTL_CLOCK_I2C0);

    /* set PIO0.27 and PIO0.28 to I2C0 SDA and SCL */
    /* function to 01 on both SDA and SCL. */
    //LPC_PINCON->PINSEL1 &= ~((0x03 << 22) | (0x03 << 24));
    //LPC_PINCON->PINSEL1 |= ((0x01 << 22) | (0x01 << 24));
    // -> moved to board init PINMUX_GRP_T pinmuxing[]
    //			{0, 27, IOCON_MODE_INACT | IOCON_FUNC1},	/* I2C0 SDA no Pull Up/down */
	//			{0, 28, IOCON_MODE_INACT | IOCON_FUNC1},	/* I2C0 SCL  no Pull Up/down */


    /*--- Clear flags ---*/
    //LPC_I2C0->I2CONCLR = I2C_I2CONCLR_AAC | I2C_I2CONCLR_SIC | I2C_I2CONCLR_STAC | I2C_I2CONCLR_I2ENC;
    LPC_I2C0->CONCLR = I2C_I2CONCLR_AAC | I2C_I2CONCLR_SIC | I2C_I2CONCLR_STAC | I2C_I2CONCLR_I2ENC;

    /*--- Reset registers ---*/
    /*#if FAST_MODE_PLUS
     LPC_PINCON->I2CPADCFG |= ((0x1<<0)|(0x1<<2));
     LPC_I2C0->SCLL   = I2SCLL_HS_SCLL;
     LPC_I2C0->SCLH   = I2SCLH_HS_SCLH;
     #else
     LPC_PINCON->I2CPADCFG &= ~((0x1<<0)|(0x1<<2));
     LPC_I2C0->I2SCLL   = I2SCLL_SCLL;
     LPC_I2C0->I2SCLH   = I2SCLH_SCLH;
     #endif*/
    SCL = PCLK[0] / (2 * clockrate * 1000); /*duty cycle wenn low und high gleich lang ist. (Siehe user manual seite 448) */

    if (SCL < 4)
        SCL = 4; /*SCL darf nicht kleiner als 4 werden (siehe user manual) */

    //LPC_I2C0->I2SCLL = SCL;
    //LPC_I2C0->I2SCLH = SCL;
    LPC_I2C0->SCLL = SCL;
    LPC_I2C0->SCLH = SCL;

    /* Install interrupt handler */
    NVIC_EnableIRQ (I2C0_IRQn);

// This is somehow configured via RTOS heqader files ....
#define configMAX_LIBRARY_INTERRUPT_PRIORITY    ( 5 )
#define I2C0_INTERRUPT_PRIORITY         (configMAX_LIBRARY_INTERRUPT_PRIORITY + 4)  /* I2C */
#define I2C1_INTERRUPT_PRIORITY         (configMAX_LIBRARY_INTERRUPT_PRIORITY + 3)  /* I2C */ //
#define I2C2_INTERRUPT_PRIORITY         (configMAX_LIBRARY_INTERRUPT_PRIORITY + 4)  /* I2C */


    NVIC_SetPriority(I2C0_IRQn, I2C0_INTERRUPT_PRIORITY);
    //LPC_I2C0->I2CONSET = I2C_I2CONSET_I2EN;
    LPC_I2C0->CONSET = I2C_I2CONSET_I2EN;

    obc_error_counters.i2c0_error_counter = 0;
    obc_status.i2c0_initialized = 1;
    return;
}

/*****************************************************************************
 ** Function name:		I2C1Init
 **
 ** Descriptions:		Initialize I2C controller as a master
 **
 ** parameters:			None
 ** Returned value:		None
 **
 *****************************************************************************/
void I2C1Init(uint32_t clockrate)
{
    int SCL;

//    obc_status.i2c1_initialized = 0;

    active_job_done[1] = 1;

 //   LPC_SC->PCONP |= (1 << 19);
    LPC_SYSCTL->PCONP |= (1 << SYSCTL_CLOCK_I2C1);


//#if 0
//    /* set PIO0.0 and PIO0.1 to I2C1 SDA and SCL */
//    /* function to 11 on both SDA and SCL. */
//    LPC_PINCON->PINSEL0 &= ~((0x3 << 0) | (0x3 << 2));
//    LPC_PINCON->PINSEL0 |= ((0x3 << 0) | (0x3 << 2));
//    LPC_PINCON->PINMODE0 &= ~((0x3 << 0) | (0x3 << 2));
//    LPC_PINCON->PINMODE0 |= ((0x2 << 0) | (0x2 << 2)); /* No pull-up no pull-down */
//    LPC_PINCON->PINMODE_OD0 |= ((0x01 << 0) | (0x1 << 1)); /* Open drain */
//#endif
//#if 1
    /* set PIO0.19 and PIO0.20 to I2C1 SDA and SCL */
    /* function to 11 on both SDA and SCL. */
//    LPC_PINCON->PINSEL1 &= ~((0x3 << 6) | (0x3 << 8));
//    LPC_PINCON->PINSEL1 |= ((0x3 << 6) | (0x3 << 8));
//    LPC_PINCON->PINMODE1 &= ~((0x3 << 6) | (0x3 << 8));
//    LPC_PINCON->PINMODE1 |= ((0x2 << 6) | (0x2 << 8)); /* No pull-up no pull-down */
//  -> moved to board init PINMUX_GRP_T pinmuxing[]
//			{0, 19, IOCON_MODE_INACT | IOCON_FUNC2},	/* I2C1 SDA */
//			{0, 20, IOCON_MODE_INACT | IOCON_FUNC2},	/* I2C1 SCL */

    //LPC_PINCON->PINMODE_OD0 |= ((0x1 << 19) | (0x1 << 20));
    Chip_IOCON_EnableOD(LPC_IOCON, 0, 19);
    Chip_IOCON_EnableOD(LPC_IOCON, 0, 20);


//#endif

    /*--- Clear flags ---*/
    LPC_I2C1->CONCLR = I2C_I2CONCLR_AAC | I2C_I2CONCLR_SIC | I2C_I2CONCLR_STAC | I2C_I2CONCLR_I2ENC;

    /*--- Reset registers ---*/
    /*LPC_I2C1->I2SCLL   = I2SCLL_SCLL; */
    /*LPC_I2C1->I2SCLH   = I2SCLH_SCLH; */
    SCL = PCLK[1] / (2 * clockrate * 1000); /*duty cycle wenn low und high gleich lang ist. (Siehe user manual seite 448) */

    if (SCL < 4)
        SCL = 4; /*SCL darf nicht kleiner als 4 werden (siehe user manual) */

    LPC_I2C1->SCLL = SCL;
    LPC_I2C1->SCLH = SCL;

    /* Install interrupt handler */
    NVIC_EnableIRQ (I2C1_IRQn);
    NVIC_SetPriority(I2C1_IRQn, I2C1_INTERRUPT_PRIORITY);
    LPC_I2C1->CONSET = I2C_I2CONSET_I2EN;

    obc_error_counters.i2c1_error_counter = 0;
    obc_status.i2c1_initialized = 1;
    return;
}

/*****************************************************************************
 ** Function name:		I2C2Init
 **
 ** Descriptions:		Initialize I2C controller as a master
 **
 ** parameters:			None
 ** Returned value:		None
 **
 *****************************************************************************/
void I2C2Init(uint32_t clockrate)
{
    int SCL;

    //obc_status.i2c2_initialized = 0;

    active_job_done[2] = 1;

    //LPC_SC->PCONP |= (1 << 26);
    LPC_SYSCTL->PCONP |= (1 << SYSCTL_CLOCK_I2C2);

    /* set PIO0.10 and PIO0.11 to I2C2 SDA and SCL */
    /* function to 10 on both SDA and SCL. */
//    LPC_PINCON->PINSEL0 &= ~((0x03 << 20) | (0x03 << 22));
//    LPC_PINCON->PINSEL0 |= ((0x02 << 20) | (0x02 << 22));
//    LPC_PINCON->PINMODE0 &= ~((0x03 << 20) | (0x03 << 22));
//    LPC_PINCON->PINMODE0 |= ((0x02 << 20) | (0x2 << 22)); /* No pull-up no pull-down */

    //  -> moved to board init PINMUX_GRP_T pinmuxing[]
	// {0, 10, IOCON_MODE_INACT | IOCON_FUNC2},	/* I2C2 SDA */
	// {0, 11, IOCON_MODE_INACT | IOCON_FUNC2},	/* I2C2 SCL */

    //LPC_PINCON->PINMODE_OD0 |= ((0x01 << 10) | (0x1 << 11));
    Chip_IOCON_EnableOD(LPC_IOCON, 0, 10);
    Chip_IOCON_EnableOD(LPC_IOCON, 0, 11);

    /*--- Clear flags ---*/
    LPC_I2C2->CONCLR = I2C_I2CONCLR_AAC | I2C_I2CONCLR_SIC | I2C_I2CONCLR_STAC | I2C_I2CONCLR_I2ENC;

    /*--- Reset registers ---*/
    /*LPC_I2C2->I2SCLL   = I2SCLL_SCLL; */
    /*LPC_I2C2->I2SCLH   = I2SCLH_SCLH; */
    SCL = PCLK[2] / (2 * clockrate * 1000); /*duty cycle wenn low und high gleich lang ist. (Siehe user manual seite 448) */

    if (SCL < 4)
        SCL = 4; /*SCL darf nicht kleiner als 4 werden (siehe user manual) */

    LPC_I2C2->SCLL = SCL;
    LPC_I2C2->SCLH = SCL;

    /* Install interrupt handler */
    NVIC_EnableIRQ (I2C2_IRQn);
    NVIC_SetPriority(I2C2_IRQn, I2C2_INTERRUPT_PRIORITY);
    LPC_I2C2->CONSET = I2C_I2CONSET_I2EN;

    obc_error_counters.i2c2_error_counter = 0;
    obc_status.i2c2_initialized = 1;
    return;
}

/**********************************************************************
 * @brief		Convert from I2C peripheral to number
 * @param[in]	I2Cx: I2C peripheral selected, should be:
 * 				- LPC_I2C0
 * 				- LPC_I2C1
 * 				- LPC_I2C2
 * @return 		I2C number, could be: 0..2
 *********************************************************************/
uint8_t I2C_getNum(LPC_I2C_T *I2Cx)
{
    if (I2Cx == LPC_I2C0)
    {
        return (0);
    }
    else if (I2Cx == LPC_I2C1)
    {
        return (1);
    }
    else if (I2Cx == LPC_I2C2)
    {
        return (2);
    }
    return (-1);
}

void I2C_Stop(LPC_I2C_T *I2Cx)
{

    /* Make sure start bit is not active */
    if (I2Cx->CONSET & I2C_I2CONSET_STA)
    {
        I2Cx->CONCLR = I2C_I2CONCLR_STAC;
    }
    I2Cx->CONSET = I2C_I2CONSET_STO;
    I2Cx->CONCLR = I2C_I2CONCLR_SIC;
}

void init_i2c(LPC_I2C_T *I2Cx, uint32_t clockrate) /* in kHz */
{
    static I2C_Data new_Data;
    int PCLKSEL;

    new_Data.job_done = 1;
    new_Data.status = 1;
    /*int I2C_PCLK; */

    if (clockrate > 400) /*mehr als 400 kHz wird ned unterstützt */
        clockrate = 400;

    PCLKSEL = 0x0000; /*PCLKSEL eintrag auslesen */

    if (I2C_getNum(I2Cx) == 0)
    {
        //PCLKSEL = (LPC_SC->PCLKSEL0 >> 14) & 3;
    	PCLKSEL = (LPC_SYSCTL->PCLKSEL[0] >> 14) & 3;		// ??? TODO

    }
    else if (I2C_getNum(I2Cx) == 1)
    {
        PCLKSEL = (LPC_SYSCTL->PCLKSEL[1] >> 6) & 3;
    }
    else if (I2C_getNum(I2Cx) == 2)
    {
        PCLKSEL = (LPC_SYSCTL->PCLKSEL[1] >> 20) & 3;
    }

    switch (PCLKSEL)
    {
    case I2C_PCLKSEL_1: /*PCLK = CCLK */
        PCLK[I2C_getNum(I2Cx)] = SystemCoreClock;
        break;
    case I2C_PCLKSEL_2: /*PCLK = CCLK/2 */
        PCLK[I2C_getNum(I2Cx)] = SystemCoreClock / 2;
        break;
    case I2C_PCLKSEL_4: /*PCLK = CCLK/4 */
        PCLK[I2C_getNum(I2Cx)] = SystemCoreClock / 4;
        break;
    case I2C_PCLKSEL_8: /*PCLK = CCLK/8 */
        PCLK[I2C_getNum(I2Cx)] = SystemCoreClock / 8;
        break;
    }

    I2C_RB_init(&I2C_buffer[I2C_getNum(I2Cx)]);
    I2C_active[I2C_getNum(I2Cx)] = &new_Data;

    switch (I2C_getNum(I2Cx))
    {
    case 0:
        I2C0Init(clockrate);

        break;
    case 1:
        I2C1Init(clockrate);
        break;
    case 2:
        I2C2Init(clockrate);
        break;
    }

}

uint8_t i2c_add_job(I2C_Data* data)
{
    /* Check if I2C hardware was initialized */
    switch (I2C_getNum(data->device))
    {
    case 0:
        if (obc_status.i2c0_initialized == 0)
        {
            return 1;
        }

        break;
    case 1:
        if (obc_status.i2c1_initialized == 0)
        {
            return 1;
        }
        break;
    case 2:
        if (obc_status.i2c2_initialized == 0)
        {
            return 1;
        }
        break;

    default:
        return 1;
    }

    (*data).job_done = 0;
    (*data).rx_count = 0;
    (*data).tx_count = 0;
    (*data).status = 0;
    (*data).error = I2C_ERROR_NO_ERROR;

    if (data->tx_size == 0)
    {
        data->dir = 1;
    }
    else
    {
        data->dir = 0;
    }

    I2C_RB_put(&I2C_buffer[I2C_getNum((*data).device)], (void *) data);

    if (active_job_done[I2C_getNum((*data).device)])
    {
        I2C_send(data->device);
    }
    return 0;
}

void I2C_send(LPC_I2C_T *I2Cx)
{

    if (!active_job_done[I2C_getNum(I2Cx)])
        return;

    active_job_done[I2C_getNum(I2Cx)] = 0;
    I2C_active[I2C_getNum(I2Cx)] = I2C_RB_read(&I2C_buffer[I2C_getNum(I2Cx)]);

    I2Cx->CONCLR = I2C_I2CONCLR_SIC;
    I2Cx->CONSET = I2C_I2CONSET_STA;
}

void I2C_Handler(LPC_I2C_T *I2Cx)
{
    uint8_t returnCode;
    uint8_t I2C_num = 3;

    if (I2Cx == LPC_I2C0)
    {
        I2C_num = 0;
    }
    else if (I2Cx == LPC_I2C1)
    {
        I2C_num = 1;
    }
    else if (I2Cx == LPC_I2C2)
    {
        I2C_num = 2;
    }
    else
    {
        return;
        /* I2C not existing */
    }

    returnCode = (I2Cx->STAT & I2C_STAT_CODE_BITMASK);

    // No Status information available?!
    if (returnCode == 0xf8)
    {
        if (!I2C_RB_empty(&I2C_buffer[I2C_num]))
            I2C_send(I2Cx); /*starte nächsten Job, sofern vorhanden. */

        return;
    }

    switch (returnCode)
    {
        /* A start/repeat start condition has been transmitted -------------------*/
    case I2C_I2STAT_M_TX_START:
    case I2C_I2STAT_M_TX_RESTART:

//        if (DEBUG_MODE == 1)
//            I2C_active[I2C_num]->status = I2C_I2STAT_M_TX_START;

        I2Cx->CONCLR = I2C_I2CONCLR_STAC; /*clear start bit */

        /*dir = 0 bedeutet sende phase, dir muss 1 gesetzt werden wenn beim erstellen der I2C_data keine tx daten angegeben werden!!!! */
        if ((*I2C_active[I2C_num]).dir == 0)
        {
            I2Cx->DAT = (I2C_active[I2C_num]->adress << 1); /*sende SLA+W um übertragung zu starten */
            I2Cx->CONCLR = I2C_I2CONCLR_SIC;
            return;
        }
        else if (I2C_active[I2C_num]->dir == 1 && I2C_active[I2C_num]->rx_count != I2C_active[I2C_num]->rx_size)
        {
            I2Cx->DAT = (I2C_active[I2C_num]->adress << 1 | 0x01); /*sende SLA+R um empfang zu starten */
            I2Cx->CONCLR = I2C_I2CONCLR_SIC;
            return;
        }
        else
        {
            I2C_Stop(I2Cx); /*wenn er hier her kommt gibt es nichts zu übertragen -> stoppe I2C kommunikation */
            I2C_active[I2C_getNum(I2Cx)]->job_done = 1; /*wenn es nichts mehr zu übertragen gibt ist der job erledigt */
            active_job_done[I2C_getNum(I2Cx)] = 1;
            if (!I2C_RB_empty(&I2C_buffer[I2C_num]))
                I2C_send(I2Cx);
            return;
        }
        break;

        /* SLA+W has been transmitted, ACK has been received ----------------------*/
    case I2C_I2STAT_M_TX_SLAW_ACK:
        /* Data has been transmitted, ACK has been received */
    case I2C_I2STAT_M_TX_DAT_ACK:

        if (DEBUG_MODE == 1)
            I2C_active[I2C_num]->status = I2C_I2STAT_M_TX_SLAW_ACK;

        if ((*I2C_active[I2C_getNum(I2Cx)]).dir == 0)
        {
            if ((*I2C_active[I2C_num]).tx_size > (*I2C_active[I2C_num]).tx_count)
            {
                I2Cx->DAT = *(uint8_t *) (I2C_active[I2C_num]->tx_data + I2C_active[I2C_num]->tx_count); /*pointer auf tx daten wird um tx_count verlängert, dann werden die daten ausgelesen */
                I2C_active[I2C_num]->tx_count++; /*nächsten daten gesendet, daher count erhöhen */
                I2Cx->CONCLR = I2C_I2CONCLR_SIC; /*reset interrupt */
                return;
            }
            else if ((*I2C_active[I2C_num]).rx_size > (*I2C_active[I2C_num]).rx_count)
            {
                I2C_active[I2C_num]->dir = 1; /*set dir = 1 => change from Transit to receive phase */
                I2Cx->CONSET = I2C_I2CONSET_STA; /*set Start condition for repeated start. */
                I2Cx->CONCLR = I2C_I2CONCLR_AAC | I2C_I2CONCLR_SIC; /*start transmit, reset interrupt */
                return;
            }
            else
            {
                I2C_Stop(I2Cx); /*wenn er hier her kommt gibt es nichts mehr zu übertragen -> stoppe I2C kommunikation */
                (*I2C_active[I2C_getNum(I2Cx)]).job_done = 1; /*wenn es nichts mehr zu übertragen gibt ist der job erledigt */
                active_job_done[I2C_getNum(I2Cx)] = 1;
                if (!I2C_RB_empty(&I2C_buffer[I2C_num]))
                    I2C_send(I2Cx); /*starte nächsten Job, sofern vorhanden. */
                return;
            }
        }
        break;
        /* SLA+R has been transmitted, ACK has been received -----------------------------*/
    case I2C_I2STAT_M_RX_SLAR_ACK:

        if (DEBUG_MODE == 1)
            I2C_active[I2C_num]->status = I2C_I2STAT_M_RX_SLAR_ACK;

        /*if(I2C_active[I2C_num]->rx_count < (I2C_active[I2C_num]->rx_size - 1)){ *//*es werden mehr als 1 Byte erwartet */
        if (I2C_active[I2C_num]->rx_size > 1)
        {
            I2Cx->CONSET = I2C_I2CONSET_AA; /*nächstes Byte mit ACK bestätigen damit die nachfolgenden kommen */
        }
        else
        {
            I2Cx->CONCLR = I2C_I2CONSET_AA; /*nächstes byte ist das letzte => muss mit NACK bestätigt werden */
        }
        I2Cx->CONCLR = I2C_I2CONCLR_SIC;
        return;

        /* Data has been received, ACK has been returned ----------------------*/
    case I2C_I2STAT_M_RX_DAT_ACK:

        if (DEBUG_MODE == 1)
            I2C_active[I2C_num]->status = I2C_I2STAT_M_RX_DAT_ACK;

        if (DEBUG_MODE == 0 || I2C_active[I2C_num]->rx_count < I2C_active[I2C_num]->rx_size)
        {
            /*check ob noch platz ist um einen overflow zu verhindern */
            *((uint8_t*) (I2C_active[I2C_num]->rx_data + I2C_active[I2C_num]->rx_count)) = I2Cx->DAT; /*daten speichern */
            I2C_active[I2C_num]->rx_count++;
        }
        else
        {
            I2C_active[I2C_num]->error = I2C_ERROR_RX_OVERFLOW;
        }

        if (I2C_active[I2C_num]->rx_count < (I2C_active[I2C_num]->rx_size - 1))
        {
            /*es werden mehr als 1 Byte erwartet */
            I2Cx->CONSET = I2C_I2CONSET_AA; /*nächstes Byte mit ACK bestätigen damit die nachfolgenden kommen */
        }
        else
        {
            I2Cx->CONCLR = I2C_I2CONSET_AA; /*nächstes byte ist das letzte => muss mit NACK bestätigt werden */
        }
        I2Cx->CONCLR = I2C_I2CONCLR_SIC;
        return;

        /* Data has been received, NACK has been return -------------------------*/
    case I2C_I2STAT_M_RX_DAT_NACK:

        if (DEBUG_MODE == 1)
            I2C_active[I2C_num]->status = I2C_I2STAT_M_RX_DAT_NACK;

        if (DEBUG_MODE == 0 || I2C_active[I2C_num]->rx_count < I2C_active[I2C_num]->rx_size)
        {
            /*check ob noch platz ist um einen overflow zu verhindern */
            *(uint8_t*) (I2C_active[I2C_num]->rx_data + I2C_active[I2C_num]->rx_count) = I2Cx->DAT; /*daten speichern */
        }
        else
        {
            I2C_active[I2C_num]->error = I2C_ERROR_RX_OVERFLOW;
        }

        I2C_Stop(I2Cx); /*wenn er hier her kommt gibt es nichts mehr zu empfangen -> stoppe I2C kommunikation */
        I2C_active[I2C_getNum(I2Cx)]->job_done = 1; /*wenn es nichts mehr zu empfangen gibt ist der job erledigt */
        active_job_done[I2C_getNum(I2Cx)] = 1;
        if (!I2C_RB_empty(&I2C_buffer[I2C_num]))
            I2C_send(I2Cx); /*starte nächsten Job, sofern vorhanden. */
        return;

    case I2C_I2STAT_M_TX_SLAW_NACK:/* SLA+W has been transmitted, NACK has been received */
    case I2C_I2STAT_M_TX_DAT_NACK: /* Data has been transmitted, NACK has been received */
    case I2C_I2STAT_M_RX_SLAR_NACK: /* SLA+R has been transmitted, NACK has been received */
    case I2C_I2STAT_M_RX_ARB_LOST: /* Arbitration lost */

        /* Device antwortet nicht, Job verwerfen */
        I2C_Stop(I2Cx);
        I2C_active[I2C_num]->error = I2C_ERROR_BUS_ERROR;
        I2C_active[I2C_num]->status = returnCode;
        I2C_active[I2C_num]->job_done = 1;
        active_job_done[I2C_getNum(I2Cx)] = 1;

        ((uint8_t *) &obc_error_counters.i2c0_error_counter)[I2C_num]++;

        if (!I2C_RB_empty(&I2C_buffer[I2C_num]))
            I2C_send(I2Cx);
        return;

    default:
        /* Sollte niemals erreicht werden  */

//        obc_status_extended.i2c_interrupt_handler_error = 1;

        ((uint8_t *) &obc_error_counters.i2c0_error_counter)[I2C_num]++;

        I2C_active[I2C_num]->error = I2C_ERROR_SM_ERROR;
        I2C_Stop(I2Cx);
        I2C_active[I2C_num]->status = returnCode; /* Eventuell eigener Return-Code für all diese Fälle */
        I2C_active[I2C_num]->job_done = 1;
        active_job_done[I2C_getNum(I2Cx)] = 1;
        if (!I2C_RB_empty(&I2C_buffer[I2C_num]))
            I2C_send(I2Cx);
        return;
    }

    // Shall never be reached
    I2Cx->CONCLR = I2C_I2CONCLR_SIC; /*wenn er hier her kommt kam es zu einem fehler. Damit der interrupt nicht ständig ausgelöst wird wird er hier resetted */
}

