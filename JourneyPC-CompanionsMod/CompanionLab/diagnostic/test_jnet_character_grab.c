#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define JNET_CHARACTER_COUNT 7
#define JNET_CHARACTER_WAYFARER 0
#define JNET_CHARACTER_LONGTAIL 1
#define JNET_STEAM_CAPACITY 3
#define JC_ACTOR_RUNTIME_ACTIVE 2
struct jnet_character_state {uintptr_t grab_event,game;unsigned initialized,failed;struct {uintptr_t creature;} binding;unsigned grab_blocked;};
static struct jnet_character_state jnet_character_instances[3],jnet_character_caches[3][21];
struct jc_network_peer {struct {unsigned admitted;uint64_t peer,remote_epoch;} connection;};
static struct {unsigned paused;struct jc_network_peer peers[2];} traveler_network;
static struct {unsigned state;} actor={2};
static __typeof__(actor) *traveler_actor=&actor;
static unsigned traveler_local_state=2,jnet_travel_busy,jnet_current_level=1,jnet_character_generation=1;
static struct {struct {unsigned character;} applied;} jnet_menu_edit;
struct jnet_game_record {uint64_t id,lease,sequence,seen;unsigned character_known,character,grab;struct {unsigned chapter;} sample;};
static struct {uint64_t room;struct jnet_game_record records[3];} jnet_sg_mail;
static uintptr_t traveler_game=1,traveler_barn=2,fault_image;
static unsigned jnet_sg_lock,calls,pickups,drops;static uint64_t clock_ms=1000;
static uint64_t GetTickCount64(void){return clock_ms;}
static void AcquireSRWLockExclusive(unsigned *p){assert(!*p);*p=1;}
static void ReleaseSRWLockExclusive(unsigned *p){assert(*p);*p=0;}
static int own_read(void *ctx,uint64_t a,void *o,size_t n){(void)ctx;memcpy(o,(void *)(uintptr_t)a,n);return 1;}
static int jc_local_windows_write(uint64_t a,const void *p,size_t n){memcpy((void *)(uintptr_t)a,p,n);return 1;}
static int jnet_character_binding_valid(void *r,void *c,void *b,unsigned g){(void)r;(void)c;(void)b;return g==1;}
static int jnet_character_grab_ready(void *r,void *c,uintptr_t i,uintptr_t e,uintptr_t creature){(void)r;(void)c;(void)i;return e && creature;}
static void jnet_character_fail(struct jnet_character_state *s){s->failed=1;}
#include "character_grab_code.h"
static unsigned char event[0x100],clump[0x20];
static void native_grab(uintptr_t e,uintptr_t game){
    assert(game==traveler_game && !jnet_sg_lock);++calls;
    if(e!=(uintptr_t)event)return;
    uint64_t count=0,attached=0;memcpy(&count,clump+0x10,8);memcpy(&attached,event+0x90,8);
    if(count){attached=123;++pickups;}else{assert(!event[0x84]);assert(attached==123);attached=0;++drops;}
    memcpy(event+0x90,&attached,8);
}
static void hold(void){
    jnet_sg_mail.room=1;jnet_sg_mail.records[0]=(struct jnet_game_record){.id=10,.lease=11,.sequence=1,.seen=clock_ms,.character_known=1,.character=1,.grab=1,.sample={1}};
    jnet_character_grab((uintptr_t)event,traveler_game);
}
static void release_check(void){
    unsigned before=drops;jnet_character_grab((uintptr_t)event,traveler_game);assert(drops==before+1);
    uint64_t count=0;memcpy(&count,clump+0x10,8);assert(count==1 && event[0x84]==1);
    before=calls;jnet_character_grab((uintptr_t)event,traveler_game);assert(calls==before);
}
int main(void){
    uintptr_t ptr=(uintptr_t)clump;uint64_t count=1;
    memcpy(event+0x48,&ptr,8);memcpy(clump+0x10,&count,8);event[0x84]=1;
    jnet_character_grab_original=native_grab;
    jnet_character_instances[1]=(struct jnet_character_state){(uintptr_t)event,traveler_game,1,0,{123},0};
    traveler_network.peers[0].connection.admitted=1;traveler_network.peers[0].connection.peer=10;traveler_network.peers[0].connection.remote_epoch=11;
    hold();assert(pickups==1);
    jnet_character_instances[1].grab_blocked=1;release_check();
    unsigned held_pickups=pickups;hold();assert(pickups==held_pickups);
    jnet_sg_mail.records[0].grab=0;jnet_character_grab((uintptr_t)event,traveler_game);
    assert(!jnet_character_instances[1].grab_blocked);
    hold();assert(pickups==held_pickups+1);jnet_sg_mail.records[0].grab=0;release_check();
    hold();clock_ms+=500;release_check();
    hold();jnet_sg_mail.room=0;release_check();
    hold();traveler_network.paused=1;release_check();traveler_network.paused=0;
    hold();jnet_travel_busy=1;release_check();jnet_travel_busy=0;
    hold();jnet_menu_edit.applied.character=1;release_check();jnet_menu_edit.applied.character=0;
    hold();jnet_sg_mail.records[0].lease=12;release_check();
    hold();jnet_sg_mail.records[0].sample.chapter=2;release_check();
    hold();jnet_character_caches[1][1]=jnet_character_instances[1];memset(&jnet_character_instances[1],0,sizeof(jnet_character_instances[1]));release_check();
    unsigned before=calls;jnet_character_grab(3,traveler_game);assert(calls==before+1);
    puts("Grab adapter: held pickup, release, stale packet, departed lobby, pause, travel, skin change, lease, chapter, cached actor and stock passthrough passed with native-call fixture.");
}
