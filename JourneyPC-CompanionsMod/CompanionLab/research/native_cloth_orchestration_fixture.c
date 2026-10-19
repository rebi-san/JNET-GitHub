
#define main cloth_header_main
#include "../diagnostic/test_native_companion_cloth.c"
#undef main
#define native_cloth_lease lease
struct callback_read_budget { uint64_t deadline; unsigned reads,fast; int expired; struct jc_read_batch *batch; };
static int JCProbeCompanionFeatures[40];
static uintptr_t JCProbeGame=GAME;
static unsigned resource_thread=7,current_thread=7;
typedef uint32_t DWORD;
static DWORD cloth_production_thread;
static unsigned cloth_production_pass;
static int native_cloth_disabled,published,finish_count,bad_finish_return,before_count,forward_count;
static unsigned GetCurrentThreadId(void) {return current_thread;}
static uint64_t GetTickCount64(void) {return 1000;}
static void *GetModuleHandleW(void *unused) {(void)unused;return 0;}
static void flight_mark(int a,int b) {(void)a;(void)b;}
static int own_read(void *c,uint64_t at,void *out,size_t bytes) {return read_memory(c,at,out,bytes);}
#define callback_read read_memory
#define companion_cloth_write write_memory
#define companion_cloth_call native_call
static void companion_cloth_publish(int code) {published=code;}
static uintptr_t fake_finish(uintptr_t at,void *out,unsigned pass,uintptr_t spare) {
    assert(at==BARN && pass==1 && spare==0);++finish_count;
    if(bad_finish_return)return 0;
    unsigned char *result=out;memset(result,0,16);result[0]=finish_count>=2;
    if(result[0]){barn[0x6bcf84]=0;job[0x58]=0;}
    return (uintptr_t)out;
}
#define JC_RVA_ClothFinish ((uintptr_t)fake_finish)
static void companion_cloth_before(uintptr_t at) {assert(at==BARN);++before_count;}
static uintptr_t fake_update(uintptr_t at,float dt,uintptr_t a3,uintptr_t a4,uintptr_t a5,uintptr_t a6,
    uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10,uintptr_t a11,uintptr_t a12,uintptr_t a13,uintptr_t a14) {
    assert(at==BARN && dt==1.25f && before_count==1);const uintptr_t args[]={a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14};
    for(unsigned i=0;i<12;++i)assert(args[i]==0x100+i);++forward_count;return 0x123456789;
}
static uintptr_t (*original_cloth_update)(uintptr_t,float,uintptr_t,uintptr_t,uintptr_t,uintptr_t,
    uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t)=fake_update;
static int companion_cloth_finish(uintptr_t barn) {
    unsigned char pending=0;
    if (!own_read(NULL,barn+0x6bcf84,&pending,1) || pending>1) return 13;
    if (pending) {
        _Alignas(16) unsigned char completed[16]={0};
        uintptr_t (*finish)(uintptr_t,void *,unsigned,uintptr_t)=
            (void *)((uintptr_t)GetModuleHandleW(NULL)+JC_RVA_ClothFinish);
        do {
            flight_mark(47,1);
            if (finish(barn,completed,1,0)!=(uintptr_t)completed) return 13;
            flight_mark(48,1);
        } while (!completed[0]);
    }
    return 0;
}
static void companion_cloth_cleanup_locked(uintptr_t dude) {
    if (native_cloth_lease.state!=JC_CLOTH_LIVE || dude!=native_cloth_lease.players+0xb0) return;
    uint64_t game=(uintptr_t)JCProbeGame,barn=0,players=0;
    if (GetCurrentThreadId()!=resource_thread || game!=native_cloth_lease.game ||
        !own_read(NULL,game+0x108,&barn,8) || barn!=native_cloth_lease.barn ||
        !own_read(NULL,game+0x1e0,&players,8) || players!=native_cloth_lease.players) {
        companion_cloth_publish(11); return;
    }
    int code=companion_cloth_finish((uintptr_t)barn);
    if (!code) {
        struct callback_read_budget budget={.deadline=GetTickCount64()+500,.fast=JCProbeCompanionFeatures[8]==1};
        code=jc_companion_cloth_retire(callback_read,companion_cloth_write,companion_cloth_call,&budget,
            game,1,1,&native_cloth_lease);
    }
    native_cloth_disabled=1; companion_cloth_publish(code);
}
static uintptr_t companion_cloth_update(uintptr_t barn,float dt,uintptr_t a3,uintptr_t a4,
        uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10,
        uintptr_t a11,uintptr_t a12,uintptr_t a13,uintptr_t a14) {
    companion_cloth_before(barn);
    DWORD old_thread=cloth_production_thread; unsigned old_pass=cloth_production_pass;
    cloth_production_thread=GetCurrentThreadId(); cloth_production_pass=0;
    uintptr_t result=original_cloth_update(barn,dt,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14);
    cloth_production_thread=old_thread; cloth_production_pass=old_pass;
    return result;
}

int main(void) {
    cloth_header_main();
    reset();finish_count=0;assert(!companion_cloth_finish(BARN) && !finish_count);
    barn[0x6bcf84]=1;job[0x58]=1;assert(!companion_cloth_finish(BARN) && finish_count==2 && !barn[0x6bcf84]);
    reset();finish_count=0;barn[0x6bcf84]=2;assert(companion_cloth_finish(BARN)==13 && !finish_count);
    reset();barn[0x6bcf84]=1;bad_finish_return=1;assert(companion_cloth_finish(BARN)==13);bad_finish_return=0;
    reset();assert(!step(1,1));lease.generation=1;barn[0x6bcf84]=1;job[0x58]=1;finish_count=0;
    native_cloth_disabled=0;published=-99;companion_cloth_cleanup_locked(PLAYERS+0xb0);
    assert(finish_count==2 && calls[JC_CLOTH_RELEASE]==1 && lease.state==JC_CLOTH_RETIRED && native_cloth_disabled && !published);
    reset();assert(!step(1,1));lease.generation=1;current_thread=8;published=0;
    companion_cloth_cleanup_locked(PLAYERS+0xb0);assert(published==11 && !calls[JC_CLOTH_RELEASE] && lease.state==JC_CLOTH_LIVE);current_thread=7;
    reset();assert(!step(1,1));lease.generation=1;companion_cloth_cleanup_locked(PLAYERS+0x146f0);assert(!calls[JC_CLOTH_RELEASE]);
    before_count=forward_count=0;assert(companion_cloth_update(BARN,1.25f,0x100,0x101,0x102,0x103,0x104,0x105,0x106,0x107,0x108,0x109,0x10a,0x10b)==0x123456789);
    assert(before_count==1 && forward_count==1);
    puts("native cloth orchestration: 8 extracted-production finish, cleanup and 14-argument forwarding scenarios passed");
}
