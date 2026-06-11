#ifndef LIB_AHT10_H
#define LIB_AHT10_H

#include <stdint.h>
#include <stdbool.h>

extern volatile float temperature;
extern volatile float humidity;

bool aht10_read(uint8_t data[6]);
void aht10_parse_temperature(volatile float *out, uint8_t data[]);
void aht10_parse_humidity(volatile float *out, uint8_t data[]);

#endif /* LIB_AHT10_H */
