#ifndef JNET_STEAM_LOBBY_H
#define JNET_STEAM_LOBBY_H
#include "jnet_lobby_bridge.h"
#define JNET_STEAM_CAPACITY 3
#define JNET_STEAM_PROTOCOL "steam-game-19"
#define JNET_NATIVE_LEVEL "journet:steam-game-7"
__declspec(dllexport) volatile LONG JNETSteamLobby[58];
__declspec(dllexport) char JNETSteamLobbyText[128];
struct jnet_steam_created { int32_t result;uint32_t pad;uint64_t room; };
struct jnet_steam_entered { uint64_t room;uint32_t permissions;unsigned char locked,pad[3];uint32_t result,pad2; };
_Static_assert(sizeof(struct jnet_steam_created)==16,"LobbyCreated Windows ABI");
_Static_assert(sizeof(struct jnet_steam_entered)==24,"LobbyEnter Windows ABI");
static struct {
    void *mm,*utils,*friends,*legacy;
    uint64_t (*search)(void *),(*create)(void *,int,int),(*join)(void *,uint64_t);
    void (*string_filter)(void *,const char *,const char *,int);
    void (*numeric_filter)(void *,const char *,int,int),(*near_filter)(void *,const char *,int);
    void (*slots_filter)(void *,int),(*distance_filter)(void *,int),(*count_filter)(void *,int),(*compatible_filter)(void *,uint64_t);
    uint64_t (*by_index)(void *,int),(*owner)(void *,uint64_t),(*member)(void *,uint64_t,int);
    int (*members)(void *,uint64_t),(*limit)(void *,uint64_t);
    void (*leave)(void *,uint64_t);
    const char *(*data)(void *,uint64_t,const char *),*(*name)(void *,uint64_t);
    unsigned char (*set_data)(void *,uint64_t,const char *,const char *),(*joinable)(void *,uint64_t,unsigned char);
    unsigned char (*set_type)(void *,uint64_t,int);
    unsigned char (*completed)(void *,uint64_t,unsigned char *),(*result)(void *,uint64_t,void *,int,int,unsigned char *);
} jnet_sl_api;
static struct {
    unsigned bound,failed,kind,access,attempts,cancelled,page,count,chat,cmatw;
    uint64_t identity,room,host,call,deadline,next_search,next_members,generation,call_generation,wanted_room,wanted_host;
    char nonce[33],code[5],status[80];
    struct jnet_lobby_action deferred;
    struct jnet_lobby_row rows[50];char names[50][33];
} jnet_sl;
static void (*jnet_sl_transport_reset)(void);
static void (*jnet_sl_transport_poll)(void);
static int (*jnet_sl_roster)(uint64_t,unsigned *);
static int (*jnet_sl_background)(unsigned *);
static volatile LONG jnet_sl_guard,jnet_sl_metadata_dirty;
static unsigned jnet_sl_metadata_events;
static struct {uint64_t room,generation,host,expires;char nonce[33];unsigned valid;} jnet_sl_metadata_cache;
static void jnet_sl_cost(unsigned index,uint64_t started) {
    LONG cost=(LONG)(GetTickCount64()-started);JNETSteamLobby[index]=cost;
    if(cost>JNETSteamLobby[index+1])JNETSteamLobby[index+1]=cost;
}
static __declspec(thread) unsigned jnet_sl_owned;
static uint64_t (*jnet_sl_search_original)(void *);
static uint64_t (*jnet_sl_create_original)(void *,int,int);
static uint64_t (*jnet_sl_join_original)(void *,uint64_t);
static void (*jnet_sl_string_original)(void *,const char *,const char *,int);
static void (*jnet_sl_numeric_original)(void *,const char *,int,int);
static void (*jnet_sl_near_original)(void *,const char *,int);
static void (*jnet_sl_slots_original)(void *,int),(*jnet_sl_distance_original)(void *,int),(*jnet_sl_count_original)(void *,int);
static void (*jnet_sl_compatible_original)(void *,uint64_t);
static int jnet_sl_block(void *mm) {
    if(mm==jnet_sl_api.mm && jnet_sl_guard && !jnet_sl_owned){InterlockedIncrement(&JNETSteamLobby[2]);return 1;}return 0;
}
static uint64_t jnet_sl_search_hook(void *m){return jnet_sl_block(m)?0:jnet_sl_search_original(m);}
static uint64_t jnet_sl_create_hook(void *m,int t,int c){return jnet_sl_block(m)?0:jnet_sl_create_original(m,t,c);}
static uint64_t jnet_sl_join_hook(void *m,uint64_t r){return jnet_sl_block(m)?0:jnet_sl_join_original(m,r);}
static void jnet_sl_string_hook(void *m,const char *k,const char *v,int c){if(!jnet_sl_block(m))jnet_sl_string_original(m,k,v,c);}
static void jnet_sl_numeric_hook(void *m,const char *k,int v,int c){if(!jnet_sl_block(m))jnet_sl_numeric_original(m,k,v,c);}
static void jnet_sl_near_hook(void *m,const char *k,int v){if(!jnet_sl_block(m))jnet_sl_near_original(m,k,v);}
static void jnet_sl_slots_hook(void *m,int v){if(!jnet_sl_block(m))jnet_sl_slots_original(m,v);}
static void jnet_sl_distance_hook(void *m,int v){if(!jnet_sl_block(m))jnet_sl_distance_original(m,v);}
static void jnet_sl_count_hook(void *m,int v){if(!jnet_sl_block(m))jnet_sl_count_original(m,v);}
static void jnet_sl_compatible_hook(void *m,uint64_t v){if(!jnet_sl_block(m))jnet_sl_compatible_original(m,v);}
static unsigned char (*jnet_sl_type_original)(void *,uint64_t,int);
static unsigned char (*jnet_sl_joinable_original)(void *,uint64_t,unsigned char);
static unsigned char jnet_sl_type_hook(void *m,uint64_t room,int type){return jnet_sl_block(m)?0:jnet_sl_type_original(m,room,type);}
static unsigned char jnet_sl_joinable_hook(void *m,uint64_t room,unsigned char value){return jnet_sl_block(m)?0:jnet_sl_joinable_original(m,room,value);}
static unsigned char jnet_sl_type(uint64_t room,int type){++jnet_sl_owned;unsigned char ok=jnet_sl_api.set_type(jnet_sl_api.mm,room,type);--jnet_sl_owned;return ok;}
static unsigned char jnet_sl_joinable(uint64_t room,unsigned char value){++jnet_sl_owned;unsigned char ok=jnet_sl_api.joinable(jnet_sl_api.mm,room,value);--jnet_sl_owned;return ok;}
static unsigned char (*jnet_sl_send_original)(void *,uint64_t,const void *,uint32_t,int,int);
static unsigned char (*jnet_sl_available_original)(void *,uint32_t *,int);
static unsigned char (*jnet_sl_read_original)(void *,void *,uint32_t,uint32_t *,uint64_t *,int);
static unsigned char (*jnet_sl_accept_original)(void *,uint64_t);
static int jnet_sl_legacy_block(void *self) {
    if(self==jnet_sl_api.legacy && jnet_sl_guard){InterlockedIncrement(&JNETSteamLobby[2]);return 1;}return 0;
}
static unsigned char jnet_sl_send_hook(void *self,uint64_t peer,const void *data,uint32_t size,int type,int channel) {
    return jnet_sl_legacy_block(self)?0:jnet_sl_send_original(self,peer,data,size,type,channel);
}
static unsigned char jnet_sl_available_hook(void *self,uint32_t *size,int channel) {
    if(jnet_sl_legacy_block(self)){if(size)*size=0;return 0;}return jnet_sl_available_original(self,size,channel);
}
static unsigned char jnet_sl_read_hook(void *self,void *data,uint32_t capacity,uint32_t *size,uint64_t *peer,int channel) {
    if(jnet_sl_legacy_block(self)){if(size)*size=0;if(peer)*peer=0;return 0;}return jnet_sl_read_original(self,data,capacity,size,peer,channel);
}
static unsigned char jnet_sl_accept_hook(void *self,uint64_t peer) {
    return jnet_sl_legacy_block(self)?0:jnet_sl_accept_original(self,peer);
}
static void jnet_sl_status(const char *text) {
    snprintf(jnet_sl.status,sizeof(jnet_sl.status),"%s",text);
    snprintf(JNETSteamLobbyText,sizeof(JNETSteamLobbyText),"%s",text);
}
static void jnet_sl_name(char out[33],uint64_t who) {
    const char *name=jnet_sl_api.name(jnet_sl_api.friends,who);unsigned i=0;
    if(name)for(;i<32 && name[i];++i)out[i]=(unsigned char)name[i]>=32 && (unsigned char)name[i]<127?name[i]:'?';
    out[i]=0;if(!i)strcpy(out,"Steam player");
}
static int jnet_sl_equal(uint64_t room,const char *key,const char *value) {
    const char *s=jnet_sl_api.data(jnet_sl_api.mm,room,key);return s && !strcmp(s,value);
}
static int jnet_sl_hex_nonce(const char *s) {
    if(!s)return 0;
    for(unsigned i=0;i<32;++i)if(!((s[i]>='0' && s[i]<='9') || (s[i]>='a' && s[i]<='f')))return 0;
    return !s[32];
}
static uint64_t jnet_sl_decimal(const char *s) {
    uint64_t value=0;if(!s || !*s)return 0;
    for(unsigned i=0;s[i];++i){if(i>=20 || s[i]<'0' || s[i]>'9' || value>(UINT64_MAX-(unsigned)(s[i]-'0'))/10)return 0;value=value*10+(unsigned)(s[i]-'0');}
    return value;
}
static unsigned jnet_sl_lighting(uint64_t room) {
    const char *mode=jnet_sl_api.data(jnet_sl_api.mm,room,"journet.lighting");
    if(mode && mode[0]>='0' && mode[0]<='3' && !mode[1])return (unsigned)(mode[0]-'0');
    return jnet_sl_equal(room,"journet.daynight","1")?2:0;
}
static void jnet_sl_travel(const struct jnet_lobby_action *action) {
    if(action->lighting>3 || (action->lighting==3 && action->level!=2 && action->level!=3) ||
       action->paradise>1 || (action->paradise && action->level!=7))return;
    unsigned previous=jnet_paradise_cutscene,lighting=jnet_daynight_mode,elapsed=jnet_daynight_elapsed;
    jnet_paradise_cutscene=action->level==7 && action->paradise;
    jnet_daynight_mode=jnet_daynight_supported(action->level)?action->lighting:0;
    if(lighting!=2 || jnet_daynight_mode!=2)jnet_daynight_elapsed=0;
    if(!jnet_travel_request(action->level)){
        jnet_paradise_cutscene=previous;jnet_daynight_mode=lighting;jnet_daynight_elapsed=elapsed;
    }
}
static int jnet_sl_metadata_check(uint64_t room,uint64_t expected,const char *nonce) {
    if(!jnet_sl_equal(room,"level",JNET_NATIVE_LEVEL) || !jnet_sl_equal(room,"journet.mod","journet"))return 1;
    if(!jnet_sl_equal(room,"journet.protocol",JNET_STEAM_PROTOCOL))return 2;
    if(!jnet_sl_equal(room,"journet.build",JC_BUILD_ID_HEX))return 9;
    if(!jnet_sl_equal(room,"journet.capacity","3") || jnet_sl_api.limit(jnet_sl_api.mm,room)!=JNET_STEAM_CAPACITY)return 3;
    if(!jnet_sl_equal(room,"journet.status","ready"))return 4;
    if(!jnet_sl_equal(room,"journet.mode","journey") || !jnet_sl_equal(room,"journet.level","All"))return 5;
    if(!jnet_sl_equal(room,"journet.visibility","public") && !jnet_sl_equal(room,"journet.visibility","code_only"))return 6;
    uint64_t advertised=jnet_sl_decimal(jnet_sl_api.data(jnet_sl_api.mm,room,"journet.host"));
    if(!advertised || (expected && advertised!=expected))return 7;
    const char *actual_nonce=jnet_sl_api.data(jnet_sl_api.mm,room,"journet.nonce");
    if(!jnet_sl_hex_nonce(actual_nonce) || (nonce && strcmp(actual_nonce,nonce)))return 8;
    return 0;
}
static int jnet_sl_metadata_result(uint64_t room,uint64_t expected) {
    return jnet_sl_metadata_check(room,expected,NULL);
}
static int jnet_sl_valid_session(uint64_t room,uint64_t expected,const char *nonce) {
    uint64_t started=GetTickCount64();int result=jnet_sl_metadata_check(room,expected,nonce);
    jnet_sl_cost(22,started);++JNETSteamLobby[26];if(result)return 0;
    started=GetTickCount64();uint64_t host=jnet_sl_api.owner(jnet_sl_api.mm,room);jnet_sl_cost(24,started);
    int valid=host && host==(expected?expected:jnet_sl_decimal(jnet_sl_api.data(jnet_sl_api.mm,room,"journet.host")));
    if(valid && room==jnet_sl.room && host!=jnet_sl.identity)
        jnet_daynight_host=jnet_sl_lighting(room);
    return valid;
}
static int jnet_sl_valid(uint64_t room,uint64_t expected) {
    return jnet_sl_valid_session(room,expected,NULL);
}
static int jnet_sl_maintain_session(void) {
    uint64_t now=GetTickCount64();
    if(InterlockedExchange(&jnet_sl_metadata_dirty,0))jnet_sl_metadata_cache.valid=0;
    int cached=jnet_sl_metadata_events && jnet_sl_metadata_cache.valid && now<jnet_sl_metadata_cache.expires &&
        jnet_sl_metadata_cache.room==jnet_sl.room && jnet_sl_metadata_cache.generation==jnet_sl.generation &&
        jnet_sl_metadata_cache.host==jnet_sl.host && !strcmp(jnet_sl_metadata_cache.nonce,jnet_sl.nonce);
    if(!cached) {
        jnet_sl_metadata_cache.valid=0;
        if(!jnet_sl_valid_session(jnet_sl.room,jnet_sl.host,jnet_sl.nonce))return 0;
        jnet_sl_metadata_cache.room=jnet_sl.room;jnet_sl_metadata_cache.generation=jnet_sl.generation;
        jnet_sl_metadata_cache.host=jnet_sl.host;memcpy(jnet_sl_metadata_cache.nonce,jnet_sl.nonce,33);
        jnet_sl_metadata_cache.expires=now+2000;jnet_sl_metadata_cache.valid=1;return 1;
    }
    ++JNETSteamLobby[27];
    uint64_t started=GetTickCount64();uint64_t owner=jnet_sl_api.owner(jnet_sl_api.mm,jnet_sl.room);
    jnet_sl_cost(24,started);return owner && owner==jnet_sl.host;
}
static int jnet_sl_random(void) {
    unsigned char bytes[20];static const char alphabet[]="ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    if(BCryptGenRandom(NULL,bytes,sizeof(bytes),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0)return 0;
    for(unsigned i=0;i<16;++i)snprintf(jnet_sl.nonce+i*2,3,"%02x",bytes[i]);
    for(unsigned i=0;i<4;++i)jnet_sl.code[i]=alphabet[bytes[16+i]&31];jnet_sl.code[4]=0;return 1;
}
static int jnet_sl_code(const char *source,char out[5]) {
    for(unsigned i=0;i<4;++i){char c=source[i];if(c>='a' && c<='z')c-=32;if(!((c>='A' && c<='Z') || (c>='0' && c<='9')))return 0;out[i]=c;}
    out[4]=0;return source[4]==0;
}
static void jnet_sl_leave(void) {
    jnet_daynight_host=~0u;
    if(jnet_sl_transport_reset)jnet_sl_transport_reset();
    if(jnet_sl.room){if(jnet_sl.host==jnet_sl.identity)jnet_sl_joinable(jnet_sl.room,0);jnet_sl_api.leave(jnet_sl_api.mm,jnet_sl.room);}
    jnet_sl.room=jnet_sl.host=0;jnet_sl.nonce[0]=jnet_sl.code[0]=0;JNETSteamLobby[6]=0;
}
static void jnet_sl_begin(unsigned kind,uint64_t call) {
    jnet_sl.kind=kind;jnet_sl.call=call;jnet_sl.call_generation=jnet_sl.generation;jnet_sl.deadline=GetTickCount64()+30000;jnet_sl.cancelled=0;
    JNETSteamLobby[3]=(LONG)kind;JNETSteamLobby[4]=0;
    if(!call){jnet_sl.kind=0;JNETSteamLobby[4]=-1;jnet_sl_status("Steam rejected the request");}
}
static void jnet_sl_search(unsigned kind) {
    ++jnet_sl_owned;
    jnet_sl_api.string_filter(jnet_sl_api.mm,"journet.mod","journet",0);
    jnet_sl_api.distance_filter(jnet_sl_api.mm,3);
    jnet_sl_api.count_filter(jnet_sl_api.mm,50);
    if(kind==1){
        jnet_sl_api.string_filter(jnet_sl_api.mm,"journet.protocol",JNET_STEAM_PROTOCOL,0);
        jnet_sl_api.string_filter(jnet_sl_api.mm,"journet.visibility","public",0);
        jnet_sl_api.string_filter(jnet_sl_api.mm,"journet.status","ready",0);
    } else jnet_sl_api.string_filter(jnet_sl_api.mm,"journet.code",jnet_sl.code,0);
    uint64_t call=jnet_sl_api.search(jnet_sl_api.mm);--jnet_sl_owned;jnet_sl_begin(kind,call);
    jnet_sl.next_search=UINT64_MAX;
}
static void jnet_sl_create(void) {
    ++jnet_sl_owned;uint64_t call=jnet_sl_api.create(jnet_sl_api.mm,0,JNET_STEAM_CAPACITY);--jnet_sl_owned;
    jnet_sl_begin(4,call);jnet_sl_status("Creating Steam lobby");
}
static void jnet_sl_join(uint64_t room,uint64_t host) {
    if(!room || !host || jnet_sl_metadata_result(room,host)){jnet_sl_status("Selected lobby is not a compatible Journet session");return;}
    jnet_sl.wanted_room=room;jnet_sl.wanted_host=host;
    ++jnet_sl_owned;uint64_t call=jnet_sl_api.join(jnet_sl_api.mm,room);--jnet_sl_owned;
    jnet_sl_begin(5,call);jnet_sl_status("Joining Steam lobby");
}
static int jnet_sl_publish(void) {
    char host[24];snprintf(host,sizeof(host),"%llu",(unsigned long long)jnet_sl.identity);
    if(!jnet_sl_joinable(jnet_sl.room,0) || !jnet_sl_type(jnet_sl.room,0))return 0;
    const char *keys[]={"level","journet.mod","journet.protocol","journet.visibility","journet.code","journet.host","journet.nonce","journet.capacity","journet.mode","journet.level","journet.status","journet.chat","journet.cmatw","journet.build"};
    const char *values[]={JNET_NATIVE_LEVEL,"journet",JNET_STEAM_PROTOCOL,jnet_sl.access?"code_only":"public",jnet_sl.access?jnet_sl.code:"",host,jnet_sl.nonce,"3","journey","All",jnet_sl.access?"checking":"ready",jnet_sl.chat?"1":"0",jnet_sl.cmatw?"1":"0",JC_BUILD_ID_HEX};
    for(unsigned i=0;i<sizeof(keys)/sizeof(keys[0]);++i)if(!jnet_sl_api.set_data(jnet_sl_api.mm,jnet_sl.room,keys[i],values[i]))return 0;
    return jnet_sl_type(jnet_sl.room,3) && jnet_sl_joinable(jnet_sl.room,1);
}
static void jnet_sl_poll_result(void) {
    if(!jnet_sl.call)return;
    unsigned char failed=0;
    if(!jnet_sl_api.completed(jnet_sl_api.utils,jnet_sl.call,&failed)) {
        if(GetTickCount64()>=jnet_sl.deadline && !jnet_sl.cancelled){jnet_sl.cancelled=1;++jnet_sl.generation;JNETSteamLobby[4]=-2;jnet_sl_status("Steam timed out; draining the pending request");}
        return;
    }
    unsigned kind=jnet_sl.kind;uint64_t generation=jnet_sl.call_generation;
    union { struct jnet_steam_created created;struct jnet_steam_entered entered;uint32_t matches; } result={0};
    int size=kind==4?16:kind==5?24:4,callback=kind==4?513:kind==5?504:510;
    int ok=!failed && jnet_sl_api.result(jnet_sl_api.utils,jnet_sl.call,&result,size,callback,&failed) && !failed;
    jnet_sl.call=0;jnet_sl.kind=0;JNETSteamLobby[3]=0;
    if(generation!=jnet_sl.generation) {
        if(ok && kind==4 && result.created.result==1)jnet_sl_api.leave(jnet_sl_api.mm,result.created.room);
        if(ok && kind==5 && result.entered.result==1)jnet_sl_api.leave(jnet_sl_api.mm,result.entered.room);
        jnet_sl.cancelled=0;return;
    }
    if(!ok){JNETSteamLobby[4]=-3;jnet_sl_status("Steam request failed");return;}
    JNETSteamLobby[4]=kind==4?result.created.result:kind==5?(LONG)result.entered.result:1;
    if(kind==4) {
        if(result.created.result!=1 || !result.created.room){jnet_sl_status("Steam could not create the lobby");return;}
        jnet_sl.room=result.created.room;jnet_sl.host=jnet_sl.identity;
        if(!jnet_sl_publish()){jnet_sl_leave();jnet_sl_status("Lobby metadata could not be published");return;}
        if(jnet_sl.access)jnet_sl_search(6);else jnet_sl_status("Hosting Steam lobby - gameplay not yet connected");
    } else if(kind==5) {
        if(result.entered.result!=1){jnet_sl_status("Steam refused lobby entry; see report");return;}
        if(result.entered.room!=jnet_sl.wanted_room || !jnet_sl_valid(result.entered.room,jnet_sl.wanted_host)) {
            jnet_sl_api.leave(jnet_sl_api.mm,result.entered.room);jnet_sl_status("Lobby changed or is incompatible");return;
        }
        jnet_sl.room=result.entered.room;jnet_sl.host=jnet_sl_api.owner(jnet_sl_api.mm,jnet_sl.room);
        snprintf(jnet_sl.nonce,sizeof(jnet_sl.nonce),"%s",jnet_sl_api.data(jnet_sl_api.mm,jnet_sl.room,"journet.nonce"));
        jnet_sl.access=jnet_sl_equal(jnet_sl.room,"journet.visibility","code_only");
        jnet_sl.cmatw=!jnet_sl_equal(jnet_sl.room,"journet.cmatw","0");
        jnet_sl.chat=(jnet_sl.access || jnet_sl.cmatw) && jnet_sl_equal(jnet_sl.room,"journet.chat","1");
        if(jnet_sl.access)snprintf(jnet_sl.code,sizeof(jnet_sl.code),"%.4s",jnet_sl_api.data(jnet_sl_api.mm,jnet_sl.room,"journet.code"));
        else jnet_sl.code[0]=0;
        jnet_sl_status("Steam lobby joined - gameplay not yet connected");
    } else {
        unsigned count=result.matches>50?50:result.matches;
        if(kind==1) {
            jnet_sl.count=0;InterlockedIncrement(&JNETSteamLobby[16]);JNETSteamLobby[17]=(LONG)result.matches;JNETSteamLobby[18]=JNETSteamLobby[19]=0;
            for(unsigned i=0;i<count;++i){uint64_t room=jnet_sl_api.by_index(jnet_sl_api.mm,(int)i);
                int reason=jnet_sl_metadata_result(room,0);if(!reason && !jnet_sl_equal(room,"journet.visibility","public"))reason=6;
                if(reason){
                    if(reason==2 || reason==9){char detail[256];
                        const char *key=reason==2?"journet.protocol":"journet.build";
                        snprintf(detail,sizeof(detail),"[JNET] lobby %llu rejected: %s expected=%.64s received=%.64s",
                            (unsigned long long)room,key,reason==2?JNET_STEAM_PROTOCOL:JC_BUILD_ID_HEX,jnet_sl_api.data(jnet_sl_api.mm,room,key));
                        message(detail);
                    }
                    ++JNETSteamLobby[18];if(!JNETSteamLobby[19])JNETSteamLobby[19]=reason;continue;}
                unsigned at=jnet_sl.count++;jnet_sl.rows[at]=(struct jnet_lobby_row){room,jnet_sl_decimal(jnet_sl_api.data(jnet_sl_api.mm,room,"journet.host")),(unsigned)jnet_sl_api.members(jnet_sl_api.mm,room),0};
                if(jnet_sl_equal(room,"journet.cmatw","0"))snprintf(jnet_sl.names[at],33,"Random's Lobby");
                else jnet_sl_name(jnet_sl.names[at],jnet_sl.rows[at].host);
            }
            if(jnet_sl.page*3>=jnet_sl.count)jnet_sl.page=0;
            if(!jnet_sl.room){
                if(!jnet_sl.count && JNETSteamLobby[19]==9)jnet_sl_status("Lobby build mismatch - use the same JNET package");
                else if(!jnet_sl.count && JNETSteamLobby[19]==2)jnet_sl_status("Lobby protocol mismatch - update both players");
                else jnet_sl_status("Steam public lobby browser ready");
            }
        } else if(kind==2 || kind==6) {
            unsigned collisions=0;
            for(unsigned i=0;i<count;++i)if(jnet_sl_api.by_index(jnet_sl_api.mm,(int)i)!=jnet_sl.room)++collisions;
            if(collisions || result.matches>=50) {
                if(jnet_sl.room)jnet_sl_joinable(jnet_sl.room,0);
                if(++jnet_sl.attempts>=8 || !jnet_sl_random()){jnet_sl_leave();jnet_sl_status("Code collision; please create a new lobby");return;}
                if(jnet_sl.room){if(!jnet_sl_publish()){jnet_sl_leave();jnet_sl_status("Code publication failed");return;}jnet_sl_search(6);}else jnet_sl_search(2);
            } else if(kind==2)jnet_sl_create();else {
                if(!jnet_sl_api.set_data(jnet_sl_api.mm,jnet_sl.room,"journet.status","ready")){jnet_sl_leave();jnet_sl_status("Code lobby publication failed");return;}
                jnet_sl_status("Hosting code lobby - gameplay not yet connected");
            }
        } else if(kind==3) {
            if(!count){jnet_sl_status("No joinable lobby found for this code");return;}
            if(result.matches!=1){jnet_sl_status("Ambiguous code; ask host to create a new lobby");return;}
            uint64_t room=jnet_sl_api.by_index(jnet_sl_api.mm,0);
            if(jnet_sl_metadata_result(room,0) || !jnet_sl_equal(room,"journet.visibility","code_only") || !jnet_sl_equal(room,"journet.code",jnet_sl.code)){jnet_sl_status("Code lobby is incompatible");return;}
            jnet_sl_join(room,jnet_sl_decimal(jnet_sl_api.data(jnet_sl_api.mm,room,"journet.host")));
        }
    }
}
static int jnet_sl_bind(void) {
    HMODULE module=GetModuleHandleW(L"steam_api64.dll");
    void *(*client_fn)(void)=(void *)GetProcAddress(module,"SteamClient");
    void *(*get_mm)(void *,int,int,const char *)=(void *)GetProcAddress(module,"SteamAPI_ISteamClient_GetISteamMatchmaking");
    void *(*get_friends)(void *,int,int,const char *)=(void *)GetProcAddress(module,"SteamAPI_ISteamClient_GetISteamFriends");
    void *(*get_utils)(void *,int,const char *)=(void *)GetProcAddress(module,"SteamAPI_ISteamClient_GetISteamUtils");
    void *(*get_legacy)(void *,int,int,const char *)=(void *)GetProcAddress(module,"SteamAPI_ISteamClient_GetISteamNetworking");
    if(!client_fn || !get_mm || !get_friends || !get_utils || !get_legacy)return 0;
    void *client=client_fn();int user=jnet_steam_service.user,pipe=jnet_steam_service.pipe;
    jnet_sl_api.mm=get_mm(client,user,pipe,"SteamMatchMaking009");jnet_sl_api.friends=get_friends(client,user,pipe,"SteamFriends015");
    jnet_sl_api.legacy=get_legacy(client,user,pipe,"SteamNetworking005");
    jnet_sl_api.utils=get_utils(client,pipe,"SteamUtils008");if(!jnet_sl_api.mm || !jnet_sl_api.friends || !jnet_sl_api.utils || !jnet_sl_api.legacy)return 0;
#define JNET_SL_BIND(field,iface,name) do {jnet_sl_api.field=(void *)GetProcAddress(module,"SteamAPI_ISteam" iface "_" name);if(!jnet_sl_api.field){jnet_sl_status("Steam binding unavailable: " name);return 0;}}while(0)
    JNET_SL_BIND(search,"Matchmaking","RequestLobbyList");JNET_SL_BIND(create,"Matchmaking","CreateLobby");JNET_SL_BIND(join,"Matchmaking","JoinLobby");
    JNET_SL_BIND(string_filter,"Matchmaking","AddRequestLobbyListStringFilter");JNET_SL_BIND(numeric_filter,"Matchmaking","AddRequestLobbyListNumericalFilter");
    JNET_SL_BIND(near_filter,"Matchmaking","AddRequestLobbyListNearValueFilter");JNET_SL_BIND(slots_filter,"Matchmaking","AddRequestLobbyListFilterSlotsAvailable");
    JNET_SL_BIND(distance_filter,"Matchmaking","AddRequestLobbyListDistanceFilter");JNET_SL_BIND(count_filter,"Matchmaking","AddRequestLobbyListResultCountFilter");
    JNET_SL_BIND(compatible_filter,"Matchmaking","AddRequestLobbyListCompatibleMembersFilter");JNET_SL_BIND(by_index,"Matchmaking","GetLobbyByIndex");
    JNET_SL_BIND(owner,"Matchmaking","GetLobbyOwner");JNET_SL_BIND(member,"Matchmaking","GetLobbyMemberByIndex");
    JNET_SL_BIND(members,"Matchmaking","GetNumLobbyMembers");JNET_SL_BIND(limit,"Matchmaking","GetLobbyMemberLimit");JNET_SL_BIND(leave,"Matchmaking","LeaveLobby");
    JNET_SL_BIND(data,"Matchmaking","GetLobbyData");JNET_SL_BIND(set_data,"Matchmaking","SetLobbyData");JNET_SL_BIND(joinable,"Matchmaking","SetLobbyJoinable");
    JNET_SL_BIND(set_type,"Matchmaking","SetLobbyType");
    JNET_SL_BIND(name,"Friends","GetFriendPersonaName");JNET_SL_BIND(completed,"Utils","IsAPICallCompleted");JNET_SL_BIND(result,"Utils","GetAPICallResult");
#undef JNET_SL_BIND
    struct {unsigned slot;void *hook;void **original;} hooks[]={
        {4,(void *)jnet_sl_search_hook,(void **)&jnet_sl_search_original},{5,(void *)jnet_sl_string_hook,(void **)&jnet_sl_string_original},
        {6,(void *)jnet_sl_numeric_hook,(void **)&jnet_sl_numeric_original},{7,(void *)jnet_sl_near_hook,(void **)&jnet_sl_near_original},
        {8,(void *)jnet_sl_slots_hook,(void **)&jnet_sl_slots_original},{9,(void *)jnet_sl_distance_hook,(void **)&jnet_sl_distance_original},
        {10,(void *)jnet_sl_count_hook,(void **)&jnet_sl_count_original},{11,(void *)jnet_sl_compatible_hook,(void **)&jnet_sl_compatible_original},
        {13,(void *)jnet_sl_create_hook,(void **)&jnet_sl_create_original},{14,(void *)jnet_sl_join_hook,(void **)&jnet_sl_join_original},
        {33,(void *)jnet_sl_type_hook,(void **)&jnet_sl_type_original},{34,(void *)jnet_sl_joinable_hook,(void **)&jnet_sl_joinable_original},
        {0,(void *)jnet_sl_send_hook,(void **)&jnet_sl_send_original},{1,(void *)jnet_sl_available_hook,(void **)&jnet_sl_available_original},
        {2,(void *)jnet_sl_read_hook,(void **)&jnet_sl_read_original},{3,(void *)jnet_sl_accept_hook,(void **)&jnet_sl_accept_original}};
    void *targets[16]={0};unsigned created=0;
    for(unsigned i=0;i<16;++i){targets[i]=jnet_steam_method(i<12?jnet_sl_api.mm:jnet_sl_api.legacy,hooks[i].slot);
        if(!targets[i] || MH_CreateHook(targets[i],hooks[i].hook,hooks[i].original)!=MH_OK){JNETSteamLobby[4]=-100-(LONG)hooks[i].slot;goto failed;}++created;}
    for(unsigned i=0;i<16;++i)if(MH_EnableHook(targets[i])!=MH_OK){JNETSteamLobby[4]=-200-(LONG)hooks[i].slot;goto failed;}
    jnet_sl_guard=1;JNETSteamLobby[1]=1;return 1;
failed:
    jnet_sl_status("Native matchmaking isolation failed; see report");
    for(unsigned i=0;i<created;++i){MH_DisableHook(targets[i]);MH_RemoveHook(targets[i]);}return 0;
}
static void jnet_sl_tick_inner(void) {
    struct jnet_lobby_action action;
    AcquireSRWLockExclusive(&jnet_lobby_lock);action=jnet_lobby_action;memset(&jnet_lobby_action,0,sizeof(jnet_lobby_action));ReleaseSRWLockExclusive(&jnet_lobby_lock);
    uint64_t now=GetTickCount64();
    if(JNETSteam[0]!=5) {
        if(jnet_sl.bound && (jnet_sl.room || jnet_sl.call)){++jnet_sl.generation;jnet_sl_leave();jnet_sl.cancelled=1;jnet_sl_poll_result();}
        if(action.kind==6 && !jnet_sl.room && jnet_travel_request)jnet_sl_travel(&action);
        else if(action.kind==1 || action.kind==2)jnet_sl_status("Cannot create or join: Steam is offline");
        else if(!jnet_sl.status[0])jnet_sl_status("Steam is offline");
        goto publish;
    }
    if(action.kind==6 && !jnet_sl.room && jnet_travel_request){jnet_sl_travel(&action);action.kind=0;}
    if(!jnet_sl.bound && !jnet_sl.failed){jnet_sl.bound=jnet_sl_bind();jnet_sl.failed=!jnet_sl.bound;jnet_sl.identity=jnet_steam_service.identity;}
    if(!jnet_sl.bound){if(!jnet_sl.status[0])jnet_sl_status("Steam lobby bindings unavailable; see report");goto publish;}
    if(jnet_sl.identity!=jnet_steam_service.identity){++jnet_sl.generation;jnet_sl_leave();jnet_sl_status("Steam identity changed; restart Journey");goto publish;}
    if((action.kind==1 || action.kind==2) && jnet_sl.call && jnet_sl.kind==1){jnet_sl.deferred=action;action.kind=0;}
    if(action.kind==3){memset(&jnet_sl.deferred,0,sizeof(jnet_sl.deferred));++jnet_sl.generation;jnet_sl_leave();jnet_sl.cancelled=!!jnet_sl.call;jnet_sl_status("Left Steam lobby");}
    jnet_sl_poll_result();
    if(!action.kind && !jnet_sl.call && jnet_sl.deferred.kind){action=jnet_sl.deferred;memset(&jnet_sl.deferred,0,sizeof(jnet_sl.deferred));}
    if(action.kind==6 && jnet_travel_request)jnet_sl_travel(&action);
    if(action.kind==5 && !jnet_sl.call){jnet_sl_search(1);jnet_sl_status("Refreshing public lobbies");}
    else if(action.kind==4){unsigned pages=(jnet_sl.count+2)/3;jnet_sl.page=pages?(jnet_sl.page+1)%pages:0;}
    else if(action.kind==1 || action.kind==2) {
        if(jnet_sl.call || jnet_sl.room)jnet_sl_status("Leave or cancel the current operation first");
        else if(action.kind==1 && !JNETSteamLobby[9])jnet_sl_status("Finish loading a level before creating a lobby");
        else if(JNETSteamLobby[10])jnet_sl_status("Native companion present or unreadable; restart Journey");
        else if(JNETSteam[13]!=1)jnet_sl_status("Steam relay and authentication are not ready");
        else if(action.kind==1) {
            if(action.mode)jnet_sl_status("Rocket Deathmatch is not implemented");
            else if(!jnet_sl_random())jnet_sl_status("Secure session initialization failed");
            else {++jnet_sl.generation;jnet_sl.access=!!action.access;jnet_sl.cmatw=!!action.cmatw;jnet_sl.chat=!!action.chat && (jnet_sl.access || jnet_sl.cmatw);jnet_sl.attempts=0;if(jnet_sl.access){jnet_sl_search(2);jnet_sl_status("Checking generated code");}else jnet_sl_create();}
        } else if(action.password[0]) {
            if(!jnet_sl_code(action.password,jnet_sl.code))jnet_sl_status("Enter a four-character lobby code");
            else {++jnet_sl.generation;jnet_sl_search(3);jnet_sl_status("Looking up lobby code");}
        } else if(action.room) {++jnet_sl.generation;jnet_sl_join(action.room,action.host);}
        else jnet_sl_status("Select a public lobby or enter a code");
    }
    unsigned background_population=0;
    int background=jnet_sl.room && jnet_sl.kind!=6 && jnet_sl_background?jnet_sl_background(&background_population):0;
    if(jnet_sl.room && jnet_sl.kind!=6 && (background<0 || now>=jnet_sl.next_members || jnet_sl_metadata_dirty)) {
        jnet_sl.next_members=now+500;
        unsigned population=0;int found=0;
        if(background){population=background_population;found=background>0;}
        else if(jnet_sl_roster)found=jnet_sl_roster(jnet_sl.identity,&population);
        else {
            population=(unsigned)jnet_sl_api.members(jnet_sl_api.mm,jnet_sl.room);
            if(population<=JNET_STEAM_CAPACITY)for(unsigned i=0;i<population;++i)found|=jnet_sl_api.member(jnet_sl_api.mm,jnet_sl.room,(int)i)==jnet_sl.identity;
        }
        if(JNETSteamLobby[10]>0){++jnet_sl.generation;jnet_sl_leave();jnet_sl_status("Native companion ownership changed; lobby ended");}
        else if(!found || population>JNET_STEAM_CAPACITY || (background<=0 && !jnet_sl_maintain_session())) {++jnet_sl.generation;jnet_sl_leave();jnet_sl_status("Host left or session changed; lobby ended");}
        else JNETSteamLobby[6]=(LONG)population;
    }
    if(!jnet_sl.call && !action.kind && now>=jnet_sl.next_search)jnet_sl_search(1);
publish:
    {
        static uint64_t published_room;static unsigned published_daynight=~0u;
        if(!jnet_sl.room){jnet_daynight_host=~0u;published_room=0;published_daynight=~0u;}
        else if(jnet_sl.host==jnet_sl.identity){
            unsigned wanted=jnet_daynight_mode;jnet_daynight_host=~0u;
            if((published_room!=jnet_sl.room || published_daynight!=wanted) &&
               jnet_sl_api.set_data(jnet_sl_api.mm,jnet_sl.room,"journet.lighting",wanted==3?"3":wanted==2?"2":wanted==1?"1":"0") &&
               jnet_sl_api.set_data(jnet_sl_api.mm,jnet_sl.room,"journet.daynight",wanted==2?"1":"0")){
                published_room=jnet_sl.room;published_daynight=wanted;
            }
        }
    }
    if(jnet_sl_transport_poll)jnet_sl_transport_poll();
    if(jnet_travel_poll)jnet_travel_poll();
    {
        struct jnet_lobby_view view={.online=1,.ready=jnet_sl.bound,.room=jnet_sl.room,.busy=jnet_sl.call && jnet_sl.kind!=1,.total=jnet_sl.count,.page=jnet_sl.page,.population=(unsigned)JNETSteamLobby[6]};
        view.self=jnet_sl.identity;view.host=jnet_sl.host;view.level=jnet_current_level;
        if(jnet_member_view)jnet_member_view(&view);
        view.chat=jnet_sl.chat;view.cmatw=jnet_sl.cmatw;view.anonymous=!jnet_sl.access && !jnet_sl.cmatw;
        if(jnet_chat_poll)jnet_chat_poll(&view);
        snprintf(view.status,sizeof(view.status),"%s",jnet_sl.status);
        if(jnet_sl.room && jnet_sl.access && jnet_sl.kind!=6)memcpy(view.code,jnet_sl.code,5);
        for(unsigned i=jnet_sl.page*3;i<jnet_sl.count && view.count<3;++i){view.rows[view.count]=jnet_sl.rows[i];memcpy(view.names[view.count],jnet_sl.names[i],33);++view.count;}
        AcquireSRWLockExclusive(&jnet_lobby_lock);jnet_lobby_view=view;ReleaseSRWLockExclusive(&jnet_lobby_lock);
        JNETSteamLobby[0]=jnet_sl.failed?-1:jnet_sl.room?3:jnet_sl.bound?2:1;JNETSteamLobby[5]=(LONG)jnet_sl.count;
        JNETSteamLobby[7]=(LONG)jnet_sl.generation;JNETSteamLobby[8]=(LONG)jnet_sl.cancelled;
    }
}
static void jnet_sl_record_poll(unsigned bucket,LONG cost) {
    unsigned at=29+bucket*7;JNETSteamLobby[at]=cost;
    if(cost>JNETSteamLobby[at+1])JNETSteamLobby[at+1]=cost;
    ++JNETSteamLobby[at+2];++JNETSteamLobby[at+3+(cost<50?0:cost<100?1:cost<250?2:3)];
}
static void jnet_sl_tick(void) {
    uint64_t started=GetTickCount64(),room=jnet_sl.room,generation=jnet_sl.generation;
    int active=room && JNETSteamLobby[11]==3 && JNETSteamLobby[12]>0;
    int busy=jnet_sl.call!=0;
    jnet_sl_tick_inner();
    JNETSteamLobby[20]=(LONG)(GetTickCount64()-started);
    if(JNETSteamLobby[20]>JNETSteamLobby[21])JNETSteamLobby[21]=JNETSteamLobby[20];
    int steady=active && room==jnet_sl.room && generation==jnet_sl.generation && JNETSteamLobby[11]==3 && JNETSteamLobby[12]>0 && !busy && !jnet_sl.call;
    jnet_sl_record_poll(steady?0:room || jnet_sl.room || busy || jnet_sl.call?2:1,JNETSteamLobby[20]);
}
#endif
