#ifndef LIB_LCD_H_
#define LIB_LCD_H_

#include "DAVE.h"

void lcd_init(I2C_MASTER_t *controller);
void lcd_write(I2C_MASTER_t *controller, int row, char str[16]);
void lcd_clear(I2C_MASTER_t *controller);

#endif /* LIB_LCD_H_ */
