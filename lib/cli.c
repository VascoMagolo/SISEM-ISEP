#include "cli.h"
#include <stdlib.h>     // Necessário para o atoi()
#include "uart.h" // A nossa nova API
#include "DAVE.h"       // Necessário para aceder à TIMER_0

// Nota: Como o wrapper já sabe qual é o UART (MY_UART_HANDLE),
// podemos remover o argumento "handler" destas funções.

volatile uint32_t timer_interval;

void cli_print_header(const UART_t* const handler) {
  uart_send_string(handler, "Grupo 3, Diogo Nogueira/Vasco Magolo, 1241692/1231562\r\n");
}

void cli_print_menu(const UART_t* const handler) {
  uart_send_string(handler,
      "\r\n\nMenu\r\n"
      " 1 - Potentiometer value \r\n"
      " 2 - Set led blinking rate \r\n"
      " 3 - Blinking rate value \r\n"
      " 4 - Exit\r\n");
}

void cli_process_command(const UART_t* const handler, char cmd, uint16_t current_adc_val) {
  switch (cmd) {
    case '1': {
      // O UART_Printf substitui o sprintf + array temporário + uart_send
      uart_printf(handler, "\r\nADC Value: %u\r\n", current_adc_val);
      cli_print_menu(handler);
      break;
    }

    case '2': {
      uart_send_string(handler, "\r\nSend timer interval max 6 chars:\r\n");

      char timer_v[7] = {0};
      // Usamos a nossa nova função de leitura
      uart_read_line(handler, timer_v, sizeof(timer_v));

      uint32_t value = (uint32_t)atoi(timer_v);

      TIMER_Stop(&TIMER_0);
      TIMER_Clear(&TIMER_0);
      timer_interval = value;
      TIMER_SetTimeInterval(&TIMER_0, value);
      TIMER_Start(&TIMER_0);

      // Confirmação direta, sem arrays extra
      uart_printf(handler, "\r\nTimer set to: %lu\r\n", value);

      cli_print_menu(handler);
      break;
    }

    case '3': {
      uart_printf(handler, "\r\nTimer value: %lu\r\n", timer_interval);

      cli_print_menu(handler);
      break;
    }

    case '4': {
      uart_send_string(handler, "\r\nExiting...\r\n");
      break;
    }

    default:
      break;
  }
}
