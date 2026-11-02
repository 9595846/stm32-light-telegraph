#ifndef CHECK_I2C_H
#define CHECK_I2C_H
#include <stdint.h>
void check_i2c_init(void);
const char *check_i2c_pins(void);
int check_scl(void);
int check_sda(void);
int check_recover(void);
int check_probe(uint8_t address);
int check_write(uint8_t address, uint8_t reg, uint8_t value);
int check_read(uint8_t address, uint8_t reg, uint8_t *data, unsigned count);
#endif
