#ifndef JC_COMPANION_CONNECTION_H
#define JC_COMPANION_CONNECTION_H
#include "companion_actor_packet.h"
#define JC_CONNECTION_HELLO_BYTES 112u
struct jc_connection {
    uint64_t local,peer,epoch,remote_epoch,candidate_epoch,sequence,last_seen;
    unsigned char nonce[16],remote_nonce[16],candidate_nonce[16];
    unsigned admitted;
};
static inline int jc_connection_hello(const struct jc_connection *c,const unsigned char build[16],
        jc_actor_packet_mac mac,void *ctx,unsigned char out[JC_CONNECTION_HELLO_BYTES]) {
    if(!c || !c->local || !c->peer || c->local==c->peer || !c->epoch || !mac)return 0;
    memset(out,0,JC_CONNECTION_HELLO_BYTES);memcpy(out,"JNH1",4);
    jc_packet_put(out+4,JC_CONNECTION_HELLO_BYTES,4);memcpy(out+8,build,16);
    jc_packet_put(out+24,c->local,8);jc_packet_put(out+32,c->peer,8);jc_packet_put(out+40,c->epoch,8);
    memcpy(out+48,c->nonce,16);memcpy(out+64,c->candidate_nonce,16);
    return mac(ctx,out,80,out+80);
}
static inline int jc_connection_receive(struct jc_connection *c,const unsigned char build[16],
        const unsigned char *p,size_t bytes,jc_actor_packet_mac mac,void *ctx) {
    if(!c || !p || bytes!=JC_CONNECTION_HELLO_BYTES || memcmp(p,"JNH1",4) ||
        jc_packet_get(p+4,4)!=bytes || memcmp(p+8,build,16) || !mac)return JC_STREAM_ARGUMENT;
    unsigned char tag[32];if(!mac(ctx,p,80,tag))return JC_STREAM_ARGUMENT;
    unsigned diff=0,any=0;for(unsigned i=0;i<32;++i)diff|=tag[i]^p[80+i];
    for(unsigned i=0;i<16;++i)any|=p[48+i];
    uint64_t epoch=jc_packet_get(p+40,8);
    if(diff || !any || !epoch || jc_packet_get(p+24,8)!=c->peer || jc_packet_get(p+32,8)!=c->local)
        return JC_STREAM_IDENTITY;
    if(epoch<c->remote_epoch || (epoch==c->remote_epoch && memcmp(p+48,c->remote_nonce,16)))return JC_STREAM_REPLAY;
    if(epoch<c->candidate_epoch || (epoch==c->candidate_epoch && memcmp(p+48,c->candidate_nonce,16)))return JC_STREAM_REPLAY;
    c->candidate_epoch=epoch;memcpy(c->candidate_nonce,p+48,16);
    if(memcmp(p+64,c->nonce,16))return JC_STREAM_HELD;
    if(!c->admitted || epoch!=c->remote_epoch) {
        c->remote_epoch=epoch;memcpy(c->remote_nonce,p+48,16);
        c->sequence=c->last_seen=0;c->admitted=1;
    }
    return 0;
}
static inline int jc_connection_key(const struct jc_connection *c,jc_actor_packet_mac mac,
        void *ctx,unsigned char key[32]) {
    unsigned char context[68]={0};memcpy(context,"JNK1",4);
    unsigned local_first=c->local<c->peer;
    jc_packet_put(context+4,local_first?c->local:c->peer,8);
    jc_packet_put(context+12,local_first?c->peer:c->local,8);
    jc_packet_put(context+20,local_first?c->epoch:c->remote_epoch,8);
    jc_packet_put(context+28,local_first?c->remote_epoch:c->epoch,8);
    memcpy(context+36,local_first?c->nonce:c->remote_nonce,16);
    memcpy(context+52,local_first?c->remote_nonce:c->nonce,16);
    return c->admitted && mac(ctx,context,sizeof(context),key);
}
static inline int jc_connection_state(struct jc_connection *c,const unsigned char build[16],
        const unsigned char *p,size_t bytes,uint64_t now,jc_actor_packet_mac mac,void *ctx,
        struct jc_actor_sender *out) {
    if(!c || !c->admitted)return JC_STREAM_IDENTITY;
    struct jc_actor_sender next;
    int code=jc_actor_packet_decode(&next,build,p,bytes,now,mac,ctx);if(code)return code;
    if(jc_packet_get(p+24,8)!=c->peer || jc_packet_get(p+32,8)!=c->remote_epoch)return JC_STREAM_IDENTITY;
    uint64_t sequence=jc_packet_get(p+40,8);
    if(!sequence || sequence<=c->sequence)return JC_STREAM_REPLAY;
    if(now<c->last_seen)return JC_STREAM_CLOCK;
    c->sequence=sequence;c->last_seen=now;*out=next;return 0;
}
#endif
