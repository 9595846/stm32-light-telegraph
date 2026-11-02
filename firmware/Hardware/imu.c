#include "imu.h"
#include "platform.h"
#include "check_i2c.h"
#include <string.h>
static int setting(Imu *s,uint8_t reg,uint8_t value,uint8_t mask)
{
    uint8_t v=0u;
    return check_write(s->address,reg,value)&&check_read(s->address,reg,&v,1u)&&((v&mask)==(value&mask));
}
int imu_init(Imu *s)
{
    unsigned a,i;
    memset(s,0,sizeof(*s));check_i2c_init();
    if((!check_scl()||!check_sda())&&!check_recover()){return 0;}
    for(a=0x68u;a<=0x69u;a++){
        uint8_t id=0u;int stable=1;
        if(!check_read((uint8_t)a,0x75u,&id,1u)||(id!=0x68u&&id!=0x70u)){continue;}
        for(i=0u;i<2u;i++){uint8_t v=0u;if(!check_read((uint8_t)a,0x75u,&v,1u)||v!=id){stable=0;}}
        if(stable){s->address=(uint8_t)a;s->id=id;break;}
    }
    if(!s->address||!check_write(s->address,0x6Bu,0x80u)){return 0;}
    delay_ms(150u);
    {uint8_t id=0u;if(!check_read(s->address,0x75u,&id,1u)||id!=s->id){return 0;}}
    if(!setting(s,0x6Bu,1u,0x7Fu)){return 0;}delay_ms(50u);
    if(!setting(s,0x6Cu,0u,0xFFu)||!setting(s,0x1Au,3u,0x3Fu)||!setting(s,0x19u,9u,0xFFu)||
       !setting(s,0x1Bu,0u,s->id==0x70u?0xFBu:0xF8u)||!setting(s,0x1Cu,0u,0xF8u)||
       (s->id==0x70u&&!setting(s,0x1Du,3u,0x0Fu))||!setting(s,0x38u,1u,0xFFu)){return 0;}
    delay_ms(100u);s->online=1u;return 1;
}
static int16_t word(uint8_t hi,uint8_t lo)
{int32_t v=(int32_t)((uint16_t)hi*256u+lo);if(v>=32768L){v-=65536L;}return (int16_t)v;}
int imu_sample(Imu *s,uint32_t now)
{
    uint8_t ready=0u,b[14];unsigned i;
    if(!s->online){return 0;}
    if(!check_read(s->address,0x3Au,&ready,1u)){s->errors++;return 0;}
    if(!(ready&1u)){return 0;}
    if(!check_read(s->address,0x3Bu,b,14u)){s->errors++;return 0;}
    for(i=0u;i<3u;i++){s->a[i]=word(b[2*i],b[2*i+1]);s->g[i]=word(b[8+2*i],b[9+2*i]);}
    s->last_good=now;return 1;
}
