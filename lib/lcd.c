#include "lcd.h"

#include <stdarg.h>
#include <stdio.h>

#define LCD_I2C_ADDRESS 0x4E

static const uint8_t row_offsets[] = {0x00, 0x40};

static void i2c_write(I2C_MASTER_t* controller, uint8_t data) {
  static uint8_t tx_data;
  tx_data = data;
  I2C_MASTER_Transmit(controller, true, LCD_I2C_ADDRESS, &tx_data, 1, true);
  for (volatile int bus_settle = 0; bus_settle < 100; bus_settle++);
  while (I2C_MASTER_IsTxBusy(controller));
}

static void lcd_send_nibble(I2C_MASTER_t* controller, uint8_t nibble,
                            uint8_t register_select) {
  uint8_t i2c_byte =
      nibble | register_select | 0x08;     // 0x08 keeps backlight on
  i2c_write(controller, i2c_byte | 0x04);  // EN = 1: latch data
  for (volatile uint32_t en_hold = 0; en_hold < 1000; en_hold++);
  i2c_write(controller, i2c_byte & ~0x04);  // EN = 0: falling edge clocks LCD
  for (volatile uint32_t en_hold = 0; en_hold < 1000; en_hold++);
}

static void lcd_send_command(I2C_MASTER_t* controller, uint8_t cmd) {
  lcd_send_nibble(controller, cmd & 0xF0, 0);
  lcd_send_nibble(controller, (cmd << 4) & 0xF0, 0);
}

static void lcd_send_char(I2C_MASTER_t* controller, char ch) {
  lcd_send_nibble(controller, (uint8_t)ch & 0xF0, 1);
  lcd_send_nibble(controller, ((uint8_t)ch << 4) & 0xF0, 1);
}

static void lcd_set_cursor(I2C_MASTER_t* controller, int col, int row) {
  lcd_send_command(controller, 0x80 | (col + row_offsets[row]));
}

void lcd_clear(I2C_MASTER_t* controller) {
  lcd_send_command(controller, 0x01);
  for (volatile uint32_t clear_delay = 0; clear_delay < 10000; clear_delay++);
}

void lcd_init(I2C_MASTER_t* controller) {
  for (volatile uint32_t power_on_delay = 0; power_on_delay < 50000;
       power_on_delay++);
  lcd_send_command(controller, 0x33);  // reset step 1: 8-bit mode
  lcd_send_command(controller, 0x32);  // reset step 2: switch to 4-bit mode
  lcd_send_command(controller, 0x28);  // 4-bit mode, 2 rows, 5x8 matrix
  lcd_send_command(controller, 0x0C);  // display on, cursor hidden
  lcd_send_command(controller, 0x06);  // cursor increments right
  lcd_clear(controller);
}

void lcd_write(I2C_MASTER_t* controller, int row, char str[16]) {
  lcd_set_cursor(controller, 0, row - 1);
  for (int char_idx = 0; char_idx < 16 && str[char_idx]; char_idx++) {
    lcd_send_char(controller, str[char_idx]);
  }
}

void lcd_printf(I2C_MASTER_t* controller, int row, const char* fmt, ...) {
  char buf[17];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  lcd_write(controller, row, buf);
}
