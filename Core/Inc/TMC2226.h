/*
 * TMC2226.h
 *
 *  Created on: Dec 13, 2023
 *      Author: brzan
 */

#ifndef INC_TMC2226_H_
#define INC_TMC2226_H_

//
#define TMC2226_SYNC 0x05	// changed for test original is 0x05
#define TMC2226_ADDR_0 0x00
#define TMC2226_ADDR_1 0x01
#define TMC2226_ADDR_2 0x02
#define TMC2226_ADDR_3 0x03
#define TMC2226_READ 0x00
#define TMC2226_WRITE 0x80

#define DATAGRAM_READ_SIZE 4
#define DATAGRAM_RESPONSE_SIZE 8
#define DATAGRAM_WRITE_SIZE 8

// General register memory positions
#define REG_GCONF 			0x00
#define REG_GSTAT 			0x01
#define REG_IFCNT 			0x02
#define REG_NODECONF 		0x03
#define REG_OTP_PROG 		0x04
#define REG_OTP_READ 		0x05
#define REG_IOIN 			0x06
#define REG_FACTORY_CONF 	0x07

// ########## API ##########
void TMC_enable_driver(uint8_t node_address);
void TMC_disable_driver(uint8_t node_address);

// ########## ##########


// ########## COMMUNICATION FUNCTIONS ##########
void read_access(uint8_t node_address, uint8_t register_address,
		uint64_t *received_datagram, uint32_t *sent_datagram);

void write_access(uint8_t node_address, uint8_t register_address,
		uint32_t data, uint64_t *sent_datagram);

uint8_t calculate_CRC(uint8_t* datagram, uint8_t datagram_length);

#endif /* INC_TMC2226_H_ */
