/*
 * task_serial_print.c
 *
 *  Created on: Dec 13, 2023
 *      Author: brzan
 */
#include <stdio.h>
#include "main.h"
#include "usart.h"
#include "gpio.h"
#include "cmsis_os.h"


void start_task_serial_print(void *argument)
{
	uint8_t i = 0;
	while (1)
	{
		i++;
		osDelay(100);
		if (i % 100 == 1)
		{
			printf("Hello, damn world! x%d\n", i);
			//HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, SET);
		}
		//if (i % 100 == 2) HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, RESET);
	}
}

