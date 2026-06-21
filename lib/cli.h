#ifndef LIB_CLI_H_
#define LIB_CLI_H_

#include "DAVE.h"
#include "../config.h"

void cli_print_header(const UART_t* const handler);
void cli_print_menu(const UART_t* const handler);
void cli_process_char(const UART_t* const handler, char c);

# if defined(FEATURE_CAN)
     void cli_process_can_rx(const UART_t* const handler, uint16_t can_id, const uint8_t data[8]);
# endif // FEATURE_CAN

# if defined(FEATURE_LCD)
    typedef enum {
        LCD_MODE_OFF,
        LCD_MODE_CAN_RX_AHT10,
        LCD_MODE_POT,

#       if defined(FEATURE_AHT10)
            LCD_MODE_LOCAL_AHT10,
#       endif // FEATURE_AHT10

#       if defined(FEATURE_GPS)
            LCD_MODE_GPS,
#       endif // FEATURE_GPS

        LCD_MODE_TEXT,
    } lcd_mode_t;

    extern volatile lcd_mode_t lcd_mode;
    extern char lcd_row1[17];
    extern char lcd_row2[17];
# endif // FEATURE_LCD

# if defined(FEATURE_EEPROM)
    void cli_save_settings(void);
    void cli_load_settings(void);
# endif // FEATURE_EEPROM

#endif /* LIB_CLI_H_ */
