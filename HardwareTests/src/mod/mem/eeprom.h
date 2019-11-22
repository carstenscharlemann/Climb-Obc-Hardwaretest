/*
 * eeprom.h
 *
 *  Created on: 22.11.2019
 */

#ifndef MOD_MEM_EEPROM_H_
#define MOD_MEM_EEPROM_H_

#define EEPROM_STATUS_PAGE 		0

void EepromInit();						    // Module Init called once prior mainloop
void EepromMain();							// Module routine participating each mainloop.


#endif /* MOD_MEM_EEPROM_H_ */
