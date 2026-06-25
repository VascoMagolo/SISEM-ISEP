#ifndef APP_STATE_H_
#define APP_STATE_H_

#include <stdint.h>
#include <stdbool.h>

extern volatile uint16_t potentiometer_value;
extern volatile bool     led_blinking;
extern volatile bool     led_on;
extern volatile uint32_t led_tick_interval;
extern volatile uint32_t timer_interval;

#endif /* APP_STATE_H_ */
