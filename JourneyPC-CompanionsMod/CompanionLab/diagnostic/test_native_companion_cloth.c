#include "native_companion_cloth.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

enum { GAME=0x100000, BARN=0x200000, PLAYERS=0x1000000, DUDE=0x1100000,
       TIMER=0x1200000, JOB=0x1210000, DEFINITION=0x1220000,
       COLLECTION=0x1300000, LINKS=0x1400000, MATERIALS=0x1500000 };
#define IMAGE UINT64_C(0x140000000)
static unsigned char game[0x1000],barn[0x6bd100],players[0x29000],dude[0x15000],timer[0x40],job[0x80],definition[0x20],collection[0x91b8],links[0x100],materials[0x2000];
struct segment { uint64_t at; unsigned char *data; size_t size; };
static const struct segment segments[]={
    {GAME,game,sizeof(game)},{BARN,barn,sizeof(barn)},{PLAYERS,players,sizeof(players)},
    {DUDE,dude,sizeof(dude)},{TIMER,timer,sizeof(timer)},{JOB,job,sizeof(job)},
    {DEFINITION,definition,sizeof(definition)},{COLLECTION,collection,sizeof(collection)},
    {LINKS,links,sizeof(links)},{MATERIALS,materials,sizeof(materials)}
};
static struct jc_cloth_lease lease;
static unsigned calls[11],writes,order_count; static char order[4096];
static int bad_fingerprint,failed_operation,partial_init;
static uint64_t failed_read,failed_write;
static void put64(unsigned char *p,size_t offset,uint64_t n) { memcpy(p+offset,&n,8); }
static void put32(unsigned char *p,size_t offset,uint32_t n) { memcpy(p+offset,&n,4); }
static void putfloat(unsigned char *p,size_t offset,float n) { memcpy(p+offset,&n,4); }
static unsigned char *resolve(uint64_t at,size_t bytes) {
    for (unsigned i=0;i<sizeof(segments)/sizeof(*segments);++i)
        if (at>=segments[i].at && at-segments[i].at<=segments[i].size && bytes<=segments[i].size-(size_t)(at-segments[i].at))
            return segments[i].data+(size_t)(at-segments[i].at);
    return NULL;
}
static int read_memory(void *ctx,uint64_t at,void *out,size_t bytes) {
    (void)ctx; if (failed_read && at<=failed_read && failed_read-at<bytes) return 0;
    for (unsigned i=0;i<sizeof(jc_cloth_entries)/sizeof(*jc_cloth_entries);++i)
        if (at==IMAGE+jc_cloth_entries[i].rva && bytes==16) {
            memcpy(out,jc_cloth_entries[i].bytes,16); if (bad_fingerprint) ((unsigned char *)out)[0]^=1; return 1;
        }
    if (at==IMAGE+0x731fb0 && bytes==4) { uint32_t maximum=40; memcpy(out,&maximum,4); return 1; }
    unsigned char *p=resolve(at,bytes); if (!p) return 0; memcpy(out,p,bytes); return 1;
}
static int write_memory(void *ctx,uint64_t at,const void *value,size_t bytes) {
    (void)ctx; if (at==failed_write) return 0;
    unsigned char *p=resolve(at,bytes); assert(p && ((at>=DUDE && at+bytes<=DUDE+sizeof(dude)) ||
        (lease.state==JC_CLOTH_LIVE && at>=BARN+0x80 && at+bytes<=BARN+0x80+2*0xd00)));
    memcpy(p,value,bytes); ++writes; return 1;
}
static int native_call(void *ctx,unsigned op,uint64_t a,uint64_t b,uint64_t c) {
    (void)ctx; assert((op>=1 && op<=5) || op==JC_CLOTH_ATLAS); assert(b==BARN);
    assert(a==(op==JC_CLOTH_RELEASE?DUDE+0x9b38:DUDE));
    assert(op==JC_CLOTH_RELEASE?c==0:c==TIMER);
    ++calls[op]; assert(order_count+1<sizeof(order));order[order_count++]=op==JC_CLOTH_ATLAS?'A':"?IUCSR"[op];order[order_count]=0;
    if ((int)op==failed_operation) return 0;
    if (op==JC_CLOTH_INIT) {
        assert(dude[0x81]==1);put64(dude,0x9b40,COLLECTION);put32(collection,0x91b0,partial_init?1:2);
        for (unsigned i=0;i<2;++i) {
            put64(dude,0x9b58+i*16,COLLECTION+i*0x20);put64(dude,0x9b60+i*16,COLLECTION+0xc0+i*0x1828);
            put64(collection,i*0x20,LINKS+i*0x40);put64(collection,i*0x20+0x18,BARN);
            put64(links,i*0x40+0x10,LINKS+i*0x40+0x20);put64(links,i*0x40+0x20+0x18,BARN+0x80+i*0xd00);
            put32(barn,0x270118+i*0xe0,i==0?1:2);
        }
    }
    if (op==JC_CLOTH_ATTACH) {put32(barn,0x270118,1);put32(barn,0x270118+0xe0,1);put32(barn,0x80+0xc94,5);put32(barn,0x80+0xd00+0xc94,4);}
    if (op==JC_CLOTH_RELEASE) {
        put64(dude,0x9b40,0);put64(dude,0x9b48,0);
        put32(barn,0x270118,3);put32(barn,0x270118+0xe0,3);
    }
    return 1;
}
static void reset(void) {
    for (unsigned i=0;i<sizeof(segments)/sizeof(*segments);++i) memset(segments[i].data,0,segments[i].size);
    memset(&lease,0,sizeof(lease));memset(calls,0,sizeof(calls));writes=order_count=0;order[0]=0;
    bad_fingerprint=failed_operation=partial_init=0;failed_read=failed_write=0;
    put64(game,0x108,BARN);put64(game,0x1e0,PLAYERS);put64(game,0x68,TIMER);
    put64(players,0,IMAGE+0x5da340);put64(dude,0,IMAGE+0x5d0380);put64(barn,0x6bcef0,JOB);
    put32(definition,0x10,2);putfloat(timer,0x10,.016f);
    put32(players,0xb0+0x6c,4);putfloat(players,0xb0+0x144c,.5f);
    for (unsigned i=0;i<4;++i) putfloat(dude,0x690+i*20,1.f);
    putfloat(dude,0x690+48,3.f);
    const uint64_t material_objects[]={DUDE+0x1e0,BARN+0x666ee8+4*0x630,BARN+0x666ee8+5*0x630,
        PLAYERS+0x290,PLAYERS+0x148d0,BARN+0x666ee8+2*0x630,BARN+0x666ee8+3*0x630};
    for (unsigned i=0;i<7;++i) {
        uint64_t base=MATERIALS+i*0x200;unsigned char *p=materials+i*0x200;
        put64(resolve(material_objects[i]+0x30,8),0,base);put64(p,0x10,base+0x20);
        put64(p,0x38,base+0x40);put64(p,0xc0,base+0xe0);put32(p,0xe0,1);
        put64(p,0xe8,base+0xf0);put64(p,0xf0,base+0xf8);put64(p,0xf8,IMAGE+0x1000);
    }
    put64(barn,0x646d30,MATERIALS+0x1000);put64(barn,0x646d38,MATERIALS+0x1800);
    put64(materials,0x1070,IMAGE+0x2000);put32(materials,0x1800,2);
}
static int step(int phase,int want) {
    return jc_companion_cloth_step(read_memory,write_memory,native_call,0,IMAGE,GAME,DUDE,DEFINITION,7,phase,want,&lease);
}
static void no_native(void) {for (unsigned i=1;i<=10;++i) assert(!calls[i]);}
int main(void) {
    for(unsigned capacity=0;capacity<=40;++capacity) {
        unsigned expected=(unsigned)((float)capacity/40.f*73.f+1.5f);
        if(expected<2)expected=2;
        if(expected>73)expected=73;
        put32(dude,0x6c,capacity);put32(dude,0x1460,expected);
        dude[0x9ba0]=0;unsigned before=writes;
        assert(!jc_cloth_scarf_resize(read_memory,write_memory,NULL,IMAGE,DUDE));
        assert(!dude[0x9ba0] && writes==before);
        put32(dude,0x1460,expected==2?73:2);
        assert(!jc_cloth_scarf_resize(read_memory,write_memory,NULL,IMAGE,DUDE));
        assert(dude[0x9ba0]==1 && writes==before+1);
        assert(!jc_cloth_scarf_resize(read_memory,write_memory,NULL,IMAGE,DUDE));
        assert(writes==before+1);
    }
    failed_write=DUDE+0x9ba0;dude[0x9ba0]=0;
    assert(jc_cloth_scarf_resize(read_memory,write_memory,NULL,IMAGE,DUDE)==JC_CLOTH_CALL);
    failed_write=0;failed_read=DUDE+0x6c;
    assert(jc_cloth_scarf_resize(read_memory,write_memory,NULL,IMAGE,DUDE)==JC_CLOTH_READ);
    failed_read=0;

    reset();assert(step(0,1)==JC_CLOTH_PHASE);no_native();assert(!writes);
    reset();barn[0x6bcf84]=1;assert(step(1,1)==JC_CLOTH_BUSY);no_native();assert(!writes);
    reset();job[0x58]=1;assert(step(1,1)==JC_CLOTH_BUSY);no_native();assert(!writes);
    reset();players[0xa4]=1;assert(step(1,1)==JC_CLOTH_PEER);no_native();assert(!writes);
    reset();put32(barn,0x270118+10*0xe0,3);put32(barn,0x80+10*0xd00+0xc94,5);
    assert(step(1,1)==JC_CLOTH_PEER);no_native();assert(!writes);
    reset();put32(barn,0x270118+10*0xe0,2);put32(barn,0x80+10*0xd00+0xc94,6);
    assert(!step(1,1));assert(lease.free_slots==767 && lease.updates==1);
    reset();put32(barn,0x270118+10*0xe0,2);put32(barn,0x80+10*0xd00+0xc94,4);
    assert(step(1,1)==JC_CLOTH_PEER);no_native();assert(!writes);
    reset();put32(barn,0x270118+10*0xe0,4);
    assert(step(1,1)==JC_CLOTH_OWNERSHIP && lease.stage==6);no_native();assert(!writes);
    reset();for(unsigned i=0;i<767;++i)put32(barn,0x270118+i*0xe0,1);
    assert(step(1,1)==JC_CLOTH_CAPACITY);no_native();assert(!writes);
    reset();put32(definition,0x10,6);assert(step(1,1)==JC_CLOTH_OWNERSHIP);no_native();assert(!writes);
    reset();bad_fingerprint=1;assert(step(1,1)==JC_CLOTH_IDENTITY);no_native();assert(!writes);
    reset();failed_read=BARN+0x270118+767*0xe0;assert(step(1,1)==JC_CLOTH_READ);no_native();assert(!writes);
    reset();putfloat(players,0xb0+0x2000,NAN);assert(step(1,1)==JC_CLOTH_PROPERTIES);no_native();assert(!writes);
    reset();put32(players,0xb0+0x68,5);assert(step(1,1)==JC_CLOTH_PROPERTIES);no_native();assert(!writes);
    reset();put32(players,0xb0+0x6c,4097);assert(step(1,1)==JC_CLOTH_PROPERTIES);no_native();assert(!writes);
    reset();put64(dude,0x860,LINKS);assert(step(1,1)==JC_CLOTH_OWNERSHIP);no_native();assert(!writes);
    reset();dude[0x180]=dude[0x1479]=1;assert(step(1,1)==JC_CLOTH_PROPERTIES);no_native();assert(!writes);
    reset();put64(materials,0x1070,0);assert(step(1,1)==JC_CLOTH_PROPERTIES);no_native();assert(!writes);
    reset();put64(barn,0x666ee8+4*0x630+0x30,MATERIALS);assert(step(1,1)==JC_CLOTH_OWNERSHIP);no_native();assert(!writes);
    reset();assert(!step(1,1));assert(lease.state==JC_CLOTH_LIVE && lease.updates==1 && lease.free_slots==768);
    assert(!strcmp(order,"IUCAS"));assert(lease.slot[0]==0 && lease.slot[1]==1 && dude[0x81]==1);
    float position;memcpy(&position,dude+0x10,4);assert(position==3.f);
    assert(!step(1,1));assert(calls[JC_CLOTH_INIT]==1 && calls[JC_CLOTH_ATTACH]==2 && lease.updates==2 && lease.free_slots==766);
    put32(barn,0x270118+0xe0,2);
    assert(jc_cloth_capture(read_memory,0,&lease,1)==JC_CLOTH_OWNERSHIP && lease.capture_detail==13);
    assert(!jc_cloth_capture(read_memory,0,&lease,0));
    put32(barn,0x270118+0xe0,3);
    assert(jc_cloth_capture(read_memory,0,&lease,0)==JC_CLOTH_OWNERSHIP);
    put32(barn,0x270118+0xe0,1);
    unsigned before=writes;players[0xa2]=1;put32(barn,0x270118+2*0xe0,1);put32(barn,0x80+2*0xd00+0xc94,4);
    assert(step(1,1)==JC_CLOTH_PEER);assert(lease.state==JC_CLOTH_RETIRED && calls[JC_CLOTH_RELEASE]==1);
    assert(calls[JC_CLOTH_APPEARANCE]==2 && writes==before+2 && dude[0x81]==0);
    uint32_t native_state;memcpy(&native_state,barn+0x270118+2*0xe0,4);assert(native_state==1);
    assert(!step(1,0));assert(calls[JC_CLOTH_RELEASE]==1);
    reset();assert(!step(1,1));put64(collection,0x18,BARN+0x10);
    assert(step(1,0)==JC_CLOTH_OWNERSHIP && !calls[JC_CLOTH_RELEASE]);
    reset();assert(!step(1,1));put64(game,0x108,BARN+0x100);
    assert(step(1,0)==JC_CLOTH_CHANGED && !calls[JC_CLOTH_RELEASE]);
    reset();assert(!step(1,1));assert(jc_companion_cloth_retire(read_memory,write_memory,native_call,0,GAME,8,1,&lease)==JC_CLOTH_OWNERSHIP);assert(!calls[JC_CLOTH_RELEASE]);
    reset();partial_init=1;assert(step(1,1)==JC_CLOTH_OWNERSHIP);assert(lease.state==JC_CLOTH_QUARANTINED && !calls[JC_CLOTH_ATTACH] && !calls[JC_CLOTH_RELEASE]);
    reset();failed_operation=JC_CLOTH_COLLIDE;assert(step(1,1)==JC_CLOTH_CALL);assert(lease.state==JC_CLOTH_RETIRED && lease.failure_stage==JC_CLOTH_COLLIDE && calls[JC_CLOTH_RELEASE]==1 && !calls[JC_CLOTH_APPEARANCE]);
    reset();assert(!step(1,1));failed_write=DUDE+0x9b58;assert(step(1,0)==JC_CLOTH_CALL);assert(lease.state==JC_CLOTH_QUARANTINED);
    reset();put32(players,0xb0+0x70,6);put32(players,0xb0+0x74,20);
    put32(players,0xb0+0x68,20);put32(players,0xb0+0x6c,40);
    assert(!step(1,1));assert(lease.atlas_ready && lease.atlas_outfit==6);
    uint32_t own_symbol;memcpy(&own_symbol,dude+0x74,4);assert(own_symbol==20);
    for (unsigned i=0;i<120;++i) assert(!jc_cloth_scarf_step(read_memory,write_memory,0,&lease,TIMER));
    float cutoff,glow;memcpy(&cutoff,barn+0x80+0xc90,4);memcpy(&glow,barn+0x80+0xc64,4);
    assert(cutoff>0 && cutoff<.5f && glow>.2f && glow<.4f && barn[0x80+0xc62]==1);
    uint32_t mode[3];memcpy(mode,barn+0x80+0xcb8,sizeof(mode));
    assert(mode[0]==3 && mode[1]==3 && !mode[2]);
    assert(!step(1,1) && calls[JC_CLOTH_ATLAS]==1);
    put32(players,0xb0+0x70,2);assert(!step(1,1) && calls[JC_CLOTH_ATLAS]==2 && lease.atlas_outfit==2);
    memcpy(mode,barn+0x80+0xcb8,sizeof(mode));assert(!mode[0] && !mode[1] && !mode[2]);
    put32(barn,0x80+0xc94,3);before=writes;
    assert(jc_cloth_scarf_step(read_memory,write_memory,0,&lease,TIMER)==JC_CLOTH_OWNERSHIP && writes==before);
    reset();failed_operation=JC_CLOTH_ATLAS;assert(step(1,1)==JC_CLOTH_CALL && lease.failure_stage==10 && lease.state==JC_CLOTH_RETIRED);
    reset();failed_write=BARN+0x80+0xc90;assert(step(1,1)==JC_CLOTH_CALL && lease.failure_stage==11 && lease.state==JC_CLOTH_RETIRED);
    reset();put32(players,0xb0+0x70,7);assert(step(1,1)==JC_CLOTH_PROPERTIES);no_native();
    reset();put32(players,0xb0+0x74,21);assert(step(1,1)==JC_CLOTH_PROPERTIES);no_native();
    const char *names[2][3];
    for (unsigned i=0;i<7;++i) { assert(jc_cloth_atlas_names(i,names)); assert(strstr(names[0][0],i<4?"Red":"White")); assert(!strcmp(names[1][0],"ClothPlayerScarf")); }
    assert(!jc_cloth_atlas_names(7,names));
    puts("native companion cloth: scarf charge/length, per-participant outfit/symbol, atlas changes and isolated descriptor failures passed");
    puts("native companion cloth: phase, capacity, peer handoff, ownership, native sequencing and retirement scenarios passed");
    return 0;
}
