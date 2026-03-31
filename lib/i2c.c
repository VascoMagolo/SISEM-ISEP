#include "i2c.h"


void i2c_write(I2C_MASTER_t *controller, uint8_t data) {
    static uint8_t tx_data;
    tx_data = data;

    I2C_MASTER_Transmit(controller, true, LCD_I2C_ADDRESS, &tx_data, 1, true);
    for(volatile int i=0; i<100; i++);
    while(I2C_MASTER_IsTxBusy(controller));
}

// sends 4 bits to the lcd controller
void lcd_send_nibble(I2C_MASTER_t *controller, uint8_t nibble, uint8_t rs) {
    uint8_t data_i2c = nibble | rs | 0x08; // 0x08 keeps backlight on

    i2c_write(controller, data_i2c | 0x04); // enables "Enable" flag (EN = 1)
    for(volatile uint32_t i=0; i<1000; i++);

    i2c_write(controller, data_i2c & ~0x04); // disables "Enable" flag (EN = 0)
    for(volatile uint32_t i=0; i<1000; i++);
}

// send a command to LCD controller
void lcd_send_command(I2C_MASTER_t *controller, uint8_t cmd) {
    uint8_t high_nibble = cmd & 0xF0;
    uint8_t low_nibble = (cmd << 4) & 0xF0;

    lcd_send_nibble(controller, high_nibble, 0); // RS = 0 for commands
    lcd_send_nibble(controller, low_nibble, 0);
}

// sends a char to LCD
void lcd_send_data(I2C_MASTER_t *controller, uint8_t data) {
    uint8_t high_nibble = data & 0xF0;
    uint8_t low_nibble = (data << 4) & 0xF0;

    lcd_send_nibble(controller, high_nibble, 1); // RS = 1 for data (text)
    lcd_send_nibble(controller, low_nibble, 1);
}

// initializes lcd in 4bit mode
void lcd_init(I2C_MASTER_t *controller) {
    // delay to allow the lcd to turn on
    for(volatile uint32_t i=0; i<50000; i++);

    lcd_send_command(controller, 0x33);
    lcd_send_command(controller, 0x32);
	lcd_send_command(controller, 0x28); // 4bit mode, 2 rows, 5x8 matrix
    lcd_send_command(controller, 0x0C); // turns display on, hides cursor
    lcd_send_command(controller, 0x06); // increments cursor
    lcd_clear(controller);
}

// writes a string to LCD
void lcd_send_string(I2C_MASTER_t *controller, char *str) {
    while (*str) {
        lcd_send_data(controller, *str++);
    }
}

// moves cursor (row 0-1, col 0-15)
void lcd_set_cursor(I2C_MASTER_t *controller, uint8_t row, uint8_t col) {
    uint8_t row_offsets[] = {0x00, 0x40};
    lcd_send_command(controller, 0x80 | (col + row_offsets[row]));
}

// clean lcd screen
void lcd_clear(I2C_MASTER_t *controller) {
    lcd_send_command(controller, 0x01);
    for(volatile uint32_t i=0; i<10000; i++);
}
