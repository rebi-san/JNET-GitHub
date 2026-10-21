#include "native_actor_runtime.h"
#include "native_cloth_material_live.h"
#include "native_local_peer.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "actor_code.h"
#define ABI __attribute__((ms_abi))
struct event { uint64_t kind,a,b; };
static struct event events[32];
static unsigned count;
static uint32_t uniforms[4];
static float hsv_uniforms[3];
static uintptr_t hsv_payload;
static uintptr_t payloads[4];
static unsigned char *image;
static float captured_axes[4],captured_direction[4];
static uintptr_t captured_actor,captured_game,captured_local;
static unsigned captured_gate,input_calls;
static unsigned update_calls;
static unsigned bone_calls;
static void put(unsigned char *p,unsigned off,uintptr_t value);
static void message(const char *line) {assert(line && *line);}
static void traveler_actor_chirp_tick(void) {}
extern void reference_world(void *,const void *,void *);
__asm__(".text\n.globl _reference_world\n_reference_world:\n"
    "subq $72,%rsp\nmovups (%rsi),%xmm0\nmovups %xmm0,24(%rsp)\n"
    "xorps %xmm7,%xmm7\nmovl $1065353216,%eax\nmovd %eax,%xmm8\n"
    "callq *%rdx\naddq $72,%rsp\nretq\n");
extern unsigned reference_cloth_gate(void *,void *,unsigned,void *);
__asm__(".text\n.globl _reference_cloth_gate\n_reference_cloth_gate:\n"
    "pushq %rbx\npushq %r12\npushq %r14\npushq %rdi\nsubq $8,%rsp\n"
    "movq %rdi,%rbx\nmovq %rsi,%rdi\nmovl %edx,%r14d\nxorl %r12d,%r12d\n"
    "callq *%rcx\naddq $8,%rsp\npopq %rdi\npopq %r14\npopq %r12\npopq %rbx\nretq\n");
extern void reference_scale(void *,const float *,float *,void *);
__asm__(".text\n.globl _reference_scale\n_reference_scale:\n"
    "pushq %rbx\npushq %rbp\nsubq $160,%rsp\n"
    "movq %rdi,%rbx\nmovq %rcx,%r11\nmovq %rsi,%rcx\n"
    "leaq 88(%rsp),%rbp\nleaq -48(%rbp),%r8\n"
    "xorps %xmm0,%xmm0\nxorps %xmm1,%xmm1\nxorps %xmm12,%xmm12\n"
    "callq *%r11\naddq $160,%rsp\npopq %rbp\npopq %rbx\nretq\n");
static void scale_cases(void) {
    unsigned char actor[0x1440]={0};
    const float world[16]={0,0,1,0,0,1,0,0,-1,0,0,0,320,60,190,1};
    float output[16];
    for (unsigned n=0;n<3;++n) {
        float scale=n==0?0:n==1?1:1.25f;
        memcpy(actor+0x1430,&scale,4);
        reference_scale(actor,world,output,image+0xa7bb6);
        for(unsigned i=0;i<16;++i) assert(output[i]==(i<12?world[i]*scale:world[i]));
        double determinant=(double)output[0]*(output[5]*output[10]-output[6]*output[9])
            -(double)output[1]*(output[4]*output[10]-output[6]*output[8])
            +(double)output[2]*(output[4]*output[9]-output[5]*output[8]);
        assert(fabs(determinant-scale*scale*scale)<1e-8);
    }
    puts("Original body-scale transform: zero collapses all basis axes; admitted scales preserve full rank and world position.");
}
static _Alignas(16) float end_bone[16]={0,0,0,0,0,0,0,0,0,0,0,0,4,5,6,0};
static uintptr_t ABI end_bone_call(uintptr_t mesh,uintptr_t scratch,uintptr_t name) {
    assert(mesh && scratch && name);++bone_calls;return (uintptr_t)end_bone;
}
/* Fixture-only entry into the original numeric remote branch. The original
   local-player dispatcher is deliberately skipped; a54a0 is intercepted. */
extern void reference_input(void *,void *,void *,unsigned,void *);
__asm__(".text\n.globl _reference_input\n_reference_input:\n"
    "pushq %rbx\npushq %rbp\npushq %r12\npushq %r13\npushq %r14\npushq %r15\n"
    "subq $0x200,%rsp\nmovq %rdi,%rbx\nmovq %rsi,%rdi\nmovq %rdx,%r15\n"
    "movl %ecx,%r14d\nxorl %r12d,%r12d\nxorps %xmm11,%xmm11\n"
    "leaq 0x91(%rsp),%rbp\ncallq *%r8\naddq $0x200,%rsp\n"
    "popq %r15\npopq %r14\npopq %r13\npopq %r12\npopq %rbp\npopq %rbx\nretq\n");
static void ABI input_call(uintptr_t actor,uintptr_t game,const float *axes,float *direction,
        uintptr_t local,unsigned char gate) {
    ++input_calls;captured_actor=actor;captured_game=game;captured_local=local;captured_gate=gate;
    memcpy(captured_axes,axes,16);memcpy(captured_direction,direction,16);
}
static void ABI update_call(uintptr_t actor,uintptr_t game,const float *direction,uintptr_t fourth,
        unsigned char gate) {
    assert(!fourth);++update_calls;captured_actor=actor;captured_game=game;captured_gate=gate;
    memcpy(captured_direction,direction,16);
    const float position[4]={4,5,6,1};
    reference_world((void *)actor,position,image+0xa9c8e);
}
static void push(uint64_t k,uint64_t a,uint64_t b) { assert(count<32);events[count++]=(struct event){k,a,b}; }
static uintptr_t ABI lookup(uintptr_t a,uintptr_t b) {
    if (a==hsv_payload) {
        const unsigned names[3]={0x65bb5c,0x65bbf0,0x65bc00};
        for (unsigned i=0;i<3;++i) if (b==(uintptr_t)image+names[i]) return (uintptr_t)(hsv_uniforms+i);
        abort();
    }
    assert(b==(uintptr_t)image+0x5dfaf8);push(1,a,b);
    for(unsigned i=0;i<4;++i)if(a==payloads[i])return (uintptr_t)&uniforms[i];
    abort();
}
static void ABI commit(uintptr_t a) { push(2,a,0); }
static void ABI mesh(uintptr_t a,uintptr_t b) { push(3,a,b); }
static void hsv_cases(void) {
    unsigned char *actor=calloc(1,0x9b48),*collection=calloc(1,0x91b4);
    unsigned char slots[3][32]={{0}},links[3][32]={{0}},descriptors[2][0xcc4]={{0}};
    assert(actor && collection);hsv_payload=(uintptr_t)hsv_uniforms;
    put(actor,0x210,(uintptr_t)slots[2]);put(slots[2],0x10,(uintptr_t)links[2]);put(links[2],0x18,hsv_payload);
    put(actor,0x9b40,(uintptr_t)collection);uint32_t two=2;memcpy(collection+0x91b0,&two,4);
    for (unsigned i=0;i<2;++i) {
        put(collection,i*32,(uintptr_t)slots[i]);put(slots[i],0x10,(uintptr_t)links[i]);
        put(links[i],0x18,(uintptr_t)descriptors[i]);
    }
    const float samples[3][3]={{0,1,1},{.31f,.57f,.92f},{-1,2,0}};
    for (unsigned n=0;n<3;++n) {
        ((void (ABI *)(uintptr_t,uintptr_t,float,float,float))(image+0x217a70))
            (0,(uintptr_t)actor,samples[n][0],samples[n][1],samples[n][2]);
        for (unsigned i=0;i<3;++i) {
            float expected=(float)(unsigned)(fminf(1,fmaxf(0,samples[n][i]))*255.f)/255.f;
            assert(fabsf(hsv_uniforms[i]-expected)<0.000001f);
            for (unsigned j=0;j<2;++j) assert(jc_receiver_float(descriptors[j]+0xca8+i*4)==samples[n][i]);
        }
    }
    hsv_payload=0;free(actor);free(collection);
    puts("Native appearance HSV: body quantization and independent cloth descriptor values passed.");
}
static int read_memory(void *ctx,uint64_t p,void *out,size_t n) { (void)ctx;memcpy(out,(void *)(uintptr_t)p,n);return 1; }
struct capture_context { uint64_t address;unsigned calls,mode; };
static int capture_read(void *opaque,uint64_t p,void *out,size_t n) {
    struct capture_context *c=opaque;
    if (p==c->address && ++c->calls==2) {
        if(c->mode==1)return 0;
        memcpy(out,(void *)(uintptr_t)p,n);((unsigned char *)out)[0]^=1;return 1;
    }
    return read_memory(NULL,p,out,n);
}
static void put(unsigned char *p,unsigned off,uintptr_t value) { memcpy(p+off,&value,8); }
static void thunk(unsigned at,void *fn) {
    unsigned char code[]={0x48,0xb8,0,0,0,0,0,0,0,0,0xff,0xe0};
    memcpy(code+2,&fn,8);memcpy(image+at,code,sizeof(code));
}
struct publication_context {
    uintptr_t actor;
    unsigned calls,fail_at,persistent,identity_change,retired;
    unsigned char *game;
};
static int publication_write(void *opaque,uint64_t address,const void *bytes,size_t count_bytes) {
    struct publication_context *c=opaque;++c->calls;
    assert(address>=c->actor && address+count_bytes<=c->actor+JC_ACTOR_PUBLICATION_BYTES);
    for (size_t n=0;n<count_bytes;++n) assert(jc_actor_numeric_byte((unsigned)(address-c->actor+n)));
    if (c->fail_at && (c->calls==c->fail_at || (c->persistent && c->calls>=c->fail_at))) {
        memcpy((void *)(uintptr_t)address,bytes,(count_bytes+1)/2);return 0;
    }
    memcpy((void *)(uintptr_t)address,bytes,count_bytes);
    if (c->identity_change && c->calls==1) put(c->game,0x1e0,0x123450);
    return 1;
}
static int publication_retired(void *opaque,const struct jc_actor_identity *id) {
    struct publication_context *c=opaque;assert(c->actor==id->actor);return c->retired!=0;
}
static void publication_cases(unsigned char *barn) {
    unsigned char *actor=malloc(0x14640);struct jc_receiver *receiver=calloc(1,sizeof(*receiver));
    struct jc_receiver *saved=malloc(sizeof(*saved));struct jc_actor_publication *work=calloc(1,sizeof(*work));
    assert(actor && receiver && saved && work);
    unsigned char game[0x1e8]={0},clock[0x20]={0},before[JC_ACTOR_PUBLICATION_BYTES];
    put(barn,0,(uintptr_t)image+0x5da340);memset(barn+0x14,0,4);put(game,0x68,(uintptr_t)clock);
    jc_receiver_store(clock+0xc,1.0f/60);jc_receiver_store(clock+0x10,1.0f/60);
    struct jc_actor_identity id={(uintptr_t)image,(uintptr_t)game,(uintptr_t)barn,(uintptr_t)actor,0x100000,11,1};
    for (unsigned mode=0;mode<7;++mode) {
        memset(actor,0xa5,0x14640);memset(receiver,0,sizeof(*receiver));memset(work,0,sizeof(*work));
        put(game,0x1e0,(uintptr_t)barn);put(actor,0,(uintptr_t)image+0x5d0380);
        put(actor,0xa620,(uintptr_t)actor+0x160);put(actor,0x2240,id.glyph);
        struct jc_receiver_state current={0},incoming={0};
        uint32_t old_appearance[4]={1,4,1,3},new_appearance[4]={2,9,6,20};
        memcpy(current.bytes+0x58,old_appearance,16);memcpy(incoming.bytes+0x58,new_appearance,16);
        jc_receiver_store(incoming.bytes,100);jc_receiver_store(incoming.bytes+4,2);
        memcpy(actor+0x10,current.bytes,sizeof(current));jc_receiver_store(actor+0x1468,0);
        for(unsigned j=0;j<3;++j)jc_receiver_store(actor+0x1448+j*8,j==1?2.f:0.f);
        assert(!jc_receiver_replace(receiver,id.peer,id.generation));
        struct jc_actor_input input={0};
        struct jc_actor_sender sender={.body_scale=1.25f,.chirp_shape={.5f,1.2f},.glow_targets={.5f,1.f,.75f},
            .state=incoming,.axes={0.3f,0.2f,0,0},.direction={0,0,1,0}};
        assert(!jc_actor_receive(receiver,&input,&sender,id.peer,id.generation,1,0));
        jc_receiver_store(actor+0x1c,1001863.19f);
        jc_receiver_store(actor+0x2c,NAN);
        memcpy(saved,receiver,sizeof(*saved));memcpy(before,actor,sizeof(before));
        struct jc_actor_input original_input=input;
        assert(jc_actor_receive(receiver,&input,&sender,id.peer,id.generation,1,0)==JC_RECEIVER_REPLAY);
        float invalid_scales[]={0,-1,NAN,INFINITY,1001};
        for(unsigned j=0;j<5;++j) {
            sender.body_scale=invalid_scales[j];
            assert(jc_actor_receive(receiver,&input,&sender,id.peer,id.generation,2,0)==JC_RECEIVER_VALUE);
        }
        sender.body_scale=original_input.body_scale;
        for(unsigned j=0;j<5;++j) {
            sender.chirp_shape[1]=invalid_scales[j];
            assert(jc_actor_receive(receiver,&input,&sender,id.peer,id.generation,2,0)==JC_RECEIVER_VALUE);
        }
        memcpy(sender.chirp_shape,original_input.chirp_shape,sizeof(sender.chirp_shape));
        const float invalid_glow[]={-1,NAN,INFINITY,17};
        for(unsigned j=0;j<4;++j) {
            sender.glow_targets[0]=invalid_glow[j];
            assert(jc_actor_receive(receiver,&input,&sender,id.peer,id.generation,2,0)==JC_RECEIVER_VALUE);
        }
        sender.glow_targets[0]=original_input.glow_targets[0];
        sender.axes[0]=NAN;
        assert(jc_actor_receive(receiver,&input,&sender,id.peer,id.generation,2,0)==JC_RECEIVER_VALUE);
        sender.axes[0]=original_input.axes[0];sender.chapter=1;
        assert(jc_actor_receive(receiver,&input,&sender,id.peer,id.generation,2,0)==JC_RECEIVER_SESSION);
        sender.chapter=0;
        assert(jc_actor_receive(receiver,&input,&sender,id.peer+1,id.generation,2,0)==JC_RECEIVER_SESSION);
        assert(jc_actor_receive(receiver,&input,&sender,id.peer,id.generation+1,2,0)==JC_RECEIVER_SESSION);
        assert(!memcmp(receiver,saved,sizeof(*saved)) && !memcmp(&input,&original_input,sizeof(input)));
        struct publication_context c={.actor=(uintptr_t)actor,.game=game,.retired=mode!=1};
        if (mode>=2 && mode<=4)c.fail_at=mode-1;
        if (mode==5){c.fail_at=2;c.persistent=1;}
        if (mode==6)c.identity_change=1;
        unsigned inputs=input_calls,updates=update_calls;
        input.sequence=2;
        assert(jc_actor_publish_numeric(read_memory,publication_write,publication_retired,&c,&id,1,receiver,&input,work)==JC_ACTOR_IDENTITY);
        assert(c.calls==0);input.sequence=1;
        input.chapter=1;
        assert(jc_actor_publish_numeric(read_memory,publication_write,publication_retired,&c,&id,1,receiver,&input,work)==JC_ACTOR_IDENTITY);
        assert(c.calls==0);input.chapter=0;
        int result=jc_actor_publish_numeric(read_memory,publication_write,publication_retired,&c,&id,1,receiver,&input,work);
        assert(inputs==input_calls && updates==update_calls);
        if (!mode) {
            assert(result==0 && c.calls==work->changed_spans && c.calls>3);
            assert(!memcmp(actor+0x68,new_appearance,16));
            assert(jc_receiver_float(actor+0x1430)==sender.body_scale);
            assert(jc_receiver_float(actor+0x1470)==sender.chirp_shape[0]);
            assert(jc_receiver_float(actor+0x1e64)==sender.chirp_shape[1]);
            assert(jc_receiver_float(actor+0x1448)==.5f && jc_receiver_float(actor+0x1450)==2.f &&
                jc_receiver_float(actor+0x1458)==.75f);
            for (unsigned at=0;at<sizeof(before);++at)if(before[at]!=actor[at])assert(jc_actor_numeric_byte(at));
            unsigned calls=c.calls;
            assert(jc_actor_publish_numeric(read_memory,publication_write,publication_retired,&c,&id,1,receiver,&input,work)==JC_ACTOR_REPLAY);
            assert(c.calls==calls);
        } else if(mode<=4) {
            assert(result==(mode==1?JC_ACTOR_APPEARANCE_BUSY:JC_ACTOR_WRITE));
            assert(!memcmp(actor,before,sizeof(before)) && !memcmp(receiver,saved,sizeof(*saved)) &&
                !memcmp(&input,&original_input,sizeof(input)) && !work->quarantined);
        } else {
            assert(result==JC_ACTOR_QUARANTINED && work->quarantined);
            unsigned calls=c.calls;
            assert(jc_actor_publish_numeric(read_memory,publication_write,publication_retired,&c,&id,2,receiver,&input,work)==JC_ACTOR_QUARANTINED);
            assert(c.calls==calls);
        }
    }
    free(work);free(saved);free(receiver);free(actor);
}
struct runtime_context {
    struct publication_context publication;
    uint64_t thread;
    unsigned ready,prepared,phase,opened,closed,drained,fail_begin,fail_end;
};
static uint64_t runtime_thread(void *p) {return ((struct runtime_context *)p)->thread;}
static int runtime_ready(void *p,const struct jc_actor_identity *id) {
    assert(id->actor==((struct runtime_context *)p)->publication.actor);
    return ((struct runtime_context *)p)->ready;
}
static int runtime_prepare(void *p,const struct jc_actor_identity *id,uint64_t frame) {
    struct runtime_context *c=p;assert(id->actor==c->publication.actor && frame);
    ++c->prepared;return 1;
}
static int runtime_begin(void *p,const struct jc_actor_identity *id,unsigned phase,uint64_t frame,uint64_t render) {
    struct runtime_context *c=p;assert(id->actor==c->publication.actor && frame && !c->phase);
    assert((phase==JC_ACTOR_PHASE_RENDER)==(render!=0));
    if(c->fail_begin)return 0;
    c->phase=phase;++c->opened;return 1;
}
static int runtime_end(void *p,const struct jc_actor_identity *id,unsigned phase,uint64_t frame,int success) {
    struct runtime_context *c=p;assert(id->actor==c->publication.actor && frame && phase==c->phase);
    c->phase=0;++c->closed;return success && !c->fail_end;
}
static void runtime_drain(void *p,const struct jc_actor_identity *id) {
    struct runtime_context *c=p;assert(id->actor==c->publication.actor);++c->drained;
}
static int runtime_retirement_failed(void *p,const struct jc_actor_identity *id) {
    (void)p;(void)id;return -1;
}
#define __declspec(x)
typedef long LONG;
typedef void *PVOID;
typedef size_t SIZE_T;
#define HEAP_ZERO_MEMORY 8
static LONG JNETTraveler[3],JNETFrames;
static uint64_t traveler_object,traveler_game,traveler_barn;
static struct {int pending,failed;} cloth_chapter;
struct mock_cloth_backend {struct {int closing;} frames;};
static struct mock_cloth_backend mock_backend,*active_cloth_backend;
static unsigned actor_scope_open,actor_scope_closed,stock_inputs,stock_updates;
static unsigned world_write_failure,cloth_enable_write_failure;
static LONG InterlockedExchange(volatile LONG *p,LONG value) {LONG old=*p;*p=value;return old;}
static LONG InterlockedIncrement(volatile LONG *p) {return ++*p;}
static LONG InterlockedCompareExchange(volatile LONG *p,LONG value,LONG expected) {LONG old=*p;if(old==expected)*p=value;return old;}
static void *InterlockedExchangePointer(PVOID volatile *p,void *v) {void *old=*p;*p=v;return old;}
static void *InterlockedCompareExchangePointer(PVOID volatile *p,void *v,void *expected) {void *old=*p;if(old==expected)*p=v;return old;}
static unsigned traveler_thread=11;
static unsigned GetCurrentThreadId(void) {return 11;}
static uint64_t mock_ticks=1000;
static uint64_t GetTickCount64(void) {return mock_ticks;}
static void *GetModuleHandleW(void *p) {(void)p;return image;}
static void *GetProcessHeap(void) {return (void *)1;}
static void *GetCurrentProcess(void) {return (void *)1;}
static void *HeapAlloc(void *heap,unsigned flags,size_t bytes) {(void)heap;assert(flags==HEAP_ZERO_MEMORY);return calloc(1,bytes);}
static int HeapFree(void *heap,unsigned flags,void *p) {(void)heap;(void)flags;free(p);return 1;}
static int WriteProcessMemory(void *process,void *address,const void *bytes,size_t n,size_t *written) {
    (void)process;
    if(cloth_enable_write_failure && address==(void *)(uintptr_t)(traveler_object+0x1479)){*written=0;return 0;}
    if(world_write_failure && address==image+0x3ce2320){*written=0;return 0;}
    memcpy(address,bytes,n);*written=n;return 1;
}
static int jc_local_windows_write(uint64_t address,const void *bytes,size_t n) {
    size_t written=0;
    return WriteProcessMemory(GetCurrentProcess(),(void *)(uintptr_t)address,bytes,n,&written) && written==n;
}
static uint64_t sand_read_failure;
static int own_read(void *p,uint64_t address,void *out,size_t n) {return address!=sand_read_failure && read_memory(p,address,out,n);}
static int traveler_context(void) {return traveler_object!=0;}
static int jc_cloth_live_owner(struct mock_cloth_backend *s) {return s==&mock_backend;}
static int cloth_actor_scope_begin(uint64_t peer,uint64_t generation) {
    assert(peer && generation);++actor_scope_open;return 0;
}
static int cloth_actor_scope_end(uint64_t peer,uint64_t generation,int success) {
    assert(peer && generation);++actor_scope_closed;
    if(success) {
        const float position[4]={7,8,9,1};
        reference_world((void *)(uintptr_t)traveler_object,position,image+0xa9c8e);
    }
    return 0;
}
#include "traveler_glyph_init.h"
#include "traveler_glyph_trace.h"
static void glyph_trace_cases(void) {
    unsigned char model[0x100]={0},render[0x70]={0};uint64_t item=0x1234;
    unsigned char *queue=calloc(1,0x10084);assert(queue);
    put(model,0x60,(uintptr_t)&item);put(model,0x68,(uintptr_t)&item+8);
    put(render,0x60,(uintptr_t)queue);put(queue,0x80,(uintptr_t)model);
    uint32_t count=1;memcpy(queue+0x10080,&count,4);
    traveler_glyph_trace_bind(1,0x111,(uintptr_t)model,0x222,(uintptr_t)render);
    traveler_glyph_trace_commit(0x111,1);traveler_glyph_trace_commit(0x111,0);
    traveler_glyph_trace_commit(0x333,1);traveler_glyph_trace_queue((uintptr_t)queue);
    traveler_glyph_trace_dispatch(item,0);traveler_glyph_trace_dispatch(0x555,5);
    assert(traveler_glyph_trace_build(0x222)==1 && traveler_glyph_trace_build(0x555)==-1);
    traveler_glyph_trace_built(1,0,0x1000,0x1080,1);
    traveler_glyph_trace_built(1,5,0x1080,0x1080,0);
    assert(traveler_glyph_trace[1].commits==1 && traveler_glyph_trace[1].denied==1);
    assert(traveler_glyph_trace[1].queued==1 && traveler_glyph_trace[1].dispatch==1);
    assert(traveler_glyph_trace[1].built==2 && traveler_glyph_trace[1].commands==1);
    count=4097;memcpy(queue+0x10080,&count,4);traveler_glyph_trace_queue((uintptr_t)queue);
    assert(traveler_glyph_trace[1].queued==1);traveler_glyph_trace_report();
    memset(traveler_glyph_trace,0,sizeof(traveler_glyph_trace));free(queue);
}
#include "actor_runtime_fixture.h"
static int traveler_peer_admit(uint64_t peer,uint64_t generation) {(void)peer;(void)generation;return 0;}
static int traveler_peer_vacant(void) {return 0;}
static void traveler_local_before_input(uintptr_t barn,uintptr_t game){(void)barn;(void)game;}
static void traveler_local_after_update(void){}
static void traveler_native_chirp_tick(void){}
static void traveler_charge_before_update(uintptr_t barn,uintptr_t game){(void)barn;(void)game;}
static void traveler_charge_after_update(void){}
static unsigned peer_fault_calls;
static int peer_fault_code;
static int traveler_peer_fault(int code) {++peer_fault_calls;peer_fault_code=code;return 0;}
static uint32_t traveler_repeat_outfit(void) {return UINT32_MAX;}
static int traveler_body_prepare(const struct jc_actor_identity *id) {(void)id;return 1;}
static int traveler_peer_before_input(void) {return 1;}
static void traveler_peer_after_input(void) {}
static void traveler_peer_after_update(void) {}
#define JC_SAND_ABI __attribute__((ms_abi))
#include "traveler_sand_guard.h"
#undef __declspec
static struct jc_actor_runtime_ops trial_ops;
static int trial_prepare_result;
int JNETActorPrepare(uint64_t peer,uint64_t generation) {
    return trial_prepare_result?trial_prepare_result:JNETActorActivate(peer,generation,&trial_ops);
}
static uintptr_t mock_stock_input(uintptr_t barn,uintptr_t game) {
    assert(barn==traveler_barn && game==traveler_game);++stock_inputs;return 71;
}
static uintptr_t mock_stock_update(uintptr_t barn,uintptr_t game) {
    assert(barn==traveler_barn && game==traveler_game);++stock_updates;return 72;
}
static uintptr_t mock_stock_finish(uintptr_t barn) {assert(barn==traveler_barn);return 73;}
struct material_fixture {
    unsigned char slot[0x20],link[0x20],payload[0xa0],collection[0x10],member[0x10],descriptor[8],buffer[64];
};
static struct material_fixture actor_materials[12];
static unsigned char actor_glyphs[3][0x600];
static int changing_material_read(void *context,uint64_t address,void *out,size_t n) {
    unsigned *seen=context;
    if(address==(uintptr_t)actor_materials[0].buffer && n==4 && ++*seen==2) {
        uint32_t changed=32;memcpy(out,&changed,4);return 1;
    }
    return read_memory(NULL,address,out,n);
}
static void actor_material_setup(unsigned char *actor,unsigned char *barn) {
    static const unsigned offset[3]={0x1e0,0x830,0xed0};
    unsigned char *actors[3]={actor,barn+0xb0,barn+0x146f0};
    memset(actor_materials,0,sizeof(actor_materials));
    for(unsigned i=0;i<3;++i)put(actors[i],0x2240,(uintptr_t)actor_glyphs[i]);
    for(unsigned row=0;row<12;++row) {
        struct material_fixture *m=actor_materials+row;
        if(row<9)put(actors[row/3],offset[row%3]+0x30,(uintptr_t)m->slot);
        else put(actor_glyphs[row-9],0xc0,(uintptr_t)m->slot);
        put(m->slot,0x10,(uintptr_t)m->link);put(m->link,0x18,(uintptr_t)m->payload);
        put(m->payload,0x80,(uintptr_t)m->collection);put(m->collection,0,1);
        put(m->collection,8,(uintptr_t)m->member);put(m->member,0,(uintptr_t)m->descriptor);
        put(m->member,8,(uintptr_t)m->buffer);put(m->buffer,0,sizeof(m->buffer));
    }
}
static void actor_material_cases(const struct jc_actor_identity *id) {
    struct jc_actor_material_proof proof;
    int handled=0;
    assert(!jc_actor_material_independent(read_memory,NULL,id,&proof));
    unsigned char empty[0x480]={0},slot_empty[0x20]={0},unchanged[0x480];
    const unsigned setters[]={0x273200,0x273400,0x273640};
    for(unsigned mode=0;mode<2;++mode) {
        put(empty,0x30,mode?(uintptr_t)slot_empty:0);memcpy(unchanged,empty,sizeof(empty));
        for(unsigned n=0;n<3;++n) {
            ((uintptr_t (ABI *)(uintptr_t,uintptr_t,uintptr_t))(image+setters[n]))
                ((uintptr_t)empty,(uintptr_t)"unused",0);
            assert(!memcmp(empty,unchanged,sizeof(empty)));
        }
    }
    assert(!jc_actor_material_write_check(read_memory,NULL,id,id->actor+0x830,0,&proof,&handled) && handled);
    assert(!jc_actor_material_write_check(read_memory,NULL,id,(uintptr_t)actor_materials[2].payload,1,&proof,&handled) && handled);
    assert(!jc_actor_material_write_check(read_memory,NULL,id,123,0,&proof,&handled) && !handled);
    assert(!jc_actor_material_write_check(read_memory,NULL,id,id->glyph+0x90,2,&proof,&handled) && handled);
    put(actor_materials[9].member,8,(uintptr_t)actor_materials[10].buffer);
    assert(jc_actor_material_independent(read_memory,NULL,id,&proof)==JC_ACTOR_MATERIAL_ALIAS);
    put(actor_materials[9].member,8,(uintptr_t)actor_materials[9].buffer);
    for(unsigned own=0;own<3;++own)for(unsigned other=own+1;other<9;++other) {
        struct material_fixture *a=actor_materials+own,*b=actor_materials+other;
        put(a->link,0x18,(uintptr_t)b->payload);
        assert(jc_actor_material_independent(read_memory,NULL,id,&proof)==JC_ACTOR_MATERIAL_ALIAS);
        assert(proof.row==own && proof.other==other);put(a->link,0x18,(uintptr_t)a->payload);
        put(a->member,8,(uintptr_t)b->buffer);
        assert(jc_actor_material_independent(read_memory,NULL,id,&proof)==JC_ACTOR_MATERIAL_ALIAS);
        put(a->member,8,(uintptr_t)a->buffer);
    }
    put(actor_materials[1].payload,0x80,0);
    assert(jc_actor_material_independent(read_memory,NULL,id,&proof)==JC_ACTOR_MATERIAL_READ);
    put(actor_materials[1].payload,0x80,(uintptr_t)actor_materials[1].collection);
    assert(!jc_actor_material_independent(read_memory,NULL,id,&proof));
    uint64_t slot;
    memcpy(&slot,(void *)(uintptr_t)(id->actor+0x860),8);
    memset((void *)(uintptr_t)(id->actor+0x860),0,8);
    assert(!jc_actor_material_independent(read_memory,NULL,id,&proof));
    assert(jc_actor_material_write_check(read_memory,NULL,id,id->actor+0x830,0,&proof,&handled)==JC_ACTOR_MATERIAL_EMPTY_SETTER && handled);
    assert(jc_actor_material_write_check(read_memory,NULL,id,id->actor+0x830,2,&proof,&handled)==JC_ACTOR_MATERIAL_READ && handled);
    memcpy((void *)(uintptr_t)(id->actor+0x860),&slot,8);
    put(actor_materials[0].payload,0x88,(uintptr_t)actor_materials[8].collection);
    assert(jc_actor_material_independent(read_memory,NULL,id,&proof)==JC_ACTOR_MATERIAL_ALIAS);
    put(actor_materials[0].payload,0x88,0);
    unsigned seen=0;
    assert(jc_actor_material_independent(changing_material_read,&seen,id,&proof)==JC_ACTOR_MATERIAL_CHANGED);
    assert(!jc_actor_material_independent(read_memory,NULL,id,&proof));
}
static void glyph_init_cases(void) {
    unsigned char *decorations=calloc(1,0x434020),*actor=calloc(1,0x2250);
    unsigned char source[0x20]={0},resource[0x190]={0},rows[3][0x30]={{0}},anchor[0x4a8]={0};
    unsigned char *barn=calloc(1,0x28e00);
    assert(decorations && actor && barn);actor_material_setup(actor,barn);
    put(source,0x10,(uintptr_t)resource);put(resource,0,(uintptr_t)image+0x680930);
    put(resource,0x180,(uintptr_t)rows);put(resource,0x188,3);
    put(anchor,0x10,(uintptr_t)source);put(anchor,0x30,(uintptr_t)rows[0]);
    for(unsigned i=0;i<3;++i) {
        put(rows[i],0x10,(uintptr_t)rows[i]);put(rows[i],0x18,(uintptr_t)actor_materials[i].payload);
    }
    put(rows[0],0,(uintptr_t)anchor+0x30);put(rows[0],8,(uintptr_t)anchor+0x30);
    traveler_glyph_sampler_hook_ready=1;
    assert(traveler_glyph_init_begin((uintptr_t)image,(uintptr_t)actor,(uintptr_t)anchor,(uintptr_t)decorations,0));
    unsigned char *material=decorations+0xa0;uint64_t pair[2]={0};
    assert(traveler_actor_sampler_write_allowed((uintptr_t)material,pair));
    assert(((unsigned char (ABI *)(uintptr_t,const uint64_t *))(image+0x272580))((uintptr_t)material,pair)==1);
    assert(material[0x470]==1);
    put(material,0x10,(uintptr_t)source);put(material,0x30,(uintptr_t)rows[1]);
    put(rows[1],0,(uintptr_t)material+0x30);put(rows[1],8,(uintptr_t)material+0x30);
    assert(traveler_actor_sampler_write_allowed((uintptr_t)material,pair));
    assert(((unsigned char (ABI *)(uintptr_t,const uint64_t *))(image+0x272580))((uintptr_t)material,pair)==1);
    put(actor,0x2240,(uintptr_t)decorations+0x10);assert(traveler_glyph_init_end());
    unsigned char runtime_source[0x20]={0},runtime_resource[0x190]={0};
    unsigned char runtime_rows[136][0x30]={{0}};
    put(runtime_source,0x10,(uintptr_t)runtime_resource);
    put(runtime_resource,0,(uintptr_t)image+0x680930);
    memcpy(runtime_resource+8,"ScatteringSoft",15);
    put(runtime_resource,0x18,14);put(runtime_resource,0x20,15);
    put(runtime_resource,0x110,136);put(runtime_resource,0x180,(uintptr_t)runtime_rows);
    put(runtime_resource,0x188,136);
    put(runtime_rows[135],0,(uintptr_t)material+0x30);
    put(runtime_rows[135],8,(uintptr_t)material+0x30);
    put(runtime_rows[135],0x10,(uintptr_t)runtime_rows[135]);
    put(runtime_rows[135],0x18,(uintptr_t)actor_materials[1].payload);
    put(material,0x10,(uintptr_t)runtime_source);put(material,0x30,(uintptr_t)runtime_rows[135]);
    struct jc_geometry_pool runtime_pool;struct jc_geometry_slots runtime_slots;
    assert(jc_geometry_pool_read_slots(own_read,NULL,(uintptr_t)material,&runtime_pool,&runtime_slots)==JC_LAYOUT_RANGE);
    assert(traveler_actor_material_write_allowed((uintptr_t)material,2));
    runtime_resource[8]='X';
    assert(!traveler_actor_material_write_allowed((uintptr_t)material,2));
    assert(traveler_glyph_init.failed==JC_GLYPH_CHANGED);
    runtime_resource[8]='S';traveler_glyph_init.failed=0;
    put(runtime_rows[0],0,(uintptr_t)anchor+0x30);
    put(anchor,0x30,(uintptr_t)runtime_rows[0]);
    put(runtime_rows[0],0x10,(uintptr_t)runtime_rows[0]);
    put(runtime_rows[0],0x18,(uintptr_t)actor_materials[0].payload);
    put(actor_materials[1].member,8,(uintptr_t)actor_materials[0].buffer);
    assert(!traveler_actor_material_write_allowed((uintptr_t)material,2));
    assert(traveler_glyph_init.failed==JC_GLYPH_ALIAS);
    put(actor_materials[1].member,8,(uintptr_t)actor_materials[1].buffer);
    traveler_glyph_init.failed=0;
    assert(traveler_actor_material_write_allowed((uintptr_t)material,2));
    put(material,0x10,(uintptr_t)source);put(material,0x30,(uintptr_t)rows[1]);
    put(anchor,0x30,(uintptr_t)rows[0]);
    put(material,0x30,0);
    assert(!traveler_actor_material_write_allowed((uintptr_t)material,2));
    assert(traveler_glyph_init.failed==JC_GLYPH_SLOT);
    assert(!traveler_actor_material_write_allowed((uintptr_t)material,2));
    put(material,0x30,(uintptr_t)rows[1]);
    assert(traveler_actor_material_write_allowed((uintptr_t)material,2));
    assert(!traveler_glyph_init.failed);
    put(rows[1],8,(uintptr_t)anchor+0x30);
    assert(!traveler_actor_material_write_allowed((uintptr_t)material,2));
    put(rows[1],8,(uintptr_t)material+0x30);
    assert(traveler_actor_material_write_allowed((uintptr_t)material,2));
    assert(traveler_glyph_init.recovered==2 && traveler_glyph_init.retries==3);
    put(actor_materials[1].member,8,(uintptr_t)actor_materials[0].buffer);
    assert(!traveler_actor_sampler_write_allowed((uintptr_t)material,pair) && traveler_glyph_init.failed==JC_GLYPH_ALIAS);
    put(actor_materials[1].member,8,(uintptr_t)actor_materials[1].buffer);
    assert(!traveler_actor_material_write_allowed((uintptr_t)material,2));
    assert(traveler_glyph_init.failed==JC_GLYPH_ALIAS);
    assert(traveler_glyph_init_release((uintptr_t)actor));
    assert(traveler_actor_sampler_write_allowed((uintptr_t)material,pair));
    assert(traveler_actor_material_write_allowed((uintptr_t)material,2));
    traveler_glyph_sampler_hook_ready=0;
    assert(!traveler_glyph_init_begin((uintptr_t)image,(uintptr_t)actor,(uintptr_t)anchor,(uintptr_t)decorations,0));
    assert(traveler_glyph_init.state==3 && traveler_glyph_init.failed==JC_GLYPH_HOOK);
    traveler_glyph_sampler_hook_ready=1;
    ++traveler_glyph_init.thread;
    assert(!traveler_glyph_init_begin((uintptr_t)image,(uintptr_t)actor,(uintptr_t)anchor,(uintptr_t)decorations,0));
    assert(traveler_glyph_init.state==3 && traveler_glyph_init.failed==JC_GLYPH_THREAD);
    traveler_glyph_init.thread=GetCurrentThreadId();
    memset(material,0,0x4a8);memset(rows[1],0,0x30);
    assert(traveler_glyph_init_begin((uintptr_t)image,(uintptr_t)actor,(uintptr_t)anchor,(uintptr_t)decorations,0));
    assert(traveler_glyph_init.state==1 && !traveler_glyph_init.failed && !traveler_glyph_init.initialized);
    assert(!traveler_glyph_init.calls && !traveler_glyph_init.retries && !traveler_glyph_init.recovered);
    assert(!traveler_glyph_init_begin((uintptr_t)image,(uintptr_t)actor,(uintptr_t)anchor,(uintptr_t)decorations,0));
    assert(traveler_glyph_init.failed==JC_GLYPH_HOOK);
    memset(&traveler_glyph_init,0,sizeof(traveler_glyph_init));
    free(actor);free(decorations);free(barn);
    puts("Floating glyph ownership: 136-slot ScatteringSoft transition accepted; unknown resources and buffer aliases denied; exclusive-slot recovery passed.");
}
static void runtime_cases(unsigned char *barn) {
    struct jc_actor_runtime *r=calloc(1,sizeof(*r));unsigned char *actor=calloc(1,0x14640);
    assert(r && actor);
    unsigned char game[0x1e8]={0},clock[0x20]={0},flags[0x50]={0},render[0x200]={0};
    put(barn,0,(uintptr_t)image+0x5da340);memset(barn+0x14,0,4);
    put(game,0x68,(uintptr_t)clock);put(game,0x178,(uintptr_t)flags);put(game,0x1e0,(uintptr_t)barn);
    put(actor,0,(uintptr_t)image+0x5d0380);put(actor,0xa620,(uintptr_t)actor+0x160);put(actor,0x2240,(uintptr_t)actor_glyphs[0]);
    put(render,0x18,0x123400);put(render,0x60,0x567800);
    jc_receiver_store(clock+0xc,1.0f/60);jc_receiver_store(clock+0x10,1.0f/60);
    struct jc_actor_identity id={(uintptr_t)image,(uintptr_t)game,(uintptr_t)barn,(uintptr_t)actor,(uintptr_t)actor_glyphs[0],22,1};
    struct runtime_context c={.publication={.actor=(uintptr_t)actor,.game=game,.retired=1},.thread=11,.ready=1};
    struct jc_actor_runtime_ops ops={read_memory,publication_write,publication_retired,runtime_thread,
        runtime_ready,runtime_prepare,runtime_begin,runtime_end,runtime_drain,&c};
    struct jc_actor_runtime_ops missing=ops;missing.end=NULL;
    assert(jc_actor_runtime_bind(r,&id,&missing)==JC_ACTOR_RUNTIME_ARGUMENT);
    c.publication.retired=0;assert(jc_actor_runtime_bind(r,&id,&ops)==JC_ACTOR_RUNTIME_BUSY);
    c.publication.retired=1;assert(!jc_actor_runtime_bind(r,&id,&ops));
    assert(jc_actor_runtime_after_input(r,1)==JC_ACTOR_RUNTIME_ORDER);
    struct jc_actor_sender sender={.body_scale=1.0f,.chirp_shape={0,1},.direction={0,0,1,0}};
    uint32_t appearance[4]={2,8,6,20};memcpy(sender.state.bytes+0x58,appearance,16);
    assert(!jc_actor_runtime_receive(r,&sender,22,1,1));
    unsigned inputs=input_calls,updates=update_calls;
    assert(jc_actor_runtime_after_update(r,1)==JC_ACTOR_RUNTIME_ORDER);
    assert(jc_actor_runtime_after_render(r,1,(uintptr_t)render)==JC_ACTOR_RUNTIME_ORDER);
    c.thread=12;assert(jc_actor_runtime_after_input(r,1)==JC_ACTOR_RUNTIME_THREAD);c.thread=11;
    c.publication.retired=0;assert(jc_actor_runtime_after_input(r,1)==JC_ACTOR_RUNTIME_BUSY);
    c.publication.retired=1;c.fail_begin=1;
    assert(jc_actor_runtime_after_input(r,1)==JC_ACTOR_RUNTIME_SCOPE);c.fail_begin=0;
    assert(!jc_actor_runtime_after_input(r,1));assert(input_calls==inputs+1 && captured_gate==1);
    assert(!memcmp(actor+0x68,appearance,16));
    assert(jc_actor_runtime_receive(r,&sender,22,1,2)==JC_ACTOR_RUNTIME_ORDER);
    assert(jc_actor_runtime_after_input(r,1)==JC_ACTOR_RUNTIME_ORDER);
    assert(!jc_actor_runtime_after_update(r,1));assert(update_calls==updates+1);
    assert(jc_actor_runtime_after_update(r,1)==JC_ACTOR_RUNTIME_ORDER);
    count=0;assert(!jc_actor_runtime_after_render(r,1,(uintptr_t)render));
    assert(r->submission.actor==(uintptr_t)actor && r->submission.meshes==1 && count==4);
    assert(jc_actor_runtime_after_render(r,1,(uintptr_t)render)==JC_ACTOR_RUNTIME_ORDER);
    assert(jc_actor_runtime_receive(r,&sender,22,1,2)==JC_ACTOR_RUNTIME_ORDER);
    jc_receiver_store(actor+0x1c50,3.5f);
    unsigned bones=bone_calls;
    assert(!jc_actor_runtime_after_finish(r,1));
    assert(bone_calls==bones+2 && jc_receiver_float(actor+0x1e50)==3.5f);
    assert(!memcmp(actor+0x2250,end_bone+12,16) && !memcmp(actor+0x2260,end_bone+12,16));
    assert(jc_actor_runtime_after_finish(r,1)==JC_ACTOR_RUNTIME_ORDER);
    assert(c.opened==4 && c.closed==4 && !c.phase);
    assert(!jc_actor_runtime_receive(r,&sender,22,1,2));
    unsigned writes_before=c.publication.calls;
    for (unsigned n=0;n<2;++n) {
        jc_receiver_store(clock+0xc,n?2.f:0.f);
        assert(jc_actor_runtime_after_input(r,2+n)==JC_ACTOR_RECEIVER);
        assert(r->state==JC_ACTOR_RUNTIME_ACTIVE && !r->busy && !c.phase && !c.drained);
        assert(!r->publication.attempted_spans && c.publication.calls==writes_before);
        assert(input_calls==inputs+1 && r->input_frame==r->finish_frame);
        assert(jc_actor_runtime_after_render(r,2+n,(uintptr_t)render)==JC_ACTOR_RUNTIME_ORDER);
    }
    jc_receiver_store(clock+0xc,1.f/60);
    assert(!jc_actor_runtime_after_input(r,4));
    assert(!jc_actor_runtime_after_update(r,4));
    count=0;assert(!jc_actor_runtime_after_render(r,4,(uintptr_t)render));
    assert(!jc_actor_runtime_after_finish(r,4));
    r->departing=1;r->input.inactive=1;memset(r->input.axes,0,sizeof(r->input.axes));
    assert(!jc_receiver_depart(&r->receiver,22,1));writes_before=c.publication.calls;
    assert(jc_actor_runtime_receive(r,&sender,22,1,3)==JC_ACTOR_RUNTIME_ORDER);
    assert(!jc_actor_runtime_after_input(r,5) && !captured_gate && c.publication.calls==writes_before);
    assert(!jc_actor_runtime_after_update(r,5));
    count=0;assert(!jc_actor_runtime_after_render(r,5,(uintptr_t)render));
    assert(!jc_actor_runtime_after_finish(r,5));
    c.publication.retired=0;assert(jc_actor_runtime_stop(r)==JC_ACTOR_RUNTIME_BUSY);
    assert(c.drained==1 && r->state==JC_ACTOR_RUNTIME_DRAINING);
    assert(jc_actor_runtime_after_input(r,2)==JC_ACTOR_RUNTIME_ORDER);
    assert(jc_actor_runtime_stop(r)==JC_ACTOR_RUNTIME_BUSY && c.drained==1);
    c.publication.retired=1;assert(!jc_actor_runtime_stop(r));assert(!jc_actor_runtime_stop(r));
    assert(r->state==JC_ACTOR_RUNTIME_RETIRED);
    memset(r,0,sizeof(*r));
    struct jc_actor_runtime_ops failed_retirement=ops;
    failed_retirement.retired=runtime_retirement_failed;
    assert(jc_actor_runtime_bind(r,&id,&failed_retirement)==JC_ACTOR_RUNTIME_BUSY);
    assert(r->state==JC_ACTOR_RUNTIME_EMPTY);
    assert(!jc_actor_runtime_bind(r,&id,&ops));
    assert(!jc_actor_runtime_receive(r,&sender,22,1,1));
    r->ops.retired=runtime_retirement_failed;
    assert(jc_actor_runtime_after_input(r,1)==JC_ACTOR_RUNTIME_BUSY);
    assert(!r->input_frame);
    assert(jc_actor_runtime_stop(r)==JC_ACTOR_RUNTIME_SCOPE);
    assert(r->state==JC_ACTOR_RUNTIME_QUARANTINED);
    assert(jc_actor_runtime_stop(r)==JC_ACTOR_QUARANTINED);
    memset(r,0,sizeof(*r));assert(!jc_actor_runtime_bind(r,&id,&ops));
    assert(!jc_actor_runtime_receive(r,&sender,22,1,1));
    c.fail_end=1;assert(jc_actor_runtime_after_input(r,1)==JC_ACTOR_RUNTIME_SCOPE);
    assert(r->state==JC_ACTOR_RUNTIME_QUARANTINED && !c.phase);
    assert(jc_actor_runtime_stop(r)==JC_ACTOR_QUARANTINED);
    free(r);
    traveler_object=(uintptr_t)actor;traveler_barn=(uintptr_t)barn;traveler_game=(uintptr_t)game;
    actor_material_setup(actor,barn);actor_material_cases(&id);
    for(unsigned n=0;n<2;++n)payloads[n]=(uintptr_t)actor_materials[n+1].payload;
    JNETTraveler[0]=3;JNETFrames=1;active_cloth_backend=&mock_backend;
    original_traveler_input=mock_stock_input;original_traveler_update=mock_stock_update;
    original_traveler_finish=mock_stock_finish;
    traveler_actor_material_hooks_ready=1;
    c.fail_end=0;ops.write=traveler_actor_write;
    assert(!JNETActorActivate(22,1,&ops));
    assert(!JNETActorReceive(&sender,22,1,1));
    actor[0x1478]=17;actor[0x1479]=0;actor[0x147a]=0;
    cloth_enable_write_failure=1;
    assert(!traveler_actor_cloth_enable(&traveler_actor->identity) && !actor[0x1479]);
    cloth_enable_write_failure=0;actor[0x1479]=2;
    assert(!traveler_actor_cloth_enable(&traveler_actor->identity) && actor[0x1479]==2);
    actor[0x1479]=0;
    assert(traveler_actor_material_write_allowed((uintptr_t)actor+0x830,0));
    traveler_actor_material_scope=JC_ACTOR_PHASE_UPDATE;
    assert(traveler_actor_material_write_allowed((uintptr_t)actor+0x830,0));
    put(actor_materials[1].member,8,(uintptr_t)actor_materials[3].buffer);
    assert(!traveler_actor_material_write_allowed((uintptr_t)actor_materials[1].payload,1));
    assert(traveler_actor_material_failed==JC_ACTOR_MATERIAL_ALIAS);
    put(actor_materials[1].member,8,(uintptr_t)actor_materials[1].buffer);
    traveler_actor_material_scope=traveler_actor_material_failed=0;
    float rejected=2.f;
    assert(!traveler_actor_write(NULL,(uintptr_t)actor-1,&rejected,4));
    assert(!traveler_actor_write(NULL,(uintptr_t)actor+0x1e68,&rejected,4));
    assert(!traveler_actor_write(NULL,(uintptr_t)actor+0x1e63,&rejected,4));
    assert(!traveler_actor_write(NULL,(uintptr_t)actor+0x2240,&rejected,4));
    jc_receiver_store(actor+0x1e64,0);
    inputs=input_calls;updates=update_calls;
    const float saved_world[4]={1,2,3,4};float saved_glow=0.25f;
    memcpy(image+0x3ce2320,saved_world,16);memcpy(image+0x3c941d0,&saved_glow,4);
    image[0x3c941bb]=1;
    assert(traveler_input_observer((uintptr_t)barn,(uintptr_t)game)==71);
    assert(traveler_actor->state==JC_ACTOR_RUNTIME_ACTIVE && traveler_actor->input_frame==1);
    assert(jc_receiver_float(actor+0x1e64)==sender.chirp_shape[1]);
    assert(traveler_update_observer((uintptr_t)barn,(uintptr_t)game)==72);
    assert(actor[0x1479]==1 && actor[0x1478]==17);
    assert(!memcmp(image+0x3ce2320,saved_world,16) && !memcmp(image+0x3c941d0,&saved_glow,4));
    assert(!traveler_actor_world.active);
    assert(traveler_actor_world_capture(&traveler_actor->identity));
    reference_world(actor,saved_world,image+0xa9c8e);
    world_write_failure=1;assert(!traveler_actor_world_restore() && traveler_actor_world.active);
    world_write_failure=0;assert(traveler_actor_world_restore());
    count=0;traveler_actor_render((uintptr_t)barn,(uintptr_t)render);
    assert(traveler_finish_observer((uintptr_t)barn)==73);
    assert(traveler_actor->finish_frame==1);
    assert(input_calls==inputs+1 && update_calls==updates+1 && stock_inputs==1 && stock_updates==1);
    assert(actor_scope_open==1 && actor_scope_closed==1 && JNETActor[4]==1);
    traveler_actor_scope_failure(JC_ACTOR_PHASE_UPDATE,4,JC_ACTOR_RUNTIME_SCOPE,99);
    traveler_actor_scope_failure(JC_ACTOR_PHASE_UPDATE,8,JC_ACTOR_RUNTIME_SCOPE,100);
    assert(JNETActor[13]==4 && JNETActor[15]==99);
    unsigned prior_faults=peer_fault_calls;
    traveler_actor->state=JC_ACTOR_RUNTIME_QUARANTINED;
    traveler_actor->last_result=JC_ACTOR_RUNTIME_SCOPE;
    traveler_actor_report(JC_ACTOR_RUNTIME_SCOPE);
    assert(peer_fault_calls==prior_faults+1 && peer_fault_code==JC_ACTOR_RUNTIME_SCOPE);
    traveler_actor->state=JC_ACTOR_RUNTIME_ACTIVE;traveler_actor->last_result=0;
    assert(!JNETActorStop());
    HeapFree(GetProcessHeap(),0,traveler_actor);traveler_actor=NULL;
    trial_ops=ops;
    put(barn+0xb0,0,(uintptr_t)image+0x5d0380);
    memset(barn+0xc0,0,JC_RECEIVER_STATE_BYTES);
    memset(barn+0x20,0,0x30);
    jc_receiver_store(barn+0x14e0,1.0f);
    jc_receiver_store(barn+0x1f14,1.0f);
    JNETTraveler[0]=2;
    assert(JNETActorTrialStart()==JC_ACTOR_RUNTIME_ARGUMENT);JNETTraveler[0]=3;
    trial_prepare_result=JC_ACTOR_RUNTIME_BUSY;
    assert(JNETActorTrialStart()==JC_ACTOR_RUNTIME_BUSY && JNETActorTrial[0]==-1);
    assert(!traveler_actor);
    memset((void *)JNETActorTrial,0,sizeof(JNETActorTrial));trial_prepare_result=0;
    assert(!JNETActorTrialStart());
    assert(JNETActorTrial[0]==1 && traveler_actor_trial_sequence==1);
    assert(jc_receiver_float(traveler_actor->receiver.image+0x28d30)==2.0f);
    assert(JNETActorTrialStart()==JC_ACTOR_RUNTIME_ARGUMENT);
    mock_ticks+=16;JNETFrames=1;
    assert(traveler_input_observer((uintptr_t)barn,(uintptr_t)game)==71);
    assert(traveler_actor_trial_sequence==2);
    unsigned char local_state[JC_RECEIVER_STATE_BYTES];memcpy(local_state,barn+0xc0,sizeof(local_state));
    for(uint32_t outfit=0;outfit<7;++outfit) {
        traveler_actor_trial_outfit=outfit;
        struct jc_actor_sender varied;uint32_t actual,symbol;
        assert(!traveler_actor_trial_sample(&varied,0) && jc_actor_sender_valid(&varied));
        memcpy(&actual,varied.state.bytes+0x60,4);memcpy(&symbol,varied.state.bytes+0x64,4);
        assert(actual==outfit && symbol==traveler_actor_trial_symbol);
        assert(!memcmp(barn+0xc0,local_state,sizeof(local_state)));
        assert(!memcmp(varied.state.bytes+0x58,local_state+0x58,8));
    }
    traveler_actor_trial_outfit=UINT32_MAX;

    assert(traveler_update_observer((uintptr_t)barn,(uintptr_t)game)==72);
    count=0;traveler_actor_render((uintptr_t)barn,(uintptr_t)render);
    assert(traveler_finish_observer((uintptr_t)barn)==73);
    c.publication.retired=0;mock_ticks=traveler_actor_trial_started+60000;
    traveler_actor_trial_tick((uintptr_t)barn,(uintptr_t)game);
    assert(JNETActorTrial[0]==2 && traveler_actor_trial_draining);
    c.publication.retired=1;assert(!JNETActorTrialStop());
    assert(JNETActorTrial[0]==2 && traveler_peer_depart_requested==1);
    assert(!JNETActorStop() && traveler_actor->state==JC_ACTOR_RUNTIME_RETIRED);
    traveler_actor_trial_stop(0);
    assert(!traveler_peer_depart_requested && !traveler_actor_trial_draining &&
        traveler_actor->state==JC_ACTOR_RUNTIME_RETIRED);
    HeapFree(GetProcessHeap(),0,traveler_actor);traveler_actor=NULL;
    memset((void *)JNETActorTrial,0,sizeof(JNETActorTrial));
    assert(!JNETActorTrialStart());
    traveler_actor_trial_tick((uintptr_t)barn+16,(uintptr_t)game);
    assert(JNETActorTrial[0]==2 && JNETActorTrial[1]==JC_ACTOR_IDENTITY && traveler_peer_depart_requested==2);
    assert(!JNETActorStop());traveler_peer_depart_requested=0;traveler_actor_trial_draining=0;
    HeapFree(GetProcessHeap(),0,traveler_actor);traveler_actor=NULL;
    free(actor);
    puts("Actor runtime: native phase ordering, paired receive, owner thread, retirement, private scope closure, duplicate suppression and quarantine passed; native actor leaves intercepted.");
}
static void sand_capacity_cases(void) {
    (void)traveler_sand_consume;
    unsigned char *renderer=calloc(1,0xece0);assert(renderer);
    _Alignas(16) unsigned char items[8*64],position[16],effect[16];
    memset(position,0x35,16);memset(effect,0x71,16);
    struct runtime_context context={.thread=11};
    struct jc_actor_runtime actor={.owner=11,.ops={.thread=runtime_thread,.context=&context}};
    traveler_actor=&actor;traveler_object=1;
    original_sand_append=(jc_sand_append_fn)(image+0x10f0a0);
    for(unsigned kind=0;kind<10;++kind) {
        size_t offset=0xb310+kind*0x5c8;put(renderer,offset,(uintptr_t)items);
        for(unsigned count=0;count<8;++count) {
            memset(items,0xa5,sizeof(items));memcpy(renderer+offset+8,&count,4);
            traveler_sand_append((uintptr_t)renderer,kind,position,effect,1,2,3,4,5,6);
            uint32_t after=0;memcpy(&after,renderer+offset+8,4);
            unsigned dropped=kind>=8 && count>=6;
            assert(after==count+!dropped);
            for(unsigned i=0;i<8;++i) {
                if(!dropped && i==count) {
                    assert(!memcmp(items+i*64,position,16));assert(!memcmp(items+i*64+16,effect,16));
                    for(unsigned j=0;j<6;++j) {float v;memcpy(&v,items+i*64+32+j*4,4);assert(v==j+1);}
                } else for(unsigned j=0;j<64;++j)assert(items[i*64+j]==0xa5);
            }
        }
    }
    for(unsigned mode=0;mode<3;++mode) {
        unsigned count=6;memcpy(renderer+0xe158,&count,4);put(renderer,0xe150,(uintptr_t)items);
        traveler_object=mode==0?0:1;traveler_actor=mode==1?NULL:&actor;context.thread=mode==2?12:11;
        traveler_sand_append((uintptr_t)renderer,8,position,effect,1,2,3,4,5,6);
        memcpy(&count,renderer+0xe158,4);assert(count==6);
    }
    traveler_actor=&actor;traveler_object=1;context.thread=11;
    sand_read_failure=(uintptr_t)renderer+0xe158;
    unsigned count=0;memcpy(renderer+0xe158,&count,4);memset(items,0xa5,sizeof(items));
    traveler_sand_append((uintptr_t)renderer,8,position,effect,1,2,3,4,5,6);
    memcpy(&count,renderer+0xe158,4);assert(!count && traveler_sand_unreadable==1);
    for(unsigned i=0;i<sizeof(items);++i)assert(items[i]==0xa5);
    sand_read_failure=0;
    traveler_actor=NULL;traveler_object=0;free(renderer);
    puts("Sand capacity: original native append executed through production guard; six-entry bounds before activation and across owner-thread changes, other kinds and ten-argument ABI passed.");
}
static unsigned sampler_lookup_count;
static unsigned char sampler_links[3][0x20],sampler_payloads[3][0xd0],sampler_textures[3][0x120];
static void ABI sampler_lookup(uintptr_t manager,uintptr_t name,uintptr_t holder) {
    (void)manager;(void)name;unsigned i=sampler_lookup_count++;assert(i<3);
    put((void *)holder,8,(uintptr_t)sampler_links[i]);
    put(sampler_links[i],0x10,(uintptr_t)sampler_payloads[i]);
    put(sampler_payloads[i],0xc8,(uintptr_t)sampler_textures[i]);
}
static void sampler_cases(void) {
    unsigned char *barn=calloc(1,0x66a000);assert(barn);
    for(unsigned role=4;role<=5;++role)for(unsigned initial=0;initial<5;++initial) {
        unsigned char expected[3][0x120];
        memset(sampler_textures,0,sizeof(sampler_textures));
        for(unsigned i=0;i<3;++i) {
            sampler_textures[i][0xa]=sampler_textures[i][0xb]=(unsigned char)initial;
            memcpy(expected[i],sampler_textures[i],0x120);
            assert(jc_cloth_atlas_sampler_prepare(expected[i],role,i));
        }
        sampler_lookup_count=0;
        ((void (ABI *)(uintptr_t,unsigned,uintptr_t,uintptr_t,uintptr_t,uintptr_t))(image+0x8ab10))
            ((uintptr_t)barn,role,0,0,0,0);
        assert(sampler_lookup_count==3 && !memcmp(expected,sampler_textures,sizeof(expected)));
    }
    free(barn);
    puts("Atlas sampler: private robe/scarf addressing matches original native selector for five initial modes; registry lookup intercepted, GPU untested.");
}
static unsigned local_peer_writes,local_peer_fail;
static int local_peer_read(void *ctx,uint64_t address,void *out,size_t bytes) {
    (void)ctx;memcpy(out,(void *)(uintptr_t)address,bytes);return 1;
}
static int local_peer_write(void *ctx,uint64_t address,const void *in,size_t bytes) {
    (void)ctx;if(++local_peer_writes==local_peer_fail)return 0;
    memcpy((void *)(uintptr_t)address,in,bytes);return 1;
}
static void local_peer_cases(void) {
    unsigned char *barn=calloc(1,JC_RECEIVER_BYTES),*before=malloc(JC_RECEIVER_BYTES);
    assert(barn && before);struct jc_actor_sender sender={0};
    sender.body_scale=.1f;sender.chirp_shape[1]=1;sender.chapter=2;
    jc_receiver_store(sender.state.bytes,10.f);
    struct jc_local_peer_packet packet;
    for(unsigned fail=1;fail<=39;++fail) {
        memset(barn,0x5a,JC_RECEIVER_BYTES);memcpy(before,barn,JC_RECEIVER_BYTES);
        assert(!jc_local_peer_packet_make(&sender,1,6,7,&packet));
        local_peer_writes=0;local_peer_fail=fail;
        assert(jc_local_peer_publish(local_peer_read,local_peer_write,NULL,(uintptr_t)barn,&packet)==3);
        assert(!memcmp(before,barn,JC_RECEIVER_BYTES));
    }
    memset(barn,0,JC_RECEIVER_BYTES);local_peer_fail=0;
    put(barn,0x68,(uintptr_t)barn+0x146f0);
    assert(!jc_local_peer_packet_make(&sender,1,6,7,&packet));
    assert(!jc_local_peer_publish(local_peer_read,local_peer_write,NULL,(uintptr_t)barn,&packet));
    assert(!memcmp(barn+0xb0,(unsigned char[0x6c]){0},0x6c));
    unsigned char game[0x70]={0},clock[0x10]={0};put(game,0x68,(uintptr_t)clock);
    jc_receiver_store(clock+0xc,1.f/60);
    for(unsigned tick=0;tick<6;++tick) {
        jc_receiver_store(barn+0x28dc4,jc_receiver_float(barn+0x28dc4)+1.f/60);
        ((void (ABI *)(void *,void *))(image+0x118730))(barn,game);
    }
    assert(jc_receiver_float(barn+0x14700)==10.f);
    uint32_t outfit,symbol;memcpy(&outfit,barn+0x14760,4);memcpy(&symbol,barn+0x14764,4);
    assert(outfit==6 && symbol==7 && barn[0xa2]==1 && barn[0xa4]==0);
    sender.chapter=99;assert(jc_local_peer_packet_make(&sender,1,6,7,&packet));
    free(before);free(barn);
    puts("Local native peer: native receiver consumed independent position/outfit/symbol; all 39 partial publication failures rolled back, local player preserved. Native animation/render and coexistence remain untested.");
}
int main(void) {
    image=mmap(NULL,0x4000000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);assert(image!=MAP_FAILED);
    load_code(image);
    image[0xa7bfb]=0xc3;
    image[0x11865a]=0xc3;image[0xa9ce5]=0xc3;
    memcpy(image+0x246522,"\xb8\x01\x00\x00\x00\xc3",6);
    memcpy(image+0x24652d,"\x31\xc0\xc3",3);
    thunk(0x273a00,lookup);thunk(0x274d70,commit);thunk(0x263aa0,mesh);
    thunk(0xa54a0,input_call);
    thunk(0xa6350,update_call);
    thunk(0x263990,end_bone_call);thunk(0x8ee60,sampler_lookup);
    load_runtime_constants(image);
    assert(!mprotect(image,0x700000,PROT_READ|PROT_EXEC));
    scale_cases();hsv_cases();sand_capacity_cases();sampler_cases();local_peer_cases();
    unsigned char *gate_actor=calloc(1,0x9b48),*gate_collection=calloc(1,0x91b4);
    unsigned char gate_definition[0x14]={0};uint32_t gate_count=2;
    assert(gate_actor && gate_collection);memcpy(gate_definition+0x10,&gate_count,4);
    assert(!reference_cloth_gate(gate_actor,gate_definition,0,image+0x2464f8));
    assert(!reference_cloth_gate(gate_actor,gate_definition,1,image+0x2464f8));
    gate_actor[0x1479]=1;
    assert(reference_cloth_gate(gate_actor,gate_definition,0,image+0x2464f8)==1);
    put(gate_actor,0x9b40,(uintptr_t)gate_collection);
    memcpy(gate_collection+0x91b0,&gate_count,4);
    assert(!reference_cloth_gate(gate_actor,gate_definition,0,image+0x2464f8));
    assert(reference_cloth_gate(gate_actor,gate_definition,1,image+0x2464f8)==1);
    free(gate_actor);free(gate_collection);
    puts("Native cloth allocation gate: disabled, empty, complete and changed-mesh branches passed; allocator intercepted.");
    glyph_init_cases();glyph_trace_cases();
    unsigned char *barn=calloc(1,0x28dd0);assert(barn);
    _Alignas(16) unsigned char render[0x200],copy[0x200];
    unsigned char links[4][0x20],owners[4][0x20];
    memset(links,0,sizeof(links));memset(owners,0,sizeof(owners));
    for(unsigned k=0;k<4;++k) {
        payloads[k]=(uintptr_t)owners[k]+0x10;
        put(links[k],0x10,(uintptr_t)owners[k]);put(owners[k],0x18,payloads[k]);
    }
    for(unsigned flags=0;flags<64;++flags) {
        memset(barn,0,0x28dd0);memset(render,0xa5,sizeof(render));
        put(render,0x18,0x123400);put(render,0x60,0x567800);
        unsigned char *actors[2]={barn+0xb0,barn+0x146f0};
        for(unsigned n=0;n<2;++n) {
            put(actors[n],0,(uintptr_t)image+0x5d0380);
            put(actors[n],0x860,(uintptr_t)links[2*n]);put(actors[n],0xf00,(uintptr_t)links[2*n+1]);
            actors[n][0xd40]=(flags>>(n*2))&1;actors[n][0x13e0]=(flags>>(n*2+1))&1;
            for(unsigned i=0;i<64;++i)actors[n][0x690+i]=(unsigned char)(i+n*100);
        }
        barn[0xa2]=(flags>>4)&1;barn[0xa4]=(flags>>5)&1;
        count=0;memset(uniforms,0,sizeof(uniforms));
        ((void (ABI *)(void *,void *))(image+0x117790))(barn,render);
        struct event expected[32];memcpy(expected,events,sizeof(expected));unsigned expected_count=count;
        uint32_t expected_uniforms[4];memcpy(expected_uniforms,uniforms,sizeof(uniforms));
        memcpy(copy,render,sizeof(copy));
        count=0;memset(uniforms,0,sizeof(uniforms));
        for(unsigned n=0;n<2;++n) if(!n || barn[0xa2] || barn[0xa4]) {
            struct jc_actor_submission out={0};
            assert(!jc_actor_submit(read_memory,NULL,(uintptr_t)image,(uintptr_t)actors[n],(uintptr_t)render,&out));
            assert(!memcmp(out.matrix,render+0x180+n*0x40,64));
            assert(out.meshes==1u+(actors[n][0xd40]!=0)+(actors[n][0x13e0]!=0));
        }
        assert(count==expected_count && !memcmp(expected,events,count*sizeof(*events)));
        assert(!memcmp(expected_uniforms,uniforms,sizeof(uniforms)));
        assert(!memcmp(copy,render,sizeof(copy)));
    }
    unsigned char game[0x70]={0},clock[0x20]={0},snapshot[JC_RECEIVER_ACTOR_BYTES];
    put(game,0x68,(uintptr_t)clock);
    for(unsigned n=0;n<512;++n) {
        memset(barn,0,0x28dd0);memset(snapshot,0xa5,sizeof(snapshot));
        struct jc_actor_input input={.packet_age=(n%23)*0.03f,.inactive=(n>>7)&1,.interaction_gate=(n>>6)&1};
        float incoming[4]={((int)(n%17)-8)*30.25f,1.0f,((int)(n%13)-6)*0.25f,0};
        float position[4]={1,2,3,0};memcpy(snapshot+0x10,position,16);
        input.axes[0]=(n%5)*0.125f;input.axes[1]=(n%7)*-0.15f;
        input.direction[0]=(n%3)*0.2f;input.direction[1]=0;input.direction[2]=0.9f;
        float delay=(n%11)*0.065f,dt=1.0f/(15+n%90);
        memcpy(barn+0x146f0,snapshot,sizeof(snapshot));
        memcpy(barn+0x28d30,incoming,16);memcpy(barn+0x80,input.axes,16);memcpy(barn+0x90,input.direction,16);
        jc_receiver_store(barn+0x28da0,delay);jc_receiver_store(barn+0x28dc4,input.packet_age);
        jc_receiver_store(clock+0x10,dt);barn[0xa4]=input.inactive;
        input_calls=0;
        reference_input(barn,game,barn+0xb0,input.interaction_gate,image+(input.inactive?0x1183f4:0x118401));
        assert(input_calls==1 && captured_actor==(uintptr_t)(barn+0x146f0) && captured_game==(uintptr_t)game &&
            captured_local==(uintptr_t)(barn+0xb0) && captured_gate==input.interaction_gate);
        float expected_axes[4],expected_direction[4];memcpy(expected_axes,captured_axes,16);memcpy(expected_direction,captured_direction,16);
        assert(!jc_actor_correct_input((uintptr_t)image,snapshot,sizeof(snapshot),incoming,&input,&delay,dt));
        if (memcmp(snapshot,barn+0x146f0,sizeof(snapshot)) || memcmp(&delay,barn+0x28da0,4)) {
            fprintf(stderr,"Remote correction mismatch case %u\n",n);abort();
        }
        assert(!jc_actor_native_input((uintptr_t)image,(uintptr_t)snapshot,(uintptr_t)game,(uintptr_t)(barn+0xb0),&input));
        assert(!memcmp(expected_axes,captured_axes,16) && !memcmp(expected_direction,captured_direction,16));
        update_calls=0;
        jc_actor_native_update((uintptr_t)image,(uintptr_t)snapshot,(uintptr_t)game,input.direction,(unsigned char)(n&255));
        assert(update_calls==1 && captured_actor==(uintptr_t)snapshot && captured_game==(uintptr_t)game &&
            captured_gate==(n&255) && !memcmp(input.direction,captured_direction,16));
    }
    unsigned char capture_game[0x1e8]={0};put(capture_game,0x1e0,(uintptr_t)barn);
    memset(barn,0,0x28dd0);put(barn,0,(uintptr_t)image+0x5da340);put(barn,0xb0,(uintptr_t)image+0x5d0380);
    for(unsigned n=0;n<21;++n) {
        uint32_t appearance[4]={3,9,n%7,n};memcpy(barn+0x118,appearance,16);
        uint32_t chapter=n%15;memcpy(barn+0x14,&chapter,4);
        float axes[4]={0.1f*n,0.2f,0,0},direction[4]={0,0,1,0};
        memcpy(barn+0x30,axes,16);memcpy(barn+0x40,direction,16);
        jc_receiver_store(barn+0x20,0.125f);jc_receiver_store(barn+0x24,0.25f);
        jc_receiver_store(barn+0x14e0,0.5f+n*0.1f);
        jc_receiver_store(barn+0x1520,.05f*n);jc_receiver_store(barn+0x1f14,1.f+.01f*n);
        for(unsigned j=0;j<3;++j)jc_receiver_store(barn+0x14f8+j*8,.01f*n+j);
        struct jc_actor_sender sender={0};
        assert(!jc_actor_capture_sender(read_memory,NULL,(uintptr_t)image,(uintptr_t)capture_game,&sender));
        assert(!memcmp(sender.state.bytes+0x58,appearance,16) && sender.chapter==chapter &&
            !memcmp(sender.axes,axes,16) && !memcmp(sender.direction,direction,16));
        assert(sender.body_scale==0.5f+n*0.1f);
        assert(sender.chirp_shape[0]==.05f*n && sender.chirp_shape[1]==1.f+.01f*n);
        for(unsigned j=0;j<3;++j)assert(sender.glow_targets[j]==.01f*n+j);
        assert(sender.scalars[0]==0.125f && sender.scalars[1]==0.25f);
    }
    struct jc_actor_sender clean;
    jc_receiver_store(barn+0xcc,NAN);jc_receiver_store(barn+0xdc,INFINITY);
    assert(!jc_actor_capture_sender(read_memory,NULL,(uintptr_t)image,(uintptr_t)capture_game,&clean));
    assert(jc_receiver_float(clean.state.bytes+0xc)==0 && jc_receiver_float(clean.state.bytes+0x1c)==0);
    assert(isnan(jc_receiver_float(barn+0xcc)) && isinf(jc_receiver_float(barn+0xdc)));
    jc_receiver_store(barn+0xc0,NAN);
    assert(jc_actor_capture_sender(read_memory,NULL,(uintptr_t)image,(uintptr_t)capture_game,&clean)==JC_LAYOUT_RANGE);
    jc_receiver_store(barn+0xc0,0);
    struct jc_actor_sender untouched,sender;memset(&untouched,0xa5,sizeof(untouched));sender=untouched;
    struct capture_context capture={.address=(uintptr_t)barn+0x30};
    assert(jc_actor_capture_sender(capture_read,&capture,(uintptr_t)image,(uintptr_t)capture_game,&sender)==JC_LAYOUT_CHANGED);
    assert(!memcmp(&sender,&untouched,sizeof(sender)));
    capture.calls=0;capture.mode=1;
    assert(jc_actor_capture_sender(capture_read,&capture,(uintptr_t)image,(uintptr_t)capture_game,&sender)==JC_LAYOUT_READ);
    jc_receiver_store(barn+0x30,NAN);
    assert(jc_actor_capture_sender(read_memory,NULL,(uintptr_t)image,(uintptr_t)capture_game,&sender)==JC_LAYOUT_RANGE);
    jc_receiver_store(barn+0x30,0);uint32_t invalid_chapter=32;memcpy(barn+0x14,&invalid_chapter,4);
    assert(jc_actor_capture_sender(read_memory,NULL,(uintptr_t)image,(uintptr_t)capture_game,&sender)==JC_LAYOUT_RANGE);
    put(barn,0,0x123456);
    assert(jc_actor_capture_sender(read_memory,NULL,(uintptr_t)image,(uintptr_t)capture_game,&sender)==JC_LAYOUT_TYPE);
    assert(!memcmp(&sender,&untouched,sizeof(sender)));
    publication_cases(barn);
    runtime_cases(barn);
    free(barn);munmap(image,0x2900000);
    puts("Actor submission: 64 original-dispatch differential cases passed; native material/mesh leaf calls intercepted. Render matrices remain participant-owned; game runtime untested.");
    puts("Remote input: 512 original-branch differential cases passed, including native catch-up, snap, delay decay and input attenuation; a54a0 leaf intercepted.");
    puts("Numeric publication: original receiver execution, sender appearance admission, exact numeric writes, replay refusal, partial-write rollback and ownership-loss quarantine passed.");
    return 0;
}
