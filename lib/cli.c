#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "DAVE.h"
#include "cli.h"
#include "i2c.h"
#include "uart.h"

// bypass confusing TIMER api units (1 unit = 10ns)
#define TIMER_MS(ms) ((uint32_t)(ms) * 100000U)

volatile uint32_t timer_interval = 1000;

void cli_print_header(const UART_t* const handler) {
	uart_send_string(handler,
			"Grupo 3, Diogo Nogueira/Vasco Magolo, 1241692/1231562\r\n");
}

void cli_print_menu(const UART_t* const handler) {
	uart_send_string(handler, "\r\n\nMenu\r\n"
			" 1 - Potentiometer value \r\n"
			" 2 - Set led blinking rate \r\n"
			" 3 - Blinking rate value \r\n"
			" 4 - Exit\r\n");
}

void cli_process_command(const UART_t* const handler, char cmd,
		uint16_t current_adc_val) {
	switch (cmd) {
	case '1': {
		lcd_clear(&I2C_MASTER_0);
		lcd_set_cursor(&I2C_MASTER_0, 0, 0); // row 0 col 0
		lcd_send_string(&I2C_MASTER_0, "Potentiometer");
		lcd_set_cursor(&I2C_MASTER_0, 1, 0); // row 1 col 0
		char data_lcd[16] = { 0 };
		sprintf(data_lcd, "Value: %u", current_adc_val);
		lcd_send_string(&I2C_MASTER_0, data_lcd);

		uart_printf(handler, "\r\nADC Value: %u\r\n", current_adc_val);
		cli_print_menu(handler);

		break;
	}
	case '2': {
		uart_send_string(handler, "\r\nSend timer interval max 4 digits:\r\n");
		char timer_v[5] = { 0 };
		uint32_t chars_read = uart_read_line(handler, timer_v, sizeof(timer_v));

		if (chars_read == 0) {
			cli_print_menu(handler);
			break;
		}

		for (int i = 0; i < chars_read; i++) {
			if (!isdigit((unsigned char )timer_v[i])) {
				uart_send_string(handler,
						"\r\n[Error] Invalid input. Please enter numbers only.\r\n");
				cli_print_menu(handler);
				return;
			}
		}

		uint32_t value_ms = (uint32_t) atoi(timer_v);

		if (value_ms < 50) {
			uart_send_string(handler,
					"\r\n[Warning] Value is too low. Adjusted to 50 ms.\r\n");
			value_ms = 50;
		} else if (value_ms > 5000) {
			uart_send_string(handler,
					"\r\n[Warning] Value is too high. Adjusted to 5000 ms.\r\n");
			value_ms = 5000;
		}

		TIMER_Stop(&TIMER_0);
		TIMER_Clear(&TIMER_0);
		timer_interval = value_ms;
		TIMER_SetTimeInterval(&TIMER_0, TIMER_MS(value_ms));
		TIMER_Start(&TIMER_0);

		uart_printf(handler, "\r\nTimer set to: %lu ms\r\n", value_ms);

		cli_print_menu(handler);
		break;
	}
	case '3': {
		lcd_clear(&I2C_MASTER_0);
		lcd_set_cursor(&I2C_MASTER_0, 0, 0); // row 0 col 0
		lcd_send_string(&I2C_MASTER_0, "Timer Value:");
		lcd_set_cursor(&I2C_MASTER_0, 1, 0); // row 1 col 0
		char data_lcd[16] = { 0 };
		sprintf(data_lcd, "%lu ms", timer_interval);

		uart_printf(handler, "\r\nTimer value: %lu ms\r\n", timer_interval);

		cli_print_menu(handler);
		break;
	}
	case '4': {
		uart_send_string(handler, "\r\nExiting...\r\n");
		while (1) {
		};
		break;
	}
	default:
		uart_printf(handler, "\r\n[Error] '%c' is not a valid option.\r\n",
				cmd);
		cli_print_menu(handler);
		break;
	}
}
