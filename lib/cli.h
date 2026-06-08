#ifndef LIB_CLI_H_
#define LIB_CLI_H_

#include "DAVE.h"
#include "../config.h"

void cli_print_header(const UART_t* const handler);
void cli_print_menu(const UART_t* const handler);
void cli_process_char(const UART_t* const handler, char c);

# ifdef FEATURE_CAN
     void cli_process_can_rx(const UART_t* const handler, uint16_t can_id, const uint8_t data[8]);
# endif

# ifdef FEATURE_LCD
    typedef enum {
        LCD_MODE_OFF,
        LCD_MODE_SENSOR,
        LCD_MODE_POT,
#       ifdef FEATURE_GPS
            LCD_MODE_GPS,
#       endif
    } lcd_mode_t;

    extern volatile lcd_mode_t lcd_mode;
# endif

#endif /* LIB_CLI_H_ */
