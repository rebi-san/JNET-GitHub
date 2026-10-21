
#define main appearance_component_main
#include "test_native_appearance.c"
#undef main
#define JC_RVA_MaterialScalarLookup 0x273a00
#define JC_FRAME_OWNERSHIP 90
struct jc_actor_identity {uint64_t image,barn,actor;};
struct jc_cloth_graphics_token {unsigned held;};
struct body_live {unsigned backend;};
static struct body_live body_live,*traveler_live=&body_live;
static uint64_t traveler_object;
static unsigned traveler_actor_materials,body_thread=7,body_destroyed,body_locked,body_independent=1;
static struct jc_actor_identity body_id;
static unsigned GetCurrentThreadId(void){return body_thread;}
static void message(const char *line){assert(line && *line);puts(line);}
static int own_read(void *ctx,uint64_t at,void *out,size_t n) {
    (void)ctx;
    if(at==body_id.barn+0xb0+0x2238 && n==8){uint64_t definition=base+0x3000;memcpy(out,&definition,8);return 1;}
    if((at==body_id.barn+0xb0+0x70 || at==body_id.actor+0x70) && n==4){uint32_t outfit=4;memcpy(out,&outfit,4);return 1;}
    if(at==body_id.actor+0x168 && n==8){uint64_t model=body_destroyed?0:1;memcpy(out,&model,8);return 1;}
    if(at>=body_id.barn+0x148d0 && at<body_id.barn+0x148d0+0x80)at=materials[1]+(at-(body_id.barn+0x148d0));
    return read_memory(NULL,at,out,n);
}
static int traveler_live_identity(const struct jc_actor_identity *id){return id==&body_id;}
static int jc_actor_material_independent(jc_layout_reader read,void *ctx,const struct jc_actor_identity *id,void *proof) {
    (void)read;(void)ctx;assert(id==&body_id && proof==&traveler_actor_materials);return !body_independent;
}
static int jc_cloth_live_graphics_enter(void *s,struct jc_cloth_graphics_token *token){
    assert(s==&body_live.backend && !body_locked);token->held=body_locked=1;return 1;
}
static int jc_cloth_live_graphics_leave(void *s,struct jc_cloth_graphics_token *token){
    assert(s==&body_live.backend && body_locked && token->held);token->held=body_locked=0;return 1;
}
struct jc_cloth_atlas_native {uint64_t image;jc_layout_reader read;void *ctx,*device;};
static int jc_cloth_atlas_native_hold(void *ctx,const uint64_t pair[2]){(void)ctx;assert(body_locked);return body_hold(NULL,pair);}
static void jc_cloth_atlas_native_drop(void *ctx,const uint64_t pair[2]){(void)ctx;assert(body_destroyed);body_drop(NULL,pair);}
static float *body_lookup(uintptr_t payload,const char *name){
    assert(body_locked && payload==base+0x20300);
    for(unsigned i=0;i<13;++i)if(!strcmp(name,jc_appearance_names[i]))return (float *)(memory+0x21004+i*32);
    return NULL;
}
static int jc_local_windows_write(uint64_t at,const void *value,size_t size){
    assert(body_locked && at>=(uintptr_t)memory && at+size<=(uintptr_t)(memory+sizeof(memory)));
    memcpy((void *)(uintptr_t)at,value,size);return 1;
}
static uintptr_t body_vector_set(uintptr_t m,uintptr_t name,uintptr_t value){assert(body_locked);return set_vector(NULL,m,(const char *)name,(const float *)value);}
static uintptr_t body_matrix_set(uintptr_t m,uintptr_t name,uintptr_t value){(void)m;(void)name;(void)value;assert(!"matrix setter called for vector");return 0;}
static uintptr_t body_texture_set(uintptr_t m,uintptr_t name,uintptr_t value){assert(body_locked);return set_texture(NULL,m,(const char *)name,value);}
static uintptr_t (*original_cloth_setter[3])(uintptr_t,uintptr_t,uintptr_t)={body_vector_set,body_matrix_set,body_texture_set};
static struct jc_body_appearance_lease traveler_body_lease;
static uint64_t traveler_body_image,traveler_body_thread;
static int traveler_body_hold(void *ctx,const uint64_t pair[2]) {
    struct jc_cloth_atlas_native native={traveler_body_image,own_read,NULL,NULL};
    (void)ctx;return jc_cloth_atlas_native_hold(&native,pair);
}
static void traveler_body_drop(void *ctx,const uint64_t pair[2]) {
    struct jc_cloth_atlas_native native={traveler_body_image,own_read,NULL,NULL};
    (void)ctx;jc_cloth_atlas_native_drop(&native,pair);
}
static int traveler_body_texture(void *ctx,uint64_t material,const char *name,uint64_t texture) {
    (void)ctx;
    if(material!=traveler_object+0x1e0 || !original_cloth_setter[2])return 0;
    original_cloth_setter[2](material,(uintptr_t)name,texture);return 1;
}
static int traveler_body_vector(void *ctx,uint64_t material,const char *name,const float value[4]) {
    (void)ctx;
    if(material!=traveler_object+0x1e0 || !original_cloth_setter[0])return 0;
    original_cloth_setter[0](material,(uintptr_t)name,(uintptr_t)value);return 1;
}
static int traveler_body_scalar(void *ctx,uint64_t material,const char *name,float value) {
    (void)ctx;uint64_t payload=0;
    if(material!=traveler_object+0x1e0 || jc_material_ready(own_read,NULL,material,&payload))return 0;
    float *(*lookup)(uintptr_t,const char *)=(void *)(uintptr_t)(traveler_body_image+JC_RVA_MaterialScalarLookup);
    float *destination=lookup(payload,name);
    return destination && jc_local_windows_write((uintptr_t)destination,&value,4);
}
static const struct jc_body_appearance_ops traveler_body_ops={
    {traveler_body_scalar,traveler_body_vector,traveler_body_texture},traveler_body_hold,traveler_body_drop};
static int traveler_body_prepare(const struct jc_actor_identity *id) {
    if(traveler_body_lease.applied)return traveler_body_lease.material==id->actor+0x1e0 &&
        traveler_body_thread==GetCurrentThreadId();
    if(traveler_body_lease.material || !traveler_live_identity(id) ||
        jc_actor_material_independent(own_read,NULL,id,&traveler_actor_materials))return 0;
    uint64_t definition=0;uint32_t local_outfit=0,own_outfit=0;
    if(!own_read(NULL,id->barn+0xb0+0x2238,&definition,8) || !definition ||
        !own_read(NULL,id->barn+0xb0+0x70,&local_outfit,4) ||
        !own_read(NULL,id->actor+0x70,&own_outfit,4) || local_outfit!=own_outfit)return 0;
    const uint64_t materials[3]={id->barn+0x290,id->barn+0x148d0,id->actor+0x1e0};
    traveler_body_image=id->image;traveler_body_thread=GetCurrentThreadId();
    struct jc_cloth_graphics_token token={0};
    if(!jc_cloth_live_graphics_enter(&traveler_live->backend,&token))return 0;
    int code=jc_body_appearance_prepare(own_read,NULL,&traveler_body_ops,definition,materials,&traveler_body_lease);
    if(!code)code=jc_actor_material_independent(own_read,NULL,id,&traveler_actor_materials);
    if(!jc_cloth_live_graphics_leave(&traveler_live->backend,&token))code=JC_FRAME_OWNERSHIP;
    char line[192];
    snprintf(line,sizeof(line),"[JC] body appearance code=%d stage=%u held=%u applied=%u",
        code,traveler_body_lease.stage,traveler_body_lease.held,traveler_body_lease.applied);message(line);
    return !code;
}
static int traveler_body_release(uint64_t actor) {
    if(!traveler_body_lease.material)return 1;
    uint64_t model=1,material=1;
    if(traveler_body_thread!=GetCurrentThreadId() ||
        !own_read(NULL,actor+0x168,&model,8) || model ||
        !own_read(NULL,actor+0x210,&material,8) || material)return 0;
    int code=jc_body_appearance_release(NULL,&traveler_body_ops,actor+0x1e0,1,&traveler_body_lease);
    if(!code)message("[JC] body texture references released after native material destruction");
    return !code;
}

int main(void){
    body_fixture();traveler_object=materials[2]-0x1e0;
    body_id=(struct jc_actor_identity){(uintptr_t)body_lookup-JC_RVA_MaterialScalarLookup,materials[0]-0x290,traveler_object};
    assert(traveler_body_prepare(&body_id) && body_holds==4 && traveler_body_lease.applied && !body_locked);
    unsigned calls=setter_calls;
    assert(traveler_body_prepare(&body_id) && setter_calls==calls && body_holds==4);
    assert(!traveler_body_release(traveler_object) && !body_drops);
    body_destroyed=1;put(0x20030,0);body_thread=8;
    assert(!traveler_body_release(traveler_object) && !body_drops);
    body_thread=7;assert(traveler_body_release(traveler_object) && body_drops==4);
    assert(traveler_body_release(traveler_object) && body_drops==4);
    body_fixture();body_independent=0;
    assert(!traveler_body_prepare(&body_id) && !body_holds && !setter_calls);
    puts("Production body appearance: actual vector/texture setters, graphics scope, one-time preparation, owner thread and native material destruction gate passed; native refcounts simulated.");
}
