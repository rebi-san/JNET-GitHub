#ifndef JNET_TRAVEL_LIVE_H
#define JNET_TRAVEL_LIVE_H
static const char *const jnet_travel_levels[]={"Level_Graveyard","Level_Barrens","Level_Desert","Level_Canyon",
    "Level_Cave","Level_Ruins","Level_Mountain","Level_Summit","Level_Credits","Level_Bryan","Level_Matt","Level_Chris"};
static void (*jnet_queue_level_original)(uintptr_t,const char *,unsigned char,unsigned char);
static struct {
    uint64_t room,generation,deadline,serial;
    uint64_t members[3];unsigned count,loaded,level,freeplay,chapter_select,restart,paradise,queued,local_loaded,init_before,delete_before,had_object;
    uint32_t handles[7];
    unsigned lighting,cycle_ms;
} jnet_travel;
#ifndef JNET_TRAVEL_FIXTURE
static int jnet_travel_level(const char *name) {
    if(!name)return -1;
    for(unsigned i=0;i<12;++i)if(!strcmp(name,jnet_travel_levels[i]))return (int)i;
    return -1;
}

static int jnet_mepal_site(uintptr_t at,const unsigned char *normal,const unsigned char *enabled,size_t bytes,int on) {
    unsigned char current[16];DWORD previous,ignored;
    if(bytes>sizeof(current) || !own_read(NULL,at,current,bytes) ||
        (memcmp(current,normal,bytes) && memcmp(current,enabled,bytes)))return 0;
    const unsigned char *desired=on?enabled:normal;
    if(!memcmp(current,desired,bytes))return 1;
    if(!VirtualProtect((void *)at,bytes,PAGE_EXECUTE_READWRITE,&previous))return 0;
    memcpy((void *)at,desired,bytes);
    int ok=FlushInstructionCache(GetCurrentProcess(),(void *)at,bytes)!=0;
    ok=VirtualProtect((void *)at,bytes,previous,&ignored)!=0 && ok;
    return ok;
}
static int jnet_mepal_prepare(unsigned level) {
    static const unsigned char a[]={0x66,0xc7,0x41,8,1,1},b[]={0x66,0xc7,0x41,8,0,0};
    static const unsigned char c[]={0x66,0xc7,0x42,9,0,1},d[]={0x66,0xc7,0x42,9,0,0};
    static const unsigned char e[]={0x0f,0x92,0xc0,0x88,0x43,9,0x48,0x8b,0x5c,0x24,0x30},f[]={0x31,0xc0,0x88,0x43,9,0x90,0x48,0x8b,0x5c,0x24,0x30};
    static const unsigned char g[]={0xc6,0x43,0x0a,1},h[]={0xc6,0x43,0x0a,0};
    const uintptr_t sites[]={fault_image+0x187f67,fault_image+0x170e22,fault_image+0x170e9c,fault_image+0x17dab7};
    const unsigned char *normal[]={a,c,g,e},*enabled[]={b,d,h,f};
    const size_t sizes[]={sizeof(a),sizeof(c),sizeof(g),sizeof(e)};
    unsigned char before[4][16];int old[4];
    const unsigned meet=1,play=jnet_mepal_play;
    if(level>=12)return 0;
    for(unsigned i=0;i<4;++i){
        if(!own_read(NULL,sites[i],before[i],sizes[i]))return 0;
        old[i]=!memcmp(before[i],enabled[i],sizes[i]);
        if(!old[i] && memcmp(before[i],normal[i],sizes[i]))return 0;
    }
    for(unsigned i=0;i<4;++i)if(!jnet_mepal_site(sites[i],normal[i],enabled[i],sizes[i],i<3?meet:play)){
        int restored=1;
        for(unsigned j=0;j<=i;++j)restored=jnet_mepal_site(sites[j],normal[j],enabled[j],sizes[j],old[j]) && restored;
        if(!restored)message("[JC] MEPAL patch rollback failed; restart Journey before further travel");
        return 0;
    }
    return 1;
}
#endif
static void jnet_travel_reset(void) {
    memset(&jnet_travel,0,sizeof(jnet_travel));jnet_travel_epoch=0;jnet_travel_busy=0;jnet_daynight_sync=0;
}
static void jnet_travel_leave_for_title(void) {
    memset(&jnet_sl.deferred,0,sizeof(jnet_sl.deferred));
    ++jnet_sl.generation;
    jnet_sl_leave();
    jnet_sl.cancelled=!!jnet_sl.call;
    jnet_travel_reset();
    jnet_sl_status("Left Steam lobby - returning to title");
}
#ifndef JNET_TRAVEL_FIXTURE
static void jnet_travel_quit_to_title(void) {
    jnet_realtime_enter();
    AcquireSRWLockExclusive(&jnet_lobby_lock);
    memset(&jnet_lobby_action,0,sizeof(jnet_lobby_action));
    ReleaseSRWLockExclusive(&jnet_lobby_lock);
    jnet_travel_leave_for_title();
    message("[JNET] confirmed Quit to Title: lobby left and pending travel cancelled");
    jnet_realtime_leave();
}
#endif
static void jnet_travel_members_view(struct jnet_lobby_view *view) {
    if(!view->room)return;
    view->member_count=0;
    if(view->host)view->members[view->member_count++]=view->host;
    if(jnet_sn_members.room!=view->room || jnet_sn_members.count>3)return;
    for(unsigned i=0;i<jnet_sn_members.count && view->member_count<3;++i)
        if(jnet_sn_members.ids[i] && jnet_sn_members.ids[i]!=view->host)view->members[view->member_count++]=jnet_sn_members.ids[i];
}
static int jnet_travel_send(unsigned type,uint64_t only) {
    unsigned char packet[104]={0};memcpy(packet,"JNT3",4);jc_packet_put(packet+4,type,4);
    jc_packet_put(packet+8,jnet_sl.room,8);memcpy(packet+16,jnet_sl.nonce,32);
    jc_packet_put(packet+48,jnet_travel_epoch,8);jc_packet_put(packet+56,jnet_travel.level,4);
    jc_packet_put(packet+60,jnet_travel.count,4);
    jc_packet_put(packet+88,jnet_travel.freeplay,4);
    jc_packet_put(packet+96,jnet_travel.lighting,4);jc_packet_put(packet+100,jnet_travel.cycle_ms,4);
    jc_packet_put(packet+92,jnet_travel.chapter_select|(jnet_travel.restart<<1)|(jnet_travel.paradise<<2),4);
    for(unsigned i=0;i<jnet_travel.count;++i)jc_packet_put(packet+64+i*8,jnet_travel.members[i],8);
    int ok=1;
    for(unsigned i=0;i<7;++i)if(jnet_sn.peers[i].admitted && jnet_sn.peers[i].connected && (!only || jnet_sn.peers[i].identity==only))
        if(jnet_sn_transmit(jnet_sn.peers[i].handle,packet,sizeof(packet),9)!=1)ok=0;
    return ok;
}
static void jnet_travel_cancel(const char *reason) {
    jnet_travel_busy=0;jnet_travel.queued=0;jnet_sg_reset();
    jnet_sl_status(reason);
}
static void jnet_travel_pause(void) {
    jnet_travel_target=jnet_travel.level;
    AcquireSRWLockExclusive(&jnet_sg_lock);
    jnet_sg_mail.event_count=jnet_sg_mail.outgoing_count=0;
    memset(&jnet_sg_mail.outgoing.chirp,0,sizeof(jnet_sg_mail.outgoing.chirp));
    if(jnet_travel.level!=jnet_current_level){
        memset(jnet_sg_mail.records,0,sizeof(jnet_sg_mail.records));
        memset(&jnet_sg_mail.outgoing,0,sizeof(jnet_sg_mail.outgoing));
    }
    ReleaseSRWLockExclusive(&jnet_sg_lock);
    jnet_travel_busy=1;
}
static int jnet_travel_begin_mode(unsigned level,unsigned freeplay,unsigned chapter_select,unsigned restart) {
    if(level>=12){jnet_sl_status("Unknown destination");return 0;}
    if(jnet_travel_busy){jnet_sl_status("Previous destination is still loading");return 0;}
    if(!jnet_queue_level_original){jnet_sl_status("Native level travel is unavailable");return 0;}
    if(JNETSteamLobby[10]){jnet_sl_status("Native companion ownership is present or unreadable");return 0;}
    if(jnet_sl.room && jnet_sl.host!=jnet_sl.identity){jnet_sl_status("Only the current host can choose a destination");return 0;}
    if(jnet_sl.room && (!jnet_sn_member(jnet_sl.identity) || jnet_sn_members.count>3))return 0;
    if(jnet_sl.room){
        unsigned admitted=1;
        for(unsigned i=0;i<7;++i)admitted+=!!(jnet_sn.peers[i].admitted && jnet_sn.peers[i].connected);
        if(admitted!=jnet_sn_members.count){jnet_sl_status("Waiting for guest connections before travel");return 0;}
    }
    for(unsigned i=0;i<7;++i)jnet_travel.handles[i]=jnet_sn.peers[i].admitted?jnet_sn.peers[i].handle:0;
    jnet_travel.count=jnet_sl.room?jnet_sn_members.count:0;
    for(unsigned i=0;i<jnet_travel.count;++i)jnet_travel.members[i]=jnet_sn_members.ids[i];
    if(jnet_travel.serial==UINT64_MAX)return 0;
    jnet_travel_epoch=++jnet_travel.serial;jnet_travel.level=level;jnet_travel.freeplay=freeplay;jnet_travel.chapter_select=chapter_select;jnet_travel.restart=restart;
    jnet_travel.paradise=level==7 && restart && jnet_paradise_cutscene;
    jnet_travel.room=jnet_sl.room;jnet_travel.generation=jnet_sl.generation;
    jnet_travel.lighting=jnet_daynight_supported(level)?jnet_daynight_mode:0;
    if(jnet_travel.lighting==3 && level!=2 && level!=3)jnet_travel.lighting=0;
    jnet_travel.cycle_ms=jnet_travel.lighting==2?jnet_daynight_elapsed:0;
    jnet_travel.deadline=GetTickCount64()+90000;jnet_travel.loaded=0;jnet_travel.local_loaded=0;
    jnet_travel.queued=0;jnet_travel_pause();
    if(jnet_sl.room && !jnet_travel_send(1,0)){jnet_travel_cancel("Travel could not reach every guest");return 0;}
    jnet_daynight_sync=(uint64_t)jnet_travel.cycle_ms+1;
    jnet_sl_status("Traveling together");return 1;
}
static int jnet_travel_begin(unsigned level) {
    return jnet_travel_begin_mode(level,level==8 || level==10 || level==11,level==0,1);
}
static int jnet_travel_message(unsigned slot,const unsigned char *p,size_t bytes) {
    if(bytes<4 || memcmp(p,"JNT3",4))return 0;
    if(bytes!=104 || slot>=7 || !jnet_sn.peers[slot].admitted || !jnet_sn.peers[slot].connected ||
        jc_packet_get(p+8,8)!=jnet_sl.room || memcmp(p+16,jnet_sl.nonce,32) || jc_packet_get(p+88,4)>1 || jc_packet_get(p+92,4)>7)return 1;
    unsigned lighting=(unsigned)jc_packet_get(p+96,4),cycle_ms=(unsigned)jc_packet_get(p+100,4);
    if(lighting>3 || cycle_ms>=1800000 || (lighting!=2 && cycle_ms))return 1;
    unsigned freeplay=(unsigned)jc_packet_get(p+88,4),chapter_select=(unsigned)jc_packet_get(p+92,4)&1u,restart=((unsigned)jc_packet_get(p+92,4)>>1)&1u,paradise=(unsigned)jc_packet_get(p+92,4)>>2;
    uint64_t source=jnet_sn.peers[slot].identity,epoch=jc_packet_get(p+48,8);
    unsigned kind=(unsigned)jc_packet_get(p+4,4),level=(unsigned)jc_packet_get(p+56,4),count=(unsigned)jc_packet_get(p+60,4);
    if((lighting && level>=8) || (lighting==3 && level!=2 && level!=3) ||
       !jnet_sn_member(source) || level>=12 || !epoch || !count || count>3 ||
       (chapter_select && level!=0) || (paradise && (level!=7 || !restart)) ||
       (freeplay && level!=8 && level!=10 && level!=11))return 1;
    uint64_t members[3]={0};unsigned self=0,host=0;
    for(unsigned i=0;i<count;++i){members[i]=jc_packet_get(p+64+i*8,8);if(!members[i] || !jnet_sn_member(members[i]))return 1;
        for(unsigned j=0;j<i;++j)if(members[j]==members[i])return 1;
        self+=members[i]==jnet_sl.identity;host+=members[i]==jnet_sl.host;}
    if(!self || !host)return 1;
    if(kind==1 && source==jnet_sl.host && source!=jnet_sl.identity && epoch>jnet_travel_epoch) {
        unsigned continuing=!restart && !chapter_select && jnet_travel_busy && level==jnet_travel.level && freeplay==jnet_travel.freeplay;
        jnet_travel_epoch=epoch;jnet_travel.room=jnet_sl.room;jnet_travel.generation=jnet_sl.generation;
        jnet_travel.level=level;jnet_travel.freeplay=freeplay;jnet_travel.chapter_select=chapter_select;jnet_travel.restart=restart;jnet_travel.paradise=paradise;jnet_travel.count=count;memcpy(jnet_travel.members,members,sizeof(members));
        jnet_travel.lighting=lighting;jnet_travel.cycle_ms=cycle_ms;
        jnet_daynight_mode=lighting;jnet_daynight_host=lighting;
        jnet_daynight_sync=(uint64_t)cycle_ms+1;
        jnet_travel.deadline=GetTickCount64()+90000;jnet_travel.loaded=0;
        if(!continuing)jnet_travel.queued=jnet_travel.local_loaded=0;
        jnet_travel_pause();
    } else if(epoch==jnet_travel_epoch && level==jnet_travel.level && count==jnet_travel.count &&
        !memcmp(members,jnet_travel.members,count*sizeof(uint64_t))) {
        if(kind==2 && jnet_sl.host==jnet_sl.identity)
            for(unsigned i=0;i<count;++i)if(members[i]==source)jnet_travel.loaded|=1u<<i;
    }
    return 1;
}
static void jnet_travel_complete(void) {
    jnet_travel.local_loaded=1;jnet_travel_busy=0;jnet_save_requested=1;
    jnet_sl_status(jnet_sl.room?"Steam connected - gameplay active":"Destination loaded - ready to travel");
    JNETSteamGame[90]=0;JNETSteamGame[95]=1;
}
static int jnet_travel_playable_menu(void);
static void jnet_travel_poll_live(void) {
    JNETSteamGame[90]=(LONG)jnet_travel_busy;JNETSteamGame[91]=(LONG)jnet_travel.level;
    JNETSteamGame[92]=(LONG)jnet_current_level;JNETSteamGame[93]=(LONG)jnet_travel_epoch;
    JNETSteamGame[94]=(LONG)jnet_travel.queued;JNETSteamGame[95]=(LONG)jnet_travel.local_loaded;
    JNETSteamGame[96]=(LONG)jnet_travel.loaded;JNETSteamGame[97]=(LONG)jnet_travel.count;
    if(jnet_travel_busy) {
        if(jnet_travel.room!=jnet_sl.room || jnet_travel.generation!=jnet_sl.generation || GetTickCount64()>=jnet_travel.deadline) {
            jnet_travel_cancel("Travel cancelled: session changed or loading timed out");return;
        }
        if(jnet_travel.local_loaded)jnet_travel_complete();
        else {jnet_sl_status("Loading destination - other players can continue");return;}
    }
    if(jnet_sl.room && jnet_sl.host==jnet_sl.identity && JNETSteamLobby[9] && jnet_travel_playable_menu()) {
        for(unsigned i=0;i<7;++i)if(jnet_sn.peers[i].admitted && jnet_sn.peers[i].connected &&
            jnet_travel.handles[i]!=jnet_sn.peers[i].handle) {
            jnet_travel_begin_mode(jnet_current_level,jnet_mepal_play,0,0);
            break;
        }
    }
}
static int jnet_travel_native_request(int level,unsigned chapter_select) {
    if(level<0 || level>=12 || !jnet_sl.room)return 0;
    if(jnet_sl.host==jnet_sl.identity){
        unsigned previous=jnet_daynight_mode,paradise=jnet_paradise_cutscene;
        if(previous==3)jnet_daynight_mode=0;
        jnet_paradise_cutscene=0;
        if(!jnet_travel_begin_mode((unsigned)level,0,level==0 && chapter_select,1)){jnet_daynight_mode=previous;jnet_paradise_cutscene=paradise;}
    }
    return 1;
}
static int jnet_travel_native_loaded(unsigned current,unsigned target,unsigned pending,int state,int resume) {
    return current==target && !pending && state>0 && state==resume;
}
#ifndef JNET_TRAVEL_FIXTURE
static uintptr_t jnet_chapter_menu,jnet_chapter_game;
static void (*jnet_menu_frame_original)(uintptr_t,uintptr_t);
static void jnet_menu_frame(uintptr_t menu,uintptr_t game) {
    jnet_chapter_menu=menu;jnet_chapter_game=game;
    jnet_menu_frame_original(menu,game);
}
static int jnet_travel_playable_menu(void) {
    uintptr_t type=0;unsigned char flags[2]={0};
    return jnet_chapter_game && jnet_chapter_menu &&
        own_read(NULL,jnet_chapter_menu,&type,8) && type==fault_image+0x679c18 &&
        own_read(NULL,jnet_chapter_menu+0x20,flags,sizeof(flags)) && !flags[0] && flags[1]==1;
}
static int jnet_travel_entry_chapter(unsigned level,unsigned restart,unsigned chapter_select,int playable) {
    return chapter_select || (!level && !restart && !playable);
}
static int jnet_travel_menu_prepare(uintptr_t game,unsigned chapter_select) {
    uintptr_t save=0,type=0,menu=jnet_chapter_menu;
    unsigned char snapshot[0x1ba8],menu_state[0xf0],spawn[12];
    uint64_t zero=0;unsigned char enabled=1;float fade;
    if(game!=jnet_chapter_game || !menu ||
       !own_read(NULL,menu,menu_state,sizeof(menu_state)))return 0;
    memcpy(&type,menu_state,8);if(type!=fault_image+0x679c18)return 0;
    if(!chapter_select)return jc_local_windows_write(menu+0x20,&zero,1) &&
        jc_local_windows_write(menu+0x21,&enabled,1);
    if(!own_read(NULL,game+0x170,&save,8) || !save ||
       !own_read(NULL,save,&type,8) || type!=fault_image+0x5e3178 ||
       !own_read(NULL,save+8,snapshot,sizeof(snapshot)) ||
       !own_read(NULL,game+0x1c0374,spawn,sizeof(spawn)))return 0;

    uintptr_t target=fault_image+0x5d8c58,black=fault_image+0x5dfe64;
    if(!own_read(NULL,fault_image+0x68b7f0,&fade,4))return 0;
    for(unsigned i=0;i<13;++i)
        if(!jc_local_windows_write(save+0x60+i*0x158,&zero,8))return 0;
    return jc_local_windows_write(save+0x1590,&zero,4) &&
        jc_local_windows_write(save+0x50,&enabled,1) &&
        jc_local_windows_write(menu+0x20,&zero,1) &&
        jc_local_windows_write(menu+0x21,&enabled,1) &&
        jc_local_windows_write(menu+0xe8,&zero,4) &&
        jc_local_windows_write(menu+0xe0,&black,8) &&
        jc_local_windows_write(game+0x1c0374,&zero,8) &&
        jc_local_windows_write(game+0x1c037c,&zero,4) &&
        jc_local_windows_write(fault_image+0x3772ae0,&target,8) &&
        jc_local_windows_write(fault_image+0x403e290,&fade,4);
}
static int jnet_travel_level_ready(uintptr_t game) {
    unsigned char pending=1;int state=0,resume=0,level=-1;
    return !jc_render_level(own_read,NULL,game,&level) && level>=0 &&
        own_read(NULL,game+0x1c0330,&pending,1) &&
        own_read(NULL,game+0x278,&state,4) && own_read(NULL,game+0x27c,&resume,4) &&
        jnet_travel_native_loaded((unsigned)level,jnet_travel.level,pending,state,resume);
}
static int jnet_traveler_rearm(void) {
    if(JNETTraveler[0]==-1 && JNETTraveler[15]==23 && !traveler_object && !JNETTraveler[4] &&
        !traveler_actor && !traveler_live && !active_cloth_backend){
        allocation_attempted=0;JNETTraveler[0]=0;JNETTraveler[19]=1;return 1;
    }
    if(JNETTraveler[0]!=4 || traveler_object || active_cloth_backend || cloth_chapter.failed ||
        (traveler_actor && traveler_actor->state!=JC_ACTOR_RUNTIME_RETIRED) ||
        (traveler_live && (!traveler_live->backend.closed || traveler_live->backend.failed)))return 0;
    if(traveler_actor){HeapFree(GetProcessHeap(),0,traveler_actor);traveler_actor=NULL;}
    if(traveler_live){HeapFree(GetProcessHeap(),0,traveler_live);traveler_live=NULL;}
    memset(&traveler_actor_services,0,sizeof(traveler_actor_services));memset(&traveler_peer,0,sizeof(traveler_peer));
    memset(&traveler_departure,0,sizeof(traveler_departure));memset(&traveler_peer_glyph_resume,0,sizeof(traveler_peer_glyph_resume));
    memset(&actor_frame,0,sizeof(actor_frame));memset(&cloth_chapter,0,sizeof(cloth_chapter));
    memset(traveler_streams,0,sizeof(traveler_streams));memset(traveler_stream_waiting,0,sizeof(traveler_stream_waiting));
    memset(traveler_network.peers,0,sizeof(traveler_network.peers));
    traveler_local_state=0;traveler_actor_trial_join_pending=traveler_actor_trial_draining=0;
    traveler_peer_depart_requested=traveler_peer_restart_requested=0;JNETActorTrial[0]=0;
    allocation_attempted=0;JNETTraveler[0]=0;JNETTraveler[19]=1;return 1;
}
static void jnet_travel_game(uintptr_t game) {
    int level=-1;if(!jc_render_level(own_read,NULL,game,&level) && level>=0 && level<12)jnet_current_level=(unsigned)level;
    if(!jnet_travel_busy)return;
    jnet_realtime_enter();
    if(!jnet_travel.queued) {
        int playable=game==jnet_chapter_game && jnet_travel_playable_menu();
        if(playable && !jnet_travel.restart && !jnet_travel.chapter_select && jnet_travel_level_ready(game) && jnet_mepal_play==jnet_travel.freeplay)jnet_travel.queued=2;
        else {
            jnet_mepal_play=jnet_travel.freeplay;
            uintptr_t match=0;unsigned char connected=1;
            if(!own_read(NULL,game+0x180,&match,8) || !match || !own_read(NULL,match+0x11db4,&connected,1) || connected ||
                !jnet_mepal_prepare(jnet_travel.level)){jnet_travel_cancel("Travel refused: native ownership or MEPAL validation failed");goto done;}
            jnet_travel.init_before=(unsigned)JNETTraveler[4];jnet_travel.delete_before=(unsigned)JNETTraveler[6];jnet_travel.had_object=traveler_object!=0;
            if(!jnet_travel_menu_prepare(game,jnet_travel_entry_chapter(jnet_travel.level,jnet_travel.restart,jnet_travel.chapter_select,playable))){
                jnet_travel_cancel("Native travel menu state is unavailable");goto done;
            }
            traveler_local_interrupt();jnet_travel.queued=1;
            unsigned char debug=jnet_travel.level!=0;
            jnet_paradise_cutscene=jnet_travel.paradise;
            jnet_queue_level_original(game,jnet_travel_levels[jnet_travel.level],debug,debug);
        }
    }
    if(jnet_travel.queued && jnet_travel_level_ready(game) && !jnet_travel.local_loaded) {
        jnet_travel_complete();
        if(jnet_sl.identity==jnet_sl.host){for(unsigned i=0;i<jnet_travel.count;++i)if(jnet_travel.members[i]==jnet_sl.identity)jnet_travel.loaded|=1u<<i;}
        else if(jnet_sl.room && !jnet_travel_send(2,jnet_sl.host))jnet_travel_cancel("Travel acknowledgement failed");
    }
done:
    jnet_realtime_leave();
}
static void jnet_queue_level(uintptr_t game,const char *target,unsigned char immediate,unsigned char reset) {
    char name[32]={0};int readable=0;
    for(unsigned i=0;i<sizeof(name);++i){if(!own_read(NULL,(uintptr_t)target+i,name+i,1))break;if(!name[i]){readable=1;break;}}
    int level=readable?jnet_travel_level(name):-1;
    uintptr_t caller=(uintptr_t)__builtin_return_address(0);
    unsigned chapter_select=level==0 && caller==fault_image+0x21b1a4;
    if(jnet_travel_native_request(level,chapter_select))return;
    if(level>=0){jnet_mepal_play=0;jnet_paradise_cutscene=0;if(jnet_daynight_mode==3)jnet_daynight_mode=0;}
    if(level>=0 && !jnet_mepal_prepare((unsigned)level)){jnet_sl_status("MEPAL code validation failed");return;}
    jnet_queue_level_original(game,target,immediate,reset);
}
#endif
#endif
