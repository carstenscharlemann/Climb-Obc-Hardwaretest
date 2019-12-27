/*
 * flash.h
 *
 *  Created on: 20.12.2019
 *
 */

#ifndef MOD_MEM_FLASH_H_
#define MOD_MEM_FLASH_H_

typedef enum flash_res_e
{
	FLASH_RES_SUCCESS = 0,
	FLASH_RES_BUSY,
	FLASH_RES_DATA_PTR_INVALID,
	FLASH_RES_TX_OVERFLOW,
	FLASH_RES_INVALID_ADR,
	FLASH_RES_JOB_ADD_ERROR,
	FLASH_RES_WIPCHECK_ERROR,
	FLASH_RES_TX_ERROR,
	FLASH_RES_TX_WRITE_TOO_LONG,
	FLASH_RES_WRONG_FLASHNR,
	FLASH_RES_RX_LEN_OVERFLOW
} flash_res_t;

void FlashInit();						    // Module Init called once prior mainloop
void FlashMain();							// Module routine participating each mainloop.

void ReadFlashAsync(uint8_t flashNr, uint32_t adr, uint32_t len, void (*finishedHandler)(flash_res_t rxtxResult, uint8_t flashNr, uint32_t adr, uint8_t *data, uint32_t len));
void WriteFlashAsync(uint8_t flashNr, uint32_t adr, uint8_t *data, uint32_t len,  void (*finishedHandler)(flash_res_t rxtxResult, uint8_t flashNr, uint32_t adr, uint32_t len));
void EraseFlashAsync(uint8_t flashNr, uint32_t adr, void (*finishedHandler)(flash_res_t rxtxResult, uint8_t flashNr, uint32_t adr, uint32_t len));


#endif /* MOD_MEM_FLASH_H_ */
