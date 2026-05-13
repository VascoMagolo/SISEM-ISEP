#ifndef LIB_CLI_H_
#define LIB_CLI_H_

#include "DAVE.h"

void cli_print_header(const UART_t* const handler);
void cli_print_menu(const UART_t* const handler);
void cli_process_char(const UART_t* const handler, char c);

#endif /* LIB_CLI_H_ */
