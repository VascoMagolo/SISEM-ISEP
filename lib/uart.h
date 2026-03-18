#ifndef LIB_UART_H_
#define LIB_UART_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "DAVE.h"

bool uart_receive_str(const UART_t* const handler, char* str, uint32_t size);
void uart_receive_blocking(const UART_t* const handler, char* buffer,
                           uint32_t length);

bool uart_send(const UART_t* const handler, const char* str);

#endif /* LIB_UART_H_ */
