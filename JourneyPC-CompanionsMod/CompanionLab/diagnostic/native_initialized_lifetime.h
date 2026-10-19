#ifndef JC_NATIVE_INITIALIZED_LIFETIME_H
#define JC_NATIVE_INITIALIZED_LIFETIME_H
#include "native_mesh_rehearsal.h"
#include "native_geometry_pool.h"
struct jc_initialized_owner {
    struct jc_clone_snapshot clone;
    uint64_t source,slot,model,control,entries,count,manager,cache,head;
    uint64_t pool_table,pool_heads[4];
    uint32_t expected_slots,expected_occupied;
};
enum jc_initialized_field { JC_IF_NONE, JC_IF_OBJECT, JC_IF_CLONE, JC_IF_SOURCE,
    JC_IF_SLOT, JC_IF_MODEL, JC_IF_CONTROL, JC_IF_ENTRIES, JC_IF_COUNT,
    JC_IF_MANAGER, JC_IF_CACHE, JC_IF_HEAD, JC_IF_CLONE_PAIR, JC_IF_POOL,
    JC_IF_STATE, JC_IF_GAME, JC_IF_THREAD, JC_IF_BARN, JC_IF_POST_CLEANUP,
    JC_IF_LOCK, JC_IF_RESTORATION, JC_IF_CHECKPOINT, JC_IF_BUDGET };
struct jc_initialized_detail {
    int field,pool_valid; uint32_t slots,occupied,free_slots;
    uint64_t pool_source,pool_table,pool_heads[4];
    int comparison_valid,same_pool,changed_mask,before[4],after[4];
    int remaining_valid,remaining_code,remaining_field;
};
static inline int jc_initialized_head_label(uint64_t head,uint64_t barn,uint64_t object) {
    if (!head) return 0;
    if (head==barn+0xb0+0x160+0xb0) return 1;
    if (head==barn+0x146f0+0x160+0xb0) return 2;
    if (head==object+0x160+0xb0) return 3;
    if (head==barn+0xb0+0x7b0+0xb0) return 5;
    if (head==barn+0xb0+0xe50+0xb0) return 6;
    if (head==barn+0x146f0+0x7b0+0xb0) return 7;
    if (head==barn+0x146f0+0xe50+0xb0) return 8;
    return 4;
}
static inline int jc_initialized_error(struct jc_initialized_detail *detail,int field,int code) {
    if (detail) detail->field=field;
    return code;
}
struct jc_initialized_lifetime {
    uint64_t object,game,barn,thread;
    struct jc_initialized_owner owner;
    int state,cleanup_stage,failure_code;
    int head_kinds[4],pool_released; uint32_t released_free;
    struct jc_initialized_detail detail;
};
static inline int jc_initialized_head_kind(jc_layout_reader read,void *ctx,uint64_t image,
    const struct jc_initialized_owner *owner,unsigned index) {
    if (index>=(owner->expected_slots ? owner->expected_slots : 3)) return 5;
    uint64_t head=owner->pool_heads[index],values[7],again[7],mesh_type;
    if (!head) return 0;
    if (head<0xb0 || (head&7)) return 5;
    uint64_t property=head-0x30;
    if (!jc_layout_field(read,ctx,property,0,values,sizeof(values))) return 5;
    if (values[0]!=image+0x680970 || values[1]!=image+0x680858) return 3;
    if (values[2]!=owner->source || values[6]!=owner->pool_table+(uint64_t)index*0x30) return 4;
    if (!jc_layout_field(read,ctx,head-0xb0,0,&mesh_type,8) ||
        !jc_layout_field(read,ctx,property,0,again,sizeof(again))) return 5;
    if (memcmp(values,again,sizeof(values))) return 4;
    return mesh_type==image+0x67f3f8 ? 2 : 1;
}
static inline int jc_initialized_compare(const struct jc_initialized_owner *now,
    const struct jc_initialized_owner *prior) {
    if (!jc_clone_snapshot_equal(&now->clone,&prior->clone)) return JC_IF_CLONE;
    const uint64_t current[]={now->source,now->slot,now->model,now->control,now->entries,now->count,now->manager,now->cache,now->head};
    const uint64_t captured[]={prior->source,prior->slot,prior->model,prior->control,
        prior->entries,prior->count,prior->manager,prior->cache,prior->head};
    const int ids[]={JC_IF_SOURCE,JC_IF_SLOT,JC_IF_MODEL,JC_IF_CONTROL,JC_IF_ENTRIES,
        JC_IF_COUNT,JC_IF_MANAGER,JC_IF_CACHE,JC_IF_HEAD};
    for (unsigned i=0;i<9;++i) if (current[i]!=captured[i]) return ids[i];
    return JC_IF_NONE;
}
static inline int jc_initialized_owner_read_group(jc_layout_reader read,void *ctx,uint64_t image,
    uint64_t object,struct jc_initialized_owner *out,struct jc_initialized_detail *detail,
    uint32_t expected_slots,uint32_t expected_occupied) {
    if ((expected_slots!=3 && expected_slots!=4) || expected_occupied<3 || expected_occupied>expected_slots)
        return jc_initialized_error(detail,JC_IF_POOL,JC_LAYOUT_RANGE);
    struct jc_initialized_owner value={.expected_slots=expected_slots,.expected_occupied=expected_occupied}; uint64_t type,end,pair[2];
    if (detail) memset(detail,0,sizeof(*detail));
    if (!jc_layout_address(object,0x14640,&end)) return jc_initialized_error(detail,JC_IF_OBJECT,JC_LAYOUT_RANGE);
    uint64_t mesh=object+0x160;
    if (!jc_layout_field(read,ctx,object,0,&type,8)) return jc_initialized_error(detail,JC_IF_OBJECT,JC_LAYOUT_READ);
    if (type!=image+0x5d0380) return jc_initialized_error(detail,JC_IF_OBJECT,JC_LAYOUT_TYPE);
    int code=jc_clone_snapshot_read(read,ctx,image,mesh,&value.clone);
    if (code) return jc_initialized_error(detail,JC_IF_CLONE,code);
    if (!value.clone.slot || !value.clone.resource || !value.clone.payload) return jc_initialized_error(detail,JC_IF_CLONE,JC_LAYOUT_TYPE);
    const uint64_t offsets[]={0x90,0xb0,0x518,0x520,0x578,0x580,0x528,0x4e8,8};
    uint64_t *fields[]={&value.source,&value.slot,&value.model,&value.control,&value.entries,
        &value.count,&value.manager,&value.cache,&value.head};
    const int ids[]={JC_IF_SOURCE,JC_IF_SLOT,JC_IF_MODEL,JC_IF_CONTROL,JC_IF_ENTRIES,
        JC_IF_COUNT,JC_IF_MANAGER,JC_IF_CACHE,JC_IF_HEAD};
    uint64_t model_fields[3],entry_fields[2];
    if (!jc_layout_field(read,ctx,mesh,0x518,model_fields,sizeof(model_fields)))
        return jc_initialized_error(detail,JC_IF_MODEL,JC_LAYOUT_READ);
    if (!jc_layout_field(read,ctx,mesh,0x578,entry_fields,sizeof(entry_fields)))
        return jc_initialized_error(detail,JC_IF_ENTRIES,JC_LAYOUT_READ);
    value.model=model_fields[0]; value.control=model_fields[1]; value.manager=model_fields[2];
    value.entries=entry_fields[0]; value.count=entry_fields[1];
    const unsigned separate[]={0,1,7,8};
    for (unsigned n=0;n<4;++n) {
        unsigned i=separate[n];
        if (!jc_layout_field(read,ctx,mesh,offsets[i],fields[i],8)) return jc_initialized_error(detail,ids[i],JC_LAYOUT_READ);
    }
    if (!jc_layout_field(read,ctx,value.clone.payload,0,pair,16)) return jc_initialized_error(detail,JC_IF_CLONE_PAIR,JC_LAYOUT_READ);
    if (!value.model || value.model!=pair[0]) return jc_initialized_error(detail,JC_IF_MODEL,JC_LAYOUT_TYPE);
    if (!value.control || value.control!=pair[1]) return jc_initialized_error(detail,JC_IF_CONTROL,JC_LAYOUT_TYPE);
    const unsigned required[]={0,1,6,7,8};
    for (unsigned i=0;i<5;++i) if (!*fields[required[i]]) return jc_initialized_error(detail,ids[required[i]],JC_LAYOUT_TYPE);
    if (!value.count || value.count>4096) return jc_initialized_error(detail,JC_IF_COUNT,JC_LAYOUT_RANGE);
    if ((value.entries&7) || !jc_layout_address(value.entries,value.count*0x50,&end)) return jc_initialized_error(detail,JC_IF_ENTRIES,JC_LAYOUT_RANGE);
    struct jc_geometry_pool pool;
    struct jc_geometry_slots slots;
    code=jc_geometry_pool_read_slots(read,ctx,mesh+0x80,&pool,&slots);
    if (code) return jc_initialized_error(detail,JC_IF_POOL,code);
    if (detail) { detail->pool_valid=1; detail->slots=pool.slots; detail->occupied=pool.occupied; detail->free_slots=pool.free_slots; }
    if (pool.slots==expected_slots) {
        value.pool_table=slots.table; memcpy(value.pool_heads,slots.heads,sizeof(value.pool_heads));
        if (detail) {
            detail->pool_source=pool.source; detail->pool_table=slots.table;
            memcpy(detail->pool_heads,slots.heads,sizeof(detail->pool_heads));
        }
    }
    *out=value;
    if (pool.source!=value.source || pool.slots!=expected_slots || pool.occupied!=expected_occupied || pool.free_slots!=expected_slots-expected_occupied) return jc_initialized_error(detail,JC_IF_POOL,JC_LAYOUT_CHANGED);
    return JC_LAYOUT_OK;
}
static inline int jc_initialized_owner_read_detail(jc_layout_reader read,void *ctx,uint64_t image,
    uint64_t object,struct jc_initialized_owner *out,struct jc_initialized_detail *detail) {
    return jc_initialized_owner_read_group(read,ctx,image,object,out,detail,3,3);
}
static inline int jc_initialized_owner_read(jc_layout_reader read,void *ctx,uint64_t image,
    uint64_t object,struct jc_initialized_owner *out) {
    return jc_initialized_owner_read_detail(read,ctx,image,object,out,NULL);
}
static inline int jc_initialized_validate_detail(const struct jc_initialized_lifetime *life,
    jc_layout_reader read,void *ctx,uint64_t image,uint64_t game,uint64_t thread,
    struct jc_initialized_detail *detail) {
    uint64_t barn;
    if (detail) memset(detail,0,sizeof(*detail));
    if (life->state!=1) return jc_initialized_error(detail,JC_IF_STATE,JC_LAYOUT_CHANGED);
    if (game!=life->game) return jc_initialized_error(detail,JC_IF_GAME,JC_LAYOUT_CHANGED);
    if (thread!=life->thread) return jc_initialized_error(detail,JC_IF_THREAD,JC_LAYOUT_CHANGED);
    if (!jc_layout_field(read,ctx,game,0x1e0,&barn,8)) return jc_initialized_error(detail,JC_IF_BARN,JC_LAYOUT_READ);
    if (barn!=life->barn) return jc_initialized_error(detail,JC_IF_BARN,JC_LAYOUT_CHANGED);
    struct jc_initialized_owner now;
    unsigned slots=life->owner.expected_slots ? life->owner.expected_slots : 3;
    unsigned occupied=life->owner.expected_occupied ? life->owner.expected_occupied : 3;
    int code=jc_initialized_owner_read_group(read,ctx,image,life->object,&now,detail,slots,occupied);
    if (detail && detail->pool_valid && detail->slots==slots && life->owner.pool_table) {
        detail->comparison_valid=1;
        detail->same_pool=detail->pool_source==life->owner.source && detail->pool_table==life->owner.pool_table;
        for (unsigned i=0;i<slots;++i) {
            detail->before[i]=jc_initialized_head_label(life->owner.pool_heads[i],life->barn,life->object);
            detail->after[i]=jc_initialized_head_label(detail->pool_heads[i],life->barn,life->object);
            if (detail->pool_heads[i]!=life->owner.pool_heads[i]) detail->changed_mask|=1<<i;
        }
    }
    if (detail && detail->pool_valid) {
        detail->remaining_valid=1;
        detail->remaining_field=jc_initialized_compare(&now,&life->owner);
        detail->remaining_code=detail->remaining_field ? JC_LAYOUT_CHANGED : JC_LAYOUT_OK;
    }
    if (code) return code;
    int changed=jc_initialized_compare(&now,&life->owner);
    if (changed) return jc_initialized_error(detail,changed,JC_LAYOUT_CHANGED);
    return JC_LAYOUT_OK;
}
static inline int jc_initialized_validate(const struct jc_initialized_lifetime *life,
    jc_layout_reader read,void *ctx,uint64_t image,uint64_t game,uint64_t thread) {
    return jc_initialized_validate_detail(life,read,ctx,image,game,thread,NULL);
}
static inline int jc_initialized_boundary_exception(const struct jc_initialized_lifetime *life) {
    const struct jc_initialized_detail *d=&life->detail;
    return life->failure_code==JC_LAYOUT_CHANGED && d->field==JC_IF_POOL &&
        d->pool_valid && d->slots==3 && d->occupied==2 && d->free_slots==1 &&
        d->comparison_valid && d->same_pool && d->changed_mask==1 &&
        d->remaining_valid && !d->remaining_code && !d->remaining_field &&
        life->head_kinds[0]==2 && d->before[0]==4 && d->after[0]==0 &&
        d->before[1]==2 && d->after[1]==2 && d->before[2]==3 && d->after[2]==3;
}
static inline int jc_initialized_pool_released(struct jc_initialized_lifetime *life,
    jc_layout_reader read,void *ctx) {
    struct jc_geometry_pool pool; struct jc_geometry_slots slots;
    const struct jc_initialized_detail *before=&life->detail;
    uint64_t remote=life->barn+0x146f0+0x160+0x80;
    if (!before->pool_valid || before->slots!=3 || !before->same_pool ||
        jc_geometry_pool_read_slots(read,ctx,remote,&pool,&slots) ||
        pool.source!=life->owner.source || slots.table!=life->owner.pool_table || pool.slots!=3 ||
        pool.occupied+1!=before->occupied || pool.free_slots!=before->free_slots+1) return 0;
    uint64_t trial_slot=life->owner.slot;
    for (unsigned i=0;i<3;++i) {
        uint64_t expected=slots.table+(uint64_t)i*0x30==trial_slot ? 0 : before->pool_heads[i];
        if (slots.heads[i]!=expected) return 0;
    }
    life->released_free=pool.free_slots;
    return 1;
}
static inline int jc_initialized_cleanup(struct jc_initialized_lifetime *life,
    const struct jc_mesh_ops *ops,uint64_t image,uint64_t game,uint64_t thread,uint64_t dude) {
    if (life->state!=1 || dude!=life->barn+0xb0) return life->state;
    const struct jc_allocation_ops *a=&ops->allocation;
    life->cleanup_stage=1;
    life->failure_code=jc_initialized_validate_detail(life,a->read,a->context,image,game,thread,&life->detail);
    if (jc_initialized_boundary_exception(life)) { life->failure_code=0; life->detail.field=JC_IF_NONE; }
    else if (!life->failure_code && (!life->detail.comparison_valid || !life->detail.same_pool || life->detail.changed_mask))
        life->failure_code=jc_initialized_error(&life->detail,JC_IF_POOL,JC_LAYOUT_CHANGED);
    if (life->failure_code) { life->object=0; return life->state=-1; }
    uint64_t object=life->object;
    life->object=0; life->state=3; life->cleanup_stage=2;
    ops->cleanup(a->context,object);
    life->cleanup_stage=3;
    if (jc_allocation_validate(a,image,life->barn,object)!=JC_ALLOC_OK) {
        life->failure_code=JC_LAYOUT_CHANGED; life->detail.field=JC_IF_POST_CLEANUP;
        return life->state=-1;
    }
    life->pool_released=jc_initialized_pool_released(life,a->read,a->context);
    if (!life->pool_released) {
        life->failure_code=12; life->detail.field=JC_IF_RESTORATION; return life->state=-1;
    }
    life->cleanup_stage=4;
    a->destroy(a->context,object);
    life->cleanup_stage=5;
    life->pool_released=jc_initialized_pool_released(life,a->read,a->context);
    if (!life->pool_released) {
        life->failure_code=12; life->detail.field=JC_IF_RESTORATION; return life->state=-1;
    }
    return life->state=2;
}
#endif
