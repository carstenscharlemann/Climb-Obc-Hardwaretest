/*
 * radtst_extmemory.c
 *
 *  Created on: 17.01.2020
 */

#include <Chip.h>
#include <stdio.h>

#include "radiation_test.h"
#include "radtst_memory.h"

#include "../../globals.h"
#include "../mem/mram.h"
#include "../mem/eeprom.h"

#define RADTST_MRAM_TARGET_PAGESIZE		MRAM_MAX_WRITE_SIZE						// 1k pages ->
#define RADTST_MRAM_TARGET_PAGES		(128 * 1024) / MRAM_MAX_WRITE_SIZE		// 128k available


uint8_t pageBuffer[RADTST_MRAM_TARGET_PAGESIZE + 4];
uint8_t curPage;
uint8_t curReadPage;

void RadReadMramFinished(mram_res_t result, uint32_t adr, uint8_t *data, uint32_t len);
void RadWriteMramFinished(mram_res_t result, uint32_t adr, uint8_t *data, uint32_t len);

void RadWriteFramFinished();
void RadReadFramFinished(eeprom_page_t *page);


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
		radtstCounter.mramPageWriteCnt++;
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
		RadTstLogReadError2(RADTST_SRC_MRAM, curReadPage, expByte, &data[0], RADTST_MRAM_TARGET_PAGESIZE );
	}

	curReadPage++;
	if (curReadPage < RADTST_MRAM_TARGET_PAGES) {
		radtstCounter.mramPageReadCnt++;
		ReadMramAsync(curReadPage * RADTST_MRAM_TARGET_PAGESIZE, pageBuffer, RADTST_MRAM_TARGET_PAGESIZE, RadReadMramFinished );
	} else {
		//printf("MRAM read test stopped\n");
		//runningBits.radtest_mramread_running = false;
	}
}

#define RADTST_FRAM_TARGET_PAGESIZE		EEPROM_PAGE_SIZE					// 32 byte
#define RADTST_FRAM_TARGET_PAGES		100 //(16 * 1024) / EEPROM_PAGE_SIZE	// 16k available
#define RADTST_EEPROM_TARGET_PAGES		100 //(8 * 1024) / EEPROM_PAGE_SIZE		// 8k available

uint8_t pageBufferFram[RADTST_FRAM_TARGET_PAGESIZE];
uint8_t curPageFram;
uint8_t curDevice;
uint8_t curMaxPage;


void RadTstWriteFram(){
	// Write the new Patterns
	curPageFram = 0;
	radtstCounter.framPageWriteCnt++;
	//printf("FRAM/EE Write all pages started ");

	uint8_t expByte = expectedPagePatternsPtr[curPageFram % RADTST_EXPECTED_PATTERN_CNT];
	for (int x =0; x < RADTST_FRAM_TARGET_PAGESIZE; x++) {
		pageBufferFram[x] = expByte;
	}
	curDevice = I2C_ADR_FRAM;
	curMaxPage = RADTST_FRAM_TARGET_PAGES;
	WritePageAsync( curDevice, curPageFram,  (char *)pageBufferFram, RadWriteFramFinished);
}

void RadWriteFramFinished(){
	curPageFram++;
	//printf(".");
	if (curPageFram < curMaxPage) {
		radtstCounter.framPageWriteCnt++;
		uint8_t expByte = expectedPagePatternsPtr[curPageFram % RADTST_EXPECTED_PATTERN_CNT];
		for (int x =0; x < RADTST_FRAM_TARGET_PAGESIZE; x++) {
			pageBufferFram[x] = expByte;
		}
		WritePageAsync( curDevice, curPageFram,  (char *)pageBufferFram, RadWriteFramFinished);
	} else {
		// All pages written for this device. take next one.
		if (curDevice == I2C_ADR_FRAM){
			curDevice = I2C_ADR_EEPROM2;
			curMaxPage =  RADTST_EEPROM_TARGET_PAGES;
			curPageFram = 0;
			radtstCounter.framPageWriteCnt++;
			uint8_t expByte = expectedPagePatternsPtr[curPageFram % RADTST_EXPECTED_PATTERN_CNT];
			for (int x =0; x < RADTST_FRAM_TARGET_PAGESIZE; x++) {
				pageBufferFram[x] = expByte;
			}
			WritePageAsync( curDevice, curPageFram,  (char *)pageBufferFram, RadWriteFramFinished);
		} else {

			// no other device needed
			readEnabled.fram = true;
			printf("FRAM/EE Write all pages ended.\n");
		}
	}
}

void RadTstCheckFram(){
	curPageFram = 0;
	//printf("FRAM/EE read test started\n");
	//runningBits.radtest_mramread_running = true;
	radtstCounter.framPageReadCnt++;

	curDevice = I2C_ADR_FRAM;
	curMaxPage = RADTST_FRAM_TARGET_PAGES;

	ReadPageAsync(curDevice, curPageFram, RadReadFramFinished);
}

void RadReadFramFinished(eeprom_page_t *page){
	uint8_t expByte = expectedPagePatternsPtr[curPageFram % RADTST_EXPECTED_PATTERN_CNT];
	uint8_t *data  = (uint8_t *)page;
	bool error = false;
	for (int x = 0; x < RADTST_FRAM_TARGET_PAGESIZE; x++) {
		if (data[x] != expByte) {
			error = true;
		}
	}
	if (error) {
		radtstCounter.framPageReadError++;
		radtst_sources_t src = RADTST_SRC_FRAM;
		if (curDevice == I2C_ADR_EEPROM2) {
			src = RADTST_SRC_EE2;
		}
		RadTstLogReadError2(src, curPageFram,  expByte, &data[0], RADTST_FRAM_TARGET_PAGESIZE );
	}

	curPageFram++;
	if (curPageFram < curMaxPage) {
		radtstCounter.framPageReadCnt++;
		ReadPageAsync(curDevice, curPageFram, RadReadFramFinished);
	} else {
		// All pages read for this device. take next one.
		if (curDevice == I2C_ADR_FRAM){
			curDevice = I2C_ADR_EEPROM2;
			curMaxPage =  RADTST_EEPROM_TARGET_PAGES;
			curPageFram = 0;
			radtstCounter.framPageReadCnt++;
			ReadPageAsync(curDevice, curPageFram, RadReadFramFinished);;
		} else {

			//no other device left

			//printf("FRAM/EE read test finished\n");
			//runningBits.radtest_mramread_running = false;
		}
	}

}
