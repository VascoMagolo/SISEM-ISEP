#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DAVE.h"
#include "lib/cli.h"
#include "lib/uart.h"

volatile bool led_blinking = false;
volatile uint16_t potentiometer_value = 0;
volatile bool micrium_btn_pressed = false;

void interrup() {
  if (led_blinking) {
    DIGITAL_IO_ToggleOutput(&DIGITAL_IO_LED);
  } else {
    DIGITAL_IO_SetOutputHigh(&DIGITAL_IO_LED);
  }
}

void update_pwm_from_potentiometer(volatile uint16_t* pot_val) {

  *pot_val = ADC_MEASUREMENT_GetResult(&ADC_MEASUREMENT_CHANNEL_0_handle);

  int duty_cycle = (int)(*pot_val) * 10000 / 255;
  PWM_SetDutyCycle(&PWM_0, duty_cycle);
}

void check_button_toggle(bool* prev_state) {
  bool physical_btn_pressed = (DIGITAL_IO_GetInput(&DIGITAL_IO_BTN) == 1);

  bool physical_triggered = physical_btn_pressed && !(*prev_state);

  if (physical_triggered) {
      micrium_btn_pressed = !micrium_btn_pressed;
  }

  led_blinking = micrium_btn_pressed;

  *prev_state = physical_btn_pressed;
}

int main(void) {
  DAVE_STATUS_t status;
  status = DAVE_Init();

  if (status != DAVE_STATUS_SUCCESS) {
    XMC_DEBUG("DAVE APPs initialization failed\n");

    return 1;
  }

  lcd_init(&I2C_MASTER_0);
  lcd_clear(&I2C_MASTER_0);
  lcd_set_cursor(&I2C_MASTER_0, 0, 0);
  lcd_send_string(&I2C_MASTER_0, "XMC4200 Ready!");
  lcd_set_cursor(&I2C_MASTER_0, 1, 0);
  lcd_send_string(&I2C_MASTER_0, "Group 3");

  cli_print_header(&UART_0);
  cli_print_menu(&UART_0);

  ADC_MEASUREMENT_StartConversion(&ADC_POTENTIOMETER);

  potentiometer_value = 0;
  bool btn_pressed_prev = false;

  while (1U) {
	WATCHDOG_Service();

    update_pwm_from_potentiometer(&potentiometer_value);
    check_button_toggle(&btn_pressed_prev);

    if (!UART_IsRXFIFOEmpty(&UART_0)) {
      char uart_input = (char)uart_read_byte(&UART_0);

      cli_process_command(&UART_0, uart_input, potentiometer_value);
    }
  }
}
