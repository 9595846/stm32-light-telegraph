#include "stm32f10x.h"
#include "config.h"
#include "platform.h"
#include "check_i2c.h"

#define SCL GPIO_Pin_6
#define SDA GPIO_Pin_7
const char *check_i2c_pins(void) { return "PB6/PB7"; }

int check_scl(void) { return (GPIOB->IDR & SCL) != 0u; }
int check_sda(void) { return (GPIOB->IDR & SDA) != 0u; }
static void data(int high)
{
    if (high) { GPIOB->BSRR = SDA; } else { GPIOB->BRR = SDA; }
    delay_us(10u);
}
static void clock_low(void) { GPIOB->BRR = SCL; delay_us(10u); }
static int clock_high(void)
{
    uint32_t start = millis();
    GPIOB->BSRR = SCL;
    while (!check_scl()) {
        if ((uint32_t)(millis() - start) >= 2u) { return 0; }
    }
    delay_us(10u);
    return 1;
}
static int start(void)
{
    data(1);
    if (!clock_high() || !check_sda()) { return 0; }
    data(0);
    clock_low();
    return 1;
}
static int stop(void)
{
    int ok;
    clock_low(); data(0);
    ok = clock_high(); data(1);
    return ok && check_sda();
}
static int write_byte(uint8_t v)
{
    unsigned i;
    int ack;
    for (i = 0u; i < 8u; i++) {
        data((v & 0x80u) != 0u);
        if (!clock_high()) { return 0; }
        clock_low(); v <<= 1;
    }
    data(1);
    if (!clock_high()) { return 0; }
    ack = !check_sda();
    clock_low();
    return ack;
}
static int read_byte(uint8_t *value, int ack)
{
    unsigned i;
    uint8_t v = 0u;
    data(1);
    for (i = 0u; i < 8u; i++) {
        if (!clock_high()) { return 0; }
        v = (uint8_t)((v << 1) | (check_sda() ? 1u : 0u));
        clock_low();
    }
    data(!ack);
    if (!clock_high()) { return 0; }
    clock_low(); data(1);
    *value = v;
    return 1;
}
void check_i2c_init(void)
{
    GPIO_InitTypeDef gpio;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIOB->BSRR = SCL | SDA;
    gpio.GPIO_Pin = SCL | SDA;
    gpio.GPIO_Mode = GPIO_Mode_Out_OD;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &gpio);
    delay_us(50u);
}
int check_recover(void)
{
    unsigned i;
    data(1);
    if (!clock_high()) { return 0; }
    for (i = 0u; i < 9u && !check_sda(); i++) {
        clock_low();
        if (!clock_high()) { return 0; }
    }
    return stop();
}
int check_probe(uint8_t address)
{
    int ok = start();
    if (ok) { ok = write_byte((uint8_t)(address << 1)); }
    if (!stop()) { ok = 0; }
    return ok;
}
int check_write(uint8_t address, uint8_t reg, uint8_t value)
{
    int ok = start();
    if (ok) { ok = write_byte((uint8_t)(address << 1)); }
    if (ok) { ok = write_byte(reg); }
    if (ok) { ok = write_byte(value); }
    if (!stop()) { ok = 0; }
    return ok;
}
int check_read(uint8_t address, uint8_t reg, uint8_t *buf, unsigned count)
{
    unsigned i;
    int ok;
    if (!buf || !count) { return 0; }
    ok = start();
    if (ok) { ok = write_byte((uint8_t)(address << 1)); }
    if (ok) { ok = write_byte(reg); }
    if (ok) { ok = start(); }
    if (ok) { ok = write_byte((uint8_t)((address << 1) | 1u)); }
    for (i = 0u; ok && i < count; i++) { ok = read_byte(&buf[i], i + 1u < count); }
    if (!stop()) { ok = 0; }
    return ok;
}
