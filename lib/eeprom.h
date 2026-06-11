#ifndef LIB_EEPROM_H_
#define LIB_EEPROM_H_

#include <stdbool.h>
#include <stdint.h>

bool eeprom_read(uint16_t addr, uint8_t *data, uint16_t len);
void eeprom_write(uint16_t addr, const uint8_t *data, uint16_t len);

#endif /* LIB_EEPROM_H_ */
