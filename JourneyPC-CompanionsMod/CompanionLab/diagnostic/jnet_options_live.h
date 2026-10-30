#ifndef JNET_OPTIONS_LIVE_H
#define JNET_OPTIONS_LIVE_H
#include <math.h>
#ifdef _WIN32
#define JNET_ENV_ABI
#else
#define JNET_ENV_ABI __attribute__((ms_abi))
#endif
#ifdef __APPLE__
#define JNET_ENV_SYMBOL(name) "_" #name
#else
#define JNET_ENV_SYMBOL(name) #name
#endif
static uintptr_t (JNET_ENV_ABI *jnet_environment_copy_original)(uintptr_t,uintptr_t) __attribute__((used));
__attribute__((naked,used)) static uintptr_t JNET_ENV_ABI jnet_environment_copy(uintptr_t destination,uintptr_t source) {
    __asm__("subq $40,%rsp\n"
        "call *" JNET_ENV_SYMBOL(jnet_environment_copy_original) "(%rip)\n"
        "addq $40,%rsp\n"
        "pushfq\n"
        "pushq %r10\n"
        "movq " JNET_ENV_SYMBOL(fault_image) "(%rip),%r10\n"
        "addq $0x376f520,%r10\n"
        "cmpq %rcx,%r10\n"
        "jne 1f\n"
        "cmpl $0," JNET_ENV_SYMBOL(jnet_fog) "(%rip)\n"
        "jne 1f\n"
        "movl $0,0x2c(%rcx)\n"
        "movl $0,0x3c(%rcx)\n"
        "1: popq %r10\n"
        "popfq\n"
        "ret\n");
}
#undef JNET_ENV_SYMBOL
#undef JNET_ENV_ABI
static uintptr_t jnet_environment_endpoint(uintptr_t environment,float z) {
    uintptr_t type=0,south=0,north=0;unsigned count=0;
    float lower=INFINITY,upper=-INFINITY;
    if(!isfinite(z) || !own_read(NULL,environment,&type,8) || type!=fault_image+0x5d7b60 ||
       !own_read(NULL,environment+0x3c70,&count,4) || !count || count>32)return 0;
    for(unsigned i=0;i<count;++i){
        uintptr_t node=environment+0x70+(uintptr_t)i*0x1e0;
        unsigned char enabled=0;unsigned kind=0;float bounds[2];
        if(!own_read(NULL,node,&type,8) || type!=fault_image+0x5d7bb0 ||
           !own_read(NULL,node+0x140,&kind,4) || kind!=0 ||
           !own_read(NULL,node+0x144,&enabled,1))return 0;
        if(!enabled)continue;
        if(!own_read(NULL,node+0x148,bounds,sizeof(bounds)) || !isfinite(bounds[0]) ||
           !isfinite(bounds[1]) || bounds[1]<=0)return 0;
        float lo=bounds[0]-bounds[1],hi=bounds[0]+bounds[1];
        if(!isfinite(lo) || !isfinite(hi))return 0;
        if(lo<lower){lower=lo;south=node;}
        if(hi>upper){upper=hi;north=node;}
    }
    return z<=lower?south:z>=upper?north:0;
}
static void (*jnet_environment_update_original)(uintptr_t,uintptr_t,uintptr_t,float,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
static void jnet_environment_update(uintptr_t environment,uintptr_t resources,uintptr_t lua,float dt,
        uintptr_t position,uintptr_t camera,uintptr_t terrain,uintptr_t extra) {
    uintptr_t endpoint=0;float z=0,bounds[2],radius=0;unsigned char reload[2]={0},dirty=0;
    if(!jnet_blackness && position && own_read(NULL,position+8,&z,4) && isfinite(z) &&
       own_read(NULL,environment+8,reload,2) && !reload[0] && !reload[1] &&
       own_read(NULL,environment+0x60,&dirty,1) && !dirty){
        endpoint=jnet_environment_endpoint(environment,z);
        if(endpoint && own_read(NULL,endpoint+0x148,bounds,sizeof(bounds))){
            radius=fabsf(z-bounds[0])+fmaxf(1.f,fabsf(z)*.00001f);
            if(!isfinite(radius) || !jc_local_windows_write(endpoint+0x14c,&radius,4))endpoint=0;
        }else endpoint=0;
    }
    jnet_environment_update_original(environment,resources,lua,dt,position,camera,terrain,extra);
    if(endpoint){
        uintptr_t type=0;float current[2];
        int unchanged=own_read(NULL,environment,&type,8) && type==fault_image+0x5d7b60 &&
            own_read(NULL,endpoint,&type,8) && type==fault_image+0x5d7bb0 &&
            own_read(NULL,endpoint+0x148,current,sizeof(current)) &&
            !memcmp(current,bounds,4) && !memcmp(current+1,&radius,4);
        if(!unchanged || !jc_local_windows_write(endpoint+0x14c,bounds+1,4)){
            jnet_blackness=1;jnet_options_failed=1;
            message("[JNET] Blackness override stopped: environment changed or radius restoration failed");
        }
    }
}
static void jnet_options_apply(void) {
    static unsigned windwalls=1,sliding=1,freezing=1;
    unsigned wanted_wind=jnet_windwalls;
    unsigned wanted_slide=jnet_sliding,wanted_freeze=jnet_freezing;
    if(wanted_wind==windwalls && wanted_slide==sliding && wanted_freeze==freezing)return;
    static const unsigned char slide[]={0x0f,0xb6,0x82,0,1,0,0},slide_off[]={0xb8,0,0,0,0,0x90,0x90};
    static const unsigned char cold[]={0xf3,0x0f,0x11,0x43,0x14,0xf3,0x0f,0x11,0x4b,0x18};
    static const unsigned char nop10[]={0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
    static const unsigned char wind[]={0x40,0x55},wind_off[]={0xc3,0x90};
    unsigned char before[10];
    if(!own_read(NULL,fault_image+0x15b240,before,2) || (memcmp(before,wind,2) && memcmp(before,wind_off,2)) ||
       !own_read(NULL,fault_image+0xaa239,before,7) || (memcmp(before,slide,7) && memcmp(before,slide_off,7)) ||
       !own_read(NULL,fault_image+0x184920,before,10) || (memcmp(before,cold,10) && memcmp(before,nop10,10)))goto failed;
    const uintptr_t sites[]={fault_image+0xaa239,fault_image+0x184920,fault_image+0x15b240};
    const unsigned char *normal[]={slide,cold,wind},*disabled[]={slide_off,nop10,wind_off};
    const size_t sizes[]={7,10,2};
    const unsigned desired[]={wanted_slide,wanted_freeze,wanted_wind},previous[]={sliding,freezing,windwalls};
    for(unsigned i=0;i<3;++i)if(!jnet_mepal_site(sites[i],normal[i],disabled[i],sizes[i],!desired[i])){
        int restored=1;
        for(unsigned j=0;j<=i;++j)restored=jnet_mepal_site(sites[j],normal[j],disabled[j],sizes[j],!previous[j]) && restored;
        if(!restored)message("[JNET] Options rollback failed; restart Journey");
        goto failed;
    }
    windwalls=wanted_wind;sliding=wanted_slide;freezing=wanted_freeze;jnet_options_failed=0;return;
failed:
    jnet_windwalls=windwalls;jnet_sliding=sliding;jnet_freezing=freezing;jnet_options_failed=1;
    message("[JNET] Options patch refused: native bytes or protection unavailable");
}
#endif
