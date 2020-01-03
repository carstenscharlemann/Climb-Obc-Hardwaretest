/*
 * obc_checksums.h
 *
 *  Created on: 29.12.2019
 *      Author: Robert
 */

#ifndef MOD_CRC_OBC_CHECKSUMS_H_
#define MOD_CRC_OBC_CHECKSUMS_H_

uint8_t CRC8(uint8_t* str, size_t length);
uint32_t crc32(uint8_t *data, uint32_t len);

#endif /* MOD_CRC_OBC_CHECKSUMS_H_ */
