#define JNET_LOBBY_TEST_MAIN lobby_fixture_main
#include "test_jnet_steam_lobby.c"
#undef main
#define JNET_ROCKET_NETWORK_FIXTURE
#include "jnet_rocket_live.h"
int main(void) {
    setup();clock_ms=10000;jnet_sl.room=1000;jnet_sl.host=jnet_sl.identity=42;jnet_sl.generation=1;
    jnet_travel_epoch=3;jnet_current_level=1;jnet_sn_fast_poll=1;
    jnet_sn_members.count=3;jnet_sn_members.ids[0]=42;jnet_sn_members.ids[1]=43;jnet_sn_members.ids[2]=44;
    jnet_sn_api.self=(void *)1;jnet_sn_api.send=send_message;
    for(unsigned i=0;i<2;++i){
        jnet_sn.peers[i].identity=43+i;jnet_sn.peers[i].admitted=jnet_sn.peers[i].connected=1;jnet_sn.peers[i].handle=100+i;
        jnet_sg.peers[i].id=43+i;jnet_sg.peers[i].lease=20+i;jnet_sg.peers[i].character_known=1;
        jnet_sg.peers[i].character=JNET_CHARACTER_ROCKET;jnet_sg.peers[i].sample.chapter=1;
    }
    jnet_rocket_poll_live();
    struct jnet_rocket_shot s={.position={1,2,3,1},.velocity={10,0,0,0},.source=43,.lease=7,.sequence=1,.epoch=3,.chapter=1};
    unsigned char p[128];jnet_rocket_pack(p,&s);
    assert(jnet_rocket_message(0,p,128));assert(jnet_rockets.in_count==1 && rocket_sends==1);
    assert(jnet_rockets.incoming[0].lease==20 && jc_packet_get(rocket_sent+56,8)==20);
    assert(jnet_rocket_message(0,p,128) && jnet_rockets.in_count==1 && rocket_sends==1);
    clock_ms+=300;s.source=44;s.sequence=2;jnet_rocket_pack(p,&s);
    assert(jnet_rocket_message(0,p,128) && jnet_rockets.in_count==1);
    assert(jnet_rocket_message(1,p,128) && jnet_rockets.in_count==2 && rocket_sends==2);
    s.sequence=3;s.epoch=4;jnet_rocket_pack(p,&s);assert(jnet_rocket_message(1,p,128) && jnet_rockets.in_count==2);
    s.epoch=3;s.velocity[0]=NAN;jnet_rocket_pack(p,&s);assert(jnet_rocket_message(1,p,128) && jnet_rockets.in_count==2);
    s.velocity[0]=10;s.sequence=4;clock_ms+=300;jnet_rocket_pack(p,&s);p[16]^=1;
    assert(jnet_rocket_message(1,p,128) && jnet_rockets.in_count==2);p[16]^=1;
    jnet_sg.peers[1].character=JNET_CHARACTER_WHALE;assert(jnet_rocket_message(1,p,128) && jnet_rockets.in_count==2);
    jnet_sl.identity=43;jnet_sn.peers[0].identity=42;s.source=44;s.lease=21;s.sequence=5;jnet_rocket_pack(p,&s);clock_ms+=300;
    assert(jnet_rocket_message(0,p,128) && jnet_rockets.in_count==3 && rocket_sends==2);
    s.sequence=6;jnet_rocket_pack(p,&s);assert(jnet_rocket_message(1,p,128) && jnet_rockets.in_count==3);
    jnet_sl.generation++;jnet_rocket_poll_live();assert(!jnet_rockets.in_count && !jnet_rockets.out_count);
    struct jnet_game_record r=game_record(43,1),decoded={0};
    r.id=43;r.lease=1;r.sequence=1;r.seen=clock_ms;r.character_known=1;r.character=JNET_CHARACTER_WAR_MACHINE;
    r.spotlight=1;r.rotation_known=1;r.rotation[3]=1;
    r.sample.body_scale=1;r.sample.chapter=1;
    unsigned char record[JNET_GAME_RECORD];jnet_sg_encode_record(record,&r,clock_ms);
    assert(jc_packet_get(record+252,4)==0x303 && record[271]==0x3f);
    assert(jnet_sg_decode_record(&decoded,record,clock_ms) && decoded.spotlight && decoded.rotation_known && decoded.rotation[3]==1);
    r.rotation[3]=2;jnet_sg_encode_record(record,&r,clock_ms);assert(!jnet_sg_decode_record(&decoded,record,clock_ms));
    r.rotation[3]=NAN;jnet_sg_encode_record(record,&r,clock_ms);assert(!jnet_sg_decode_record(&decoded,record,clock_ms));
    r.rotation_known=0;r.character=JNET_CHARACTER_ROCKET;jnet_sg_encode_record(record,&r,clock_ms);
    assert(jnet_sg_decode_record(&decoded,record,clock_ms) && decoded.character==JNET_CHARACTER_ROCKET && !decoded.spotlight);
    record[252]=255;assert(!jnet_sg_decode_record(&decoded,record,clock_ms));
    (void)jnet_rocket_last_shot;
    puts("Rocket relay: three identities, host fanout, source binding, replay, epoch, nonce, finite data, character gates and session reset passed; fixture only.");
    return 0;
}
