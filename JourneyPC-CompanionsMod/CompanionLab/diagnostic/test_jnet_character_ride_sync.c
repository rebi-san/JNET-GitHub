#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "jnet_character_native.h"
#define JNET_STEAM_CAPACITY 3
struct jnet_game_record {
    uint64_t id,lease,sequence,seen,carrier,carrier_lease;
    unsigned character,character_known,grab;float ride_offset[3];
    struct {unsigned chapter;struct {unsigned char bytes[108];} state;} sample;
};
struct jnet_character_state {unsigned initialized,failed,kind,grab_blocked,ride_anchor_valid;float ride_anchor[3];uintptr_t game,grab_event;struct jnet_character_binding binding;};
static struct jnet_character_state jnet_character_instances[3];
struct jc_network_peer {struct {unsigned admitted;uint64_t peer,remote_epoch;} connection;};
static struct {unsigned paused;struct jc_network_peer peers[2];} traveler_network;
static unsigned jnet_travel_busy,jnet_grab_down=1,jnet_character_generation=1;
static uintptr_t traveler_game=123;
static unsigned char local_actor[512];
static uintptr_t traveler_barn;
static struct {uint64_t room,identity;struct jnet_game_record records[3];} jnet_sg_mail;
static unsigned jnet_sg_lock;
static uint64_t GetTickCount64(void){return 1000;}
static void AcquireSRWLockExclusive(unsigned *p){assert(!*p);*p=1;}
static void ReleaseSRWLockExclusive(unsigned *p){assert(*p);*p=0;}
static int own_read(void *ctx,uint64_t at,void *out,size_t n){(void)ctx;if(!at)return 0;memcpy(out,(void *)(uintptr_t)at,n);return 1;}
#include "character_ride_code.h"
#include "character_authority_code.h"
static unsigned authority_calls;
static unsigned char authority(uintptr_t creature,uintptr_t barn){assert(creature && barn==123);++authority_calls;return 1;}
int main(void){
    traveler_barn=(uintptr_t)local_actor;
    unsigned char creature[0x200]={0},target[0x50]={0},head[0x50]={0},clump[0x20]={0},event[0x100]={0};
    uint64_t type=0x5ce669,ptr=(uintptr_t)target,items[1]={(uintptr_t)head},count=1;
    memcpy(creature,&type,8);memcpy(creature+0x10,&ptr,8);ptr=(uintptr_t)clump;memcpy(creature+0x18,&ptr,8);
    ptr=(uintptr_t)items;memcpy(clump+8,&ptr,8);memcpy(clump+0x10,&count,8);
    float position[3]={10,20,30},q[4]={0,0,0,1};memcpy(head+0x10,position,12);memcpy(head+0x20,q,16);
    ptr=(uintptr_t)creature;memcpy(event+0x90,&ptr,8);
    struct jnet_character_state holder={.initialized=1,.kind=JNET_CHARACTER_LONGTAIL,.game=123,.grab_event=(uintptr_t)event,
        .binding={.image=1,.generation=1,.creature=(uintptr_t)creature,.target=(uintptr_t)target}};
    jnet_character_instances[1]=holder;
    traveler_network.peers[0].connection.admitted=1;traveler_network.peers[0].connection.peer=10;traveler_network.peers[0].connection.remote_epoch=12;
    struct jnet_game_record rider={.id=20,.lease=13,.sequence=1,.seen=1000,.character_known=1,.character=JNET_CHARACTER_WAYFARER,.sample={.chapter=1}};
    float riding[3]={11,22,30};memcpy(rider.sample.state.bytes,riding,12);
    jnet_character_ride_capture(&rider);assert(rider.carrier==10 && rider.carrier_lease==12);
    assert(rider.ride_offset[0]==1 && rider.ride_offset[1]==2 && rider.ride_offset[2]==0);
    jnet_sg_mail.room=1;jnet_sg_mail.identity=10;
    jnet_sg_mail.records[0]=(struct jnet_game_record){.id=10,.lease=12,.sequence=1,.seen=1000,.character_known=1,.character=JNET_CHARACTER_LONGTAIL,.grab=1,.sample={.chapter=1}};
    jnet_character_instances[0]=holder;
    position[0]=100;q[1]=q[3]=.70710678f;memcpy(head+0x10,position,12);memcpy(head+0x20,q,16);
    float resolved[3];assert(jnet_character_ride_resolve(&rider,resolved));
    assert(fabsf(resolved[0]-100)<.00001f && fabsf(resolved[1]-22)<.00001f && fabsf(resolved[2]-29)<.00001f);
    jnet_sg_mail.identity=30;assert(jnet_character_ride_resolve(&rider,resolved));
    position[1]=150;memcpy(head+0x10,position,12);
    float native_rider[3]={102,153,29};memcpy(rider.sample.state.bytes,native_rider,12);
    memset(local_actor,0x5a,sizeof(local_actor));
    jnet_character_ride_capture(&rider);
    assert(fabsf(rider.ride_offset[0]-1)<.00001f && fabsf(rider.ride_offset[1]-3)<.00001f && fabsf(rider.ride_offset[2]-2)<.00001f);
    assert(jnet_character_ride_resolve(&rider,resolved));
    for(unsigned k=0;k<3;++k)assert(fabsf(resolved[k]-native_rider[k])<.00001f);
    assert(!memcmp(rider.sample.state.bytes,native_rider,12));
    for(unsigned k=0;k<sizeof(local_actor);++k)assert(local_actor[k]==0x5a);
    rider.carrier_lease=99;assert(!jnet_character_ride_resolve(&rider,resolved));rider.carrier_lease=12;
    jnet_sg_mail.records[0].grab=0;assert(!jnet_character_ride_resolve(&rider,resolved));jnet_sg_mail.records[0].grab=1;
    rider.seen=500;assert(!jnet_character_ride_resolve(&rider,resolved));rider.seen=1000;
    jnet_sg_mail.records[0].sample.chapter=2;assert(!jnet_character_ride_resolve(&rider,resolved));jnet_sg_mail.records[0].sample.chapter=1;
    jnet_sg_mail.room=0;assert(!jnet_character_ride_resolve(&rider,resolved));
    memset(event+0x90,0,8);jnet_character_ride_capture(&rider);
    assert(!rider.carrier && !rider.carrier_lease && !rider.ride_offset[0] && !rider.ride_offset[1] && !rider.ride_offset[2]);
    jnet_character_authority_original=authority;
    assert(jnet_character_authority(1,123)==1 && authority_calls==1);
    jnet_character_attack_scope=1;
    assert(jnet_character_authority(1,123)==0 && authority_calls==1);
    assert(jnet_character_authority(2,123)==1 && authority_calls==2);
    jnet_character_attack_scope=0;assert(jnet_character_authority(1,123)==1 && authority_calls==3);
    puts("Ride synchronization: capture, carrier and observer rotation, stale/lease/chapter/release guards; scoped native attack authority passed.");
}
