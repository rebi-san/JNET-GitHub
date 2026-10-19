#include "jnet_menu_draw.h"
#include "menu_code.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

extern void native_glyph_packet(void *,const struct jnet_menu_glyph *,void *,
                                struct jnet_menu_quad *,uint32_t,float,float,float);
__asm__(".text\n"
        "_native_glyph_packet:\n"
        "pushq %rbp\n pushq %r12\n pushq %r14\n subq $512,%rsp\n"
        "movq %rcx,480(%rsp)\n movq %rsi,%rax\n movq %rdx,%r12\n"
        "movl %r8d,%r14d\n movaps %xmm0,%xmm8\n movaps %xmm1,%xmm10\n"
        "movaps %xmm2,%xmm11\n movl $0x3f7ff972,%eax\n movd %eax,%xmm9\n movq %rsi,%rax\n"
        "leaq 248(%rsp),%rbp\n callq *%rdi\n"
        "movq 480(%rsp),%rdi\n leaq 104(%rsp),%rsi\n movl $100,%ecx\n rep movsb\n"
        "addq $512,%rsp\n popq %r14\n popq %r12\n popq %rbp\n retq\n");

static void check_text(unsigned char *image) {
    unsigned char menu[0xbb4]={0};
    struct jnet_menu_glyph glyphs[]={
        {.1f,.2f,.3f,.4f,-.02f,.03f,.15f,.2f,.17f},
        {0,0,0,0,0,0,0,0,.06f},
        {.3f,.6f,.5f,.4f,.01f,-.02f,.12f,.19f,.14f}
    };
    struct jnet_menu_text text,saved;
    for(unsigned pass=0;pass<3;++pass) {
        float scale=pass==0?.5f:pass==1?1.f:2.f,x=-.6f;
        assert(jnet_menu_text_layout(&text,glyphs,3,x,.1f,scale,0xabcdef12));
        for(unsigned i=0;i<3;++i) {
            struct jnet_menu_quad native;
            native_glyph_packet(image+0x1042b5,&glyphs[i],menu,&native,0xabcdef12,scale,x,.1f);
            assert(!memcmp(&native,&text.quads[i],sizeof(native)));
            x+=scale*glyphs[i].advance;
        }
        assert(text.count==3 && text.next_x==x);
    }
    saved=text;
    glyphs[2].advance=NAN;
    assert(!jnet_menu_text_layout(&text,glyphs,3,0,0,1,0));
    assert(!memcmp(&text,&saved,sizeof(text)));
    assert(!jnet_menu_text_layout(&text,glyphs,JNET_MENU_TEXT_LIMIT+1,0,0,1,0));
    assert(!jnet_menu_text_layout(&text,NULL,1,0,0,1,0));
    assert(!jnet_menu_text_layout(&text,glyphs,1,0,0,INFINITY,0));
    glyphs[0].width=3.4e38f;
    assert(!jnet_menu_text_layout(&text,glyphs,1,0,0,2,0));
    assert(!memcmp(&text,&saved,sizeof(text)));
    assert(jnet_menu_text_layout(&text,NULL,0,.25f,0,1,0));
    assert(text.count==0 && text.next_x==.25f);
    puts("JNET text layout: glyph packets match original arithmetic at three scales, including spacing and offsets; invalid runs leave output unchanged.");
}

static void put32(unsigned char *p,size_t at,uint32_t v){memcpy(p+at,&v,4);}
static uint32_t get32(const unsigned char *p,size_t at){uint32_t v;memcpy(&v,p+at,4);return v;}
static float getf(const unsigned char *p,size_t at){float v;memcpy(&v,p+at,4);return v;}
static void (__attribute__((ms_abi)) *native_append)(uintptr_t,const struct jnet_menu_quad *);
static void __attribute__((ms_abi)) checked_append(uintptr_t batch,const struct jnet_menu_quad *quad) {
    assert(((uintptr_t)quad&15u)==0);
    native_append(batch,quad);
}
int main(void) {
    const unsigned sizes[][2]={{1920,1080},{2560,1080},{1280,1024},{1080,1920}};
    const float bounds[4]={.58f,.51f,.86f,.25f};
    const float sources[][2]={{256,128},{32,32},{1024,49},{1024.f/30,40}};
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j) {
        float fitted[4];
        assert(jnet_menu_image_fit(fitted,bounds,sources[j][0],sources[j][1],sizes[i][0],sizes[i][1]));
        float ratio=(fitted[2]-fitted[0])*sizes[i][0]/((fitted[1]-fitted[3])*sizes[i][1]);
        assert(fabsf(ratio-sources[j][0]/sources[j][1])<.001f);
        assert(fitted[0]>=bounds[0]-.00001f && fitted[2]<=bounds[2]+.00001f);
        assert(fitted[1]<=bounds[1]+.00001f && fitted[3]>=bounds[3]-.00001f);
    }
    puts("Image fitting: source proportions preserved within bounds at four viewport shapes, including cropped scarf.");

    unsigned char *image=mmap(NULL,0x3770000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
    assert(image!=MAP_FAILED);load_menu_code(image);
    assert(!mprotect(image,0x110000,PROT_READ|PROT_EXEC));
    check_text(image);
    native_append=(void *)(image+0x1024a0);
    unsigned char batch[0x90]={0},storage[4*112+32],before[sizeof(storage)];
    uintptr_t mapped=(uintptr_t)(storage+16);memcpy(batch+0x88,&mapped,8);
    struct jnet_menu_quad quad;
    assert(jnet_menu_quad_make(&quad,-.5f,-.25f,.5f,.25f,.1f,.2f,.8f,.9f,0xaabbccddu));
    _Alignas(16) struct jnet_menu_quad inputs[4];
    for(unsigned n=0;n<4;++n) {
        inputs[n]=quad;
        assert(((uintptr_t)&inputs[n]&15u)==n*4u);
    }
    for(unsigned aspect=0;aspect<3;++aspect) {
        uint32_t width=aspect==0?1920:aspect==1?1024:3440,height=aspect==1?768:1440;
        put32(image,0x3763e74,width);put32(image,0x3763e78,height);
        memset(storage,0x5a,sizeof(storage));put32(batch,0x80,4);put32(batch,0x84,0);
        for(unsigned n=0;n<4;++n) {
            assert(jnet_menu_batch_room(4,n,(void *)mapped,4*112,width,height,1));
            jnet_menu_append_aligned(checked_append,(uintptr_t)batch,&inputs[n]);
            assert(get32(batch,0x84)==n+1);
            for(unsigned v=0;v<4;++v) {
                const unsigned char *vertex=storage+16+n*112+v*28;
                float ratio=(float)height/(float)width;
                assert(fabsf(getf(vertex,0)-quad.position[v][0]*(1.f/ratio))<.00001f);
                assert(getf(vertex,4)==quad.position[v][1] && getf(vertex,8)==0);
                assert(getf(vertex,12)==((v==1 || v==2)? .8f:.1f));
                assert(getf(vertex,16)==(v>=2?.9f:.2f) && getf(vertex,20)==1);
                assert(get32(vertex,24)==0xaabbccddu);
            }
        }
        memcpy(before,storage,sizeof(storage));
        assert(!jnet_menu_batch_room(4,4,(void *)mapped,4*112,width,height,1));
        jnet_menu_append_aligned(checked_append,(uintptr_t)batch,&quad);
        assert(get32(batch,0x84)==4 && !memcmp(before,storage,sizeof(storage)));
        for(unsigned i=0;i<16;++i)assert(storage[i]==0x5a && storage[sizeof(storage)-1-i]==0x5a);
    }
    struct jnet_menu_quad saved=quad;
    assert(!jnet_menu_quad_make(&quad,NAN,0,1,1,0,0,1,1,0) && !memcmp(&saved,&quad,sizeof(quad)));
    assert(!jnet_menu_quad_make(&quad,0,0,1,1,0,0,2,1,0));
    assert(!jnet_menu_batch_room(4,0,NULL,448,1920,1080,1));
    assert(!jnet_menu_batch_room(4,0,(void *)mapped,111,1920,1080,1));
    assert(!jnet_menu_batch_room(4,0,(void *)mapped,448,0,1080,1));
    assert(!jnet_menu_batch_room(UINT32_MAX,0,(void *)mapped,SIZE_MAX,1920,1080,1));
    assert(!jnet_menu_batch_room(4,5,(void *)mapped,448,1920,1080,1));
    assert(!munmap(image,0x3770000));
    puts("JNET native menu quad: aligned native calls from all four input alignments, original append instructions, three aspect ratios, positions/UV/color, full-buffer refusal and canaries passed. No GPU or Journey process used.");
}
