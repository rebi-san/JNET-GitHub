#include "native_actor_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "actor_code.h"
typedef int32_t LONG;
static unsigned char *image,*game,*barn,*match,*extra,clock_data[0x20];
static uint64_t traveler_game,traveler_barn,traveler_object,ticks;
static struct jc_actor_runtime runtime,*traveler_actor;
static LONG JNETActor[32],JNETTraveler[44],JNETFrames=1;
static struct {int state;unsigned completed;} traveler_repeat;
static unsigned writes,starts,cancels,stops;
static int start_result,context_ok=1,storage_ok=1;
static uint64_t failed_read;
static int own_read(void *ctx,uint64_t address,void *out,size_t bytes) {
    (void)ctx;if(address==failed_read)return 0;memcpy(out,(void *)(uintptr_t)address,bytes);return 1;
}
static int jc_local_windows_write(uint64_t address,const void *data,size_t bytes) {
    assert(address>=traveler_barn && address+bytes<=traveler_barn+JC_RECEIVER_BYTES);
    ++writes;memcpy((void *)(uintptr_t)address,data,bytes);return 1;
}
static int traveler_context(void){return context_ok;}
static int traveler_admission_storage_matches(uintptr_t base){return storage_ok && base==(uintptr_t)image;}
static int traveler_peer_vacant(void){return !traveler_actor || traveler_actor->state==JC_ACTOR_RUNTIME_RETIRED;}
static int traveler_repeat_start(int outfits){assert(outfits==1);++starts;if(!start_result)traveler_repeat.state=1;return start_result;}
static void traveler_repeat_cancel(void){++cancels;traveler_repeat.state=0;}
static void traveler_actor_trial_stop(int cause){assert(!cause);++stops;}
static uint64_t GetTickCount64(void){return ticks;}
static void *GetModuleHandleW(void *p){assert(!p);return image;}
static unsigned arrivals;
static void traveler_native_arrival(uint64_t actor,uint64_t generation){assert(actor==traveler_barn+0x146f0 && generation);++arrivals;}
static void message(const char *p){assert(p && *p);}
#include "traveler_stream_test.h"
static unsigned forced_sample_chapter=32,sample_calls;
static int test_local_sample(unsigned slot,int first,uint64_t peer,uint64_t generation,
        unsigned outfit,unsigned symbol,struct jc_actor_sender *sender) {
    ++sample_calls;
    if(forced_sample_chapter<32){sender->chapter=forced_sample_chapter;return 0;}
    return traveler_stream_sample(slot,first,peer,generation,outfit,symbol,sender);
}
#define traveler_stream_sample test_local_sample
#include "traveler_local_peer.h"
#undef traveler_stream_sample
static void put(void *p,unsigned offset,uint64_t v){memcpy((unsigned char *)p+offset,&v,8);}
static const char *__attribute__((ms_abi)) med_name(uintptr_t controller) {
    assert(controller==traveler_barn+0x146f0+0xa390);return "Med";
}
static void __attribute__((naked,ms_abi)) finish_gates(uintptr_t barn_arg,uintptr_t entry) {
    __asm__("push %rbx\nsub $0x20,%rsp\nmov %rcx,%rbx\njmp *%rdx");
}
static void setup(void) {
    memset(barn,0,JC_RECEIVER_BYTES);memset(match,0,0x11dc0);memset(extra,0,0x100);
    memset(JNETActor,0,sizeof(JNETActor));traveler_local_state=0;traveler_local_completed=0;traveler_local_frames=0;
    traveler_local_owned_barn=0;traveler_network_active=0;storage_ok=1;
    traveler_stream_enabled=0;memset(traveler_streams,0,sizeof(traveler_streams));
    memset(traveler_stream_waiting,0,sizeof(traveler_stream_waiting));
    traveler_local_overlap=0;traveler_actor=NULL;traveler_repeat.state=0;traveler_repeat.completed=0;context_ok=1;start_result=0;
    put(game,0x180,(uintptr_t)match);put(game,0x1e0,(uintptr_t)barn);
    put(game,0x68,(uintptr_t)clock_data);jc_receiver_store(clock_data+0x10,1.f/60);
    put(barn,0,(uintptr_t)image+0x5da340);put(barn,0xb0,(uintptr_t)image+0x5d0380);
    put(barn,0x146f0,(uintptr_t)image+0x5d0380);put(barn,0x68,(uintptr_t)barn+0x146f0);
    uint32_t chapter=2,outfit=6;memcpy(barn+0x14,&chapter,4);memcpy(barn+0x120,&outfit,4);
    barn[0xa3]=barn[0xa4]=1;jc_receiver_store(barn+0x14e0,.1f);jc_receiver_store(barn+0x1f14,1);
    JNETTraveler[0]=3;
}
int main(void) {
    image=mmap(NULL,0x690000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);assert(image!=MAP_FAILED);
    load_code(image);
    memcpy(image+0xaedd0,"\x48\xb8",2);put(image,0xaedd2,(uintptr_t)med_name);memcpy(image+0xaedda,"\xff\xe0",2);
    assert(!mprotect(image,0x690000,PROT_READ|PROT_EXEC));
    game=calloc(1,0x208);barn=calloc(1,JC_RECEIVER_BYTES);match=calloc(1,0x11dc0);extra=calloc(1,0x100);
    assert(game && barn && match && extra);traveler_game=(uintptr_t)game;traveler_barn=(uintptr_t)barn;traveler_object=(uintptr_t)extra;
    setup();traveler_local_state=6;traveler_local_completed=3;
    traveler_local_level_initialized(traveler_barn+0xb0);assert(traveler_local_state==6);
    traveler_local_level_initialized(traveler_barn+0x146f0);assert(!traveler_local_state && !traveler_local_completed);
    setup();
    {
        struct jc_actor_sender sender;
        float position[4]={318,78,102,1},shape[2]={0,.1f};uint32_t appearance[4]={1,9,2,0};
        assert(jc_actor_sender_spawn(position,2,1,shape,appearance,&sender));
        uint32_t own_symbol=13;memcpy(barn+0xb0+0x74,&own_symbol,4);
        for(unsigned next=0;next<21;++next){
            struct jc_local_peer_packet packet;
            assert(!jc_local_peer_packet_make(&sender,0,2,next,&packet));
            assert(packet.count<=40);
            for(unsigned i=0;i<packet.count;++i)assert(packet.rows[i].offset!=0xa1);
            assert(!jc_local_peer_publish(own_read,traveler_local_write,NULL,traveler_barn,&packet));
            uint32_t received,local;memcpy(&received,barn+0x146f0+0x74,4);memcpy(&local,barn+0xb0+0x74,4);
            assert(received==next && local==own_symbol);
        }
        struct jc_local_peer_packet invalid;assert(jc_local_peer_packet_make(&sender,0,2,21,&invalid));
    }
    writes=0;setup();assert(traveler_local_offline() && !traveler_local_guard);
    context_ok=0;assert(!traveler_local_offline() && traveler_local_guard==1);context_ok=1;
    failed_read=traveler_game+0x180;assert(!traveler_local_offline() && traveler_local_guard==2);failed_read=0;
    put(game,0x180,0);assert(!traveler_local_offline() && traveler_local_guard==3);put(game,0x180,(uintptr_t)match);
    failed_read=(uintptr_t)match+0x11db4;assert(!traveler_local_offline() && traveler_local_guard==4);failed_read=0;
    match[0x11db4]=1;assert(!traveler_local_offline() && traveler_local_guard==5);
    setup();match[0x11db4]=1;assert(traveler_local_start()==JC_ACTOR_RUNTIME_BUSY && !writes);
    setup();assert(!traveler_local_start());unsigned before=writes;
    traveler_local_before_input(traveler_barn+1,traveler_game);assert(writes==before && traveler_local_state==1);
    traveler_local_before_input(traveler_barn,traveler_game);
    assert(traveler_local_state==2 && traveler_local_outfit==3 && traveler_local_symbol==1 && starts==1);
    assert(barn[0xa2]==1 && barn[0xa4]==0 && traveler_local_frames==1);
    ++ticks;traveler_local_before_input(traveler_barn,traveler_game);assert(traveler_local_frames==2);
    assert(barn[0xa3]==0 && barn[0xa4]==0);
    finish_gates(traveler_barn,(uintptr_t)image+0x118e57);
    assert(barn[0xa2]==1 && barn[0xa3]==0 && barn[0xa4]==0);
    runtime.state=JC_ACTOR_RUNTIME_ACTIVE;runtime.update_frame=1;traveler_actor=&runtime;uint32_t symbol=2;memcpy(extra+0x74,&symbol,4);
    traveler_local_after_update();assert(traveler_local_overlap==1 && JNETActor[23]==1);
    symbol=1;memcpy(extra+0x74,&symbol,4);traveler_local_after_update();assert(traveler_local_overlap==1 && !JNETActor[23]);
    match[0x11db4]=1;before=writes;traveler_local_before_input(traveler_barn,traveler_game);
    assert(traveler_local_state==5 && JNETActor[17]==1 && writes==before && cancels && stops);
    setup();assert(!traveler_local_start());
    traveler_network_active=traveler_stream_enabled=1;forced_sample_chapter=4;
    barn[0x64]=2;before=writes;
    traveler_local_before_input(traveler_barn,traveler_game);
    assert(traveler_local_state==1 && writes==before);
    forced_sample_chapter=2;traveler_local_before_input(traveler_barn,traveler_game);
    assert(traveler_local_state==2 && writes>before && barn[0xa2]==1 && !barn[0xa3] && !barn[0xa4]);
    forced_sample_chapter=32;
    setup();assert(!traveler_local_start());barn[0xa2]=1;before=writes;traveler_local_before_input(traveler_barn,traveler_game);
    assert(traveler_local_state==1 && writes==before);
    barn[0xa2]=0;traveler_local_before_input(traveler_barn,traveler_game);
    assert(traveler_local_state==2 && traveler_local_frames==1);
    traveler_network_active=1;
    traveler_repeat_cancel();
    for(unsigned cycle=0;cycle<3;++cycle){
        traveler_local_cleanup(traveler_barn+0xb0);
        traveler_local_level_initialized(traveler_barn+0x146f0);
        assert(!traveler_local_state && barn[0xa2]==1);
        assert(!traveler_local_start());
        traveler_local_before_input(traveler_barn,traveler_game);
        assert(traveler_local_state==2 && traveler_local_frames==1 && barn[0xa2]==1);
    }
    traveler_local_level_initialized(traveler_barn+0x146f0);
    assert(!traveler_local_start());
    traveler_stream_enabled=1;forced_sample_chapter=2;
    unsigned samples_before=sample_calls;
    barn[0xa3]=1;before=writes;traveler_local_before_input(traveler_barn,traveler_game);
    assert(sample_calls==samples_before);
    traveler_stream_enabled=0;forced_sample_chapter=32;
    assert(traveler_local_state==1 && writes==before);
    barn[0xa3]=0;match[0x11db4]=1;
    assert(!traveler_local_offline() && !traveler_local_owned_barn);
    match[0x11db4]=0;before=writes;traveler_local_before_input(traveler_barn,traveler_game);
    assert(traveler_local_state==1 && writes==before);
    traveler_local_owned_barn=traveler_barn;barn[0x1670c]=1;
    traveler_local_before_input(traveler_barn,traveler_game);
    assert(traveler_local_state==1 && writes==before);
    barn[0x1670c]=0;barn[0x64]=1;
    traveler_local_before_input(traveler_barn,traveler_game);
    assert(traveler_local_state==1 && writes==before);
    traveler_stream_enabled=1;forced_sample_chapter=4;
    traveler_local_before_input(traveler_barn,traveler_game);
    assert(traveler_local_state==1 && writes==before && barn[0x64]==1);
    forced_sample_chapter=2;
    traveler_local_before_input(traveler_barn,traveler_game);
    assert(traveler_local_state==2 && traveler_local_frames==1 && barn[0x64]==2 && writes>before);
    forced_sample_chapter=32;
    traveler_stream_enabled=0;
    traveler_local_level_initialized(traveler_barn+0x146f0);
    assert(!traveler_local_start());before=writes;
    barn[0x64]=2;storage_ok=0;
    traveler_local_before_input(traveler_barn,traveler_game);
    assert(traveler_local_state==5 && JNETActor[17]==5 && writes==before);
    setup();assert(!traveler_local_start());traveler_local_before_input(traveler_barn,traveler_game);
    traveler_repeat.state=3;traveler_local_before_input(traveler_barn,traveler_game);
    assert(traveler_local_state==3 && barn[0xa2]==0 && barn[0xa3]==1 && barn[0xa4]==1);
    for(unsigned i=0;i<1500 && traveler_local_state==3;++i) {
        ticks+=17;traveler_local_before_input(traveler_barn,traveler_game);
    }
    assert(traveler_local_state==4 && !barn[0xa2] && !barn[0xa4]);
    assert(jc_receiver_float(barn+0x146f0+0x1e00)>.999f);
    setup();assert(!traveler_local_start());traveler_local_cleanup(traveler_barn+0xb0);assert(traveler_local_state==6);
    setup();start_result=43;assert(!traveler_local_start());traveler_local_before_input(traveler_barn,traveler_game);
    assert(traveler_local_state==5 && JNETActor[17]==63 && barn[0xa2]==0);
    setup();assert(!traveler_local_start());traveler_local_before_input(traveler_barn,traveler_game);
    for(unsigned cycle=0;cycle<7;++cycle) {
        assert(traveler_local_state==2 && traveler_local_outfit==(cycle+3)%7);
        traveler_repeat.completed=0;
        traveler_repeat.state=cycle<3?1:2;
        ticks=traveler_local_started+11000;traveler_local_before_input(traveler_barn,traveler_game);
        assert(traveler_local_state==3);barn[0x1670c]=1;
        for(unsigned tick=0;tick<1500 && traveler_local_state==3;++tick) {
            ticks+=17;traveler_local_before_input(traveler_barn,traveler_game);
            if(traveler_local_state==3)finish_gates(traveler_barn,(uintptr_t)image+0x118e57);
        }
        assert(traveler_local_completed==cycle+1);
        if(cycle<6) {
            assert(traveler_local_state==1);
            traveler_local_before_input(traveler_barn,traveler_game);
            assert(jc_receiver_float(barn+0x146f0+0x1e00)==0);
        }
    }
    assert(traveler_local_state==4 && arrivals>=7);
    setup();assert(!traveler_local_start());traveler_local_before_input(traveler_barn,traveler_game);
    traveler_repeat.completed=1;traveler_local_before_input(traveler_barn,traveler_game);
    uint32_t changed_chapter=3;memcpy(barn+0x14,&changed_chapter,4);before=writes;
    traveler_local_before_input(traveler_barn,traveler_game);
    assert(traveler_local_state==6 && writes==before && !JNETActor[17]);
    setup();traveler_stream_enabled=1;
    assert(!traveler_local_start());
    jc_receiver_store(barn+0xc0,NAN);before=writes;
    traveler_local_before_input(traveler_barn,traveler_game);
    assert(traveler_local_state==1 && JNETActor[17]==4 && writes==before);
    jc_receiver_store(barn+0xc0,0);
    jc_receiver_store(barn+0xcc,NAN);jc_receiver_store(barn+0xdc,INFINITY);
    traveler_local_before_input(traveler_barn,traveler_game);
    assert(traveler_local_state==2 && traveler_streams[1].stream.sequence==1);
    uint64_t source_started=ticks;
    for(unsigned i=0;i<1000 && traveler_local_state==2;++i) {
        ticks+=17;traveler_local_before_input(traveler_barn,traveler_game);
    }
    assert(traveler_local_state==3 && ticks-source_started<11000);
    assert(traveler_streams[1].stream.departing && traveler_streams[1].stream.rejected);
    for(unsigned i=0;i<1500 && traveler_local_state==3;++i) {
        ticks+=17;traveler_local_before_input(traveler_barn,traveler_game);
    }
    assert(traveler_local_state==1);traveler_local_before_input(traveler_barn,traveler_game);
    assert(traveler_local_state==2 && traveler_streams[1].stream.generation==2);
    setup();assert(!traveler_local_start());
    failed_read=traveler_barn+0x14f8;
    traveler_local_before_input(traveler_barn,traveler_game);
    assert(traveler_local_state==5 && JNETActor[17]==4);
    failed_read=0;traveler_stream_enabled=0;
    free(extra);free(match);free(barn);free(game);munmap(image,0x690000);
    puts("Local 3 adapter: seven independent cycles while the other slot stays active or finishes, outfit changes, arrival reset, departure with hidden native gate, independent timing and chapter interruption passed. Meditation controller stubbed; animation/render/GPU are not exercised.");
}
