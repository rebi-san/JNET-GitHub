#include "native_actor_runtime.h"
#include "native_companion_chirp.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "actor_code.h"
#define JC_GLYPH_ABI __attribute__((ms_abi))
static struct jc_actor_runtime *traveler_actor;
static uint64_t traveler_object;
static unsigned traveler_actor_materials,alias,calls,display_seen;
static uintptr_t barn_seen,read_failure;
static int own_read(void *p,uint64_t a,void *v,size_t n) {
    (void)p;if(a==read_failure)return 0;memcpy(v,(void *)(uintptr_t)a,n);return 1;
}
static uint64_t thread(void *p) {return *(uint64_t *)p;}
static int traveler_context(void) {return 1;}
static int jc_actor_material_independent(jc_layout_reader r,void *p,const struct jc_actor_identity *id,unsigned *o) {
    (void)r;(void)p;(void)id;(void)o;return alias;
}
static void message(const char *s) {assert(s && *s);}
#include "traveler_chirp_glyph.h"
static void put(void *p,size_t at,uint64_t n) {memcpy((char *)p+at,&n,8);}
static void number(void *p,size_t at,float n) {memcpy((char *)p+at,&n,4);}
static void JC_GLYPH_ABI mock(uintptr_t e,float dt,uintptr_t barn,uintptr_t camera,
    uintptr_t a5,uintptr_t a6,uintptr_t sounds,uintptr_t a8,uintptr_t a9,
    uintptr_t terrain,uintptr_t physics,unsigned char display,uintptr_t a13,uintptr_t chirps) {
    assert(e && dt==.125f && camera==4 && a5==5 && a6==6 && sounds==7 && a8==8 && a9==9);
    assert(terrain==10 && physics==11 && a13==13 && chirps);
    ++calls;barn_seen=barn;display_seen=display;
}
static int JC_GLYPH_ABI native_strcmp(const char *a,const char *b) {return strcmp(a,b);}
static uintptr_t JC_GLYPH_ABI sound_lookup(void) {return 0;}
static float JC_GLYPH_ABI terrain_height(void) {return -100.0f;}
static _Alignas(16) unsigned char ground_hit[32];
static uintptr_t JC_GLYPH_ABI ground_probe(void) {return (uintptr_t)ground_hit;}
static void thunk(unsigned char *image,size_t at,uintptr_t target) {
    unsigned char code[12]={0x48,0xb8};memcpy(code+2,&target,8);code[10]=0xff;code[11]=0xe0;
    memcpy(image+at,code,12);
}
int main(void) {
    unsigned char *image=mmap(NULL,0x4000000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);assert(image!=MAP_FAILED);
    load_code(image);
    thunk(image,0x147fc0,(uintptr_t)sound_lookup);thunk(image,0x82340,(uintptr_t)terrain_height);
    thunk(image,0xea8b0,(uintptr_t)ground_probe);number(ground_hit,16,1000);
    unsigned char jump[12]={0x48,0xb8};uintptr_t compare=(uintptr_t)native_strcmp;
    memcpy(jump+2,&compare,8);jump[10]=0xff;jump[11]=0xe0;memcpy(image+0x49bac4,jump,12);
    const unsigned char trig[]={0x0f,0x10,0x05,1,0,0,0,0xc3};
    memcpy(image+0x49a890,trig,8);number(image,0x49a89c,1);
    unsigned char *actor=calloc(1,0x14640),*barn=calloc(1,0x28dd0),*game=calloc(1,0x200);
    _Alignas(16) unsigned char pool[0x720]={0},glyph[0x10d0]={0},bubble[0x10d0]={0},camera[0x100]={0};
    assert(actor && barn && game);traveler_object=(uintptr_t)actor;
    uint64_t owner=11;struct jc_actor_runtime runtime={.owner=11,.state=JC_ACTOR_RUNTIME_ACTIVE,
        .ops={.thread=thread,.context=&owner},.identity={.actor=(uintptr_t)actor,.barn=(uintptr_t)barn,
            .game=(uintptr_t)game,.glyph=(uintptr_t)glyph,.peer=1,.generation=2}};
    traveler_actor=&runtime;put(game,0x1b8,(uintptr_t)pool);put(actor,0x2240,(uintptr_t)glyph);
    struct jc_chirp_bridge bridge={.barn=(uintptr_t)pool,.slot=0,.counter=17};
    uint32_t counter=17;memcpy(pool+0x50,&counter,4);pool[0x68]=1;
    traveler_chirp_glyph_mark(&bridge);original_glyph_update=mock;
    for(unsigned test=0;test<8;++test) {
        alias=test==1;owner=test==2?12:11;counter=test==3?18:17;
        memcpy(pool+0x50,&counter,4);pool[0x4d]=test==4;pool[0x68]=test==5?0:1;
        runtime.identity.generation=test==6?3:2;read_failure=test==7?(uintptr_t)actor+0x2240:0;
        traveler_glyph_update((uintptr_t)pool+0x10,.125f,(uintptr_t)barn,4,5,6,7,8,9,10,11,0,13,(uintptr_t)pool);
        assert(display_seen==(test==0));assert(barn_seen==(test==0?(uintptr_t)actor-0xb0:(uintptr_t)barn));
    }
    assert(calls==8);alias=0;owner=11;counter=17;memcpy(pool+0x50,&counter,4);
    pool[0x4d]=0;pool[0x68]=1;runtime.identity.generation=2;read_failure=0;
    put(pool,0x60,(uintptr_t)bubble);put(pool,0x70,UINT64_MAX);
    number(pool,0x20,3);number(pool,0x24,4);number(pool,0x28,5);number(pool,0x2c,1);
    number(pool,0x40,2);number(pool,0x44,1);
    number(actor,0x1470,1);number(actor,0x1e64,1);number(camera,0x60,10);number(camera,0x68,20);
    number(image,0x732004,1);number(image,0x73200c,1);
    const size_t globals[]={0x358d918,0x358cc68,0x358d338,0x358cc64,0x358f5a8};
    for(unsigned i=0;i<sizeof(globals)/sizeof(*globals);++i)number(image,globals[i],1);
    uint32_t six=6;memcpy(glyph+0xf20,&six,4);
    for(unsigned i=0;i<6;++i)snprintf((char *)glyph+0x740+i*128,32,"existing%u",i);
    unsigned char before[0x28dd0];memcpy(before,barn,sizeof(before));
    assert(!mprotect(image,0x700000,PROT_READ|PROT_EXEC));
    original_glyph_update=(jc_glyph_update_fn)(image+0x13e6b0);
    unsigned char saved_pool[sizeof(pool)],saved_glyph[sizeof(glyph)],saved_bubble[sizeof(bubble)];
    memcpy(saved_pool,pool,sizeof(pool));memcpy(saved_glyph,glyph,sizeof(glyph));memcpy(saved_bubble,bubble,sizeof(bubble));
    number(actor,0x1e64,0);
    traveler_glyph_update((uintptr_t)pool+0x10,.125f,(uintptr_t)barn,(uintptr_t)camera,0,0,0,0,0,0,0,0,0,(uintptr_t)pool);
    float broken[16];unsigned nonfinite=0;memcpy(broken,glyph+0x540,sizeof(broken));
    for(unsigned i=0;i<16;++i)nonfinite+=!isfinite(broken[i]);
    assert(nonfinite);
    memcpy(pool,saved_pool,sizeof(pool));memcpy(glyph,saved_glyph,sizeof(glyph));memcpy(bubble,saved_bubble,sizeof(bubble));
    number(actor,0x1e64,1);
    traveler_glyph_update((uintptr_t)pool+0x10,.125f,(uintptr_t)barn,(uintptr_t)camera,0,0,0,0,0,0,0,0,0,(uintptr_t)pool);
    uint32_t count;memcpy(&count,glyph+0xf20,4);assert(count==7);
    assert(!strcmp((char *)glyph+0x740+6*128,"alpha"));
    float alpha;memcpy(&alpha,glyph+0x730+6*128,4);assert(isfinite(alpha) && alpha>0);
    assert(glyph[0x80]==1 && !memcmp(before,barn,sizeof(before)));
    float matrix[16];memcpy(matrix,glyph+0x540,64);
    for(unsigned i=0;i<16;++i)assert(isfinite(matrix[i]));
    assert(matrix[15]==1 && (matrix[0]!=0 || matrix[1]!=0 || matrix[2]!=0));
    unsigned char *sounds=calloc(1,0x7c7b8);assert(sounds);
    strcpy((char *)sounds+0x70234,"Player2Chirp");pool[0x78]=1;
    memcpy(glyph+0xf20,&six,4);memset(glyph+0x540,0,64);
    traveler_glyph_update((uintptr_t)pool+0x10,.125f,(uintptr_t)barn,(uintptr_t)camera,0,0,
        (uintptr_t)sounds,0,0,0,0,0,0,(uintptr_t)pool);
    memcpy(&count,glyph+0xf20,4);assert(count==7 && pool[0x78]==0);
    memcpy(&alpha,glyph+0x730+6*128,4);assert(isfinite(alpha) && alpha>0);
    assert(!memcmp(before,barn,sizeof(before)) && !pool[0x4c] && !pool[0x4d]);
    runtime.state=JC_ACTOR_RUNTIME_RETIRED;
    number(pool,0x54,1.0f);
    traveler_glyph_update((uintptr_t)pool+0x10,.125f,(uintptr_t)barn,(uintptr_t)camera,0,0,
        (uintptr_t)sounds,0,0,0,0,0,0,(uintptr_t)pool);
    memcpy(&alpha,glyph+0x730+6*128,4);assert(alpha==0 && pool[0x68]==0);
    free(sounds);
    free(actor);free(barn);free(game);munmap(image,0x4000000);
    puts("Chirp glyph: production routing/14-argument ABI, reused slots, wrong thread, alias and identity rejection passed. Zero chirp multiplier reproduced nonfinite native glyph geometry; valid multiplier restored finite geometry. Original native display pass added alpha and positioned owned glyph; first-use, ongoing display and retirement fade executed; audio/terrain lookups intercepted.");
    return 0;
}
