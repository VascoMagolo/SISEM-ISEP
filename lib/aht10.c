#include "aht10.h"

#include "i2c.h"

// 1 byte shifted i2c address for AHT10 (0x38 << 1)
#define AHT10_ADDRESS 0x70

volatile float temperature = 0.0f;
volatile float humidity    = 0.0f;

// Spin-wait required by the sensor between the measurement trigger and data
// read (~80ms per datasheet)
static void delay_aht10(void) {
    volatile uint32_t count = 0x8FFFF;
    while (--count) {
      // waits
    }
}

// byte 1: HHHHHHHH (most significant H bits)
// byte 2: HHHHHHHH (middle significant H bits)
// byte 3(top 4 bits): HHHH (least significant H bits)
// byte 3(least 4 bits): TTTT (most significant T bits)
// byte 4: TTTTTTTT (middle significant T bits)
// byte 5: TTTTTTTT (least significant T bits)

void aht10_parse_temperature(volatile float* out, uint8_t data[]) {
    uint32_t t_part1 = ((uint32_t)(data[3] & 0x0F)) << 16;
    uint32_t t_part2 = (uint32_t)data[4] << 8;
    uint32_t t_part3 = (uint32_t)data[5];

    uint32_t t_raw = t_part1 | t_part2 | t_part3;

    *out = ((float)t_raw * 200.0f) / (float)(1 << 20) - 50.0f;
}

void aht10_parse_humidity(volatile float* out, uint8_t data[]) {
    uint32_t h_part1 = (uint32_t)data[1] << 12;
    uint32_t h_part2 = (uint32_t)data[2] << 4;
    uint32_t h_part3 = (uint32_t)data[3] >> 4;

    uint32_t h_raw = h_part1 | h_part2 | h_part3;

    *out = ((float)h_raw * 100.0f) / (float)(1 << 20);
}

bool aht10_read(uint8_t data[6]) {
    uint8_t cmd[3] = {0xAC, 0x33, 0x00};

    // send measurement trigger
    i2c_transmit(AHT10_ADDRESS, cmd, 3);
    delay_aht10();
    // read 6 bytes of data
    i2c_receive(AHT10_ADDRESS, data, 6);

    // check the busy bit (bit 7 of byte 0 (data[0])). If it's 0, data is ready.
    bool is_data_ready = (data[0] & (1 << 7)) == 0;

    return is_data_ready;
}
