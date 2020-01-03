/*
 * radiation_test.c
 *
 *  Created on: 03.01.2020
 */

#include <stdio.h>
#include <Chip.h>

#include "../tim/timer.h"
#include "../tim/obc_rtc.h"
#include "../cli/cli.h"

#define RADTST_SEQ_LOGBERRY_WATCHDOG_SECONDS			60			// Send Watchdog message every 60 seconds
#define RADTST_SEQ_CHECK_RTCGPR_SECONDS					30			// Check on RTC General purpose registers

typedef struct radtst_counter_s {
	uint32_t rtcgprTestCnt;

	uint32_t rtcgprTestErrors;
} radtst_counter_t;

typedef enum radtst_sources_e {
	RADTST_SRC_RTCGPR,

} radtst_sources_t;

uint32_t 			radtstTicks = 0;
radtst_counter_t	radtstCounter;

// prototypes
void RadTstLogReadError(radtst_sources_t source, uint8_t *expPtr, uint8_t *actPtr, uint16_t len);

void RadTstLogberryWatchdog();
void RadTstCheckRtcGpr();

void RadTstProvokeErrorCmd(int argc, char *argv[]);

void RadTstInit(void) {
	RegisterCommand("simErr",RadTstProvokeErrorCmd);
}

void RadTstMain(void) {
	radtstTicks++;
	if ((radtstTicks % (RADTST_SEQ_LOGBERRY_WATCHDOG_SECONDS * 1000 / TIM_MAIN_TICK_MS))  == 0) {
		RadTstLogberryWatchdog();
	}
	if ((radtstTicks % (RADTST_SEQ_CHECK_RTCGPR_SECONDS * 1000 / TIM_MAIN_TICK_MS))  == 0) {
		RadTstCheckRtcGpr();
	}
}

void RadTstLogberryWatchdog() {
	printf("Supervision watchdog feed\n");
}

void RadTstCheckRtcGpr() {
	static uint8_t expectedData[20];
	uint8_t actualData[20];
	radtstCounter.rtcgprTestCnt++;

	if (RtcIsGprChecksumOk()) {
		RtcReadAllGprs(expectedData);
	} else {
		// Checksum error. Lets count individual bit errors.
		radtstCounter.rtcgprTestErrors++;
		RtcReadAllGprs(actualData);
		RadTstLogReadError(RADTST_SRC_RTCGPR, expectedData, actualData, 20);
	}
}


void RadTstLogReadError(radtst_sources_t source, uint8_t *expPtr, uint8_t *actPtr, uint16_t len) {
	uint16_t diffCntBits  = 0;
	uint16_t diffCntBytes = 0;

	for (uint16_t idx = 0; idx < len; idx++ ) {
		if (expPtr[idx] != actPtr[idx]) {
			diffCntBytes++;
			uint8_t bitErrors = expPtr[idx] ^ actPtr[idx];
			while (bitErrors) {
				diffCntBits += bitErrors & 1;
		        bitErrors >>= 1;
		    }
		}
	}

	if (diffCntBytes > 0) {
		printf("ReadError from %d: %d bits in %d/%d bytes\n", source, diffCntBits, diffCntBytes, len);
	}
}

void RadTstProvokeErrorCmd(int argc, char *argv[]) {
	int arg0 = 0;
	if (argc > 0) {
		arg0 = atoi(argv[0]);
	}
	uint32_t *gprbase = (uint32_t *)(&(LPC_RTC->GPREG));
	uint32_t gprx;
	if (arg0 == 1) {
		// Flip  3 bit in 2 words
		gprx = *(gprbase + 1);
		gprx ^= (1 << 7);
		gprx ^= (1 << 13);
		*(gprbase + 1) = gprx;
		gprx = *(gprbase + 3);
		gprx ^= (1 << 22);
		*(gprbase + 3) = gprx;
	} else {
		// Flip a single bit
		gprx = *(gprbase + 1);
		gprx ^= (1 << 7);
		*(gprbase + 1) = gprx;
	}

}
