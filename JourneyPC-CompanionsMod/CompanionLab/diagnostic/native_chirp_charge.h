#ifndef JC_NATIVE_CHIRP_CHARGE_H
#define JC_NATIVE_CHIRP_CHARGE_H
#include "native_actor_live.h"
#if defined(__x86_64__) && !defined(_WIN32)
#define JC_CHARGE_ABI __attribute__((ms_abi))
#else
#define JC_CHARGE_ABI
#endif
struct jc_chirp_charge {float remaining[3];uint64_t generation;unsigned hits[3];};
static inline int jc_chirp_charge_hit(uintptr_t image,const unsigned char pool[0x718],
        const unsigned owners[16],unsigned recipient,const float position[4],float dt) {
    uint32_t count;memcpy(&count,pool+0x710,4);
    if(count>16 || recipient>2 || !isfinite(dt) || dt<=0 || dt>1)return 0;
    for(unsigned i=0;i<4;++i)if(!isfinite(position[i]))return 0;
    _Alignas(16) unsigned char selected[0x718]={0};
    memcpy(selected+0x710,&count,4);
    for(unsigned i=0;i<count;++i) {
        const unsigned char *s=pool+i*0x70;
        if(!owners[i] || owners[i]>3 || owners[i]==recipient+1 ||
            (recipient!=2 && owners[i]!=3) || s[0x68]!=1)continue;
        int valid=1;
        for(unsigned at=0x20;at<=0x44;at+=4)if(!isfinite(jc_receiver_float(s+at)))valid=0;
        float age=jc_receiver_float(s+0x54);
        if(!valid || !isfinite(age) || age<0 || jc_receiver_float(s+0x40)<0 ||
            jc_receiver_float(s+0x44)<=0)continue;
        memcpy(selected+i*0x70+0x20,s+0x20,0x38);selected[i*0x70+0x68]=1;
    }
    unsigned char result[0x38]={0};
    return ((unsigned char (JC_CHARGE_ABI *)(uintptr_t,const float *,float,unsigned char,unsigned char,void *))
        (image+0x140790))((uintptr_t)selected,position,dt,0,0,result)!=0;
}
static inline int jc_chirp_charge_tick(struct jc_chirp_charge *charge,unsigned recipient,
        uint64_t generation,int active,int hit,float dt) {
    if(recipient>2 || !isfinite(dt) || dt<0 || dt>1)return 0;
    if(recipient==2 && charge->generation!=generation) {
        charge->generation=generation;charge->remaining[2]=0;
    }
    if(!active){charge->remaining[recipient]=0;return 0;}
    if(hit){charge->remaining[recipient]=fmaxf(charge->remaining[recipient],1.25f);++charge->hits[recipient];}
    charge->remaining[recipient]=fmaxf(0,charge->remaining[recipient]-dt);
    return charge->remaining[recipient]>0;
}
#endif
