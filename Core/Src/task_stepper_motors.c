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
#include  <stdio.h>

extern uint8_t command_triggered;

uint64_t recieved_data = 0;
uint32_t sent_read = 0;

uint64_t sent_write = 0;
uint32_t data = 0;

void start_task_stepper_motors(void *argument)
{
	/* ## EXAMPLE OF API USE ## */
	TMC_HandleTypeDef htmc1;
	TMC_Init(&htmc1, TMC2226_ADDR_0, &huart1);

	uint8_t trigger_counter = 0;
	while (1)
	{
		if (command_triggered)
		{
			command_triggered = 0;
			trigger_counter++;
			switch (trigger_counter)
			{
				case 1:
					data = read_access(&htmc1, R_SG_RESULT, &recieved_data, &sent_read);
					break;
				case 2:
					TMC_set_speed(&htmc1, 2000);
					break;
				case 3:
					TMC_set_speed(&htmc1, -2000);
					break;
				case 4:
					TMC_set_speed(&htmc1, 0);
					break;
				default:
					trigger_counter = 0;
					break;
			}
			printf("Current trigger_counter = %d", trigger_counter);
		}
		osDelay(10);
	}
}
