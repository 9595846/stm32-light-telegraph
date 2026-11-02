#include "light_link.h"
#include <string.h>

static const char *const codes[36] = {
    ".-","-...","-.-.","-..",".","..-.","--.","....","..",".---",
    "-.-",".-..","--","-.","---",".--.","--.-",".-.","...","-",
    "..-","...-",".--","-..-","-.--","--..",
    "-----",".----","..---","...--","....-",".....","-....","--...","---..","----."
};
/* Off preamble, then ... --- ... . Final silence lets independent RX finish. */
static const unsigned char sos_units[] = {
    7,1,1,1,1,1,3,3,1,3,1,3,3,1,1,1,1,1,7
};

static void rx_reset(LightLink *s, uint32_t now, unsigned clear)
{
    s->level=0u; s->candidate=0u; s->armed=0u;
    s->symbol_count=0u; s->dropping=0u; s->word_done=0u;
    s->candidate_at=now; s->edge_at=now;
    if(clear){s->received_count=0u; s->received[0]=0; s->errors=0u;}
}
void link_init(LightLink *s, uint32_t now)
{
    memset(s,0,sizeof(*s)); s->unit=300u; s->last_sample=now;
    rx_reset(s,now,1u);
}
void link_stop(LightLink *s, uint32_t now)
{
    if(s->mode!=LINK_IDLE && s->mode!=LINK_LAMP){s->calibrated=0u;}
    s->tx=0u; s->lamp=0u; s->mode=LINK_IDLE; s->error=LINK_STOPPED;
    rx_reset(s,now,0u);
}
void link_lamp(LightLink *s, unsigned on, uint32_t now)
{
    link_stop(s,now); s->mode=on?LINK_LAMP:LINK_IDLE;
    s->lamp=on?1u:0u; s->error=LINK_OK;
}
static void collect_start(LightLink *s, uint32_t now)
{
    s->since=now; s->sum=0u; s->count=0u;
    s->minimum=4095u; s->maximum=0u;
}
void link_calibrate(LightLink *s, uint32_t now)
{
    link_stop(s,now); s->calibrated=0u; s->error=LINK_OK;
    s->mode=LINK_DARK_SETTLE; s->since=now;
    rx_reset(s,now,1u);
}
int link_unit(LightLink *s, unsigned unit, uint32_t now)
{
    if(s->mode!=LINK_IDLE || s->tx || unit<300u || unit>1000u){return 0;}
    s->unit=unit; rx_reset(s,now,1u); return 1;
}
int link_send_sos(LightLink *s, uint32_t now)
{
    if(!s->calibrated || s->mode!=LINK_IDLE || s->tx ||
       !s->sample_seen || (uint32_t)(now-s->last_sample)>100u){return 0;}
    rx_reset(s,now,1u); s->tx=1u; s->segment=0u;
    s->lamp=0u; s->tx_at=now; s->error=LINK_OK; return 1;
}
static void fail(LightLink *s, unsigned error, uint32_t now)
{
    link_stop(s,now); s->calibrated=0u; s->error=error;
}
static void append(LightLink *s, char c)
{
    if(s->received_count>=sizeof(s->received)-1u){
        memmove(s->received,s->received+1,sizeof(s->received)-2u);
        s->received_count=sizeof(s->received)-2u;
    }
    s->received[s->received_count++]=c;
    s->received[s->received_count]=0;
}
static void finish_letter(LightLink *s)
{
    unsigned i;
    if(s->dropping){append(s,'?'); s->errors++;}
    else if(s->symbol_count){
        s->symbols[s->symbol_count]=0;
        for(i=0u;i<36u;i++){if(!strcmp(s->symbols,codes[i])){break;}}
        append(s,i<26u?(char)('A'+i):i<36u?(char)('0'+i-26u):'?');
        if(i==36u){s->errors++;}
    }
    s->symbol_count=0u; s->dropping=0u;
}
static void rx_sample(LightLink *s, uint32_t now, uint16_t raw)
{
    unsigned observed=s->level;
    uint32_t elapsed;
    if(s->bright>s->dark){
        if(raw>=s->high){observed=1u;}else if(raw<=s->low){observed=0u;}
    }else{
        if(raw<=s->low){observed=1u;}else if(raw>=s->high){observed=0u;}
    }
    if(observed!=s->candidate){s->candidate=observed;s->candidate_at=now;}
    if(s->candidate!=s->level && (uint32_t)(now-s->candidate_at)>=20u){
        uint32_t edge=s->candidate_at;
        elapsed=(uint32_t)(edge-s->edge_at);
        if(s->level && s->armed){
            if(elapsed>=s->unit/2u && elapsed<=s->unit*18u/10u){
                if(s->symbol_count<5u){s->symbols[s->symbol_count++]='.';}else{s->dropping=1u;}
            }else if(elapsed>=s->unit*2u && elapsed<=s->unit*45u/10u){
                if(s->symbol_count<5u){s->symbols[s->symbol_count++]='-';}else{s->dropping=1u;}
            }else{s->dropping=1u;}
            s->word_done=0u;
        }
        s->level=s->candidate; s->edge_at=edge;
    }
    elapsed=(uint32_t)(now-s->edge_at);
    if(!s->level){
        /* Require a real dark interval after reset; a stuck bright input cannot arm. */
        if(!s->candidate && elapsed>=s->unit*3u){s->armed=1u;}
        if(s->armed && elapsed>=s->unit*5u/2u &&
           (s->symbol_count || s->dropping)){finish_letter(s);}
        if(s->armed && elapsed>=s->unit*6u && !s->word_done && s->received_count){
            append(s,' ');s->word_done=1u;
        }
    }else if(s->armed && elapsed>s->unit*5u){s->dropping=1u;}
}
static void calibration_sample(LightLink *s, uint16_t raw)
{
    s->sum+=raw; s->count++;
    if(raw<s->minimum){s->minimum=raw;}
    if(raw>s->maximum){s->maximum=raw;}
}
void link_task(LightLink *s, uint32_t now, unsigned has_sample, uint16_t raw)
{
    if(has_sample){s->raw=raw;s->sample_seen=1u;s->last_sample=now;}
    if((uint32_t)(now-s->last_sample)>100u){
        if(s->error!=LINK_NO_ADC){fail(s,LINK_NO_ADC,now);}return;
    }
    if(s->mode==LINK_DARK_SETTLE && (uint32_t)(now-s->since)>=600u){
        s->mode=LINK_DARK_SAMPLE;collect_start(s,now);
    }else if(s->mode==LINK_BRIGHT_SETTLE && (uint32_t)(now-s->since)>=600u){
        s->mode=LINK_BRIGHT_SAMPLE;collect_start(s,now);
    }
    if(s->mode==LINK_DARK_SAMPLE || s->mode==LINK_BRIGHT_SAMPLE){
        if(has_sample){calibration_sample(s,raw);}
        if((uint32_t)(now-s->since)>=1000u){
            unsigned span,noise,mid,sep;
            if(s->count<50u){fail(s,LINK_NO_ADC,now);return;}
            span=s->maximum-s->minimum;
            if(s->mode==LINK_DARK_SAMPLE){
                s->dark=(uint16_t)(s->sum/s->count);s->dark_noise=span;
                s->mode=LINK_BRIGHT_SETTLE;s->lamp=1u;s->since=now;
            }else{
                s->bright=(uint16_t)(s->sum/s->count);
                sep=s->bright>s->dark?s->bright-s->dark:s->dark-s->bright;
                noise=span>s->dark_noise?span:s->dark_noise;
                /* Conservative startup criterion, not a claimed sensor specification. */
                if(sep<16u || sep<=noise*4u){fail(s,LINK_LOW_CONTRAST,now);return;}
                mid=((unsigned)s->bright+s->dark)/2u;
                s->low=(uint16_t)(mid-sep/6u);s->high=(uint16_t)(mid+sep/6u);
                s->mode=LINK_FINAL_SETTLE;s->lamp=0u;s->since=now;
            }
        }
    }
    if(s->mode==LINK_FINAL_SETTLE && (uint32_t)(now-s->since)>=600u){
        s->mode=LINK_IDLE;s->calibrated=1u;s->error=LINK_OK;rx_reset(s,now,1u);
    }
    if(s->mode!=LINK_IDLE){return;}
    if(s->calibrated && has_sample){rx_sample(s,now,raw);}
    if(s->tx){
        uint32_t duration=(uint32_t)sos_units[s->segment]*s->unit;
        uint32_t elapsed=(uint32_t)(now-s->tx_at);
        if(elapsed>=duration){
            if(elapsed-duration>s->unit/3u){
                s->tx=0u;s->lamp=0u;s->error=LINK_TIMING;
                rx_reset(s,now,0u);return;
            }
            s->segment++;s->tx_at=now;
            if(s->segment>=sizeof(sos_units)/sizeof(sos_units[0])){s->tx=0u;s->lamp=0u;}
            else{s->lamp=s->segment&1u;}
        }
    }
}
const char *link_error(unsigned error)
{
    switch(error){
    case LINK_NO_ADC:return "NO ADC";
    case LINK_LOW_CONTRAST:return "LOW CONTRAST";
    case LINK_TIMING:return "TIMING";
    case LINK_STOPPED:return "STOPPED";
    default:return "OK";
    }
}
