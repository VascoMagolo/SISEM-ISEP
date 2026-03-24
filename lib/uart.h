#ifndef LIB_UART_H_
#define LIB_UART_H_

#include <stdint.h>
#include "DAVE.h"

void uart_send_string(const UART_t* const handler, const char *str);
void uart_printf(const UART_t* const handler, const char *format, ...);

uint8_t uart_read_byte(const UART_t* const handler);
uint32_t uart_read_line(const UART_t* const handler, char *buffer, uint32_t max_length);

#endif /* LIB_UART_H_ */
