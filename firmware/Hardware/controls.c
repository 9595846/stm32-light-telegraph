#include "stm32f10x.h"
#include "controls.h"
#include "platform.h"
static uint32_t beep_start;
static unsigned beeping,adc_ready;
void controls_init(void)
{
    GPIO_InitTypeDef g;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_TIM1,ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);
    GPIO_SetBits(GPIOB,GPIO_Pin_14);
    GPIO_ResetBits(GPIOA,GPIO_Pin_0);GPIO_ResetBits(GPIOB,GPIO_Pin_0);
    g.GPIO_Speed=GPIO_Speed_2MHz;g.GPIO_Mode=GPIO_Mode_Out_PP;
    g.GPIO_Pin=GPIO_Pin_0;GPIO_Init(GPIOA,&g);
    g.GPIO_Pin=GPIO_Pin_0|GPIO_Pin_14;GPIO_Init(GPIOB,&g);
    g.GPIO_Mode=GPIO_Mode_IPU;g.GPIO_Pin=GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_12|GPIO_Pin_15;GPIO_Init(GPIOB,&g);
    g.GPIO_Pin=GPIO_Pin_6|GPIO_Pin_7;GPIO_Init(GPIOA,&g);
    TIM3->CR1=0u;TIM3->PSC=0u;TIM3->ARR=65535u;
    TIM3->CCMR1=0xA1A1u;TIM3->SMCR=3u;TIM3->CCER=0u;TIM3->CNT=0u;TIM3->CR1=1u;
    GPIO_ResetBits(GPIOA,GPIO_Pin_8);g.GPIO_Mode=GPIO_Mode_Out_PP;g.GPIO_Pin=GPIO_Pin_8;GPIO_Init(GPIOA,&g);
    TIM1->CR1=0u;TIM1->PSC=63u;TIM1->ARR=19999u;TIM1->CCR1=1500u;
    TIM1->CCMR1=0x68u;TIM1->CCER=0u;TIM1->BDTR=0u;TIM1->EGR=1u;TIM1->CR1=0x81u;
}
unsigned buttons_task(uint32_t now)
{
    static uint8_t sample[4],stable[4];static uint32_t changed[4];
    static const uint16_t pins[4]={GPIO_Pin_12,GPIO_Pin_15,GPIO_Pin_8,GPIO_Pin_9};
    unsigned i,events=0u;
    for(i=0;i<4;i++){
        uint8_t down=(GPIOB->IDR&pins[i])?0u:1u;
        if(down!=sample[i]){sample[i]=down;changed[i]=now;}
        if(down!=stable[i]&&(uint32_t)(now-changed[i])>=20u){stable[i]=down;if(down){events|=1u<<i;}}
    }
    return events;
}
int encoder_delta(void)
{
    static uint16_t previous;static int remaining;
    uint16_t count=(uint16_t)TIM3->CNT;int32_t delta=(int32_t)count-previous;int steps;
    if(delta>32767L){delta-=65536L;}if(delta< -32768L){delta+=65536L;}
    previous=count;remaining+=(int)delta;steps=remaining/4;remaining-=steps*4;return steps;
}
int light_init(void)
{
    GPIO_InitTypeDef g;uint32_t begin;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);RCC_ADCCLKConfig(RCC_PCLK2_Div6);
    g.GPIO_Pin=GPIO_Pin_1;g.GPIO_Mode=GPIO_Mode_AIN;g.GPIO_Speed=GPIO_Speed_2MHz;GPIO_Init(GPIOA,&g);
    ADC1->CR1=0u;ADC1->CR2=ADC_CR2_CONT;ADC1->SMPR2=7u<<3;
    ADC1->SQR1=0u;ADC1->SQR2=0u;ADC1->SQR3=1u;ADC1->CR2|=ADC_CR2_ADON;delay_us(10u);
    ADC1->CR2|=ADC_CR2_RSTCAL;begin=millis();
    while(ADC1->CR2&ADC_CR2_RSTCAL){if((uint32_t)(millis()-begin)>=20u){return 0;}}
    ADC1->CR2|=ADC_CR2_CAL;begin=millis();
    while(ADC1->CR2&ADC_CR2_CAL){if((uint32_t)(millis()-begin)>=20u){return 0;}}
    ADC1->CR2|=ADC_CR2_ADON;adc_ready=1u;return 1;
}
int light_read(uint16_t *value)
{if(!adc_ready||!(ADC1->SR&ADC_SR_EOC)){return 0;}*value=(uint16_t)ADC1->DR;return 1;}
void servo_set(unsigned enabled,unsigned pulse)
{
    GPIO_InitTypeDef g;g.GPIO_Pin=GPIO_Pin_8;g.GPIO_Speed=GPIO_Speed_2MHz;
    if(!enabled){TIM1->CCER=0u;TIM1->BDTR=0u;GPIO_ResetBits(GPIOA,GPIO_Pin_8);g.GPIO_Mode=GPIO_Mode_Out_PP;GPIO_Init(GPIOA,&g);return;}
    if(pulse<1400u){pulse=1400u;}if(pulse>1600u){pulse=1600u;}
    TIM1->CCR1=pulse;g.GPIO_Mode=GPIO_Mode_AF_PP;GPIO_Init(GPIOA,&g);TIM1->CCER=1u;TIM1->BDTR=0x8000u;
}
void beep(uint32_t now){beeping=1u;beep_start=now;GPIO_ResetBits(GPIOB,GPIO_Pin_14);}
void led2_set(unsigned on){GPIO_WriteBit(GPIOB,GPIO_Pin_0,on?Bit_SET:Bit_RESET);}
void lamp_set(unsigned on){GPIO_WriteBit(GPIOA,GPIO_Pin_0,on?Bit_SET:Bit_RESET);}
void outputs_task(uint32_t now)
{
    if(beeping&&(uint32_t)(now-beep_start)>=80u){beeping=0u;GPIO_SetBits(GPIOB,GPIO_Pin_14);}
}
