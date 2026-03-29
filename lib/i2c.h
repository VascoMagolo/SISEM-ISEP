#ifndef LIB_I2C_H_
#define LIB_I2C_H_

#include "DAVE.h"

#define LCD_I2C_ADDRESS 0x4E

void i2c_write(I2C_MASTER_t *controller, uint8_t data);
void lcd_init(I2C_MASTER_t *controller);
void lcd_send_command(I2C_MASTER_t *controller, uint8_t cmd);
void lcd_send_data(I2C_MASTER_t *controller, uint8_t data);
void lcd_send_string(I2C_MASTER_t *controller, char *str);
void lcd_set_cursor(I2C_MASTER_t *controller, uint8_t row, uint8_t col);
void lcd_clear(I2C_MASTER_t *controller);

#endif /* LIB_I2C_H_ */
