#ifndef OBC_RTC_H
#define OBC_RTC_H

/* Standard includes */
#include <stdint.h>
#include <stdlib.h>

#include <Chip.h>

// Module Main API
void RtcInit(void);

//  Module functions API
bool RtcIsGprChecksumOk(void);
void RtcSetDate(uint16_t year, uint8_t month, uint8_t dayOfMonth);
void RtcSetTime(uint8_t hours, uint8_t minutes, uint8_t seconds);


#endif /* OBC_RTC_H */
