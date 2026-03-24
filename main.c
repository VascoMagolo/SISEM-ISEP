#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DAVE.h"
#include "lib/cli.h"
#include "lib/uart.h"

volatile bool led_blinking = false;
volatile uint16_t potentiometer_value = 0;


void interrup() {
  if (led_blinking) {
    DIGITAL_IO_ToggleOutput(&DIGITAL_IO_LED);
  } else {
    DIGITAL_IO_SetOutputHigh(&DIGITAL_IO_LED);
  }
}

void update_pwm_from_potentiometer(uint16_t* pot_val) {
  *pot_val = ADC_MEASUREMENT_GetResult(&ADC_MEASUREMENT_CHANNEL_0_handle);
  int duty_cycle = (int)(*pot_val) * 10000 / 255;
  PWM_SetDutyCycle(&PWM_0, duty_cycle);
}

void check_button_toggle(bool* prev_state) {
  bool btn_pressed = DIGITAL_IO_GetInput(&DIGITAL_IO_LED) == 1;

  if (btn_pressed && !(*prev_state)) {
    led_blinking = !led_blinking;
  }
  *prev_state = btn_pressed;
}

int main(void) {
  DAVE_STATUS_t status;
  status = DAVE_Init();

  if (status != DAVE_STATUS_SUCCESS) {
    XMC_DEBUG("DAVE APPs initialization failed\n");

    return 1;
  }

  cli_print_header(&UART_0);
  cli_print_menu(&UART_0);

  ADC_MEASUREMENT_StartConversion(&ADC_POTENTIOMETER);

  potentiometer_value = 0;
  bool btn_pressed_prev = false;

  while (1U) {
    update_pwm_from_potentiometer(&potentiometer_value);
    check_button_toggle(&btn_pressed_prev);

    if (!UART_IsRXFIFOEmpty(&UART_0)) {

          // Como sabemos que há dados, a leitura é instantânea
          char uart_input = (char)uart_read_byte(&UART_0);

          // Processa o comando
          cli_process_command(&UART_0, uart_input, potentiometer_value);
        }
  }
}
