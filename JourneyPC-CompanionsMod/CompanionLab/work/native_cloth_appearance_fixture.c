
#include <assert.h>
#include <stdio.h>
#include "native_companion_cloth.h"
#define jc_shader_inputs_read actual_shader_inputs_read
#define jc_shader_storage_read actual_shader_storage_read
#include "native_shader_inputs.h"
#undef jc_shader_inputs_read
#undef jc_shader_storage_read
typedef int32_t LONG;
struct callback_read_budget { uint64_t deadline; unsigned reads,fast; int expired; struct jc_read_batch *batch; };
struct jc_inspection { struct callback_read_budget *budget; };
static LONG JCProbeCompanionFeatures[40];
static LONG InterlockedExchange(volatile LONG *p,LONG n) {LONG old=*p;*p=n;return old;}
static int inspection_read(void *ctx,uint64_t at,void *out,size_t bytes) {(void)ctx;(void)at;(void)out;(void)bytes;return 1;}
static struct jc_cloth_lease native_cloth_lease;
static struct {struct {uint64_t object;} s_initialized_life;} body_contexts[2];
static struct jc_shader_inputs shader_inputs[8];
static unsigned char shader_data[8][256];
static uintptr_t material_addresses[8];
static uint64_t environment[2]={0x110000,0x120000};
static const unsigned widths[]={4,16,64};
static int appearance_calls,texture_calls,writes,read_failure,layout_change,environment_change,write_failure,texture_mutation,expire_after_call;
static struct callback_read_budget budget;
static uint64_t GetTickCount64(void) {return 1000;}
static void *GetModuleHandleW(void *unused) {(void)unused;return 0;}
static void flight_mark(int a,int b) {(void)a;(void)b;}
static int callback_read(void *c,uint64_t at,void *out,size_t bytes) {
    (void)c;assert(at==0x300000+0x646d30 && bytes==16);
    memcpy(out,environment,16);return 1;
}
static int jc_shader_inputs_read(jc_layout_reader read,void *c,uint64_t at,struct jc_shader_inputs *out) {
    (void)read;(void)c;
    if(read_failure)return 1;
    for(unsigned i=0;i<8;++i)if(at==material_addresses[i]){*out=shader_inputs[i];return 0;}
    assert(0);return 1;
}
static int jc_shader_storage_read(jc_layout_reader read,void *c,uint64_t at,struct jc_shader_inputs *out) {
 return jc_shader_inputs_read(read,c,at,out);
}
static int evaluation_writable(void *c,uint64_t at,size_t bytes) {(void)c;(void)at;return bytes>0;}
static int companion_cloth_write(void *c,uint64_t at,const void *data,size_t bytes) {
    struct callback_read_budget *b=c;assert(!b->expired && b->deadline>GetTickCount64());
    if(write_failure)return 0;
    assert(writes<3 && bytes==widths[writes] && at==shader_inputs[0].buffer+shader_inputs[0].offsets[writes]);
    memcpy(shader_inputs[0].values[writes],data,bytes);++writes;return 1;
}
static void fake_appearance(uintptr_t dude,uintptr_t barn) {
    assert(dude==0x200000 && barn==0x300000);++appearance_calls;
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<widths[i]/4;++j)shader_inputs[0].values[i][j]+=7.25f;
    if(layout_change)++shader_inputs[0].offsets[2];
    if(environment_change)++environment[0];
    if(expire_after_call)budget.expired=1;
}
static void fake_texture(uintptr_t material,const char *name,uintptr_t texture) {
    assert(texture_calls<3 && material==material_addresses[texture_calls] && (uintptr_t)name==0x5cf7f0);
    assert(texture==0x110000);++texture_calls;
    if(texture_mutation && texture_calls==3)shader_inputs[0].values[1][0]+=1;
}
static int companion_cloth_atlas(struct callback_read_budget *b,uintptr_t image,uint64_t dude,uint64_t barn) {(void)b;(void)image;(void)dude;(void)barn;assert(0);return 0;}
static void fake_unused(void) {assert(0);}
#define JC_RVA_ClothRelease ((uintptr_t)fake_unused)
#define JC_RVA_DudeClothInit ((uintptr_t)fake_unused)
#define JC_RVA_DudeClothAttach ((uintptr_t)fake_unused)
#define JC_RVA_DudeClothCollide ((uintptr_t)fake_unused)
#define JC_RVA_DudeClothAppearance ((uintptr_t)fake_appearance)
#define JC_TEST_MATERIAL_TEXTURE_SET ((uintptr_t)fake_texture)
static void reset_appearance(void) {
    appearance_calls=texture_calls=writes=read_failure=layout_change=environment_change=write_failure=texture_mutation=expire_after_call=0;
    budget=(struct callback_read_budget){.deadline=1500};environment[0]=0x110000;environment[1]=0x120000;
    native_cloth_lease.players=0x400000;body_contexts[1].s_initialized_life.object=0x500000;
    const uintptr_t addresses[]={0x2001e0,0x300000+0x666ee8+4*0x630,0x300000+0x666ee8+5*0x630,
        0x400290,0x4148d0,0x300000+0x666ee8+2*0x630,0x300000+0x666ee8+3*0x630,0x5001e0};
    memcpy(material_addresses,addresses,sizeof(addresses));memset(shader_inputs,0,sizeof(shader_inputs));
    for(unsigned i=0;i<8;++i){
        struct jc_shader_inputs *s=shader_inputs+i;s->buffer=(uintptr_t)shader_data[i];s->bytes=256;
        s->payload=0x600000+i*0x100;s->instance=0x700000+i*0x100;s->shader=0x800000+i*0x100;s->found=s->finite=7;
        s->offsets[0]=4;s->offsets[1]=16;s->offsets[2]=32;
        for(unsigned j=0;j<3;++j){s->sizes[j]=widths[j];for(unsigned k=0;k<widths[j]/4;++k)s->values[j][k]=(float)(i*100+j*20+k);}
    }
}
static int companion_cloth_call(void *opaque,unsigned op,uint64_t a,uint64_t b,uint64_t c) {
    struct callback_read_budget *budget=opaque;
    if (op!=JC_CLOTH_RELEASE) InterlockedExchange(&JCProbeCompanionFeatures[11],1);
    if (budget->expired || GetTickCount64()>=budget->deadline) return 0;
    uintptr_t image=(uintptr_t)GetModuleHandleW(NULL);
    flight_mark(45,(LONG)op);
    if (op==JC_CLOTH_RELEASE) ((void (*)(uintptr_t))(image+JC_RVA_ClothRelease))((uintptr_t)a);
    else if (op==JC_CLOTH_INIT) ((void (*)(uintptr_t,uintptr_t))(image+JC_RVA_DudeClothInit))((uintptr_t)a,(uintptr_t)b);
    else if (op==JC_CLOTH_ATLAS) {
        if (!companion_cloth_atlas(budget,image,a,b)) return 0;
    }
    else if (op==JC_CLOTH_APPEARANCE) {
        struct jc_inspection inspection={.budget=budget};
        InterlockedExchange(&JCProbeCompanionFeatures[11],2);
        uint64_t environment[2],again[2];
        if (!callback_read(budget,b+0x646d30,environment,sizeof(environment)) || !environment[0] || !environment[1]) return 0;
        const uintptr_t materials[]={(uintptr_t)a+0x1e0,(uintptr_t)b+0x666ee8+4*0x630,(uintptr_t)b+0x666ee8+5*0x630,
            (uintptr_t)native_cloth_lease.players+0x290,(uintptr_t)native_cloth_lease.players+0x148d0,
            (uintptr_t)b+0x666ee8+2*0x630,(uintptr_t)b+0x666ee8+3*0x630,
            (uintptr_t)body_contexts[1].s_initialized_life.object+0x1e0};
        struct jc_shader_inputs inputs[8],held,after;
        for (unsigned i=0;i<8;++i) {
            InterlockedExchange(&JCProbeCompanionFeatures[11],10+(LONG)i);
            int storage_code=i ? jc_shader_storage_read(inspection_read,&inspection,materials[i],&inputs[i]) :
                jc_shader_inputs_read(inspection_read,&inspection,materials[i],&inputs[i]);
            InterlockedExchange(&JCProbeCompanionFeatures[12],storage_code);
            if (storage_code) return 0;
            if (i<3 && !evaluation_writable(budget,inputs[i].buffer,inputs[i].bytes)) {
                InterlockedExchange(&JCProbeCompanionFeatures[12],16); return 0;
            }
        }
        InterlockedExchange(&JCProbeCompanionFeatures[11],20);
        held=inputs[0];
        if ((held.found&7)!=7 || (held.finite&7)!=7) return 0;
        InterlockedExchange(&JCProbeCompanionFeatures[11],21);
        for (unsigned i=0;i<3;++i) for (unsigned j=0;j<8;++j)
            if (i!=j && inputs[i].buffer<inputs[j].buffer+inputs[j].bytes &&
                inputs[j].buffer<inputs[i].buffer+inputs[i].bytes) return 0;
        ((void (*)(uintptr_t,uintptr_t))(image+JC_RVA_DudeClothAppearance))((uintptr_t)a,(uintptr_t)b);
        InterlockedExchange(&JCProbeCompanionFeatures[11],22);
        struct callback_read_budget restore_budget={.deadline=GetTickCount64()+500};
        if (jc_shader_inputs_read(callback_read,&restore_budget,materials[0],&after) ||
            after.buffer!=held.buffer || after.payload!=held.payload || after.instance!=held.instance ||
            after.shader!=held.shader || after.bytes!=held.bytes ||
            memcmp(after.offsets,held.offsets,3*sizeof(*held.offsets)) ||
            memcmp(after.sizes,held.sizes,3*sizeof(*held.sizes))) return 0;
        InterlockedExchange(&JCProbeCompanionFeatures[11],23);
        const unsigned sizes[]={4,16,64};
        for (unsigned i=0;i<3;++i)
            if (!companion_cloth_write(&restore_budget,held.buffer+held.offsets[i],held.values[i],sizes[i])) return 0;
        void (*set_texture)(uintptr_t,const char *,uintptr_t)=(void *)(image+JC_TEST_MATERIAL_TEXTURE_SET);
        for (unsigned i=0;i<3;++i) set_texture(materials[i],(const char *)(image+0x5cf7f0),(uintptr_t)environment[0]);
        InterlockedExchange(&JCProbeCompanionFeatures[11],24);
        if (!callback_read(&restore_budget,b+0x646d30,again,sizeof(again)) || memcmp(environment,again,sizeof(environment)) ||
            jc_shader_inputs_read(callback_read,&restore_budget,materials[0],&after) ||
            after.buffer!=held.buffer || after.payload!=held.payload || after.instance!=held.instance ||
            after.shader!=held.shader || after.bytes!=held.bytes ||
            memcmp(after.offsets,held.offsets,3*sizeof(*held.offsets)) ||
            memcmp(after.sizes,held.sizes,3*sizeof(*held.sizes))) return 0;
        for (unsigned i=0;i<3;++i) if (memcmp(after.values[i],held.values[i],sizes[i])) return 0;
    }
    else if (op==JC_CLOTH_ATTACH || op==JC_CLOTH_COLLIDE)
        ((void (*)(uintptr_t,uintptr_t,uintptr_t))(image+(op==JC_CLOTH_ATTACH ? JC_RVA_DudeClothAttach : JC_RVA_DudeClothCollide)))((uintptr_t)a,(uintptr_t)b,(uintptr_t)c);
    else return 0;
    if (op!=JC_CLOTH_RELEASE) InterlockedExchange(&JCProbeCompanionFeatures[11],0);
    flight_mark(46,(LONG)op); return 1;
}

static int invoke(void) {return companion_cloth_call(&budget,JC_CLOTH_APPEARANCE,0x200000,0x300000,0);}
int main(void) {
    unsigned scenarios=0;
    reset_appearance();struct jc_shader_inputs saved=shader_inputs[0];assert(invoke());++scenarios;
    assert(appearance_calls==1 && writes==3 && texture_calls==3 && !memcmp(saved.values,shader_inputs[0].values,sizeof(saved.values)));
    for(unsigned owner=0;owner<3;++owner)for(unsigned other=0;other<8;++other)if(owner!=other){
        reset_appearance();shader_inputs[owner].buffer=shader_inputs[other].buffer+128;
        assert(!invoke() && !appearance_calls && !writes && !texture_calls);++scenarios;
    }
    reset_appearance();read_failure=1;assert(!invoke() && !appearance_calls);++scenarios;
    reset_appearance();layout_change=1;assert(!invoke() && appearance_calls==1 && !writes && !texture_calls);++scenarios;
    reset_appearance();environment_change=1;assert(!invoke() && appearance_calls==1 && writes==3 && texture_calls==3);++scenarios;
    reset_appearance();write_failure=1;assert(!invoke() && appearance_calls==1 && !writes && !texture_calls);++scenarios;
    reset_appearance();texture_mutation=1;assert(!invoke() && appearance_calls==1 && writes==3 && texture_calls==3);++scenarios;
    reset_appearance();expire_after_call=1;assert(invoke() && budget.expired && writes==3 && texture_calls==3);++scenarios;
    reset_appearance();budget.expired=1;assert(!invoke() && !appearance_calls);++scenarios;
    printf("native cloth appearance: %u extracted-production buffer-disjointness, exact restoration and texture-binding scenarios passed\n",scenarios);
}
