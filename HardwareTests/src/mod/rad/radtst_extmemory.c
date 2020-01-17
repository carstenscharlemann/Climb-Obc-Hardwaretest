/*
 * radtst_extmemory.c
 *
 *  Created on: 17.01.2020
 */

#include <Chip.h>
#include <stdio.h>

#include "radiation_test.h"
#include "radtst_memory.h"
#include "../mem/mram.h"

#define RADTST_MRAM_TARGET_PAGESIZE		MRAM_MAX_WRITE_SIZE						// 1k pages ->
#define RADTST_MRAM_TARGET_PAGES		(128 * 1024) / MRAM_MAX_WRITE_SIZE		// 128k available


uint8_t pageBuffer[RADTST_MRAM_TARGET_PAGESIZE + 4];
uint8_t curPage;
uint8_t curReadPage;

void RadReadMramFinished(mram_res_t result, uint32_t adr, uint8_t *data, uint32_t len);
void RadWriteMramFinished(mram_res_t result, uint32_t adr, uint8_t *data, uint32_t len);

void RadTstWriteMram() {
	// Write the new Patterns
	curPage = 0;
	radtstCounter.mramPageWriteCnt++;
	//printf("MRAM Write all pages started\n");

	uint8_t expByte = expectedPagePatternsPtr[curPage % RADTST_EXPECTED_PATTERN_CNT];
	for (int x =0; x < RADTST_MRAM_TARGET_PAGESIZE; x++) {
		pageBuffer[4 + x] = expByte;
	}
	WriteMramAsync(curPage * RADTST_MRAM_TARGET_PAGESIZE, pageBuffer, RADTST_MRAM_TARGET_PAGESIZE, RadWriteMramFinished );
}

void RadWriteMramFinished(mram_res_t result, uint32_t adr, uint8_t *data, uint32_t len) {
	if (result != MRAM_RES_SUCCESS) {
		radtstCounter.mramPageWriteError++;
		printf("MRAM write bus error %d.\n", result);
//      TODO: keine Ahnung, was hier besser/wahrscheinlich besser ist/wäre :-((((
//		readEnabled.mram = true;		// Maybe this will work later ...
//		return;							// but to continue with another page write makes no sense !?
	}
	curPage++;
	if (curPage < RADTST_MRAM_TARGET_PAGES) {
		uint8_t expByte = expectedPagePatternsPtr[curPage % RADTST_EXPECTED_PATTERN_CNT];
		for (int x =0; x < RADTST_MRAM_TARGET_PAGESIZE; x++) {
			pageBuffer[4 + x] = expByte;
		}
		WriteMramAsync(curPage * RADTST_MRAM_TARGET_PAGESIZE, pageBuffer, RADTST_MRAM_TARGET_PAGESIZE, RadWriteMramFinished );
	} else {
		// All pages written restart read tests
		readEnabled.mram = true;
		printf("MRAM Write all pages ended.\n");
	}
}

void RadTstCheckMram() {
	curReadPage = 0;
	//printf("MRAM read test started\n");
	//runningBits.radtest_mramread_running = true;
	radtstCounter.mramPageReadCnt++;

	ReadMramAsync(curReadPage * RADTST_MRAM_TARGET_PAGESIZE, pageBuffer, RADTST_MRAM_TARGET_PAGESIZE, RadReadMramFinished );
}

void RadReadMramFinished(mram_res_t result, uint32_t adr, uint8_t *data, uint32_t len) {
	if (result != MRAM_RES_SUCCESS) {
		printf("MRAM read bus Error %d\n", result);
		radtstCounter.mramPageReadError++;
	}
	uint8_t expByte = expectedPagePatternsPtr[curReadPage % RADTST_EXPECTED_PATTERN_CNT];
	bool error = false;
	for (int x = 0; x < RADTST_MRAM_TARGET_PAGESIZE; x++) {
		if (data[x] != expByte) {
			error = true;
		}
	}
	if (error) {
		radtstCounter.mramPageReadError++;
		RadTstLogReadError2(RADTST_SRC_MRAM, expByte, &data[0], RADTST_MRAM_TARGET_PAGESIZE );
	}

	curReadPage++;
	if (curReadPage < RADTST_MRAM_TARGET_PAGES) {
		ReadMramAsync(curReadPage * RADTST_MRAM_TARGET_PAGESIZE, pageBuffer, RADTST_MRAM_TARGET_PAGESIZE, RadReadMramFinished );
	} else {
		//printf("MRAM read test stopped\n");
		//runningBits.radtest_mramread_running = false;
	}
}

