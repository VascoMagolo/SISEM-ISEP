#include "app_state.h"

volatile uint16_t potentiometer_value = 0;
volatile bool     led_blinking        = false;
volatile bool     led_on              = false;
volatile uint32_t led_tick_interval   = 100; // 100 ticks * 10ms = 1000ms default
volatile uint32_t timer_interval      = 2000; // full blink period in ms
