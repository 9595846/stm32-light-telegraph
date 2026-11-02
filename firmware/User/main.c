#include "platform.h"
#include "controls.h"
#include "display.h"
#include "imu.h"
#include "light_link.h"
#include "ui_text.h"
#include <stdio.h>
#include <string.h>

static LightLink link;
static Imu imu;
static unsigned page,used,discard,last_mode=99u,last_error=99u,last_tx;
static unsigned reported_count,reported_errors;
static char input[64],reported[22];

static void status(uint32_t now)
{
    char b[220];
    sprintf(b,"STATUS MODE=%u CAL=%u ERROR=%s ADC=%u AGE=%lu RAW=%u DARK=%u BRIGHT=%u LOW=%u HIGH=%u UNIT=%u TX=%u LAMP=%u\r\n",
        link.mode,link.calibrated,link_error(link.error),link.sample_seen,
        (unsigned long)(now-link.last_sample),link.raw,link.dark,link.bright,
        link.low,link.high,link.unit,link.tx,link.lamp);
    console_write(b);
    sprintf(b,"RX=%s RX_ERR=%u OLED=%u MPU_ID=%02X MPU=%u MPU_AGE=%lu TX_DROP=%u SERVO=OFF\r\n",
        link.received,link.errors,display_online(),imu.id,
        (unsigned)(imu.online && (uint32_t)(now-imu.last_good)<300u),
        (unsigned long)(now-imu.last_good),console_dropped());console_write(b);
}
static void send_sos(uint32_t now)
{
    if(link_send_sos(&link,now)){console_write("TX SOS START; RX USES ADC ONLY\r\n");}
    else{console_write("ERR CALIBRATE FIRST; IDLE/FRESH ADC REQUIRED\r\n");}
}
static void command(char *s,uint32_t now)
{
    unsigned i,value=0u;
    for(i=0u;s[i];i++){if(s[i]>='a'&&s[i]<='z'){s[i]=(char)(s[i]-'a'+'A');}}
    if(!strcmp(s,"HELP")){
        console_write("CAL | SEND SOS | STOP | STATUS | UNIT 300..1000 | LAMP ON | LAMP OFF\r\nK1 CAL K2 STOP K3 SOS K4 PAGE; KNOB UNIT; SERVO ALWAYS OFF\r\n");
    }else if(!strcmp(s,"STATUS")){status(now);}
    else if(!strcmp(s,"CAL")){link_calibrate(&link,now);console_write("CAL START; KEEP LIGHT PATH OPEN\r\n");}
    else if(!strcmp(s,"STOP")){link_stop(&link,now);console_write("STOP; LAMP OFF\r\n");}
    else if(!strcmp(s,"SEND SOS")){send_sos(now);}
    else if(!strcmp(s,"LAMP ON")){link_lamp(&link,1u,now);console_write("LAMP ON; RX PAUSED\r\n");}
    else if(!strcmp(s,"LAMP OFF")){link_lamp(&link,0u,now);console_write("LAMP OFF; RX REARMING\r\n");}
    else if(!strncmp(s,"UNIT ",5u)){
        if(strlen(s)<8u||strlen(s)>9u){console_write("ERR UNIT FORMAT\r\n");return;}
        for(i=5u;s[i];i++){
            if(s[i]<'0'||s[i]>'9'){console_write("ERR UNIT FORMAT\r\n");return;}
            value=value*10u+(unsigned)(s[i]-'0');
        }
        console_write(link_unit(&link,value,now)?"OK UNIT\r\n":"ERR UNIT RANGE OR BUSY\r\n");
    }else{console_write("ERR UNKNOWN; HELP\r\n");}
}
static void rx_byte(uint8_t c,uint32_t now)
{
    if(c=='\r'||c=='\n'){
        if(discard){discard=0u;used=0u;return;}
        if(used){input[used]=0;command(input,now);}used=0u;return;
    }
    if(discard){return;}
    if(c==8u||c==127u){if(used){used--;}return;}
    if(c<32u||c>126u||used>=sizeof(input)-1u){
        used=0u;discard=1u;console_write("ERR LINE; ENTER THEN RESEND\r\n");return;
    }
    input[used++]=(char)c;
}
static void handle_keys(unsigned keys,uint32_t now)
{
    /* Stop wins when several buttons are pressed together. */
    if(keys&2u){link_stop(&link,now);console_write("KEY STOP\r\n");return;}
    if(keys&8u){page=(page+1u)%3u;}
    if(keys&1u){link_calibrate(&link,now);console_write("KEY CAL; KEEP PATH OPEN\r\n");}
    else if(keys&4u){send_sos(now);}
}
static void rotate(int delta,uint32_t now)
{
    int unit;
    if(!delta || link.tx || link.mode!=LINK_IDLE){return;}
    if(delta>20){delta=20;}if(delta< -20){delta= -20;}
    unit=(int)link.unit+delta*50;
    if(unit<300){unit=300;}if(unit>1000){unit=1000;}
    (void)link_unit(&link,(unsigned)unit,now);
}
static void report(void)
{
    if(last_mode!=link.mode || last_error!=link.error){
        char b[90];last_mode=link.mode;last_error=link.error;
        sprintf(b,"STATE MODE=%u CAL=%u %s\r\n",link.mode,link.calibrated,link_error(link.error));console_write(b);
    }
    if(last_tx && !link.tx){console_write("TX ENDED; CHECK RX/STATUS, NOT A SUCCESS CLAIM\r\n");}
    last_tx=link.tx;
    if(reported_count!=link.received_count || reported_errors!=link.errors || strcmp(reported,link.received)){
        char b[64];reported_count=link.received_count;reported_errors=link.errors;
        strcpy(reported,link.received);
        sprintf(b,"RX=%s ERR=%u\r\n",link.received,link.errors);console_write(b);
        if(link.received_count){beep(millis());}
    }
}
static void draw(void)
{
    char b[64];display_clear();
    if(page==0u){
        display_text(0,0,UI_TITLE);
        if(link.mode!=LINK_IDLE){display_text(0,16,link.mode==LINK_LAMP?UI_LAMP:UI_CAL);}
        else if(link.error!=LINK_OK){display_text(0,16,link_error(link.error));}
        else{display_text(0,16,link.tx?UI_TX:link.calibrated?UI_READY:UI_NEED_CAL);}
        sprintf(b,"U:%u RX:%s",link.unit,link.received_count?link.received:"-");
        b[21]=0;display_text(0,32,b);display_text(0,48,UI_BUTTONS);
    }else if(page==1u){
        display_text(0,0,UI_LIGHT);sprintf(b,"RAW:%u CAL:%u",link.raw,link.calibrated);display_text(0,16,b);
        sprintf(b,"D:%u B:%u",link.dark,link.bright);display_text(0,32,b);
        sprintf(b,"LO:%u HI:%u",link.low,link.high);display_text(0,48,b);
    }else{
        display_text(0,0,UI_IMU);
        sprintf(b,"ID:%02X A:%d",imu.id,imu.a[0]);display_text(0,16,b);
        sprintf(b,"Y:%d Z:%d",imu.a[1],imu.a[2]);display_text(0,32,b);
        display_text(0,48,UI_SERVO_OFF);
    }
}
#ifndef TELEGRAPH_HOST_TEST
int main(void)
{
    uint32_t sample_at,imu_at,draw_at;uint16_t raw=0u;
    platform_init();controls_init();servo_set(0u,1500u);display_init();
    (void)light_init();(void)imu_init(&imu);link_init(&link,millis());
    sample_at=imu_at=draw_at=millis();
    console_write("LIGHT TELEGRAPH STAGE2 V1.0 READY; 115200 8N1\r\nLED1 PA0 -> LIGHT AO PA1; SERVO OFF; SEND HELP\r\n");
    for(;;){
        uint32_t now=millis();unsigned n,has_sample=0u;uint8_t c;
        if(console_error()){used=0u;discard=1u;console_write("ERR RX OVERFLOW; ENTER THEN RESEND\r\n");}
        for(n=0u;n<16u&&console_read(&c);n++){rx_byte(c,millis());}
        now=millis();handle_keys(buttons_task(now),now);rotate(encoder_delta(),now);
        if((uint32_t)(now-sample_at)>=5u){sample_at=now;has_sample=(unsigned)light_read(&raw);}
        link_task(&link,now,has_sample,raw);lamp_set(link.lamp);
        led2_set(link.calibrated && !link.error && !link.errors);
        outputs_task(now);report();
        /* Avoid IMU bus latency during calibration and optical reception/transmission. */
        if(!link.tx && !link.calibrated && link.mode==LINK_IDLE && (uint32_t)(now-imu_at)>=100u){
            imu_at=now;(void)imu_sample(&imu,now);
        }
        now=millis();if((uint32_t)(now-draw_at)>=100u){draw_at=now;draw();}
        display_task(now);
    }
}
#endif
