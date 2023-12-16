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
	while (1)
	{
		osDelay(1000);
	}
}

