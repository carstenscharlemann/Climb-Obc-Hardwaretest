/*
 * eeprom.c
 *
 *  Created on: 22.11.2019
 *  Code parts are copied from PEG flight software
  */
#include <stdio.h>
#include <string.h>

#include "eeprom.h"
#include "../../globals.h"
#include "../../layer1/I2C/obc_i2c.h"
#include "../main.h"
#include "../cli/cli.h"


// module variables
eeprom_status_page_t statusPage1;
//eeprom_status_page_t statusPage2;
//eeprom_status_page_t statusPage3;

// We control one possible write job and one read job at a given time ....
I2C_Data writePageJob;
uint8_t writePageTx[5 + EEPROM_PAGE_SIZE];
bool writeInProgress = false;
//void (*writeFinishedHandler)(void) = 0;

I2C_Data readPageJob;
uint8_t readPageTx[2];
uint8_t readPageRx[EEPROM_PAGE_SIZE];
bool readInProgress = false;
void (*readFinishedHandler)(eeprom_page_t *page) = 0;

// Prototypes
void TestEeprom(uint8_t adr);
void WriteStatusCmd(int argc, char *argv[]);
void ReadStatusCmd(int argc, char *argv[]);
uint32_t crc32(uint8_t *data, uint32_t len);

//void delay_ms(uint32_t i)
//{
//	/* This delay shall only be used without the scheduler running, except you are exactly knowing, what you are doing */
//	volatile uint32_t j = 0;
//	i *= SystemCoreClock / 12000; /* Grob empirisch ermittelter Wert f�r ca 1 ms */
//
//	for (j = 0; j < i; j++)
//		;
//}


void EepromInit() {
	// Register module Commands
	RegisterCommand("eeWriteName", WriteStatusCmd);
	RegisterCommand("eeStatus", ReadStatusCmd);
}

//void TestEepromCmd(int argc, char *argv[]) {
//	TestEeprom(I2C_ADR_EEPROM1);
//}


void WriteStatusCmd(int argc, char *argv[]) {
	if (!writeInProgress) {
		writeInProgress = true;

		eeprom_status_page_t page;
		page.id = EEPROM_STATUS_PAGE_ID;
		page.reset_counter1 = statusPage1.reset_counter1;
		page.reset_counter2 = statusPage1.reset_counter2;
		page.reset_counter3 = statusPage1.reset_counter3;
		uint32_t cycles = (statusPage1.cycles_high << 16) | statusPage1.cycles_low;
		cycles++;
		page.cycles_low = cycles & 0x0000FFFF;
		page.cycles_high = (uint8_t)(cycles >> 16);
		strncpy(page.obc_name,"<nset>",7);
		strncpy(page.obc_hardware_version,"-.-",4);
		if (argc >= 1) {
			strncpy( page.obc_name, argv[0], 7);
			if (argc >= 2) {
				strncpy(page.obc_hardware_version, argv[1], 4);
			}
		}
		/* Calculate checksum */
		page.cs = crc32((uint8_t *)(&page), 28);

		writePageJob.device = ONBOARD_I2C;
		writePageJob.adress = I2C_ADR_EEPROM1;
		writePageTx[0] = ((EEPROM_STATUS_PAGE * 32) >> 8); 		// Address high
		writePageTx[1] = ((EEPROM_STATUS_PAGE * 32) & 0xFF); 	// Address low
		memcpy(&(writePageTx[2]),&page, 32);			// Data
		writePageJob.tx_size = 34;
		writePageJob.tx_data = writePageTx;
		writePageJob.rx_data = NULL;
		writePageJob.rx_size = 0;
		i2c_add_job(&writePageJob);
	}
}


void ReadStatusReceived(eeprom_page_t *page) {
	memcpy(&statusPage1, page, EEPROM_PAGE_SIZE);
	printf("Name: %s\n", statusPage1.obc_name);
	printf("HWRev: %s\n", statusPage1.obc_hardware_version);
	printf("ResetCnt %d %d %d\n", statusPage1.reset_counter1, statusPage1.reset_counter2, statusPage1.reset_counter3 );
	printf("Write cycles: %d\n",(statusPage1.cycles_high << 16) | statusPage1.cycles_low );
}



void ReadStatusCmd(int argc, char *argv[]) {
	if (!readInProgress) {
		readInProgress = true;
		readPageJob.device = ONBOARD_I2C;
		readPageJob.tx_size = 2;
		readPageJob.tx_data = readPageTx;
		readPageJob.rx_size = EEPROM_PAGE_SIZE;
		readPageJob.rx_data = readPageRx;

		readPageJob.adress = I2C_ADR_EEPROM1;
		readPageTx[0] = ((EEPROM_STATUS_PAGE * 32) >> 8); 		// Addr. high
		readPageTx[1] = ((EEPROM_STATUS_PAGE * 32) & 0xFF); 	// Addr. low

		readFinishedHandler = ReadStatusReceived;
		i2c_add_job(&readPageJob);
	}
}


void EepromMain() {
	if (writeInProgress) {
		if (writePageJob.job_done == 1) {
			// Result is finshed -> call handler routine
			if (writePageJob.error != I2C_ERROR_NO_ERROR) {
				printf("Error %d while writting to EEPROM", writePageJob.error);
			} else {
				uint16_t pagenr = (writePageTx[0] << 8 | writePageTx[1]) / 32;
				printf("Page %d (ID:%x) written.", pagenr, writePageTx[2]);
			}
			writeInProgress = false;
		}
	}

	if (readInProgress) {
		if (readPageJob.job_done == 1) {
			// Result is finshed -> call handler routine
			if (readFinishedHandler != 0) {
				readFinishedHandler((eeprom_page_t *)readPageRx);
			}
			readInProgress = false;
		}
	}
}


uint32_t crc32(uint8_t *data, uint32_t len)
{
    unsigned int i;
    int j;
    unsigned int byte, crc, mask;

    i = 0;
    crc = 0xFFFFFFFF;

    while (i < len)
    {
        byte = data[i];            // Get next byte.
        crc = crc ^ byte;
        for (j = 7; j >= 0; j--)
        {    // Do eight times.
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
        i = i + 1;
    }
    return ~crc;
}


//void TestEeprom(uint8_t adr) {
//	static I2C_Data job;
//	static uint8_t tx[5];
//	static uint8_t rx[200];
//	volatile uint32_t counter;
//
//	job.adress = adr;
//
//	tx[0] = ((EEPROM_STATUS_PAGE * 32) >> 8); 		// Addr. high
//	tx[1] = ((EEPROM_STATUS_PAGE * 32) & 0xFF); 	// Addr. low
//
//	// Read testdata
//	job.device = ONBOARD_I2C;
//	job.tx_data = tx;
//	job.tx_size = 2;
//	job.rx_data = rx;
//	job.rx_size = 128;
//
//	i2c_add_job(&job);
//
//	counter = 0;
//	while (job.job_done != 1)
//	{
//		counter++;
//		delay_ms(100);
//		if (counter > 100000)
//		{
//			return;
//		}
//	}
//
//	if (job.error != 0)
//	{
//		return;
//	}
//
//	printf("Rx: %d\n", job.rx_count);
//	printf("Data: %s\n", &rx[20]);
//
//	/* Everything fine. */
//	return;
//}
