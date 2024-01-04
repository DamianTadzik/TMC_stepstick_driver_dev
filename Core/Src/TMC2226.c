/*
 * TMC2226.c
 *
 *  Created on: Dec 13, 2023
 *      Author: brzan
 */
#include "TMC2226.h"

#include "main.h"
#include "tim.h"
#include "usart.h"
#include "cmsis_os.h"


/* ################ API ################*/

/**
 * \brief			Initializer function that has to be called on TMC_HandleTypeDef
 * 					It sets structure parameters and some registers in the device
 * \param[in]		htmc: handle for proper TMC structure instance
 * \param[in]		node_addr: node address based on MS1 and MS2 pins configuration
 * \param[in]		htim: handle for specific TIM interface
 * \param[in]		huart: handle for specific UART interface
 * \param[in]		engine_steps_per_full_turn: specific engine characteristic typically 200
 */
void TMC_Init(TMC_HandleTypeDef* htmc, TMC2226_NodeAddress node_addr, TIM_HandleTypeDef* htim,
			UART_HandleTypeDef* huart, uint16_t engine_steps_per_full_turn)
{
	uint64_t sent_datagram = 0;

	htmc->htim = htim;
	htmc->huart = huart;
	htmc->node_address = node_addr;
	htmc->engine_steps_per_full_turn = engine_steps_per_full_turn;

	// Internal clock used
	htmc->clock_constant = 0.715;
	// Microstep resolution as fullstep
	htmc->microstep_resolution = uSteps_256;

	// Assume multi-node operation and set SENDDELAY in NODECONF to at least 2, that's from documentation
	/*
	 * Assuming multi-node operation, and according to documentation
	 * so SENDDELAY for read access (time until reply is sent)
	 * is set to at least 2: respond is delayed 3*8 bit times
	 */
	htmc->reg_NODECONF_val = (0x02<<8);
	write_access(htmc, W_NODECONF, htmc->reg_NODECONF_val, &sent_datagram);

	/*
	 * GCONF register configuration:
	 * 1 I_scale_analog - VREF voltage as current reference
	 * 0 internal_Rsense - external sense resistors used
	 * 0 en_SpreadCycle
	 * 0 shaft - spinning direction
	 * 0 index_otpw
	 * 1 index_step
	 * 1 pdn_disable - UART control on PDN pin mode
	 * 1 mstep_reg_select - microstep resolution selected by MRES in CHOPCONF register
	 * 1 multistep_filt
	 * 0 test_mode - never set to 1
	 */
	htmc->reg_GCONF_val = 0b0111100001;
	write_access(htmc, W_GCONF, htmc->reg_GCONF_val, &sent_datagram);

	/*
	 * CHOPCONF driver configuration (Reset default=0x10000053)
	 * 3:0		= 0011 	TOFF and driver
	 * 6:4		= 101 	HSTRT
	 * 10:7		= 0000 	HEND
	 * 14:11	= 0000 	 reserved
	 * 16:15	= 00 	TBL
	 * 17		= 0		vsense
	 * 23:18 	= 000000 reserved
	 * 27:24	= 0000 	MRES - native 256 microstepping higher value -> resolution divided by 2
	 * 28		= 1 	intpol
	 * 29		= 0 	dedge - enable double edge step pulses
	 * 30		= 0		diss2g
	 * 31		= 0		diss2vs
	 */
//	htmc->reg_CHOPCONF_val = 0x10000053; // + (htmc->microstep_resolution << 24);
//	write_access(htmc, W_CHOPCONF, htmc->reg_CHOPCONF_val, &sent_datagram);
}

/**
 * \brief			This runs stepper motor at desired speed
 */
void TMC_set_speed_by_UART(TMC_HandleTypeDef* htmc, float rpm_speed)
{
	uint64_t sent_datagram;
	float FSC_x_USC = htmc->engine_steps_per_full_turn * (1 << (8 - htmc->microstep_resolution));
	float vactual_speed = (rpm_speed * FSC_x_USC / htmc->clock_constant);
	write_access(htmc, W_VACTUAL, (int32_t)vactual_speed, &sent_datagram);
}


/* ################ Low level functions ################ */
/**
 * \brief			LL function for obtaining registry value from TMC2226
 * \param[in]		htmc: something like "self" parameter
 * \param[in]		register_address: readable register address to read from
 * \param[out]		received_datagram: storing received datagram for debugging purposes
 * \param[out]		sent_datagram: storing sent datagram for debugging purposes
 * \return			32 bit registry value but with masked only bits that are pointed in documentation
 *					for example GCONF has only 10 first bits pointed out, so it is masked with 0x3FF
 */
uint32_t read_access(TMC_HandleTypeDef* htmc, TMC2226_ReadRegisters register_address,
		uint64_t *received_datagram, uint32_t *sent_datagram)
{
	// Datagram creation, CRC calculation
	uint8_t datagram[4];
	datagram[0] = 0x05;
	datagram[1] = htmc->node_address;
	datagram[2] = register_address | TMC2226_READ;
	datagram[3] = calculate_CRC(datagram, 4);

	// Sending the datagram
	for (uint8_t i = 0; i < 4; i++)
	{
	    HAL_UART_Transmit(htmc->huart, &datagram[i], 1, HAL_MAX_DELAY);	// TODO Check if it's okay to use HAL_MAX_DELAY
	}

	// Flush one byte
	uint8_t flush = 0;
	HAL_UART_Receive(htmc->huart, &flush, 1, 1);	// TODO Check if it's okay to use 1 as Timeout

	// Receiving response
	uint8_t response[8] = {0};
	HAL_UART_Receive(htmc->huart, response, 8, HAL_MAX_DELAY);	// TODO Check if it's okay to use 1000 as Timeout

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
	uint8_t response_based_CRC = calculate_CRC(response, 8);
	if (response_based_CRC == response[7])
	{
		return apply_mask_and_convert(get_mask_for_given_register(register_address), *received_datagram);
	}
	return 0;
}

/**
 * \brief			LL function for setting a registers in TMC2226
 * \param[in]		htmc: something like "self"
 * \param[in]		register_address: chooses register to write
 * \param[in]		data: data to be set in the register
 * \param[out]		sent_datagram: storing sent datagram for debugging purposes
 */
void write_access(TMC_HandleTypeDef* htmc, TMC2226_WriteRegisters register_address,
		uint32_t data, uint64_t *sent_datagram)
{
	// Datagram creation, CRC calculation
	uint8_t datagram[8];
	datagram[0] = TMC2226_SYNC;
	datagram[1] = htmc->node_address;
	datagram[2] = register_address | 0x80;
	datagram[3] = (data >> 24) & 0xFF;
	datagram[4] = (data >> 16) & 0xFF;
	datagram[5] = (data >> 8 ) & 0xFF;
	datagram[6] = (data      ) & 0xFF;
	datagram[7] = calculate_CRC(datagram, 8);

	// Sending the datagram
	for (uint8_t i = 0; i < 8; i++)
	{
	    HAL_UART_Transmit(htmc->huart, &datagram[i], 1, HAL_MAX_DELAY);	// TODO Check if it is okay to use HAL_MAX_DELAY
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
			return 0b1111111111;		// TODO: Finish this dude
		case R_GSTAT:
			return 0b111;
		case R_IFCNT:
			return 0b11111111;
		case R_OTP_READ:
			return 0xFFFFFF;
		case R_IOIN:
			return 0xFF0003FF;
		case R_FACTORY_CONF:
			return 0b1100011111;
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

