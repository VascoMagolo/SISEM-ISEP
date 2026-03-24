#ifndef LIB_CLI_H_
#define LIB_CLI_H_

#include <stdint.h>

#include "DAVE.h"

void cli_print_header(const UART_t* const handler);
void cli_print_menu(const UART_t* const handler);
void cli_process_command(const UART_t* const handler, char cmd, uint16_t current_adc_val);

#endif /* LIB_CLI_H_ */
