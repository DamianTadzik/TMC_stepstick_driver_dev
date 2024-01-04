/*
 * task_task_stepper_motors.c
 *
 *  Created on: Dec 13, 2023
 *      Author: brzan
 */
#include "main.h"
#include "tim.h"
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
	TMC_Init(&htmc1, TMC2226_ADDR_0, &htim2, &huart1, 200);

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
					data = read_access(&htmc1, R_GCONF, &recieved_data, &sent_read);
					break;
				case 2:
					data = read_access(&htmc1, R_SG_RESULT, &recieved_data, &sent_read);
					break;
				case 3:
					TMC_set_speed_by_UART(&htmc1, -10.0f);
					break;
				case 4:
					TMC_set_speed_by_UART(&htmc1, 0.0f);
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


/*
 * UART has to be configured in Single Wire (Half-Duplex) Mode
 * 		Baud Rate 		9600 Bits/s
 * 		Word Length		8 Bits (including parity)
 * 		Parity			None
 * 		Stop Bits		1
 * 		Data Direction	Receive and Transmit
 */

/*
 * TIM has to output PWM 50% duty cycle with varying Frequency,
 * also it has to be able to count pulses or pulse certain number of times
 *
 * Funkcja przyjmuje czestotliwosc i ilosc krokow
 * nastepnie ustawia timer z przerwaniami
 * uruchamia go
 * 	A w przerwaniach toggluje sie stepa
 * dopóki timer nie doliczy do zera
 */
