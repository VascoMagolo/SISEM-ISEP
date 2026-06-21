#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "DAVE.h"
#include "../config.h"
#include "../app_state.h"
#include "cli.h"
#include "uart.h"

#if defined(FEATURE_AHT10)
#   include "aht10.h"
#endif // FEATURE_AHT10

#if defined(FEATURE_LCD)
#   include "lcd.h"
    volatile lcd_mode_t lcd_mode = LCD_MODE_OFF;

    static void print_lcd_menu(const UART_t *const handler) {
        uart_send_string(handler, "\r\nLCD mode:\r\n"
            " 0 - Off\r\n"
#           if defined(FEATURE_CAN)
                " 1 - Sensor (CAN RX Temp + Humidity)\r\n"
#           endif // FEATURE_CAN
            " 2 - Potentiometer\r\n"
#           if defined(FEATURE_GPS)
                " 3 - GPS\r\n"
#           endif // FEATURE_GPS
#           if defined(FEATURE_AHT10)
                " 4 - AHT10 (local sensor)\r\n"
#           endif // FEATURE_AHT10
            " 5 - Custom text\r\n"
        );
    }
#endif // FEATURE_LCD

#if defined(FEATURE_CAN)
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
        
#       if defined(FEATURE_EEPROM)
            cli_save_settings();
#       endif // FEATURE_EEPROM

        cli_print_menu(handler);
    }
#endif // FEATURE_CAN

#if defined(FEATURE_LCD)
    char lcd_row1[17] = {0};
    char lcd_row2[17] = {0};
#endif // FEATURE_LCD

#if defined(FEATURE_EEPROM)
#   include "eeprom.h"

#   define EEPROM_MAGIC         0xAB
#   define EEPROM_SETTINGS_ADDR 0x0000

    typedef union {
        struct __attribute__((packed)) eeprom_header_fields {
            uint8_t  magic;
            uint32_t timer_interval;
            uint16_t can_filter_id;
            uint8_t  can_filter_active;
            uint8_t  lcd_mode;
        } as_fields;
        uint8_t as_bytes[sizeof(struct eeprom_header_fields)];
    } eeprom_header_t;

    void cli_save_settings(void) {
        eeprom_header_t settings = {0};
        settings.as_fields.magic          = EEPROM_MAGIC;
        settings.as_fields.timer_interval = timer_interval;

#       if defined(FEATURE_CAN)
            settings.as_fields.can_filter_id     = can_filter_id;
            settings.as_fields.can_filter_active = can_filter_active ? 1 : 0;
#       endif // FEATURE_CAN

#       if defined(FEATURE_LCD)
            settings.as_fields.lcd_mode = (uint8_t)lcd_mode;
#       endif // FEATURE_LCD

        eeprom_write(EEPROM_SETTINGS_ADDR, settings.as_bytes, sizeof(settings.as_bytes));

#       if defined(FEATURE_LCD)
            eeprom_write(EEPROM_SETTINGS_ADDR + sizeof(settings.as_bytes),      (const uint8_t *)lcd_row1, 16);
            eeprom_write(EEPROM_SETTINGS_ADDR + sizeof(settings.as_bytes) + 16, (const uint8_t *)lcd_row2, 16);
#       endif // FEATURE_LCD
    }

    void cli_load_settings(void) {
        eeprom_header_t settings = {0};
        if (!eeprom_read(EEPROM_SETTINGS_ADDR, settings.as_bytes, sizeof(settings.as_bytes))) return;
        if (settings.as_fields.magic != EEPROM_MAGIC) return;

        if (settings.as_fields.timer_interval >= 100 && settings.as_fields.timer_interval <= 10000) {
            timer_interval    = settings.as_fields.timer_interval;
            led_tick_interval = (settings.as_fields.timer_interval / 2) / TICK_MS;
        }

#       if defined(FEATURE_CAN)
            can_filter_id     = settings.as_fields.can_filter_id;
            can_filter_active = (settings.as_fields.can_filter_active != 0);
#       endif // FEATURE_CAN

#       if defined(FEATURE_LCD)
            lcd_mode = (lcd_mode_t)settings.as_fields.lcd_mode;
            eeprom_read(EEPROM_SETTINGS_ADDR + sizeof(settings.as_bytes),      (uint8_t *)lcd_row1, 16);
            eeprom_read(EEPROM_SETTINGS_ADDR + sizeof(settings.as_bytes) + 16, (uint8_t *)lcd_row2, 16);
            lcd_row1[16] = '\0';
            lcd_row2[16] = '\0';
#       endif // FEATURE_LCD
    }
#endif // FEATURE_EEPROM

typedef enum {
    CLI_STATE_MENU,
    CLI_STATE_READING_TIMER,

#   if defined(FEATURE_CAN)
        CLI_STATE_READING_CAN_ID,
#   endif // FEATURE_CAN

#   if defined(FEATURE_LCD)
        CLI_STATE_LCD_MODE,
#   endif // FEATURE_LCD

#   if defined(FEATURE_LCD)
        CLI_STATE_READING_LCD_ROW,
#   endif // FEATURE_LCD
} cli_state_t;

static cli_state_t cli_state    = CLI_STATE_MENU;
static char        timer_buf[6];  // up to 5 digits for 10000 (ms)
static uint8_t     timer_buf_len = 0;

#if defined(FEATURE_LCD)
    static char    lcd_row_buf[17];
    static uint8_t lcd_row_buf_len = 0;
    static uint8_t lcd_row_target  = 0;
#endif // FEATURE_LCD

void cli_print_header(const UART_t *const handler) {
    uart_send_string(handler, "Grupo 3, Diogo Nogueira/Vasco Magolo, 1241692/1231562\r\n");
}

void cli_print_menu(const UART_t *const handler) {
    uart_send_string(handler, "\r\n\nMenu\r\n"
        " 0 - Exit\r\n"
        " 1 - Potentiometer value\r\n"
        " 2 - Set led blinking rate\r\n"
        " 3 - Blinking rate value\r\n"
#       if defined(FEATURE_AHT10)
            " 4 - Read AHT10 Sensor\r\n"
#       endif // FEATURE_AHT10
#       if defined(FEATURE_CAN)
            " 5 - Set CAN RX filter\r\n"
#       endif // FEATURE_CAN
#       if defined(FEATURE_LCD)
            " 6 - Set LCD mode\r\n"
            " 7 - Write LCD row 1\r\n"
            " 8 - Write LCD row 2\r\n"
#       endif // FEATURE_LCD
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

#   if defined(FEATURE_EEPROM)
        cli_save_settings();
#   endif // FEATURE_EEPROM

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
#       if defined(FEATURE_AHT10)
            case '4': {
                float temp = 0.0f;
                float hum  = 0.0f;
                uint8_t data[6] = { 0 };

                uart_send_string(handler, "\r\nReading AHT10...\r\n");
                if (aht10_read(data)) {
                    aht10_parse_humidity(&hum, data);
                    aht10_parse_temperature(&temp, data);

                    uart_printf(handler, "\r\nTemperature: %.1f C\r\nHumidity: %.1f %%\r\n", temp, hum);
                } else {
                    uart_send_string(handler, "\r\n[Error] Sensor busy or not responding.\r\n");
                }

                cli_print_menu(handler);
                break;
            }
#       endif // FEATURE_AHT10

#       if defined(FEATURE_CAN)
            case '5':
                uart_send_string(handler, "\r\nCAN ID to filter (hex, e.g. 4C0):\r\n");
                can_id_buf_len = 0;
                cli_state = CLI_STATE_READING_CAN_ID;
                break;
#       endif // FEATURE_CAN

#       if defined(FEATURE_LCD)
            case '6':
                print_lcd_menu(handler);
                cli_state = CLI_STATE_LCD_MODE;
                break;
            case '7':
                uart_send_string(handler, "\r\nLCD row 1 text (max 16 chars):\r\n");
                lcd_row_buf_len = 0;
                lcd_row_target  = 1;
                cli_state = CLI_STATE_READING_LCD_ROW;
                break;
            case '8':
                uart_send_string(handler, "\r\nLCD row 2 text (max 16 chars):\r\n");
                lcd_row_buf_len = 0;
                lcd_row_target  = 2;
                cli_state = CLI_STATE_READING_LCD_ROW;
                break;
#       endif // FEATURE_LCD

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
            while (UART_IsTXFIFOFull(handler)) {
                // waits
            }
            UART_TransmitWord(handler, (uint8_t)c);
        }
        break;

#   if defined(FEATURE_LCD)
        case CLI_STATE_LCD_MODE:
            switch (c) {
            case '0':
                lcd_mode = LCD_MODE_OFF;
                lcd_clear(&I2C_MASTER);
                uart_send_string(handler, "\r\nLCD off.\r\n");
                break;

#           if defined(FEATURE_CAN)
                case '1':
                    lcd_mode = LCD_MODE_CAN_RX_AHT10;
                    lcd_clear(&I2C_MASTER);
                    lcd_write(&I2C_MASTER, 1, "Waiting CAN RX..");
                    uart_send_string(handler, "\r\nLCD: Sensor mode.\r\n");
                    break;
#           endif // FEATURE_CAN

            case '2':
                lcd_mode = LCD_MODE_POT;
                lcd_clear(&I2C_MASTER);
                uart_send_string(handler, "\r\nLCD: Potentiometer mode.\r\n");
                break;

#           if defined(FEATURE_AHT10)
                case '4':
                    lcd_mode = LCD_MODE_LOCAL_AHT10;
                    lcd_clear(&I2C_MASTER);
                    lcd_write(&I2C_MASTER, 1, "AHT10 local");
                    uart_send_string(handler, "\r\nLCD: AHT10 local sensor mode.\r\n");
                    break;
#           endif // FEATURE_AHT10

#           if defined(FEATURE_GPS)
                case '3':
                    lcd_mode = LCD_MODE_GPS;
                    lcd_clear(&I2C_MASTER);
                    lcd_write(&I2C_MASTER, 1, "Waiting GPS fix.");
                    uart_send_string(handler, "\r\nLCD: GPS mode.\r\n");
                    break;
#           endif // FEATURE_GPS

            case '5': {
                lcd_mode = LCD_MODE_TEXT;
                lcd_clear(&I2C_MASTER);
                lcd_printf(&I2C_MASTER, 1, "%-16s", lcd_row1)
                lcd_printf(&I2C_MASTER, 2, "%-16s", lcd_row2);
                uart_send_string(handler, "\r\nLCD: Custom text mode.\r\n");
                break;
            }

            default:
                uart_printf(handler, "\r\n[Error] '%c' is not a valid option.\r\n", c);
                break;
            }

#           if defined(FEATURE_EEPROM)
                cli_save_settings();
#           endif // FEATURE_EEPROM

            cli_state = CLI_STATE_MENU;
            cli_print_menu(handler);
            break;
#   endif // FEATURE_LCD

#   if defined(FEATURE_CAN)
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
#   endif // FEATURE_CAN

#   if defined(FEATURE_LCD)
        case CLI_STATE_READING_LCD_ROW:
            if (c == '\r' || c == '\n') {
                char *dst = (lcd_row_target == 1) ? lcd_row1 : lcd_row2;

                memset(dst, 0, 17);
                memcpy(dst, lcd_row_buf, lcd_row_buf_len);
#               if defined(FEATURE_EEPROM)
                    cli_save_settings();
#               endif // FEATURE_EEPROM

                uart_printf(handler, "\r\nLCD row %u written.\r\n", (unsigned)lcd_row_target);
                cli_state = CLI_STATE_MENU;
                cli_print_menu(handler);
            } else if (lcd_row_buf_len < 16) {
                lcd_row_buf[lcd_row_buf_len++] = c;
                while (UART_IsTXFIFOFull(handler)) {
                    // waits
                }
                
                UART_TransmitWord(handler, (uint8_t)c);
            }
            break;
#   endif // FEATURE_LCD
    }
}

