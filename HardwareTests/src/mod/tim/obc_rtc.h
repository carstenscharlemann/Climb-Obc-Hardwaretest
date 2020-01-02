#ifndef OBC_RTC_H
#define OBC_RTC_H

/* Standard includes */
#include <stdint.h>
#include <stdlib.h>

#include <Chip.h>

// Module Main API
void RtcInit(void);

//  Module fuinctions API
bool RtcIsGprChecksumOk(void);



#endif /* OBC_RTC_H */
