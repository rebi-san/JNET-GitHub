#ifndef JC_COMPANION_ROOM_H
#define JC_COMPANION_ROOM_H
#include "companion_pose.h"
struct jc_pose_room {
    struct jc_pose_receiver peers[2];
    uint64_t clock,rate_start;
    unsigned attempts,host,selected,received[2];
    uint16_t host_port;
};
static inline void jc_room_init(struct jc_pose_room *r,unsigned host,const unsigned char own[16],uint16_t port) {
    memset(r,0,sizeof(*r)); r->host=host; r->host_port=port; r->selected=2;
    for (unsigned i=0;i<2;++i) {
        memcpy(r->peers[i].own_identity,own,16);
        r->peers[i].required_flags=JC_POSE_ROOM;
        if (!host) r->peers[i].peer_port=port;
    }
}
static inline int jc_room_receive(struct jc_pose_room *r,const unsigned char build[16],
    const unsigned char *packet,size_t length,uint32_t chapter,uint64_t now,uint16_t port,
    jc_pose_mac mac,void *ctx,unsigned *accepted) {
    *accepted=2;
    if (now<r->clock) return 1;
    r->clock=now;
    if (now-r->rate_start>=1000) { r->attempts=0; r->rate_start=now; }
    if (r->attempts>=32) return 2;
    ++r->attempts;
    if (!packet || length!=JC_POSE_BYTES) return 3;
    unsigned flags=(unsigned)jc_session_get(packet+52,4);
    if (!(flags&JC_POSE_ROOM) || (flags&~JC_POSE_FLAGS)) return 3;
    if (!port || (!r->host && port!=r->host_port)) return 5;
    unsigned slot=2;
    if (r->host) {
        if ((flags&JC_POSE_ROLE_FLAGS)!=JC_POSE_ROOM) return 5;
        for (unsigned i=0;i<2;++i)
            if (r->peers[i].paired && !memcmp(r->peers[i].sample.identity,packet+24,16)) slot=i;
        if (slot==2) {
            for (unsigned i=0;i<2;++i) if (!r->peers[i].paired) { slot=i; break; }
        }
        if (slot==2) return 10;
        for (unsigned i=0;i<2;++i)
            if (i!=slot && r->peers[i].paired && r->peers[i].peer_port==port) return 5;
    } else {
        slot=(flags&JC_POSE_DUPLEX) ? 0 : 1;
        unsigned other=1-slot;
        if (r->peers[other].paired && !memcmp(r->peers[other].sample.identity,packet+24,16)) return 5;
    }
    int code=jc_pose_receive_from(&r->peers[slot],build,packet,length,chapter,now,port,mac,ctx);
    if (!code || code==8) {
        *accepted=slot; ++r->received[slot];
        if (r->selected==2) r->selected=slot;
    }
    return code;
}
static inline uint16_t jc_room_relay_port(const struct jc_pose_room *r,unsigned accepted) {
    if (!r->host || accepted>=2 || !r->peers[accepted].paired || !r->peers[1-accepted].paired ||
        r->peers[1-accepted].departed) return 0;
    return r->peers[1-accepted].peer_port;
}
static inline int jc_room_next(struct jc_pose_room *r) {
    for (unsigned step=1;step<=2;++step) {
        unsigned slot=(r->selected+step)%2;
        if (r->peers[slot].paired) { r->selected=slot; return 1; }
    }
    return 0;
}
static inline int jc_room_selected(const struct jc_pose_room *r,struct jc_pose_receiver *out) {
    if (r->selected>=2 || !r->peers[r->selected].paired) { memset(out,0,sizeof(*out)); return 0; }
    *out=r->peers[r->selected]; return 1;
}
/* Winsock would-block, UDP reset/refusal: no evidence of an authenticated departure. */
static inline int jc_room_io_defer(unsigned mode,int error) {
    return error==10035 || (mode>=4 && (error==10054 || error==10061));
}
#endif
