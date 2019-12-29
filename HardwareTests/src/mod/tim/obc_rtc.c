#include <stdio.h>
#include <time.h>
#include <chip.h>

/* Other includes */
#include "obc_rtc.h"
#include "../crc/obc_checksums.h"
#include "../cli/cli.h"

typedef struct obc_status_s
{
	unsigned int last_reset_source1 :1; /* Bit 0 *//* (source1 source2) POR: 0b00, EXTR: 0b01, WDTR: 0b10; BODR: 0b11 */
	unsigned int last_reset_source2 :1; /* Bit 1 */
	unsigned int obc_powersave :1; /* Bit 2 */
	unsigned int rtc_synchronized :1; /* Bit 3 */
	unsigned int rtc_initialized :1; /* Bit 4 */
	unsigned int error_code_before_reset :1; /* Bit 5 */
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
	unsigned int rtc_oszillator_error :1; /* Bit 16 */
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
	unsigned int :1; /* Bit 31 */
}
volatile obc_status_t;

obc_status_t obc_status;

void taskDISABLE_INTERRUPTS() {
	// in peg here is some ASM inline code called from RTOS
	__disable_irq();
}

void taskENABLE_INTERRUPTS() {
	// in peg here is some ASM inline code called from RTOS
	__enable_irq();
}

#define EXTENDED_DEBUG_MESSAGES true

volatile uint32_t rtc_epoch_time;

#define RTC_AUX ((uint32_t *) 0x40024058)


void RtcGetTimeCmd(int argc, char *argv[]) {
	printf("OBC: RTC: Time: %ld\n", rtc_get_time());
	printf("OBC: RTC: Date: %ld\n", rtc_get_date());
}



void RTC_IRQHandler(void)
{
	//LPC_TIM0->TC = 0; // Synchronize ms-timer to RTC seconds

	Chip_RTC_ClearIntPending(LPC_RTC, RTC_INT_COUNTER_INCREASE);
	Chip_RTC_ClearIntPending(LPC_RTC, RTC_INT_ALARM);

	rtc_epoch_time++; /* increment QB50 s epoch variable and calculate UTC time */

	/* Do powersave modes or other things here */
	/*if (obc_status.obc_powersave)
	{
		// Reset watchdog regularly if OBC is in powersave
		WDT_Feed();
	}*/
}

/*void rtc_set_time(RTC_TIME_Type * etime)
{
	RTC_TIME_Type tim;

	if (obc_status.rtc_initialized == 0)
	{
		return;
	}

	if (etime == NULL)
	{
		tim.SEC = 0;
		tim.MIN = 36;
		tim.HOUR = 16;
		tim.DOM = 1;
		tim.DOW = 1;
		tim.DOY = 29;
		tim.MONTH = 1;
		tim.YEAR = 2016;

		RTC_SetFullTime(LPC_RTC, &tim);
	}
	else
	{
		RTC_SetFullTime(LPC_RTC, etime);
	}

	rtc_backup_reg_reset(1); // optional
}*/


struct tm * gmtime_wrapper(int32_t * corrected_time)
{
	return gmtime((uint32_t *)corrected_time);
}

void rtc_correct_by_offset(int32_t offset_in_seconds)
{
	struct tm now;
	RTC_TIME_T rtc_tim;
	volatile uint32_t corrected_time;

	if (obc_status.rtc_initialized == 0)
	{
		return;
	}

	/* Current time */
	Chip_RTC_GetFullTime(LPC_RTC, &rtc_tim);

	now.tm_hour = rtc_tim.time[RTC_TIMETYPE_HOUR];
	now.tm_min = rtc_tim.time[RTC_TIMETYPE_MINUTE];
	now.tm_sec = rtc_tim.time[RTC_TIMETYPE_SECOND];
	now.tm_mday = rtc_tim.time[RTC_TIMETYPE_DAYOFMONTH]; 	/* Day of month (1 - 31) */
	now.tm_mon = rtc_tim.time[RTC_TIMETYPE_MONTH]- 1;	 	/* Months since January (0 -11)*/
	now.tm_year = rtc_tim.time[RTC_TIMETYPE_MONTH] - 1900;   /* Years since 1900 */

	corrected_time = ((uint32_t) mktime(&now)) + offset_in_seconds;

	if (corrected_time > 2147483648U)
	{
		// Corrected time is out of range
		return;
	}

	volatile struct tm *t = gmtime_wrapper((int32_t *) &corrected_time);

	rtc_tim.time[RTC_TIMETYPE_HOUR] 		= t->tm_hour;
	rtc_tim.time[RTC_TIMETYPE_MINUTE] 		= t->tm_min;
	rtc_tim.time[RTC_TIMETYPE_SECOND] 		= t->tm_sec;
	rtc_tim.time[RTC_TIMETYPE_DAYOFMONTH] 	= t->tm_mday;
	rtc_tim.time[RTC_TIMETYPE_MONTH]   		= t->tm_mon + 1; 		// RTC months from 1 - 12
	rtc_tim.time[RTC_TIMETYPE_YEAR]    		= t->tm_year + 1900; 	// RTC years with 4 digits (YYYY)

	Chip_RTC_SetFullTime(LPC_RTC, &rtc_tim);
	rtc_calculate_epoch_time();
	return;
}

void rtc_init(void)
{
	RTC_TIME_T tim;

	Chip_RTC_Init(LPC_RTC);

	if (*RTC_AUX & RTC_AUX_RTC_OSCF)
	{
		/* RTC oszillator is not running*/
		obc_status.rtc_oszillator_error = 1;
#if EXTENDED_DEBUG_MESSAGES

		printf("OBC: RTC: Oszillator error\n");
#endif

		obc_status.rtc_initialized = 0;
		obc_status.rtc_synchronized = 0;
		rtc_backup_reg_set(1, 0x00);	// RTC is definitely not in sync
	}

	/* Init Timer 1 before RTC enable */			// TODO module inits and sequence to be determined ....
	//timer0_init();

	/* Check RTC reset */
	if (rtc_check_if_reset())
	{
		/* RTC module has been reset, time and data invalid */

#if EXTENDED_DEBUG_MESSAGES
		printf("OBC: RTC was reset\n", 0);
#endif

		/* Set to default values */
		tim.time[RTC_TIMETYPE_SECOND] = 0;
		tim.time[RTC_TIMETYPE_MINUTE] = 0;
		tim.time[RTC_TIMETYPE_HOUR] = 0;
		tim.time[RTC_TIMETYPE_DAYOFMONTH] = 1;
		tim.time[RTC_TIMETYPE_DAYOFWEEK] = 1;		// ?? stimmt das hier wirklich ??
		tim.time[RTC_TIMETYPE_DAYOFYEAR] = 1;
		tim.time[RTC_TIMETYPE_MONTH] = 1;
		tim.time[RTC_TIMETYPE_YEAR] = 2015;

		Chip_RTC_SetFullTime(LPC_RTC, &tim);
		rtc_backup_reg_reset(1);
		rtc_backup_reg_set(1, 0x00);	// RTC is definitely not in sync
		obc_status.rtc_synchronized = 0;
	}
	else
	{
#if EXTENDED_DEBUG_MESSAGES
		printf("OBC: RTC was running\n");
		printf("OBC: RTC: Time: %ld\n", rtc_get_time());
		printf("OBC: RTC: Date: %ld\n", rtc_get_date());

#endif

		/* RTC was running - check if time is correct */
		uint8_t rval = 0x00;
		rtc_backup_reg_read(1, &rval);
		if (rval == RTC_SYNCHRONIZED)
		{
			// RTC was running and a previous synchronization was successful -> time is valid
			obc_status.rtc_synchronized = 1;
#if EXTENDED_DEBUG_MESSAGES

			printf("OBC: RTC is synchronized\n", 0);
#endif

		}
		else
		{
			// RTC was running, but time was out of sync already
			obc_status.rtc_synchronized = 0;
#if EXTENDED_DEBUG_MESSAGES

			printf("OBC: RTC is NOT synchronized\n", 0);
#endif

		}

		/* Restore last error code from RTC register */
		//obc_status.error_code_before_reset = error_code_get();		//TODO
	}

	rtc_calculate_epoch_time();

	Chip_RTC_CntIncrIntConfig(LPC_RTC, RTC_AMR_CIIR_IMSEC,  ENABLE);
	NVIC_SetPriority(RTC_IRQn, RTC_INTERRUPT_PRIORITY);
	NVIC_EnableIRQ(RTC_IRQn); /* Enable interrupt */

	Chip_RTC_Enable(LPC_RTC, ENABLE);

	obc_status.rtc_initialized = 1;

	RegisterCommand("getTim", RtcGetTimeCmd);

	return;
}

void rtc_calculate_epoch_time(void)
{
	struct tm start, now;
	RTC_TIME_T rtc_tim;
	rtc_epoch_time = 0;

	/* 01.01.2000, 00:00:00 */
	start.tm_hour = 0;
	start.tm_min = 0;
	start.tm_sec = 0;
	start.tm_mday = 1; /* Day of month (1 - 31) */
	start.tm_mon = 0; /* Months since January (0 -11)*/
	start.tm_year = 100; /* Years since 1900 */

	/* Current time */

	Chip_RTC_GetFullTime(LPC_RTC, &rtc_tim);

	now.tm_hour = rtc_tim.time[RTC_TIMETYPE_HOUR];
	now.tm_min = rtc_tim.time[RTC_TIMETYPE_MINUTE];
	now.tm_sec = rtc_tim.time[RTC_TIMETYPE_SECOND];
	now.tm_mday = rtc_tim.time[RTC_TIMETYPE_DAYOFMONTH]; /* Day of month (1 - 31) */
	now.tm_mon = rtc_tim.time[RTC_TIMETYPE_MONTH] - 1; /* Months since January (0 -11)*/
	now.tm_year = rtc_tim.time[RTC_TIMETYPE_YEAR] - 1900; /* Years since 1900 */

	rtc_epoch_time = (uint32_t) difftime(mktime(&now), mktime(&start));
	return;
}

void rtc_get_val(RTC_TIME_T *tim)
{
	Chip_RTC_GetFullTime(LPC_RTC, tim);
}

uint32_t rtc_get_time(void)
{
	RTC_TIME_T tim;
	Chip_RTC_GetFullTime(LPC_RTC, &tim);

	// TODO: tim0 is not used for ms counting yet.....
	return (0 + tim.time[RTC_TIMETYPE_SECOND] * 1000 + tim.time[RTC_TIMETYPE_MINUTE] * 100000 + tim.time[RTC_TIMETYPE_HOUR] * 10000000);
	//return ((LPC_TIM0->TC) + tim.SEC * 1000 + tim.MIN * 100000 + tim.HOUR * 10000000);
}

uint32_t rtc_get_date(void)
{
	/* Returns the current RTC date according to UTC.
	 * Parameters: 	none
	 * Return value: date
	 */
	RTC_TIME_T tim;
	Chip_RTC_GetFullTime(LPC_RTC, &tim);

	return (tim.time[RTC_TIMETYPE_DAYOFMONTH] + tim.time[RTC_TIMETYPE_MONTH] * 100 + (tim.time[RTC_TIMETYPE_YEAR] - 2000) * 10000);
}

uint32_t rtc_get_epoch_time(void)
{
	return rtc_epoch_time;
}

//
// copied from lpc17xx_rtc.c and adapted to current lpc_chip_175x_6x library
// @version		3.0
// @date		18. June. 2010
/*********************************************************************//**
 * @brief 		Read value from General purpose registers
 * @param[in]	RTCx	RTC peripheral selected, should be LPC_RTC
 * @param[in]	Channel General purpose registers Channel number,
 * 				should be in range from 0 to 4.
 * @return 		Read Value
 * Note: These General purpose registers can be used to store important
 * information when the main power supply is off. The value in these
 * registers is not affected by chip reset.
 **********************************************************************/
uint32_t RTC_ReadGPREG(LPC_RTC_T *RTCx, uint8_t Channel)
{
	uint32_t *preg;
	uint32_t value;

//	CHECK_PARAM(PARAM_RTCx(RTCx));
//	CHECK_PARAM(PARAM_RTC_GPREG_CH(Channel));

	preg = (uint32_t *) &(RTCx->GPREG);
	preg += Channel;
	value = *preg;
	return (value);
}

/*********************************************************************//**
 * @brief 		Write value to General purpose registers
 * @param[in]	RTCx	RTC peripheral selected, should be LPC_RTC
 * @param[in]	Channel General purpose registers Channel number,
 * 				should be in range from 0 to 4.
 * @param[in]	Value Value to write
 * @return 		None
 * Note: These General purpose registers can be used to store important
 * information when the main power supply is off. The value in these
 * registers is not affected by chip reset.
 **********************************************************************/
void RTC_WriteGPREG(LPC_RTC_T *RTCx, uint8_t Channel, uint32_t Value)
{
	uint32_t *preg;

//	CHECK_PARAM(PARAM_RTCx(RTCx));
//	CHECK_PARAM(PARAM_RTC_GPREG_CH(Channel));

	preg = (uint32_t *) &(RTCx->GPREG);
	preg += Channel;
	*preg = Value;
}



RetVal rtc_backup_reg_set(uint8_t id, uint8_t val)
{
	/* Writes one byte to the RTC's buffered registers. The register number is specified with the parameter id.
	 * Parameters: 	uint8_t id - Register number to write
	 * 				uint8_t val - Value written to the register
	 * Return value: SUCCESS/ERROR
	 */

	uint8_t regs[20] =
	{ };

	if (id >= 19)
	{
		/* Register does not exist or is protected. */
		return FAILED;
	}

	/* Prevent all other tasks and interrupts from altering the register contents */
	taskDISABLE_INTERRUPTS();

	/* Read all registers */
	*((uint32_t *) &regs[0]) = RTC_ReadGPREG(LPC_RTC, 0);
	*((uint32_t *) &regs[4]) = RTC_ReadGPREG(LPC_RTC, 1);
	*((uint32_t *) &regs[8]) = RTC_ReadGPREG(LPC_RTC, 2);
	*((uint32_t *) &regs[12]) = RTC_ReadGPREG(LPC_RTC, 3);
	*((uint32_t *) &regs[16]) = RTC_ReadGPREG(LPC_RTC, 4);

	/* Set given value */
	regs[id] = val;

	/* Calculate and store new checksum */
	regs[19] = rtc_checksum_calc(&regs[0]);

	/* Store values */
	RTC_WriteGPREG(LPC_RTC, 0, *((uint32_t *) (&regs[0])));
	RTC_WriteGPREG(LPC_RTC, 1, *((uint32_t *) (&regs[4])));
	RTC_WriteGPREG(LPC_RTC, 2, *((uint32_t *) (&regs[8])));
	RTC_WriteGPREG(LPC_RTC, 3, *((uint32_t *) (&regs[12])));
	RTC_WriteGPREG(LPC_RTC, 4, *((uint32_t *) (&regs[16])));

	taskENABLE_INTERRUPTS();

	return DONE;
}

RetVal rtc_backup_reg_reset(uint8_t go)
{
	/* Resets the backup registers and calculates the checksum
	 *
	 */

	if (go == 0)
	{
		return FAILED;
	}

	uint8_t regs[20] =
	{ };

	/* Prevent all other tasks and interrupts from altering the register contents */
	taskDISABLE_INTERRUPTS();

	/* Calculate and store new checksum */
	regs[19] = rtc_checksum_calc(&regs[0]);

	/* Store values */
	RTC_WriteGPREG(LPC_RTC, 0, *((uint32_t *) (&regs[0])));
	RTC_WriteGPREG(LPC_RTC, 1, *((uint32_t *) (&regs[4])));
	RTC_WriteGPREG(LPC_RTC, 2, *((uint32_t *) (&regs[8])));
	RTC_WriteGPREG(LPC_RTC, 3, *((uint32_t *) (&regs[12])));
	RTC_WriteGPREG(LPC_RTC, 4, *((uint32_t *) (&regs[16])));

	taskENABLE_INTERRUPTS();

	return DONE;
}

RetVal rtc_backup_reg_read(uint8_t id, uint8_t * val)
{
	/* Reads one byte from the RTC's buffered registers. The register number is specified with the parameter id.
	 * Parameters: 	uint8_t id - Register number to read from
	 * 				uint8_t * - pointer where value shall be stored
	 * Return value: 0 in case of success, else (wrong checksum) != 0
	 * ID
	 */

	/* Read all registers */

	uint8_t cs;
	uint8_t regs[20] =
	{ };

	if (id > 19)
	{
		/* Register does not exist */
		return FAILED;
	}

	taskDISABLE_INTERRUPTS();

	*((uint32_t *) &regs[0]) = RTC_ReadGPREG(LPC_RTC, 0);
	*((uint32_t *) &regs[4]) = RTC_ReadGPREG(LPC_RTC, 1);
	*((uint32_t *) &regs[8]) = RTC_ReadGPREG(LPC_RTC, 2);
	*((uint32_t *) &regs[12]) = RTC_ReadGPREG(LPC_RTC, 3);
	*((uint32_t *) &regs[16]) = RTC_ReadGPREG(LPC_RTC, 4);

	/* Calculate checksum */
	cs = rtc_checksum_calc(&regs[0]);

	taskENABLE_INTERRUPTS();

	/* Compare checksums */
	if (cs != regs[19])
	{
		/* Checksum is not correct - values may be corrupted */
		return FAILED;
	}

	*val = regs[id];

	return DONE;
}

RetVal rtc_check_if_reset(void)
{
	/**
	 * Check if the RTC module was reseted and therefore time and register values are invalid.
	 */

	uint8_t cs;
	uint8_t regs[20] =
	{ };

	taskDISABLE_INTERRUPTS();

	*((uint32_t *) &regs[0]) = RTC_ReadGPREG(LPC_RTC, 0);
	*((uint32_t *) &regs[4]) = RTC_ReadGPREG(LPC_RTC, 1);
	*((uint32_t *) &regs[8]) = RTC_ReadGPREG(LPC_RTC, 2);
	*((uint32_t *) &regs[12]) = RTC_ReadGPREG(LPC_RTC, 3);
	*((uint32_t *) &regs[16]) = RTC_ReadGPREG(LPC_RTC, 4);

	/* Calculate checksum */
	cs = rtc_checksum_calc(&regs[0]);

	taskENABLE_INTERRUPTS();

	/* Compare checksums */
	if (cs != regs[19])
	{
#if EXTENDED_DEBUG_MESSAGES
		printf("OBC: RTC: Was reset.\n");
#endif
		/* Checksum is not correct - values may be corrupted */

		return FAILED;
	}

	return DONE;
}

uint8_t rtc_checksum_calc(uint8_t *data)
{
	/* Add 1 to ensure 0 for all data entries gives a cs != 0 */
	return (CRC8(data, 19) + 1);
}
