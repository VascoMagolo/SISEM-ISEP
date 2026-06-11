#ifndef LIB_I2C_H_
#define LIB_I2C_H_

#include <stdint.h>

void endTxCallback(void);
void endRxCallback(void);

void i2c_transmit(uint16_t addr, const uint8_t *data, uint32_t len);
void i2c_receive(uint16_t addr, uint8_t *data, uint32_t len);

#endif /* LIB_I2C_H_ */
