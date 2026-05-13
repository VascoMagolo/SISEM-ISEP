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
