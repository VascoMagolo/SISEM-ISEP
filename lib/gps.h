#ifndef LIB_GPS_H_
#define LIB_GPS_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float   latitude;
    float   longitude;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t satellites;
    bool    fix_valid;
} gps_data_t;

extern gps_data_t gps_data;
extern bool       gps_updated;

void gps_feed(char c);

#endif /* LIB_GPS_H_ */
