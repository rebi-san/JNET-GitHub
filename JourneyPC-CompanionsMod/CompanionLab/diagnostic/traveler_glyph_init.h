#ifndef JC_TRAVELER_GLYPH_INIT_H
#define JC_TRAVELER_GLYPH_INIT_H
#include "native_geometry_pool.h"
#include "native_cloth_material_live.h"
static struct {
    uint64_t image,actor,material,source,table,thread;
    unsigned count,selected,state,failed,calls,initialized;
    unsigned slot_detail,slot_result,retries,recovered,slot_rejects,first_detail,first_pool,live_slots;
    uint64_t slot,link,head,tail;
} traveler_glyph_init;
static unsigned traveler_glyph_sampler_hook_ready;
enum { JC_GLYPH_HOOK=80,JC_GLYPH_BOUNDS,JC_GLYPH_READ,JC_GLYPH_OCCUPIED,
    JC_GLYPH_ANCHOR,JC_GLYPH_CAPACITY,JC_GLYPH_CHANGED,JC_GLYPH_SLOT,
    JC_GLYPH_STORAGE,JC_GLYPH_ALIAS,JC_GLYPH_THREAD,JC_GLYPH_ASSIGNMENT };
static int traveler_glyph_fail(unsigned code) {
    if(code==JC_GLYPH_SLOT && !traveler_glyph_init.slot_rejects++) {
        traveler_glyph_init.first_detail=traveler_glyph_init.slot_detail;
        traveler_glyph_init.first_pool=traveler_glyph_init.slot_result;
    }
    traveler_glyph_init.failed=code;return 0;
}
static int traveler_glyph_runtime_pool(const struct jc_geometry_pool *pool) {
    uint64_t payload=0,type=0,length=0,capacity=0,address=0;uint32_t limit=0;
    unsigned char name[32];char text[24]={0};
    if(!own_read(NULL,pool->source+0x10,&payload,8) ||
        !own_read(NULL,payload,&type,8) || type!=traveler_glyph_init.image+0x680930 ||
        !own_read(NULL,payload+0x110,&limit,4) || limit!=pool->slots ||
        !own_read(NULL,payload+8,name,sizeof(name)))return 0;
    memcpy(&length,name+16,8);memcpy(&capacity,name+24,8);
    if(length>=sizeof(text) || capacity<length || capacity>4096)return 0;
    address=payload+8;
    if(capacity>=16)memcpy(&address,name,8);
    if(!own_read(NULL,address,text,(size_t)length+1) || text[length])return 0;
    return (length==14 && !memcmp(text,"ScatteringSoft",15) && pool->slots==136) ||
        (length==21 && !memcmp(text,"ScatteringSoftNoDepth",22) && pool->slots==32);
}
static int traveler_glyph_pool_check(uint64_t material) {
    struct jc_geometry_pool pool;struct jc_geometry_slots slots;
    struct jc_cloth_storage owned,other;
    uint64_t selected=0,self=0;
    traveler_glyph_init.slot_detail=1;
    traveler_glyph_init.slot_result=(unsigned)jc_geometry_pool_read_slots_bounded(own_read,NULL,material,
        traveler_glyph_init.initialized?JC_GEOMETRY_GLYPH_SLOTS:64,&pool,&slots);
    traveler_glyph_init.slot=traveler_glyph_init.link=traveler_glyph_init.head=traveler_glyph_init.tail=0;
    if(traveler_glyph_init.slot_result) {
        own_read(NULL,material+0x30,&traveler_glyph_init.slot,8);
        uint64_t row=traveler_glyph_init.slot,table=traveler_glyph_init.table;
        if(row>=table && row-table<(uint64_t)traveler_glyph_init.count*0x30 && (row-table)%0x30==0) {
            own_read(NULL,row,&traveler_glyph_init.head,8);
            own_read(NULL,row+8,&traveler_glyph_init.tail,8);
            own_read(NULL,row+0x10,&traveler_glyph_init.link,8);
        }
        return traveler_glyph_fail(JC_GLYPH_SLOT);
    }
    traveler_glyph_init.live_slots=pool.slots;
    int original=pool.source==traveler_glyph_init.source && pool.slots==traveler_glyph_init.count &&
        slots.table==traveler_glyph_init.table;
    if(!original && (!traveler_glyph_init.initialized || traveler_glyph_init.state!=2 ||
        !traveler_glyph_runtime_pool(&pool)))return traveler_glyph_fail(JC_GLYPH_CHANGED);
    traveler_glyph_init.slot_detail=2;
    if(!own_read(NULL,material+0x30,&selected,8) ||
        !own_read(NULL,selected+0x10,&self,8))return traveler_glyph_fail(JC_GLYPH_READ);
    traveler_glyph_init.slot=selected;traveler_glyph_init.link=self;
    if(self!=selected)return traveler_glyph_fail(JC_GLYPH_SLOT);
    traveler_glyph_init.slot_detail=3;
    if(traveler_glyph_init.state==1 && selected!=slots.table+traveler_glyph_init.selected*0x30u)
        return traveler_glyph_fail(JC_GLYPH_SLOT);
    if(jc_cloth_storage_read(own_read,NULL,material,&owned))return traveler_glyph_fail(JC_GLYPH_STORAGE);
    for(unsigned i=0;i<pool.slots;++i) {
        uint64_t row=slots.table+i*0x30u,head=slots.heads[i],tail=0;
        if(!head)continue;
        if(head<0x30 || !own_read(NULL,row+8,&tail,8))return traveler_glyph_fail(JC_GLYPH_READ);
        if(row==selected) {
            traveler_glyph_init.slot_detail=4;
            traveler_glyph_init.head=head;traveler_glyph_init.tail=tail;
            if(head!=material+0x30 || tail!=head)return traveler_glyph_fail(JC_GLYPH_SLOT);
        } else {
            if(jc_cloth_storage_read(own_read,NULL,head-0x30,&other))return traveler_glyph_fail(JC_GLYPH_STORAGE);
            if(!jc_cloth_storage_disjoint(&owned,&other))return traveler_glyph_fail(JC_GLYPH_ALIAS);
        }
    }
    struct jc_geometry_pool fresh;struct jc_geometry_slots heads;
    struct jc_cloth_storage again;
    int valid=!jc_geometry_pool_read_slots_bounded(own_read,NULL,material,
        traveler_glyph_init.initialized?JC_GEOMETRY_GLYPH_SLOTS:64,&fresh,&heads) &&
        !memcmp(&pool,&fresh,sizeof(pool)) && !memcmp(&slots,&heads,sizeof(slots)) &&
        !jc_cloth_storage_read(own_read,NULL,material,&again) && !memcmp(&owned,&again,sizeof(owned));
    return valid?1:traveler_glyph_fail(JC_GLYPH_CHANGED);
}
static int traveler_glyph_init_begin(uint64_t image,uint64_t actor,uint64_t anchor,
        uint64_t decorations,uint32_t before) {
    struct jc_geometry_pool pool;struct jc_geometry_slots slots;
    uint64_t glyph,material,links[3],source=0,type=0,payload=0;uint32_t count=0;
    if(!traveler_glyph_sampler_hook_ready)return traveler_glyph_fail(JC_GLYPH_HOOK);
    if(traveler_glyph_init.state==3) {
        if(traveler_glyph_init.thread!=GetCurrentThreadId())return traveler_glyph_fail(JC_GLYPH_THREAD);
        memset(&traveler_glyph_init,0,sizeof(traveler_glyph_init));
    }
    if(traveler_glyph_init.state)return traveler_glyph_fail(JC_GLYPH_HOOK);
    if(before>=1024 || !jc_layout_address(decorations,0x10+(uint64_t)before*0x10d0,&glyph) ||
        !jc_layout_address(glyph,0x90,&material))return traveler_glyph_fail(JC_GLYPH_BOUNDS);
    if(!own_read(NULL,decorations+0x434010,&count,4) ||
        !own_read(NULL,material+0x10,&source,8) ||
        !own_read(NULL,material+0x30,links,sizeof(links)))return traveler_glyph_fail(JC_GLYPH_READ);
    if(count!=before)return traveler_glyph_fail(JC_GLYPH_CHANGED);
    if(source || links[0] || links[1] || links[2])return traveler_glyph_fail(JC_GLYPH_OCCUPIED);
    if(jc_geometry_pool_read_slots(own_read,NULL,anchor,&pool,&slots) ||
        !own_read(NULL,pool.source+0x10,&payload,8) ||
        !own_read(NULL,payload,&type,8) || type!=image+0x680930)return traveler_glyph_fail(JC_GLYPH_ANCHOR);
    unsigned first=0;while(first<pool.slots && slots.heads[first])++first;
    if(!pool.free_slots || first==pool.slots)return traveler_glyph_fail(JC_GLYPH_CAPACITY);
    traveler_glyph_init.failed=0;
    traveler_glyph_init.image=image;traveler_glyph_init.actor=actor;
    traveler_glyph_init.material=material;traveler_glyph_init.source=pool.source;
    traveler_glyph_init.table=slots.table;traveler_glyph_init.count=pool.slots;
    traveler_glyph_init.selected=first;traveler_glyph_init.thread=GetCurrentThreadId();
    traveler_glyph_init.state=1;return 1;
}
static int traveler_glyph_write_allowed(uint64_t material,int sampler,const uint64_t *model) {
    if(!traveler_glyph_init.state || traveler_glyph_init.state==3 || material!=traveler_glyph_init.material)return 1;
    int retry=traveler_glyph_init.initialized && traveler_glyph_init.state==2 &&
        traveler_glyph_init.failed==JC_GLYPH_SLOT;
    if(traveler_glyph_init.failed && !retry)return 0;
    if(traveler_glyph_init.thread!=GetCurrentThreadId())return traveler_glyph_fail(JC_GLYPH_THREAD);
    if(retry)++traveler_glyph_init.retries;
    uint64_t slot=0,pair[2];
    if(sampler && model && own_read(NULL,(uintptr_t)model,pair,sizeof(pair)) &&
        own_read(NULL,material+0x30,&slot,8) && !slot)return 1;
    if(!traveler_glyph_pool_check(material))return 0;
    if(retry)++traveler_glyph_init.recovered;
    traveler_glyph_init.failed=0;traveler_glyph_init.slot_detail=0;
    ++traveler_glyph_init.calls;return 1;
}
static int traveler_glyph_init_release(uint64_t actor) {
    if(!traveler_glyph_init.state)return 1;
    if(traveler_glyph_init.actor!=actor || traveler_glyph_init.thread!=GetCurrentThreadId())return 0;
    traveler_glyph_init.state=3;return 1;
}
static int traveler_glyph_object_allowed(uint64_t object,unsigned kind) {
    if(!traveler_glyph_init.state || traveler_glyph_init.state==3)return 1;
    if(kind!=1)return traveler_glyph_write_allowed(object,0,NULL);
    uint64_t slot=0,link=0,payload=0;
    if(!own_read(NULL,traveler_glyph_init.material+0x30,&slot,8))return traveler_glyph_fail(JC_GLYPH_READ);
    if(!slot)return 1;
    if(!own_read(NULL,slot+0x10,&link,8))return traveler_glyph_fail(JC_GLYPH_READ);
    if(!link)return 1;
    if(!own_read(NULL,link+0x18,&payload,8))return traveler_glyph_fail(JC_GLYPH_READ);
    return payload!=object || traveler_glyph_write_allowed(traveler_glyph_init.material,0,NULL);
}
static int traveler_glyph_init_end(void) {
    uint64_t glyph=0;
    int ok=traveler_glyph_init.state==1 && !traveler_glyph_init.failed && traveler_glyph_init.calls &&
        own_read(NULL,traveler_glyph_init.actor+0x2240,&glyph,8) &&
        glyph+0x90==traveler_glyph_init.material && traveler_glyph_pool_check(glyph+0x90);
    traveler_glyph_init.state=2;traveler_glyph_init.initialized=(unsigned)ok;
    if(!ok && !traveler_glyph_init.failed)traveler_glyph_init.failed=JC_GLYPH_ASSIGNMENT;
    return ok;
}
#endif
