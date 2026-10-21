#include <stddef.h>
#define main probe_fixture_main
#include "test_jnet_steam_probe.c"
#undef main
typedef unsigned SRWLOCK;
#define SRWLOCK_INIT 0
static SRWLOCK jnet_lobby_lock;
#define BCRYPT_USE_SYSTEM_PREFERRED_RNG 2
static void AcquireSRWLockExclusive(SRWLOCK *lock){(void)lock;}
static void ReleaseSRWLockExclusive(SRWLOCK *lock){(void)lock;}
static int MH_EnableHook(void *target){assert(target);return MH_OK;}
static int MH_DisableHook(void *target){assert(target);return MH_OK;}
static int MH_RemoveHook(void *target){assert(target);return MH_OK;}
static int BCryptGenRandom(void *algorithm,unsigned char *out,unsigned bytes,unsigned flags){static unsigned seed;assert(!algorithm && flags==2);for(unsigned i=0;i<bytes;++i)out[i]=(unsigned char)(++seed*37);return 0;}
#define JC_BUILD_ID_HEX "fixture-build"
#include "jnet_steam_lobby.h"
#include "jnet_steam_transport.h"
static struct {uint64_t id,host,people[3];unsigned count,limit,joinable,type,local_member;char keys[16][32],values[16][80];unsigned fields;} rooms[8];
static unsigned maintenance_getters;
static unsigned room_count,leave_count,search_count,create_count,join_count,filters,worldwide,closed_count;
static uint64_t selected[50];static unsigned selected_count;
static struct {uint64_t handle;unsigned kind,done,failed;union {struct jnet_steam_created created;struct jnet_steam_entered entered;uint32_t count;} result;} pending;
static uint64_t next_call=100;
static unsigned room_index(uint64_t id){for(unsigned i=0;i<room_count;++i)if(rooms[i].id==id)return i;assert(!"unknown room");return 0;}
static const char *data(void *mm,uint64_t id,const char *key){++maintenance_getters;assert(mm);unsigned r=room_index(id);for(unsigned i=0;i<rooms[r].fields;++i)if(!strcmp(rooms[r].keys[i],key))return rooms[r].values[i];return "";}
static unsigned char set_data(void *mm,uint64_t id,const char *key,const char *value){assert(mm);unsigned r=room_index(id),i;for(i=0;i<rooms[r].fields;++i)if(!strcmp(rooms[r].keys[i],key))break;if(i==rooms[r].fields)++rooms[r].fields;assert(i<16);snprintf(rooms[r].keys[i],32,"%s",key);snprintf(rooms[r].values[i],80,"%s",value);return 1;}
static uint64_t owner(void *mm,uint64_t id){++maintenance_getters;assert(mm);unsigned r=room_index(id);return rooms[r].local_member?rooms[r].host:0;}
static int members(void *mm,uint64_t id){++maintenance_getters;assert(mm);return (int)rooms[room_index(id)].count;}
static int limit(void *mm,uint64_t id){++maintenance_getters;assert(mm);return (int)rooms[room_index(id)].limit;}
static uint64_t member(void *mm,uint64_t id,int i){++maintenance_getters;assert(mm);assert(i>=0 && i<3);return rooms[room_index(id)].people[i];}
static unsigned char joinable(void *mm,uint64_t id,unsigned char value){assert(mm);rooms[room_index(id)].joinable=value;return 1;}
static unsigned char set_type(void *mm,uint64_t id,int type){assert(mm && (type==0 || type==3));unsigned r=room_index(id);if(type==3)assert(!rooms[r].joinable && !strcmp(data(mm,id,"level"),JNET_NATIVE_LEVEL) && *data(mm,id,"journet.nonce"));rooms[r].type=(unsigned)type;return 1;}
static void leave(void *mm,uint64_t id){assert(mm && id);rooms[room_index(id)].local_member=0;++leave_count;}
static const char *name(void *friends,uint64_t id){assert(friends && id);return "Steam fixture player";}
static uint64_t by_index(void *mm,int i){assert(mm && i>=0 && (unsigned)i<selected_count);return selected[i];}
static void string_filter(void *mm,const char *key,const char *value,int compare){assert(mm && key && value && compare==0 && jnet_sl_owned);++filters;}
static void distance_filter(void *mm,int value){assert(mm && value==3 && jnet_sl_owned);++worldwide;}
static void count_filter(void *mm,int value){assert(mm && value==50 && jnet_sl_owned);}
static uint64_t start(unsigned kind){assert(!jnet_sl.call);memset(&pending,0,sizeof(pending));pending.handle=++next_call;pending.kind=kind;return pending.handle;}
static uint64_t search(void *mm){assert(mm && jnet_sl_owned);++search_count;return start(510);}
static uint64_t create(void *mm,int type,int capacity){assert(mm && type==0 && capacity==3 && jnet_sl_owned);++create_count;return start(513);}
static uint64_t join(void *mm,uint64_t room){assert(mm && room && jnet_sl_owned);++join_count;return start(504);}
static unsigned char completed(void *utils,uint64_t call,unsigned char *failed){assert(utils && call==pending.handle);*failed=(unsigned char)pending.failed;return (unsigned char)pending.done;}
static unsigned char result(void *utils,uint64_t call,void *out,int size,int callback,unsigned char *failed){assert(utils && call==pending.handle && callback==(int)pending.kind);assert(size==(callback==513?16:callback==504?24:4));memcpy(out,&pending.result,(size_t)size);*failed=0;return 1;}
static void finish_search(unsigned count){pending.result.count=count;pending.done=1;jnet_sl_poll_result();}
static uint64_t add_room(uint64_t host) {
    unsigned r=room_count++;assert(r<8);rooms[r].id=1000+r;rooms[r].host=host;rooms[r].count=1;rooms[r].people[0]=host;rooms[r].limit=3;rooms[r].joinable=1;
    return rooms[r].id;
}
static void finish_create(uint64_t room){rooms[room_index(room)].local_member=1;pending.result.created=(struct jnet_steam_created){1,0,room};pending.done=1;jnet_sl_poll_result();}
static void finish_join(uint64_t room,unsigned response){if(response==1)rooms[room_index(room)].local_member=1;pending.result.entered=(struct jnet_steam_entered){.room=room,.result=response};pending.done=1;jnet_sl_poll_result();}
static void setup(void) {
    jnet_sl_api.mm=(void *)1;jnet_sl_api.utils=(void *)2;jnet_sl_api.friends=(void *)3;
    jnet_sl_api.search=search;jnet_sl_api.create=create;jnet_sl_api.join=join;jnet_sl_api.leave=leave;
    jnet_sl_api.string_filter=string_filter;jnet_sl_api.distance_filter=distance_filter;jnet_sl_api.count_filter=count_filter;
    jnet_sl_api.data=data;jnet_sl_api.set_data=set_data;jnet_sl_api.owner=owner;jnet_sl_api.members=members;jnet_sl_api.member=member;
    jnet_sl_api.set_type=set_type;jnet_sl_api.legacy=(void *)5;jnet_sl_api.limit=limit;jnet_sl_api.joinable=joinable;jnet_sl_api.name=name;jnet_sl_api.by_index=by_index;
    jnet_sl_api.completed=completed;jnet_sl_api.result=result;
    jnet_sl.bound=1;jnet_sl.identity=42;jnet_sl.generation=1;jnet_sl.next_search=UINT64_MAX;
    jnet_steam_service.identity=42;JNETSteam[0]=5;JNETSteam[13]=1;JNETSteamLobby[9]=1;JNETSteamLobby[10]=0;
}
static void action(unsigned kind,unsigned access,const char *code,uint64_t room,uint64_t host) {
    struct jnet_lobby_action a={.kind=kind,.access=access,.room=room,.host=host};if(code)snprintf(a.password,sizeof(a.password),"%s",code);
    assert(jnet_lobby_submit(&a));jnet_sl_tick();
}
static unsigned char close_connection(void *self,uint32_t connection,int reason,const char *text,unsigned char linger){assert(self && connection && reason && text && !linger);++closed_count;return 1;}
static unsigned char set_userdata(void *self,uint32_t connection,int64_t generation){assert(self && connection && generation);return 1;}
static int accept_connection(void *self,uint32_t connection){assert(self && connection);return 1;}
static unsigned char connection_info(void *self,uint32_t connection,struct jnet_sn_info *info){assert(self && connection && info);return 0;}
static unsigned char close_listener(void *self,uint32_t listener){assert(self && listener);return 1;}
static unsigned rocket_sends;static unsigned char rocket_sent[128];
static unsigned chat_sends;
static unsigned char chat_sent[184];
static unsigned char travel_sent[104];
static unsigned char sent[72],game_sent[JNET_GAME_BYTES];static unsigned sent_type,game_sent_flags;
static int send_message(void *self,uint32_t connection,const void *data,uint32_t bytes,int flags,int64_t *number){assert(self && connection && !number);if(bytes==128){assert(flags==9);memcpy(rocket_sent,data,128);++rocket_sends;return 1;}if(bytes==184){assert(flags==9);memcpy(chat_sent,data,bytes);++chat_sends;return 1;}if(bytes==104){assert(!memcmp(data,"JNT3",4) && flags==9);memcpy(travel_sent,data,104);return 1;}if(bytes==JNET_GAME_BYTES){assert(!memcmp(data,"JNG1",4) && (flags==1 || flags==9));memcpy(game_sent,data,bytes);game_sent_flags=(unsigned)flags;return 1;}assert(bytes==72);memcpy(sent,data,72);sent_type=sent[4];assert(flags==(sent_type<=2?9:1));return 1;}
static unsigned char incoming[72];static unsigned incoming_count,releases;
static struct jnet_sn_message incoming_message;
static void release_message(struct jnet_sn_message *m){assert(m==&incoming_message);++releases;}
static int receive_messages(void *self,uint32_t connection,struct jnet_sn_message **out,int count){assert(self && connection && count==16);if(!incoming_count)return 0;incoming_count=0;*out=&incoming_message;return 1;}
static void input(unsigned slot,unsigned type,uint64_t sequence) {
    memset(incoming,0,72);memcpy(incoming,"JNS1",4);incoming[4]=(unsigned char)type;
    memcpy(incoming+8,&jnet_sl.room,8);memcpy(incoming+16,&jnet_sn.peers[slot].identity,8);memcpy(incoming+24,&jnet_sl.identity,8);memcpy(incoming+32,jnet_sl.nonce,32);memcpy(incoming+64,&sequence,8);
    incoming_message=(struct jnet_sn_message){.data=incoming,.bytes=72,.connection=jnet_sn.peers[slot].handle,.peer={.type=16,.size=8},.release=release_message};
    memcpy(incoming_message.peer.value,&jnet_sn.peers[slot].identity,8);incoming_count=1;
}
static struct jnet_sn_callback_base *registered_callback,*registered_members,*registered_data;
static unsigned refuse_members,refuse_data;
static unsigned registrations,unregistrations;
static void register_callback(void *object,int id) {
    struct jnet_sn_callback_base *cb=object;assert(cb->id==id);
    int (*size)(void *)=(void *)cb->table[2];
    if(id==506){assert(!registered_members && size(cb)==32);if(refuse_members)return;registered_members=cb;}
    else if(id==505){assert(!registered_data && size(cb)==24);if(refuse_data)return;registered_data=cb;}
    else {assert(id==1221 && !registered_callback && size(cb)==712);registered_callback=cb;++registrations;}
    cb->flags|=1;
}
static void unregister_callback(void *object) {
    struct jnet_sn_callback_base *cb=object;cb->flags&=~1;
    if(object==registered_members)registered_members=NULL;
    else if(object==registered_data)registered_data=NULL;
    else {assert(object==registered_callback);registered_callback=NULL;++unregistrations;}
}
static void *registration_export(const char *name) {
    if(!strcmp(name,"SteamAPI_RegisterCallback"))return (void *)register_callback;
    if(!strcmp(name,"SteamAPI_UnregisterCallback"))return (void *)unregister_callback;
    return NULL;
}
static void dispatch_registered(struct jnet_sn_event *event) {
    assert(registered_callback);void (*run)(void *,void *)=(void *)registered_callback->table[1];run(registered_callback,event);
}
static void membership_changed(uint64_t room,uint64_t id,unsigned flags) {
    assert(registered_members);struct jnet_sn_lobby_event event={.room=room,.changed=id,.flags=flags};
    void (*run)(void *,void *)=(void *)registered_members->table[1];run(registered_members,&event);
}
static void transport_test(uint64_t room) {
    jnet_sl.room=room;jnet_sl.host=jnet_sl.identity;jnet_sl.generation=70;snprintf(jnet_sl.nonce,33,"0123456789abcdef0123456789abcdef");
    jnet_sn_api.self=(void *)4;jnet_sn_api.close=close_connection;jnet_sn_api.close_listener=close_listener;jnet_sn_api.userdata=set_userdata;
    jnet_sn_api.info=connection_info;jnet_sn_api.accept=accept_connection;jnet_sn_api.send=send_message;jnet_sn_api.receive=receive_messages;
    jnet_sn.room=room;jnet_sn.generation=70;jnet_sn.listener=77;
    extra_exports=registration_export;assert(jnet_sn_register() && jnet_sn_register() && registrations==1 && JNETSteamP2P[8]==1);
    struct jnet_sn_event unrelated={.connection=999,.info={.listener=999,.state=1}};dispatch_registered(&unrelated);assert(!jnet_sn_queue.count && JNETSteamP2P[9]==1);
    struct jnet_sn_event event={.connection=101,.info={.remote={.type=16,.size=8},.listener=77,.state=1,.flags=3}};uint64_t guest=43;memcpy(event.info.remote.value,&guest,8);
    dispatch_registered(&event);jnet_sn_tick();assert(jnet_sn.peers[0].handle==101 && !jnet_sn.peers[0].admitted);
    assert(!jnet_sn.peers[0].connected && JNETSteamP2P[2]==1 && !JNETSteamLobby[14]);
    event.info.flags=0;event.info.state=3;event.info.userdata=70;jnet_sn_event(&event);assert(jnet_sn.peers[0].connected);
    input(0,1,0);jnet_sn_receive(0);assert(jnet_sn.peers[0].hello && !jnet_sn.peers[0].admitted && releases==1);
    unsigned r=room_index(room);rooms[r].people[1]=43;rooms[r].count=2;membership_changed(room,43,1);jnet_sn_tick();assert(!jnet_sn.peers[0].admitted);
    set_data((void *)1,room,"journet.nonce",jnet_sl.nonce);jnet_sn_tick();assert(jnet_sn.peers[0].admitted && JNETSteamLobby[12]==1);
    input(0,3,1);jnet_sn_receive(0);assert(sent_type==4 && releases==2);
    input(0,4,1);jnet_sn_receive(0);assert(jnet_sn.peers[0].last_received[0]==1 && jnet_sn.peers[0].last_received[1]==1);
    LONG received=JNETSteamLobby[14];input(0,3,1);jnet_sn_receive(0);assert(JNETSteamLobby[14]==received);
    input(0,3,2);incoming[32]^=1;jnet_sn_receive(0);assert(JNETSteamLobby[14]==received && JNETSteamLobby[15]==-103);
    event.connection=102;event.info.state=1;jnet_sn_event(&event);assert(closed_count==1 && strstr(JNETSteamP2PText,"duplicate"));
    guest=44;memcpy(event.info.remote.value,&guest,8);jnet_sn_event(&event);assert(jnet_sn.peers[1].handle==102);
    event.connection=103;guest=45;memcpy(event.info.remote.value,&guest,8);jnet_sn_event(&event);assert(closed_count==2 && strstr(JNETSteamP2PText,"capacity"));
    rooms[r].count=1;membership_changed(room,43,2);jnet_sn_tick();assert(!jnet_sn.peers[0].handle);
    clock_ms+=30001;jnet_sn_tick();assert(!jnet_sn.peers[1].handle);
    jnet_sn_reset();assert(!jnet_sn.listener && !registered_callback && !registered_members && unregistrations==1 && !JNETSteamP2P[8]);
    assert(jnet_sn_register() && registrations==2);
    assert(JNETSteamP2P[1]==4 && JNETSteamP2P[2]==2 && JNETSteamP2P[3]==2);
    jnet_sn.listener=77;
    for(unsigned flags=1;flags<=3;++flags) {
        event.connection=300+flags;event.info.state=1;event.info.flags=3;
        guest=44;memcpy(event.info.remote.value,&guest,8);jnet_sn_event(&event);
        assert(jnet_sn.peers[0].handle==event.connection && !jnet_sn.peers[0].connected);
        LONG before=JNETSteamLobby[14];event.info.state=3;event.info.flags=flags;jnet_sn_event(&event);
        assert(!jnet_sn.peers[0].handle && JNETSteamLobby[15]==-100 && JNETSteamLobby[14]==before);
    }
    event.info.flags=0;
    jnet_sl.host=43;jnet_sn.room=room;jnet_sn.generation=jnet_sl.generation;
    jnet_sl_transport_reset=jnet_sn_reset;
    jnet_sn.peers[0].handle=201;jnet_sn.peers[0].identity=43;jnet_sn.peers[0].generation=jnet_sl.generation;
    event.connection=201;event.info.listener=0;event.info.state=5;event.info.reason=5003;guest=43;memcpy(event.info.remote.value,&guest,8);
    snprintf(event.info.debug,sizeof(event.info.debug),"Timed out waiting for remote acceptance");
    dispatch_registered(&event);jnet_sn_tick();
    assert(unregistrations==2 && !registered_callback && !registered_members);
    assert(!jnet_sl.room && !jnet_sn.peers[0].handle && JNETSteamP2P[6]==5003);
    assert(strstr(jnet_sl.status,"Host connection ended") && strstr(JNETSteamP2PText,"remote acceptance"));
    jnet_sl_transport_reset=NULL;
}
static struct jnet_game_record game_record(uint64_t id,uint64_t sequence) {
    struct jnet_game_record r={.id=id,.lease=99,.sequence=sequence,.seen=clock_ms};
    float position[4]={318,78,102,1},shape[2]={0,0.1f};uint32_t appearance[4]={1,9,2,5};
    assert(jc_actor_sender_spawn(position,2,1,shape,appearance,&r.sample));return r;
}
static void game_packet(unsigned char *p,unsigned type,uint64_t sequence,const struct jnet_game_record *r) {
    memset(p,0,JNET_GAME_BYTES);memcpy(p,"JNG1",4);jc_packet_put(p+4,type,4);jc_packet_put(p+8,jnet_sl.room,8);
    memcpy(p+16,jnet_sl.nonce,32);jc_packet_put(p+48,sequence,8);jnet_sg_encode_record(p+64,r,clock_ms);
}
static int64_t arrival_clock;
static int64_t fake_timestamp(void *self){(void)self;return arrival_clock;}
static void arrival_test(void) {
    jnet_sn_timestamp=fake_timestamp;
    struct jnet_sn_message m={.connection=900,.time=1000000};arrival_clock=1020000;jnet_sn_arrival(0,&m);
    assert(JNETSteamP2P[113]==20 && JNETSteamP2P[115]==1);
    clock_ms+=100;m.time+=30000;arrival_clock+=100000;jnet_sn_arrival(0,&m);
    assert(JNETSteamP2P[109]==30 && JNETSteamP2P[111]==100 && JNETSteamP2P[113]==90);
    m.connection=901;jnet_sn_arrival(0,&m);assert(JNETSteamP2P[115]==1 && !JNETSteamP2P[110]);
    jnet_sn_timestamp=NULL;jnet_sn_arrival(0,&m);assert(JNETSteamP2P[113]==-1);
    puts("Delivery timing: Steam receipt vs processing gap, receive queue age, connection reset and unavailable clock passed.");
}
static unsigned metric_calls;static int metric_result=1;
static int fake_realtime(void *self,uint32_t handle,struct jnet_sn_realtime *status,int lanes,void *lane_data) {
    (void)self;assert(handle && !lanes && !lane_data);++metric_calls;clock_ms+=3;
    *status=(struct jnet_sn_realtime){.state=3,.ping=47,.queue_us=1800,.pending_unreliable=832,.pending_reliable=72,.unacked=144};return metric_result;
}
static void metrics_test(void) {
    memset(jnet_sn.peers,0,sizeof(jnet_sn.peers));jnet_sn_next_metrics=0;jnet_sn_api.realtime=fake_realtime;
    jnet_sn.peers[0].handle=999;jnet_sn.peers[0].identity=43;jnet_sn.peers[0].connected=1;
    jnet_sn_metrics();assert(metric_calls==1 && JNETSteamP2P[29]==47 && JNETSteamP2P[31]==1800 && JNETSteamP2P[33]==832 && JNETSteamP2P[35]==144 && JNETSteamP2P[23]==3);
    jnet_sn_metrics();assert(metric_calls==1);
    metric_result=8;clock_ms+=1000;jnet_sn_metrics();assert(JNETSteamP2P[28]==8 && JNETSteamP2P[29]==-1 && JNETSteamP2P[31]==-1);
    jnet_sn.peers[0].handle=1000;jnet_sn_metrics();assert(JNETSteamP2P[28]==0 && !JNETSteamP2P[36] && !JNETSteamP2P[30]);
    JNETSteamP2P[115]=17;JNETSteamP2P[114]=91;
    jnet_sn_drop(0);jnet_sn_metrics();assert(!JNETSteamP2P[27]);
    assert(JNETSteamP2P[167]==1000 && JNETSteamP2P[183]==17 && JNETSteamP2P[182]==91);
    jnet_sn_reset();assert(JNETSteamP2P[167]==1000 && JNETSteamP2P[183]==17);
    jnet_sn.peers[0].handle=1001;jnet_sn.peers[0].identity=44;jnet_sn.peers[0].connected=1;
    metric_result=1;jnet_sn_metrics();jnet_sn_drop(0);
    assert(JNETSteamP2P[165]==44 && JNETSteamP2P[167]==1001);
    for(unsigned i=29;i<50;++i)JNETSteamLobby[i]=0;
    jnet_sl_record_poll(0,49);jnet_sl_record_poll(0,50);jnet_sl_record_poll(0,100);jnet_sl_record_poll(0,250);
    jnet_sl_record_poll(1,900);jnet_sl_record_poll(2,800);
    assert(JNETSteamLobby[30]==250 && JNETSteamLobby[31]==4);
    assert(JNETSteamLobby[32]==1 && JNETSteamLobby[33]==1 && JNETSteamLobby[34]==1 && JNETSteamLobby[35]==1);
    assert(JNETSteamLobby[37]==900 && JNETSteamLobby[44]==800);
    jnet_sn_api.realtime=NULL;memset(jnet_sn.peers,0,sizeof(jnet_sn.peers));
    puts("Steam metrics: field mapping, query cost, bounded sampling, failure invalidation, connection replacement and disconnect passed.");
}
#define JNET_MAINTENANCE_FIXTURE
static unsigned maintenance_available=1,maintenance_wakes;
static int jnet_maintenance_start(void){return maintenance_available;}
static void jnet_maintenance_wake(void){++maintenance_wakes;}
#include "jnet_steam_maintenance.h"
static void maintenance_complete(void) {
    static struct jnet_maintenance_snapshot metadata;
    struct jnet_maintenance_snapshot job=jnet_maintenance.request;
    jnet_maintenance_read(&job,&metadata);jnet_maintenance.result=job;
    jnet_maintenance.pending=0;jnet_maintenance.ready=1;
}
static void maintenance_test(uint64_t room) {
    unsigned ri=room_index(room),population=0;
    jnet_sl.room=room;jnet_sl.host=jnet_sl.identity=42;++jnet_sl.generation;
    rooms[ri].local_member=1;rooms[ri].host=42;rooms[ri].count=3;
    rooms[ri].people[0]=42;rooms[ri].people[1]=43;rooms[ri].people[2]=44;
    snprintf(jnet_sl.nonce,33,"0123456789abcdef0123456789abcdef");
    set_data((void *)1,room,"journet.nonce",jnet_sl.nonce);
    jnet_sn_members_dirty=jnet_sl_metadata_dirty=0;
    assert(!jnet_maintenance_poll(&population) && maintenance_wakes==1);
    maintenance_complete();assert(jnet_maintenance_poll(&population)==1 && population==3);
    assert(jnet_sn_members.loaded && jnet_sn_members.ids[2]==44);
    jnet_sl_background=jnet_maintenance_poll;jnet_sl.next_members=0;
    LONG foreground=JNETSteamLobby[26];unsigned reads=maintenance_getters;jnet_sl_tick();
    assert(jnet_sl.room==room && JNETSteamLobby[6]==3 && JNETSteamLobby[26]==foreground && maintenance_getters==reads);
    jnet_sl_background=NULL;
    clock_ms+=250;assert(jnet_maintenance_poll(&population)==1);maintenance_complete();
    ++jnet_sl.generation;assert(!jnet_maintenance_poll(&population) && JNETSteamLobby[55]);
    maintenance_complete();assert(jnet_maintenance_poll(&population)==1);
    ++JNETSteamP2P[16];jnet_sn_members_dirty=1;
    assert(!jnet_maintenance_poll(&population));maintenance_complete();
    assert(!jnet_maintenance_poll(&population));jnet_sn_members_dirty=0;
    assert(jnet_maintenance_poll(&population)==1);
    clock_ms+=501;assert(!jnet_maintenance_poll(&population));
    rooms[ri].host=77;maintenance_complete();assert(jnet_maintenance_poll(&population)==-1);
    rooms[ri].host=42;++JNETSteamLobby[28];set_data((void *)1,room,"journet.nonce","bad");
    assert(!jnet_maintenance_poll(&population));maintenance_complete();assert(jnet_maintenance_poll(&population)==-1);
    maintenance_available=0;assert(!jnet_maintenance_poll(&population));maintenance_available=1;
    set_data((void *)1,room,"journet.nonce",jnet_sl.nonce);
    jnet_sl.identity=43;++jnet_sl.generation;++JNETSteamLobby[28];
    set_data((void *)1,room,"journet.lighting","");
    set_data((void *)1,room,"journet.daynight","1");jnet_daynight_mode=0;
    assert(!jnet_maintenance_poll(&population));maintenance_complete();
    assert(jnet_maintenance_poll(&population)==1 && jnet_daynight_host && !jnet_daynight_mode);
    jnet_daynight_enter(8);assert(!jnet_daynight_mode);
    jnet_daynight_enter(3);assert(jnet_daynight_mode==2);
    set_data((void *)1,room,"journet.lighting","1");++JNETSteamLobby[28];
    assert(!jnet_maintenance_poll(&population));maintenance_complete();
    assert(jnet_maintenance_poll(&population)==1 && jnet_daynight_host==1);
    assert(jnet_daynight_mode==2);jnet_daynight_enter(2);assert(jnet_daynight_mode==1);
    set_data((void *)1,room,"journet.lighting","0");
    jnet_daynight_mode=2;++JNETSteamLobby[28];set_data((void *)1,room,"journet.daynight","0");
    assert(!jnet_maintenance_poll(&population));maintenance_complete();
    assert(jnet_maintenance_poll(&population)==1 && !jnet_daynight_host);
    jnet_daynight_enter(0);assert(!jnet_daynight_mode);
    jnet_daynight_host=~0u;jnet_daynight_mode=2;jnet_daynight_enter(0);assert(jnet_daynight_mode==2);
    jnet_daynight_mode=0;
    jnet_sl.identity=42;jnet_sl.room=0;
    puts("Background maintenance: immutable jobs, cached roster, generation/event invalidation, expiration, owner/nonce rejection and worker failure fallback passed. Fixture runs worker jobs deterministically.");
}
static void membership_poll_test(uint64_t room) {
    jnet_sl.room=room;unsigned ri=room_index(room);rooms[ri].count=3;
    rooms[ri].people[0]=42;rooms[ri].people[1]=43;rooms[ri].people[2]=44;
    jnet_sn_members.active=1;jnet_sn_members.loaded=0;LONG calls=JNETSteamP2P[14];
    for(unsigned i=0;i<32;++i){assert(jnet_sn_member(43));assert(jnet_sn_member(44));assert(!jnet_sn_member(45));}
    assert(JNETSteamP2P[14]==calls+4);
    jnet_sn_members.active=0;assert(jnet_sn_register());
    unsigned population=0;
    assert(jnet_sn_roster(42,&population) && population==3 && JNETSteamP2P[14]==calls+4);
    assert(!jnet_sn_members.active);
    jnet_sn_members.active=0;jnet_sn_members.active=1;clock_ms+=100;
    assert(jnet_sn_member(43) && JNETSteamP2P[14]==calls+4);
    struct jnet_sn_lobby_event change={.room=room+1,.changed=44,.flags=2};
    jnet_sn_members_dispatch(NULL,&change);assert(jnet_sn_member(43) && JNETSteamP2P[14]==calls+4);
    rooms[ri].count=2;change.room=room;jnet_sn_members_dispatch(NULL,&change);
    assert(!jnet_sn_member(44) && jnet_sn_member(43));assert(JNETSteamP2P[14]==calls+7);
    rooms[ri].people[1]=45;++jnet_sl.generation;
    assert(!jnet_sn_member(43) && jnet_sn_member(45));
    rooms[ri].count=4;jnet_sn_members.loaded=0;assert(!jnet_sn_member(42));
    rooms[ri].count=3;rooms[ri].people[1]=43;jnet_sn_members.active=0;
    assert(jnet_sn_member(44));rooms[ri].count=2;assert(!jnet_sn_member(44));
    rooms[ri].count=3;jnet_sn_members.loaded=0;jnet_sn_members.active=1;assert(jnet_sn_member(44));
    rooms[ri].count=2;clock_ms+=500;assert(!jnet_sn_member(44));
    rooms[ri].count=3;jnet_sn_members.active=0;jnet_sn_members.loaded=0;
    jnet_sn_unregister_fn(&jnet_sn_registration);jnet_sn_unregister_fn(&jnet_sn_members_registration);jnet_sn_unregister_fn(&jnet_sn_data_registration);
    refuse_members=1;assert(!jnet_sn_register() && !registered_callback && !registered_members && !registered_data && JNETSteamP2P[8]==-2);refuse_members=0;
    refuse_data=1;assert(!jnet_sn_register() && !registered_callback && !registered_members && !registered_data && !jnet_sl_metadata_events);refuse_data=0;
    puts("Steam roster: cached polls, membership notifications, unrelated-event isolation, 500 ms fallback, generation change, invalid capacity, uncached calls and partial registration cleanup verified.");
}
static void character_switch_matrix(uint64_t room) {
    unsigned ri=room_index(room),cases=0;
    rooms[ri].count=3;for(unsigned i=0;i<3;++i)rooms[ri].people[i]=42+i;
    jnet_sn_members.loaded=0;
    for(unsigned level=0;level<12;++level)for(unsigned player=0;player<3;++player)
    for(unsigned from=0;from<JNET_CHARACTER_COUNT;++from)for(unsigned to=0;to<JNET_CHARACTER_COUNT;++to){
        unsigned char snapshots[2][JNET_GAME_BYTES],packet[JNET_GAME_BYTES];
        jnet_sg_reset();jnet_sl.room=room;jnet_sl.identity=jnet_sl.host=42;++jnet_sl.generation;
        memset(jnet_sn.peers,0,sizeof(jnet_sn.peers));
        for(unsigned i=0;i<2;++i){jnet_sn.peers[i].handle=500+i;jnet_sn.peers[i].identity=43+i;jnet_sn.peers[i].connected=jnet_sn.peers[i].admitted=1;}
        jnet_sg_tick();
        LONG rejected=JNETSteamGame[3];
        for(unsigned step=0;step<2;++step){
            clock_ms+=34;
            for(unsigned i=0;i<3;++i){
                struct jnet_game_record r=game_record(42+i,step+1);
                r.sample.chapter=level;r.character_known=1;r.character=i==player?(step?to:from):(i+1)%JNET_CHARACTER_COUNT;
                r.rotation_known=jnet_character_is_creature(r.character);r.rotation[3]=1;
                if(!i)jnet_sg_mail.outgoing=r;
                else {game_packet(packet,1,step+1,&r);jc_packet_put(packet+56,jnet_travel_epoch,8);assert(jnet_sg_receive(i-1,packet,sizeof(packet)));}
            }
            jnet_sg_tick();assert(JNETSteamGame[3]==rejected);
            assert(jnet_sg_mail.records[player].character==(step?to:from));
            memcpy(snapshots[step],game_sent,sizeof(game_sent));
        }
        for(unsigned guest=43;guest<=44;++guest){
            jnet_sg_reset();jnet_sl.identity=guest;++jnet_sl.generation;
            memset(jnet_sn.peers,0,sizeof(jnet_sn.peers));
            jnet_sn.peers[0].handle=700;jnet_sn.peers[0].identity=42;jnet_sn.peers[0].admitted=jnet_sn.peers[0].connected=1;
            jnet_sg_tick();
            for(unsigned step=0;step<2;++step){
                rejected=JNETSteamGame[3];assert(jnet_sg_receive(0,snapshots[step],sizeof(snapshots[step])));
                assert(JNETSteamGame[3]==rejected);
                for(unsigned i=0;i<3;++i){
                    const struct jnet_game_record *r=&jnet_sg_mail.records[i];
                    unsigned kind=i==player?(step?to:from):(i+1)%JNET_CHARACTER_COUNT;
                    assert(r->id==42+i && r->sample.chapter==level && r->character_known && r->character==kind);
                    assert(r->rotation_known==(unsigned)jnet_character_is_creature(kind));
                }
            }
        }
        ++cases;
    }
    assert(cases==12*3*JNET_CHARACTER_COUNT*JNET_CHARACTER_COUNT);
    jnet_sg_reset();memset(jnet_sn.peers,0,sizeof(jnet_sn.peers));jnet_sl.identity=42;
    puts("Character transport: all 1764 level/player/from/to combinations passed host receive, fanout and both guest receivers with mocked Steam. Native rendering and Internet play not exercised.");
}
static void gameplay_test(uint64_t room) {
    jnet_sg_reset();jnet_sl.room=room;jnet_sl.identity=jnet_sl.host=42;jnet_sl.generation=90;
    unsigned ri=room_index(room);rooms[ri].count=3;rooms[ri].people[0]=42;rooms[ri].people[1]=43;rooms[ri].people[2]=44;
    for(unsigned i=0;i<2;++i){jnet_sn.peers[i].handle=500+i;jnet_sn.peers[i].identity=43+i;jnet_sn.peers[i].connected=jnet_sn.peers[i].admitted=1;}
    jnet_sg_tick();jnet_sg_mail.outgoing=game_record(42,1);
    jc_receiver_store(jnet_sg_mail.outgoing.sample.state.bytes,1000000.f);
    unsigned char packet[JNET_GAME_BYTES];struct jnet_game_record r=game_record(43,1);
    jc_receiver_store(r.sample.state.bytes,-1000000.f);
    game_packet(packet,1,1,&r);jnet_sn.peers[0].admitted=0;LONG rejected=JNETSteamGame[3];
    assert(jnet_sg_receive(0,packet,sizeof(packet)) && JNETSteamGame[3]==rejected+1 && !jnet_sg.peers[0].sequence);jnet_sn.peers[0].admitted=1;
    assert(jnet_sg_receive(0,packet,sizeof(packet)) && jnet_sg.peers[0].sequence==1 && jnet_sg.peers[0].lease!=99);
    rejected=JNETSteamGame[3];jnet_sg_receive(0,packet,sizeof(packet));assert(JNETSteamGame[3]==rejected+1 && JNETSteamGame[22]==1);
    assert(jc_receiver_float(jnet_sg.peers[0].sample.state.bytes)==-1000000.f);
    r=game_record(44,1);jc_receiver_store(r.sample.state.bytes+8,1000000.f);game_packet(packet,1,1,&r);jnet_sg_receive(1,packet,sizeof(packet));
    r=game_record(43,2);r.chirp=(struct jc_chirp_event){.participant=43,.epoch=99,.sequence=1,.sound=1,.radius=1,.duration=1};r.chirp_seen=clock_ms;
    game_packet(packet,3,2,&r);packet[16]^=1;rejected=JNETSteamGame[3];jnet_sg_receive(0,packet,sizeof(packet));assert(JNETSteamGame[3]==rejected+1);packet[16]^=1;
    jc_packet_put(packet+64,44,8);jnet_sg_receive(0,packet,sizeof(packet));assert(JNETSteamGame[3]==rejected+2);jc_packet_put(packet+64,43,8);
    jnet_sg_receive(0,packet,sizeof(packet));assert(game_sent_flags==9 && jnet_sg_mail.event_count==1);
    unsigned char chirp_packet[JNET_GAME_BYTES];memcpy(chirp_packet,game_sent,sizeof(chirp_packet));
    clock_ms+=34;jnet_sg_tick();assert(game_sent_flags==1);
    unsigned char snapshot[JNET_GAME_BYTES];memcpy(snapshot,game_sent,sizeof(snapshot));
    LONG host_sent=JNETSteamGame[1];clock_ms+=34;jnet_sg_tick();
    assert(JNETSteamGame[1]==host_sent && JNETSteamGame[61]);
    clock_ms+=100;jnet_sg_tick();assert(JNETSteamGame[1]==host_sent+2);
    uint64_t lease=jnet_sg.peers[0].lease;assert(jnet_sg_mail.records[1].id==43 && jnet_sg_mail.records[2].id==44);
    jnet_sn.peers[0].handle=600;jnet_sg_tick();assert(jnet_sg.peers[0].lease>lease && !jnet_sg.peers[0].sequence);
    jnet_sl.identity=43;jnet_sl.host=42;++jnet_sl.generation;jnet_sg_reset();memset(jnet_sn.peers,0,sizeof(jnet_sn.peers));
    jnet_sn.peers[0].handle=700;jnet_sn.peers[0].identity=42;jnet_sn.peers[0].admitted=jnet_sn.peers[0].connected=1;jnet_sg_tick();
    jnet_sg_receive(0,snapshot,sizeof(snapshot));assert(jnet_sg_mail.records[0].id==42 && jnet_sg_mail.records[2].id==44 && !jnet_sg_mail.records[1].chirp.sequence);
    rejected=JNETSteamGame[3];jnet_sg_receive(0,snapshot,sizeof(snapshot));assert(JNETSteamGame[3]==rejected+1);
    jnet_sg_receive(0,chirp_packet,sizeof(chirp_packet));assert(jnet_sg_mail.event_count==0);
    jc_packet_put(chirp_packet+64,44,8);jnet_sg_receive(0,chirp_packet,sizeof(chirp_packet));assert(jnet_sg_mail.event_count==1);
    jnet_sg_mail.outgoing=game_record(43,1);jnet_sg_tick();LONG sent=JNETSteamGame[1];
    uint64_t previous_send=jnet_sg.last_send;
    clock_ms+=34;jnet_sg_tick();assert(JNETSteamGame[1]==sent && JNETSteamGame[40] && jnet_sg.last_send==previous_send);
    jnet_sg_mail.outgoing=game_record(43,2);jnet_sg_tick();assert(JNETSteamGame[1]==sent+1);
    jnet_sn.peers[0].handle=701;clock_ms+=34;jnet_sg_tick();assert(JNETSteamGame[1]==sent+2);
    struct jnet_game_record event=game_record(43,2);event.chirp=(struct jc_chirp_event){.participant=43,.epoch=99,.sequence=2,.sound=1,.radius=1,.duration=1};event.chirp_seen=clock_ms;
    jnet_sg_event_send(&event,0);assert(JNETSteamGame[1]==sent+3 && game_sent_flags==9);
    LONG reasons=0;for(unsigned i=18;i<24;++i)reasons+=JNETSteamGame[i];assert(reasons==JNETSteamGame[3]);
    struct jnet_game_record decoded;unsigned char encoded[JNET_GAME_RECORD];r=game_record(42,1);jnet_sg_encode_record(encoded,&r,clock_ms);
    for(unsigned kind=0;kind<JNET_CHARACTER_COUNT;++kind){
        r.character=kind;r.character_known=1;jnet_sg_encode_record(encoded,&r,clock_ms);
        assert(jnet_sg_decode_record(&decoded,encoded,clock_ms));
        assert(decoded.character_known && decoded.character==kind);
        assert(jnet_sg_war_target(&decoded,99,decoded.sample.chapter,clock_ms,500)==(kind==JNET_CHARACTER_WAYFARER));
        r.rotation_known=1;r.rotation[0]=0;r.rotation[1]=0;r.rotation[2]=0;r.rotation[3]=1;
        jnet_sg_encode_record(encoded,&r,clock_ms);
        assert(jnet_sg_decode_record(&decoded,encoded,clock_ms)==jnet_character_is_creature(kind));
        if(jnet_character_is_creature(kind))assert(decoded.character==kind && decoded.rotation_known && decoded.rotation[3]==1);
        r.rotation_known=0;
    }
    r.character=JNET_CHARACTER_WAYFARER;r.character_known=1;
    jnet_sg_encode_record(encoded,&r,clock_ms);assert(jnet_sg_decode_record(&decoded,encoded,clock_ms));
    assert(!jnet_sg_war_target(&decoded,decoded.id,decoded.sample.chapter,clock_ms,500));
    assert(!jnet_sg_war_target(&decoded,99,(decoded.sample.chapter+1)%12,clock_ms,500));
    assert(!jnet_sg_war_target(&decoded,99,decoded.sample.chapter,clock_ms+500,500));
    assert(!jnet_sg_war_target(&decoded,99,decoded.sample.chapter,decoded.seen-1,500));
    decoded.lease=0;assert(!jnet_sg_war_target(&decoded,99,decoded.sample.chapter,clock_ms,500));
    jc_packet_put(encoded+252,0,4);assert(jnet_sg_decode_record(&decoded,encoded,clock_ms));
    assert(!decoded.character_known && !jnet_sg_war_target(&decoded,99,decoded.sample.chapter,clock_ms,500));
    jc_packet_put(encoded+252,JNET_CHARACTER_COUNT+1,4);assert(!jnet_sg_decode_record(&decoded,encoded,clock_ms));
    jc_packet_put(encoded+252,1,4);jc_packet_put(encoded+16,0,8);assert(!jnet_sg_decode_record(&decoded,encoded,clock_ms));
    jnet_sg_encode_record(encoded,&r,clock_ms);
    jc_packet_put(encoded+32,0x7fc00000,4);assert(!jnet_sg_decode_record(&decoded,encoded,clock_ms));
    jnet_sg_encode_record(encoded,&r,clock_ms+2000);assert(jnet_sg_decode_record(&decoded,encoded,clock_ms+2000) && !decoded.sequence);
    memset(jnet_sn.peers,0,sizeof(jnet_sn.peers));clock_ms+=34;
    jnet_sg.last_send=clock_ms-6000;jnet_sg_tick();assert(!jnet_sg.last_send);
    jnet_sg_reset();assert(!jnet_sg_mail.room && !jnet_sg_mail.records[0].id);
    puts("Steam gameplay: authenticated host fanout, three identities, reliable individual chirps, nonce/owner/replay rejection, renewed leases, invalid floats, expiry, duplicate-send suppression, reconnect resend and mailbox reset passed.");
}
#include "jnet_menu_model.h"
static struct jnet_appearance_edit jnet_menu_edit;
#include "companion_connection.h"
#include "companion_chirp_packet.h"
struct jc_network_peer {struct jc_connection connection;struct jc_actor_sender sample;struct jc_chirp_inbox wire_chirps;uint64_t consumed;};
static struct {uint64_t identity;unsigned chapter,paused;struct jc_network_peer peers[2];} traveler_network;
static unsigned traveler_network_external,traveler_network_active,traveler_stream_enabled,traveler_local_state,traveler_actor_trial_join_pending;
static void *traveler_actor;static int actor_idle=1;
static LONG JNETNetwork[24],JNETActorTrial[5],JNETFrames;
static struct {unsigned ready;} traveler_udp[2];
static int traveler_peer_vacant(void){return actor_idle;}
static void traveler_network_chirp_take(unsigned slot,uint64_t now){assert(slot<2 && now==clock_ms);}
static int fake_capture_result;
static int fake_capture(jc_layout_reader read,void *ctx,uint64_t image,uint64_t game,struct jc_actor_sender *out){(void)read;(void)ctx;(void)image;(void)game;*out=game_record(42,1).sample;return fake_capture_result;}
static unsigned fake_chirps;
static int fake_chirp(jc_layout_reader read,void *ctx,uint64_t game,uint64_t frame,uint64_t id,uint64_t epoch,struct jc_chirp_capture *capture,struct jc_chirp_batch *batch){(void)read;(void)ctx;(void)game;(void)frame;(void)id;(void)epoch;(void)capture;memset(batch,0,sizeof(*batch));batch->count=fake_chirps;for(unsigned i=0;i<fake_chirps;++i)batch->source[i].event=(struct jc_chirp_event){.participant=id,.epoch=epoch,.sequence=++capture->sequence,.sound=1,.radius=1,.duration=1};fake_chirps=0;return 0;}
#define jc_actor_capture_sender fake_capture
#define jc_chirp_capture_local fake_chirp
#define own_read NULL
static unsigned jnet_grab_down;
#include "jnet_steam_game_live.h"
#undef own_read
#undef jc_chirp_capture_local
#undef jc_actor_capture_sender
static void jnet_sg_game_tick(uintptr_t game) {jnet_sg_game_phase(game,1);jnet_sg_game_phase(game,0);}
static void game_bridge_test(void) {
    uint64_t original_epoch=jnet_travel_epoch;
    jnet_menu_edit.applied.character=JNET_CHARACTER_WAR_MACHINE;
    jnet_sg_mail.room=999;jnet_sg_mail.identity=43;jnet_sg_mail.generation=91;jnet_sg_mail.received=clock_ms;
    jnet_sg_mail.records[0]=game_record(42,1);jnet_sg_mail.records[1]=game_record(43,1);jnet_sg_mail.records[2]=game_record(44,1);
    JNETSteamLobby[9]=1;JNETSteamLobby[10]=0;
    LONG captures_before=JNETSteamGame[6];jnet_sg_game_phase(1,1);
    assert(JNETSteamGame[6]==captures_before && traveler_network.peers[1].connection.peer==42);
    uint64_t applied=traveler_network.peers[1].connection.sequence;
    jnet_sg_mail.records[0].sequence++;
    jnet_sg_game_phase(1,0);assert(traveler_network.peers[1].connection.sequence==applied);
    jnet_sg_mail.records[0].sequence=applied;
    assert(traveler_network_external && traveler_network_active && traveler_stream_enabled && !traveler_network.paused);
    assert(traveler_network.peers[1].connection.peer==42 && traveler_network.peers[0].connection.peer==44);
    assert(traveler_network.peers[1].sample.chapter==2 && jnet_sg_mail.outgoing.id==43 && JNETSteamGame[6]);
    assert(jnet_sg_mail.outgoing.character_known && jnet_sg_mail.outgoing.character==JNET_CHARACTER_WAR_MACHINE);
    assert((uint32_t)JNETSteamGame[24]==44 && (uint32_t)JNETSteamGame[32]==42 && JNETSteamGame[27]==1 && JNETSteamGame[35]==1);
    clock_ms+=45;jnet_sg_mail.records[2]=game_record(44,2);jnet_sg_game_tick(1);
    assert(JNETSteamGame[27]==2 && JNETSteamGame[30]==45 && JNETSteamGame[35]==1 && JNETSteamGame[36]==45);
    fake_chirps=2;jnet_sg_game_tick(1);assert(jnet_sg_mail.outgoing_count==2 && jnet_sg_mail.outgoing_events[0].event.sequence==1 && jnet_sg_mail.outgoing_events[1].event.sequence==2);
    ++jnet_travel_epoch;fake_chirps=1;jnet_sg_game_tick(1);
    assert(jnet_sg_mail.outgoing_count==3 && jnet_sg_mail.outgoing_events[2].event.sequence==3);
    uint64_t capture_seen=jnet_sg_mail.outgoing.seen,capture_sequence=jnet_sg_mail.outgoing.sequence;
    fake_capture_result=JC_LAYOUT_RANGE;fake_chirps=1;clock_ms+=40;jnet_sg_game_tick(1);
    assert(jnet_sg_mail.outgoing_count==4 && jnet_sg_mail.outgoing_events[3].event.sequence==4);
    assert(jnet_sg_mail.outgoing.seen==capture_seen && jnet_sg_mail.outgoing.sequence==capture_sequence);
    fake_capture_result=0;clock_ms+=40;jnet_sg_game_tick(1);
    assert(jnet_sg_mail.outgoing.sequence>capture_sequence && jnet_sg_mail.outgoing.seen==clock_ms);
    for(unsigned i=0;i<2;++i){jnet_sg_mail.events[i]=game_record(42,1);jnet_sg_mail.events[i].chirp=(struct jc_chirp_event){.participant=42,.epoch=99,.sequence=1+i,.sound=1,.radius=1,.duration=1};jnet_sg_mail.events[i].chirp_seen=clock_ms;}
    jnet_sg_mail.event_count=2;jnet_sg_game_tick(1);assert(traveler_network.peers[1].wire_chirps.sequence==1 && jnet_sg_mail.event_count==1);
    traveler_network.peers[1].wire_chirps.pending=0;jnet_sg_game_tick(1);assert(traveler_network.peers[1].wire_chirps.sequence==2 && !jnet_sg_mail.event_count);
    traveler_local_state=2;traveler_actor=(void *)1;actor_idle=0;
    jnet_sg_mail.records[0]=game_record(45,1);jnet_sg_game_tick(1);
    assert(traveler_network.peers[1].connection.peer==42 && !traveler_network.peers[1].connection.admitted && traveler_network.peers[0].connection.admitted);
    traveler_local_state=4;jnet_sg_game_tick(1);
    assert(traveler_network.peers[1].connection.peer==42 && !traveler_network.peers[1].connection.admitted);
    clock_ms+=110000;jnet_sg_mail.received=clock_ms;jnet_sg_mail.records[0]=game_record(42,3);jnet_sg_game_tick(1);
    assert(traveler_network.peers[1].connection.peer==42 && traveler_network.peers[1].connection.admitted);
    jnet_lobby_view.room=999;jnet_lobby_view.member_count=3;
    jnet_lobby_view.members[0]=43;jnet_lobby_view.members[1]=44;jnet_lobby_view.members[2]=45;
    jnet_sg_mail.records[0]=game_record(45,1);jnet_sg_game_tick(1);assert(traveler_network.peers[1].connection.peer==45 && traveler_network.peers[1].connection.admitted);
    unsigned queued=jnet_sg_mail.outgoing_count;LONG expired=JNETSteamGame[17];
    capture_seen=jnet_sg_mail.outgoing.seen;fake_capture_result=JC_LAYOUT_RANGE;fake_chirps=1;
    clock_ms+=2001;jnet_sg_game_tick(1);assert(!traveler_network.peers[0].connection.admitted && !traveler_network.peers[1].connection.admitted);
    assert(jnet_sg_mail.outgoing_count==queued && JNETSteamGame[17]==expired+1 && jnet_sg_mail.outgoing.seen==capture_seen);
    fake_capture_result=0;
    clock_ms+=120001;jnet_sg_mail.received=clock_ms;jnet_sg_mail.records[0]=game_record(46,1);
    jnet_lobby_view.member_count=0;jnet_sg_game_tick(1);
    assert(traveler_network.peers[1].connection.peer==45 && !traveler_network.peers[1].connection.admitted);
    jnet_lobby_view.member_count=3;jnet_lobby_view.members[2]=46;jnet_sg_game_tick(1);
    assert(traveler_network.peers[1].connection.peer==46 && traveler_network.peers[1].connection.admitted);
    jnet_sg_mail.records[0]=game_record(44,8);memset(jnet_sg_mail.records+2,0,sizeof(jnet_sg_mail.records[2]));
    jnet_lobby_view.member_count=2;jnet_lobby_view.members[0]=43;jnet_lobby_view.members[1]=44;
    traveler_local_state=2;jnet_sg_game_tick(1);
    assert(traveler_network.peers[0].connection.peer==44 && traveler_network.peers[1].connection.peer==46);
    traveler_local_state=4;jnet_sg_game_tick(1);
    assert(traveler_network.peers[0].connection.peer==44 && !traveler_network.peers[1].connection.peer);
    traveler_actor=NULL;traveler_actor_trial_join_pending=1;jnet_sg_game_tick(1);
    assert(traveler_network.peers[0].connection.peer==44 && !traveler_network.peers[1].connection.peer);
    traveler_actor_trial_join_pending=0;jnet_sg_game_tick(1);
    assert(!traveler_network.peers[0].connection.peer && traveler_network.peers[1].connection.peer==44);
    assert(traveler_network.peers[1].connection.admitted);
    LONG retained_extra_samples=JNETSteamGame[27];
    memset(&jnet_lobby_view,0,sizeof(jnet_lobby_view));
    jnet_sg_reset();jnet_sg_game_tick(1);assert(traveler_network.paused && !JNETSteamGame[26] && !JNETSteamGame[34] && JNETSteamGame[27]==retained_extra_samples);
    jnet_travel_epoch=original_epoch;
    puts("Steam native bridge fixture: state capture, ordered chirp bursts, stable independent slots, cleanup before replacement, return after separation, stable lobby identity without invented expiry, explicit leave release and stale state expiry passed; no game engine calls.");
}
#define JNET_REALTIME_FIXTURE
#include "jnet_steam_realtime.h"
static void realtime_test(uint64_t room) {
    setup();jnet_sg_reset();memset(&jnet_sn,0,sizeof(jnet_sn));
    jnet_sl.room=room;jnet_sl.kind=0;jnet_sn.room=room;jnet_sn.generation=jnet_sl.generation;
    unsigned ri=room_index(room);rooms[ri].count=2;rooms[ri].people[0]=42;rooms[ri].people[1]=43;
    jnet_sn.peers[0].handle=500;jnet_sn.peers[0].identity=43;jnet_sn.peers[0].connected=jnet_sn.peers[0].admitted=1;
    jnet_sn_api.self=(void *)1;jnet_sn_api.receive=receive_messages;jnet_sn_api.send=send_message;
    jnet_sn_members.active=1;jnet_sn_members.loaded=0;jnet_sn_members_dirty=jnet_sl_metadata_dirty=0;
    assert(jnet_sn_member(43));jnet_sg_tick();jnet_sg_mail.outgoing=game_record(42,1);
    unsigned reads=maintenance_getters;LONG sends=JNETSteamGame[1];
    assert(jnet_realtime_service());assert(maintenance_getters==reads && JNETSteamGame[1]>sends);
    struct jnet_game_record record=game_record(43,1);unsigned char packet[JNET_GAME_BYTES];
    game_packet(packet,1,1,&record);input(0,3,1);
    incoming_message.data=packet;incoming_message.bytes=sizeof(packet);
    clock_ms+=8;assert(jnet_realtime_service());assert(jnet_sg_mail.records[1].sequence==1 && !incoming_count);
    assert(maintenance_getters==reads);
    input(0,3,2);jnet_sn_members_dirty=1;assert(!jnet_realtime_service() && incoming_count);
    jnet_sn_members_dirty=0;jnet_sl_metadata_dirty=1;assert(!jnet_realtime_service() && incoming_count);
    jnet_sl_metadata_dirty=0;uint64_t expiry=jnet_sn_members.expires;jnet_sn_members.expires=clock_ms;
    assert(!jnet_realtime_service() && incoming_count);jnet_sn_members.expires=expiry;
    ++jnet_sn.generation;assert(!jnet_realtime_service());--jnet_sn.generation;
    jnet_sn.peers[0].admitted=0;assert(!jnet_realtime_service());jnet_sn.peers[0].admitted=1;
    assert(jnet_realtime_service() && !incoming_count && maintenance_getters==reads);
    jnet_sg_reset();assert(!jnet_realtime_service());
    puts("Frame-independent service: receive/publish and send without callback frames; stale/dirty membership, generation and admission gates; no synchronous roster APIs passed.");
}
static float motion_x(const struct jc_actor_sender *sample){float x;memcpy(&x,sample->state.bytes,4);return x;}
static void motion_test(void) {
    struct jnet_game_record r=game_record(43,1);struct jc_actor_sender out;
    memset(r.sample.state.bytes,0,12);memset(jnet_motion,0,sizeof(jnet_motion));
    jnet_motion_ms=0;assert(jnet_motion_apply(0,&r,1,100,&out)==r.sequence && !memcmp(&out,&r.sample,sizeof(out)));
    jnet_motion_ms=80;memset(jnet_motion,0,sizeof(jnet_motion));
    uint64_t seq=jnet_motion_apply(0,&r,1,100,&out);assert(motion_x(&out)==0);
    float x=.8f;memcpy(r.sample.state.bytes,&x,4);++r.sequence;
    assert(jnet_motion_apply(0,&r,1,120,&out)>seq && motion_x(&out)==0);
    seq=jnet_motion_apply(0,&r,1,160,&out);assert(fabsf(motion_x(&out)-.4f)<.001f);
    assert(!memcmp(out.state.bytes+12,r.sample.state.bytes+12,JC_RECEIVER_STATE_BYTES-12));
    assert(jnet_motion_apply(0,&r,1,200,&out)>seq && motion_x(&out)==.8f);
    seq=jnet_motion_apply(0,&r,1,201,&out);assert(jnet_motion_apply(0,&r,1,210,&out)==seq);
    x=20;memcpy(r.sample.state.bytes,&x,4);++r.sequence;jnet_motion_apply(0,&r,1,220,&out);assert(motion_x(&out)==20);
    x=20.5f;memcpy(r.sample.state.bytes,&x,4);++r.sequence;++r.lease;jnet_motion_apply(0,&r,1,240,&out);assert(motion_x(&out)==20.5f);
    x=21;memcpy(r.sample.state.bytes,&x,4);++r.sequence;jnet_motion_apply(0,&r,1,600,&out);assert(motion_x(&out)==21);
    x=21.5f;memcpy(r.sample.state.bytes,&x,4);++r.sequence;r.sample.state.bytes[0x40]^=1;
    jnet_motion_apply(0,&r,1,610,&out);assert(motion_x(&out)==21.5f);
    jnet_motion_ms=0;memset(jnet_motion,0,sizeof(jnet_motion));
    puts("Motion option: byte-exact baseline, first pose, bounded interpolation, final target delivery, held sequence, large step, new lease, stall and action bypass passed.");
}
static int travel_playable=1;
static int jnet_travel_playable_menu(void){return travel_playable;}
#define JNET_TRAVEL_FIXTURE
#include "jnet_travel_live.h"
static void queue_fixture(uintptr_t game,const char *level,unsigned char a,unsigned char b){(void)game;(void)level;(void)a;(void)b;}
static void travel_test(uint64_t room) {
    for(unsigned level=0;level<12;++level) {
        assert(jnet_travel_native_loaded(level,level,0,1,1));
        assert(!jnet_travel_native_loaded(level,level,1,1,1));
        assert(!jnet_travel_native_loaded(level,level,0,0,1));
        assert(!jnet_travel_native_loaded(level,level,0,2,1));
        assert(!jnet_travel_native_loaded(level,(level+1)%12,0,1,1));
    }
    jnet_travel_reset();jnet_queue_level_original=queue_fixture;
    jnet_sl.room=0;jnet_sl_status("Loading destination - other players can continue");
    jnet_travel_busy=1;jnet_travel_complete();
    assert(!jnet_travel_busy && jnet_travel.local_loaded && JNETSteamGame[95]==1);
    assert(!strcmp(jnet_sl.status,"Destination loaded - ready to travel"));
    JNETSteamLobby[10]=0;assert(jnet_travel_begin(3));jnet_travel_reset();
    assert(jnet_travel_begin(0) && jnet_travel.chapter_select);jnet_travel_reset();
    assert(jnet_travel_begin_mode(0,0,0,0) && !jnet_travel.chapter_select);jnet_travel_reset();
    jnet_paradise_cutscene=1;
    assert(jnet_travel_begin(7) && jnet_travel.paradise);jnet_travel_reset();
    assert(jnet_travel_begin_mode(7,0,0,0) && !jnet_travel.paradise);jnet_travel_reset();
    jnet_paradise_cutscene=0;
    assert(jnet_travel_begin(7) && !jnet_travel.paradise);jnet_travel_reset();
    jnet_sl.room=room;jnet_sl.host=jnet_sl.identity=42;jnet_sl.generation=700;
    unsigned ri=room_index(room);rooms[ri].host=42;rooms[ri].count=3;
    rooms[ri].people[0]=42;rooms[ri].people[1]=43;rooms[ri].people[2]=44;
    jnet_sn_members.loaded=0;jnet_sn_members.active=1;assert(jnet_sn_member(42));
    memset(jnet_sn.peers,0,sizeof(jnet_sn.peers));
    for(unsigned i=0;i<2;++i){jnet_sn.peers[i].identity=43+i;jnet_sn.peers[i].handle=900+i;jnet_sn.peers[i].connected=jnet_sn.peers[i].admitted=1;}
    struct jnet_lobby_view view={.room=room,.host=42};jnet_travel_members_view(&view);
    assert(view.member_count==3 && view.members[0]==42 && view.members[2]==44);
    JNETSteamLobby[10]=0;
    assert(jnet_travel_native_request(0,0) && jnet_travel.restart && !jnet_travel.chapter_select);
    assert(jc_packet_get(travel_sent+92,4)==2);jnet_travel_reset();
    for(unsigned guest=0;guest<2;++guest){
        jnet_sl.room=room;jnet_sl.host=42;jnet_sl.identity=guest?43:42;
        jnet_travel_busy=1;jnet_travel.room=room;jnet_travel_epoch=9;
        jnet_sl.deferred.kind=2;
        uint64_t generation=jnet_sl.generation;
        jnet_travel_leave_for_title();
        assert(!jnet_sl.room && !jnet_sl.host && !jnet_sl.deferred.kind);
        assert(!jnet_travel_busy && !jnet_travel.room && !jnet_travel_epoch);
        assert(jnet_sl.generation==generation+1);
        assert(!jnet_travel_native_request(0,0));
    }
    jnet_sl.room=room;jnet_sl.host=jnet_sl.identity=42;
    assert(jnet_travel_native_request(0,1) && jnet_travel.restart && jnet_travel.chapter_select);
    assert(jc_packet_get(travel_sent+92,4)==3);jnet_travel_reset();
    assert(jnet_travel_native_request(3,0) && jnet_travel_busy && jnet_travel.level==3);
    jnet_travel_reset();jnet_sl.identity=43;
    assert(jnet_travel_native_request(4,0) && !jnet_travel_busy);
    jnet_sl.identity=42;
    JNETSteamLobby[10]=0;assert(!jnet_travel_begin(12));assert(jnet_travel_begin(8));
    assert(jnet_travel_busy && jnet_travel_epoch==1 && jc_packet_get(travel_sent+56,4)==8);
    assert(jc_packet_get(travel_sent+92,4)==2 && jnet_travel.restart);
    unsigned char proposal[104];memcpy(proposal,travel_sent,104);
    assert(!jnet_travel_begin(9));jnet_travel.local_loaded=1;jnet_travel.loaded=1;
    jc_packet_put(proposal+4,2,4);proposal[16]^=1;jnet_travel_message(0,proposal,104);assert(jnet_travel.loaded==1);proposal[16]^=1;
    jnet_travel_message(0,proposal,104);assert(jnet_travel.loaded==3);jnet_travel_poll_live();assert(!jnet_travel_busy);
    jnet_travel_message(1,proposal,104);assert(jnet_travel.loaded==7);jnet_travel_poll_live();assert(!jnet_travel_busy && jc_packet_get(travel_sent+4,4)==1);
    jnet_travel_reset();jnet_sl.identity=43;jnet_sn.peers[0].identity=42;
    jc_packet_put(proposal+4,1,4);jnet_travel_message(0,proposal,95);assert(!jnet_travel_busy);
    jnet_travel_message(1,proposal,104);assert(!jnet_travel_busy);
    jnet_travel_message(0,proposal,104);assert(jnet_travel_busy && jnet_travel.level==8);
    jc_packet_put(proposal+56,9,4);jnet_travel_message(0,proposal,104);assert(jnet_travel.level==8);jc_packet_put(proposal+56,8,4);
    jc_packet_put(proposal+4,3,4);jnet_travel_message(0,proposal,104);assert(jnet_travel_busy);
    jnet_travel.local_loaded=1;jnet_travel_poll_live();assert(!jnet_travel_busy);
    jnet_travel_message(0,proposal,104);assert(!jnet_travel_busy);
    jnet_travel_reset();jc_packet_put(proposal+4,1,4);jc_packet_put(proposal+56,7,4);jc_packet_put(proposal+88,0,4);jc_packet_put(proposal+92,6,4);
    jnet_travel_message(0,proposal,104);assert(jnet_travel_busy && jnet_travel.paradise && jnet_travel.level==7);
    jnet_travel_reset();jc_packet_put(proposal+92,4,4);
    jnet_travel_message(0,proposal,104);assert(!jnet_travel_busy);
    jc_packet_put(proposal+56,8,4);
    jnet_travel_reset();jc_packet_put(proposal+4,1,4);jc_packet_put(proposal+92,1,4);
    jnet_travel_message(0,proposal,104);assert(!jnet_travel_busy);
    jc_packet_put(proposal+56,0,4);jc_packet_put(proposal+88,0,4);
    jnet_travel_message(0,proposal,104);assert(jnet_travel_busy && jnet_travel.chapter_select);
    jnet_travel.queued=1;jc_packet_put(proposal+48,jnet_travel_epoch+1,8);
    jnet_travel_message(0,proposal,104);assert(jnet_travel.queued==0);
    jnet_travel_reset();assert(!jnet_travel_epoch);jnet_sl.identity=42;
    jnet_sn.peers[0].identity=43;assert(jnet_travel_begin(9));clock_ms=jnet_travel.deadline+1;
    jnet_travel_poll_live();assert(!jnet_travel_busy && jnet_sl.room==room);
    jnet_travel_reset();jnet_current_level=6;JNETSteamLobby[9]=1;
    memset(jnet_sg.peers,0,sizeof(jnet_sg.peers));
    jnet_sn.peers[1].admitted=jnet_sn.peers[1].connected=0;
    rooms[ri].count=2;jnet_sn_members.loaded=0;
    travel_playable=0;jnet_travel_poll_live();assert(!jnet_travel_busy);
    travel_playable=1;jnet_travel_poll_live();assert(jnet_travel_busy && jnet_travel.level==6);
    unsigned char first_join[104];memcpy(first_join,travel_sent,104);
    jnet_travel.local_loaded=1;jnet_travel_poll_live();assert(!jnet_travel_busy);
    rooms[ri].count=3;jnet_sn_members.loaded=0;
    jnet_sn.peers[1].admitted=jnet_sn.peers[1].connected=1;
    jnet_travel_poll_live();assert(jnet_travel_busy && jnet_travel_epoch==2);
    unsigned char second_join[104];memcpy(second_join,travel_sent,104);
    jnet_travel_reset();jnet_sl.identity=43;jnet_sn.peers[0].identity=42;
    jnet_travel_message(0,first_join,104);assert(jnet_travel_busy && jnet_travel_epoch==1);
    jnet_travel.queued=1;jnet_travel.init_before=77;
    jnet_travel_message(0,second_join,104);
    assert(jnet_travel_epoch==2 && jnet_travel.queued==1 && jnet_travel.init_before==77);
    jnet_travel.local_loaded=1;jnet_travel_poll_live();assert(!jnet_travel_busy && jnet_sl.room==room);
    jnet_travel_reset();jnet_current_level=2;
    jnet_sl.identity=42;jnet_sn.peers[0].identity=43;
    jnet_daynight_mode=2;jnet_daynight_elapsed=1350000;
    assert(jnet_travel_begin(3));
    assert(jc_packet_get(travel_sent+96,4)==2 && jc_packet_get(travel_sent+100,4)==1350000);
    memcpy(proposal,travel_sent,sizeof(proposal));
    jnet_travel_reset();jnet_sl.identity=43;jnet_sn.peers[0].identity=42;
    jc_packet_put(proposal+100,1800000,4);jnet_travel_message(0,proposal,sizeof(proposal));assert(!jnet_travel_busy);
    jc_packet_put(proposal+100,1350000,4);jc_packet_put(proposal+96,3,4);
    jnet_travel_message(0,proposal,sizeof(proposal));assert(!jnet_travel_busy);
    jc_packet_put(proposal+96,2,4);jnet_travel_message(0,proposal,sizeof(proposal));
    assert(jnet_travel_busy && jnet_daynight_mode==2 && jnet_daynight_sync==1350001);
    jnet_daynight_sync=0;jnet_travel_message(0,proposal,sizeof(proposal));assert(!jnet_daynight_sync);
    jnet_travel_reset();jnet_sl.identity=42;jnet_sn.peers[0].identity=43;
    jnet_daynight_mode=3;assert(jnet_travel_native_request(3,0));
    assert(jnet_travel.lighting==0 && !jnet_travel.cycle_ms && !jnet_daynight_mode);
    jnet_travel_reset();jnet_daynight_host=~0u;
    (void)jnet_travel_levels;
    puts("Live travel protocol with fake Steam: host authority, independent local completion, authenticated nonce, replay, invalid destination, ignored legacy resume, local timeout without lobby loss and session reset passed. Native loading not exercised.");
}
#define JNET_CHAT_FIXTURE 1
#include "jnet_chat.h"
#include "jnet_chat_live.h"
static void chat_test(void) {
    setup();memset(&jnet_sn,0,sizeof(jnet_sn));memset(&jnet_chat_session,0,sizeof(jnet_chat_session));
    jnet_sl.room=1000;jnet_sl.host=42;jnet_sl.chat=1;
    memset(jnet_sl.nonce,'a',32);jnet_sl.nonce[32]=0;
    jnet_sn_api.self=(void *)1;jnet_sn_api.send=send_message;
    jnet_sn.peers[0].handle=701;jnet_sn.peers[0].identity=43;jnet_sn.peers[0].admitted=1;
    jnet_sn.peers[1].handle=702;jnet_sn.peers[1].identity=44;jnet_sn.peers[1].admitted=1;
    struct jnet_lobby_view v={.room=1000,.self=42,.host=42,.members={42,43,44},.member_count=3,.chat=1};
    clock_ms=10000;jnet_chat_lobby(&v);assert(jnet_chat_allowed && v.member_names[0][0] && v.host_name[0]);
    assert(strstr(jnet_chat.lines[4],"connected"));
    unsigned char packet[184]={0};memcpy(packet,"JNC1",4);uint64_t source=43,seq=1;memcpy(packet+8,&v.room,8);memcpy(packet+16,&source,8);memcpy(packet+24,&seq,8);memcpy(packet+32,jnet_sl.nonce,32);memcpy(packet+64,"Hello",6);
    unsigned sends=chat_sends;assert(jnet_chat_message(0,packet,sizeof(packet)));assert(chat_sends==sends+1 && strstr(jnet_chat.lines[4],"Hello"));
    jnet_chat_message(0,packet,sizeof(packet));assert(chat_sends==sends+1);
    seq=2;memcpy(packet+24,&seq,8);jnet_chat_message(0,packet,sizeof(packet));assert(chat_sends==sends+1); /* rate */
    clock_ms+=1000;seq=3;memcpy(packet+24,&seq,8);source=44;memcpy(packet+16,&source,8);jnet_chat_message(0,packet,sizeof(packet));assert(chat_sends==sends+1); /* forged sender */
    source=43;memcpy(packet+16,&source,8);packet[32]='b';jnet_chat_message(0,packet,sizeof(packet));assert(chat_sends==sends+1);packet[32]='a';
    jnet_sl.chat=0;jnet_chat_message(0,packet,sizeof(packet));assert(chat_sends==sends+1);jnet_sl.chat=1;
    jnet_chat_message(0,packet,183);assert(chat_sends==sends+1);
    packet[64]=1;jnet_chat_message(0,packet,sizeof(packet));assert(chat_sends==sends+1);packet[64]='H';
    strcpy(jnet_chat.pending,"Host message");jnet_chat_lobby(&v);assert(chat_sends==sends+3 && strstr(jnet_chat.lines[4],"Host message"));
    /* A guest accepts a member's relay only from the admitted host. */
    jnet_sl.identity=44;jnet_sl.host=42;jnet_sn.peers[0].identity=42;
    clock_ms+=1000;seq=4;memcpy(packet+24,&seq,8);jnet_chat_message(1,packet,sizeof(packet));assert(jnet_chat_session.received[1]==2);
    jnet_chat_message(0,packet,sizeof(packet));assert(jnet_chat_session.received[1]==4 && chat_sends==sends+3);
    jnet_sl.identity=42;jnet_sn.peers[0].identity=43;
    v.member_count=2;v.members[2]=0;jnet_chat_lobby(&v);assert(strstr(jnet_chat.lines[4],"disconnected"));
    v.room=0;jnet_chat_lobby(&v);assert(!jnet_chat_allowed && !jnet_chat_focus && !jnet_chat.pending[0]);
    v.room=1001;v.chat=0;jnet_chat_lobby(&v);assert(!jnet_chat_allowed && !jnet_chat.lines[4][0]);
    (void)jnet_chat_gameplay;
    memset(&jnet_sl,0,sizeof(jnet_sl));memset(&jnet_sn,0,sizeof(jnet_sn));
    puts("Lobby chat: authenticated host relay, spoof/nonce/replay/rate/length/control-character rejection, disabled defaults, roster notices and room reset passed with fake Steam.");
}
static unsigned solo_destination=99;
static int solo_request(unsigned level){solo_destination=level;return 1;}
#ifndef JNET_LOBBY_TEST_MAIN
#define JNET_LOBBY_TEST_MAIN main
#endif
int JNET_LOBBY_TEST_MAIN(void) {
    chat_test();
    jnet_travel_request=solo_request;JNETSteam[0]=0;
    jnet_lobby_action=(struct jnet_lobby_action){.kind=6,.level=6};
    jnet_sl_tick_inner();assert(solo_destination==6);jnet_travel_request=NULL;

    setup();
    char code[5];assert(jnet_sl_code("ab2z",code) && !strcmp(code,"AB2Z"));assert(!jnet_sl_code("abc",code) && !jnet_sl_code("abcde",code));
    assert(!jnet_sl_decimal("18446744073709551616") && !jnet_sl_decimal("42x"));
    jnet_sl_guard=1;assert(jnet_sl_search_hook(jnet_sl_api.mm)==0 && JNETSteamLobby[2]==1);
    jnet_sl_search_original=search;++jnet_sl_owned;uint64_t call=jnet_sl_search_hook(jnet_sl_api.mm);--jnet_sl_owned;assert(call);
    struct jnet_lobby_action anonymous={.kind=1,.access=0,.cmatw=0,.chat=1};
    assert(jnet_lobby_submit(&anonymous));jnet_sl_tick();
    assert(jnet_sl.kind==4 && create_count==1 && !jnet_sl.chat && !jnet_sl.cmatw);
    uint64_t public_room=add_room(42);finish_create(public_room);assert(jnet_sl.room==public_room && jnet_sl_valid(public_room,42));
    set_data((void *)1,public_room,"journet.protocol","steam-game-12");
    assert(jnet_sl_metadata_result(public_room,42)==2);
    set_data((void *)1,public_room,"journet.protocol",JNET_STEAM_PROTOCOL);
    set_data((void *)1,public_room,"journet.build","different-assets");
    assert(jnet_sl_metadata_result(public_room,42)==9);
    set_data((void *)1,public_room,"journet.build",JC_BUILD_ID_HEX);
    assert(!jnet_sl_metadata_result(public_room,42));
    assert(jnet_sl_equal(public_room,"journet.chat","0") && jnet_sl_equal(public_room,"journet.cmatw","0"));
    assert(jnet_sl_equal(public_room,"journet.visibility","public") && !strcmp(data((void *)1,public_room,"journet.code"),""));
    selected[0]=public_room;selected_count=1;jnet_sl_search(1);finish_search(1);assert(jnet_sl.count==1 && worldwide==1 && filters==4);
    assert(!strcmp(jnet_sl.names[0],"Random's Lobby"));
    action(3,0,NULL,0,0);assert(!jnet_sl.room && leave_count==1);
    action(1,1,NULL,0,0);assert(jnet_sl.kind==2 && strlen(jnet_sl.code)==4);finish_search(0);assert(jnet_sl.kind==4);
    uint64_t private_room=add_room(42);finish_create(private_room);assert(jnet_sl.kind==6 && jnet_sl_equal(private_room,"journet.visibility","code_only"));
    selected[0]=private_room;finish_search(1);assert(jnet_sl.room==private_room && !jnet_sl.call);
    selected[0]=private_room;jnet_sl_search(1);finish_search(1);assert(jnet_sl.count==0);
    action(3,0,NULL,0,0);
    set_data((void *)1,private_room,"journet.code","ABCD");
    action(2,0,"ABCD",0,0);finish_search(0);assert(strstr(jnet_sl.status,"No joinable"));
    action(2,0,"ABCD",0,0);selected_count=2;selected[0]=private_room;selected[1]=public_room;finish_search(2);assert(strstr(jnet_sl.status,"Ambiguous") && !join_count);
    action(2,0,"ABCD",0,0);selected_count=1;selected[0]=private_room;finish_search(1);assert(jnet_sl.kind==5);finish_join(private_room,4);assert(!jnet_sl.room && JNETSteamLobby[4]==4);
    action(2,0,NULL,public_room,42);action(3,0,NULL,0,0);unsigned before=leave_count;finish_join(public_room,1);assert(!jnet_sl.room && leave_count==before+1);
    action(1,0,NULL,0,0);clock_ms+=30001;jnet_sl_poll_result();assert(jnet_sl.cancelled && jnet_sl.call);before=leave_count;finish_create(public_room);assert(!jnet_sl.room && leave_count==before+1);
    action(1,0,NULL,0,0);finish_create(public_room);rooms[0].host=99;clock_ms+=1000;jnet_sl_tick();assert(!jnet_sl.room && strstr(jnet_sl.status,"Host left"));rooms[0].host=42;
    if(!jnet_sl.call)jnet_sl_search(1);action(1,0,NULL,0,0);assert(jnet_sl.deferred.kind==1);pending.result.count=0;pending.done=1;jnet_sl_tick();assert(jnet_sl.kind==4 && !jnet_sl.deferred.kind);finish_create(public_room);action(3,0,NULL,0,0);
    JNETSteamLobby[9]=0;before=create_count;action(1,0,NULL,0,0);assert(create_count==before && strstr(jnet_sl.status,"Finish loading"));JNETSteamLobby[9]=1;
    JNETSteamLobby[10]=1;action(1,0,NULL,0,0);assert(create_count==before && strstr(jnet_sl.status,"Native companion"));JNETSteamLobby[10]=0;
    uint64_t ordinary=add_room(55);set_data((void *)1,ordinary,"level","2");
    before=join_count;action(2,0,NULL,ordinary,55);assert(join_count==before && !jnet_sl.call && strstr(jnet_sl.status,"not a compatible"));
    JNETSteamLobby[9]=0;before=join_count;
    action(2,0,NULL,public_room,42);assert(join_count==before+1);finish_join(public_room,1);
    assert(jnet_sl.room==public_room);clock_ms+=501;jnet_sl_tick();assert(jnet_sl.room==public_room);
    action(3,0,NULL,0,0);JNETSteamLobby[9]=1;
    assert(rooms[room_index(public_room)].type==3);
    for(int level=-1000;level<1000;++level){char numeric[16];snprintf(numeric,sizeof(numeric),"%d",level);assert(strcmp(data((void *)1,public_room,"level"),numeric));}
    uint32_t bytes=99;uint64_t peer=55;
    assert(!jnet_sl_send_hook((void *)5,peer,"test",4,2,0));
    assert(!jnet_sl_available_hook((void *)5,&bytes,0) && !bytes);
    assert(!jnet_sl_read_hook((void *)5,NULL,0,&bytes,&peer,0) && !bytes && !peer);
    assert(!jnet_sl_accept_hook((void *)5,55));
    assert(!jnet_sl_legacy_block((void *)6));jnet_sl_guard=0;assert(!jnet_sl_legacy_block((void *)5));jnet_sl_guard=1;
    assert(!owner((void *)1,public_room));
    selected_count=1;selected[0]=public_room;jnet_sl_search(1);finish_search(1);
    assert(jnet_sl.count==1 && JNETSteamLobby[17]==1 && !JNETSteamLobby[18]);
    action(2,0,NULL,public_room,42);assert(jnet_sl.kind==5);finish_join(public_room,1);assert(jnet_sl.room==public_room);
    assert(jnet_sl_valid_session(public_room,42,jnet_sl.nonce));
    jnet_sl_metadata_events=1;jnet_sl_metadata_cache.valid=0;
    assert(jnet_sl_maintain_session());LONG validations=JNETSteamLobby[26];
    assert(jnet_sl_maintain_session() && JNETSteamLobby[26]==validations);
    struct jnet_sn_data_event data_event={.room=public_room+1,.member=public_room+1,.success=1};
    jnet_sn_data_dispatch(NULL,&data_event);assert(jnet_sl_maintain_session() && JNETSteamLobby[26]==validations);
    data_event.room=public_room;data_event.member=43;jnet_sn_data_dispatch(NULL,&data_event);
    assert(jnet_sl_maintain_session() && JNETSteamLobby[26]==validations);
    data_event.member=public_room;jnet_sn_data_dispatch(NULL,&data_event);
    assert(jnet_sl_maintain_session() && JNETSteamLobby[26]==validations+1);
    clock_ms+=2000;assert(jnet_sl_maintain_session() && JNETSteamLobby[26]==validations+2);
    rooms[0].host=77;assert(!jnet_sl_maintain_session());rooms[0].host=42;
    ++jnet_sl.generation;assert(jnet_sl_maintain_session() && JNETSteamLobby[26]==validations+3);
    jnet_sl_metadata_events=0;assert(jnet_sl_maintain_session() && JNETSteamLobby[26]==validations+4);

    assert(!jnet_sl_valid_session(public_room,43,jnet_sl.nonce));
    char different_nonce[33];memcpy(different_nonce,jnet_sl.nonce,33);different_nonce[0]=different_nonce[0]=='a'?'b':'a';
    assert(!jnet_sl_valid_session(public_room,42,different_nonce));
    jnet_sl_metadata_events=1;assert(jnet_sl_maintain_session());
    set_data((void *)1,public_room,"journet.nonce",different_nonce);jnet_sn_data_dispatch(NULL,&data_event);
    assert(!jnet_sl_maintain_session());
    set_data((void *)1,public_room,"journet.nonce",jnet_sl.nonce);assert(jnet_sl_maintain_session());
    set_data((void *)1,public_room,"journet.nonce",different_nonce);clock_ms+=2000;
    assert(!jnet_sl_maintain_session());
    set_data((void *)1,public_room,"journet.nonce",jnet_sl.nonce);jnet_sl_metadata_events=0;
    puts("Session metadata: notifications, unrelated/member events, missed-event fallback, generation invalidation, owner transfer, nonce changes and uncached admission verified.");
    action(3,0,NULL,0,0);
    action(2,0,NULL,public_room,42);assert(jnet_sl.kind==5);rooms[0].host=77;finish_join(public_room,1);assert(!jnet_sl.room && strstr(jnet_sl.status,"incompatible"));rooms[0].host=42;
    selected[0]=ordinary;jnet_sl_search(1);finish_search(1);assert(!jnet_sl.count && JNETSteamLobby[18]==1 && JNETSteamLobby[19]==1);
    before=search_count;action(5,0,NULL,0,0);assert(search_count==before+1 && jnet_sl.kind==1);finish_search(0);
    clock_ms+=60000;before=search_count;jnet_sl_tick();assert(search_count==before);
    rooms[0].local_member=1;
    transport_test(public_room);
    membership_poll_test(public_room);
    gameplay_test(public_room);
    character_switch_matrix(public_room);
    game_bridge_test();
    metrics_test();
    arrival_test();
    maintenance_test(public_room);
    realtime_test(public_room);motion_test();travel_test(public_room);
    puts("Actual Steam lobby/P2P adapter with fake Steam: public/code filters, generated code, ambiguity, failed/cancelled/late joins, late create cleanup, host departure, search serialization, native-call ownership, Desert guard, callback admission, identity/nonce validation, capacity, reliable handshake, unreliable heartbeat, duplicate rejection, message release and independent cleanup passed. Live Steam not exercised.");
    return 0;
}
