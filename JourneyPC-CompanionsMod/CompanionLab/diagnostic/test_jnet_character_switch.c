#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "jnet_character_native.h"
#include "jnet_remote_pose.h"
struct jnet_war_hit {uint64_t victim,lease,sequence,seen;};
#include "character_switch_state.h"
static struct jnet_character_state jnet_character_instances[3],jnet_character_caches[3][JNET_CHARACTER_COUNT*3];
static unsigned jnet_character_owner,jnet_character_failed_masks[3];
#define jnet_character_instance jnet_character_instances[jnet_character_owner]
#define jnet_character_cached jnet_character_caches[jnet_character_owner]
#define jnet_character_failed_mask jnet_character_failed_masks[jnet_character_owner]
static uint64_t jnet_character_generation=1,fault_image=0x10000000;
static unsigned jnet_menu_supported_characters=(1u<<JNET_CHARACTER_COUNT)-1;
static unsigned allocations[3][JNET_CHARACTER_COUNT*3],repositions,reads,writes,reject_reposition;
static unsigned char barns[4][0xa000],creatures[3][JNET_CHARACTER_COUNT*3][0x20],targets[3][JNET_CHARACTER_COUNT*3][0x40];
static const uintptr_t game=0x30000000;
static uint64_t GetTickCount64(void){return 1000;}
static int own_read(void *ctx,uint64_t at,void *out,size_t n){(void)ctx;++reads;memcpy(out,(void *)(uintptr_t)at,n);return 1;}
static int jc_local_windows_write(uint64_t at,const void *in,size_t n){++writes;memcpy((void *)(uintptr_t)at,in,n);return 1;}
static void message(const char *line){assert(line);}
static int jnet_character_restore_wayfarer(struct jnet_character_state *s){
    memcpy((void *)(uintptr_t)(s->barn+0xb0+0x1e00),s->saved_dissolve,4);
    memcpy((void *)(uintptr_t)(s->barn+0xb0+0x2000),s->saved_dissolve+1,4);
    memcpy((void *)(uintptr_t)(s->barn+0xb0+0x9c34),s->saved_visibility,1);
    memcpy((void *)(uintptr_t)(s->barn+0xb0+0x9cc4),s->saved_visibility+1,1);
    return 1;
}
static int jnet_character_visibility(struct jnet_character_state *s,int visible){
    assert(jnet_character_binding_valid(own_read,NULL,&s->binding,jnet_character_generation));
    if(!visible && s->kind==JNET_CHARACTER_WAR_MACHINE)s->control.spotlight=0;
    return 1;
}
static void jnet_character_fail(struct jnet_character_state *s){
    s->failed=1;jnet_character_failed_masks[s->owner]|=1u<<s->kind;
    jnet_character_restore_wayfarer(s);
}
static void jnet_character_restore_previous(const struct jnet_character_state *previous){
    if(previous->binding.creature && !previous->failed)jnet_character_instance=*previous;
}
#include "character_reactivate_code.h"
static int jnet_character_reposition_cached(uintptr_t selected_game,unsigned kind){
    assert(selected_game==game && kind==jnet_character_instance.kind);++repositions;
    if(reject_reposition)return 0;
    jnet_character_reactivate_state(&jnet_character_instance);return 1;
}
static int jnet_character_prepare_native(uintptr_t selected_game,uintptr_t barn,unsigned kind,unsigned skin){
    assert(selected_game==game);unsigned index=kind+JNET_CHARACTER_COUNT*skin;
    assert(!allocations[jnet_character_owner][index]++);
    uintptr_t creature=(uintptr_t)creatures[jnet_character_owner][index];
    uintptr_t target=(uintptr_t)targets[jnet_character_owner][index],type=fault_image+0x5ce668;
    memcpy((void *)creature,&type,8);memcpy((void *)(creature+0x10),&target,8);
    jnet_character_instance=(struct jnet_character_state){.game=game,.barn=barn,.kind=kind,.skin=skin,
        .owner=jnet_character_owner,.binding={fault_image,creature,target,jnet_character_generation},.control={.kind=kind}};
    return 1;
}
#include "character_switch_code.h"
#include "character_ride_owner_code.h"
static const struct {unsigned kind,variant;} selections[]={
    {JNET_CHARACTER_WAYFARER,0},{JNET_CHARACTER_LONGTAIL,0},{JNET_CHARACTER_LONGTAIL,1},
    {JNET_CHARACTER_WAR_MACHINE,0},{JNET_CHARACTER_WAR_MACHINE,1},{JNET_CHARACTER_WAR_MACHINE,2},
    {JNET_CHARACTER_WHALE,0},{JNET_CHARACTER_WHALE,1},{JNET_CHARACTER_FLOW_1,0},{JNET_CHARACTER_FLOW_2,0}
};
static void baseline(unsigned owner){
    float dissolve[2]={.1f*(float)(owner+1),.15f*(float)(owner+1)};
    unsigned char visibility[2]={owner&1,(owner+1)&1};
    memcpy(barns[owner]+0xb0+0x1e00,dissolve,4);memcpy(barns[owner]+0xb0+0x2000,dissolve+1,4);
    memcpy(barns[owner]+0xb0+0x9c34,visibility,1);memcpy(barns[owner]+0xb0+0x9cc4,visibility+1,1);
}
static void select_character(unsigned owner,unsigned selection){
    jnet_character_owner=owner;unsigned kind=selections[selection].kind;
    assert(jnet_character_switch_native(game,(uintptr_t)barns[owner],kind,selections[selection].variant));
    struct jnet_character_state *s=&jnet_character_instance;
    assert(s->kind==kind);
    float expected[2]={.1f*(float)(owner+1),.15f*(float)(owner+1)};
    unsigned char visibility[2]={owner&1,(owner+1)&1};
    if(kind){
        assert(s->owner==owner && s->skin==(kind==JNET_CHARACTER_WAR_MACHINE?selections[selection].variant:0));
        assert(!memcmp(s->saved_dissolve,expected,sizeof(expected)) && !memcmp(s->saved_visibility,visibility,sizeof(visibility)));
        s->initialized=1;
        float hidden=1;unsigned char disabled=0;
        memcpy(barns[owner]+0xb0+0x1e00,&hidden,4);memcpy(barns[owner]+0xb0+0x2000,&hidden,4);
        memcpy(barns[owner]+0xb0+0x9c34,&disabled,1);memcpy(barns[owner]+0xb0+0x9cc4,&disabled,1);
    }else{
        assert(!memcmp(barns[owner]+0xb0+0x1e00,expected,4) && !memcmp(barns[owner]+0xb0+0x2000,expected+1,4));
        assert(barns[owner][0xb0+0x9c34]==visibility[0] && barns[owner][0xb0+0x9cc4]==visibility[1]);
    }
}
int main(void){
    for(unsigned owner=0;owner<4;++owner)baseline(owner);
    for(unsigned a=0;a<10;++a)for(unsigned b=0;b<10;++b)for(unsigned c=0;c<10;++c){
        select_character(0,a);select_character(1,b);select_character(2,c);
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<i;++j)
            if(jnet_character_instances[i].binding.creature && jnet_character_instances[j].binding.creature)
                assert(jnet_character_instances[i].binding.creature!=jnet_character_instances[j].binding.creature);
    }
    for(unsigned owner=0;owner<3;++owner){
        for(unsigned before=0;before<10;++before)for(unsigned after=0;after<10;++after){
            select_character(owner,before);select_character(owner,after);
        }
        unsigned count=0;for(unsigned i=0;i<JNET_CHARACTER_COUNT*3;++i)count+=allocations[owner][i];
        assert(count==7);select_character(owner,0);
    }
    select_character(0,1);
    struct jnet_character_state *s=&jnet_character_instance;
    s->sprint=1;s->follow_position_valid=1;s->grab_blocked=1;s->motion[0]=8;
    s->ride_peer=s->ride_epoch=s->ride_tick=s->ride_active=9;
    s->attack_busy=1;s->attack_target=s->attack_target_lease=s->attack_actor=9;
    s->remote_pose.count=1;s->war_hits[0].sequence=7;
    jnet_character_reactivate_state(s);
    assert(!s->initialized && !s->sprint && !s->follow_position_valid && !s->grab_blocked && !s->motion[0]);
    assert(!s->ride_peer && !s->ride_epoch && !s->ride_tick && !s->ride_active);
    assert(!s->attack_busy && !s->attack_target && !s->attack_target_lease && !s->attack_actor);
    assert(!s->remote_pose.count && s->war_hits[0].sequence==7);
    assert(jnet_character_switch_native(game,(uintptr_t)barns[3],JNET_CHARACTER_WHALE,0));
    float expected[2]={.4f,.6f};
    assert(!memcmp(jnet_character_instance.saved_dissolve,expected,sizeof(expected)));
    assert(jnet_character_switch_native(game,(uintptr_t)barns[3],0,0));
    select_character(0,1);
    unsigned before_reads=reads,before_writes=writes;
    assert(!jnet_character_switch_native(game+1,(uintptr_t)barns[0],JNET_CHARACTER_LONGTAIL,0));
    ++jnet_character_generation;
    assert(!jnet_character_switch_native(game,(uintptr_t)barns[0],JNET_CHARACTER_WHALE,0));
    --jnet_character_generation;
    assert(reads==before_reads && writes==before_writes);
    jnet_character_cached[JNET_CHARACTER_WHALE].binding.generation=2;
    assert(!jnet_character_switch_native(game,(uintptr_t)barns[0],JNET_CHARACTER_WHALE,0));
    assert(reads==before_reads && writes==before_writes);
    jnet_character_cached[JNET_CHARACTER_WHALE].binding.generation=1;
    reject_reposition=1;
    assert(!jnet_character_switch_native(game,(uintptr_t)barns[0],JNET_CHARACTER_WHALE,0));
    assert(jnet_character_instance.kind==JNET_CHARACTER_LONGTAIL && !jnet_character_instance.failed);
    assert(jnet_character_failed_mask&(1u<<JNET_CHARACTER_WHALE));
    assert(!jnet_character_switch_native(game,(uintptr_t)barns[0],JNET_CHARACTER_COUNT,0));
    assert(!jnet_character_switch_native(game,(uintptr_t)barns[0],JNET_CHARACTER_WAR_MACHINE,3));
    assert(!jnet_character_switch_native(game,(uintptr_t)barns[0],JNET_CHARACTER_ROCKET,0));
    reject_reposition=0;select_character(0,3);
    unsigned char attack[0xb0]={0};uintptr_t attack_type=fault_image+0x66ce70;
    memcpy(attack,&attack_type,8);memcpy(attack+0x48,&jnet_character_instance.binding.creature,8);
    attack[8]=attack[9]=1;jnet_character_instance.attack_event=(uintptr_t)attack;
    before_writes=writes;
    assert(!jnet_character_switch_native(game,(uintptr_t)barns[0],JNET_CHARACTER_WAYFARER,0));
    assert(jnet_character_instance.kind==JNET_CHARACTER_WAR_MACHINE && writes==before_writes);
    attack[9]=0;assert(jnet_character_switch_native(game,(uintptr_t)barns[0],JNET_CHARACTER_WAYFARER,0));
    for(unsigned owner=0;owner<3;++owner){
        struct jnet_character_state *s=&jnet_character_instances[owner];
        s->ride_event=100+owner;s->grab_event=200+owner;s->game=game;s->initialized=1;s->failed=0;
        assert(jnet_character_ride_owner(s->ride_event,game)==(owner!=0));
        assert(jnet_character_ride_owner(s->grab_event,game)==(owner!=0));
        assert(!jnet_character_ride_owner(s->ride_event,game+1));
        s->initialized=0;assert(!jnet_character_ride_owner(s->ride_event,game));s->initialized=1;
        s->failed=1;assert(!jnet_character_ride_owner(s->ride_event,game));s->failed=0;
        jnet_character_caches[owner][1].ride_event=300+owner;
        assert(!jnet_character_ride_owner(300+owner,game));
    }
    assert(jnet_character_ride_owner(999,game)==-1);
    assert(repositions>100);
    puts("Creature switches: 1000 three-owner combinations, 300 ordered transitions, 21 bounded native instances, actor baseline preservation, cache resets, stale generations and failure rollback passed. Native allocation/teleport mocked.");
}
