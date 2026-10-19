#ifndef JC_TRAVELER_LOCAL_PEER_H
#define JC_TRAVELER_LOCAL_PEER_H
#include "native_local_peer.h"
#if defined(__x86_64__) && !defined(_WIN32)
#define JC_LOCAL_ABI __attribute__((ms_abi))
#else
#define JC_LOCAL_ABI
#endif
static unsigned traveler_local_state,traveler_local_outfit,traveler_local_symbol,traveler_local_completed;
static uint64_t traveler_local_frames,traveler_local_overlap,traveler_local_departed,traveler_local_sent;
static _Alignas(16) unsigned char traveler_local_departure[0x30];
static uint64_t traveler_local_dust_finished,traveler_local_started;
static uintptr_t traveler_local_owned_barn;
static void traveler_native_arrival(uint64_t actor,uint64_t generation);
static void traveler_local_report(int code) {
    if(traveler_stream_enabled)for(unsigned i=0;i<2;++i) {
        JNETActor[26+i*3]=(LONG)traveler_streams[i].stream.accepted;
        JNETActor[27+i*3]=(LONG)traveler_streams[i].stream.rejected;
        JNETActor[28+i*3]=(LONG)traveler_streams[i].dropped;
    }
    JNETActor[24]=(LONG)traveler_local_completed;JNETActor[25]=7;
    JNETActor[16]=(LONG)traveler_local_state;JNETActor[17]=code;
    JNETActor[18]=(LONG)traveler_local_frames;JNETActor[19]=(LONG)traveler_local_outfit;
    JNETActor[20]=(LONG)traveler_local_symbol;JNETActor[22]=(LONG)traveler_local_overlap;
}
static void traveler_local_interrupt(void) {
    #ifdef JC_NETWORK_UDP
    if(traveler_network_active){traveler_network.paused=1;JNETNetwork[0]=3;}
    #endif
    traveler_stream_close(0);traveler_stream_close(1);
    if(traveler_local_state && traveler_local_state<4) {
        traveler_local_state=6;traveler_repeat_cancel();traveler_local_report(0);
    }
}
static void traveler_local_level_initialized(uintptr_t dude) {
    if(!traveler_context() || dude!=traveler_barn+0x146f0)return;
    traveler_stream_close(1);
    traveler_local_state=0;traveler_local_completed=0;traveler_local_frames=0;
    traveler_local_departed=traveler_local_sent=traveler_local_dust_finished=0;
    memset(traveler_local_departure,0,sizeof(traveler_local_departure));
    traveler_local_report(0);
}
static unsigned traveler_local_guard;
static int traveler_local_offline(void) {
    uint64_t match=0;unsigned char connected=1;
    traveler_local_guard=0;
    if(!traveler_context())traveler_local_guard=1;
    else if(!own_read(NULL,traveler_game+0x180,&match,8))traveler_local_guard=2;
    else if(!match)traveler_local_guard=3;
    else if(!own_read(NULL,match+0x11db4,&connected,1))traveler_local_guard=4;
    else if(connected)traveler_local_guard=5;
    if(traveler_local_guard)traveler_local_owned_barn=0;
    return !traveler_local_guard;
}
#ifdef JC_NETWORK_UDP
static void traveler_local_network_guard(void) {
    if(!JNETNetwork[23]) {
        char line[160];
        JNETNetwork[23]=(LONG)traveler_local_guard;
        snprintf(line,sizeof(line),"[JC] network ownership guard reason=%u native-state=%u; restart required",
            traveler_local_guard,traveler_local_state);message(line);
    }
    traveler_network.paused=1;JNETNetwork[0]=-1;JNETNetwork[2]=123;
}
static int traveler_local_network_connect(void) {
    if(JNETNetwork[23])return 123;
    if(traveler_local_state>=5)return 124;
    if(JNETTraveler[0]!=3)return JC_ACTOR_RUNTIME_BUSY;
    if(!traveler_local_offline()) {traveler_local_network_guard();return 123;}
    if(!traveler_network_active && (traveler_local_state || traveler_actor || traveler_stream_enabled))
        return JC_ACTOR_RUNTIME_BUSY;
    return traveler_network_start();
}
#endif
static void traveler_local_stop(void) {
    traveler_stream_close(1);
    if(traveler_local_state==1){traveler_local_state=4;traveler_local_report(0);return;}
    if(traveler_local_state!=2)return;
    if(!traveler_local_offline()) {traveler_local_state=5;traveler_local_report(1);return;}
    static const unsigned char entry[21]={0xc6,0x41,0x08,0x00,0x66,0xc7,0x81,0xa2,0,0,0,0,1,0xc6,0x81,0xa4,0,0,0,1,0xc3};
    unsigned char actual[sizeof(entry)];
    if(!own_read(NULL,(uintptr_t)GetModuleHandleW(NULL)+0x117770,actual,sizeof(actual)) ||
        memcmp(entry,actual,sizeof(entry))){traveler_local_state=5;traveler_local_report(6);return;}
    ((void (JC_LOCAL_ABI *)(uintptr_t))((uintptr_t)GetModuleHandleW(NULL)+0x117770))(traveler_barn);
    memset(traveler_local_departure,0,sizeof(traveler_local_departure));
    ((void (JC_LOCAL_ABI *)(uintptr_t,uintptr_t))((uintptr_t)GetModuleHandleW(NULL)+0x21d980))
        ((uintptr_t)traveler_local_departure,traveler_game);
    traveler_local_dust_finished=0;
    traveler_local_state=3;traveler_local_departed=GetTickCount64();traveler_local_report(0);
}
static void traveler_local_fail(int code) {
    traveler_local_stop();traveler_local_state=5;traveler_local_report(code);
    traveler_repeat_cancel();if(!traveler_peer_vacant())traveler_actor_trial_stop(0);
}
static int traveler_local_write(void *ctx,uint64_t address,const void *data,size_t bytes) {
    (void)ctx;return traveler_local_offline() && jc_local_windows_write(address,data,bytes);
}
static void traveler_local_before_input(uintptr_t barn,uintptr_t game) {
    #ifdef JC_NETWORK_UDP
    if(traveler_network_active && barn==traveler_barn && game==traveler_game) {
        if(JNETNetwork[23]) {traveler_local_network_guard();return;}
        if(traveler_local_state>=5) {
            traveler_network.paused=1;JNETNetwork[0]=-1;JNETNetwork[2]=124;return;
        }
        if(!traveler_local_offline()) {
            traveler_local_network_guard();
            if(traveler_actor && !traveler_peer_vacant())traveler_actor_trial_stop(1);
            traveler_local_state=5;traveler_local_report(1);return;
        }
        traveler_network_tick(game,(uint32_t)JNETFrames);
        uint64_t now=GetTickCount64();
        for(unsigned slot=0;slot<2;++slot) {
            struct jc_network_peer *p=&traveler_network.peers[slot];
#ifdef JNET_LOBBY_ENABLED
            if(!jnet_lobby_allows(p->connection.peer))continue;
#endif
            if(traveler_network.paused || !p->connection.sequence || now<p->connection.last_seen ||
                now-p->connection.last_seen>=2000 || p->sample.chapter!=traveler_network.chapter)continue;
            if(slot==1 && (traveler_local_state==0 || traveler_local_state==4))traveler_local_state=1;
            if(slot==0 && (!traveler_actor || traveler_peer_vacant()) &&
                (JNETActorTrial[0]==0 || JNETActorTrial[0]==3) && !traveler_actor_trial_join_pending)
                traveler_peer_restart_requested=1;
        }
    }
    #endif
    if(!traveler_local_state || traveler_local_state>=4 || barn!=traveler_barn || game!=traveler_game)return;
    if(!traveler_local_offline()) {
        traveler_local_fail(1);return;
    }
    struct jc_layout_result layout;
    int code=jc_native_layout(own_read,NULL,(uintptr_t)GetModuleHandleW(NULL),game,&layout);
    if(code){traveler_local_fail(2);return;}
    JNETActor[21]=layout.remote_gate;
    if(traveler_local_state==2 && layout.local_chapter!=layout.remote_chapter) {
        traveler_local_interrupt();return;
    }
    if(traveler_local_state==3) {
        if(GetTickCount64()-traveler_local_departed>30000){traveler_local_fail(3);return;}
        if(traveler_local_departure[0x14]) {
            if(layout.local_chapter!=layout.remote_chapter) {traveler_local_interrupt();return;}
            if(!layout.gates[0] && !layout.gates[2]) {traveler_local_interrupt();return;}
            ((void (JC_LOCAL_ABI *)(uintptr_t,uintptr_t))((uintptr_t)GetModuleHandleW(NULL)+0x24fa70))
                (traveler_game,(uintptr_t)traveler_local_departure);
        } else if(!traveler_local_dust_finished) {
            traveler_local_dust_finished=GetTickCount64();
            message("[JC] local native companion meditation/dust finished; allowing effects to fade");
        } else if(GetTickCount64()-traveler_local_dust_finished>=2000) {
            unsigned char gates[3]={0,0,0};
            if(!jc_local_windows_write(barn+0xa2,gates,3)){traveler_local_fail(8);return;}
            ++traveler_local_completed;
            traveler_local_state=!traveler_network_active && traveler_repeat.state!=3 && traveler_repeat.state!=-1 && traveler_local_completed<7 ? 1 : 4;
            JNETActor[21]=0;traveler_local_report(0);
            message("[JC] local native companion departure complete without chapter change");
        }
        return;
    }
    if(!traveler_network_active && traveler_local_state==2 && (traveler_repeat.state==3 || traveler_repeat.state==-1 || GetTickCount64()-traveler_local_started>=11000)) {traveler_local_stop();return;}
    struct jc_actor_sender sender;
    unsigned first=traveler_local_state==1;
    if(!traveler_stream_enabled || first)code=jc_actor_capture_sender(own_read,NULL,(uintptr_t)GetModuleHandleW(NULL),game,&sender);
    if(code){
        if(first && traveler_stream_enabled && (code==JC_LAYOUT_RANGE || code==JC_LAYOUT_CHANGED)){
            traveler_local_report(4);return;
        }
        traveler_local_fail(4);return;
    }
    if(first) {
        uint64_t recipient=0;
        if(!own_read(NULL,barn+0x68,&recipient,8) || recipient!=barn+0x146f0 ||
            !traveler_admission_storage_matches((uintptr_t)GetModuleHandleW(NULL))) {
            char line[224];snprintf(line,sizeof(line),"[JNET] native admission declined: recipient=%u gate=%u flags=%u/%u/%u blocked=%u completed=%u storage=%u",
                recipient==barn+0x146f0,layout.remote_gate,layout.gates[0],layout.gates[1],layout.gates[2],layout.blocked,
                traveler_local_completed,traveler_admission_storage_matches((uintptr_t)GetModuleHandleW(NULL)));
            message(line);traveler_local_fail(5);return;
        }
        unsigned retained=traveler_network_active && traveler_local_owned_barn==barn &&
            layout.remote_gate && layout.gates[0]==1 && !layout.gates[1] && !layout.gates[2] &&
            !layout.blocked && layout.local_chapter==layout.remote_chapter;
        if(!retained && (layout.remote_gate || layout.gates[0] || (layout.gates[2] && traveler_local_completed) || (layout.blocked && !traveler_local_completed))){
            static uint64_t last_wait;
            static uintptr_t last_barn;
            static unsigned previous[8];
            unsigned snapshot[8]={(unsigned)layout.remote_gate,layout.gates[0],layout.gates[1],layout.gates[2],
                layout.blocked,layout.local_chapter,layout.remote_chapter,traveler_local_completed};
            uint64_t now=GetTickCount64();
            if(JNETActor[17]!=5 || last_barn!=barn || memcmp(previous,snapshot,sizeof(snapshot)) ||
                now<last_wait || now-last_wait>=5000){
                unsigned reasons=(layout.remote_gate?1u:0u) | (layout.gates[0]?2u:0u) |
                    (layout.gates[2] && traveler_local_completed?4u:0u) |
                    (layout.blocked && !traveler_local_completed?8u:0u);
                char line[480];
#ifdef JC_NETWORK_UDP
                long network_state=JNETNetwork[0],network_result=JNETNetwork[2];
                unsigned paused=traveler_network.paused;
#else
                long network_state=0,network_result=0;unsigned paused=0;
#endif
                snprintf(line,sizeof(line),"[JNET] native admission waiting: reasons=%u gate=%u flags=%u/%u/%u blocked=%u chapters=%u/%u completed=%u receiver=%u samples=%llu network=%ld/%ld paused=%u game=%llx barn=%llx; reasons: 1 remote gate, 2 a2, 4 a4 after departure, 8 blocked before departure; no native state overwritten",
                    reasons,snapshot[0],snapshot[1],snapshot[2],snapshot[3],snapshot[4],snapshot[5],snapshot[6],snapshot[7],
                    traveler_local_state,(unsigned long long)traveler_local_frames,network_state,network_result,
                    paused,(unsigned long long)game,(unsigned long long)barn);
                message(line);last_wait=now;last_barn=barn;memcpy(previous,snapshot,sizeof(snapshot));
            }
            traveler_local_report(5);return;
        }
        if(!traveler_stream_waiting[1]) {
            uint32_t symbol;memcpy(&symbol,sender.state.bytes+0x64,4);
            traveler_local_outfit=(traveler_local_completed+3)%7;
            traveler_local_symbol=(symbol+1+traveler_local_completed)%21;
            if(traveler_actor) {
                uint32_t extra_symbol;
                if(!own_read(NULL,traveler_object+0x74,&extra_symbol,4)){traveler_local_fail(4);return;}
                while(traveler_local_symbol==symbol || traveler_local_symbol==extra_symbol)
                    traveler_local_symbol=(traveler_local_symbol+1)%21;
            }
        }
    }
    if(traveler_stream_enabled) {
        code=traveler_stream_sample(1,first,UINT64_C(0x4e4154495645),traveler_local_completed+1,
            traveler_local_outfit,traveler_local_symbol,&sender);
        if(code==JC_STREAM_HELD)return;
        if(code==JC_STREAM_TIMEOUT){if(first)traveler_local_fail(30+code);else traveler_local_stop();return;}
        if(code){traveler_local_fail(30+code);return;}
    }
    if(!traveler_stream_enabled)jc_receiver_store(sender.state.bytes,jc_receiver_float(sender.state.bytes)-3.f);
    else {
        memcpy(&traveler_local_outfit,sender.state.bytes+0x60,4);
        memcpy(&traveler_local_symbol,sender.state.bytes+0x64,4);
    }
    struct jc_local_peer_packet packet;
    code=jc_local_peer_packet_make(&sender,first,traveler_local_outfit,traveler_local_symbol,&packet);
    if(!code)code=jc_local_peer_publish(own_read,traveler_local_write,NULL,barn,&packet);
    if(code){if(code>=3)traveler_local_state=2;traveler_local_fail(10+code);return;}
    if(first && traveler_local_owned_barn==barn && layout.gates[0])
        message("[JNET] native admission resumed retained JNET receiver after verified storage and ownership checks");
    traveler_local_owned_barn=barn;
#ifdef JC_NETWORK_UDP
    if(traveler_network_active && !JNETNetwork[23] && JNETNetwork[2]==124)JNETNetwork[2]=0;
#endif
    traveler_local_sent=GetTickCount64();traveler_local_state=2;++traveler_local_frames;traveler_local_report(0);
    if(first) {traveler_local_started=GetTickCount64();traveler_native_arrival(barn+0x146f0,(uint64_t)traveler_local_completed+1);}
    if(first && !traveler_local_completed && !traveler_network_active) {
        code=traveler_repeat_start(1);
        if(code){traveler_local_fail(20+code);}
        message("[JC] local three-traveler trial: native receive feed started; no online transport");
    }
}
static int traveler_local_start(void) {
    if(traveler_local_state)return JC_ACTOR_RUNTIME_BUSY;
    if(JNETTraveler[0]!=3 || !traveler_local_offline() ||
        traveler_repeat.state==1 || (traveler_actor && !traveler_peer_vacant())) {
        traveler_local_state=5;traveler_local_report(JC_ACTOR_RUNTIME_BUSY);return JC_ACTOR_RUNTIME_BUSY;
    }
    traveler_local_state=1;traveler_local_report(0);return 0;
}
static void traveler_local_after_update(void) {
    if(traveler_local_state!=2)return;
    struct jc_layout_result layout;
    if(!jc_native_layout(own_read,NULL,(uintptr_t)GetModuleHandleW(NULL),traveler_game,&layout)) {
        JNETActor[21]=layout.remote_gate;
        static uint64_t visibility_sample;
        if(visibility_sample!=traveler_local_frames &&
           (traveler_local_frames==1 || traveler_local_frames==60 || traveler_local_frames==180)){
            visibility_sample=traveler_local_frames;
            float local[3]={0},remote[3]={0},incoming[3]={0},fade[2]={0};
            unsigned char hidden=0;
            int readable=own_read(NULL,traveler_barn+0xc0,local,sizeof(local)) &&
                own_read(NULL,traveler_barn+0x14700,remote,sizeof(remote)) &&
                own_read(NULL,traveler_barn+0x28d30,incoming,sizeof(incoming)) &&
                own_read(NULL,traveler_barn+0x146f0+0x1e00,fade,4) &&
                own_read(NULL,traveler_barn+0x146f0+0x2000,fade+1,4) &&
                own_read(NULL,traveler_barn+0x146f0+0x56,&hidden,1);
            char line[360];snprintf(line,sizeof(line),"[JNET] native receiver visibility samples=%llu read=%d chapters=%u/%u gate=%u flags=%u/%u/%u blocked=%u hidden=%u fade=%.3g/%.3g local=%.3g,%.3g,%.3g remote=%.3g,%.3g,%.3g incoming=%.3g,%.3g,%.3g",
                (unsigned long long)traveler_local_frames,readable,layout.local_chapter,layout.remote_chapter,layout.remote_gate,
                layout.gates[0],layout.gates[1],layout.gates[2],layout.blocked,hidden,fade[0],fade[1],
                local[0],local[1],local[2],remote[0],remote[1],remote[2],incoming[0],incoming[1],incoming[2]);message(line);
        }
        uint32_t symbols[3]={0};
        int distinct=traveler_actor && own_read(NULL,traveler_barn+0xb0+0x74,symbols,4) &&
            own_read(NULL,traveler_barn+0x146f0+0x74,symbols+1,4) &&
            own_read(NULL,traveler_object+0x74,symbols+2,4) &&
            symbols[0]<=20 && symbols[1]<=20 && symbols[2]<=20 &&
            symbols[0]!=symbols[1] && symbols[0]!=symbols[2] && symbols[1]!=symbols[2];
        JNETActor[23]=distinct;
        if(layout.remote_gate && distinct && traveler_actor->state==JC_ACTOR_RUNTIME_ACTIVE &&
            traveler_actor->update_frame==(uint32_t)JNETFrames)++traveler_local_overlap;
    }
    traveler_local_report(0);
}
static void traveler_local_cleanup(uintptr_t dude) {
    if(traveler_local_state && dude==traveler_barn+0xb0) {
        if(traveler_local_state<4)traveler_local_state=6;
        traveler_local_report((int)JNETActor[17]);
    }
}
#endif
