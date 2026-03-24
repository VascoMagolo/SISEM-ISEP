#include <stdio.h>
#include <stdarg.h>
#include "uart.h"

void uart_send_string(const UART_t* const handler, const char *str) {
    while (*str != '\0') {
        // Bloqueia apenas enquanto o FIFO de TX estiver cheio
        while (UART_IsTXFIFOFull(handler)) {
            // Aguarda espaço no buffer
        }
        UART_TransmitWord(handler, (uint8_t)(*str));
        str++;
    }
}

void uart_printf(const UART_t* const handler, const char *format, ...) {
    char buffer[128]; // Buffer local para a formatação (ajusta se precisares de mais)
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    uart_send_string(handler, buffer);
}


uint8_t uart_read_byte(const UART_t* const handler) {
    // Bloqueia até que um byte seja recebido
    while (UART_IsRXFIFOEmpty(handler)) {
        // Aguarda dado
    }
    return UART_GetReceivedWord(handler);
}

uint32_t uart_read_line(const UART_t* const handler, char *buffer, uint32_t max_length) {
    uint32_t count = 0;
    char c;

    while (count < (max_length - 1)) {
        c = (char)uart_read_byte(handler); // Usa a nossa função bloqueante

        // Identifica o fim de linha (enter no Docklight)
        if (c == '\n' || c == '\r') {
            if (count == 0) continue; // Ignora se for o início (ex: sobra de \r\n)
            break;
        }

        buffer[count] = c;
        count++;
    }

    buffer[count] = '\0'; // Termina a string estilo C
    return count;
}
