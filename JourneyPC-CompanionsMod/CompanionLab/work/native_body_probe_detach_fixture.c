
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum { JC_FRAME_OK,JC_FRAME_OWNERSHIP,JC_FRAME_BUSY,JC_FRAME_MATERIAL,
       JC_ACTOR_RUNTIME_RETIRED=9 };
struct jc_environment_probe { uint64_t pair[2]; unsigned pins; };
struct jc_cloth_live { unsigned closed; struct jc_environment_probe probes[2]; };
struct jc_actor_identity { unsigned unused; };
struct fixture_actor { unsigned state; struct jc_actor_identity identity; };
struct fixture_live { struct jc_cloth_live backend; };
struct jc_cloth_graphics_token { unsigned locked; };
struct jc_environment_texture { uint64_t texture; };
static struct fixture_live live,*traveler_live=&live;
static struct fixture_actor actor,*traveler_actor=&actor;
static unsigned traveler_actor_materials;
static uint64_t traveler_object,traveler_game,barn_addr,stock=0x1000,private_texture=0x2000,binding;
static uint32_t refs=1,private_refs=1;
static unsigned retired,independent,set_calls,bad_set,pair_reads,change_pair,lock_calls,unlock_calls,probe_bad;
static unsigned char object[0x400],game[0x200],*barn;
static int own_read(void *ctx,uint64_t address,void *out,size_t n) {
 (void)ctx;
 if(address==barn_addr+0x646d40) {
  assert(n==16);uint64_t pair[2]={stock,(uintptr_t)&refs};
  if(change_pair && ++pair_reads>1)pair[0]=0x3000;
  memcpy(out,pair,16);return 1;
 }
 if(address==(uintptr_t)&refs && n==4) {memcpy(out,&refs,4);return 1;}
 if(address==traveler_game+0x108 && n==8) {memcpy(out,&barn_addr,8);return 1;}
 if(address==traveler_object+0x81 && n==1) {*(unsigned char *)out=1;return 1;}
 assert(!"unexpected production read");return 0;
}
static int traveler_live_retired(void *ctx,const struct jc_actor_identity *id) {
 (void)ctx;assert(id==&actor.identity);return retired;
}
static int jc_actor_material_independent(void *read,void *ctx,const struct jc_actor_identity *id,void *proof) {
 (void)read;(void)ctx;assert(id==&actor.identity && proof==&traveler_actor_materials);return !independent;
}
static int jc_cloth_live_graphics_enter(struct jc_cloth_live *s,struct jc_cloth_graphics_token *t) {
 assert(s==&live.backend);++lock_calls;t->locked=1;return 1;
}
static int jc_cloth_live_graphics_leave(struct jc_cloth_live *s,struct jc_cloth_graphics_token *t) {
 assert(s==&live.backend && t->locked);++unlock_calls;t->locked=0;return 1;
}
static int jc_environment_texture_read(void *read,void *ctx,uint64_t texture,struct jc_environment_texture *out) {
 (void)read;(void)ctx;out->texture=texture;return 0;
}
static int jc_environment_texture_ready(const struct jc_environment_texture *t) {return t->texture==stock;}
static int jc_environment_texture_same(const struct jc_environment_texture *a,const struct jc_environment_texture *b) {
 return a->texture==b->texture;
}
static int jc_cloth_texture_check(void *read,void *ctx,uint64_t material,const char *name,uint64_t texture,unsigned bound) {
 (void)read;(void)ctx;(void)texture;assert(material==traveler_object+0x1e0 && !strcmp(name,"envOccCube") && !bound);return 0;
}
static int jc_cloth_texture_bound(void *read,void *ctx,uint64_t material,const char *name,uint64_t texture) {
 (void)read;(void)ctx;assert(material==traveler_object+0x1e0 && !strcmp(name,"envOccCube"));return binding!=texture;
}
static int jc_environment_probe_check(void *read,void *ctx,struct jc_environment_probe *p) {
 (void)read;(void)ctx;assert(p==live.backend.probes);return probe_bad;
}
static uintptr_t setter(uintptr_t material,uintptr_t name,uintptr_t texture) {
 assert(lock_calls>unlock_calls && material==traveler_object+0x1e0 && !strcmp((char *)name,"envOccCube") && texture==stock);
 ++set_calls;if(!bad_set)binding=texture;return 0;
}
static uintptr_t (*original_cloth_setter[3])(uintptr_t,uintptr_t,uintptr_t)={NULL,NULL,setter};
static int traveler_live_detach_body_probe(struct jc_cloth_live *s) {
    if (!s || s->closed) return JC_FRAME_OK;
    unsigned owned=0;
    for (unsigned i=0;i<sizeof(s->probes)/sizeof(s->probes[0]);++i)
        owned|=s->probes[i].pair[0]!=0;
    if (!owned) return JC_FRAME_OK;
    if (!traveler_live || s!=&traveler_live->backend || !traveler_actor ||
        traveler_actor->state!=JC_ACTOR_RUNTIME_RETIRED || !original_cloth_setter[2])
        return JC_FRAME_OWNERSHIP;
    if (!traveler_live_retired(NULL,&traveler_actor->identity)) return JC_FRAME_BUSY;
    if (jc_actor_material_independent(own_read,NULL,&traveler_actor->identity,&traveler_actor_materials))
        return JC_FRAME_MATERIAL;
    struct jc_cloth_graphics_token token={0};
    if (!jc_cloth_live_graphics_enter(s,&token)) return JC_FRAME_OWNERSHIP;
    int code=JC_FRAME_MATERIAL;
    uint64_t barn=0,pair[2]={0},again[2]={0};uint32_t refs=0;unsigned char remote=0;
    struct jc_environment_texture before,after;
    uint64_t material=traveler_object+0x1e0;
    if (!own_read(NULL,traveler_game+0x108,&barn,8) || !barn ||
        !own_read(NULL,traveler_object+0x81,&remote,1) || remote!=1 ||
        !own_read(NULL,barn+0x646d40,pair,sizeof(pair)) || !pair[0] || !pair[1] ||
        !own_read(NULL,pair[1],&refs,4) || !refs || refs>0x1000000 ||
        jc_environment_texture_read(own_read,NULL,pair[0],&before) ||
        !jc_environment_texture_ready(&before) ||
        jc_cloth_texture_check(own_read,NULL,material,"envOccCube",pair[0],0)) goto done;
    unsigned bound=!jc_cloth_texture_bound(own_read,NULL,material,"envOccCube",pair[0]);
    for (unsigned i=0;i<sizeof(s->probes)/sizeof(s->probes[0]);++i) {
        struct jc_environment_probe *p=&s->probes[i];
        if (!p->pair[0]) continue;
        if (p->pair[0]==pair[0] || p->pair[1]==pair[1] || p->pins ||
            jc_environment_probe_check(own_read,NULL,p)) goto done;
        bound|=!jc_cloth_texture_bound(own_read,NULL,material,"envOccCube",p->pair[0]);
    }
    if (!bound || !own_read(NULL,barn+0x646d40,again,sizeof(again)) ||
        memcmp(pair,again,sizeof(pair)) ||
        jc_environment_texture_read(own_read,NULL,pair[0],&after) ||
        !jc_environment_texture_same(&before,&after)) goto done;
    original_cloth_setter[2](material,(uintptr_t)"envOccCube",pair[0]);
    if (jc_cloth_texture_bound(own_read,NULL,material,"envOccCube",pair[0]) ||
        !own_read(NULL,barn+0x646d40,again,sizeof(again)) || memcmp(pair,again,sizeof(pair)) ||
        jc_environment_texture_read(own_read,NULL,pair[0],&after) ||
        !jc_environment_texture_same(&before,&after) ||
        jc_actor_material_independent(own_read,NULL,&traveler_actor->identity,&traveler_actor_materials)) goto done;
    code=JC_FRAME_OK;
done:
    if (!jc_cloth_live_graphics_leave(s,&token)) return JC_FRAME_OWNERSHIP;
    return code;
}

static void setup(void) {
 memset(&live,0,sizeof(live));actor.state=JC_ACTOR_RUNTIME_RETIRED;
 traveler_live=&live;traveler_actor=&actor;traveler_object=(uintptr_t)object;traveler_game=(uintptr_t)game;
 live.backend.probes[0].pair[0]=private_texture;live.backend.probes[0].pair[1]=(uintptr_t)&private_refs;
 binding=private_texture;refs=1;retired=independent=1;
 set_calls=bad_set=pair_reads=change_pair=lock_calls=unlock_calls=probe_bad=0;
}
int main(void) {
 barn=calloc(1,0x647000);assert(barn);barn_addr=(uintptr_t)barn;unsigned cases=0;
 setup();retired=0;assert(traveler_live_detach_body_probe(&live.backend)==JC_FRAME_BUSY && !set_calls && !lock_calls);++cases;
 setup();change_pair=1;assert(traveler_live_detach_body_probe(&live.backend)==JC_FRAME_MATERIAL && !set_calls && lock_calls==unlock_calls);++cases;
 setup();binding=0x4000;assert(traveler_live_detach_body_probe(&live.backend)==JC_FRAME_MATERIAL && !set_calls && lock_calls==unlock_calls);++cases;
 setup();bad_set=1;assert(traveler_live_detach_body_probe(&live.backend)==JC_FRAME_MATERIAL && set_calls==1 && binding==private_texture && lock_calls==unlock_calls);++cases;
 setup();assert(!traveler_live_detach_body_probe(&live.backend) && binding==stock && set_calls==1);
 assert(!traveler_live_detach_body_probe(&live.backend) && binding==stock && set_calls==2 && lock_calls==unlock_calls);
 live.backend.closed=1;assert(!traveler_live_detach_body_probe(&live.backend) && set_calls==2);++cases;
 setup();independent=0;assert(traveler_live_detach_body_probe(&live.backend)==JC_FRAME_MATERIAL && !set_calls && !lock_calls);++cases;
 setup();live.backend.probes[0].pins=1;assert(traveler_live_detach_body_probe(&live.backend)==JC_FRAME_MATERIAL && !set_calls && lock_calls==unlock_calls);++cases;
 setup();probe_bad=1;assert(traveler_live_detach_body_probe(&live.backend)==JC_FRAME_MATERIAL && !set_calls && lock_calls==unlock_calls);++cases;
 setup();memset(live.backend.probes,0,sizeof(live.backend.probes));traveler_actor=NULL;
 assert(!traveler_live_detach_body_probe(&live.backend) && !set_calls && !lock_calls);++cases;
 free(barn);printf("Production body probe detachment: %u retirement, identity, foreign binding, failed setter, repeated close and lease cases passed.\n",cases);
}
