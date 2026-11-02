#include "light_link.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t clock_ms;
static void step(LightLink *s,unsigned inverse,unsigned blocked,unsigned noise)
{
    unsigned lit=s->lamp && !blocked;
    uint16_t value=(uint16_t)(inverse?(lit?900u:3000u):(lit?3000u:900u));
    if(noise){value=(uint16_t)(value+((clock_ms/5u)%7u));}
    clock_ms+=5u;link_task(s,clock_ms,1u,value);
}
static void calibrate(LightLink *s,unsigned inverse)
{
    unsigned i;link_init(s,clock_ms);assert(!link_send_sos(s,clock_ms));
    link_calibrate(s,clock_ms);
    for(i=0u;i<800u;i++){step(s,inverse,0u,1u);}
    assert(s->mode==LINK_IDLE && s->calibrated && s->error==LINK_OK);
    assert(s->low<s->high);
}
static void optical_roundtrip(unsigned inverse,unsigned unit,unsigned blocked)
{
    LightLink s;unsigned i;
    calibrate(&s,inverse);assert(link_unit(&s,unit,clock_ms));
    assert(link_send_sos(&s,clock_ms));
    assert(!link_unit(&s,500u,clock_ms));
    for(i=0u;i<16000u && s.tx;i++){step(&s,inverse,blocked,1u);}
    assert(!s.tx && !s.lamp && s.error==LINK_OK);
    if(blocked){assert(s.received_count==0u);}
    else{assert(!strcmp(s.received,"SOS "));assert(s.errors==0u);}
}
static void external_pulse(LightLink *s,unsigned bright,unsigned duration)
{
    unsigned i;for(i=0u;i<duration/5u;i++){
        clock_ms+=5u;link_task(s,clock_ms,1u,(uint16_t)(bright?3000u:900u));
    }
}
int main(void)
{
    LightLink s;unsigned i;
    optical_roundtrip(0u,300u,0u);optical_roundtrip(1u,300u,0u);
    optical_roundtrip(0u,1000u,0u);optical_roundtrip(1u,1000u,0u);
    optical_roundtrip(0u,300u,1u); /* A transmitting lamp does not create RX text. */
    clock_ms=0xFFFFF000u;optical_roundtrip(0u,300u,0u); /* timer wrap */
    calibrate(&s,0u);assert(link_send_sos(&s,clock_ms));
    link_task(&s,clock_ms+101u,0u,0u);
    assert(!s.tx && !s.lamp && !s.calibrated && s.error==LINK_NO_ADC);
    assert(!link_send_sos(&s,clock_ms+101u));clock_ms+=101u;
    link_init(&s,clock_ms);link_calibrate(&s,clock_ms);
    for(i=0u;i<800u;i++){clock_ms+=5u;link_task(&s,clock_ms,1u,1800u);}
    assert(!s.calibrated && s.error==LINK_LOW_CONTRAST && !s.lamp);
    link_init(&s,clock_ms);link_calibrate(&s,clock_ms);
    for(i=0u;i<800u;i++){
        clock_ms+=5u;link_task(&s,clock_ms,1u,(uint16_t)(1800u+(i%2u)*500u+s.lamp*50u));
    }
    assert(!s.calibrated && s.error==LINK_LOW_CONTRAST);
    calibrate(&s,0u);link_lamp(&s,1u,clock_ms);
    external_pulse(&s,1u,4000u);assert(s.received_count==0u);
    link_stop(&s,clock_ms);assert(s.mode==LINK_IDLE && !s.lamp && !s.tx);
    calibrate(&s,0u);assert(link_send_sos(&s,clock_ms));
    clock_ms+=s.unit*7u+101u;link_task(&s,clock_ms,1u,900u);
    assert(!s.tx && !s.lamp && s.error==LINK_TIMING);
    calibrate(&s,0u);
    external_pulse(&s,0u,1200u);
    external_pulse(&s,1u,10u);external_pulse(&s,0u,2200u);
    assert(s.received_count==0u); /* 10ms optical glitch ignored. */
    external_pulse(&s,1u,300u);external_pulse(&s,0u,2100u);
    assert(!strcmp(s.received,"E ")); /* external light; TX never started */
    for(i=0u;i<30u;i++){
        external_pulse(&s,1u,300u);external_pulse(&s,0u,2100u);
    }
    assert(s.received_count==21u && s.received[21]==0); /* bounded rolling window */
    calibrate(&s,0u);external_pulse(&s,0u,1200u);
    external_pulse(&s,1u,1700u);external_pulse(&s,0u,2100u);
    assert(strchr(s.received,'?') && s.errors); /* overlong mark rejected */
    calibrate(&s,0u);link_calibrate(&s,clock_ms);link_stop(&s,clock_ms);
    assert(!s.calibrated && !s.lamp); /* interrupted calibration cannot be used */
    puts("PASS: independent optical SOS, inverted polarity, 300/1000ms, blocked path, wrap, ADC loss, contrast/noise rejection, stop, scheduling fault, glitch, external RX, bounded buffer, invalid pulse.");
    return 0;
}
