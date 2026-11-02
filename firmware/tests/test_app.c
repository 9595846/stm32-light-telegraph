#define TELEGRAPH_HOST_TEST
#include "../User/main.c"
#include <assert.h>
#include <stdlib.h>

static char log_text[32768];
uint32_t millis(void){return 0u;}
void console_write(const char *s){strcat(log_text,s);}
unsigned console_dropped(void){return 0u;}
void beep(uint32_t now){(void)now;}

static void line(const char *s)
{
    while(*s){rx_byte((uint8_t)*s++,0u);}
}
static void frame(unsigned index)
{
    char name[64];FILE *f;
    draw();assert(!display_clipped_text());
    sprintf(name,"Previews/page%u.bin",index);
    f=fopen(name,"wb");assert(f);assert(fwrite(display_framebuffer(),1u,1024u,f)==1024u);fclose(f);
}
int main(void)
{
    unsigned i;display_init();link_init(&link,0u);
    line("UNIT 500\r\n");assert(link.unit==500u);
    line("UNIT 299\nUNIT 1001\nUNIT 0500X\n");assert(link.unit==500u);
    line("send sos\n");assert(!link.tx);
    line("LAMP ON\n");assert(link.lamp && link.mode==LINK_LAMP);
    handle_keys(7u,0u);assert(!link.lamp && link.mode==LINK_IDLE);
    line("CAL\n");assert(link.mode==LINK_DARK_SETTLE);
    line("STOP\n");assert(!link.calibrated && !link.lamp);
    for(i=0u;i<70u;i++){rx_byte('X',0u);}
    line("LAMP ON\n");assert(!link.lamp); /* overflow tail must not execute */
    line("LAMP ON\n");assert(link.lamp);
    rx_byte(1u,0u);line("STOP\n");assert(link.lamp);
    line("STOP\n");assert(!link.lamp);
    rotate(100000,0u);assert(link.unit==1000u);
    rotate(-100000,0u);assert(link.unit==300u);
    link.calibrated=1u;link.sample_seen=1u;link.last_sample=0u;
    line("SEND SOS\n");assert(link.tx);
    line("UNIT 1000\n");assert(link.unit==300u);
    handle_keys(6u,0u);assert(!link.tx && !link.lamp);
    page=0u;frame(1u);
    link.error=LINK_OK;link.calibrated=0u;frame(2u);
    link.mode=LINK_DARK_SAMPLE;frame(3u);
    link.mode=LINK_IDLE;link.calibrated=1u;link.tx=1u;
    strcpy(link.received,"SOS ");link.received_count=4u;frame(4u);
    link.raw=4095u;link.dark=4095u;link.bright=4095u;link.low=4095u;link.high=4095u;
    page=1u;frame(5u);
    imu.id=0x70u;imu.a[0]=-32768;imu.a[1]=-32768;imu.a[2]=-32768;
    page=2u;frame(6u);
    page=0u;strcpy(link.received,"ABCDEFGHIJKLMNOPQRSTU");link.received_count=21u;
    link.unit=1000u;draw();assert(!display_clipped_text());
    report();status(0u);assert(strstr(log_text,"ERR") && strstr(log_text,"SERVO=OFF"));
    puts("PASS: UART parsing/overflow recovery, stop priority, knob limits, busy rejection, real OLED renderer bounds and six framebuffer exports.");
    return 0;
}
