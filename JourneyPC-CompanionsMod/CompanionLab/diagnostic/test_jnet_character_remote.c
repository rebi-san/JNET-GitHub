#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "jnet_character_native.h"
#include "jnet_remote_pose.h"
#define JNET_STEAM_CAPACITY 3
#define JC_ACTOR_RUNTIME_ACTIVE 2
struct jnet_war_hit {uint64_t victim,lease,sequence,seen;};
struct jnet_game_record {
    uint64_t id,lease,sequence,seen;
    unsigned character_known,character,variant,spotlight,war_active,rotation_known;
    float war_anger,rotation[4];struct jnet_war_hit war_hits[2];
    struct {unsigned chapter;struct {unsigned char bytes[128];} state;} sample;
};
#include "character_switch_state.h"
static struct jnet_character_state jnet_character_instances[3],jnet_character_caches[3][JNET_CHARACTER_COUNT*3];
static unsigned jnet_character_owner;
#define jnet_character_instance jnet_character_instances[jnet_character_owner]
#define jnet_character_cached jnet_character_caches[jnet_character_owner]
static unsigned jnet_character_generation=1,jnet_current_level=1,jnet_sg_lock;
static struct {struct {unsigned character;} applied;} jnet_menu_edit;
static struct {uint64_t room,identity;struct jnet_game_record records[3],war_events[3];} jnet_sg_mail;
struct jc_network_peer {struct {unsigned admitted;uint64_t peer,remote_epoch;} connection;};
static struct {unsigned paused;struct jc_network_peer peers[2];} traveler_network;
static struct {unsigned state;} actor={2},*traveler_actor=&actor;
static unsigned traveler_local_state=2,reject_switch,switches,hits,invalid_binding,writes;
static uint64_t clock_ms=1000;
static unsigned char game_storage[0x300],barn_storage[0x20000],actor_storage[0xa000],events_storage[0x20];
static unsigned char creatures[3][0x20],targets[3][0x40];
static uintptr_t traveler_barn,traveler_object,fault_image;
static uint64_t GetTickCount64(void){return clock_ms;}
static void AcquireSRWLockExclusive(unsigned *p){assert(!*p);*p=1;}
static void ReleaseSRWLockExclusive(unsigned *p){assert(*p);*p=0;}
static int own_read(void *ctx,uint64_t at,void *out,size_t n){
    (void)ctx;if(at<0x100000000)return 0;memcpy(out,(void *)(uintptr_t)at,n);return 1;
}
static int jc_local_windows_write(uint64_t at,const void *in,size_t n){assert(at>=0x100000000);++writes;memcpy((void *)(uintptr_t)at,in,n);return 1;}
static int jnet_character_visibility(struct jnet_character_state *s,int visible){(void)s;(void)visible;return 1;}
static int jnet_character_restore_wayfarer(struct jnet_character_state *s){
    return jc_local_windows_write(s->barn+0xb0+0x1e00,s->saved_dissolve,4) &&
        jc_local_windows_write(s->barn+0xb0+0x2000,s->saved_dissolve+1,4) &&
        jc_local_windows_write(s->barn+0xb0+0x9c34,s->saved_visibility,1) &&
        jc_local_windows_write(s->barn+0xb0+0x9cc4,s->saved_visibility+1,1);
}
static void jnet_character_fail(struct jnet_character_state *s){s->failed=1;}
static int jnet_character_switch_native(uintptr_t game,uintptr_t barn,unsigned kind,unsigned variant){
    ++switches;if(reject_switch && kind)return 0;
    struct jnet_character_state *s=&jnet_character_instance;
    if(!kind){if(s->binding.creature)assert(jnet_character_restore_wayfarer(s));memset(s,0,sizeof(*s));return 1;}
    *s=(struct jnet_character_state){.game=game,.barn=barn,.owner=jnet_character_owner,.kind=kind,
        .skin=kind==JNET_CHARACTER_WAR_MACHINE?variant:0,.initialized=1,.saved_dissolve={.25f,.75f},.saved_visibility={1,1},
        .binding={fault_image,(uintptr_t)creatures[jnet_character_owner],(uintptr_t)targets[jnet_character_owner],jnet_character_generation}};
    uintptr_t type=fault_image+0x5ce668;
    memcpy(creatures[jnet_character_owner],&type,8);memcpy(creatures[jnet_character_owner]+0x10,&s->binding.target,8);
    if(invalid_binding)s->binding.target+=8;
    return 1;
}
static void jnet_character_tick_native(uintptr_t game){assert(game==(uintptr_t)game_storage);}
static void jnet_character_hit_original(uintptr_t event,uintptr_t dude,uintptr_t a,uintptr_t b,uintptr_t events){
    (void)event;assert(dude==traveler_barn+0xb0 && !a && !b && events==(uintptr_t)events_storage);++hits;
}
static int jnet_character_ride_resolve(const struct jnet_game_record *r,float position[3]){(void)r;(void)position;return 0;}
#include "character_remote_code.h"
static void tick(void){jnet_character_remote_tick((uintptr_t)game_storage);assert(!jnet_character_owner && !jnet_sg_lock);}
static void hit(unsigned slot,uint64_t sequence){
    struct jnet_game_record *r=&jnet_sg_mail.records[slot+1];
    r->seen=clock_ms;r->war_hits[0]=(struct jnet_war_hit){1,10,sequence,clock_ms};
}
int main(void){
    fault_image=1;traveler_barn=(uintptr_t)barn_storage;traveler_object=(uintptr_t)actor_storage;
    uintptr_t events=(uintptr_t)events_storage;memcpy(game_storage+0x250,&events,8);
    jnet_sg_mail.room=1;jnet_sg_mail.identity=1;jnet_sg_mail.records[0].id=1;jnet_sg_mail.records[0].lease=10;
    for(unsigned slot=0;slot<2;++slot){
        traveler_network.peers[slot].connection=(__typeof__(traveler_network.peers[slot].connection)){1,2+slot,20+slot};
        jnet_sg_mail.records[slot+1]=(struct jnet_game_record){.id=2+slot,.lease=20+slot,.sequence=1,.seen=1000,
            .character_known=1,.character=JNET_CHARACTER_WAR_MACHINE,.sample={.chapter=1}};
        hit(slot,5);
    }
    tick();assert(hits==2);tick();assert(hits==2);
    for(unsigned skin=0;skin<3;++skin){
        for(unsigned slot=0;slot<2;++slot)jnet_sg_mail.records[slot+1].variant=skin;
        tick();assert(hits==2);
    }
    jnet_sg_mail.records[1].character=JNET_CHARACTER_LONGTAIL;tick();
    jnet_sg_mail.records[1].character=JNET_CHARACTER_WAR_MACHINE;tick();assert(hits==2);
    jnet_sg_mail.records[1].character_known=0;tick();
    jnet_sg_mail.records[1].character_known=1;tick();assert(hits==2);
    traveler_network.paused=1;tick();traveler_network.paused=0;tick();assert(hits==2);
    jnet_menu_edit.applied.character=JNET_CHARACTER_LONGTAIL;hit(0,6);tick();assert(hits==2);
    jnet_menu_edit.applied.character=JNET_CHARACTER_WAYFARER;tick();assert(hits==2);
    jnet_character_instances[0].binding.creature=1;hit(0,7);tick();assert(hits==2);
    jnet_character_instances[0].binding.creature=0;tick();assert(hits==2);
    hit(0,8);tick();assert(hits==3);
    ++traveler_network.peers[0].connection.remote_epoch;++jnet_sg_mail.records[1].lease;
    hit(0,1);tick();assert(hits==4);
    hit(0,2);jnet_sg_mail.records[1].war_hits[0].lease=11;tick();assert(hits==4);
    hit(0,3);jnet_sg_mail.records[1].war_hits[0].seen=clock_ms+1;tick();assert(hits==4);
    clock_ms+=2000;hit(0,4);jnet_sg_mail.records[1].war_hits[0].seen=clock_ms-2000;tick();assert(hits==4);
    hit(0,5);tick();assert(hits==5);
    invalid_binding=1;hit(0,6);
    unsigned before_writes=writes;tick();
    assert(hits==5 && writes==before_writes && jnet_character_instances[1].failed);
    invalid_binding=0;tick();assert(hits==6);
    jnet_sg_mail.records[1].variant=1;reject_switch=1;
    unsigned before=switches;tick();assert(switches==before+2 && !jnet_character_instances[1].binding.creature);
    float dissolve[2];memcpy(dissolve,actor_storage+0x1e00,4);memcpy(dissolve+1,actor_storage+0x2000,4);
    assert(dissolve[0]==.25f && dissolve[1]==.75f && actor_storage[0x9c34]==1 && actor_storage[0x9cc4]==1);
    float moved[3]={11,12,13};memcpy(actor_storage+0x10,moved,sizeof(moved));before_writes=writes;
    for(unsigned i=0;i<20;++i){++clock_ms;tick();}assert(switches==before+2 && writes==before_writes);
    assert(!memcmp(actor_storage+0x10,moved,sizeof(moved)));
    clock_ms+=1000;tick();assert(switches==before+3);
    reject_switch=0;clock_ms+=1000;jnet_sg_mail.records[1].seen=clock_ms;tick();
    assert(jnet_character_instances[1].initialized && jnet_character_instances[1].skin==1 && hits==6);
    puts("Remote creatures: both slots dedupe across skin/species switches, missing state and pause; lease reset, creature immunity, stale bindings/hits, visible fallback and failed-skin retry limit passed. Native actors mocked.");
}
