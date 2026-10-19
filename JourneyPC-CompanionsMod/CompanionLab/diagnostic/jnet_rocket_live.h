#ifndef JNET_ROCKET_LIVE_H
#define JNET_ROCKET_LIVE_H
struct jnet_rocket_shot { float position[4],velocity[4];uint64_t source,lease,sequence,epoch,seen,room;unsigned chapter; };
static SRWLOCK jnet_rocket_lock=SRWLOCK_INIT;
static struct {struct jnet_rocket_shot incoming[32],outgoing[32];unsigned in_count,out_count;} jnet_rockets;
static uint64_t jnet_rocket_generation,jnet_rocket_room,jnet_rocket_sequence,jnet_rocket_last_shot;
static uint64_t jnet_rocket_received[7],jnet_rocket_ids[7],jnet_rocket_leases[7],jnet_rocket_last_received[7];

static int jnet_rocket_valid(const struct jnet_rocket_shot *s) {
    float speed=0;
    if(s->chapter>=12)return 0;
    for(unsigned i=0;i<4;++i){
        if(!isfinite(s->position[i]) || fabsf(s->position[i])>100000 || !isfinite(s->velocity[i]) || fabsf(s->velocity[i])>2000)return 0;
        if(i<3)speed+=s->velocity[i]*s->velocity[i];
    }
    return speed>0 && speed<=4000000;
}
#ifndef JNET_ROCKET_NETWORK_FIXTURE
static void (*jnet_rocket_update_original)(uintptr_t,uintptr_t);
static int jnet_rocket_mode(uintptr_t game,int enabled) {
    lua_State *state=NULL;
    if(!own_read(NULL,game+0x20,&state,8) || !state)return 0;
    const char *source=enabled?
        "if not IsDMActive(game) then ToggleDMActive(game) end":
        "if IsDMActive(game) then ToggleDMActive(game) end";
    int top=gettop(state),ok=jnet_character_lua_chunk(state,source,0);settop(state,top);return ok;
}
static int jnet_character_apply(uintptr_t game,uintptr_t barn,unsigned kind) {
    if(kind>=JNET_CHARACTER_COUNT)return 0;
    unsigned previous=jnet_menu_edit.applied.character;
    if(!jnet_rocket_mode(game,kind==JNET_CHARACTER_ROCKET))return 0;
    if(jnet_character_switch_native(game,barn,kind==JNET_CHARACTER_ROCKET?0:kind))return 1;
    jnet_rocket_mode(game,previous==JNET_CHARACTER_ROCKET);return 0;
}
static void jnet_rocket_update(uintptr_t module,uintptr_t game) {
    uintptr_t events=0;unsigned before=512,after=512;unsigned char enabled=0;
    int readable=own_read(NULL,module+0x10,&enabled,1) && own_read(NULL,game+0x250,&events,8) && events &&
        own_read(NULL,events+0x1008,&before,4);
    uint64_t now=GetTickCount64();
    int suppress=readable && enabled && (jnet_menu_edit.applied.character!=JNET_CHARACTER_ROCKET || before>=480 ||
        (jnet_rocket_last_shot && now-jnet_rocket_last_shot<250) || jnet_travel_busy);
    uintptr_t input=0,pad=0,channel=0;unsigned char pressed=0,off=0;
    if(suppress){
        if(!own_read(NULL,game+0x40,&input,8) || !input || !own_read(NULL,input+8,&pad,8) || !pad ||
           !own_read(NULL,pad+0x260,&channel,8) || !channel || !own_read(NULL,channel+0x17,&pressed,1) ||
           !jc_local_windows_write(channel+0x17,&off,1))return;
    }
    jnet_rocket_update_original(module,game);
    if(suppress){jc_local_windows_write(channel+0x17,&pressed,1);return;}
    if(!readable || !enabled || before>=480 || !own_read(NULL,events+0x1008,&after,4) || after!=before+1)return;
    uintptr_t event=0,type=0;struct jnet_rocket_shot shot={0};
    if(!own_read(NULL,events+8+(uint64_t)before*8,&event,8) || !event ||
       !own_read(NULL,event,&type,8) || type!=fault_image+0x66b160 ||
       !own_read(NULL,event+0x70,shot.position,16) || !own_read(NULL,event+0x60,shot.velocity,16))return;
    shot.chapter=jnet_current_level;shot.epoch=jnet_travel_epoch;shot.seen=now;
    if(!jnet_rocket_valid(&shot))return;
    jnet_rocket_last_shot=now;
    AcquireSRWLockExclusive(&jnet_rocket_lock);
    if(jnet_rockets.out_count<32)jnet_rockets.outgoing[jnet_rockets.out_count++]=shot;
    ReleaseSRWLockExclusive(&jnet_rocket_lock);
}
#endif
static void jnet_rocket_pack(unsigned char p[128],const struct jnet_rocket_shot *s) {
    memset(p,0,128);memcpy(p,"JNR1",4);jc_packet_put(p+8,jnet_sl.room,8);memcpy(p+16,jnet_sl.nonce,32);
    jc_packet_put(p+48,s->source,8);jc_packet_put(p+56,s->lease,8);jc_packet_put(p+64,s->sequence,8);
    jc_packet_put(p+72,s->epoch,8);jc_packet_put(p+80,s->chapter,4);
    jnet_sg_floats(p+88,s->position,4);jnet_sg_floats(p+104,s->velocity,4);
}
static void jnet_rocket_poll_live(void) {
    if(jnet_rocket_room!=jnet_sl.room || jnet_rocket_generation!=jnet_sl.generation){
        jnet_rocket_room=jnet_sl.room;jnet_rocket_generation=jnet_sl.generation;
        memset(jnet_rocket_received,0,sizeof(jnet_rocket_received));memset(jnet_rocket_ids,0,sizeof(jnet_rocket_ids));
        memset(jnet_rocket_leases,0,sizeof(jnet_rocket_leases));memset(jnet_rocket_last_received,0,sizeof(jnet_rocket_last_received));
        AcquireSRWLockExclusive(&jnet_rocket_lock);memset(&jnet_rockets,0,sizeof(jnet_rockets));ReleaseSRWLockExclusive(&jnet_rocket_lock);
    }
    struct jnet_rocket_shot shots[32];unsigned count;
    AcquireSRWLockExclusive(&jnet_rocket_lock);count=jnet_rockets.out_count;
    memcpy(shots,jnet_rockets.outgoing,count*sizeof(shots[0]));jnet_rockets.out_count=0;ReleaseSRWLockExclusive(&jnet_rocket_lock);
    if(!jnet_sl.room || jnet_travel_busy)return;
    uint64_t now=GetTickCount64();
    for(unsigned n=0;n<count;++n){
        struct jnet_rocket_shot *s=shots+n;
        if(now<s->seen || now-s->seen>500 || s->epoch!=jnet_travel_epoch || s->chapter!=jnet_current_level)continue;
        s->source=jnet_sl.identity;s->lease=jnet_sg.local.lease;s->sequence=++jnet_rocket_sequence;
        if(!s->lease)continue;
        unsigned char packet[128];jnet_rocket_pack(packet,s);
        for(unsigned i=0;i<7;++i)if(jnet_sn.peers[i].connected && jnet_sn.peers[i].admitted)
            jnet_sn_transmit(jnet_sn.peers[i].handle,packet,sizeof(packet),9);
    }
}
static int jnet_rocket_message(unsigned slot,const unsigned char *p,size_t bytes) {
    if(bytes<4 || memcmp(p,"JNR1",4))return 0;
    if(bytes!=128 || slot>=7 || !jnet_sl.room || jnet_travel_busy ||
       !jnet_sn.peers[slot].admitted || !jnet_sn.peers[slot].connected || !jnet_sn_member(jnet_sn.peers[slot].identity) ||
       jc_packet_get(p+8,8)!=jnet_sl.room || memcmp(p+16,jnet_sl.nonce,32))return 1;
    struct jnet_rocket_shot s={0};s.source=jc_packet_get(p+48,8);s.lease=jc_packet_get(p+56,8);s.sequence=jc_packet_get(p+64,8);
    s.room=jnet_sl.room;s.epoch=jc_packet_get(p+72,8);s.chapter=(unsigned)jc_packet_get(p+80,4);s.seen=GetTickCount64();
    jnet_sg_read_floats(s.position,p+88,4);jnet_sg_read_floats(s.velocity,p+104,4);
    int host=jnet_sl.host==jnet_sl.identity;
    if(!s.source || !s.lease || !s.sequence || s.source==jnet_sl.identity || !jnet_sn_member(s.source) ||
       (host?s.source!=jnet_sn.peers[slot].identity:jnet_sn.peers[slot].identity!=jnet_sl.host) ||
       s.epoch!=jnet_travel_epoch || !jnet_rocket_valid(&s))return 1;
    if(host){
        if(jnet_sg.peers[slot].id!=s.source || !jnet_sg.peers[slot].character_known ||
           jnet_sg.peers[slot].character!=JNET_CHARACTER_ROCKET || jnet_sg.peers[slot].sample.chapter!=s.chapter)return 1;
        s.lease=jnet_sg.peers[slot].lease;
    }
    unsigned at=0;
    for(;at<7;++at)if(jnet_rocket_ids[at]==s.source)break;
    if(at==7)for(at=0;at<7;++at)if(!jnet_rocket_ids[at] || !jnet_sn_member(jnet_rocket_ids[at]))break;
    if(at==7)return 1;
    if(jnet_rocket_ids[at]!=s.source || jnet_rocket_leases[at]!=s.lease){jnet_rocket_ids[at]=s.source;jnet_rocket_leases[at]=s.lease;jnet_rocket_received[at]=jnet_rocket_last_received[at]=0;}
    if(s.sequence<=jnet_rocket_received[at])return 1;
    jnet_rocket_received[at]=s.sequence;
    if(jnet_rocket_last_received[at] && s.seen-jnet_rocket_last_received[at]<200)return 1;
    jnet_rocket_last_received[at]=s.seen;
    if(host){unsigned char packet[128];jnet_rocket_pack(packet,&s);
        for(unsigned i=0;i<7;++i)if(i!=slot && jnet_sn.peers[i].connected && jnet_sn.peers[i].admitted)
            jnet_sn_transmit(jnet_sn.peers[i].handle,packet,sizeof(packet),9);}
    AcquireSRWLockExclusive(&jnet_rocket_lock);
    if(jnet_rockets.in_count<32)jnet_rockets.incoming[jnet_rockets.in_count++]=s;
    ReleaseSRWLockExclusive(&jnet_rocket_lock);return 1;
}
#ifndef JNET_ROCKET_NETWORK_FIXTURE
static void jnet_rocket_game(uintptr_t game) {
    static uint64_t generation;
    uintptr_t barn=0,save=0;
    if(!jnet_appearance_context(game,&barn,&save))return;
    if(generation!=jnet_character_generation && jnet_rocket_mode(game,jnet_menu_edit.applied.character==JNET_CHARACTER_ROCKET))generation=jnet_character_generation;
    struct jnet_rocket_shot shots[32];unsigned count;
    AcquireSRWLockExclusive(&jnet_rocket_lock);count=jnet_rockets.in_count;
    memcpy(shots,jnet_rockets.incoming,count*sizeof(shots[0]));jnet_rockets.in_count=0;ReleaseSRWLockExclusive(&jnet_rocket_lock);
    struct jnet_game_record records[3];uint64_t room;
    AcquireSRWLockExclusive(&jnet_sg_lock);room=jnet_sg_mail.room;memcpy(records,jnet_sg_mail.records,sizeof(records));ReleaseSRWLockExclusive(&jnet_sg_lock);
    if(!room || jnet_travel_busy)return;
    uint64_t now=GetTickCount64();
    for(unsigned n=0;n<count;++n){
        struct jnet_rocket_shot *s=shots+n;int admitted=0;
        if(s->room!=room || now<s->seen || now-s->seen>500 || s->epoch!=jnet_travel_epoch || s->chapter!=jnet_current_level)continue;
        for(unsigned i=0;i<3;++i)if(records[i].id==s->source && records[i].lease==s->lease && records[i].character_known &&
            records[i].character==JNET_CHARACTER_ROCKET && records[i].sample.chapter==s->chapter &&
            now>=records[i].seen && now-records[i].seen<2000)admitted=1;
        if(!admitted)continue;
        uintptr_t events=0;unsigned used=512;
        if(!own_read(NULL,game+0x250,&events,8) || !events || !own_read(NULL,events+0x1008,&used,4) || used>=480)continue;
        uintptr_t (*metadata)(void)=(void *)(fault_image+0x191790);
        uintptr_t (*create)(uintptr_t,uintptr_t)=(void *)(fault_image+0xcabe0);
        uintptr_t event=create(events,metadata()),type=0,start=0;
        float zero[4]={0};unsigned char remote=1;
        if(!event || !own_read(NULL,event,&type,8) || type!=fault_image+0x66b160 || !own_read(NULL,type+0x28,&start,8) || !start)continue;
        if(!jc_local_windows_write(event+0x50,zero,16) || !jc_local_windows_write(event+0x60,s->velocity,16) ||
           !jc_local_windows_write(event+0x70,s->position,16) || !jc_local_windows_write(event+0x80,&remote,1))continue;
        ((void (*)(uintptr_t))start)(event);
    }
}
#endif
#endif
