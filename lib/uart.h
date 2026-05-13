#ifndef LIB_UART_H_
#define LIB_UART_H_

#include <stdint.h>
#include "DAVE.h"

void uart_send_string(const UART_t* const handler, const char *str);
void uart_printf(const UART_t* const handler, const char *format, ...);

#endif /* LIB_UART_H_ */
