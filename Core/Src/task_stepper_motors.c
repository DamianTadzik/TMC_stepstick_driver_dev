/*
 * task_task_stepper_motors.c
 *
 *  Created on: Dec 13, 2023
 *      Author: brzan
 */
#include "main.h"
#include "usart.h"
#include "TMC2226.h"
#include "cmsis_os.h"

extern uint8_t command_triggered;

uint64_t recieved_data = 0;
uint32_t sent_read = 0;

uint64_t sent_write = 0;
uint32_t data = 0;

void start_task_stepper_motors(void *argument)
{
	while (1)
	{
		if (command_triggered)
		{
			command_triggered = 0;

			//write_access(TMC2226_ADDR_0, REG_GCONF, 0b100000, &sent_write);
			read_access(TMC2226_ADDR_0, REG_NODECONF, &recieved_data, &sent_read);


		}
		osDelay(10);
	}
}
