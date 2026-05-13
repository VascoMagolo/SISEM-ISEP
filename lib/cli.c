#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "DAVE.h"
#include "cli.h"
#include "uart.h"
#include "aht10.h"

#define TICK_MS 10U
volatile uint32_t timer_interval = 2000;  // full blink period in ms

// set by main when user changes the LED blink interval
extern volatile uint32_t led_tick_interval;

// updated by the timer ISR every tick
extern volatile uint16_t potentiometer_value;

typedef enum {
    CLI_STATE_MENU,
    CLI_STATE_READING_TIMER,
} cli_state_t;

static cli_state_t cli_state = CLI_STATE_MENU;
static char timer_buf[6];  // up to 5 digits for 10000 (ms)
static uint8_t timer_buf_len = 0;

void cli_print_header(const UART_t *const handler) {
    uart_send_string(handler, "Grupo 3, Diogo Nogueira/Vasco Magolo, 1241692/1231562\r\n");
}

void cli_print_menu(const UART_t *const handler) {
    uart_send_string(handler, "\r\n\nMenu\r\n"
        " 1 - Potentiometer value\r\n"
        " 2 - Set led blinking rate\r\n"
        " 3 - Blinking rate value\r\n"
        " 4 - Read AHT10 Sensor\r\n"
        " 5 - Exit\r\n");
}

static void apply_timer_interval(const UART_t *const handler) {
    uint32_t value_ms = (uint32_t)atoi(timer_buf);

    if (value_ms < 100) {
        uart_send_string(handler, "\r\n[Warning] Adjusted to minimum: 100 ms.\r\n");
        value_ms = 100;
    } else if (value_ms > 10000) {
        uart_send_string(handler, "\r\n[Warning] Adjusted to maximum: 10000 ms.\r\n");
        value_ms = 10000;
    }

    timer_interval = value_ms;
    led_tick_interval = (value_ms / 2) / TICK_MS;

    uart_printf(handler, "\r\nBlink period set to %lu ms\r\n", value_ms);
    cli_print_menu(handler);
}

void cli_process_char(const UART_t *const handler, char c) {
    switch (cli_state) {

    case CLI_STATE_MENU:
        switch (c) {
        case '1':
            uart_printf(handler, "\r\nADC Value: %u\r\n", potentiometer_value);
            cli_print_menu(handler);
            break;
        case '2':
            uart_send_string(handler, "\r\nLED blink period in ms (full on/off cycle, 100-10000):\r\n");
            timer_buf_len = 0;
            cli_state = CLI_STATE_READING_TIMER;
            break;
        case '3':
            uart_printf(handler, "\r\nBlink period: %lu ms\r\n", timer_interval);
            cli_print_menu(handler);
            break;
        case '4': {
            float temp = 0.0f;
            float hum = 0.0f;
			uint8_t data[6] = { 0 };
            uart_send_string(handler, "\r\nReading AHT10...\r\n");
			if (aht10_read(data)) {
				aht10_parse_humidity(&hum, data);
				aht10_parse_temperature(&temp, data);
                uart_printf(handler, "\r\nTemperature: %d C\r\nHumidity: %d %%\r\n",
                    (int)temp, (int)hum);
            } else {
                uart_send_string(handler, "\r\n[Error] Sensor busy or not responding.\r\n");
            }
            cli_print_menu(handler);
            break;
        }
        case '5':
            uart_send_string(handler, "\r\nExiting...\r\n");
            while (1) {}
            break;
        default:
            uart_printf(handler, "\r\n[Error] '%c' is not a valid option.\r\n", c);
            cli_print_menu(handler);
            break;
        }
        break;

    case CLI_STATE_READING_TIMER:
        if (c == '\r' || c == '\n') {
            if (timer_buf_len == 0) {
                cli_state = CLI_STATE_MENU;
                cli_print_menu(handler);
                break;
            }
            timer_buf[timer_buf_len] = '\0';
            apply_timer_interval(handler);
            cli_state = CLI_STATE_MENU;
        } else if (!isdigit((unsigned char)c)) {
            uart_send_string(handler, "\r\n[Error] Numbers only.\r\n");
            cli_state = CLI_STATE_MENU;
            cli_print_menu(handler);
        } else if (timer_buf_len < (sizeof(timer_buf) - 1)) {
            timer_buf[timer_buf_len++] = c;
            while (UART_IsTXFIFOFull(handler)) {}
            UART_TransmitWord(handler, (uint8_t)c);
        }
        break;
    }
}
