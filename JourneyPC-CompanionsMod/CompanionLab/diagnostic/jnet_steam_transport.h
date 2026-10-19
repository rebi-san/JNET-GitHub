#ifndef JNET_STEAM_TRANSPORT_H
#define JNET_STEAM_TRANSPORT_H
static void jnet_sg_reset(void);
static void jnet_sg_tick(void);
static int jnet_sg_receive(unsigned slot,const unsigned char *data,size_t bytes);
__declspec(dllexport) volatile LONG JNETSteamP2P[305];
__declspec(dllexport) char JNETSteamP2PText[128];
#pragma pack(push,1)
struct jnet_sn_identity { int32_t type,size;unsigned char value[128]; };
#pragma pack(pop)
struct jnet_sn_info {
    struct jnet_sn_identity remote;int64_t userdata;uint32_t listener;unsigned char address[18];uint16_t pad;
    uint32_t remote_pop,relay_pop;int32_t state,reason;char debug[128],description[128];int32_t flags;uint32_t reserved[63];
};
struct jnet_sn_event { uint32_t connection,pad;struct jnet_sn_info info;int32_t old,pad2; };
struct jnet_sn_message {
    void *data;int32_t bytes;uint32_t connection;struct jnet_sn_identity peer;
    int64_t userdata,time,number;void (*free_data)(struct jnet_sn_message *);void (*release)(struct jnet_sn_message *);
};
struct jnet_sn_option { int32_t key,type;void *value; };
_Static_assert(sizeof(struct jnet_sn_identity)==136,"Steam identity ABI");
_Static_assert(sizeof(struct jnet_sn_info)==696,"Steam connection info ABI");
_Static_assert(sizeof(struct jnet_sn_event)==712,"Steam connection callback ABI");
_Static_assert(offsetof(struct jnet_sn_message,release)==184,"Steam message release ABI");
_Static_assert(sizeof(struct jnet_sn_option)==16,"Steam config ABI");
struct jnet_sn_realtime {
    int32_t state,ping;float quality_local,quality_remote,out_packets,out_bytes,in_packets,in_bytes;
    int32_t send_rate,pending_unreliable,pending_reliable,unacked;
    int64_t queue_us;uint32_t reserved[16];
};
_Static_assert(sizeof(struct jnet_sn_realtime)==120 && offsetof(struct jnet_sn_realtime,queue_us)==48,"Steam realtime status ABI");
static uint64_t jnet_sn_next_metrics;
static int64_t (*jnet_sn_timestamp)(void *);
static struct {uint32_t handle;int64_t received;uint64_t processed;} jnet_sn_arrivals[7];
static void jnet_sn_arrival(unsigned slot,const struct jnet_sn_message *m) {
    unsigned at=109+slot*8;uint64_t now=GetTickCount64();
    if(jnet_sn_arrivals[slot].handle!=m->connection) {
        memset(jnet_sn_arrivals+slot,0,sizeof(jnet_sn_arrivals[slot]));jnet_sn_arrivals[slot].handle=m->connection;
        for(unsigned j=0;j<8;++j)JNETSteamP2P[at+j]=0;
    }
    int64_t stamp=jnet_sn_timestamp?jnet_sn_timestamp(jnet_steam_service.utils):0;
    if(m->time>0 && stamp>=m->time) {
        LONG age=(LONG)((stamp-m->time)/1000>60000?60000:(stamp-m->time)/1000);
        JNETSteamP2P[at+4]=age;if(age>JNETSteamP2P[at+5])JNETSteamP2P[at+5]=age;
        if(jnet_sn_arrivals[slot].received && m->time>=jnet_sn_arrivals[slot].received) {
            LONG gap=(LONG)((m->time-jnet_sn_arrivals[slot].received)/1000>60000?60000:(m->time-jnet_sn_arrivals[slot].received)/1000);
            JNETSteamP2P[at]=gap;if(gap>JNETSteamP2P[at+1])JNETSteamP2P[at+1]=gap;
        }
        jnet_sn_arrivals[slot].received=m->time;
    } else JNETSteamP2P[at+4]=-1;
    if(jnet_sn_arrivals[slot].processed) {
        LONG gap=(LONG)(now-jnet_sn_arrivals[slot].processed>60000?60000:now-jnet_sn_arrivals[slot].processed);
        JNETSteamP2P[at+2]=gap;if(gap>JNETSteamP2P[at+3])JNETSteamP2P[at+3]=gap;
    }
    jnet_sn_arrivals[slot].processed=now;++JNETSteamP2P[at+6];
}
static struct {
    void *self;
    uint32_t (*listen)(void *,int,int,const struct jnet_sn_option *),(*connect)(void *,const struct jnet_sn_identity *,int,int,const struct jnet_sn_option *);
    int (*realtime)(void *,uint32_t,struct jnet_sn_realtime *,int,void *);
    int (*accept)(void *,uint32_t),(*send)(void *,uint32_t,const void *,uint32_t,int,int64_t *),(*receive)(void *,uint32_t,struct jnet_sn_message **,int);
    unsigned char (*close)(void *,uint32_t,int,const char *,unsigned char),(*close_listener)(void *,uint32_t),(*info)(void *,uint32_t,struct jnet_sn_info *),(*userdata)(void *,uint32_t,int64_t);
} jnet_sn_api;
static struct {
    uint32_t listener;uint64_t room,generation;unsigned failed;
    struct {uint32_t handle;uint64_t identity,generation,deadline,next_send,sequence,last_received[2];unsigned connected,admitted,hello;} peers[7];
} jnet_sn;
static struct {struct jnet_sn_event events[32];unsigned count,overflow;} jnet_sn_queue;
static SRWLOCK jnet_sn_lock=SRWLOCK_INIT;
static uint64_t jnet_sn_id(const struct jnet_sn_identity *id) {
    uint64_t value=0;if(id->type==16 && id->size==8)memcpy(&value,id->value,8);return value;
}
static struct {uint64_t room,generation,expires,ids[JNET_STEAM_CAPACITY];unsigned active,loaded,count;} jnet_sn_members;
static volatile LONG jnet_sn_members_dirty;
static unsigned jnet_sn_fast_poll;
static void jnet_sn_cost(unsigned index,uint64_t started) {
    LONG cost=(LONG)(GetTickCount64()-started);JNETSteamP2P[index]=cost;
    if(cost>JNETSteamP2P[index+1])JNETSteamP2P[index+1]=cost;
}
static int jnet_sn_member(uint64_t id) {
    if(!jnet_sl.room || !id)return 0;
    ++JNETSteamP2P[13];
    if(jnet_sn_fast_poll) {
        ++JNETSteamP2P[15];
        for(unsigned i=0;i<jnet_sn_members.count;++i)if(jnet_sn_members.ids[i]==id)return 1;
        return 0;
    }
    uint64_t now=GetTickCount64();
    if(InterlockedExchange(&jnet_sn_members_dirty,0))jnet_sn_members.loaded=0;
    if(!jnet_sn_members.active || !jnet_sn_members.loaded || now>=jnet_sn_members.expires || jnet_sn_members.room!=jnet_sl.room || jnet_sn_members.generation!=jnet_sl.generation) {
        jnet_sn_members.room=jnet_sl.room;jnet_sn_members.generation=jnet_sl.generation;
        jnet_sn_members.loaded=1;jnet_sn_members.count=0;jnet_sn_members.expires=now+500;
        ++JNETSteamP2P[14];int count=jnet_sl_api.members(jnet_sl_api.mm,jnet_sl.room);
        if(count>=1 && count<=JNET_STEAM_CAPACITY) {
            for(int i=0;i<count;++i){++JNETSteamP2P[14];jnet_sn_members.ids[i]=jnet_sl_api.member(jnet_sl_api.mm,jnet_sl.room,i);}
            jnet_sn_members.count=(unsigned)count;
        }
        jnet_sn_cost(17,now);
    } else ++JNETSteamP2P[15];
    for(unsigned i=0;i<jnet_sn_members.count;++i)if(jnet_sn_members.ids[i]==id)return 1;
    return 0;
}
static void jnet_sn_callback(struct jnet_sn_event *event) {
    int owned=event && event->connection && ((jnet_sn.listener && event->info.listener==jnet_sn.listener));
    if(event)for(unsigned i=0;i<7 && !owned;++i)owned=jnet_sn.peers[i].handle && jnet_sn.peers[i].handle==event->connection;
    if(!owned){InterlockedIncrement(&JNETSteamP2P[9]);return;}
    InterlockedIncrement(&JNETSteamP2P[0]);
    AcquireSRWLockExclusive(&jnet_sn_lock);
    if(jnet_sn_queue.count<32)jnet_sn_queue.events[jnet_sn_queue.count++]=*event;
    else {++jnet_sn_queue.overflow;jnet_sn_api.close(jnet_sn_api.self,event->connection,1001,"JOURNET callback queue full",0);}
    ReleaseSRWLockExclusive(&jnet_sn_lock);
}
struct jnet_sn_callback_base { void **table;unsigned char flags,pad[3];int32_t id; };
_Static_assert(sizeof(struct jnet_sn_callback_base)==16 && offsetof(struct jnet_sn_callback_base,id)==12,"Steam callback object ABI");
static void (*jnet_sn_register_fn)(void *,int);
static void (*jnet_sn_unregister_fn)(void *);
static void jnet_sn_dispatch(void *self,void *event) { (void)self;jnet_sn_callback(event); }
static void jnet_sn_dispatch_result(void *self,void *event,unsigned char failed,uint64_t call) { (void)call;if(!failed)jnet_sn_dispatch(self,event); }
static int jnet_sn_callback_size(void *self) { (void)self;return sizeof(struct jnet_sn_event); }
static void *jnet_sn_callback_table[]={(void *)jnet_sn_dispatch_result,(void *)jnet_sn_dispatch,(void *)jnet_sn_callback_size};
static struct jnet_sn_callback_base jnet_sn_registration={jnet_sn_callback_table,0,{0},1221};
struct jnet_sn_lobby_event {uint64_t room,changed,actor;uint32_t flags,pad;};
_Static_assert(sizeof(struct jnet_sn_lobby_event)==32 && offsetof(struct jnet_sn_lobby_event,flags)==24,"Steam lobby member callback ABI");
static void jnet_sn_members_dispatch(void *self,void *payload) {
    (void)self;const struct jnet_sn_lobby_event *event=payload;
    if(event && event->room && event->room==jnet_sl.room){InterlockedExchange(&jnet_sn_members_dirty,1);InterlockedExchange(&jnet_sl_metadata_dirty,1);++JNETSteamP2P[16];}
}
static void jnet_sn_members_result(void *self,void *payload,unsigned char failed,uint64_t call){(void)call;if(!failed)jnet_sn_members_dispatch(self,payload);}
static int jnet_sn_members_size(void *self){(void)self;return sizeof(struct jnet_sn_lobby_event);}
static void *jnet_sn_members_table[]={(void *)jnet_sn_members_result,(void *)jnet_sn_members_dispatch,(void *)jnet_sn_members_size};
static struct jnet_sn_callback_base jnet_sn_members_registration={jnet_sn_members_table,0,{0},506};
struct jnet_sn_data_event {uint64_t room,member;unsigned char success,pad[7];};
_Static_assert(sizeof(struct jnet_sn_data_event)==24 && offsetof(struct jnet_sn_data_event,success)==16,"Steam lobby data callback ABI");
static void jnet_sn_data_dispatch(void *self,void *payload) {
    (void)self;const struct jnet_sn_data_event *event=payload;
    if(event && event->room && event->room==jnet_sl.room && (event->member==event->room || !event->success)) {
        InterlockedExchange(&jnet_sl_metadata_dirty,1);++JNETSteamLobby[28];
    }
}
static void jnet_sn_data_result(void *self,void *payload,unsigned char failed,uint64_t call){(void)call;if(!failed)jnet_sn_data_dispatch(self,payload);}
static int jnet_sn_data_size(void *self){(void)self;return sizeof(struct jnet_sn_data_event);}
static void *jnet_sn_data_table[]={(void *)jnet_sn_data_result,(void *)jnet_sn_data_dispatch,(void *)jnet_sn_data_size};
static struct jnet_sn_callback_base jnet_sn_data_registration={jnet_sn_data_table,0,{0},505};
static int jnet_sn_register(void) {
    if((jnet_sn_registration.flags&1) && (jnet_sn_members_registration.flags&1) && (jnet_sn_data_registration.flags&1))return 1;
    HMODULE module=GetModuleHandleW(L"steam_api64.dll");
    jnet_sn_register_fn=(void *)GetProcAddress(module,"SteamAPI_RegisterCallback");
    jnet_sn_unregister_fn=(void *)GetProcAddress(module,"SteamAPI_UnregisterCallback");
    if(!jnet_sn_register_fn || !jnet_sn_unregister_fn){JNETSteamP2P[8]=-1;return 0;}
    InterlockedIncrement(&JNETSteamP2P[10]);
    if(!(jnet_sn_registration.flags&1))jnet_sn_register_fn(&jnet_sn_registration,1221);
    if(!(jnet_sn_members_registration.flags&1))jnet_sn_register_fn(&jnet_sn_members_registration,506);
    if(!(jnet_sn_data_registration.flags&1))jnet_sn_register_fn(&jnet_sn_data_registration,505);
    JNETSteamP2P[8]=((jnet_sn_registration.flags&1) && (jnet_sn_members_registration.flags&1) && (jnet_sn_data_registration.flags&1))?1:-2;
    jnet_sl_metadata_events=JNETSteamP2P[8]==1;
    if(JNETSteamP2P[8]!=1){
        if(jnet_sn_registration.flags&1)jnet_sn_unregister_fn(&jnet_sn_registration);
        if(jnet_sn_members_registration.flags&1)jnet_sn_unregister_fn(&jnet_sn_members_registration);
        if(jnet_sn_data_registration.flags&1)jnet_sn_unregister_fn(&jnet_sn_data_registration);
    }return JNETSteamP2P[8]==1;
}
static void jnet_sn_drop(unsigned slot) {
    unsigned at=25+slot*12,saved=165+slot*20;
    if(jnet_sn.peers[slot].handle && (uint32_t)JNETSteamP2P[at+2]==jnet_sn.peers[slot].handle) {
        for(unsigned i=0;i<12;++i)JNETSteamP2P[saved+i]=JNETSteamP2P[at+i];
        for(unsigned i=0;i<8;++i)JNETSteamP2P[saved+12+i]=JNETSteamP2P[109+slot*8+i];
    }
    if(jnet_sn.peers[slot].handle)jnet_sn_api.close(jnet_sn_api.self,jnet_sn.peers[slot].handle,1000,"JOURNET session ended",0);
    memset(&jnet_sn.peers[slot],0,sizeof(jnet_sn.peers[slot]));
    memset(&jnet_sn_arrivals[slot],0,sizeof(jnet_sn_arrivals[slot]));
    for(unsigned i=0;i<8;++i)JNETSteamP2P[109+slot*8+i]=0;
}
static void jnet_sn_reset(void) {
    if(jnet_travel_session_reset)jnet_travel_session_reset();
    jnet_sg_reset();
    jnet_sl_metadata_events=0;jnet_sl_metadata_cache.valid=0;InterlockedExchange(&jnet_sl_metadata_dirty,0);
    if((jnet_sn_data_registration.flags&1) && jnet_sn_unregister_fn)jnet_sn_unregister_fn(&jnet_sn_data_registration);
    jnet_sn_next_metrics=0;
    if((jnet_sn_registration.flags&1) && jnet_sn_unregister_fn)jnet_sn_unregister_fn(&jnet_sn_registration);
    if((jnet_sn_members_registration.flags&1) && jnet_sn_unregister_fn)jnet_sn_unregister_fn(&jnet_sn_members_registration);
    memset(&jnet_sn_members,0,sizeof(jnet_sn_members));InterlockedExchange(&jnet_sn_members_dirty,0);
    JNETSteamP2P[8]=0;
    for(unsigned i=0;i<7;++i)jnet_sn_drop(i);
    memset(jnet_sn_arrivals,0,sizeof(jnet_sn_arrivals));for(unsigned i=25;i<165;++i)JNETSteamP2P[i]=0;
    if(jnet_sn.listener)jnet_sn_api.close_listener(jnet_sn_api.self,jnet_sn.listener);
    jnet_sn.listener=0;jnet_sn.room=0;JNETSteamLobby[11]=JNETSteamLobby[12]=0;
}
static int jnet_sn_bind(void) {
    jnet_sn_api.self=jnet_steam_service.sockets;
    jnet_sn_timestamp=jnet_steam_method(jnet_steam_service.utils,12);
#define JNET_SN_BIND(field,slot) do {jnet_sn_api.field=jnet_steam_method(jnet_sn_api.self,slot);if(!jnet_sn_api.field)return 0;}while(0)
    JNET_SN_BIND(listen,2);JNET_SN_BIND(connect,3);JNET_SN_BIND(accept,4);JNET_SN_BIND(close,5);JNET_SN_BIND(close_listener,6);
    JNET_SN_BIND(userdata,7);JNET_SN_BIND(send,11);JNET_SN_BIND(receive,14);JNET_SN_BIND(info,15);
    jnet_sn_api.realtime=jnet_steam_method(jnet_sn_api.self,16);
#undef JNET_SN_BIND
    return 1;
}
static int jnet_sn_transmit(uint32_t handle,const void *data,uint32_t bytes,int flags) {
    uint64_t started=GetTickCount64();int result=jnet_sn_api.send(jnet_sn_api.self,handle,data,bytes,flags,NULL);jnet_sn_cost(21,started);return result;
}
static void jnet_sn_send(unsigned slot,unsigned type,uint64_t sequence) {
    unsigned char packet[72]={0};memcpy(packet,"JNS1",4);packet[4]=(unsigned char)type;
    memcpy(packet+8,&jnet_sl.room,8);memcpy(packet+16,&jnet_sl.identity,8);memcpy(packet+24,&jnet_sn.peers[slot].identity,8);
    memcpy(packet+32,jnet_sl.nonce,32);memcpy(packet+64,&sequence,8);
    int result=jnet_sn_transmit(jnet_sn.peers[slot].handle,packet,sizeof(packet),type<=2?9:1);
    if(result==1)InterlockedIncrement(&JNETSteamLobby[13]);else JNETSteamLobby[15]=-result;
}
static void jnet_sn_event(const struct jnet_sn_event *event) {
    unsigned slot=7;for(unsigned i=0;i<7;++i)if(jnet_sn.peers[i].handle==event->connection){slot=i;break;}
    if(slot==7 && event->info.listener && event->info.listener==jnet_sn.listener && event->info.state==1) {
        InterlockedIncrement(&JNETSteamP2P[1]);
        JNETSteamP2P[4]=event->info.state;JNETSteamP2P[5]=event->info.flags;JNETSteamP2P[6]=event->info.reason;
        uint64_t id=jnet_sn_id(&event->info.remote);unsigned used=0;
        for(unsigned i=0;i<7;++i)if(jnet_sn.peers[i].handle){++used;if(jnet_sn.peers[i].identity==id)used=JNET_STEAM_CAPACITY;}
        const char *rejection=!jnet_sl.room?"JOURNET lobby no longer active":
            jnet_sl.host!=jnet_sl.identity?"JOURNET endpoint is not host":
            !id?"JOURNET missing Steam identity":id==jnet_sl.identity?"JOURNET self connection":
            used==JNET_STEAM_CAPACITY?"JOURNET duplicate connection":
            used>=JNET_STEAM_CAPACITY-1?"JOURNET pending capacity full":NULL;
        if(rejection) {
            InterlockedIncrement(&JNETSteamP2P[3]);
            snprintf(JNETSteamP2PText,sizeof(JNETSteamP2PText),"%s",rejection);
            jnet_sn_api.close(jnet_sn_api.self,event->connection,1001,rejection,0);return;
        }
        for(unsigned i=0;i<7;++i)if(!jnet_sn.peers[i].handle){slot=i;break;}
        jnet_sn.peers[slot].handle=event->connection;jnet_sn.peers[slot].identity=id;jnet_sn.peers[slot].generation=jnet_sl.generation;
        jnet_sn.peers[slot].deadline=GetTickCount64()+20000;
        if(!jnet_sn_api.userdata(jnet_sn_api.self,event->connection,(int64_t)jnet_sl.generation)){JNETSteamP2P[7]=-1;jnet_sn_drop(slot);return;}
        int accepted=jnet_sn_api.accept(jnet_sn_api.self,event->connection);JNETSteamP2P[7]=accepted;
        if(accepted!=1){jnet_sn_drop(slot);return;}InterlockedIncrement(&JNETSteamP2P[2]);
    }
    if(slot==7 || jnet_sn.peers[slot].generation!=jnet_sl.generation)return;
    if(event->info.state>=3 && (uint64_t)event->info.userdata!=jnet_sn.peers[slot].generation)return;
    JNETSteamP2P[4]=event->info.state;JNETSteamP2P[5]=event->info.flags;JNETSteamP2P[6]=event->info.reason;
    if(event->info.state>=4){
        for(unsigned i=0;i<127;++i){unsigned char c=(unsigned char)event->info.debug[i];JNETSteamP2PText[i]=c>=32 && c<127?(char)c:c?' ':0;if(!c)break;}
        JNETSteamP2PText[127]=0;
    }
    if(event->info.state==3) {
        if(jnet_sn_id(&event->info.remote)!=jnet_sn.peers[slot].identity || (event->info.flags&3)){
            snprintf(JNETSteamP2PText,sizeof(JNETSteamP2PText),"%s",jnet_sn_id(&event->info.remote)!=jnet_sn.peers[slot].identity?
                "JOURNET connected identity mismatch":"JOURNET connected authentication/encryption unavailable");
            jnet_sn_drop(slot);JNETSteamLobby[15]=-100;return;
        }
        jnet_sn.peers[slot].connected=1;JNETSteamLobby[11]=2;
        if(jnet_sl.host!=jnet_sl.identity)jnet_sn_send(slot,1,0);
    } else if(event->info.state>=4) {
        JNETSteamLobby[15]=event->info.reason;jnet_sn_drop(slot);
        if(jnet_sl.room && jnet_sl.host!=jnet_sl.identity){jnet_sl_leave();jnet_sl_status("Host connection ended; left Steam lobby");}
    }
}
static void jnet_sn_receive_inner(unsigned slot) {
    struct jnet_sn_message *messages[16];int count=jnet_sn_api.receive(jnet_sn_api.self,jnet_sn.peers[slot].handle,messages,16);
    if(count<0){JNETSteamLobby[15]=-101;jnet_sn_drop(slot);return;}
    if(count==16)++JNETSteamP2P[109+slot*8+7];
    for(int i=0;i<count;++i) {
        struct jnet_sn_message *m=messages[i];
        if(m && m->connection==jnet_sn.peers[slot].handle && jnet_sn_id(&m->peer)==jnet_sn.peers[slot].identity)jnet_sn_arrival(slot,m);
        if(m && m->bytes>0 && m->data && m->connection==jnet_sn.peers[slot].handle &&
            jnet_sn_id(&m->peer)==jnet_sn.peers[slot].identity && ( (jnet_rocket_receive && jnet_rocket_receive(slot,m->data,(size_t)m->bytes)) || (jnet_chat_receive && jnet_chat_receive(slot,m->data,(size_t)m->bytes)) || (jnet_travel_receive && jnet_travel_receive(slot,m->data,(size_t)m->bytes)) || jnet_sg_receive(slot,m->data,(size_t)m->bytes))){m->release(m);continue;}
        unsigned char p[72];int valid=m && m->bytes==72 && m->data &&
            m->connection==jnet_sn.peers[slot].handle && jnet_sn_id(&m->peer)==jnet_sn.peers[slot].identity;
        if(valid)memcpy(p,m->data,sizeof(p));if(m)m->release(m);
        if(!valid){JNETSteamLobby[15]=-102;continue;}
        uint64_t room,source,destination,sequence;memcpy(&room,p+8,8);memcpy(&source,p+16,8);memcpy(&destination,p+24,8);memcpy(&sequence,p+64,8);
        if(memcmp(p,"JNS1",4) || p[5] || p[6] || p[7] || room!=jnet_sl.room || source!=jnet_sn.peers[slot].identity || destination!=jnet_sl.identity || memcmp(p+32,jnet_sl.nonce,32)) {JNETSteamLobby[15]=-103;continue;}
        unsigned type=p[4];
        if(type==1 && jnet_sl.host==jnet_sl.identity) {jnet_sn.peers[slot].hello=1;}
        else if(type==2 && source==jnet_sl.host && jnet_sn_member(source) && (jnet_sn.peers[slot].admitted || jnet_sl_valid_session(jnet_sl.room,jnet_sl.host,jnet_sl.nonce)))jnet_sn.peers[slot].admitted=1;
        else if((type==3 || type==4) && jnet_sn.peers[slot].admitted && sequence>jnet_sn.peers[slot].last_received[type-3]) {
            jnet_sn.peers[slot].last_received[type-3]=sequence;if(type==3)jnet_sn_send(slot,4,sequence);
        } else {JNETSteamLobby[15]=-104;continue;}
        InterlockedIncrement(&JNETSteamLobby[14]);if(jnet_sn.peers[slot].admitted)jnet_sn.peers[slot].deadline=GetTickCount64()+30000;
    }
}
static void jnet_sn_receive(unsigned slot) {
    uint64_t started=GetTickCount64();jnet_sn_receive_inner(slot);jnet_sn_cost(19,started);
}
static void jnet_sn_tick_inner(void) {
    if(!jnet_sl.room){if(jnet_sn.room)jnet_sn_reset();return;}
    if(jnet_sl.kind==6)return;
    if(jnet_sn.room!=jnet_sl.room || jnet_sn.generation!=jnet_sl.generation) {
        jnet_sn_reset();jnet_sn_members.active=1;jnet_sn.room=jnet_sl.room;jnet_sn.generation=jnet_sl.generation;jnet_sn.failed=0;
        if(!jnet_sn_bind()){jnet_sn.failed=1;JNETSteamLobby[11]=-1;jnet_sl_status("Steam P2P bindings unavailable");return;}
        if(!jnet_sn_register()){jnet_sn.failed=1;JNETSteamLobby[11]=-4;jnet_sl_status("Steam P2P callback registration failed");return;}
        if(jnet_sl.host==jnet_sl.identity) {
            jnet_sn.listener=jnet_sn_api.listen(jnet_sn_api.self,21746,0,NULL);
            if(!jnet_sn.listener){jnet_sn.failed=1;JNETSteamLobby[11]=-2;jnet_sl_status("Steam P2P listen failed; leave and retry");return;}
        } else {
            struct jnet_sn_identity id={.type=16,.size=8};memcpy(id.value,&jnet_sl.host,8);
            uint32_t handle=jnet_sn_api.connect(jnet_sn_api.self,&id,21746,0,NULL);
            if(!handle){jnet_sn.failed=1;JNETSteamLobby[11]=-3;jnet_sl_status("Steam P2P connect failed; leave and retry");return;}
            jnet_sn.peers[0].handle=handle;jnet_sn.peers[0].identity=jnet_sl.host;jnet_sn.peers[0].generation=jnet_sl.generation;
            jnet_sn.peers[0].deadline=GetTickCount64()+30000;
            if(!jnet_sn_api.userdata(jnet_sn_api.self,handle,(int64_t)jnet_sl.generation)){jnet_sn_drop(0);jnet_sn.failed=1;JNETSteamLobby[11]=-3;jnet_sl_status("Steam P2P generation binding failed");return;}
        }
        JNETSteamLobby[11]=1;
    }
    if(jnet_sn.failed)return;
    struct jnet_sn_event events[32];unsigned count,overflow;
    AcquireSRWLockExclusive(&jnet_sn_lock);count=jnet_sn_queue.count;overflow=jnet_sn_queue.overflow;
    memcpy(events,jnet_sn_queue.events,count*sizeof(events[0]));jnet_sn_queue.count=jnet_sn_queue.overflow=0;ReleaseSRWLockExclusive(&jnet_sn_lock);
    if(overflow){jnet_sn_reset();jnet_sl_leave();jnet_sl_status("P2P callback overflow; left lobby");return;}
    for(unsigned i=0;i<count && jnet_sl.room;++i)jnet_sn_event(&events[i]);
    if(!jnet_sl.room)return;
    unsigned admitted=0;uint64_t now=GetTickCount64();
    for(unsigned i=0;i<7;++i)if(jnet_sn.peers[i].handle) {
        if(!jnet_sn.peers[i].connected) {
            struct jnet_sn_event current={.connection=jnet_sn.peers[i].handle};
            if(jnet_sn_api.info(jnet_sn_api.self,current.connection,&current.info)){
                JNETSteamP2P[4]=current.info.state;JNETSteamP2P[5]=current.info.flags;
                if(current.info.state>=3)jnet_sn_event(&current);
            }
            if(!jnet_sl.room)return;
            if(!jnet_sn.peers[i].handle)continue;
        }
        if(jnet_sn.peers[i].connected)jnet_sn_receive(i);
        if(!jnet_sn.peers[i].handle)continue;
        if((now>=jnet_sn.peers[i].deadline && !jnet_travel_busy) || (jnet_sn.peers[i].admitted && !jnet_sn_member(jnet_sn.peers[i].identity))) {
            jnet_sn_drop(i);JNETSteamLobby[15]=-105;
            if(jnet_sl.host!=jnet_sl.identity){jnet_sl_leave();jnet_sl_status("P2P admission or heartbeat timed out");return;}continue;
        }
        if(jnet_sn.peers[i].hello && !jnet_sn.peers[i].admitted && jnet_sn_member(jnet_sn.peers[i].identity) && jnet_sl_valid_session(jnet_sl.room,jnet_sl.host,jnet_sl.nonce)) {jnet_sn.peers[i].admitted=1;jnet_sn.peers[i].deadline=now+30000;jnet_sn_send(i,2,0);}
        if(jnet_sn.peers[i].admitted){++admitted;
            if(now>=jnet_sn.peers[i].next_send){jnet_sn.peers[i].next_send=now+1000;jnet_sn_send(i,3,++jnet_sn.peers[i].sequence);}}
    }
    jnet_sg_tick();
    if(jnet_rocket_poll)jnet_rocket_poll();
    JNETSteamLobby[12]=(LONG)admitted;
    if(admitted){JNETSteamLobby[11]=3;jnet_sl_status("Steam connected - gameplay active");}
    else if(jnet_sl.host==jnet_sl.identity){JNETSteamLobby[11]=1;jnet_sl_status("Hosting Steam lobby - waiting for P2P guests");}
    else {unsigned handles=0;for(unsigned i=0;i<7;++i)handles+=!!jnet_sn.peers[i].handle;
        if(!handles){jnet_sl_leave();jnet_sl_status("Host connection failed; left Steam lobby");}}
}
static int jnet_sn_roster(uint64_t identity,unsigned *population) {
    unsigned active=jnet_sn_members.active;
    jnet_sn_members.active=1;
    if(!(jnet_sn_members_registration.flags&1))jnet_sn_members.loaded=0;
    int found=jnet_sn_member(identity);
    *population=jnet_sn_members.count;
    jnet_sn_members.active=active;
    return found;
}
static void jnet_sn_metrics(void) {
    uint64_t now=GetTickCount64();
    for(unsigned i=0;i<7;++i) {
        unsigned at=25+i*12;uint32_t handle=jnet_sn.peers[i].connected?jnet_sn.peers[i].handle:0;
        if((uint32_t)JNETSteamP2P[at+2]!=handle) {
            for(unsigned j=0;j<12;++j)JNETSteamP2P[at+j]=0;
            JNETSteamP2P[at]=(LONG)(uint32_t)jnet_sn.peers[i].identity;
            JNETSteamP2P[at+1]=(LONG)(uint32_t)(jnet_sn.peers[i].identity>>32);
            JNETSteamP2P[at+2]=(LONG)handle;JNETSteamP2P[at+4]=JNETSteamP2P[at+6]=-1;
        }
    }
    if(now<jnet_sn_next_metrics)return;jnet_sn_next_metrics=now+1000;
    for(unsigned i=0;i<7;++i) {
        unsigned at=25+i*12;if(!JNETSteamP2P[at+2])continue;
        struct jnet_sn_realtime status={0};
        int result=jnet_sn_api.realtime?jnet_sn_api.realtime(jnet_sn_api.self,(uint32_t)JNETSteamP2P[at+2],&status,0,NULL):-1;
        JNETSteamP2P[at+3]=result;
        if(result!=1){JNETSteamP2P[at+4]=JNETSteamP2P[at+6]=-1;continue;}
        JNETSteamP2P[at+4]=status.ping;
        if(status.ping>JNETSteamP2P[at+5])JNETSteamP2P[at+5]=status.ping;
        LONG queue=status.queue_us<0?-1:status.queue_us>INT32_MAX?INT32_MAX:(LONG)status.queue_us;
        JNETSteamP2P[at+6]=queue;if(queue>JNETSteamP2P[at+7])JNETSteamP2P[at+7]=queue;
        JNETSteamP2P[at+8]=status.pending_unreliable;JNETSteamP2P[at+9]=status.pending_reliable;JNETSteamP2P[at+10]=status.unacked;++JNETSteamP2P[at+11];
    }
    jnet_sn_cost(23,now);
}
static void jnet_sn_tick(void) {
    uint64_t started=GetTickCount64();
    jnet_sn_members.active=1;
    if(!(jnet_sn_members_registration.flags&1))jnet_sn_members.loaded=0;
    jnet_sn_tick_inner();
    jnet_sn_metrics();
    jnet_sn_members.active=0;
    JNETSteamP2P[11]=(LONG)(GetTickCount64()-started);
    if(JNETSteamP2P[11]>JNETSteamP2P[12])JNETSteamP2P[12]=JNETSteamP2P[11];
}
#include "jnet_steam_game.h"
#endif
