#include <string.h>
#include <stdlib.h>

#include "gps.h"

#define GPS_BUF_SIZE 100

gps_data_t gps_data    = {0};
bool       gps_updated = false;

static char    buf[GPS_BUF_SIZE];
static uint8_t buf_len = 0;

static float parse_coord(const char* val, char dir) {
    if (!val || val[0] == '\0') return 0.0f;

    float raw = (float)atof(val);
    int   deg = (int)(raw / 100.0f);
    float min = raw - (float)(deg * 100);
    float result = (float)deg + min / 60.0f;
    
    if (dir == 'S' || dir == 'W') {
        result = -result;
    }
    
    return result;
}

/*
 * f[0]=$GPGGA
 * f[1]=HHMMSS
 * f[2]=lat
 * f[3]=N/S
 * f[4]=lon
 * f[5]=E/W
 * f[6]=fix
 * f[7]=satellites
 */
static void parse_gpgga(char* sentence) {
    char* star = strchr(sentence, '*');
    if (star) {
        *star = '\0';
    }

    char*   fields[15] = {0};
    uint8_t field_count = 0;
    char*   cursor = sentence;
    while (*cursor && field_count < 15) {
        fields[field_count++] = cursor;
        while (*cursor && *cursor != ',') cursor++;
        if (*cursor == ',') *cursor++ = '\0';
    }
    if (field_count < 8) return;

    gps_data.fix_valid  = ((uint8_t)atoi(fields[6]) > 0);
    gps_data.satellites = (uint8_t)atoi(fields[7]);

    if (fields[1] && fields[1][0] != '\0') {
        char digit_buf[3] = { fields[1][0], fields[1][1], '\0' };
        gps_data.hour = (uint8_t)atoi(digit_buf);
        digit_buf[0] = fields[1][2];
        digit_buf[1] = fields[1][3];
        gps_data.minute = (uint8_t)atoi(digit_buf);
        digit_buf[0] = fields[1][4];
        digit_buf[1] = fields[1][5];
        gps_data.second = (uint8_t)atoi(digit_buf);
    }

    if (gps_data.fix_valid && fields[2] && fields[2][0] != '\0') {
        gps_data.latitude  = parse_coord(fields[2], fields[3] ? fields[3][0] : 'N');
        gps_data.longitude = parse_coord(fields[4], fields[5] ? fields[5][0] : 'E');
    }

    gps_updated = true;
}

void gps_feed(char c) {
    if (c == '$') {
        buf_len = 0;
        buf[buf_len++] = c;
        return;
    }

    if (buf_len == 0) return;
    
    if (buf_len < GPS_BUF_SIZE - 1) {
        buf[buf_len++] = c;
    }

    if (c == '\n') {
        buf[buf_len] = '\0';
        
        while (buf_len > 0 && (buf[buf_len - 1] == '\r' || buf[buf_len - 1] == '\n')) {
            buf[--buf_len] = '\0';
        }

        if (strncmp(buf, "$GPGGA,", 7) == 0) {
            parse_gpgga(buf);
        }

        buf_len = 0;
    }
}
