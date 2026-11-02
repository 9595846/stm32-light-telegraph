#include "stm32f10x.h"
#include "config.h"
#include "platform.h"

/* This CMSIS revision does not declare DWT; Cortex-M3 CYCCNT registers. */
#define CYCLE_CTRL (*(volatile uint32_t *)0xE0001000u)
#define CYCLE_COUNT (*(volatile uint32_t *)0xE0001004u)

static volatile uint32_t ticks;
static volatile uint8_t rx[128];
static volatile unsigned head, tail, rx_error;
static volatile uint8_t tx[2048];
static volatile unsigned tx_head,tx_tail,tx_dropped;

void SysTick_Handler(void) { ticks++; }
uint32_t millis(void) { return ticks; }
void delay_ms(uint32_t ms)
{
    uint32_t start = millis();
    while ((uint32_t)(millis() - start) < ms) { __NOP(); }
}
void delay_us(uint32_t us)
{
    uint32_t start = CYCLE_COUNT;
    uint32_t cycles = us * (CPU_HZ / 1000000u);
    while ((uint32_t)(CYCLE_COUNT - start) < cycles) { __NOP(); }
}
void USART1_IRQHandler(void)
{
    uint32_t status = USART1->SR;
    if (status & (USART_SR_RXNE | USART_SR_ORE | USART_SR_FE | USART_SR_NE)) {
        uint8_t value = (uint8_t)USART1->DR;
        unsigned next = (head + 1u) & 127u;
        if (status & (USART_SR_ORE | USART_SR_FE | USART_SR_NE)) { rx_error = 1u; }
        else if (next == tail) { rx_error = 1u; }
        else { rx[head] = value; head = next; }
    }
    if ((status & USART_SR_TXE) && (USART1->CR1 & USART_CR1_TXEIE)) {
        if(tx_tail==tx_head){USART1->CR1 &= ~USART_CR1_TXEIE;}
        else{USART1->DR=tx[tx_tail];tx_tail=(tx_tail+1u)&2047u;}
    }
}
void platform_init(void)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef uart;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    CYCLE_COUNT = 0u;
    CYCLE_CTRL |= 1u;
    (void)SysTick_Config(CPU_HZ / 1000u);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &gpio);
    USART_StructInit(&uart);
    uart.USART_BaudRate = 115200u;
    USART_Init(USART1, &uart);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    NVIC_EnableIRQ(USART1_IRQn);
    USART_Cmd(USART1, ENABLE);
}
void console_write(const char *s)
{
    unsigned n=0u,i,free_count;uint32_t saved;
    while(s[n]){n++;}
    saved=__get_PRIMASK();__disable_irq();
    free_count=(tx_tail+2047u-tx_head)&2047u;
    if(n>free_count){tx_dropped++;__set_PRIMASK(saved);return;}
    for(i=0u;i<n;i++){tx[tx_head]=(uint8_t)s[i];tx_head=(tx_head+1u)&2047u;}
    USART1->CR1|=USART_CR1_TXEIE;__set_PRIMASK(saved);
}
unsigned console_dropped(void){return tx_dropped;}
int console_read(uint8_t *value)
{
    if (tail == head) { return 0; }
    *value = rx[tail];
    tail = (tail + 1u) & 127u;
    return 1;
}
int console_error(void)
{
    unsigned error;
    uint32_t saved = __get_PRIMASK();
    __disable_irq();
    error = rx_error;
    rx_error = 0u;
    if (error) { tail = head; }
    __set_PRIMASK(saved);
    return error != 0u;
}
