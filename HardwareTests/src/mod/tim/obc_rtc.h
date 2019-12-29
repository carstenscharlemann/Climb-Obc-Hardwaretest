#ifndef OBC_RTC_H
#define OBC_RTC_H

/* Standard includes */
#include <stdint.h>
#include <stdlib.h>

#include <Chip.h>
/* FreeRTOS includes */
//#include "FreeRTOS.h"
//#include "task.h"
//#include "queue.h"
//#include "semphr.h"
//
///* Other includes */
//#include "lpc17xx_timer.h"
//#include "lpc17xx_rtc.h"
//
//#include "main.h"
// From RTOS
//typedef long BaseType_t;

#define configMAX_LIBRARY_INTERRUPT_PRIORITY    ( 5 )
#define RTC_INTERRUPT_PRIORITY  (configMAX_LIBRARY_INTERRUPT_PRIORITY + 1)  /* RTC - highest priority after watchdog! */

typedef enum
{
	DONE = 0, FAILED = !DONE
} RetVal;


#define RTC_SYNCHRONIZED 0xAB

typedef struct rtc_status_s
{
	uint8_t rtc_synchronized;
} rtc_status_t;

void rtc_init(void);

void rtc_get_val(RTC_TIME_T *tim);
uint32_t rtc_get_date(void);
uint32_t rtc_get_time(void);
uint64_t rtc_get_extended_time(void);
void rtc_calculate_epoch_time(void);
uint32_t rtc_get_epoch_time(void);
//void rtc_set_time(RTC_TIME_Type * etime);
void rtc_correct_by_offset(int32_t offset_in_seconds);

RetVal rtc_check_if_reset(void);
uint8_t rtc_checksum_calc(uint8_t *data);
RetVal rtc_sync(RTC_TIME_T *tim);

RetVal rtc_backup_reg_read(uint8_t id, uint8_t * val);
RetVal rtc_backup_reg_set(uint8_t id, uint8_t val);
RetVal rtc_backup_reg_reset(uint8_t go);

#endif /* OBC_RTC_H */
