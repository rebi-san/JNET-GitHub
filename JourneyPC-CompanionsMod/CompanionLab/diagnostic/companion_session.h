#ifndef JC_COMPANION_SESSION_H
#define JC_COMPANION_SESSION_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <float.h>
_Static_assert(sizeof(float)==4 && FLT_RADIX==2 && FLT_MANT_DIG==24,"IEEE binary32 required");

#define JC_SESSION_MAX_MEMBERS 16u
#define JC_SESSION_HEADER 64u
#define JC_SESSION_STATE_BYTES 92u
#define JC_SESSION_BURST 120u

enum jc_session_result {
    JC_SESSION_OK, JC_SESSION_ARGUMENT, JC_SESSION_FULL, JC_SESSION_MEMBER,
    JC_SESSION_FORMAT, JC_SESSION_VERSION, JC_SESSION_ROOM, JC_SESSION_OWNER,
    JC_SESSION_REPLAY, JC_SESSION_CHAPTER, JC_SESSION_VALUE,
    JC_SESSION_RATE, JC_SESSION_CLOCK, JC_SESSION_EXHAUSTED
};
enum jc_session_message { JC_SESSION_TRANSITION=1, JC_SESSION_STATE=2 };
struct jc_motion { float position[3],velocity[3],heading; };
struct jc_member {
    uint64_t id,lease,sequence,chapter_epoch,last_seen,rate_start;
    uint32_t chapter,attempts;
    unsigned active,has_chapter,has_state;
    struct jc_motion motion;
};
struct jc_session {
    unsigned char room[16];
    uint64_t local_id,next_lease,clock;
    unsigned limit;
    struct jc_member members[JC_SESSION_MAX_MEMBERS-1];
};
struct jc_session_packet {
    uint64_t id,lease,sequence,chapter_epoch;
    uint32_t chapter;
    unsigned type;
    struct jc_motion motion;
};
static inline uint64_t jc_session_get(const unsigned char *p,unsigned bytes) {
    uint64_t v=0;
    for (unsigned i=0;i<bytes;++i) v|=(uint64_t)p[i]<<(8*i);
    return v;
}
static inline void jc_session_put(unsigned char *p,uint64_t v,unsigned bytes) {
    for (unsigned i=0;i<bytes;++i) p[i]=(unsigned char)(v>>(8*i));
}
static inline int jc_motion_valid(const struct jc_motion *m) {
    for (unsigned i=0;i<3;++i)
        if (!isfinite(m->position[i]) || fabsf(m->position[i])>1000000.0f ||
            !isfinite(m->velocity[i]) || fabsf(m->velocity[i])>10000.0f) return 0;
    return isfinite(m->heading) && fabsf(m->heading)<=6.283186f;
}
static inline int jc_session_init(struct jc_session *s,const unsigned char room[16],
                                   uint64_t local_id,unsigned total_limit,uint64_t now) {
    if (!s || !room || !local_id || total_limit<2 || total_limit>JC_SESSION_MAX_MEMBERS)
        return JC_SESSION_ARGUMENT;
    unsigned any=0; for (unsigned i=0;i<16;++i) any|=room[i];
    if (!any) return JC_SESSION_ARGUMENT;
    memset(s,0,sizeof(*s)); memcpy(s->room,room,16);
    s->local_id=local_id; s->limit=total_limit; s->clock=now;
    return JC_SESSION_OK;
}
static inline struct jc_member *jc_session_find(struct jc_session *s,uint64_t id) {
    if (!s || s->limit<2 || s->limit>JC_SESSION_MAX_MEMBERS) return NULL;
    for (unsigned i=0;i<s->limit-1;++i)
        if (s->members[i].active && s->members[i].id==id) return &s->members[i];
    return NULL;
}
static inline int jc_session_clock(struct jc_session *s,uint64_t now) {
    if (!s || s->limit<2 || s->limit>JC_SESSION_MAX_MEMBERS) return JC_SESSION_ARGUMENT;
    if (now<s->clock) return JC_SESSION_CLOCK;
    s->clock=now; return JC_SESSION_OK;
}
/* Admission is called only by a trusted membership adapter, never by a packet. */
static inline int jc_session_join(struct jc_session *s,uint64_t id,uint64_t now,uint64_t *lease) {
    if (lease) *lease=0;
    if (!s || !lease || !id || id==s->local_id) return JC_SESSION_ARGUMENT;
    int code=jc_session_clock(s,now); if (code) return code;
    if (jc_session_find(s,id)) return JC_SESSION_MEMBER;
    if (s->next_lease==UINT64_MAX) return JC_SESSION_EXHAUSTED;
    for (unsigned i=0;i<s->limit-1;++i) if (!s->members[i].active) {
        struct jc_member m={0}; m.id=id; m.lease=++s->next_lease;
        m.active=1; m.last_seen=m.rate_start=now;
        s->members[i]=m; *lease=m.lease; return JC_SESSION_OK;
    }
    return JC_SESSION_FULL;
}
static inline int jc_session_leave(struct jc_session *s,uint64_t id,uint64_t lease) {
    struct jc_member *m=jc_session_find(s,id);
    if (!m || m->lease!=lease) return JC_SESSION_MEMBER;
    memset(m,0,sizeof(*m)); return JC_SESSION_OK;
}
static inline int jc_session_encode(const unsigned char room[16],const struct jc_session_packet *v,
                                     unsigned char *data,size_t capacity,size_t *length) {
    if (length) *length=0;
    if (!room || !v || !data || !length || !v->id || !v->lease || !v->sequence ||
        !v->chapter_epoch || v->chapter>8 ||
        (v->type!=JC_SESSION_STATE && v->type!=JC_SESSION_TRANSITION)) return JC_SESSION_ARGUMENT;
    size_t size=v->type==JC_SESSION_STATE ? JC_SESSION_STATE_BYTES : JC_SESSION_HEADER;
    if (capacity<size) return JC_SESSION_ARGUMENT;
    if (v->type==JC_SESSION_STATE && !jc_motion_valid(&v->motion)) return JC_SESSION_VALUE;
    memset(data,0,size); memcpy(data,"JNS1",4);
    jc_session_put(data+4,1,2); jc_session_put(data+6,v->type,2);
    jc_session_put(data+8,size,2); memcpy(data+12,room,16);
    jc_session_put(data+28,v->id,8); jc_session_put(data+36,v->lease,8);
    jc_session_put(data+44,v->sequence,8); jc_session_put(data+52,v->chapter_epoch,8);
    jc_session_put(data+60,v->chapter,4);
    if (v->type==JC_SESSION_STATE) for (unsigned i=0;i<7;++i) {
        float f=i<3 ? v->motion.position[i] : i<6 ? v->motion.velocity[i-3] : v->motion.heading;
        uint32_t bits; memcpy(&bits,&f,4); jc_session_put(data+64+4*i,bits,4);
    }
    *length=size; return JC_SESSION_OK;
}
/* bound_id/lease must come from authenticated transport state, not this header. */
static inline int jc_session_receive(struct jc_session *s,uint64_t bound_id,uint64_t bound_lease,
                                      const unsigned char *data,size_t size,uint64_t now) {
    int code=jc_session_clock(s,now); if (code) return code;
    struct jc_member *m=jc_session_find(s,bound_id);
    if (!m || !bound_lease || m->lease!=bound_lease) return JC_SESSION_MEMBER;
    if (now-m->rate_start>=1000) { m->rate_start=now; m->attempts=0; }
    if (m->attempts>=JC_SESSION_BURST) return JC_SESSION_RATE;
    ++m->attempts;
    if (!data || (size!=JC_SESSION_HEADER && size!=JC_SESSION_STATE_BYTES)) return JC_SESSION_FORMAT;
    if (memcmp(data,"JNS1",4) || jc_session_get(data+8,2)!=size || jc_session_get(data+10,2))
        return JC_SESSION_FORMAT;
    if (jc_session_get(data+4,2)!=1) return JC_SESSION_VERSION;
    if (memcmp(s->room,data+12,16)) return JC_SESSION_ROOM;
    if (jc_session_get(data+28,8)!=bound_id || jc_session_get(data+36,8)!=bound_lease)
        return JC_SESSION_OWNER;
    unsigned type=(unsigned)jc_session_get(data+6,2);
    if ((type!=JC_SESSION_TRANSITION && type!=JC_SESSION_STATE) ||
        size!=(type==JC_SESSION_STATE ? JC_SESSION_STATE_BYTES : JC_SESSION_HEADER)) return JC_SESSION_FORMAT;
    uint64_t seq=jc_session_get(data+44,8),epoch=jc_session_get(data+52,8);
    uint32_t chapter=(uint32_t)jc_session_get(data+60,4);
    if (!seq || seq<=m->sequence) return JC_SESSION_REPLAY;
    if (!epoch || chapter>8) return JC_SESSION_CHAPTER;
    struct jc_member next=*m;
    if (type==JC_SESSION_TRANSITION) {
        if (epoch<=m->chapter_epoch) return JC_SESSION_CHAPTER;
        next.has_chapter=1; next.has_state=0; next.chapter=chapter; next.chapter_epoch=epoch;
        memset(&next.motion,0,sizeof(next.motion));
    } else {
        if (!m->has_chapter || epoch!=m->chapter_epoch || chapter!=m->chapter) return JC_SESSION_CHAPTER;
        for (unsigned i=0;i<7;++i) {
            uint32_t bits=(uint32_t)jc_session_get(data+64+4*i,4); float f; memcpy(&f,&bits,4);
            if (i<3) next.motion.position[i]=f;
            else if (i<6) next.motion.velocity[i-3]=f;
            else next.motion.heading=f;
        }
        if (!jc_motion_valid(&next.motion)) return JC_SESSION_VALUE;
        next.has_state=1;
    }
    next.sequence=seq; next.last_seen=now; *m=next;
    return JC_SESSION_OK;
}
static inline const struct jc_motion *jc_session_motion(struct jc_session *s,uint64_t id,
                                                        uint64_t lease,uint32_t local_chapter) {
    struct jc_member *m=jc_session_find(s,id);
    return m && m->lease==lease && m->has_state && m->chapter==local_chapter ? &m->motion : NULL;
}
/* Caller consumes each expired identity for entity cleanup before admitting replacements. */
static inline int jc_session_expire_one(struct jc_session *s,uint64_t now,uint64_t timeout,
                                         uint64_t *id,uint64_t *lease) {
    if (!id || !lease || !timeout) return JC_SESSION_ARGUMENT;
    *id=*lease=0;
    int code=jc_session_clock(s,now); if (code) return code;
    for (unsigned i=0;i<s->limit-1;++i) {
        struct jc_member *m=&s->members[i];
        if (m->active && now-m->last_seen>=timeout) {
            *id=m->id; *lease=m->lease; memset(m,0,sizeof(*m)); return JC_SESSION_OK;
        }
    }
    return JC_SESSION_MEMBER;
}
#endif
