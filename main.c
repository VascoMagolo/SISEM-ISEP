#include <stdbool.h>

#include "DAVE.h"
#include "config.h"
#include "lib/cli.h"
#include "lib/uart.h"

#define TICK_MS 10U
#define TIMER_MS(ms) ((uint32_t)(ms) * 100000U)

# ifdef FEATURE_AHT10
#	include "lib/aht10.h"
	volatile float temperature = 0.0f;
	volatile float humidity = 0.0f;
# endif

# ifdef FEATURE_CAN
#	include "lib/can.h"
#	define CAN_SENSOR_TICKS (500U / TICK_MS)  // 500ms : 10ms = 50 ticks
	static volatile bool can_sensor_tick = false;
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

#	ifdef FEATURE_CAN
		if (tick_count % CAN_SENSOR_TICKS == 0) {
			can_sensor_tick = true;
		}
#	endif

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

    TIMER_Stop(&TIMER_0);
    TIMER_Clear(&TIMER_0);
    TIMER_SetTimeInterval(&TIMER_0, TIMER_MS(TICK_MS));
    TIMER_Start(&TIMER_0);

    // The UART APP configures an RX FIFO interrupt (IRQn 89) whose handler
    // calls UART_lReceiveHandler. Since we never call UART_Receive(), that
    // handler does nothing but the interrupt would keep re-asserting while
    // bytes sit in the FIFO. Disable it and poll the FIFO directly instead.
    NVIC_DisableIRQ((IRQn_Type)89);

    ADC_MEASUREMENT_StartConversion(&ADC_POTENTIOMETER);

    cli_print_header(&UART_0);
    cli_print_menu(&UART_0);

    while (1) {
        WATCHDOG_Service();

        if (adc_tick) {
            adc_tick = false;
            potentiometer_value = ADC_MEASUREMENT_GetResult(&ADC_MEASUREMENT_CHANNEL_0_handle);
            PWM_SetDutyCycle(&PWM_0, (uint32_t)potentiometer_value * 10000 / 255);
        }

        if (btn_event) {
            btn_event = false;
            led_blinking = !led_blinking;
        }

#		ifdef FEATURE_CAN
			if (flag_data_rx) {
				flag_data_rx = 0;
				cli_process_can_rx(&UART_0, can_id_rx, data_rx);
			}

			if (can_sensor_tick) {
				can_sensor_tick = false;
#			ifdef FEATURE_AHT10
				uint8_t raw[8] = { 0 };
				if (aht10_read(raw)) {
					aht10_parse_temperature((float*)&temperature, raw);
					aht10_parse_humidity((float*)&humidity, raw);
					can_send_sensor(&CAN_NODE_0, CAN_ID_GROUP, temperature, humidity);
				}
#			endif
        }
#		endif

        while (!UART_IsRXFIFOEmpty(&UART_0)) {
            char c = (char)UART_GetReceivedWord(&UART_0);
            cli_process_char(&UART_0, c);
        }
    }
}
