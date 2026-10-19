#include "native_connection_scan.h"
#include <assert.h>
#include <stdio.h>
static unsigned char memory[0x200000];
static const uint64_t base=0x100000,image=UINT64_C(0x140000000),game=0x100000,barn=0x101000,match=0x150000,shout=0x180000;
static unsigned reads,limit=10000,mutate;
static void put(uint64_t p,const void *v,size_t n) { assert(p>=base && p-base+n<=sizeof(memory)); memcpy(memory+(p-base),v,n); }
static void u64(uint64_t p,uint64_t v) { put(p,&v,8); }
static void u32(uint64_t p,uint32_t v) { put(p,&v,4); }
static void byte(uint64_t p,unsigned char v) { put(p,&v,1); }
static void scalar(uint64_t p,float v) { put(p,&v,4); }
static int reader(void *ctx,uint64_t p,void *out,size_t n) {
    (void)ctx;
    if (++reads>limit || p<base || n>sizeof(memory) || p-base>sizeof(memory)-n) return 0;
    if (mutate && p==barn+0xa2 && reads>20) byte(barn+0xa2,0);
    memcpy(out,memory+(p-base),n); return 1;
}
static void setup(void) {
    memset(memory,0,sizeof(memory)); reads=mutate=0; limit=10000;
    u64(game+0x1e0,barn); u64(game+0x180,match); u64(game+0x1b8,shout);
    put(game+0x1c02f0,"Level_Desert",sizeof("Level_Desert"));
    u64(barn,image+0x5da340); byte(barn+0xa2,1); u32(barn+0x14,2); u32(barn+0x64,2); byte(match+0x11db4,1);
    u32(shout+0x714,7); byte(shout+0x68,1); byte(shout+0x4c,1);
    byte(shout+0x70+0x68,1); byte(shout+0x70+0x4d,1);
    for (unsigned i=0;i<2;++i) {
        uint64_t d=barn+(i ? 0x146f0 : 0xb0),mesh=d+0x160,entries=0x170000+i*0x2000;
        uint64_t slot=0x174000+i*0x100,resource=0x175000+i*0x100,payload=0x176000+i*0x100;
        u64(d,image+0x5d0380); u64(mesh,image+0x67f3f8); u32(d+0x74,i ? 20 : 0);
        scalar(d+0x10,1+(float)i); scalar(d+0x1e00,.8f); scalar(d+0x2000,.3f);
        byte(d+0xa264,1); byte(d+0xa2f4,1);
        u64(mesh+0x578,entries); u64(mesh+0x580,3); u64(mesh+0x30,slot);
        u64(slot+0x10,resource); u64(resource+0x18,payload);
        u64(payload+0x40,0x177000+i*0x100); u64(payload+0x48,3);
        for (unsigned j=0;j<3;++j) {
            uint64_t row=entries+j*80,state=0x178000+i*0x1000+j*0x100;
            u32(row+20,j); scalar(row+4,.25f); scalar(row+8,-1); scalar(row+12,1); scalar(row+16,j<2 ? 1.f : 0.f);
            const char *name=j ? "Med" : "ReadyToMed";
            put(row+32,name,strlen(name)+1); u64(row+48,strlen(name)); u64(row+56,15);
            u64(0x177000+i*0x100+j*8,state); scalar(state+0x10,.3f); scalar(state+0x20,10); scalar(state+0x24,12);
        }
    }
}
int main(void) {
    struct jc_connection_snapshot s;
    setup(); s.code=jc_connection_read(reader,NULL,image,game,1,2,&s);
    assert(!s.code && s.connected==1 && s.remote_gate && s.level==2);
    assert(s.dude[0].symbol==0 && s.dude[1].symbol==20 && s.dude[0].active==2 && s.dude[1].total==3);
    assert(!s.dude[0].animation_code && !strcmp(s.dude[0].clips[0].name,"ReadyToMed"));
    assert(s.dude[0].clips[0].frame==.25f && s.dude[0].clips[0].range[0]==10 && s.dude[0].clips[0].state_code==0);
    assert(s.shout_counter==7 && s.shout_active==2 && s.shout_local==1 && s.shout_remote==1);
    assert(s.dude[1].dust && s.dude[1].chunk && s.dude[1].spine_code!=0 && s.cloth_code==1);
    unsigned complete=reads;
    static unsigned char preserved[sizeof(memory)]; memcpy(preserved,memory,sizeof(memory));
    for (unsigned n=0;n<complete;++n) {
        reads=0; limit=n;
        (void)jc_connection_read(reader,NULL,image,game,1,2,&s);
        assert(reads<=complete+16 && !memcmp(preserved,memory,sizeof(memory)));
    }
    setup(); byte(barn+0xa2,0); byte(barn+0xa3,1); byte(match+0x11db4,0);
    s.code=jc_connection_read(reader,NULL,image,game,0,2,&s);
    assert(!s.code && !s.remote_gate && !s.connected && s.gates[1] && !s.dude[1].code);
    setup(); mutate=1; assert(jc_connection_read(reader,NULL,image,game,0,2,&s)==3);
    setup(); u64(barn,0); assert(jc_connection_read(reader,NULL,image,game,0,2,&s)==2);
    setup(); u64(barn+0xb0+0x160+0x580,513); s.code=jc_connection_read(reader,NULL,image,game,1,2,&s);
    assert(!s.code && s.dude[0].animation_code==4);
    reads=0; s.code=jc_connection_read(reader,NULL,image,game,0,2,&s);
    assert(!s.code && s.dude[0].animation_code==-1 && !s.dude[0].active && reads<complete);
    printf("Connection scan base-only: %u reads versus %u deep; animation arrays are not read on base samples.\n",reads,complete);
    setup(); scalar(barn+0xb0+0x2000,NAN); s.code=jc_connection_read(reader,NULL,image,game,0,2,&s);
    assert(!s.code && s.dude[0].code==4);
    char text[32000];
    size_t length=jc_connection_format(text,sizeof(text),&s,100,10,11,5,reads,1);
    assert(length && !strstr(text,"nan") && !strstr(text,"nan(") && strstr(text,"\"frame_after\":11"));
    setup(); s.code=jc_connection_read(reader,NULL,image,game,1,2,&s);
    assert(s.cloth_code==1 && s.dude[0].spine_code!=0);
    s.dude[0].spine_code=0; for(unsigned i=0;i<16;++i)s.dude[0].spine[i]=(float)(i%5==0);
    s.dude[0].stored=s.dude[0].active=s.dude[1].stored=s.dude[1].active=JC_SCAN_CLIPS;
    for(unsigned i=0;i<2;++i) for(unsigned j=0;j<JC_SCAN_CLIPS;++j) {
        s.dude[i].clips[j]=s.dude[i].clips[0];
        memset(s.dude[i].clips[j].name,'A',127);s.dude[i].clips[j].name[127]=0;
    }
    length=jc_connection_format(text,sizeof(text),&s,UINT64_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX,1);
    assert(length>8000 && length<sizeof(text));
    for (size_t cap=1;cap<=length;++cap) { text[cap]='!'; assert(!jc_connection_format(text,cap,&s,UINT64_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX,1)); assert(text[cap]=='!'); }
    printf("Connection scan: native solo/connected/departure flags, symbols, clips and clocks, shout slots, partial reads, changed context, invalid ranges/nonfinite values; no game writes. Largest format %zu/32000; every truncation checked.\n",length);
    return 0;
}
