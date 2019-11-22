/*
 * eeprom.c
 *
 *  Created on: 22.11.2019
 *  Code parts are copied from PEG flight software
  */

#include "eeprom.h"
#include "../../globals.h"
#include "../../layer1/I2C/obc_i2c.h"
#include "../main.h"

// Prototypes
void TestEeprom(uint8_t adr);
void TestEepromCmd(int argc, char *argv[]);


void delay_ms(uint32_t i)
{
	/* This delay shall only be used without the scheduler running, except you are exactly knowing, what you are doing */
	volatile uint32_t j = 0;
	i *= SystemCoreClock / 12000; /* Grob empirisch ermittelter Wert f�r ca 1 ms */

	for (j = 0; j < i; j++)
		;
}


void EepromInit() {
	//TestEeprom(I2C_ADR_EEPROM1);
	// Register module Commands
	RegisterCommand("eeTest", TestEepromCmd);
}

void TestEepromCmd(int argc, char *argv[]) {
	// { __asm volatile ("cpsie i"); }
	TestEeprom(I2C_ADR_EEPROM1);
}

void EepromMain() {
	//....
}


void TestEeprom(uint8_t adr) {
	static I2C_Data job;
	static uint8_t tx[5];
	static uint8_t rx;
	volatile uint32_t counter;

	job.adress = adr;

	tx[0] = ((EEPROM_STATUS_PAGE * 32) >> 8); 		// Addr. high
	tx[1] = ((EEPROM_STATUS_PAGE * 32) & 0xFF); 	// Addr. low (testdata field)

	// Read testdata
	job.device = ONBOARD_I2C;
	job.tx_data = tx;
	job.tx_size = 2;
	job.rx_data = &rx;
	job.rx_size = 3;

	i2c_add_job(&job);

	counter = 0;
	while (job.job_done != 1)
	{
		counter++;
		delay_ms(1);
		if (counter > 50)
		{
			return;
		}
	}

	if (job.error != 0)
	{
		return;
	}

	/* Everything fine. */
	return;
}
