/*
 * TMC2226.h
 *
 *  Created on: Dec 13, 2023
 *      Author: brzan
 */

#ifndef INC_TMC2226_H_
#define INC_TMC2226_H_

/**
 * \brief			Those are possible addresses of TMC2226 nodes
 */
typedef enum {
	TMC2226_ADDR_0 = 0x00u,					 	/* MS1 and MS2 pin set to LOW and LOW correspondingly */
	TMC2226_ADDR_1 = 0x01u,						/* MS1 and MS2 pin set to HIGH and LOW correspondingly */
	TMC2226_ADDR_2 = 0x02u,						/* MS1 and MS2 pin set to LOW and HIGH correspondingly */
	TMC2226_ADDR_3 = 0x03u						/* MS1 and MS2 pin set to HIGH and HIGH correspondingly */
} TMC2226_NodeAddress;

/**
 * \brief			This is a basic TMC2226 structure type
 */
typedef struct {
	TMC2226_NodeAddress	node_addr;				/* This is a node address */
	uint32_t	reg_GCONF;
} TMC_HandleTypeDef;


// Basic definitions used in LL section
#define TMC2226_SYNC 0x05
#define TMC2226_READ 0x00
#define TMC2226_WRITE 0x80

// General register memory positions
#define REG_GCONF 			0x00
#define REG_GSTAT 			0x01
#define REG_IFCNT 			0x02
#define REG_NODECONF 		0x03
#define REG_OTP_PROG 		0x04
#define REG_OTP_READ 		0x05
#define REG_IOIN 			0x06
#define REG_FACTORY_CONF 	0x07
// Velocity dependent control registers
#define REG_VACTUAL 		0x22



/* ########## API FUNCTIONS ########## */
void TMC_Init(TMC_HandleTypeDef* htmc, TMC2226_NodeAddress node_addr);


void TMC_configure_uart_control(TMC_HandleTypeDef* htmc);

void TMC_enable_driver(uint8_t node_address);
void TMC_disable_driver(uint8_t node_address);
void TMC_set_speed(uint8_t node_address, uint32_t speed);


// ########## COMMUNICATION FUNCTIONS ##########
void read_access(uint8_t node_address, uint8_t register_address,
		uint64_t *received_datagram, uint32_t *sent_datagram);

void write_access(uint8_t node_address, uint8_t register_address,
		uint32_t data, uint64_t *sent_datagram);

uint8_t calculate_CRC(uint8_t* datagram, uint8_t datagram_length);

#endif /* INC_TMC2226_H_ */
