#include <stdbool.h>
#include <stdio.h>

#include "DAVE.h"
#include "config.h"
#include "lib/cli.h"
#include "lib/uart.h"

#define TICK_MS 10U
#define TIMER_MS(ms) ((uint32_t)(ms) * 100000U)

# ifdef FEATURE_LCD
#    include "lib/lcd.h"
# endif

# ifdef FEATURE_AHT10
#    include "lib/aht10.h"
     volatile float temperature = 0.0f;
     volatile float humidity = 0.0f;
# endif

# ifdef FEATURE_CAN
#    include "lib/can.h"
#    define CAN_SENSOR_TICKS (500U / TICK_MS)  // 500ms : 10ms = 50 ticks
     static volatile bool can_sensor_tick = false;
# endif

# ifdef FEATURE_GPS
#    include "lib/gps.h"
# endif

volatile uint16_t potentiometer_value = 0;
volatile bool     led_blinking        = false;
volatile uint32_t led_tick_interval   = 100;  // 100 ticks * 10ms = 1000ms default

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

#   ifdef FEATURE_CAN
        if (tick_count % CAN_SENSOR_TICKS == 0) {
            can_sensor_tick = true;
        }
#   endif

    if (!led_blinking) {
        DIGITAL_IO_SetOutputHigh(&DIGITAL_IO_LED);
    } else if (tick_count % led_tick_interval == 0) {
        DIGITAL_IO_ToggleOutput(&DIGITAL_IO_LED);
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

#   ifdef FEATURE_LCD
        lcd_init(&I2C_MASTER);
#   endif

    cli_print_header(&UART_CLI);
    cli_print_menu(&UART_CLI);

    while (1) {
        if (adc_tick) {
            adc_tick = false;
            potentiometer_value = ADC_MEASUREMENT_GetResult(&ADC_MEASUREMENT_CHANNEL_0_handle);
            PWM_SetDutyCycle(&PWM_POTENTIOMETER, (uint32_t)potentiometer_value * 10000 / 255);
#           ifdef FEATURE_LCD
                if (lcd_mode == LCD_MODE_POT) {
                    static uint8_t lcd_pot_ticks = 0;
                    if (++lcd_pot_ticks >= 10) {
                        lcd_pot_ticks = 0;
                        char buf[17];
                        lcd_clear(&I2C_MASTER);
                        snprintf(buf, sizeof(buf), "Pot: %u", potentiometer_value);
                        lcd_write(&I2C_MASTER, 1, buf);
                    }
                }
#           endif
        }

        if (btn_event) {
            btn_event = false;
            led_blinking = !led_blinking;
        }

#       ifdef FEATURE_CAN
            if (flag_data_rx) {
                flag_data_rx = 0;
                cli_process_can_rx(&UART_CLI, can_id_rx, data_rx);
#               ifdef FEATURE_LCD
                    if (lcd_mode == LCD_MODE_SENSOR) {
                        float rx_temp, rx_hum;
                        can_decode_sensor(data_rx, &rx_temp, &rx_hum);
                        char buf[17];
                        lcd_clear(&I2C_MASTER);
                        snprintf(buf, sizeof(buf), "CAN ID: 0x%03X", can_id_rx);
                        lcd_write(&I2C_MASTER, 1, buf);
                        snprintf(buf, sizeof(buf), "T:%.1fC H:%.1f%%", rx_temp, rx_hum);
                        lcd_write(&I2C_MASTER, 2, buf);
                    }
#               endif
            }

            if (can_sensor_tick) {
                can_sensor_tick = false;
#               ifdef FEATURE_AHT10
                    uint8_t raw[8] = { 0 };
                    if (aht10_read(raw)) {
                        aht10_parse_temperature((float*)&temperature, raw);
                        aht10_parse_humidity((float*)&humidity, raw);
                        can_send_sensor(&CAN_NODE, CAN_ID_GROUP, temperature, humidity);
                    }
#               endif
            }
#       endif

#       ifdef FEATURE_GPS
            while (!UART_IsRXFIFOEmpty(&UART_GPS)) {
                gps_feed((char)UART_GetReceivedWord(&UART_GPS));
            }

            if (gps_updated) {
                gps_updated = false;
                if (gps_data.fix_valid) {
                    uart_printf(&UART_CLI,
                        "\r\n[GPS] %02u:%02u:%02uZ  Lat: %.6f  Lon: %.6f  Satellites: %u\r\n",
                        gps_data.hour, gps_data.minute, gps_data.second,
                        gps_data.latitude, gps_data.longitude, gps_data.satellites);
#                   ifdef FEATURE_CAN
                        can_send_gps(&CAN_NODE, CAN_ID_GROUP, gps_data.latitude, gps_data.longitude);
#                   endif
                } else {
                    uart_printf(&UART_CLI,
                        "\r\n[GPS] %02u:%02u:%02uZ  No fix  Satellites: %u\r\n",
                        gps_data.hour, gps_data.minute, gps_data.second,
                        gps_data.satellites);
                }
#               ifdef FEATURE_LCD
                    if (lcd_mode == LCD_MODE_GPS) {
                        static bool    last_fix  = false;
                        static float   last_lat  = 0.0f;
                        static float   last_lon  = 0.0f;
                        static uint8_t last_sats = 0xFF;
                        char buf[17];
                        if (gps_data.fix_valid) {
                            if (!last_fix || gps_data.latitude != last_lat || gps_data.longitude != last_lon) {
                                last_fix = true;
                                last_lat = gps_data.latitude;
                                last_lon = gps_data.longitude;
                                snprintf(buf, sizeof(buf), "Lat: %.6f", gps_data.latitude);
                                lcd_clear(&I2C_MASTER);
                                lcd_write(&I2C_MASTER, 1, buf);
                                snprintf(buf, sizeof(buf), "Lon: %.6f", gps_data.longitude);
                                lcd_write(&I2C_MASTER, 2, buf);
                            }
                        } else {
                            if (last_fix || gps_data.satellites != last_sats) {
                                last_fix  = false;
                                last_sats = gps_data.satellites;
                                lcd_clear(&I2C_MASTER);
                                lcd_write(&I2C_MASTER, 1, "No GPS fix");
                                snprintf(buf, sizeof(buf), "Satellites: %u", gps_data.satellites);
                                lcd_write(&I2C_MASTER, 2, buf);
                            }
                        }
                    }
#               endif
            }
#       endif

        while (!UART_IsRXFIFOEmpty(&UART_CLI)) {
            char c = (char)UART_GetReceivedWord(&UART_CLI);
            cli_process_char(&UART_CLI, c);
        }
    }
}
