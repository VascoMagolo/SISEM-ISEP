#include "eeprom.h"
#include "i2c.h"
#include <string.h>

#define EEPROM_ADDRESS   0xA0
#define EEPROM_PAGE_SIZE 32

// Spin-wait for the AT24C32E write cycle (10ms max per datasheet)
static void delay_write(void) {
    volatile uint32_t count = 0x40000;
    while (--count) {
        // waits
    }
}

void eeprom_write(uint16_t addr, const uint8_t *data, uint16_t len) {
    uint8_t buf[2 + EEPROM_PAGE_SIZE];
    while (len > 0) {
        uint16_t page_space = EEPROM_PAGE_SIZE - (addr % EEPROM_PAGE_SIZE);
        uint16_t chunk = (len < page_space) ? len : page_space;

        buf[0] = (uint8_t)(addr >> 8);
        buf[1] = (uint8_t)(addr & 0xFF);
        memcpy(buf + 2, data, chunk);
        
        i2c_transmit(EEPROM_ADDRESS, buf, (uint32_t)(2 + chunk));
        delay_write();
        
        addr += chunk;
        data += chunk;
        len  -= chunk;
    }
}

bool eeprom_read(uint16_t addr, uint8_t *data, uint16_t len) {
    uint8_t addr_buf[2] = { (uint8_t)(addr >> 8), (uint8_t)(addr & 0xFF) };
    i2c_transmit(EEPROM_ADDRESS, addr_buf, 2);
    i2c_receive(EEPROM_ADDRESS, data, (uint32_t)len);
    return true;
}
