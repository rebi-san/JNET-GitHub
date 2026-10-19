#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
static unsigned char game_data[0x100],slides_data[0x3550],input_data[16],pad_data[0x270],channel_data[0x100];
static uintptr_t JNETGame=(uintptr_t)game_data,fault_image=0x140000000ULL;
static unsigned pressed,calls;
static int input_focus=1;
static int jnet_intro_input_down(void){return input_focus?(int)pressed:-1;}
static int own_read(void *ctx,uintptr_t p,void *out,size_t n){
    (void)ctx;
    if(p==fault_image+0x3c9420d && n==1){*(unsigned char *)out=0;return 1;}
    memcpy(out,(void *)p,n);return 1;
}
static int jc_local_windows_write(uintptr_t p,const void *in,size_t n){memcpy((void *)p,in,n);return 1;}
static void message(const char *text){(void)text;}
#include "jnet_intro_live.h"
static void original(uintptr_t slides,float dt,uintptr_t resources){
    assert(slides==(uintptr_t)slides_data && dt>0 && resources==42);++calls;
    float elapsed;uint32_t index,count;
    memcpy(&elapsed,slides_data+0x3538,4);memcpy(&index,slides_data+0x353c,4);memcpy(&count,slides_data+0x3530,4);
    if(elapsed>100 && index<count){
        /* Native completion waits for the next slide only when one remains. */
        if(index+1<count)return;
        ++index;memcpy(slides_data+0x353c,&index,4);
        elapsed=0;memcpy(slides_data+0x3538,&elapsed,4);
    }
}
static void frame(unsigned key){pressed=key;channel_data[0x0d]=(unsigned char)key;jnet_intro_update((uintptr_t)slides_data,.016f,42);}
int main(void){
    uintptr_t ptr=(uintptr_t)slides_data;memcpy(game_data+0xa8,&ptr,8);
    ptr=(uintptr_t)input_data;memcpy(game_data+0x40,&ptr,8);ptr=(uintptr_t)pad_data;memcpy(input_data+8,&ptr,8);
    ptr=(uintptr_t)channel_data;memcpy(pad_data+0x260,&ptr,8);
    uint32_t count=5;memcpy(slides_data+0x3530,&count,4);jnet_intro_update_original=original;
    frame(1);assert(!slides_data[0x353c]);
    frame(1);assert(!slides_data[0x353c]);
    frame(0);assert(!slides_data[0x353c]);
    frame(1);assert(!slides_data[0x353c]);
    frame(1);assert(!slides_data[0x353c]);
    input_focus=0;frame(0);input_focus=1;frame(1);
    assert(!slides_data[0x353c]);
    frame(0);frame(1);assert(!slides_data[0x353c]);
    frame(0);frame(1);assert(slides_data[0x353c]==1);
    assert(slides_data[0x3530]==1);
    unsigned completed_calls=calls;
    frame(0);frame(1);assert(slides_data[0x353c]==1 && calls==completed_calls+2);
    /* A later logo completes in one update even with the next image unavailable. */
    count=5;memcpy(slides_data+0x3530,&count,4);
    uint32_t index=3;memcpy(slides_data+0x353c,&index,4);
    frame(0);frame(1);assert(slides_data[0x353c]==3);
    frame(0);completed_calls=calls;frame(1);
    assert(slides_data[0x353c]==4 && slides_data[0x3530]==4 && calls==completed_calls+1);
    /* A different slideshow must not receive startup skipping. */
    uintptr_t other=0;memcpy(game_data+0xa8,&other,8);
    count=5;index=0;memcpy(slides_data+0x3530,&count,4);memcpy(slides_data+0x353c,&index,4);
    frame(0);frame(1);frame(0);frame(1);
    assert(slides_data[0x353c]==0 && slides_data[0x3530]==5);
    for(count=1;count<=16;++count)for(index=0;index<count;++index){
        memcpy(game_data+0xa8,&other,8);frame(0);
        memcpy(slides_data+0x3530,&count,4);memcpy(slides_data+0x353c,&index,4);
        float elapsed=0;memcpy(slides_data+0x3538,&elapsed,4);
        ptr=(uintptr_t)slides_data;memcpy(game_data+0xa8,&ptr,8);
        frame(0);frame(1);frame(0);completed_calls=calls;frame(1);
        uint32_t end;memcpy(&end,slides_data+0x353c,4);
        assert(end==index+1 && calls==completed_calls+1);
        memcpy(&end,slides_data+0x3530,4);assert(end==index+1);
    }
    puts("Intro: two separate inputs, held-input refusal, focus reset, single-update completion, unavailable next image and unrelated slideshow guards passed; native rendering not exercised.");
}
