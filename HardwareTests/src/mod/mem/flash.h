/*
 * flash.h
 *
 *  Created on: 20.12.2019
 *
 */

#ifndef MOD_MEM_FLASH_H_
#define MOD_MEM_FLASH_H_

void FlashInit();						    // Module Init called once prior mainloop
void FlashMain();							// Module routine participating each mainloop.

bool ReadFlashPageAsync(uint8_t flashNr, uint16_t adr, uint16_t len, void (*finishedHandler)(uint8_t flashNr, uint16_t adr, uint8_t *data, uint16_t len));
bool WritePageAsync(uint8_t flashNr, uint16_t pageNr, char *data);



#endif /* MOD_MEM_FLASH_H_ */
