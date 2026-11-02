#ifndef CHECK_PLATFORM_H
#define CHECK_PLATFORM_H
#include <stdint.h>
void platform_init(void);
uint32_t millis(void);
void delay_ms(uint32_t ms);
void delay_us(uint32_t us);
void console_write(const char *s);
int console_read(uint8_t *value);
int console_error(void);
unsigned console_dropped(void);
#endif
