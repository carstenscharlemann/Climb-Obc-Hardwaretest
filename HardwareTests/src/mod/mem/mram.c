/*
 * mram.c
 *
 *  Created on: 27.12.2019
 *      Author: Robert
 */


#define MRAM_CS_PORT	 0
#define MRAM_CS_PIN		22

typedef enum mram_status_e {
	MRAM_STAT_NOT_INITIALIZED,
	MRAM_STAT_IDLE,
	MRAM_STAT_ERROR							// TODO: what specific errors are there and what too do now ???? -> reinit SSP ???
} mram_status_t;

mram_status_t mram_status = MRAM_STAT_NOT_INITIALIZED;

#include <chip.h>

#include "../../layer1/ssp/obc_ssp.h"

void MramChipSelect(bool select) {
	Chip_GPIO_SetPinState(LPC_GPIO, MRAM_CS_PORT, MRAM_CS_PIN, !select);
}

void MramInit() {
	/* --- Chip selects --- */
	Chip_IOCON_PinMuxSet(LPC_IOCON, MRAM_CS_PORT, MRAM_CS_PIN, IOCON_FUNC0 | IOCON_MODE_INACT);
	Chip_IOCON_DisableOD(LPC_IOCON, MRAM_CS_PORT, MRAM_CS_PIN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, MRAM_CS_PORT, MRAM_CS_PIN);
	Chip_GPIO_SetPinState(LPC_GPIO, MRAM_CS_PORT, MRAM_CS_PIN, true);

//	/* Init mram  read Status register
//	 * B7		B6		B5		B4		B3		B2		B1		B0
//	 * SRWD		d.c		d.c		d.c		BP1		BP2		WEL		d.c.
//	 *
//	 * On init all bits should read 0x00.
//	 * The only bit we use here is WEL (Write Enable) and this is reset to 0
//	 * on power up.
//	 * None of the other (protection) bits are used at this moment by this software.
//	 */
//	uint8_t tx[1];
//	uint8_t rx[1];
//	uint8_t *job_status = NULL;
//	volatile uint32_t helper;
//
//	/* Read Status register */
//	tx[0] = 0x05;
//	rx[0] = 0xFF;
//
//	if (ssp_add_job(busNr , dev1Nr, tx, 1, rx, 1, &job_status))
//	{
//		/* Error while adding job */
//		mram_status = MRAM_STAT_ERROR;
//		return;
//	}
//
//		helper = 0;
//		while ((*job_status != SSP_JOB_STATE_DONE) && (helper < 1000000))
//		{
//			/* Wait for job to finish */
//			helper++;
//		}
//
//		if (rx[0] != 0x01)
//		{
//			/* Error - Flash could not be accessed */
//			//return FLASH_RET_INIT_ERROR;
//			worker->FlashStatus = FLASH_STAT_ERROR;
//			return false;
//		}

}

void MramMain() {

}

void ReadMramAsync(uint32_t adr,  uint8_t *rx_data,  uint32_t len, void (*finishedHandler)(uint8_t result, uint32_t adr, uint8_t *data, uint32_t len)) {

}

void WriteMramAsync(uint32_t adr, uint8_t *data, uint32_t len,  void (*finishedHandler)(uint8_t result, uint32_t adr, uint32_t len)) {

}

