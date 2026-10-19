#include "native_layout.h"
#include <assert.h>
#include <stdio.h>
static unsigned char game[0x200], barn[0x28dd0];
static int reads, mutate, deny_remote;
static const uint64_t image = UINT64_C(0x140000000), game_address = 0x100000, barn_address = 0x200000;
static void put64(unsigned char *p, uint64_t v) { memcpy(p, &v, 8); }
static void put32(unsigned char *p, uint32_t v) { memcpy(p, &v, 4); }
static int reader(void *unused, uint64_t address, void *out, size_t size) {
    (void)unused;
    ++reads;
    if (mutate && reads == 10) put64(game + 0x1e0, 0x300000);
    if (deny_remote && address == barn_address + 0x14700) return 0;
    if (address >= game_address && address - game_address <= sizeof(game) && size <= sizeof(game) - (address-game_address)) {
        memcpy(out, game + (size_t)(address-game_address), size); return 1;
    }
    if (address >= barn_address && address - barn_address <= sizeof(barn) && size <= sizeof(barn) - (address-barn_address)) {
        memcpy(out, barn + (size_t)(address-barn_address), size); return 1;
    }
    return 0;
}
static void fixture(void) {
    memset(game,0,sizeof(game)); memset(barn,0,sizeof(barn));
    reads=mutate=deny_remote=0;
    put64(game+0x1e0,barn_address); put64(barn,image+0x5da340);
    put64(barn+0xb0,image+0x5d0380); put64(barn+0x146f0,image+0x5d0380);
    put32(barn+0x14,2);put32(barn+0x64,2);
}
int main(void) {
    struct jc_layout_result result;
    fixture(); deny_remote=1;
    assert(jc_native_layout(reader,NULL,image,game_address,&result)==JC_LAYOUT_OK && !result.remote_gate);
    fixture(); barn[0xa4]=1;
    assert(jc_native_layout(reader,NULL,image,game_address,&result)==JC_LAYOUT_OK && result.remote_gate && result.local_position_finite && result.remote_position_finite);
    fixture(); barn[0xa2]=1; barn[0x1670c]=1;
    assert(jc_native_layout(reader,NULL,image,game_address,&result)==JC_LAYOUT_OK && !result.remote_gate);
    fixture(); barn[0xa2]=1; put32(barn+0x64,3);
    assert(jc_native_layout(reader,NULL,image,game_address,&result)==JC_LAYOUT_OK && !result.remote_gate);
    fixture(); put64(barn+0x146f0,0);
    assert(jc_native_layout(reader,NULL,image,game_address,&result)==JC_LAYOUT_TYPE);
    fixture(); put64(game+0x1e0,0);
    assert(jc_native_layout(reader,NULL,image,game_address,&result)==JC_LAYOUT_READ);
    fixture(); mutate=1;
    assert(jc_native_layout(reader,NULL,image,game_address,&result)==JC_LAYOUT_CHANGED);
    fixture(); barn[0xa4]=1;deny_remote=1;
    assert(jc_native_layout(reader,NULL,image,game_address,&result)==JC_LAYOUT_READ);
    fixture();put32(barn+0xc0,0x7fc00000);
    assert(jc_native_layout(reader,NULL,image,game_address,&result)==JC_LAYOUT_OK && !result.local_position_finite);
    uint64_t out;
    assert(!jc_layout_address(65536,UINT64_MAX,&out));
    assert(!jc_layout_address(UINT64_MAX,16,&out));
    assert(jc_native_layout(reader,NULL,UINT64_MAX,game_address,&result)==JC_LAYOUT_RANGE);
    puts("Native layout reader: 12 synthetic checks passed; no live game validation.");
    return 0;
}
