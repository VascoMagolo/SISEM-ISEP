#include "cli.h"

#include <stdio.h>
#include <stdlib.h>

#include "uart.h"

void cli_print_header(const UART_t* const handler) {
  uart_send(handler, "Grupo 3, Diogo Nogueira/Vasco Magolo, 1241692/1231562\n");
}

void cli_print_menu(const UART_t* const handler) {
  uart_send(handler,
            "\nMenu\n 1 - Potentiometer value \n 2 - Set led blinking rate \n "
            "3 - Blinking rate value \n 4 - Exit\n> ");
}

void cli_process_command(const UART_t* const handler, char cmd,
                         uint16_t current_adc_val) {
  switch (cmd) {
    case '1': {
      char output[32];
      sprintf(output, "\nADC Value: %u\n", current_adc_val);
      uart_send(handler, output);
      cli_print_menu(handler);
      break;
    }
    case '2': {
      uart_send(handler, "\nSend timer interval max 6 chars:\n");

      char timer_v[7] = {0};
      uart_receive_blocking(handler, timer_v, 6);

      uint32_t value = (uint32_t)atoi(timer_v);
      TIMER_Stop(&TIMER_0);
      TIMER_Clear(&TIMER_0);
      TIMER_SetTimeInterval(&TIMER_0, value);
      TIMER_Start(&TIMER_0);

      char conf[64] = {0};
      sprintf(conf, "\nTimer set to: %lu\n", value);
      uart_send(handler, conf);

      cli_print_menu(handler);
      break;
    }
    case '3': {
      uart_send(handler, "\nFeature coming soon...\n");
      cli_print_menu(handler);
      break;
    }
    case '4': {
      uart_send(handler, "\nExiting...\n");
      break;
    }
    default:
      break;
  }
}
