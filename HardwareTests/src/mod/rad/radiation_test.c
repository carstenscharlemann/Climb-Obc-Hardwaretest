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
#define RADTST_SEQ_RESET_READ_EXPECTATIONS_SECONDS	   600			// Every 10 minutes we make a new baseline for the expected read values
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

// Compare data for different memory types
static uint8_t rtRtcGprExpectedData[20];


// prototypes
void RadTstLogReadError(radtst_sources_t source, uint8_t *expPtr, uint8_t *actPtr, uint16_t len);

void RadTstResetReadExpectations();
void RadTstLogberryWatchdog();
void RadTstCheckRtcGpr();

void RadTstProvokeErrorCmd(int argc, char *argv[]);

void RadTstInit(void) {
	RadTstResetReadExpectations();			// initialize all read expectations
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
	if ((radtstTicks % (RADTST_SEQ_RESET_READ_EXPECTATIONS_SECONDS * 1000 / TIM_MAIN_TICK_MS))  == 0) {
		RadTstResetReadExpectations();
	}
}

void RadTstLogberryWatchdog() {
	printf("Supervision watchdog feed\n");
}

void RadTstCheckRtcGpr() {

	uint8_t actualData[20];
	radtstCounter.rtcgprTestCnt++;

	if (!RtcIsGprChecksumOk()) {
		// Checksum error. Lets count individual bit errors.
		radtstCounter.rtcgprTestErrors++;
		RtcReadAllGprs(actualData);
		RadTstLogReadError(RADTST_SRC_RTCGPR, rtRtcGprExpectedData, actualData, 20);
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


void RadTstResetReadExpectations() {
	printf("Radtest reset all read expectations\n");
	// Rtc GPRs
	// --------
	if (!RtcIsGprChecksumOk()) {
		// lets write (an unused) byte to get the Checksum corrected.
		RtcWriteGpr(12, 55);
	}
	// read all 20 bytes to compare when Checksum lost again.
	RtcReadAllGprs(rtRtcGprExpectedData);

	// Flash Checksum
	// ______________
	// TODO ....

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
