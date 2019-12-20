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

bool ReadPageAsync(uint8_t flashNr, uint16_t pageNr, void (*finishedHandler)(void *pagePtr));
bool WritePageAsync(uint8_t flashNr, uint16_t pageNr, char *data);



#endif /* MOD_MEM_FLASH_H_ */
