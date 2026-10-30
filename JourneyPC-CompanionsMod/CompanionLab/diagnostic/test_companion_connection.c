#include "companion_connection.h"
#include <CommonCrypto/CommonHMAC.h>
#include <assert.h>
#include <stdio.h>
static unsigned char derived_context[68];
static int mac(void *key,const unsigned char *data,size_t bytes,unsigned char tag[32]) {
    if(bytes==sizeof(derived_context))memcpy(derived_context,data,bytes);
    CCHmac(kCCHmacAlgSHA256,key,32,data,bytes,tag);return 1;
}
static const unsigned char build[16]={14,1};
static void exchange(struct jc_connection *a,struct jc_connection *b,unsigned char key[32]) {
    unsigned char hello[JC_CONNECTION_HELLO_BYTES];
    assert(jc_connection_hello(a,build,mac,key,hello));
    assert(!memcmp(hello,"JNH1",4) && jc_packet_get(hello+4,4)==112);
    int result=jc_connection_receive(b,build,hello,sizeof(hello),mac,key);
    assert(!result || result==JC_STREAM_HELD);
}
static void pair(struct jc_connection *a,struct jc_connection *b,unsigned char root[32],unsigned char session[32]) {
    exchange(a,b,root);exchange(b,a,root);exchange(a,b,root);
    assert(a->admitted && b->admitted);
    unsigned char other[32];
    assert(jc_connection_key(a,mac,root,session));assert(jc_connection_key(b,mac,root,other));
    assert(!memcmp(session,other,32));assert(memcmp(session,root,32));
    assert(!memcmp(derived_context,"JNK1",4));
    unsigned char legacy_context[68],legacy_key[32];memcpy(legacy_context,derived_context,68);
    memcpy(legacy_context,"JCK1",4);assert(mac(root,legacy_context,68,legacy_key));
    assert(memcmp(session,legacy_key,32));
}
int main(void) {
    unsigned char root[32]={41},wrong[32]={42},session[32],hello[JC_CONNECTION_HELLO_BYTES];
    struct jc_connection a={.local=1,.peer=2,.epoch=100,.nonce={1,3,5}};
    struct jc_connection b={.local=2,.peer=1,.epoch=200,.nonce={2,4,6}};
    assert(jc_connection_hello(&a,build,mac,root,hello));
    unsigned char legacy_hello[sizeof(hello)];memcpy(legacy_hello,hello,sizeof(hello));
    memcpy(legacy_hello,"JCH1",4);assert(mac(root,legacy_hello,80,legacy_hello+80));
    struct jc_connection unchanged=b;
    assert(jc_connection_receive(&b,build,legacy_hello,sizeof(legacy_hello),mac,root)==JC_STREAM_ARGUMENT);
    assert(!memcmp(&b,&unchanged,sizeof(b)));
    unsigned char different_build[16]={15,1};
    assert(jc_connection_receive(&b,different_build,hello,sizeof(hello),mac,root)==JC_STREAM_ARGUMENT);
    assert(!memcmp(&b,&unchanged,sizeof(b)));
    for(unsigned i=0;i<sizeof(hello);++i) {
        struct jc_connection before=b;unsigned char damaged[sizeof(hello)];memcpy(damaged,hello,sizeof(hello));damaged[i]^=1;
        assert(jc_connection_receive(&b,build,damaged,sizeof(damaged),mac,root));
        assert(!memcmp(&before,&b,sizeof(b)));
        assert(jc_connection_receive(&b,build,hello,i,mac,root));
        assert(!memcmp(&before,&b,sizeof(b)));
    }
    assert(jc_connection_receive(&b,build,hello,sizeof(hello),mac,wrong)==JC_STREAM_IDENTITY);
    assert(jc_connection_receive(&b,build,hello,sizeof(hello),mac,root)==JC_STREAM_HELD);
    assert(!b.admitted && !b.remote_epoch);
    pair(&a,&b,root,session);
    struct jc_actor_sender source={.body_scale=.1f,.chirp_shape={1,1},.chapter=2},received={0};
    source.axes[0]=.7f;source.direction[2]=1;source.scalars[1]=.4f;
    jc_receiver_store(source.state.bytes,123.f);
    unsigned char state[JC_ACTOR_PACKET_BYTES];
    assert(jc_actor_packet_encode(build,1,100,1,&source,mac,session,state));
    assert(!memcmp(state,"JNA1",4) && sizeof(state)==256);
    unsigned char legacy_state[sizeof(state)];memcpy(legacy_state,state,sizeof(state));
    memcpy(legacy_state,"JCA1",4);assert(mac(session,legacy_state,JC_ACTOR_PACKET_BODY,legacy_state+JC_ACTOR_PACKET_BODY));
    unchanged=b;
    assert(jc_connection_state(&b,build,legacy_state,sizeof(legacy_state),1000,mac,session,&received)==JC_STREAM_ARGUMENT);
    assert(!memcmp(&b,&unchanged,sizeof(b)));
    assert(!jc_connection_state(&b,build,state,sizeof(state),1000,mac,session,&received));
    assert(!memcmp(&source,&received,sizeof(source)));
    assert(jc_connection_state(&b,build,state,sizeof(state),1001,mac,session,&received)==JC_STREAM_REPLAY);
    assert(jc_actor_packet_encode(build,3,100,2,&source,mac,session,state));
    assert(jc_connection_state(&b,build,state,sizeof(state),1001,mac,session,&received)==JC_STREAM_IDENTITY);
    assert(b.sequence==1 && b.last_seen==1000);
    assert(jc_actor_packet_encode(build,1,100,2,&source,mac,root,state));
    assert(jc_connection_state(&b,build,state,sizeof(state),1001,mac,session,&received)==JC_STREAM_IDENTITY);
    assert(b.sequence==1);
    assert(jc_connection_hello(&a,build,mac,root,hello));
    struct jc_connection restarted={.local=1,.peer=2,.epoch=101,.nonce={7,8,9}};
    unsigned char fresh_key[32];pair(&restarted,&b,root,fresh_key);
    assert(memcmp(fresh_key,session,32));assert(!b.sequence && !b.last_seen);
    assert(jc_connection_receive(&b,build,hello,sizeof(hello),mac,root)==JC_STREAM_REPLAY);
    assert(jc_actor_packet_encode(build,1,100,3,&source,mac,session,state));
    assert(jc_connection_state(&b,build,state,sizeof(state),1002,mac,fresh_key,&received)==JC_STREAM_IDENTITY);
    assert(jc_actor_packet_encode(build,1,101,1,&source,mac,fresh_key,state));
    assert(!jc_connection_state(&b,build,state,sizeof(state),1002,mac,fresh_key,&received));
    struct jc_connection third_a={.local=1,.peer=3,.epoch=101,.nonce={10}};
    struct jc_connection third_b={.local=3,.peer=1,.epoch=300,.nonce={11}};
    unsigned char third_key[32];pair(&third_a,&third_b,wrong,third_key);
    assert(jc_connection_state(&third_b,build,state,sizeof(state),1003,mac,third_key,&received)==JC_STREAM_IDENTITY);
    assert(b.sequence==1 && b.remote_epoch==101 && b.last_seen==1002);
    puts("Connection: HMAC corruption/truncation, challenge proof, pinned identities, derived keys, state fidelity, replay, restart and independent third peer passed.");
    return 0;
}
