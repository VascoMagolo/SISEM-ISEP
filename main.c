#include <stdbool.h>

#include "DAVE.h"
#include "lib/cli.h"
#include "lib/uart.h"

volatile bool led_blinking = false;
volatile uint16_t potentiometer_value = 0;
volatile bool micrium_btn_pressed = false;

void interrup(void) {
    if (led_blinking) {
        DIGITAL_IO_ToggleOutput(&DIGITAL_IO_LED);
    } else {
        DIGITAL_IO_SetOutputHigh(&DIGITAL_IO_LED);
    }
}

static void update_pwm_from_potentiometer(volatile uint16_t *pot_val) {
    *pot_val = ADC_MEASUREMENT_GetResult(&ADC_MEASUREMENT_CHANNEL_0_handle);
    PWM_SetDutyCycle(&PWM_0, (uint32_t)(*pot_val) * 10000 / 255);
}

static void check_button_toggle(bool *prev_state) {
    bool pressed = (DIGITAL_IO_GetInput(&DIGITAL_IO_BTN) == 1);
    if (pressed && !(*prev_state)) {
        micrium_btn_pressed = !micrium_btn_pressed;
    }
    led_blinking = micrium_btn_pressed;
    *prev_state = pressed;
}

int main(void) {
    if (DAVE_Init() != DAVE_STATUS_SUCCESS) {
        XMC_DEBUG("DAVE APPs initialization failed\n");
        return 1;
    }

    cli_print_header(&UART_0);
    cli_print_menu(&UART_0);

    ADC_MEASUREMENT_StartConversion(&ADC_POTENTIOMETER);

    bool btn_pressed_prev = false;

    while (1) {
        WATCHDOG_Service();
        update_pwm_from_potentiometer(&potentiometer_value);
        check_button_toggle(&btn_pressed_prev);

        if (!UART_IsRXFIFOEmpty(&UART_0)) {
            char c = (char)uart_read_byte(&UART_0);
			cli_process_command(&UART_0, c, potentiometer_value);
        }
    }
}
