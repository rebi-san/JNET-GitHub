#ifndef JC_COMPANION_POSE_H
#define JC_COMPANION_POSE_H
#include "companion_session.h"
#include "companion_animation.h"
#define JC_POSE_BYTES 568u
#define JC_POSE_BODY 536u
#define JC_POSE_TIMEOUT 5000u
#define JC_POSE_DUPLEX 1u
#define JC_POSE_ROOM 2u
#define JC_POSE_LEAVE 4u
#define JC_POSE_APPEARANCE 8u
#define JC_POSE_ROLE_FLAGS (JC_POSE_DUPLEX | JC_POSE_ROOM)
#define JC_POSE_FLAGS (JC_POSE_ROLE_FLAGS | JC_POSE_LEAVE | JC_POSE_APPEARANCE)
struct jc_pose_sample { unsigned char identity[16]; uint64_t sequence; uint32_t chapter,flags; float transform[16]; struct jc_anim_sample animation; uint32_t appearance[4]; };
struct jc_pose_receiver {
    struct jc_pose_sample sample;
    uint64_t clock,last_seen,rate_start;
    unsigned attempts,paired,ready,departed;
    unsigned char own_identity[16];
    unsigned required_flags;
    uint16_t peer_port;
};
typedef int (*jc_pose_mac)(void *,const unsigned char *,size_t,unsigned char[32]);
static inline int jc_pose_transform_valid(const float m[16]) {
    for (unsigned i=0;i<16;++i) if (!isfinite(m[i]) || fabsf(m[i])>(i>=12 && i<=14 ? 1000000.0f : 4.0f)) return 0;
    if (fabsf(m[3])>1e-5f || fabsf(m[7])>1e-5f || fabsf(m[11])>1e-5f || fabsf(m[15]-1)>1e-5f) return 0;
    double det=(double)m[0]*(m[5]*m[10]-m[6]*m[9])-(double)m[4]*(m[1]*m[10]-m[2]*m[9])+(double)m[8]*(m[1]*m[6]-m[2]*m[5]);
    return isfinite(det) && fabs(det)>1e-8 && fabs(det)<64;
}
/* Native +68/+6c/+70/+74: scarf current/capacity, outfit 0..6, symbol 0..20.
   Absence is explicit; it must never be replaced with the receiving host's values. */
static inline int jc_pose_appearance_valid(unsigned flags,const uint32_t a[4]) {
    if (!(flags&JC_POSE_APPEARANCE)) return !(a[0]|a[1]|a[2]|a[3]);
    return a[0]<=a[1] && a[1]<=4096 && a[2]<=6 && a[3]<=20;
}
static inline int jc_pose_encode(const unsigned char build[16],const struct jc_pose_sample *s,
                                 jc_pose_mac mac,void *ctx,unsigned char out[JC_POSE_BYTES]) {
    unsigned any=0; for (unsigned i=0;i<16;++i) any|=s->identity[i];
    if (!any || !s->sequence || s->chapter<1 || s->chapter>6 || (s->flags&~JC_POSE_FLAGS) || !jc_pose_transform_valid(s->transform) || !jc_anim_valid(&s->animation) || !jc_pose_appearance_valid(s->flags,s->appearance)) return 0;
    if ((s->flags&JC_POSE_LEAVE) && !(s->flags&JC_POSE_ROOM)) return 0;
    memset(out,0,JC_POSE_BYTES); memcpy(out,"JNP5",4); jc_session_put(out+4,5,2);
    jc_session_put(out+6,JC_POSE_BYTES,2); memcpy(out+8,build,16); memcpy(out+24,s->identity,16);
    jc_session_put(out+40,s->sequence,8); jc_session_put(out+48,s->chapter,4);
    jc_session_put(out+52,s->flags,4);
    for (unsigned i=0;i<16;++i) { uint32_t bits; memcpy(&bits,s->transform+i,4); jc_session_put(out+56+4*i,bits,4); }
    jc_session_put(out+120,s->animation.schema,8); jc_session_put(out+128,s->animation.total,4);
    jc_session_put(out+132,s->animation.count,4);
    for (unsigned i=0;i<s->animation.count;++i) {
        const struct jc_anim_clip *c=s->animation.clips+i; unsigned at=136+i*24;
        jc_session_put(out+at,c->index,4); jc_session_put(out+at+4,c->additive,4);
        const float values[4]={c->frame,c->duration,c->speed,c->weight};
        for (unsigned j=0;j<4;++j) { uint32_t bits; memcpy(&bits,values+j,4); jc_session_put(out+at+8+j*4,bits,4); }
    }
    for (unsigned i=0;i<4;++i) jc_session_put(out+520+i*4,s->appearance[i],4);
    return mac && mac(ctx,out,JC_POSE_BODY,out+JC_POSE_BODY);
}
/* MAC callback is supplied by BCrypt in the Windows adapter. No unauthenticated data reaches the sample. */
static inline int jc_pose_receive(struct jc_pose_receiver *r,const unsigned char build[16],
    const unsigned char *packet,size_t length,uint32_t chapter,uint64_t now,jc_pose_mac mac,void *ctx) {
    if (now<r->clock) return 1;
    r->clock=now;
    if (now-r->rate_start>=1000) { r->attempts=0; r->rate_start=now; }
    if (r->attempts>=32) return 2;
    ++r->attempts;
    if (!packet || length!=JC_POSE_BYTES || memcmp(packet,"JNP5",4) ||
        jc_session_get(packet+4,2)!=5 || jc_session_get(packet+6,2)!=JC_POSE_BYTES ||
        memcmp(packet+8,build,16) || (jc_session_get(packet+52,4)&~JC_POSE_FLAGS)) return 3;
    unsigned char tag[32]; unsigned difference=0;
    if (!mac || !mac(ctx,packet,JC_POSE_BODY,tag)) return 4;
    for (unsigned i=0;i<32;++i) difference|=tag[i]^packet[JC_POSE_BODY+i];
    if (difference) return 4;
    struct jc_pose_sample next={0}; unsigned any=0;
    memcpy(next.identity,packet+24,16); for (unsigned i=0;i<16;++i) any|=next.identity[i];
    next.flags=(uint32_t)jc_session_get(packet+52,4);
    if (!any || !memcmp(next.identity,r->own_identity,16) ||
        (next.flags&r->required_flags)!=r->required_flags ||
        (r->paired && (memcmp(next.identity,r->sample.identity,16) ||
         (next.flags&JC_POSE_ROLE_FLAGS)!=(r->sample.flags&JC_POSE_ROLE_FLAGS)))) return 5;
    if ((next.flags&JC_POSE_LEAVE) && (!(next.flags&JC_POSE_ROOM) || !r->paired)) return 5;
    if (r->departed) return 11;
    next.sequence=jc_session_get(packet+40,8); next.chapter=(uint32_t)jc_session_get(packet+48,4);
    if (!next.sequence || (r->paired && next.sequence<=r->sample.sequence)) return 6;
    for (unsigned i=0;i<16;++i) { uint32_t bits=(uint32_t)jc_session_get(packet+56+4*i,4); memcpy(next.transform+i,&bits,4); }
    if (next.chapter<1 || next.chapter>6 || !jc_pose_transform_valid(next.transform)) return 7;
    next.animation.schema=jc_session_get(packet+120,8); next.animation.total=(uint32_t)jc_session_get(packet+128,4);
    next.animation.count=(uint32_t)jc_session_get(packet+132,4);
    if (next.animation.count>JC_ANIM_ACTIVE) return 7;
    for (unsigned i=0;i<next.animation.count;++i) {
        struct jc_anim_clip *c=next.animation.clips+i; unsigned at=136+i*24;
        c->index=(uint32_t)jc_session_get(packet+at,4); c->additive=(uint32_t)jc_session_get(packet+at+4,4);
        float values[4];
        for (unsigned j=0;j<4;++j) { uint32_t bits=(uint32_t)jc_session_get(packet+at+8+j*4,4); memcpy(values+j,&bits,4); }
        c->frame=values[0]; c->duration=values[1]; c->speed=values[2]; c->weight=values[3];
    }
    for (unsigned i=0;i<4;++i) next.appearance[i]=(uint32_t)jc_session_get(packet+520+i*4,4);
    if (!jc_anim_valid(&next.animation) || !jc_pose_appearance_valid(next.flags,next.appearance)) return 7;
    r->sample=next; r->paired=1; r->last_seen=now;
    r->departed=(next.flags&JC_POSE_LEAVE)!=0;
    r->ready=!r->departed && next.chapter==chapter;
    if (r->departed) return 0;
    return r->ready ? 0 : 8;
}
static inline int jc_pose_receive_from(struct jc_pose_receiver *r,const unsigned char build[16],
    const unsigned char *packet,size_t length,uint32_t chapter,uint64_t now,uint16_t port,
    jc_pose_mac mac,void *ctx) {
    if (!port || (r->peer_port && port!=r->peer_port)) return 5;
    int code=jc_pose_receive(r,build,packet,length,chapter,now,mac,ctx);
    if (!code || code==8) r->peer_port=port;
    return code;
}
static inline int jc_pose_fresh(const struct jc_pose_receiver *r,uint32_t chapter,uint64_t now) {
    return r->ready && r->paired && !r->departed && r->sample.chapter==chapter && now>=r->last_seen && now-r->last_seen<JC_POSE_TIMEOUT;
}
static inline unsigned jc_pose_presence(const struct jc_pose_receiver *r,uint32_t chapter,uint64_t now) {
    if (!r->paired) return 0;
    if (r->departed) return 3;
    if (r->sample.chapter!=chapter) return 4;
    return jc_pose_fresh(r,chapter,now) ? 1 : 2;
}
#endif
