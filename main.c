#include <stdbool.h>

#include "DAVE.h"
#include "lib/cli.h"
#include "lib/uart.h"

volatile bool led_blinking = false;
volatile uint16_t potentiometer_value = 0;
volatile bool micrium_btn_pressed = false;

void led_interrupt(void) {
    if (led_blinking) {
        DIGITAL_IO_ToggleOutput(&DIGITAL_IO_LED);
    } else {
        DIGITAL_IO_SetOutputHigh(&DIGITAL_IO_LED);
    }
}

void CAN_A_RX() {
	CAN_NODE_STATUS_t receive_status;
	CAN_NODE_STATUS_t status;
	XMC_CAN_MO_t *MO_Ptr;
	const CAN_NODE_t *HandlePtr1 = &CAN_NODE_A;
	MO_Ptr = HandlePtr1->lmobj_ptr[0]->mo_ptr;

	status = CAN_NODE_MO_GetStatus(HandlePtr1->lmobj_ptr[0]);
	//Check receive pending status
	if (status & XMC_CAN_MO_STATUS_RX_PENDING) {
		// Clear the flag
		XMC_CAN_MO_ResetStatus(MO_Ptr, XMC_CAN_MO_RESET_STATUS_RX_PENDING);
		// Read the received Message object
		receive_status = CAN_NODE_MO_Receive(HandlePtr1->lmobj_ptr[0]);
		if (receive_status == CAN_NODE_STATUS_SUCCESS) {
			memcpy(data_rx, &MO_Ptr->can_data[0], sizeof(MO_Ptr->can_data[0]));
			memcpy(data_rx + sizeof(MO_Ptr->can_data[0]), &MO_Ptr->can_data[1],
					sizeof(MO_Ptr->can_data[1]));

			length_rx = MO_Ptr->can_data_length;

			flag_data_rx = 0x01;
		} else {
			// message object failed to receive.
		}
	}

	return;
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
