#ifndef LIB_AHT10_H
#define LIB_AHT10_H

#include <stdint.h>
#include <stdbool.h>

bool aht10_read(float *temperature, float *humidity);

#endif /* LIB_AHT10_H */
