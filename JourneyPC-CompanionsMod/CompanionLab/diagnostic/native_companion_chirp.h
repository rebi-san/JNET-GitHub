#ifndef JC_NATIVE_COMPANION_CHIRP_H
#define JC_NATIVE_COMPANION_CHIRP_H
#include "native_layout.h"
#include <math.h>
#define JC_CHIRP_SLOTS 16u
#define JC_CHIRP_BARN_BYTES 0x718u
#define JC_CHIRP_SOUND_CAP 160u
enum { JC_CHIRP_OK,JC_CHIRP_READ,JC_CHIRP_ARGUMENT,JC_CHIRP_CHANGED,
    JC_CHIRP_RANGE,JC_CHIRP_FULL,JC_CHIRP_SOUND,JC_CHIRP_WRITE,JC_CHIRP_POSTCHECK,JC_CHIRP_CODE };
struct jc_chirp_event {
    uint64_t participant,epoch,sequence;
    uint32_t sound;
    float position[3],velocity[3],radius,duration;
};
struct jc_chirp_source { struct jc_chirp_event event; unsigned slot,id; unsigned char native[0x38]; };
struct jc_chirp_batch { uint64_t barn,sounds,frame; unsigned count,dropped; struct jc_chirp_source source[JC_CHIRP_SLOTS]; };
struct jc_chirp_capture { uint64_t barn,frame,sequence; uint32_t next; unsigned primed; };
struct jc_chirp_delivery { uint64_t participant,epoch,sequence; };
struct jc_chirp_bridge {
    uint64_t game,barn,sounds,frame;
    uint32_t counter,sound_index,sound_count;
    unsigned slot;
    unsigned char before[JC_CHIRP_BARN_BYTES],name[32];
    _Alignas(16) unsigned char input[0x40];
};
typedef int (*jc_chirp_writable)(void *,uint64_t,size_t);
static inline uint32_t jc_chirp_u32(const void *p) { uint32_t n; memcpy(&n,p,4); return n; }
static inline uint64_t jc_chirp_u64(const void *p) { uint64_t n; memcpy(&n,p,8); return n; }
static inline const char *jc_chirp_sound_name(unsigned sound,int companion) {
    static const char *const names[2][4]={{"PlayerCoo","PlayerChirp","PlayerCall","PlayerShout"},
        {"Player2Coo","Player2Chirp","Player2Call","Player2Shout"}};
    static const char *const creatures[]={"FishComeHere","Guardian_OutOfSnowVO","HappyWhale",
        "FishIdleMoveShort","FishIdleMoveMedium","FishIdleMoveLong","ClothGuardianShout", "JNTFishShort", "JNTFishMedium", "JNTFishLong", "JNTFishCall",
        "JNTWarVoice", "JNTWarLock", "JNTWhale", "JNTWarHungry", "JNTWarAway", "JNTFlowSnake", "JNTFlowHunter"};
    if(sound>=5 && sound<=22)return creatures[sound-5];
    return sound>=1 && sound<=4 ? names[companion!=0][sound-1] : NULL;
}
static inline unsigned jc_chirp_sound_kind(const unsigned char name[32]) {
    if (!memchr(name,0,32)) return 0;
    for (unsigned i=1;i<=22;++i) if (!strcmp((const char *)name,jc_chirp_sound_name(i,0))) return i;
    return 0;
}
static inline int jc_chirp_event_valid(const struct jc_chirp_event *e) {
    if (!e || !e->participant || !e->epoch || !e->sequence || !jc_chirp_sound_name(e->sound,0) ||
        !isfinite(e->radius) || e->radius<=0 || e->radius>100 ||
        !isfinite(e->duration) || e->duration<0.01f || e->duration>10) return 0;
    for (unsigned i=0;i<3;++i) if (!isfinite(e->position[i]) || fabsf(e->position[i])>1000000 ||
        !isfinite(e->velocity[i]) || fabsf(e->velocity[i])>1000) return 0;
    return 1;
}
static inline int jc_chirp_delivery_fresh(const struct jc_chirp_delivery *d,const struct jc_chirp_event *e) {
    return d && jc_chirp_event_valid(e) && (!d->participant ||
        (d->participant==e->participant && d->epoch==e->epoch && e->sequence>d->sequence));
}
static inline void jc_chirp_delivery_commit(struct jc_chirp_delivery *d,const struct jc_chirp_event *e) {
    d->participant=e->participant; d->epoch=e->epoch; d->sequence=e->sequence;
}
static inline int jc_chirp_snapshot(jc_layout_reader read,void *ctx,uint64_t game,
        uint64_t *barn,uint64_t *sounds,unsigned char slots[JC_CHIRP_BARN_BYTES]) {
    unsigned char again[JC_CHIRP_BARN_BYTES]; uint64_t b,s;
    if (!read || !jc_layout_field(read,ctx,game,0x1b8,barn,8) ||
        !jc_layout_field(read,ctx,game,0x150,sounds,8)) return JC_CHIRP_READ;
    if (((*barn|*sounds)&15) || !*barn || !*sounds) return JC_CHIRP_RANGE;
    if (!jc_layout_field(read,ctx,*barn,0,slots,JC_CHIRP_BARN_BYTES) ||
        !jc_layout_field(read,ctx,*barn,0,again,sizeof(again)) ||
        !jc_layout_field(read,ctx,game,0x1b8,&b,8) ||
        !jc_layout_field(read,ctx,game,0x150,&s,8)) return JC_CHIRP_READ;
    if (b!=*barn || s!=*sounds || memcmp(slots,again,sizeof(again))) return JC_CHIRP_CHANGED;
    return 0;
}
static inline int jc_chirp_capture_local(jc_layout_reader read,void *ctx,uint64_t game,uint64_t frame,
        uint64_t participant,uint64_t epoch,struct jc_chirp_capture *capture,struct jc_chirp_batch *batch) {
    unsigned char slots[JC_CHIRP_BARN_BYTES]; uint32_t sound_count; int code;
    if (!capture || !batch || !participant || !epoch || !frame) return JC_CHIRP_ARGUMENT;
    memset(batch,0,sizeof(*batch)); batch->frame=frame;
    code=jc_chirp_snapshot(read,ctx,game,&batch->barn,&batch->sounds,slots); if (code) return code;
    uint32_t next=jc_chirp_u32(slots+0x714),delta=next-capture->next;
    if (!capture->primed || capture->barn!=batch->barn) {
        capture->primed=1; capture->barn=batch->barn; capture->next=next; capture->frame=frame; return 0;
    }
    if (frame==capture->frame) return 0;
    if (frame<capture->frame || delta>=UINT32_C(0x80000000)) return JC_CHIRP_CHANGED;
    if (!delta) {capture->frame=frame;return 0;}
    if (!jc_layout_field(read,ctx,batch->sounds,0x7c7b4,&sound_count,4)) return JC_CHIRP_READ;
    if (!sound_count || sound_count>JC_CHIRP_SOUND_CAP) return JC_CHIRP_SOUND;
    unsigned order[JC_CHIRP_SLOTS],count=0;
    for (unsigned i=0;i<JC_CHIRP_SLOTS;++i) {
        const unsigned char *s=slots+i*0x70; uint32_t id=jc_chirp_u32(s+0x50);
        if (s[0x68]!=1 || s[0x4c]!=1 || s[0x4d] || id-capture->next>=delta) continue;
        unsigned at=count;
        while (at && jc_chirp_u32(slots+order[at-1]*0x70+0x50)-capture->next>id-capture->next) {
            order[at]=order[at-1]; --at;
        }
        order[at]=i; ++count;
    }
    for (unsigned i=0;i<count;++i) {
        const unsigned char *s=slots+order[i]*0x70; unsigned char name[32];
        if (i && jc_chirp_u32(s+0x50)==jc_chirp_u32(slots+order[i-1]*0x70+0x50)) return JC_CHIRP_CHANGED;
        uint32_t index=jc_chirp_u32(s+0x48);
        if (index>=sound_count || !jc_layout_field(read,ctx,batch->sounds,0x70234+(uint64_t)index*0x13c,name,32)) return JC_CHIRP_READ;
        struct jc_chirp_source *source=batch->source+batch->count;
        memset(source,0,sizeof(*source)); source->event.participant=participant; source->event.epoch=epoch;
        source->event.sequence=capture->sequence+batch->count+1;
        source->event.sound=jc_chirp_sound_kind(name);
        memcpy(source->event.position,s+0x20,12); memcpy(source->event.velocity,s+0x30,12);
        memcpy(&source->event.radius,s+0x40,4); memcpy(&source->event.duration,s+0x44,4);
        if (!jc_chirp_event_valid(&source->event)) { ++batch->dropped; continue; }
        source->slot=order[i]; source->id=jc_chirp_u32(s+0x50); memcpy(source->native,s+0x20,0x38); ++batch->count;
    }
    uint64_t b,s; unsigned char again[JC_CHIRP_BARN_BYTES];
    code=jc_chirp_snapshot(read,ctx,game,&b,&s,again); if (code) return code;
    if (b!=batch->barn || s!=batch->sounds || memcmp(slots,again,sizeof(slots))) return JC_CHIRP_CHANGED;
    capture->next=next; capture->frame=frame; capture->sequence+=batch->count;
    return 0;
}
static const unsigned char jc_chirp_enqueue_code[16]={0x40,0x56,0x57,0x48,0x81,0xec,0xa8,0,0,0,0x33,0xc0,0x48,0x8b,0xf9,0x48};
static inline int jc_chirp_hook_entry_read(jc_layout_reader read,void *ctx,uint64_t image,uint64_t original,void *out) {
    if(!original)return read(ctx,image+0x13fbb0,out,sizeof(jc_chirp_enqueue_code));
    return read(ctx,original,out,10) && read(ctx,image+0x13fbba,(unsigned char *)out+10,6);
}
static inline int jc_chirp_bridge_prepare_event(jc_layout_reader read,jc_chirp_writable writable,void *ctx,
        uint64_t image,uint64_t game,uint64_t frame,
        const struct jc_chirp_event *event,struct jc_chirp_bridge *bridge) {
    unsigned char bytes[32]; int code;
    if (!bridge || !writable || !frame || !jc_chirp_event_valid(event)) return JC_CHIRP_ARGUMENT;
    memset(bridge,0,sizeof(*bridge)); bridge->game=game; bridge->frame=frame;
    code=jc_chirp_snapshot(read,ctx,game,&bridge->barn,&bridge->sounds,bridge->before); if (code) return code;
    bridge->counter=jc_chirp_u32(bridge->before+0x714); bridge->slot=JC_CHIRP_SLOTS;
    for (unsigned i=0;i<JC_CHIRP_SLOTS;++i) if (!bridge->before[i*0x70+0x68]) { bridge->slot=i; break; }
    if (bridge->slot==JC_CHIRP_SLOTS) return JC_CHIRP_FULL;
    if (!jc_layout_field(read,ctx,image,0x13fbb0,bytes,16)) return JC_CHIRP_READ;
    if (memcmp(bytes,jc_chirp_enqueue_code,16)) return JC_CHIRP_CODE;
    if (!jc_layout_field(read,ctx,bridge->sounds,0x7c7b4,&bridge->sound_count,4)) return JC_CHIRP_READ;
    if (!bridge->sound_count || bridge->sound_count>JC_CHIRP_SOUND_CAP) return JC_CHIRP_SOUND;
    unsigned matches=0;
    for (unsigned i=0;i<bridge->sound_count;++i) {
        if (!jc_layout_field(read,ctx,bridge->sounds,0x70234+(uint64_t)i*0x13c,bytes,32)) return JC_CHIRP_READ;
        if (memchr(bytes,0,32) && !strcmp((const char *)bytes,jc_chirp_sound_name(event->sound,1))) {
            bridge->sound_index=i; memcpy(bridge->name,bytes,32); ++matches;
        }
    }
    if (matches!=1) return JC_CHIRP_SOUND;
    if (!writable(ctx,bridge->barn,JC_CHIRP_BARN_BYTES)) return JC_CHIRP_WRITE;
    memcpy(bridge->input,event->position,12); memcpy(bridge->input+0x10,event->velocity,12);
    memcpy(bridge->input+0x20,&event->radius,4); memcpy(bridge->input+0x24,&event->duration,4);
    memcpy(bridge->input+0x28,&bridge->sound_index,4);
    float one=1; memcpy(bridge->input+12,&one,4);
    uint64_t b,sp; unsigned char again[JC_CHIRP_BARN_BYTES]; uint32_t n;
    code=jc_chirp_snapshot(read,ctx,game,&b,&sp,again); if (code) return code;
    if (b!=bridge->barn || sp!=bridge->sounds || memcmp(again,bridge->before,sizeof(again))) return JC_CHIRP_CHANGED;
    if (!jc_layout_field(read,ctx,sp,0x7c7b4,&n,4) ||
        !jc_layout_field(read,ctx,sp,0x70234+(uint64_t)bridge->sound_index*0x13c,bytes,32)) return JC_CHIRP_READ;
    return n!=bridge->sound_count || memcmp(bytes,bridge->name,32) ? JC_CHIRP_CHANGED : 0;
}
static inline int jc_chirp_bridge_prepare(jc_layout_reader read,jc_chirp_writable writable,void *ctx,
        uint64_t image,uint64_t game,uint64_t frame,const struct jc_chirp_batch *batch,unsigned source_index,
        const struct jc_chirp_event *event,struct jc_chirp_bridge *bridge) {
    if(!batch || !bridge || frame!=batch->frame || batch->count>JC_CHIRP_SLOTS || source_index>=batch->count ||
        !jc_chirp_event_valid(event))return JC_CHIRP_ARGUMENT;
    const struct jc_chirp_source *source=batch->source+source_index;
    if(source->slot>=JC_CHIRP_SLOTS || event->epoch!=source->event.epoch || event->sequence!=source->event.sequence ||
        event->sound!=source->event.sound || event->radius!=source->event.radius || event->duration!=source->event.duration ||
        memcmp(event->velocity,source->event.velocity,sizeof(event->velocity)))return JC_CHIRP_ARGUMENT;
    int code=jc_chirp_bridge_prepare_event(read,writable,ctx,image,game,frame,event,bridge);
    if(code)return code;
    const unsigned char *slot=bridge->before+source->slot*0x70;
    return bridge->barn!=batch->barn || bridge->sounds!=batch->sounds || slot[0x68]!=1 || slot[0x4c]!=1 || slot[0x4d] ||
        jc_chirp_u32(slot+0x50)!=source->id || memcmp(slot+0x20,source->native,0x38) ? JC_CHIRP_CHANGED : 0;
}
static inline int jc_chirp_bridge_post(jc_layout_reader read,void *ctx,uint64_t frame,const struct jc_chirp_bridge *bridge) {
    uint64_t barn,sounds; unsigned char after[JC_CHIRP_BARN_BYTES],expected[JC_CHIRP_BARN_BYTES];
    if (!bridge || frame!=bridge->frame || bridge->slot>=JC_CHIRP_SLOTS) return JC_CHIRP_ARGUMENT;
    int code=jc_chirp_snapshot(read,ctx,bridge->game,&barn,&sounds,after); if (code) return code;
    if (barn!=bridge->barn || sounds!=bridge->sounds) return JC_CHIRP_CHANGED;
    memcpy(expected,bridge->before,sizeof(expected)); unsigned at=bridge->slot*0x70;
    expected[at+0x68]=expected[at+0x78]=1;
    memcpy(expected+at+0x20,bridge->input,0x2e);
    memcpy(expected+at+0x50,&bridge->counter,4); memcpy(expected+at+0x54,bridge->input+0x34,4);
    uint32_t next=bridge->counter+1; memcpy(expected+0x714,&next,4);
    return memcmp(expected,after,sizeof(after)) ? JC_CHIRP_POSTCHECK : 0;
}
struct jc_chirp_relay {
    struct jc_chirp_capture capture;
    struct jc_chirp_delivery delivery;
    uint64_t game,participant,epoch;
    unsigned emitted,dropped,stopped;
};
typedef void (*jc_chirp_emit)(void *,const struct jc_chirp_bridge *);
static inline int jc_chirp_deliver_event(jc_layout_reader read,jc_chirp_writable writable,jc_chirp_emit emit,
        void *ctx,uint64_t image,uint64_t game,uint64_t frame,const struct jc_chirp_event *event,
        struct jc_chirp_delivery *delivery,unsigned native_peer) {
    if(!emit || native_peer>1 || !jc_chirp_delivery_fresh(delivery,event))return JC_CHIRP_ARGUMENT;
    struct jc_chirp_bridge bridge;
    int code=jc_chirp_bridge_prepare_event(read,writable,ctx,image,game,frame,event,&bridge);
    if(code)return code;
    bridge.input[0x2d]=(unsigned char)native_peer;
    emit(ctx,&bridge);jc_chirp_delivery_commit(delivery,event);
    return jc_chirp_bridge_post(read,ctx,frame,&bridge);
}

static inline int jc_chirp_relay_tick_role(jc_layout_reader read,jc_chirp_writable writable,
        jc_chirp_emit emit,void *ctx,uint64_t image,uint64_t game,uint64_t frame,
        uint64_t participant,uint64_t epoch,const float position[3],struct jc_chirp_relay *relay,unsigned native_peer) {
    if (!read || !writable || !emit || !relay || !position || !participant || !epoch || !frame ||
        relay->stopped || native_peer>1) return JC_CHIRP_ARGUMENT;
    for (unsigned i=0;i<3;++i) if (!isfinite(position[i]) || fabsf(position[i])>1000000) return JC_CHIRP_RANGE;
    if (relay->game && (relay->game!=game || relay->participant!=participant || relay->epoch!=epoch))
        return JC_CHIRP_CHANGED;
    relay->game=game;relay->participant=participant;relay->epoch=epoch;
    struct jc_chirp_batch batch;
    int code=jc_chirp_capture_local(read,ctx,game,frame,participant,epoch,&relay->capture,&batch);
    if (code) {relay->stopped=1;return code;}
    relay->dropped+=batch.dropped;
    for (unsigned i=0;i<batch.count;++i) {
        struct jc_chirp_event event=batch.source[i].event;
        memcpy(event.position,position,sizeof(event.position));
        if (!jc_chirp_delivery_fresh(&relay->delivery,&event)) continue;
        struct jc_chirp_bridge bridge;
        code=jc_chirp_bridge_prepare(read,writable,ctx,image,game,frame,&batch,i,&event,&bridge);
        if (code) {
            relay->dropped+=batch.count-i;
            if (code!=JC_CHIRP_FULL) relay->stopped=1;
            return code;
        }
        bridge.input[0x2d]=(unsigned char)native_peer;
        emit(ctx,&bridge);
        jc_chirp_delivery_commit(&relay->delivery,&event);++relay->emitted;
        code=jc_chirp_bridge_post(read,ctx,frame,&bridge);
        if (code) {relay->stopped=1;return code;}
    }
    return 0;
}
static inline int jc_chirp_relay_tick(jc_layout_reader read,jc_chirp_writable writable,
        jc_chirp_emit emit,void *ctx,uint64_t image,uint64_t game,uint64_t frame,
        uint64_t participant,uint64_t epoch,const float position[3],struct jc_chirp_relay *relay) {
    return jc_chirp_relay_tick_role(read,writable,emit,ctx,image,game,frame,participant,epoch,position,relay,0);
}
#endif
