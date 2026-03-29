#include <stdarg.h>
#include <stdio.h>

#include "uart.h"

void uart_send_string(const UART_t* const handler, const char* str) {
  while (*str != '\0') {
    while (UART_IsTXFIFOFull(handler)) {}

    UART_TransmitWord(handler, (uint8_t)(*str));
    str++;
  }
}

void uart_printf(const UART_t* const handler, const char* format, ...) {
  char buffer[128];
  va_list args;

  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  uart_send_string(handler, buffer);
}

uint8_t uart_read_byte(const UART_t* const handler) {
  while (UART_IsRXFIFOEmpty(handler)) {}

  return UART_GetReceivedWord(handler);
}

uint32_t uart_read_line(const UART_t* const handler, char* buffer, uint32_t max_length) {
  uint32_t count = 0;
  char c;

  while (count < (max_length - 1)) {
    c = (char)uart_read_byte(handler);

    if (c == '\n' || c == '\r') {
      if (count == 0) continue;
      break;
    }

    buffer[count] = c;
    count++;

    while (UART_IsTXFIFOFull(handler)) {}
	UART_TransmitWord(handler, (uint8_t)c);
  }

  buffer[count] = '\0';
  return count;
}
