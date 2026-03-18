#include "uart.h"

bool uart_receive_str(const UART_t* const handler, char* str, uint32_t size) {
  return UART_Receive(handler, (uint8_t*)str, size) == UART_STATUS_SUCCESS;
}

bool uart_send(const UART_t* const handler, const char* str) {
  return UART_Transmit(handler, (uint8_t*)str, strlen(str)) ==
         UART_STATUS_SUCCESS;
}

void uart_receive_blocking(const UART_t* const handler, char* buffer,
                           uint32_t length) {
  for (uint32_t i = 0; i < length; i++) {
    while (!uart_receive_str(handler, &buffer[i], 1)) {
    }
  }
}
