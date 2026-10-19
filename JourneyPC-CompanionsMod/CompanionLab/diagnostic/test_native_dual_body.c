#include "native_dual_body.h"
#include <assert.h>
#include <stdio.h>
static struct jc_initialized_lifetime life[2];
static struct jc_initialized_owner now[2];
static void fixture(void) {
    memset(life,0,sizeof(life)); memset(now,0,sizeof(now));
    const uint64_t objects[]={0x200000,0x240000};
    for (unsigned i=0;i<2;++i) {
        struct jc_initialized_owner *o=&now[i];
        o->expected_slots=4; o->expected_occupied=4;
        o->source=0x600000; o->pool_table=0x610000; o->slot=o->pool_table+(2+i)*0x30;
        o->model=0x700000+i*0x10000; o->control=0x800000+i*0x10000;
        o->clone=(struct jc_clone_snapshot){0x900000,0xa00000+i*0x10000,0xb00000+i*0x10000,0xc00000+i*0x10000};
        o->entries=0xd00000+i*0x10000; o->count=100; o->manager=0xe00000; o->cache=0xf00000+i*0x10000; o->head=0x1100000+i*0x10000;
        o->pool_heads[0]=0x100210; o->pool_heads[1]=0x114900;
        o->pool_heads[2]=objects[0]+0x210; o->pool_heads[3]=objects[1]+0x210;
        life[i]=(struct jc_initialized_lifetime){.object=objects[i],.game=0x1200000,.barn=0x100000,.thread=7,.state=1,.owner=*o};
        life[i].owner.expected_occupied=3+i;
    }
    life[0].owner.pool_heads[3]=0;
}
int main(void) {
    fixture(); assert(!jc_dual_owner_pair(life,now));
    fixture(); life[1].object=life[0].object; assert(jc_dual_owner_pair(life,now)==1);
    fixture(); life[1].object=life[0].object+0x14630; assert(jc_dual_owner_pair(life,now)==1);
    fixture(); life[1].thread++; assert(jc_dual_owner_pair(life,now)==1);
    fixture(); life[1].barn++; assert(jc_dual_owner_pair(life,now)==1);
    fixture(); life[1].state=-1; assert(jc_dual_owner_pair(life,now)==1);
    fixture(); now[0].model++; assert(jc_dual_owner_pair(life,now)==2);
    fixture(); now[0].expected_occupied=3; assert(jc_dual_owner_pair(life,now)==2);
    fixture(); now[0].expected_slots=3; assert(jc_dual_owner_pair(life,now)==2);
    fixture(); life[0].owner.expected_occupied=4; assert(jc_dual_owner_pair(life,now)==2);
    fixture(); now[1].control=life[1].owner.control=now[0].control; assert(jc_dual_owner_pair(life,now)==3);
    fixture(); now[1].slot=life[1].owner.slot=now[0].slot; assert(jc_dual_owner_pair(life,now)==3);
    fixture(); now[1].model=life[1].owner.model=now[0].model+0x100; assert(jc_dual_owner_pair(life,now)==3);
    fixture(); now[1].clone.payload=life[1].owner.clone.payload=now[0].clone.payload; assert(jc_dual_owner_pair(life,now)==3);
    fixture(); now[1].source=life[1].owner.source=now[0].source+8; assert(jc_dual_owner_pair(life,now)==3);
    for (unsigned i=0;i<4;++i) {
        fixture(); now[1].pool_heads[i]=0; assert(jc_dual_owner_pair(life,now)==4);
        fixture(); now[0].pool_heads[i]++; assert(jc_dual_owner_pair(life,now)==4);
    }
    fixture(); life[0].owner.pool_heads[0]=0; assert(jc_dual_owner_pair(life,now)==5);
    fixture(); life[0].owner.pool_heads[1]++; assert(jc_dual_owner_pair(life,now)==5);
    fixture(); life[0].owner.pool_heads[3]=now[0].pool_heads[3]; assert(jc_dual_owner_pair(life,now)==5);
    fixture(); now[0].slot=life[0].owner.slot=0x610001; assert(jc_dual_owner_pair(life,now)==6);
    fixture(); now[0].slot=life[0].owner.slot=0x6100c0; assert(jc_dual_owner_pair(life,now)==6);
    fixture(); now[0].slot=life[0].owner.slot=0x610030; assert(jc_dual_owner_pair(life,now)==6);
    struct jc_dual_pose_snapshot a={0},b={0};
    assert(jc_dual_pose_same(&a,&b));
    b.world[12]=1; assert(!jc_dual_pose_same(&a,&b)); b=a;
    b.root[13]=1; assert(!jc_dual_pose_same(&a,&b)); b=a;
    b.control=1; assert(!jc_dual_pose_same(&a,&b)); b=a;
    b.storage.root=1; assert(!jc_dual_pose_same(&a,&b)); b=a;
    b.storage.clone.payload=1; assert(!jc_dual_pose_same(&a,&b));
    puts("Dual body: independent objects/controllers/clones, exact four-slot ownership, sole expected slot addition, changed first-body ownership and partial failures checked.");
    return 0;
}
