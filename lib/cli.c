#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "DAVE.h"
#include "../config.h"
#include "cli.h"
#include "uart.h"

#define TICK_MS 10U

# ifdef FEATURE_AHT10
#   include "aht10.h"
# endif

# ifdef FEATURE_LCD
#   include "lcd.h"
    volatile lcd_mode_t lcd_mode = LCD_MODE_OFF;

    static void print_lcd_menu(const UART_t *const handler) {
        uart_send_string(handler, "\r\nLCD mode:\r\n"
            " 0 - Off\r\n"
            " 1 - Sensor (Temp + Humidity)\r\n"
            " 2 - Potentiometer\r\n");
    }
# endif

# ifdef FEATURE_CAN
#   include "can.h"

    static char     can_id_buf[5];  // up to 4 hex digits for 0x7FF
    static uint8_t  can_id_buf_len  = 0;
    static uint16_t can_filter_id   = 0;
    static bool     can_filter_active = false;

    void cli_process_can_rx(const UART_t *const handler, uint16_t can_id, const uint8_t data[8]) {
        if (!can_filter_active || can_id != can_filter_id) return;
        static uint8_t last_data[8] = { 0 };
        if (memcmp(data, last_data, 8) == 0) return;
        memcpy(last_data, data, 8);
        float temp, hum;
        can_decode_sensor(data, &temp, &hum);
        uart_printf(handler, "\r\n[CAN 0x%03X] Temp: %.1f C  Hum: %.1f %%\r\n", can_id, temp, hum);
    }

    static void apply_can_filter(const UART_t *const handler) {
        uint32_t value = (uint32_t)strtoul(can_id_buf, NULL, 16);
        if (value > 0x7FF) {
            uart_send_string(handler, "\r\n[Warning] Adjusted to maximum: 0x7FF.\r\n");
            value = 0x7FF;
        }
        can_filter_id     = (uint16_t)value;
        can_filter_active = true;
        uart_printf(handler, "\r\nFiltering CAN ID 0x%03X\r\n", (unsigned)value);
        cli_print_menu(handler);
    }
# endif


volatile uint32_t timer_interval = 2000;  // full blink period in ms

extern volatile uint32_t led_tick_interval;
extern volatile uint16_t potentiometer_value;

typedef enum {
    CLI_STATE_MENU,
    CLI_STATE_READING_TIMER,
#   ifdef FEATURE_CAN
        CLI_STATE_READING_CAN_ID,
#   endif
#   ifdef FEATURE_LCD
        CLI_STATE_LCD_MODE,
#   endif
} cli_state_t;

static cli_state_t cli_state    = CLI_STATE_MENU;
static char        timer_buf[6];  // up to 5 digits for 10000 (ms)
static uint8_t     timer_buf_len = 0;

void cli_print_header(const UART_t *const handler) {
    uart_send_string(handler, "Grupo 3, Diogo Nogueira/Vasco Magolo, 1241692/1231562\r\n");
}

void cli_print_menu(const UART_t *const handler) {
    uart_send_string(handler, "\r\n\nMenu\r\n"
        " 0 - Exit\r\n"
        " 1 - Potentiometer value\r\n"
        " 2 - Set led blinking rate\r\n"
        " 3 - Blinking rate value\r\n"
#       ifdef FEATURE_AHT10
            " 4 - Read AHT10 Sensor\r\n"
#       endif
#       ifdef FEATURE_CAN
            " 5 - Set CAN RX filter\r\n"
#       endif
#       ifdef FEATURE_LCD
            " 6 - Set LCD mode\r\n"
#       endif
    );
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

    timer_interval    = value_ms;
    led_tick_interval = (value_ms / 2) / TICK_MS;

    uart_printf(handler, "\r\nBlink period set to %lu ms\r\n", value_ms);
    cli_print_menu(handler);
}

void cli_process_char(const UART_t *const handler, char c) {
    switch (cli_state) {

    case CLI_STATE_MENU:
        switch (c) {
        case '0':
            uart_send_string(handler, "\r\nExiting...\r\n");
            while (1) {}
            break;
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
#       ifdef FEATURE_AHT10
            case '4': {
                float temp = 0.0f;
                float hum  = 0.0f;
                uint8_t data[6] = { 0 };
                uart_send_string(handler, "\r\nReading AHT10...\r\n");
                if (aht10_read(data)) {
                    aht10_parse_humidity(&hum, data);
                    aht10_parse_temperature(&temp, data);
                    uart_printf(handler, "\r\nTemperature: %.1f C\r\nHumidity: %.1f %%\r\n",
                        temp, hum);
                } else {
                    uart_send_string(handler, "\r\n[Error] Sensor busy or not responding.\r\n");
                }
                cli_print_menu(handler);
                break;
            }
#       endif
#       ifdef FEATURE_CAN
            case '5':
                uart_send_string(handler, "\r\nCAN ID to filter (hex, e.g. 4C0):\r\n");
                can_id_buf_len = 0;
                cli_state = CLI_STATE_READING_CAN_ID;
                break;
#       endif
#       ifdef FEATURE_LCD
            case '6':
                print_lcd_menu(handler);
                cli_state = CLI_STATE_LCD_MODE;
                break;
#       endif
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

#   ifdef FEATURE_LCD
        case CLI_STATE_LCD_MODE:
            switch (c) {
            case '0':
                lcd_mode = LCD_MODE_OFF;
                lcd_clear(&I2C_MASTER_0);
                uart_send_string(handler, "\r\nLCD off.\r\n");
                break;
            case '1':
                lcd_mode = LCD_MODE_SENSOR;
                lcd_clear(&I2C_MASTER_0);
                lcd_write(&I2C_MASTER_0, 1, "Waiting AHT10...");
                uart_send_string(handler, "\r\nLCD: Sensor mode.\r\n");
                break;
            case '2':
                lcd_mode = LCD_MODE_POT;
                lcd_clear(&I2C_MASTER_0);
                uart_send_string(handler, "\r\nLCD: Potentiometer mode.\r\n");
                break;
            default:
                uart_printf(handler, "\r\n[Error] '%c' is not a valid option.\r\n", c);
                break;
            }
            cli_state = CLI_STATE_MENU;
            cli_print_menu(handler);
            break;
#   endif

#   ifdef FEATURE_CAN
        case CLI_STATE_READING_CAN_ID:
            if (c == '\r' || c == '\n') {
                if (can_id_buf_len == 0) {
                    cli_state = CLI_STATE_MENU;
                    cli_print_menu(handler);
                    break;
                }
                can_id_buf[can_id_buf_len] = '\0';
                apply_can_filter(handler);
                cli_state = CLI_STATE_MENU;
            } else if (!isxdigit((unsigned char)c)) {
                uart_send_string(handler, "\r\n[Error] Hex digits only.\r\n");
                cli_state = CLI_STATE_MENU;
                cli_print_menu(handler);
            } else if (can_id_buf_len < (sizeof(can_id_buf) - 1)) {
                can_id_buf[can_id_buf_len++] = c;
                while (UART_IsTXFIFOFull(handler)) {}
                UART_TransmitWord(handler, (uint8_t)c);
            }
            break;
#   endif
    }
}

