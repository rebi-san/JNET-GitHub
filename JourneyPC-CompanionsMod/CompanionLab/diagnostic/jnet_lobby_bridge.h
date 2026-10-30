#ifndef JNET_LOBBY_BRIDGE_H
#define JNET_LOBBY_BRIDGE_H
#include <stdatomic.h>
struct jnet_lobby_row { uint64_t room,host; unsigned players,access; };
struct jnet_lobby_view {
    struct jnet_lobby_row rows[3];unsigned count,identity,ready;
    uint64_t room,self,host,members[3];unsigned member_count,level;char status[80];
    unsigned online,busy,searching,total,page,population;char names[3][33],code[5];
    char member_names[3][33],host_name[33];unsigned chat,cmatw,anonymous;
};
struct jnet_lobby_action { unsigned kind,access,mode,level,chat,cmatw,lighting,paradise;uint64_t host,room;char password[33]; };
static _Atomic unsigned jnet_windwalls=1,jnet_sliding=1,jnet_freezing=1;
static _Atomic unsigned jnet_fog __attribute__((used))=1;
static _Atomic unsigned jnet_blackness=1;
static _Atomic unsigned jnet_options_failed;
static _Atomic unsigned jnet_credits=1;
static _Atomic unsigned jnet_paradise_cutscene;
static _Atomic unsigned jnet_daynight_mode,jnet_daynight_host=~0u;
static _Atomic unsigned jnet_daynight_elapsed;
static _Atomic uint64_t jnet_daynight_sync;
static inline unsigned jnet_daynight_variants(unsigned level){return level==2 || level==3 || level==7?4:3;}
static inline const char *jnet_daynight_label(unsigned level,unsigned variant){
    static const char *labels[]={"Day","Night","Cycle"};
    return variant<3?labels[variant]:level==2?"Dusk":level==3?"Dawn":level==7?"Cutscene":"Day";
}
static inline int jnet_daynight_supported(unsigned level){return level<8;}
static inline void jnet_daynight_enter(unsigned level){
    unsigned host=jnet_daynight_host;
    if(host<=3 && jnet_daynight_supported(level))jnet_daynight_mode=(host==3 && level!=2 && level!=3)?0:host;
}
static _Atomic unsigned jnet_character_spotlight;
static int (*jnet_character_capture_rotation)(float *);
struct jnet_game_record;
static void (*jnet_character_capture_war)(struct jnet_game_record *);
static void (*jnet_character_capture_ride)(struct jnet_game_record *);
static int (*jnet_character_resolve_ride)(const struct jnet_game_record *,float *);
static _Atomic unsigned jnet_current_level=2;
static _Atomic unsigned jnet_mepal_play;
static _Atomic unsigned jnet_save_requested;
static _Atomic uint64_t jnet_travel_epoch;
static _Atomic unsigned jnet_travel_busy,jnet_travel_target;
static int (*jnet_travel_request)(unsigned);
static int (*jnet_travel_receive)(unsigned,const unsigned char *,size_t);
static void (*jnet_travel_poll)(void);
static int (*jnet_travel_peer_wait)(uint64_t,uint64_t);
static void (*jnet_travel_session_reset)(void);
static int (*jnet_rocket_receive)(unsigned,const unsigned char *,size_t);
static void (*jnet_rocket_poll)(void);
static int (*jnet_chat_receive)(unsigned,const unsigned char *,size_t);
static void (*jnet_chat_poll)(struct jnet_lobby_view *);
static void (*jnet_member_view)(struct jnet_lobby_view *);
static struct jnet_lobby_view jnet_lobby_view;
static struct jnet_lobby_action jnet_lobby_action;
#ifdef _WIN32
static SRWLOCK jnet_lobby_lock=SRWLOCK_INIT;
#endif
static inline void jnet_lobby_snapshot(struct jnet_lobby_view *view) {
#ifdef _WIN32
    AcquireSRWLockShared(&jnet_lobby_lock);
#endif
    *view=jnet_lobby_view;
#ifdef _WIN32
    ReleaseSRWLockShared(&jnet_lobby_lock);
#endif
}
static inline int jnet_lobby_submit(const struct jnet_lobby_action *action) {
#ifdef _WIN32
    AcquireSRWLockExclusive(&jnet_lobby_lock);
#endif
    int ok=!jnet_lobby_action.kind;if(ok)jnet_lobby_action=*action;
#ifdef _WIN32
    ReleaseSRWLockExclusive(&jnet_lobby_lock);
#endif
    return ok;
}
static inline const char *jnet_player_name(unsigned id) {
    static const char *names[]={"Unknown","Test client 1","Test client 2","Test client 3"};
    return names[id<=3?id:0];
}
#endif
