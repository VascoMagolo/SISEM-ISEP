#include "aht10.h"
#include "DAVE.h"

// 1byte shifted i2c address for AHT10 (0x38 << 1)
#define AHT10_ADDRESS 0x70

volatile uint8_t i2c_tx_completion = 0;
volatile uint8_t i2c_rx_completion = 0;

void endTxCallback() {
	i2c_tx_completion = 1;
}

void endRxCallback() {
	i2c_rx_completion = 1;
}

// delay for sensor conversion (80ms)
// TODO: get rid of this ugly pos X)
static void delay_aht10() {
	volatile uint32_t count = 0x8FFFF;
	while (--count) {
		// waits
    }
}

void parse_temperature(uint8_t data[6], float* temperature) {
	uint32_t t_part1 = ((uint32_t) (data[3] & 0x0F)) << 16;
	uint32_t t_part2 = (uint32_t) data[4] << 8;
	uint32_t t_part3 = (uint32_t) data[5];

	uint32_t t_raw = t_part1 | t_part2 | t_part3;

	*temperature = ((float) t_raw * 200.0f) / (float) (1 << 20) - 50.0f;
}

void parse_humidity(uint8_t data[6], float* humidity) {
	uint32_t h_part1 = (uint32_t) data[1] << 12;
	uint32_t h_part2 = (uint32_t) data[2] << 4;
	uint32_t h_part3 = (uint32_t) data[3] >> 4;

	uint32_t h_raw = h_part1 | h_part2 | h_part3;

	*humidity = ((float) h_raw * 100.0f) / (float) (1 << 20);
}

bool aht10_read(float *temperature, float *humidity) {
	uint8_t cmd[3] = { 0xAC, 0x33, 0x00 };
	uint8_t data[6] = { 0 };

	// send measurement trigger
	i2c_tx_completion = 0;
	I2C_MASTER_Transmit(&I2C_MASTER_0, true, AHT10_ADDRESS, cmd, 3, true);
	while (i2c_tx_completion == 0) {
		// wait for i2c bus
	}

	delay_aht10();

	// read 6 bytes of data
	i2c_rx_completion = 0;
	I2C_MASTER_Receive(&I2C_MASTER_0, true, AHT10_ADDRESS, data, 6, true, true);
	while (i2c_rx_completion == 0) {
		// wait for i2c bus
	}

	// check the busy bit (bit 7 of byte 0 (data[0])). If it's 0, data is ready.
	bool is_data_ready = (data[0] & (1 << 7)) == 0;

	/* byte 1: HHHHHHHH (most significant H bits)
	 * byte 2: HHHHHHHH (middle significant H bits)
	 * byte 3(top 4 bits): HHHH (least significant H bits)
	 * byte 3(least 4 bits): TTTT (most significant T bits)
	 * byte 4: TTTTTTTT (middle significant T bits)
	 * byte 5: TTTTTTTT (least significant T bits)
	 */
	if (is_data_ready) {
		parse_humidity(data, humidity);
		parse_temperature(data, temperature);

		return true;
	}

	return false; // busy or failed
}
