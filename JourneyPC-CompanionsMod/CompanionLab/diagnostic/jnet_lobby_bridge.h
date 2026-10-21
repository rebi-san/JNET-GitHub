#ifndef JNET_LOBBY_BRIDGE_H
#define JNET_LOBBY_BRIDGE_H
#include <stdatomic.h>
struct jnet_lobby_row { uint64_t room,host; unsigned players,access; };
struct jnet_lobby_view {
    struct jnet_lobby_row rows[3];unsigned count,identity,ready;
    uint64_t room,self,host,members[3];unsigned member_count,level;char status[80];
    unsigned online,busy,total,page,population;char names[3][33],code[5];
    char member_names[3][33],host_name[33];unsigned chat,cmatw,anonymous;
};
struct jnet_lobby_action { unsigned kind,access,mode,level,chat,cmatw,lighting,paradise;uint64_t host,room;char password[33]; };
static _Atomic unsigned jnet_windwalls=1,jnet_sliding=1,jnet_freezing=1;
static _Atomic unsigned jnet_options_failed;
static _Atomic unsigned jnet_credits=1;
static _Atomic unsigned jnet_paradise_cutscene;
static _Atomic unsigned jnet_daynight_mode,jnet_daynight_host=~0u;
static inline int jnet_daynight_supported(unsigned level){return level<8;}
/* Lighting 0 Day, 1 Night (Dusk on Pink Desert, Dawn on Sunken City), 2 Cycle, 3 Night on Pink Desert and Sunken City only. */
static inline unsigned jnet_daynight_normalize(unsigned level,unsigned mode){return mode==3 && level!=2 && level!=3?1u:mode;}
static inline void jnet_daynight_enter(unsigned level){
    unsigned host=jnet_daynight_host;
    if(host<=3 && jnet_daynight_supported(level))jnet_daynight_mode=jnet_daynight_normalize(level,host);
}
static _Atomic unsigned jnet_character_spotlight;
static int (*jnet_character_capture_rotation)(float *);
static _Atomic unsigned jnet_current_level=2;
static _Atomic unsigned jnet_mepal_play;
static _Atomic unsigned jnet_save_requested;
static _Atomic uint64_t jnet_travel_epoch;
static _Atomic unsigned jnet_travel_busy,jnet_travel_target;
static int (*jnet_travel_request)(unsigned);
static int (*jnet_travel_receive)(unsigned,const unsigned char *,size_t);
static void (*jnet_travel_poll)(void);
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
