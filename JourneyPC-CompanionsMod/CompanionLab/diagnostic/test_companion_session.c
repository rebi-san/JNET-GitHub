#include "companion_session.h"
#include <assert.h>
#include <stdio.h>
#include <float.h>
_Static_assert(sizeof(float)==4 && FLT_RADIX==2 && FLT_MANT_DIG==24,"IEEE binary32 required");
static const unsigned char room[16]={1,7,9};
static struct jc_session fresh(void) {
    struct jc_session s; assert(!jc_session_init(&s,room,10,3,0)); return s;
}
static size_t packet(unsigned char *wire,unsigned type,uint64_t id,uint64_t lease,
                      uint64_t seq,uint64_t epoch,uint32_t chapter,float x) {
    struct jc_session_packet p={.type=type,.id=id,.lease=lease,.sequence=seq,
        .chapter_epoch=epoch,.chapter=chapter,.motion={{x,2,3},{4,5,6},1}};
    size_t size; assert(!jc_session_encode(room,&p,wire,JC_SESSION_STATE_BYTES,&size)); return size;
}
static void same_game_state(const struct jc_member *a,const struct jc_member *b) {
    struct jc_member x=*a,y=*b; x.rate_start=y.rate_start=0; x.attempts=y.attempts=0;
    assert(!memcmp(&x,&y,sizeof(x)));
}
static void rejection(struct jc_session *s,uint64_t id,uint64_t lease,unsigned char *wire,
                       size_t size,uint64_t now,int expected) {
    struct jc_member before=*jc_session_find(s,id);
    assert(jc_session_receive(s,id,lease,wire,size,now)==expected);
    same_game_state(&before,jc_session_find(s,id));
}
static void three_clients(void) {
    struct jc_session clients[3]; uint64_t lease[3][3]={{0}}; unsigned char wire[92];
    for (unsigned dst=0;dst<3;++dst) {
        assert(!jc_session_init(&clients[dst],room,100+dst,3,0));
        for (unsigned src=0;src<3;++src) if (src!=dst) {
            assert(!jc_session_join(&clients[dst],100+src,0,&lease[dst][src]));
            size_t n=packet(wire,JC_SESSION_TRANSITION,100+src,lease[dst][src],1,1,2,0);
            assert(!jc_session_receive(&clients[dst],100+src,lease[dst][src],wire,n,0));
        }
    }
    for (unsigned frame=0;frame<300;++frame) for (unsigned src=0;src<3;++src)
        for (unsigned dst=0;dst<3;++dst) if (src!=dst) {
            unsigned other=3-src-dst;
            struct jc_member before=*jc_session_find(&clients[dst],100+other);
            size_t n=packet(wire,JC_SESSION_STATE,100+src,lease[dst][src],frame+2,1,2,(float)(1000*src+frame));
            assert(!jc_session_receive(&clients[dst],100+src,lease[dst][src],wire,n,frame*20));
            assert(!memcmp(&before,jc_session_find(&clients[dst],100+other),sizeof(before)));
        }
    for (unsigned dst=0;dst<3;++dst) for (unsigned src=0;src<3;++src) if (src!=dst) {
        const struct jc_motion *m=jc_session_motion(&clients[dst],100+src,lease[dst][src],2);
        assert(m && m->position[0]==(float)(1000*src+299));
        assert(!jc_session_motion(&clients[dst],100+src,lease[dst][src],3));
    }
}
int main(void) {
    three_clients();
    struct jc_session s=fresh(); uint64_t a,b,c; unsigned char wire[93]; size_t n;
    assert(!jc_session_join(&s,20,0,&a)); assert(!jc_session_join(&s,30,0,&b));
    assert(jc_session_join(&s,40,0,&c)==JC_SESSION_FULL && !c);
    assert(jc_session_join(&s,20,0,&c)==JC_SESSION_MEMBER);
    assert(jc_session_join(&s,10,0,&c)==JC_SESSION_ARGUMENT);
    n=packet(wire,JC_SESSION_STATE,20,a,1,1,2,5);
    rejection(&s,20,a,wire,n,0,JC_SESSION_CHAPTER);
    n=packet(wire,JC_SESSION_TRANSITION,20,a,1,1,2,0);
    assert(!jc_session_receive(&s,20,a,wire,n,0));
    n=packet(wire,JC_SESSION_STATE,20,a,3,1,2,9);
    assert(!jc_session_receive(&s,20,a,wire,n,0));
    rejection(&s,20,a,wire,n,0,JC_SESSION_REPLAY);
    jc_session_put(wire+44,2,8); rejection(&s,20,a,wire,n,0,JC_SESSION_REPLAY);
    n=packet(wire,JC_SESSION_STATE,30,b,4,1,2,17);
    rejection(&s,20,a,wire,n,0,JC_SESSION_OWNER);
    n=packet(wire,JC_SESSION_STATE,20,a,4,1,2,17);
    wire[12]^=1; rejection(&s,20,a,wire,n,0,JC_SESSION_ROOM); wire[12]^=1;
    wire[4]=2; rejection(&s,20,a,wire,n,0,JC_SESSION_VERSION); wire[4]=1;
    wire[6]=9; rejection(&s,20,a,wire,n,0,JC_SESSION_FORMAT); wire[6]=2;
    wire[10]=1; rejection(&s,20,a,wire,n,0,JC_SESSION_FORMAT); wire[10]=0;
    wire[0]='X'; rejection(&s,20,a,wire,n,0,JC_SESSION_FORMAT); wire[0]='J';
    rejection(&s,20,a,wire,n+1,0,JC_SESSION_FORMAT);
    for (size_t length=0;length<n;++length) rejection(&s,20,a,wire,length,1000,JC_SESSION_FORMAT);
    jc_session_put(wire+64,0x7fc00000,4); rejection(&s,20,a,wire,n,2000,JC_SESSION_VALUE);
    jc_session_put(wire+64,0x7f800000,4); rejection(&s,20,a,wire,n,2000,JC_SESSION_VALUE);
    n=packet(wire,JC_SESSION_TRANSITION,20,a,4,2,3,0);
    assert(!jc_session_receive(&s,20,a,wire,n,2000));
    assert(!jc_session_find(&s,20)->has_state && !jc_session_motion(&s,20,a,3));
    n=packet(wire,JC_SESSION_STATE,20,a,5,1,2,100);
    rejection(&s,20,a,wire,n,2000,JC_SESSION_CHAPTER);
    n=packet(wire,JC_SESSION_STATE,20,a,5,2,3,100);
    assert(!jc_session_receive(&s,20,a,wire,n,2000));
    n=packet(wire,JC_SESSION_TRANSITION,20,a,6,3,2,0);
    assert(!jc_session_receive(&s,20,a,wire,n,2000));
    n=packet(wire,JC_SESSION_STATE,20,a,7,1,2,100);
    rejection(&s,20,a,wire,n,2000,JC_SESSION_CHAPTER);
    assert(jc_session_leave(&s,20,a+1)==JC_SESSION_MEMBER);
    assert(!jc_session_leave(&s,20,a)); assert(!jc_session_join(&s,20,2000,&c) && c!=a);
    assert(!jc_session_motion(&s,20,a,2));
    assert(jc_session_receive(&s,20,a,wire,n,2000)==JC_SESSION_MEMBER);
    rejection(&s,20,c,wire,n,2000,JC_SESSION_OWNER);
    assert(jc_session_receive(&s,20,c,wire,n,1999)==JC_SESSION_CLOCK);
    n=packet(wire,JC_SESSION_TRANSITION,20,c,1,1,2,0);
    assert(!jc_session_receive(&s,20,c,wire,n,3000));
    for (unsigned i=1;i<JC_SESSION_BURST;++i) rejection(&s,20,c,wire,n,3000,JC_SESSION_REPLAY);
    rejection(&s,20,c,wire,n,3000,JC_SESSION_RATE);
    uint64_t id,expired; assert(!jc_session_expire_one(&s,8000,5000,&id,&expired));
    assert(id==20 && expired==c); assert(!jc_session_find(&s,20));
    assert(!jc_session_expire_one(&s,8000,5000,&id,&expired) && id==30);
    assert(jc_session_expire_one(&s,8000,5000,&id,&expired)==JC_SESSION_MEMBER && !id && !expired);
    s.next_lease=UINT64_MAX; assert(jc_session_join(&s,20,8000,&a)==JC_SESSION_EXHAUSTED);
    s=fresh(); assert(!jc_session_join(&s,20,0,&a));
    n=packet(wire,JC_SESSION_TRANSITION,20,a,UINT64_MAX,1,2,0);
    assert(!jc_session_receive(&s,20,a,wire,n,0));
    n=packet(wire,JC_SESSION_STATE,20,a,1,1,2,0);
    rejection(&s,20,a,wire,n,0,JC_SESSION_REPLAY);
    /* All mutations of a valid packet must be bounded; rejection preserves gameplay state. */
    uint32_t random=17;
    for (unsigned iteration=0;iteration<10000;++iteration) {
        s=fresh(); assert(!jc_session_join(&s,20,0,&a));
        n=packet(wire,JC_SESSION_TRANSITION,20,a,1,1,2,0);
        assert(!jc_session_receive(&s,20,a,wire,n,0));
        n=packet(wire,JC_SESSION_STATE,20,a,2,1,2,5);
        random=random*1664525u+1013904223u; unsigned offset=random%92;
        random=random*1664525u+1013904223u; wire[offset]^=(unsigned char)(1u<<(random%8));
        struct jc_member before=*jc_session_find(&s,20);
        int code=jc_session_receive(&s,20,a,wire,n,0);
        if (code) same_game_state(&before,jc_session_find(&s,20));
        else assert(jc_motion_valid(&jc_session_find(&s,20)->motion));
    }
    puts("Session core: three simulated clients / 1800 independent state deliveries; rejection, reconnect, chapter, timeout and 10000 mutation checks passed. No sockets or game clients.");
}
