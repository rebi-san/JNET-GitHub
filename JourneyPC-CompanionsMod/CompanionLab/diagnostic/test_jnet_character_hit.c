#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
struct jnet_war_hit {uint64_t victim,lease,sequence,seen;};
struct jnet_character_state {unsigned owner;uint64_t attack_target,attack_target_lease,attack_actor;struct jnet_war_hit war_hits[2];};
struct jc_network_peer {struct {unsigned admitted;uint64_t peer,remote_epoch;} connection;};
static struct {struct jc_network_peer peers[2];unsigned paused;} traveler_network;
static uintptr_t traveler_game=1,traveler_object=200,traveler_barn=400;
static unsigned jnet_travel_busy,traveler_local_state=2,binding_valid=1;
#define JC_ACTOR_RUNTIME_ACTIVE 2
static struct {unsigned state;} actor={2},*traveler_actor=&actor;
static struct jnet_character_state state;
static unsigned active=1,stock_hits,stock_collisions;
static uint64_t last_actor;
static uint64_t GetTickCount64(void){return 1000;}
static struct jnet_character_state *jnet_character_attack_instance(uintptr_t event,unsigned handler,unsigned *on){assert(!handler);*on=active;return event==100?&state:0;}
static int jnet_character_attack_active(struct jnet_character_state *instance,uintptr_t game,unsigned on){assert(game==1);return on && !instance->owner && binding_valid;}
#include "character_hit_code.h"
static void hit(uintptr_t event,uintptr_t dude,uintptr_t a,uintptr_t b,uintptr_t events){(void)event;(void)a;(void)b;(void)events;++stock_hits;last_actor=dude;}
static void collision(uintptr_t event,uintptr_t dude,uintptr_t other,uintptr_t camera,uintptr_t events,uintptr_t sounds){
    assert(!other);(void)camera;(void)sounds;++stock_collisions;last_actor=dude;
    jnet_character_hit(event,dude,0,0,events);
}
int main(void){
    jnet_character_hit_original=hit;jnet_character_collision_original=collision;
    traveler_network.peers[0].connection=(__typeof__(traveler_network.peers[0].connection)){1,43,7};
    traveler_network.peers[1].connection=(__typeof__(traveler_network.peers[1].connection)){1,44,8};
    state.attack_target=43;state.attack_target_lease=7;state.attack_actor=200;
    jnet_character_collision(100,999,888,0,0,0);
    assert(last_actor==200 && stock_hits==0 && state.war_hits[0].victim==43 && state.war_hits[0].sequence==1);
    state.attack_target=44;state.attack_target_lease=8;state.attack_actor=traveler_barn+0x146f0;
    jnet_character_collision(100,999,888,0,0,0);
    assert(last_actor==state.attack_actor && stock_hits==0 && state.war_hits[1].victim==44 && state.war_hits[1].sequence==2);
    traveler_local_state=0;jnet_character_collision(100,999,888,0,0,0);assert(stock_collisions==2);traveler_local_state=2;
    state.attack_target=43;state.attack_target_lease=7;state.attack_actor=200;
    jnet_character_collision(100,999,888,0,0,0);
    assert(state.war_hits[0].sequence==3 && stock_collisions==3);
    actor.state=0;jnet_character_collision(100,999,888,0,0,0);assert(stock_collisions==3);actor.state=2;
    traveler_object=201;jnet_character_collision(100,999,888,0,0,0);assert(stock_collisions==3);traveler_object=200;
    traveler_actor=0;jnet_character_collision(100,999,888,0,0,0);assert(stock_collisions==3);traveler_actor=&actor;
    binding_valid=0;jnet_character_collision(100,999,888,0,0,0);assert(stock_collisions==3);binding_valid=1;
    jnet_travel_busy=1;jnet_character_collision(100,999,888,0,0,0);assert(stock_collisions==3);jnet_travel_busy=0;
    traveler_network.paused=1;jnet_character_collision(100,999,888,0,0,0);assert(stock_collisions==3);traveler_network.paused=0;
    jnet_character_hit(100,999,0,0,0);assert(state.war_hits[0].sequence==3 && stock_hits==0);
    state.owner=1;jnet_character_collision(100,999,888,0,0,0);assert(stock_collisions==3);
    state.owner=0;state.attack_target_lease=9;jnet_character_collision(100,999,888,0,0,0);assert(stock_collisions==3);
    state.attack_target_lease=7;active=0;jnet_character_collision(100,999,888,0,0,0);assert(stock_collisions==3);
    active=1;
    struct jnet_character_state cached=state;
    memset(&state,0,sizeof(state));
    state.attack_target=43;state.attack_target_lease=7;state.attack_actor=200;
    jnet_character_collision(100,999,888,0,0,0);
    assert(state.war_hits[0].sequence==4 && stock_collisions==4);
    state=cached;jnet_character_collision(100,999,888,0,0,0);
    assert(state.war_hits[0].sequence==5 && stock_collisions==5);
    jnet_character_hit(101,999,0,0,0);assert(stock_hits==1 && last_actor==999);
    puts("War hit routing: both remote slots, target and skin switching, monotonic cached-skin sequences, hidden local exclusion, stale leases, inactive and remote owners, stock passthrough passed.");
}
