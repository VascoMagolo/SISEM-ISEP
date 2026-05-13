#ifndef LIB_AHT10_H
#define LIB_AHT10_H

#include <stdint.h>
#include <stdbool.h>

bool aht10_read(uint8_t data[6]);
void aht10_parse_temperature(float* temperature, uint8_t data[]);
void aht10_parse_humidity(float* humidity, uint8_t data[]);

#endif /* LIB_AHT10_H */
