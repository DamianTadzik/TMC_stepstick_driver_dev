/*
 * TMC2226.c
 *
 *  Created on: Dec 13, 2023
 *      Author: brzan
 */
#include "TMC2226.h"

#include "main.h"
#include "usart.h"
#include "cmsis_os.h"


/* ################ API ################*/

/**
 * \brief			Initializer function that has to be called on TMC_HandleTypeDef
 * \param[in]		htmc: handle for proper TMC structure instance
 * \param[in]		node_addr: node address based on MS1 and MS2 pins configuration
 */
void TMC_Init(TMC_HandleTypeDef* htmc, TMC2226_NodeAddress node_addr)
{
	uint64_t sent_datagram = 0;
	htmc->node_addr = node_addr;
	// Assume multi-node operation and set SENDDELAY in NODECONF to at least 2, that's from documentation
	htmc->reg_NODECONF_val = (0x02<<8);
	write_access(htmc->node_addr, W_NODECONF, htmc->reg_NODECONF_val, &sent_datagram);
	// This is UART operation mode so we have to set pdn_disable to 1 in GCONF, also leaving defaults (for now)
	htmc->reg_GCONF_val = 0b101000001;
	write_access(htmc->node_addr, W_GCONF, htmc->reg_GCONF_val, &sent_datagram);


	// TODO: Check if it is correctly configured by reading GCONF register and return

}


void TMC_enable_driver(uint8_t node_address)
{

}

void TMC_set_speed(uint8_t node_address, uint32_t speed)
{
	// For debugging purposes
	uint64_t sent_datagram;
	write_access(node_address, W_VACTUAL, speed, &sent_datagram);
}


/* ################ Low level functions ################ */
UART_HandleTypeDef *huart = &huart1;

/**
 * \brief			LL function for obtaining registry value from TMC2226
 * \param[in]		node_address: chooses node to read from
 * \param[in]		register_address: readable register address to read from
 * \param[out]		received_datagram: storing received datagram for debugging purposes
 * \param[out]		sent_datagram: storing sent datagram for debugging purposes
 * \return			32 bit registry value but with masked only bits that are pointed in documentation
 *					for example GCONF has only 10 first bits pointed out, so it is masked with 0x3FF
 */
uint32_t read_access(TMC2226_NodeAddress node_address, TMC2226_ReadRegisters register_address,
		uint64_t *received_datagram, uint32_t *sent_datagram)
{
	// Datagram creation, CRC calculation
	uint8_t datagram[4];
	datagram[0] = 0x05;
	datagram[1] = node_address;
	datagram[2] = register_address | TMC2226_READ;
	datagram[3] = calculate_CRC(datagram, 4);

	// Sending the datagram
	for (uint8_t i = 0; i < 4; i++)
	{
	    HAL_UART_Transmit(huart, &datagram[i], 1, HAL_MAX_DELAY);	// TODO Check if it's okay to use HAL_MAX_DELAY
	}

	// Flush one byte
	uint8_t flush = 0;
	HAL_UART_Receive(huart, &flush, 1, 1);	// TODO Check if it's okay to use 1 as Timeout

	// Receiving response
	uint8_t response[8] = {0};
	HAL_UART_Receive(huart, response, 8, 1000);	// TODO Check if it's okay to use 1000 as Timeout

	// Storing received datagram for later use
	for (uint8_t i = 0; i < 8; i++)
	{
		*received_datagram = (*received_datagram << 8) | response[i];
	}

	// Storing back the datagram we created earlier for debugging purposes
	for (uint8_t i = 0; i < 4; i++)
	{
		*sent_datagram = (*sent_datagram << 8) | datagram[i];
	}

	// Check CRC correctness
	uint8_t response_based_CRC = calculate_CRC(response, 8);	// TODO check if calc CRC works with response[8] filled with CRC
	if (response_based_CRC == response[7])
	{
		return 1;
		//return apply_mask_and_convert(get_mask_for_given_register(register_address), *received_datagram);
	}
	return 0;
}

/**
 * \brief			LL function for setting a registers in TMC2226
 * \param[in]		node_address: chooses node to write to
 * \param[in]		register_address: chooses register to write
 * \param[in]		data: data to be set in the register
 * \param[out]		sent_datagram: storing sent datagram for debugging purposes
 */
void write_access(TMC2226_NodeAddress node_address, TMC2226_WriteRegisters register_address,
		uint32_t data, uint64_t *sent_datagram)
{
	// Datagram creation, CRC calculation
	uint8_t datagram[8];
	datagram[0] = TMC2226_SYNC;
	datagram[1] = node_address;
	datagram[2] = register_address | 0x80;
	datagram[3] = (data >> 24) & 0xFF;
	datagram[4] = (data >> 16) & 0xFF;
	datagram[5] = (data >> 8 ) & 0xFF;
	datagram[6] = (data      ) & 0xFF;
	datagram[7] = calculate_CRC(datagram, 8);

	// Sending the datagram
	for (uint8_t i = 0; i < 8; i++)
	{
	    HAL_UART_Transmit(huart, &datagram[i], 1, HAL_MAX_DELAY);	// TODO Check if it is okay to use HAL_MAX_DELAY
	}

	// Storing back the datagram we created earlier for debugging purposes
	for (uint8_t i = 0; i < 8; i++)
	{
		*sent_datagram = (*sent_datagram << 8) | datagram[i];
	}
}

/**
 * \brief			Calculates CRC of datagram
 * \param[in]		datagram: array that holds filled datagram
 * \param[in]		datagram_length: length of array
 * \return			CRC based on datagram and it's length
 */
uint8_t calculate_CRC(uint8_t* datagram, uint8_t datagram_length)
{
	uint8_t currentByte;
	uint8_t crc = 0;
	for (uint8_t i = 0; i < (datagram_length-1); i++)
	{ // Execute for all bytes of a message
		currentByte = datagram[i]; // Retrieve a byte to be sent from Array
		for (uint8_t j = 0; j < 8; j++)
		{
			if ((crc >> 7) ^ (currentByte&0x01)) // update CRC based result of XOR operation
			{
				crc = (crc << 1) ^ 0x07;
			}
			else
			{
				crc = (crc << 1);
			}
		currentByte = currentByte >> 1;
		} // for CRC bit
	} // for message byte
	return crc;
}

/**
 * \brief			Function holds and returns mask accordingly to provided address
 * \param[in]		register_address: register whom mask should be returned
 * \return		 	Byte mask for certain register
 * \note			By default it returns 0xFF..F mask so none of bytes masked are omitted
 */
uint32_t get_mask_for_given_register(TMC2226_ReadRegisters register_address)
{
	switch (register_address)
	{
		case R_GCONF:
			return 0b1111111111;
		default:
			return 0xFFFFFFFF;
	}
}

/**
 * \brief			Applies mask, shifts bits and returns registry value
 * \param[in]		mask: mask to be applied
 * \param[in]		received_datagram: datagram from witch data is being extracted
 * \return			Register value
 */
uint32_t apply_mask_and_convert(uint32_t mask, uint64_t received_datagram)
{
	return (uint32_t)((received_datagram >> 8) & mask);
}
