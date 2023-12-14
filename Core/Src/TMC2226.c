/*
 * TMC2226.c
 *
 *  Created on: Dec 13, 2023
 *      Author: brzan
 */
#include "main.h"
#include "usart.h"
#include "TMC2226.h"
#include "cmsis_os.h"

void TMC_enable_driver(uint8_t node_address)
{

}

// ########## COMMUNICATION FUNCTIONS ##########
UART_HandleTypeDef *huart = &huart1;


void read_access(uint8_t node_address, uint8_t register_address,
		uint64_t *received_datagram, uint32_t *sent_datagram)
{
	// Datagram creation, CRC calculation
	uint8_t datagram[4];
	datagram[0] = 0x05;
	datagram[1] = node_address;
	datagram[2] = register_address;
	datagram[3] = calculate_CRC(datagram, 4);

	// Clearing the buffer from anything that might have been there
	//clear_uart_buffer(huart, 10);

	// Sending the datagram
	for (uint8_t i = 0; i < 4; i++)
	{
	    HAL_UART_Transmit(huart, &datagram[i], 1, HAL_MAX_DELAY);
	}

	// Flush one byte
	uint8_t flush = 0;
	HAL_UART_Receive(huart, &flush, 1, 1);

	// Receiving response
	uint8_t response[8] = {0};
	HAL_UART_Receive(huart, response, 8, 1000);

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
	int x = 0;
	x++;
}


void write_access(uint8_t node_address, uint8_t register_address,
		uint32_t data, uint64_t *sent_datagram)
{
	// Datagram creation, CRC calculation
	uint8_t datagram[8];
	datagram[0] = TMC2226_SYNC;
	datagram[1] = node_address;
	datagram[2] = register_address | TMC2226_WRITE;
	datagram[3] = (data >> 24) & 0xFF;
	datagram[4] = (data >> 16) & 0xFF;
	datagram[5] = (data >> 8 ) & 0xFF;
	datagram[6] = (data      ) & 0xFF;
	datagram[7] = calculate_CRC(datagram, 7);

	// Sending the datagram
	for (uint8_t i = 0; i < 8; i++)
	{
	    HAL_UART_Transmit(huart, &datagram[i], 1, HAL_MAX_DELAY);
	}

	// Storing back the datagram we created earlier for debugging purposes
	for (uint8_t i = 0; i < 8; i++)
	{
		*sent_datagram = (*sent_datagram << 8) | datagram[i];
	}
	int x = 0;
	x++;
}


uint8_t calculate_CRC(uint8_t* datagram, uint8_t datagram_length)
{
	//uint8_t* crc = datagram + (datagram_length-1); // CRC located in last byte of message
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
