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
#include "../main.h"		// for Flash signature

#define RADTST_SEQ_LOGBERRY_WATCHDOG_SECONDS			60			// Send Watchdog message every 60 seconds
#define RADTST_SEQ_RESET_READ_EXPECTATIONS_SECONDS	   600			// Every 10 minutes we make a new baseline for the expected read values
#define RADTST_SEQ_CHECK_RTCGPR_SECONDS					30			// Check on RTC General purpose registers RAM
#define RADTST_SEQ_CHECK_PRGFLASH_SECONDS				40			// Check on Program Flash
#define RADTST_SEQ_REPORTLINE_SECONDS				   300			// print out a report line with all check and error counters.

typedef struct radtst_counter_s {
	uint32_t rtcgprTestCnt;
	uint32_t rtcgprTestErrors;
	uint32_t signatureCheckCnt;
	uint32_t signatureErrorCnt;
	uint32_t expSignatureChanged;				// This should stay on 1 (first time read after reset).
} radtst_counter_t;

typedef enum radtst_sources_e {
	RADTST_SRC_RTCGPR,

} radtst_sources_t;

uint32_t 			radtstTicks = 0;
radtst_counter_t	radtstCounter;
bool 				verboseExpectations;
bool 				reportLineWithHeader;

// Compare data for different memory types
static uint8_t rtRtcGprExpectedData[20];

#define RADTST_FLASHSIG_PARTS	4
#define RADTST_PART_FLASHSIZE	(0x0007FFFF / RADTST_FLASHSIG_PARTS)


static bool 		expSigReadActive = false;
static uint8_t		flashPartIdx = 0;
static FlashSign_t 	expectedFlashSig[RADTST_FLASHSIG_PARTS];


// prototypes
void RadTstLogReadError(radtst_sources_t source, uint8_t *expPtr, uint8_t *actPtr, uint16_t len);
void RadTstResetReadExpectations();
void RadTstLogberryWatchdog();
void RadTstCheckRtcGpr();
void RadTstCheckPrgFlash();
void RadTstProvokeErrorCmd(int argc, char *argv[]);
void RadTstExpectedSigCalculated(FlashSign_t signature);
void RadTstSigCalculated(FlashSign_t signature);
void RadTstPrintReportLine();

void RadTstInit(void) {
	verboseExpectations = true;
	reportLineWithHeader = true;
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
		verboseExpectations = false;
		RadTstResetReadExpectations();
	}
	if ((radtstTicks % (RADTST_SEQ_CHECK_PRGFLASH_SECONDS * 1000 / TIM_MAIN_TICK_MS))  == 0) {
		RadTstCheckPrgFlash();
	}
	if ((radtstTicks % (RADTST_SEQ_REPORTLINE_SECONDS * 1000 / TIM_MAIN_TICK_MS))  == 0) {
		RadTstPrintReportLine();
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

void RadTstCheckPrgFlash() {
	// We only start the check, if no rebase of expected value is ongoing.
	if (!expSigReadActive) {
		flashPartIdx = 0;
		CalculateFlashSignatureAsync(0x000000, RADTST_PART_FLASHSIZE, RadTstSigCalculated);
	}
}

void RadTstSigCalculated(FlashSign_t signature) {
	radtstCounter.signatureCheckCnt++;
	if (! IEC60335_IsEqualSignature(&expectedFlashSig[flashPartIdx], &signature)) {
		radtstCounter.signatureErrorCnt++;
		// TODO: if upper part of flash we can scan for differences from expected to actual
		//       to find out how many bits where flipped ....
	}
	flashPartIdx++;
	if (flashPartIdx < RADTST_FLASHSIG_PARTS) {
		CalculateFlashSignatureAsync(RADTST_PART_FLASHSIZE*flashPartIdx, RADTST_PART_FLASHSIZE, RadTstSigCalculated);
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
	if (verboseExpectations) {
		printf("Current GPR Content: ");
		for (int i=0;i<20;i++) {
			printf("%02X ", rtRtcGprExpectedData[i]);
		}
		printf("\n");
	}

	// Flash Checksum
	// ______________
	expSigReadActive = true;
	flashPartIdx = 0;
	CalculateFlashSignatureAsync(0x000000, RADTST_PART_FLASHSIZE, RadTstExpectedSigCalculated);
}


void RadTstExpectedSigCalculated(FlashSign_t signature) {
	if (!IEC60335_IsEqualSignature(&expectedFlashSig[flashPartIdx], &signature)) {
		radtstCounter.expSignatureChanged++;
		expectedFlashSig[flashPartIdx] = signature;
	}
	flashPartIdx++;
	if (flashPartIdx < RADTST_FLASHSIG_PARTS) {
		CalculateFlashSignatureAsync(RADTST_PART_FLASHSIZE*flashPartIdx, RADTST_PART_FLASHSIZE, RadTstExpectedSigCalculated);
	} else {
		expSigReadActive = false;
		if (verboseExpectations) {
			printf("Expected FlashSignatures:\n");
			for (int i=0; i<RADTST_FLASHSIG_PARTS; i++) {
				printf("Part[%d]: %08X / %08X / %08X / %08X\n", i,
						expectedFlashSig[i].word0,
						expectedFlashSig[i].word1,
						expectedFlashSig[i].word2,
						expectedFlashSig[i].word3 );
			}
		}
	}
}

void RadTstPrintReportLine() {
	if (reportLineWithHeader) {
		reportLineWithHeader = false;
		printf("LineID ; RTC Date ; RTC Time ; Seconds after reset ; RTCGPR CRC Tests ; RTCGPR CRC Errors  ; PRG Flash sign checks ; PRG Flash sign errors ; expected sign changes\n");
	}
	printf("R1 ; %ld ; %ld ; %ld ; %ld ; %ld ; %ld ; %ld ; %ld \n",
		    rtc_get_date(),
			rtc_get_time(),
			secondsAfterReset,
			radtstCounter.rtcgprTestCnt,
			radtstCounter.rtcgprTestErrors,
			radtstCounter.signatureCheckCnt,
			radtstCounter.signatureErrorCnt,
			radtstCounter.expSignatureChanged
			);
}

void RadTstProvokeErrorCmd(int argc, char *argv[]) {
	int arg0 = 0;
	if (argc > 0) {
		arg0 = atoi(argv[0]);
	}
	uint32_t *gprbase = (uint32_t *)(&(LPC_RTC->GPREG));
	uint32_t gprx;
	if (arg0 == 1) {
		// RTC GPR RAM Flip 3 bit in 2 words
		gprx = *(gprbase + 1);
		gprx ^= (1 << 7);
		gprx ^= (1 << 13);
		*(gprbase + 1) = gprx;
		gprx = *(gprbase + 3);
		gprx ^= (1 << 22);
		*(gprbase + 3) = gprx;
	} else {
		// RTC GPR RAM Flip a single bit
		gprx = *(gprbase + 1);
		gprx ^= (1 << 7);
		*(gprbase + 1) = gprx;
	}
}
