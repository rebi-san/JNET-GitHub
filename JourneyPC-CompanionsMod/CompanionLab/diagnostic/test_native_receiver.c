#include "native_receiver.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned calls;
static void inspect(void *context,void *barn,void *game) {
    struct jc_receiver *r=context; uintptr_t actor,time;
    assert(barn==r->image);
    memcpy(&actor,r->image+0x68,8); memcpy(&time,(char *)game+0x68,8);
    assert(actor==(uintptr_t)(r->image+JC_RECEIVER_DUDE));
    assert(jc_receiver_float((unsigned char *)time+0xc)>0);
    ++calls;
}
static struct jc_receiver_state sample(unsigned outfit,unsigned symbol) {
    struct jc_receiver_state s={0}; uint32_t a[4]={2,5,outfit,symbol};
    memcpy(s.bytes+0x58,a,sizeof(a)); return s;
}
int main(void) {
    struct jc_receiver *a=calloc(1,sizeof(*a)),*b=calloc(1,sizeof(*b));
    struct jc_receiver *before=malloc(sizeof(*a));
    assert(a && b && before);
    struct jc_receiver_state sa=sample(6,20),sb=sample(1,3);
    struct jc_receiver_frame f={0}; f.current=sample(0,7); f.dt=1.0f/60;
    struct jc_receiver_output out;
    assert(jc_receiver_replace(a,1,10)==0 && jc_receiver_replace(b,2,11)==0);
    assert(jc_receiver_step(a,&f,inspect,a,&out)==JC_RECEIVER_EMPTY);
    assert(jc_receiver_receive(a,1,10,1,&sa)==0);
    assert(jc_receiver_receive(b,2,11,1,&sb)==0);
    assert(jc_receiver_step(a,&f,inspect,a,&out)==0);
    assert(out.sender_appearance[2]==6 && out.sender_appearance[3]==20);
    assert(jc_receiver_step(b,&f,inspect,b,&out)==0);
    assert(out.sender_appearance[2]==1 && out.sender_appearance[3]==3);
    *before=*a;
    assert(jc_receiver_receive(a,1,10,1,&sa)==JC_RECEIVER_REPLAY);
    assert(jc_receiver_receive(a,2,10,2,&sa)==JC_RECEIVER_SESSION);
    assert(jc_receiver_receive(a,1,9,2,&sa)==JC_RECEIVER_SESSION);
    assert(memcmp(before,a,sizeof(*a))==0);
    for (unsigned i=0;i<0x40;i+=4) {
        struct jc_receiver_state bad=sa; jc_receiver_store(bad.bytes+i,NAN);
        assert(jc_receiver_receive(a,1,10,2,&bad)==JC_RECEIVER_VALUE);
        assert(memcmp(before,a,sizeof(*a))==0);
    }
    for (unsigned i=0x4c;i<=0x54;i+=4) {
        struct jc_receiver_state bad=sa; jc_receiver_store(bad.bytes+i,INFINITY);
        assert(jc_receiver_receive(a,1,10,2,&bad)==JC_RECEIVER_VALUE);
    }
    for (unsigned i=0;i<4;++i) {
        struct jc_receiver_state bad=sa; uint32_t value=UINT32_MAX;
        memcpy(bad.bytes+0x58+4*i,&value,4);
        assert(jc_receiver_receive(a,1,10,2,&bad)==JC_RECEIVER_VALUE);
    }
    f.dt=NAN; assert(jc_receiver_step(a,&f,inspect,a,&out)==JC_RECEIVER_VALUE);
    f.dt=0; assert(jc_receiver_step(a,&f,inspect,a,&out)==JC_RECEIVER_VALUE);
    f.dt=2; assert(jc_receiver_step(a,&f,inspect,a,&out)==JC_RECEIVER_VALUE);
    before->evaluated=0;
    assert(memcmp(before,a,sizeof(*a))==0 && calls==2);
    assert(jc_receiver_depart(a,1,10)==0);
    assert(jc_receiver_receive(a,1,10,2,&sa)==JC_RECEIVER_SESSION);
    assert(jc_receiver_replace(a,3,10)==JC_RECEIVER_SESSION);
    assert(jc_receiver_replace(a,3,12)==0);
    assert(jc_receiver_receive(a,1,10,3,&sa)==JC_RECEIVER_SESSION);
    assert(a->sequence==0 && a->first && !a->received);
    assert(jc_receiver_receive(a,3,12,1,&sb)==0);
    assert(b->peer==2 && b->generation==11 && b->sequence==1);
    unsigned char storage[JC_RECEIVER_ACTOR_BYTES+2];
    unsigned char *actor=storage+1;
    memset(storage,0xa5,sizeof(storage)); f.dt=1.0f/60;
    memcpy(actor+0x10,f.current.bytes,JC_RECEIVER_STATE_BYTES);
    memcpy(actor+0x1468,&f.chirp_cooldown,4);
    assert(jc_receiver_project(a,&f,actor,JC_RECEIVER_ACTOR_BYTES)==JC_RECEIVER_EMPTY);
    assert(jc_receiver_step(a,&f,inspect,a,&out)==0);
    assert(jc_receiver_project(a,&f,actor,JC_RECEIVER_ACTOR_BYTES-1)==JC_RECEIVER_ARGUMENT);
    assert(jc_receiver_project(a,&f,actor,JC_RECEIVER_ACTOR_BYTES)==0);
    assert(jc_receiver_project(a,&f,actor,JC_RECEIVER_ACTOR_BYTES)==JC_RECEIVER_EMPTY);
    assert(storage[0]==0xa5 && storage[sizeof(storage)-1]==0xa5 && actor[0x1800]==0xa5);
    assert(jc_receiver_step(a,&f,inspect,a,&out)==0);
    a->image[JC_RECEIVER_DUDE+0x147a]=1;
    memset(a->image+JC_RECEIVER_DUDE+0x17f0,0xcc,0x3b4);
    assert(jc_receiver_project(a,&f,actor,JC_RECEIVER_ACTOR_BYTES)==0);
    assert(actor[0x17f0]==0xcc && actor[0x17f8]==0xa5 && actor[0x1961]==0xa5);
    assert(actor[0x1a2c]==0xa5 && actor[0x1ba0]==0xcc);
    assert(storage[0]==0xa5 && storage[sizeof(storage)-1]==0xa5);
    free(before); free(b); free(a);
    puts("native receiver: private staging, sender appearance, invalid input, replay/replacement and bounded single-use actor projection passed");
    return 0;
}
