#include <stdbool.h>

#include "DAVE.h"
#include "config.h"
#include "app_state.h"
#include "lib/cli.h"
#include "lib/uart.h"

#define TIMER_MS(ms) ((uint32_t)(ms) * 100000) // 100000 timer ticks per ms

#if defined(FEATURE_LCD)
#   include "lib/lcd.h"
#endif // FEATURE_LCD

#if defined(FEATURE_AHT10)
#   include "lib/aht10.h"
#endif // FEATURE_AHT10

#if defined(FEATURE_CAN)
#   include "lib/can.h"
#   define CAN_SENSOR_TICKS (500 / TICK_MS)   // fire every 500ms
    static volatile bool can_sensor_tick = false;
#endif // FEATURE_CAN

#if defined(FEATURE_GPS)
#   include "lib/gps.h"
#endif // FEATURE_GPS

static volatile uint32_t tick_count = 0;
static volatile bool     adc_tick   = false;
static volatile bool     btn_event  = false;

void system_tick(void) {
    tick_count++;

    adc_tick = true;

    static bool btn_prev = false;
    bool btn_now = (DIGITAL_IO_GetInput(&DIGITAL_IO_BTN) == 1);
    if (btn_now && !btn_prev) {
        btn_event = true;
    }
    btn_prev = btn_now;

#   if defined(FEATURE_CAN)
        if (tick_count % CAN_SENSOR_TICKS == 0) {
            can_sensor_tick = true;
        }
#   endif // FEATURE_CAN

    if (!led_blinking) {
        DIGITAL_IO_SetOutputHigh(&DIGITAL_IO_LED);
        led_on = true;
    } else if (tick_count % led_tick_interval == 0) {
        DIGITAL_IO_ToggleOutput(&DIGITAL_IO_LED);
        led_on = !led_on;
    }
}

int main(void) {
    if (DAVE_Init() != DAVE_STATUS_SUCCESS) {
        XMC_DEBUG("DAVE APPs initialization failed\n");
        return 1;
    }

    TIMER_Stop(&TIMER_TICK);
    TIMER_Clear(&TIMER_TICK);
    TIMER_SetTimeInterval(&TIMER_TICK, TIMER_MS(TICK_MS));
    TIMER_Start(&TIMER_TICK);

    ADC_MEASUREMENT_StartConversion(&ADC_POTENTIOMETER);

#   if defined(FEATURE_LCD)
        lcd_init(&I2C_MASTER);
#   endif // FEATURE_LCD

#   if defined(FEATURE_EEPROM)
        cli_load_settings();
        uart_send_string(&UART_CLI, "[EEPROM] Loaded settings:\r\n");
        uart_printf(&UART_CLI, "  Timer interval : %lu ms\r\n", timer_interval);
#       if defined(FEATURE_CAN)
            uart_printf(&UART_CLI, "  CAN filter ID  : 0x%03X (%s)\r\n", (unsigned)can_filter_id, can_filter_active ? "active" : "inactive");
#       endif // FEATURE_CAN
#       if defined(FEATURE_LCD)
            uart_printf(&UART_CLI, "  LCD mode       : %u\r\n", (unsigned)lcd_mode);
            uart_printf(&UART_CLI, "  LCD row 1      : \"%s\"\r\n", lcd_row1);
            uart_printf(&UART_CLI, "  LCD row 2      : \"%s\"\r\n", lcd_row2);
#       endif // FEATURE_LCD
#   endif // FEATURE_EEPROM

    cli_print_header(&UART_CLI);
    cli_print_menu(&UART_CLI);

    while (1) {
        if (adc_tick) {
            adc_tick = false;
            
            potentiometer_value = ADC_MEASUREMENT_GetResult(&ADC_MEASUREMENT_CHANNEL_0_handle);
            PWM_SetDutyCycle(&PWM_POTENTIOMETER, (uint32_t)potentiometer_value * 10000 / 255); // scale 0-255 to 0-10000
            
#           if defined(FEATURE_LCD)
                if (lcd_mode == LCD_MODE_POT) {
                    static uint8_t lcd_pot_ticks = 0;
                    if (++lcd_pot_ticks >= 10) {
                        lcd_pot_ticks = 0;
                        lcd_clear(&I2C_MASTER);
                        lcd_printf(&I2C_MASTER, 1, "Pot: %u", potentiometer_value);
                    }
                }
#           endif // FEATURE_LCD
        }

        if (btn_event) {
            btn_event = false;
            led_blinking = !led_blinking;
        }

#       if defined(FEATURE_CAN)
            if (flag_data_rx) {
                flag_data_rx = 0;
                cli_process_can_rx(&UART_CLI, can_id_rx, data_rx);

#               if defined(FEATURE_LCD)
                    if (lcd_mode == LCD_MODE_CAN_RX_AHT10) {
                        float rx_temp, rx_hum;
                        can_decode_sensor(data_rx, &rx_temp, &rx_hum);
                        
                        lcd_clear(&I2C_MASTER);
                        lcd_printf(&I2C_MASTER, 1, "CAN ID: 0x%03X", can_id_rx);
                        lcd_printf(&I2C_MASTER, 2, "T:%.1fC H:%.1f%%", rx_temp, rx_hum);
                    }
#               endif // FEATURE_LCD
            }

            if (can_sensor_tick) {
                can_sensor_tick = false;

#               if defined(FEATURE_AHT10)
                    uint8_t raw[8] = { 0 };
                    if (aht10_read(raw)) {
                        aht10_parse_temperature(&temperature, raw);
                        aht10_parse_humidity(&humidity, raw);

                        can_send_sensor(&CAN_NODE, CAN_ID_SENSOR, temperature, humidity);

#                       if defined(FEATURE_LCD)
                            if (lcd_mode == LCD_MODE_LOCAL_AHT10) {
                                lcd_clear(&I2C_MASTER);
                                lcd_printf(&I2C_MASTER, 1, "T: %.1f C", temperature);
                                lcd_printf(&I2C_MASTER, 2, "H: %.1f %%", humidity);
                            }
#                       endif // FEATURE_LCD
                    }
#               endif // FEATURE_AHT10
            }
#       endif // FEATURE_CAN

#       if defined(FEATURE_GPS)
            while (!UART_IsRXFIFOEmpty(&UART_GPS)) {
                gps_feed((char)UART_GetReceivedWord(&UART_GPS));
            }

            if (gps_updated) {
                gps_updated = false;

                static bool    prev_fix_valid  = true;
                static uint8_t prev_satellites = 0;

                if (gps_data.fix_valid) {
                    uart_printf(&UART_CLI,
                        "\r\n[GPS] %02u:%02u:%02uZ  Lat: %.6f  Lon: %.6f  Satellites: %u\r\n",
                        gps_data.hour, gps_data.minute, gps_data.second,
                        gps_data.latitude, gps_data.longitude, gps_data.satellites);
#                   if defined(FEATURE_CAN)
                        can_send_gps(&CAN_NODE, CAN_ID_GPS_POS, gps_data);
#                   endif // FEATURE_CAN
                } else if (prev_fix_valid || gps_data.satellites != prev_satellites) {
                    uart_printf(&UART_CLI,
                        "\r\n[GPS] %02u:%02u:%02uZ  No fix  Satellites: %u\r\n",
                        gps_data.hour, gps_data.minute, gps_data.second,
                        gps_data.satellites);
                }
#               if defined(FEATURE_CAN)
                    can_send_gps_meta(&CAN_NODE, CAN_ID_GPS_META, gps_data);
#               endif // FEATURE_CAN

                prev_fix_valid  = gps_data.fix_valid;
                prev_satellites = gps_data.satellites;

#               if defined(FEATURE_LCD)
                    if (lcd_mode == LCD_MODE_GPS) {
                        static bool    last_fix  = false;
                        static float   last_lat  = 0.0f;
                        static float   last_lon  = 0.0f;
                        static uint8_t last_sats = 0xFF;

                        if (gps_data.fix_valid) {
                            if (!last_fix || gps_data.latitude != last_lat || gps_data.longitude != last_lon) {
                                last_fix = true;
                                last_lat = gps_data.latitude;
                                last_lon = gps_data.longitude;

                                lcd_clear(&I2C_MASTER);
                                lcd_printf(&I2C_MASTER, 1, "Lat: %.6f", gps_data.latitude);
                                lcd_printf(&I2C_MASTER, 2, "Lon: %.6f", gps_data.longitude);
                            }
                        } else {
                            if (last_fix || gps_data.satellites != last_sats) {
                                last_fix  = false;
                                last_sats = gps_data.satellites;
                                
                                lcd_clear(&I2C_MASTER);
                                lcd_write(&I2C_MASTER, 1, "No GPS fix");
                                lcd_printf(&I2C_MASTER, 2, "Satellites: %u", gps_data.satellites);
                            }
                        }
                    }
#               endif // FEATURE_LCD
            }
#       endif // FEATURE_GPS

        while (!UART_IsRXFIFOEmpty(&UART_CLI)) {
            char c = (char)UART_GetReceivedWord(&UART_CLI);
            cli_process_char(&UART_CLI, c);
        }
    }
}
