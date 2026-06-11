#include "i2c.h"
#include "DAVE.h"

static volatile uint8_t tx_done = 0;
static volatile uint8_t rx_done = 0;

void endTxCallback(void) {
    tx_done = 1;
}

void endRxCallback(void) {
    rx_done = 1;
}

void i2c_transmit(uint16_t addr, const uint8_t *data, uint32_t len) {
    tx_done = 0;
    I2C_MASTER_Transmit(&I2C_MASTER, true, addr, (uint8_t *)data, len, true);
    while (!tx_done) {}
}

void i2c_receive(uint16_t addr, uint8_t *data, uint32_t len) {
    rx_done = 0;
    I2C_MASTER_Receive(&I2C_MASTER, true, addr, data, len, true, true);
    while (!rx_done) {}
}
