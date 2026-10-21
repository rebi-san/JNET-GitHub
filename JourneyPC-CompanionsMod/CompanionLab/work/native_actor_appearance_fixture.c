
#define main cloth_header_main
#include "../diagnostic/test_native_companion_cloth.c"
#undef main
#include "native_cloth_frame.h"
static struct jc_cloth_lease cloth_actor_scarf;
static uint64_t cloth_actor_scarf_peer;
static uint64_t traveler_object=DUDE,traveler_game=GAME,traveler_barn=PLAYERS;
static long JCProbeFrames=1;
static unsigned hsv_calls,status_messages;
static int context_ready=1;
static void *GetModuleHandleW(void *p) {(void)p;return (void *)(uintptr_t)IMAGE;}
static int traveler_context(void) {return context_ready;}
static void message(const char *s) {assert(s && *s);++status_messages;}
static int own_read(void *ctx,uint64_t at,void *out,size_t bytes) {
    if (at==failed_read) return 0;
    if (at==IMAGE+0x217a70 && bytes==16) {
        const unsigned char entry[16]={64,83,72,131,236,96,15,41,116,36,80,15,40,194,15,41};
        memcpy(out,entry,16);if(bad_fingerprint)((unsigned char *)out)[0]^=1;return 1;
    }
    if (bytes==4 && (at==IMAGE+0x3c943dc || at==IMAGE+0x3763b38 || at==IMAGE+0x3763b3c)) {
        float value=at==IMAGE+0x3c943dc?0.f:1.f;memcpy(out,&value,4);return 1;
    }
    return read_memory(ctx,at,out,bytes);
}
static int jc_local_windows_write(uint64_t at,const void *data,size_t bytes) {return write_memory(NULL,at,data,bytes);}
static void fake_actor_hsv(uintptr_t unused,uintptr_t actor,float hue,float saturation,float value) {
    assert(!unused && actor==DUDE && hue==0 && saturation==1 && value==1);++hsv_calls;
}
static int cloth_actor_appearance_status(int code,unsigned stage,unsigned detail) {
    static int previous_code=-1;
    static unsigned previous_stage,previous_detail;
    if (code!=previous_code || stage!=previous_stage || detail!=previous_detail) {
        char line[192];
        snprintf(line,sizeof(line),"[JC] actor appearance code/stage/detail=%d/%u/%u frame=%ld",
            code,stage,detail,(long)JCProbeFrames);
        message(line);previous_code=code;previous_stage=stage;previous_detail=detail;
    }
    return code;
}

static int cloth_actor_descriptor_write(void *context,uint64_t address,const void *data,size_t bytes) {
    struct jc_cloth_lease *lease=context;
    unsigned char check[16];
    if (!bytes || bytes>sizeof(check) || !traveler_context()) return 0;
    for (unsigned i=0;i<2;++i) {
        uint64_t base=lease->descriptor[i];
        if (address<base+0xc62 || address>base+0xcc4 || bytes>base+0xcc4-address) continue;
        return jc_local_windows_write(address,data,bytes) && own_read(NULL,address,check,bytes) &&
            !memcmp(check,data,bytes);
    }
    return 0;
}

static int cloth_actor_numeric_appearance(uint64_t peer,uint64_t generation,uint64_t barn) {
    static const unsigned char entry[16]={64,83,72,131,236,96,15,41,116,36,80,15,40,194,15,41};
    uint64_t image=(uintptr_t)GetModuleHandleW(NULL),timer=0,collection=0;
    unsigned char actual[sizeof(entry)];
    uint32_t count=0,scarf[2],maximum;
    float hsv[3];
    if (!own_read(NULL,image+0x217a70,actual,sizeof(actual)) || memcmp(actual,entry,sizeof(entry)))
        return cloth_actor_appearance_status(JC_FRAME_OWNERSHIP,1,0);
    if (!own_read(NULL,traveler_object+0x9b40,&collection,8))
        return cloth_actor_appearance_status(JC_FRAME_OWNERSHIP,2,0);
    if (!collection) return cloth_actor_appearance_status(0,2,0);
    if (!own_read(NULL,collection+0x91b0,&count,4))
        return cloth_actor_appearance_status(JC_FRAME_OWNERSHIP,3,0);
    if (count<2) return cloth_actor_appearance_status(0,3,count);
    if (count!=2) return cloth_actor_appearance_status(JC_FRAME_OWNERSHIP,3,count);
    struct jc_cloth_lease ready={.image=image,.game=traveler_game,.barn=barn,.dude=traveler_object};
    int capture=jc_cloth_capture(own_read,NULL,&ready,0);
    if (capture) return cloth_actor_appearance_status(JC_FRAME_OWNERSHIP,4,ready.capture_detail);
    unsigned pending=0;
    for (unsigned i=0;i<2;++i) {
        uint32_t state=0,role=0;
        if (!own_read(NULL,barn+0x270118+(uint64_t)ready.slot[i]*0xe0,&state,4) ||
            !own_read(NULL,ready.descriptor[i]+0xc94,&role,4))
            return cloth_actor_appearance_status(JC_FRAME_OWNERSHIP,5,i);
        if (state==2) pending|=1u<<i;
        else if (state!=1 || role!=(i?4u:5u))
            return cloth_actor_appearance_status(JC_FRAME_OWNERSHIP,5,16+i);
    }
    if (pending) return cloth_actor_appearance_status(0,5,pending);
    if (cloth_actor_scarf.dude!=traveler_object || cloth_actor_scarf.generation!=generation ||
        cloth_actor_scarf_peer!=peer) {
        if (!own_read(NULL,traveler_object+0x68,scarf,sizeof(scarf)) ||
            !own_read(NULL,image+0x731fb0,&maximum,4) || !maximum || maximum>4096 ||
            scarf[0]>scarf[1] || scarf[1]>4096) return cloth_actor_appearance_status(JC_FRAME_MATERIAL,6,0);
        cloth_actor_scarf=(struct jc_cloth_lease){.image=image,.game=traveler_game,.barn=barn,
            .players=traveler_barn,.dude=traveler_object,.generation=generation,
            .scarf_charge=fminf(1.f,(float)scarf[0]/(scarf[1]?scarf[1]:1)),
            .scarf_length=fminf(1.f,(float)scarf[1]/maximum)};
        cloth_actor_scarf_peer=peer;
    }
    cloth_actor_scarf.collection=0;
    if (jc_cloth_capture(own_read,NULL,&cloth_actor_scarf,1) ||
        !own_read(NULL,traveler_game+0x68,&timer,8) ||
        !own_read(NULL,image+0x3c943dc,hsv,4) ||
        !own_read(NULL,image+0x3763b38,hsv+1,4) ||
        !own_read(NULL,image+0x3763b3c,hsv+2,4)) return cloth_actor_appearance_status(JC_FRAME_MATERIAL,6,cloth_actor_scarf.capture_detail);
    for (unsigned i=0;i<3;++i) if (!isfinite(hsv[i]) || fabsf(hsv[i])>1000)
        return cloth_actor_appearance_status(JC_FRAME_MATERIAL,7,i);
    int step=jc_cloth_scarf_step(own_read,cloth_actor_descriptor_write,&cloth_actor_scarf,
            &cloth_actor_scarf,timer);
    if (step) return cloth_actor_appearance_status(JC_FRAME_MATERIAL,8,(unsigned)step);
    ((void (*)(uintptr_t,uintptr_t,float,float,float))(uintptr_t)fake_actor_hsv)
        (0,traveler_object,hsv[0],hsv[1],hsv[2]);
    return cloth_actor_appearance_status(0,9,0);
}

static void numeric_reset(void) {
    reset();memset(&cloth_actor_scarf,0,sizeof(cloth_actor_scarf));cloth_actor_scarf_peer=0;
    hsv_calls=0;context_ready=1;
}
static void numeric_ready(void) {
    numeric_reset();put32(players,0xb0+0x70,6);put32(players,0xb0+0x68,2);
    assert(!step(1,1));writes=0;
}
int main(void) {
    numeric_reset();
    assert(!cloth_actor_numeric_appearance(1,1,BARN) && !writes && !hsv_calls);
    unsigned messages=status_messages;
    assert(!cloth_actor_numeric_appearance(1,1,BARN) && status_messages==messages);
    put64(dude,0x9b40,COLLECTION);
    assert(!cloth_actor_numeric_appearance(1,1,BARN) && !writes && !hsv_calls);
    put32(collection,0x91b0,1);
    assert(!cloth_actor_numeric_appearance(1,1,BARN) && !writes && !hsv_calls);
    numeric_ready();put32(barn,0x270118+0xe0,2);put32(barn,0x80+0xd00+0xc94,0);
    assert(!cloth_actor_numeric_appearance(1,1,BARN) && !writes && !hsv_calls);
    put32(barn,0x270118+0xe0,1);put32(barn,0x80+0xd00+0xc94,4);
    assert(!cloth_actor_numeric_appearance(1,1,BARN) && writes && hsv_calls==1);
    uint32_t mode;memcpy(&mode,barn+0x80+0xcb8,4);assert(mode==3);
    assert(!cloth_actor_numeric_appearance(1,1,BARN) && hsv_calls==2);
    numeric_ready();bad_fingerprint=1;
    assert(cloth_actor_numeric_appearance(1,1,BARN)==JC_FRAME_OWNERSHIP && !writes && !hsv_calls);
    numeric_ready();failed_read=DUDE+0x9b40;
    assert(cloth_actor_numeric_appearance(1,1,BARN)==JC_FRAME_OWNERSHIP && !writes && !hsv_calls);
    numeric_ready();put32(collection,0x91b0,3);
    assert(cloth_actor_numeric_appearance(1,1,BARN)==JC_FRAME_OWNERSHIP && !writes && !hsv_calls);
    numeric_ready();put64(collection,0x18,BARN+0x10);
    assert(cloth_actor_numeric_appearance(1,1,BARN)==JC_FRAME_OWNERSHIP && !writes && !hsv_calls);
    numeric_ready();put32(barn,0x270118,3);
    assert(cloth_actor_numeric_appearance(1,1,BARN)==JC_FRAME_OWNERSHIP && !writes && !hsv_calls);
    numeric_ready();put32(barn,0x80+0xc94,4);
    assert(cloth_actor_numeric_appearance(1,1,BARN)==JC_FRAME_OWNERSHIP && !writes && !hsv_calls);
    numeric_ready();failed_write=BARN+0x80+0xca4;
    assert(cloth_actor_numeric_appearance(1,1,BARN)==JC_FRAME_MATERIAL && !hsv_calls);
    numeric_ready();context_ready=0;
    assert(cloth_actor_numeric_appearance(1,1,BARN)==JC_FRAME_MATERIAL && !writes && !hsv_calls);
    puts("Actor appearance integration: absent, empty, partial and pending cloth wait without writes; activation, colour, ownership and write failures checked. Native HSV intercepted.");
}
