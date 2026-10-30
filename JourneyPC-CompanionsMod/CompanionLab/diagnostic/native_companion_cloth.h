#ifndef JC_NATIVE_COMPANION_CLOTH_H
#define JC_NATIVE_COMPANION_CLOTH_H
#include "native_layout.h"
#include "native_material_ready.h"
#include <math.h>

enum { JC_CLOTH_OK=0, JC_CLOTH_READ=1, JC_CLOTH_IDENTITY=2,
    JC_CLOTH_CHANGED=3, JC_CLOTH_BUSY=4, JC_CLOTH_PEER=5,
    JC_CLOTH_CAPACITY=6, JC_CLOTH_OWNERSHIP=7, JC_CLOTH_CALL=8,
    JC_CLOTH_PHASE=9, JC_CLOTH_PROPERTIES=10 };
enum { JC_CLOTH_INIT=1, JC_CLOTH_ATTACH=2, JC_CLOTH_COLLIDE=3,
    JC_CLOTH_APPEARANCE=4, JC_CLOTH_RELEASE=5, JC_CLOTH_ATLAS=10 };
enum { JC_CLOTH_EMPTY=0, JC_CLOTH_LIVE=1, JC_CLOTH_RETIRED=2,
    JC_CLOTH_QUARANTINED=3 };
typedef int (*jc_cloth_writer)(void *,uint64_t,const void *,size_t);
typedef int (*jc_cloth_call)(void *,unsigned,uint64_t,uint64_t,uint64_t);
struct jc_cloth_lease {
    uint64_t image,game,barn,players,dude,collection,generation,handle[2],descriptor[2];
    unsigned slot[2],state,code,stage,native_calls,updates,free_slots,capture_detail,failure_stage;
    unsigned char saved_remote;
    unsigned atlas_ready,atlas_outfit;
    float scarf_charge,scarf_length;
};
struct jc_cloth_view { uint64_t barn,players,timer,job; unsigned free_slots,foreign_roles,peer; };
struct jc_cloth_entry { unsigned rva; unsigned char bytes[16]; };
static const struct jc_cloth_entry jc_cloth_entries[]={
    {0x246230,{0x40,0x53,0x41,0x56,0x41,0x57,0x48,0x83,0xec,0x40,0x48,0x8b,0xd9,0x45,0x33,0xff}},
    {0x246470,{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x48}},
    {0x246d40,{0x48,0x8b,0xc4,0x48,0x89,0x58,0x18,0x57,0x41,0x56,0x41,0x57,0x48,0x81,0xec,0x30}},
    {0x0a9470,{0x48,0x8b,0xc4,0x48,0x89,0x58,0x08,0x48,0x89,0x70,0x10,0x48,0x89,0x78,0x18,0x55}},
    {0x087880,{0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0x51,0x10,0x48,0x8b,0xd9,0x48,0x85,0xd2}},
    {0x2ce120,{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57}},
    {0x08ab10,{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x20,0x48}},
    {0x08ee60,{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x20,0x49}},
    {0x273640,{0x48,0x89,0x5c,0x24,0x10,0x56,0x48,0x83,0xec,0x20,0x48,0x8b,0xf1,0x49,0x8b,0xd8}}
};

static inline int jc_cloth_fingerprints(jc_layout_reader read,void *ctx,uint64_t image) {
    unsigned char bytes[16];
    for (unsigned i=0;i<sizeof(jc_cloth_entries)/sizeof(*jc_cloth_entries);++i)
        if (!jc_layout_field(read,ctx,image,jc_cloth_entries[i].rva,bytes,sizeof(bytes)) ||
            memcmp(bytes,jc_cloth_entries[i].bytes,sizeof(bytes))) return JC_CLOTH_IDENTITY;
    return 0;
}

static inline int jc_cloth_idle(jc_layout_reader read,void *ctx,uint64_t barn,uint64_t *job) {
    unsigned char pending,busy;
    if (!jc_layout_field(read,ctx,barn,0x6bcf84,&pending,1) ||
        !jc_layout_field(read,ctx,barn,0x6bcef0,job,8) || !*job ||
        !jc_layout_field(read,ctx,*job,0x58,&busy,1)) return JC_CLOTH_READ;
    return pending || busy ? JC_CLOTH_BUSY : 0;
}

static inline int jc_cloth_view_read(jc_layout_reader read,void *ctx,uint64_t image,
        uint64_t game,uint64_t dude,const struct jc_cloth_lease *lease,struct jc_cloth_view *out) {
    uint64_t type,again; unsigned char gates[3];
    memset(out,0,sizeof(*out));
    if (!jc_layout_field(read,ctx,game,0x108,&out->barn,8) ||
        !jc_layout_field(read,ctx,game,0x1e0,&out->players,8) ||
        !jc_layout_field(read,ctx,game,0x68,&out->timer,8) ||
        !jc_layout_field(read,ctx,out->players,0,&type,8)) return JC_CLOTH_READ;
    if (type!=image+0x5da340 || dude==out->players+0xb0 || dude==out->players+0x146f0)
        return JC_CLOTH_IDENTITY;
    if (!jc_layout_field(read,ctx,dude,0,&type,8) ||
        !jc_layout_field(read,ctx,out->players,0xa2,gates,3)) return JC_CLOTH_READ;
    if (type!=image+0x5d0380) return JC_CLOTH_IDENTITY;
    out->peer=gates[0]!=0 || gates[2]!=0;
    int code=jc_cloth_idle(read,ctx,out->barn,&out->job); if (code) return code;
    unsigned char states[8*0xe0];
    for (unsigned first=0;first<768;first+=8) {
        if (!jc_layout_field(read,ctx,out->barn,0x270080+(uint64_t)first*0xe0,states,sizeof(states))) return JC_CLOTH_READ;
        for (unsigned k=0;k<8;++k) {
            uint32_t state,role; unsigned index=first+k;
            memcpy(&state,states+k*0xe0+0x98,4);
            if (!state) { ++out->free_slots; continue; }
            if (state!=1 && state!=2 && state!=3) return JC_CLOTH_OWNERSHIP;
            if (!jc_layout_field(read,ctx,out->barn,0x80+(uint64_t)index*0xd00+0xc94,&role,4)) return JC_CLOTH_READ;
            if (role>6) return JC_CLOTH_OWNERSHIP;
            int owned=lease && lease->state==JC_CLOTH_LIVE && lease->barn==out->barn &&
                (index==lease->slot[0] || index==lease->slot[1]);
            if ((role==4 || role==5) && !owned) ++out->foreign_roles;
        }
    }
    if (!jc_layout_field(read,ctx,game,0x108,&again,8)) return JC_CLOTH_READ;
    if (again!=out->barn) return JC_CLOTH_CHANGED;
    if (!jc_layout_field(read,ctx,game,0x1e0,&again,8)) return JC_CLOTH_READ;
    if (again!=out->players) return JC_CLOTH_CHANGED;
    return 0;
}

/* Native handle -> intrusive owner +10 -> descriptor +18; handles retain their barn at +18. */
static inline int jc_cloth_capture(jc_layout_reader read,void *ctx,struct jc_cloth_lease *lease,int roles) {
    uint64_t collection,handle[2],record[2],descriptor[2]; unsigned slot[2]; uint32_t count;
    lease->capture_detail=1;
    if (!jc_layout_field(read,ctx,lease->dude,0x9b40,&collection,8) || !collection ||
        !jc_layout_field(read,ctx,collection,0x91b0,&count,4)) return JC_CLOTH_READ;
    if (count!=2 || (lease->collection && lease->collection!=collection)) return JC_CLOTH_OWNERSHIP;
    for (unsigned i=0;i<2;++i) {
        uint64_t owner,link,barn; uint32_t state,role;
        lease->capture_detail=2+i*8;
        if (!jc_layout_field(read,ctx,lease->dude,0x9b58+i*16,&handle[i],8) ||
            !jc_layout_field(read,ctx,lease->dude,0x9b60+i*16,&record[i],8)) return JC_CLOTH_READ;
        if (handle[i]<collection || (handle[i]-collection)%0x20 || (handle[i]-collection)/0x20>=2 ||
            record[i]!=collection+0xc0+((handle[i]-collection)/0x20)*0x1828) return JC_CLOTH_OWNERSHIP;
        lease->capture_detail=3+i*8;
        if (!jc_layout_field(read,ctx,handle[i],0,&owner,8) || !owner ||
            !jc_layout_field(read,ctx,handle[i],0x18,&barn,8) ||
            !jc_layout_field(read,ctx,owner,0x10,&link,8) || !link ||
            !jc_layout_field(read,ctx,link,0x18,&descriptor[i],8)) return JC_CLOTH_READ;
        lease->capture_detail=4+i*8;
        if (barn!=lease->barn || descriptor[i]<barn+0x80 ||
            (descriptor[i]-barn-0x80)%0xd00 || (descriptor[i]-barn-0x80)/0xd00>=768) return JC_CLOTH_OWNERSHIP;
        slot[i]=(unsigned)((descriptor[i]-barn-0x80)/0xd00);
        lease->capture_detail=5+i*8;
        if (!jc_layout_field(read,ctx,barn,0x270118+(uint64_t)slot[i]*0xe0,&state,4) ||
            !jc_layout_field(read,ctx,descriptor[i],0xc94,&role,4)) return JC_CLOTH_READ;
        /* 87110 creates state2; first attach activates it through 86fd0. */
        if ((state!=1 && (roles || state!=2)) || (roles && role!=(i==0?5u:4u)) ||
            (lease->collection && (lease->handle[i]!=handle[i] || lease->descriptor[i]!=descriptor[i]))) return JC_CLOTH_OWNERSHIP;
    }
    lease->capture_detail=18;
    if (handle[0]==handle[1] || slot[0]==slot[1]) return JC_CLOTH_OWNERSHIP;
    lease->collection=collection; memcpy(lease->handle,handle,sizeof(handle));
    memcpy(lease->descriptor,descriptor,sizeof(descriptor)); memcpy(lease->slot,slot,sizeof(slot));
    lease->capture_detail=0;
    return 0;
}

static inline int jc_cloth_invoke(jc_cloth_call call,void *ctx,struct jc_cloth_lease *lease,unsigned op,uint64_t timer) {
    lease->stage=op; ++lease->native_calls;
    return call(ctx,op,op==JC_CLOTH_RELEASE?lease->dude+0x9b38:lease->dude,lease->barn,timer)?0:JC_CLOTH_CALL;
}

static inline int jc_cloth_properties(jc_layout_reader read,jc_cloth_writer write,void *ctx,
        uint64_t source,uint64_t dude,uint64_t timer) {
    static const unsigned offsets[]={0x60,0x68,0x143c,0x144c,0x1454,0x145c,0x2000,0x1c00};
    static const unsigned sizes[]={4,8,4,4,4,4,16,16};
    unsigned char values[8][16]; float transform[16],dt; uint64_t symbols[2];
    uint32_t appearance[4],again[4];
    if (!jc_layout_field(read,ctx,source,0x68,appearance,sizeof(appearance)) ||
        !jc_layout_field(read,ctx,source,0x68,again,sizeof(again))) return JC_CLOTH_READ;
    if (memcmp(appearance,again,sizeof(appearance))) return JC_CLOTH_CHANGED;
    if (appearance[0]>appearance[1] || appearance[1]>4096 || appearance[2]>6 || appearance[3]>20) return JC_CLOTH_PROPERTIES;
    if (!jc_layout_field(read,ctx,dude,0x860,&symbols[0],8) ||
        !jc_layout_field(read,ctx,dude,0xf00,&symbols[1],8)) return JC_CLOTH_READ;
    /* a9470 also writes optional Symbol materials. This torso-only owner has none. */
    if (symbols[0] || symbols[1]) return JC_CLOTH_OWNERSHIP;
    for (unsigned i=0;i<8;++i) {
        if (!jc_layout_field(read,ctx,source,offsets[i],values[i],sizes[i])) return JC_CLOTH_READ;
        if (i!=1) for (unsigned j=0;j<sizes[i]/4;++j) {
            float value; memcpy(&value,values[i]+j*4,4);
            if (!isfinite(value) || fabsf(value)>100000.f) return JC_CLOTH_PROPERTIES;
        }
    }
    /* a6c23 clamps current +68 to capacity +6c; 247d71 sizes scarf from unsigned +6c. */
    uint32_t lengths[2]; memcpy(lengths,values[1],sizeof(lengths));
    if (memcmp(lengths,appearance,sizeof(lengths))) return JC_CLOTH_CHANGED;
    if (lengths[0]>lengths[1] || lengths[1]>4096) return JC_CLOTH_PROPERTIES;
    if (!jc_layout_field(read,ctx,dude,0x690,transform,sizeof(transform)) ||
        !jc_layout_field(read,ctx,timer,0x10,&dt,4)) return JC_CLOTH_READ;
    if (!isfinite(dt) || dt<0 || dt>1) return JC_CLOTH_PROPERTIES;
    for (unsigned i=0;i<16;++i) if (!isfinite(transform[i])) return JC_CLOTH_PROPERTIES;
    for (unsigned i=0;i<8;++i) if (!write(ctx,dude+offsets[i],values[i],sizes[i])) return JC_CLOTH_CALL;
    if (!write(ctx,dude+0x70,appearance+2,8)) return JC_CLOTH_CALL;
    if (!write(ctx,dude+0x10,transform+12,16)) return JC_CLOTH_CALL;
    return 0;
}

/* Atlas names are native resource identifiers, selected by this participant. */
static inline int jc_cloth_atlas_names(unsigned outfit,const char *names[2][3]) {
    static const char *const base[]={"ClothPlayerAtlasRed0","ClothPlayerAtlasRed1","ClothPlayerAtlasRed2","ClothPlayerAtlasRed3","ClothPlayerAtlasWhite1","ClothPlayerAtlasWhite2","ClothPlayerAtlasWhite3"};
    static const char *const glow[]={"ClothPlayerAtlasRedGlow0","ClothPlayerAtlasRedGlow1","ClothPlayerAtlasRedGlow2","ClothPlayerAtlasRedGlow3","ClothPlayerAtlasWhiteGlow1","ClothPlayerAtlasWhiteGlow2","ClothPlayerAtlasWhiteGlow3"};
    static const char *const mask[]={"ClothPlayerAtlasMask1","ClothPlayerAtlasMask3","ClothPlayerAtlasMask5","ClothPlayerAtlasMask6","ClothPlayerAtlasMask3","ClothPlayerAtlasMask5","ClothPlayerAtlasMask6"};
    if (outfit>6) return 0;
    names[0][0]=base[outfit]; names[0][1]=glow[outfit]; names[0][2]=mask[outfit];
    names[1][0]="ClothPlayerScarf"; names[1][1]="ClothPlayerScarfGlow"; names[1][2]="ClothPlayerScarfMask";
    return 1;
}
/* 2cca00 binds holder+8 to the typed registry entry+28. Payload names are
   not the registry key and must not be used as proof of resource identity. */
static inline int jc_cloth_atlas_loaded(jc_layout_reader read,void *ctx,uint64_t holder,
        uint64_t entry,int scarf,uint64_t *texture_out) {
    uint64_t pair[2],again[2],payload,check,texture,texture_check; uint32_t flags,flags_check;
    if (!entry || !jc_layout_field(read,ctx,holder,0,pair,sizeof(pair))) return 1;
    if (!pair[0] || pair[1]!=entry+0x28) return 2;
    if (!jc_layout_field(read,ctx,entry,0x38,&payload,8) || !payload) return 3;
    if (!jc_layout_field(read,ctx,payload,0xc8,&texture,8) || !texture ||
        !jc_layout_field(read,ctx,texture,0x24,&flags,4)) return 4;
    /* 8ab10 changes sampler addressing only for scarf roles3/5. */
    if (scarf && (flags&0x80)) return 5;
    if (!jc_layout_field(read,ctx,holder,0,again,sizeof(again)) ||
        !jc_layout_field(read,ctx,entry,0x38,&check,8) ||
        !jc_layout_field(read,ctx,payload,0xc8,&texture_check,8) ||
        !jc_layout_field(read,ctx,texture,0x24,&flags_check,4) ||
        check!=payload || texture_check!=texture || flags_check!=flags || memcmp(pair,again,sizeof(pair))) return 6;
    if (texture_out) *texture_out=texture;
    return 0;
}
/* The steady appearance branch of 24c010, limited to our two owned descriptors.
   No global appearance state, effects, resizing or native-player flags are used. */
static inline int jc_cloth_scarf_resize(jc_layout_reader read,jc_cloth_writer write,
        void *ctx,uint64_t image,uint64_t dude) {
    uint32_t capacity,maximum,rows;unsigned char pending;
    if (!jc_layout_field(read,ctx,dude,0x6c,&capacity,4) ||
        !jc_layout_field(read,ctx,image,0x731fb0,&maximum,4) ||
        !jc_layout_field(read,ctx,dude,0x1460,&rows,4) ||
        !jc_layout_field(read,ctx,dude,0x9ba0,&pending,1)) return JC_CLOTH_READ;
    if (!maximum || maximum>4096 || capacity>4096) return JC_CLOTH_PROPERTIES;
    unsigned wanted=(unsigned)((float)capacity/(float)maximum*73.f+1.5f);
    if(wanted<2)wanted=2;
    if(wanted>73)wanted=73;
    if(rows==wanted || pending)return JC_CLOTH_OK;
    pending=1;
    return write(ctx,dude+0x9ba0,&pending,1)?JC_CLOTH_OK:JC_CLOTH_CALL;
}

static inline int jc_cloth_scarf_step(jc_layout_reader read,jc_cloth_writer write,void *ctx,
        struct jc_cloth_lease *lease,uint64_t timer) {
    uint32_t scarf[3],maximum; float dt;
    if (!jc_layout_field(read,ctx,lease->dude,0x68,scarf,sizeof(scarf)) ||
        !jc_layout_field(read,ctx,lease->image,0x731fb0,&maximum,4) ||
        !jc_layout_field(read,ctx,timer,0x10,&dt,4)) return JC_CLOTH_READ;
    if (scarf[0]>scarf[1] || scarf[1]>4096 || scarf[2]>6 || !maximum || maximum>4096 ||
        !isfinite(dt) || dt<0 || dt>1) return JC_CLOTH_PROPERTIES;
    float charge=fminf(1.f,(float)scarf[0]/(float)(scarf[1]?scarf[1]:1));
    float length=fminf(1.f,(float)scarf[1]/(float)maximum);
    charge=lease->scarf_charge+fmaxf(-.075f,fminf(.075f,(charge-lease->scarf_charge)*dt*2.f));
    length=lease->scarf_length+fmaxf(-.075f,fminf(.075f,(length-lease->scarf_length)*dt*.5f));
    float cutoff=1.f-length,glow=powf(charge,.75f)*.495f+.05f;
    unsigned char visible=1,off=0; uint32_t zero=0,scarf_mode[3]={0},cape_mode[4]={1,0,0,0};
    if (scarf[2]>3) scarf_mode[0]=scarf_mode[1]=3;
    int code=jc_cloth_capture(read,ctx,lease,1); if (code) return code;
    for (unsigned i=0;i<2;++i) {
        uint64_t target=lease->descriptor[i];
        if (!write(ctx,target+0xca4,&zero,4) || !write(ctx,target+0xc68,&off,1)) return JC_CLOTH_CALL;
        if (i==0) {
            if (!write(ctx,target+0xc62,&visible,1) || !write(ctx,target+0xc90,&cutoff,4) ||
                !write(ctx,target+0xc64,&glow,4) || !write(ctx,target+0xcb8,scarf_mode,sizeof(scarf_mode))) return JC_CLOTH_CALL;
        } else if (!write(ctx,target+0xcb4,cape_mode,sizeof(cape_mode))) return JC_CLOTH_CALL;
    }
    lease->scarf_charge=charge; lease->scarf_length=length;
    return 0;
}

static inline int jc_cloth_materials(jc_layout_reader read,void *ctx,uint64_t barn,uint64_t players,uint64_t dude) {
    const uint64_t materials[]={dude+0x1e0,barn+0x666ee8+4*0x630,barn+0x666ee8+5*0x630,
        players+0x290,players+0x148d0,barn+0x666ee8+2*0x630,barn+0x666ee8+3*0x630};
    uint64_t payloads[7];
    for (unsigned i=0;i<7;++i) {
        if (jc_material_ready(read,ctx,materials[i],&payloads[i])) return JC_CLOTH_PROPERTIES;
        for (unsigned j=0;j<i;++j) if (j<3 && payloads[j]==payloads[i]) return JC_CLOTH_OWNERSHIP;
    }
    for (unsigned i=0;i<2;++i) {
        uint64_t pair[2],check[2],resource; uint32_t references;
        if (!jc_layout_field(read,ctx,barn,0x646d30+i*16,pair,sizeof(pair))) return JC_CLOTH_READ;
        if (i==1 && !pair[0] && !pair[1]) continue;
        if (!pair[0] || !pair[1] ||
            !jc_layout_field(read,ctx,pair[0],0x70,&resource,8) || !resource ||
            !jc_layout_field(read,ctx,pair[1],0,&references,4) || !references || references>1000000 ||
            !jc_layout_field(read,ctx,barn,0x646d30+i*16,check,sizeof(check))) return JC_CLOTH_PROPERTIES;
        if (memcmp(pair,check,sizeof(pair))) return JC_CLOTH_CHANGED;
    }
    return 0;
}

static inline int jc_companion_cloth_retire(jc_layout_reader read,jc_cloth_writer write,
        jc_cloth_call call,void *ctx,uint64_t game,uint64_t generation,int pre_cloth,
        struct jc_cloth_lease *lease) {
    if (lease->state==JC_CLOTH_EMPTY || lease->state==JC_CLOTH_RETIRED) return 0;
    if (!pre_cloth) return lease->code=JC_CLOTH_PHASE;
    if (lease->state!=JC_CLOTH_LIVE || game!=lease->game || generation!=lease->generation)
        return lease->code=JC_CLOTH_OWNERSHIP;
    uint64_t barn,job;
    if (!jc_layout_field(read,ctx,game,0x108,&barn,8)) return lease->code=JC_CLOTH_READ;
    if (barn!=lease->barn) return lease->code=JC_CLOTH_CHANGED;
    int code=jc_cloth_idle(read,ctx,barn,&job); if (code) return lease->code=code;
    code=jc_cloth_fingerprints(read,ctx,lease->image); if (code) return lease->code=code;
    code=jc_cloth_capture(read,ctx,lease,0); if (code) return lease->code=code;
    code=jc_cloth_invoke(call,ctx,lease,JC_CLOTH_RELEASE,0);
    if (!code) {
        uint64_t fields[2];
        if (!jc_layout_field(read,ctx,lease->dude,0x9b40,fields,sizeof(fields)) || fields[0] || fields[1]) code=JC_CLOTH_OWNERSHIP;
        for (unsigned i=0;!code && i<2;++i) {
            uint32_t state;
            if (!jc_layout_field(read,ctx,barn,0x270118+(uint64_t)lease->slot[i]*0xe0,&state,4) || state!=3) code=JC_CLOTH_OWNERSHIP;
        }
        uint64_t zero[4]={0};
        if (!code && (!write(ctx,lease->dude+0x9b58,zero,sizeof(zero)) ||
            !write(ctx,lease->dude+0x81,&lease->saved_remote,1))) code=JC_CLOTH_CALL;
    }
    lease->state=code?JC_CLOTH_QUARANTINED:JC_CLOTH_RETIRED;
    return lease->code=code;
}

/* Caller holds its lifetime lock, validates the private evaluated Dude/model and enters
   immediately before original 89a00, after native pass-1 completion. No worker is active. */
static inline int jc_companion_cloth_step(jc_layout_reader read,jc_cloth_writer write,
        jc_cloth_call call,void *ctx,uint64_t image,uint64_t game,uint64_t dude,
        uint64_t definition,uint64_t generation,int pre_cloth,int want,struct jc_cloth_lease *lease) {
    if (!read || !write || !call || !lease || !generation) return JC_CLOTH_IDENTITY;
    if (!want) return jc_companion_cloth_retire(read,write,call,ctx,game,generation,pre_cloth,lease);
    if (!pre_cloth) return lease->code=JC_CLOTH_PHASE;
    if (lease->state==JC_CLOTH_RETIRED || lease->state==JC_CLOTH_QUARANTINED) return lease->code=JC_CLOTH_OWNERSHIP;
    if (lease->state==JC_CLOTH_LIVE && (game!=lease->game || dude!=lease->dude || generation!=lease->generation))
        return lease->code=JC_CLOTH_CHANGED;
    lease->stage=6;
    struct jc_cloth_view view;
    int code=jc_cloth_view_read(read,ctx,image,game,dude,lease,&view); if (code) return lease->code=code;
    if (lease->state==JC_CLOTH_LIVE && (view.barn!=lease->barn || view.players!=lease->players))
        return lease->code=JC_CLOTH_CHANGED;
    if (view.peer || view.foreign_roles) {
        if (lease->state==JC_CLOTH_LIVE) {
            code=jc_companion_cloth_retire(read,write,call,ctx,game,generation,pre_cloth,lease);
            if (code) return code;
        }
        return lease->code=JC_CLOTH_PEER;
    }
    unsigned char dirty,spawned;
    if (!jc_layout_field(read,ctx,dude,0x180,&dirty,1) ||
        !jc_layout_field(read,ctx,dude,0x1479,&spawned,1)) return lease->code=JC_CLOTH_READ;
    if (dirty && spawned) return lease->code=JC_CLOTH_PROPERTIES;
    code=jc_cloth_fingerprints(read,ctx,image); if (code) return lease->code=code;
    lease->stage=7;
    code=jc_cloth_materials(read,ctx,view.barn,view.players,dude); if (code) return lease->code=code;
    lease->free_slots=view.free_slots;
    if (lease->state==JC_CLOTH_EMPTY) {
        lease->stage=8;
        uint64_t empty[7]; uint32_t attachments;
        if (view.free_slots<2) return lease->code=JC_CLOTH_CAPACITY;
        if (!jc_layout_field(read,ctx,dude,0x9b40,empty,sizeof(empty)) ||
            !jc_layout_field(read,ctx,definition,0x10,&attachments,4)) return lease->code=JC_CLOTH_READ;
        /* +9b50 is collection metadata, not a pointer. */
        if (empty[0] || empty[1] || empty[3] || empty[4] || empty[5] || empty[6] || attachments!=2)
            return lease->code=JC_CLOTH_OWNERSHIP;
        if (!jc_layout_field(read,ctx,dude,0x81,&lease->saved_remote,1)) return lease->code=JC_CLOTH_READ;
        lease->stage=9;
        code=jc_cloth_properties(read,write,ctx,view.players+0xb0,dude,view.timer);
        if (code) return lease->code=code;
        lease->image=image; lease->game=game; lease->barn=view.barn; lease->players=view.players; lease->dude=dude; lease->generation=generation;
        unsigned char remote=1;
        if (!write(ctx,dude+0x81,&remote,1)) return lease->code=JC_CLOTH_CALL;
        code=jc_cloth_invoke(call,ctx,lease,JC_CLOTH_INIT,view.timer);
        if (!code) code=jc_cloth_capture(read,ctx,lease,0);
        if (code) { lease->state=JC_CLOTH_QUARANTINED; return lease->code=code; }
        lease->state=JC_CLOTH_LIVE;
    } else {
        unsigned char remote;
        if (!jc_layout_field(read,ctx,dude,0x81,&remote,1)) return lease->code=JC_CLOTH_READ;
        if (remote!=1) return lease->code=JC_CLOTH_OWNERSHIP;
        code=jc_cloth_capture(read,ctx,lease,1); if (code) return lease->code=code;
        lease->stage=9;
        code=jc_cloth_properties(read,write,ctx,view.players+0xb0,dude,view.timer);
        if (code) return lease->code=code;
    }
    code=jc_cloth_invoke(call,ctx,lease,JC_CLOTH_ATTACH,view.timer);
    if (!code) code=jc_cloth_capture(read,ctx,lease,1);
    if (!code) code=jc_cloth_invoke(call,ctx,lease,JC_CLOTH_COLLIDE,view.timer);
    if (!code) {
        uint32_t outfit;
        if (!jc_layout_field(read,ctx,dude,0x70,&outfit,4)) code=JC_CLOTH_READ;
        else if (!lease->atlas_ready || lease->atlas_outfit!=outfit) {
            code=jc_cloth_invoke(call,ctx,lease,JC_CLOTH_ATLAS,view.timer);
            if (!code) { lease->atlas_ready=1; lease->atlas_outfit=outfit; }
        }
    }
    if (!code) { lease->stage=11; code=jc_cloth_scarf_step(read,write,ctx,lease,view.timer); }
    if (!code) code=jc_cloth_invoke(call,ctx,lease,JC_CLOTH_APPEARANCE,view.timer);
    if (code) {
        lease->failure_stage=lease->stage;
        int retired=jc_companion_cloth_retire(read,write,call,ctx,game,generation,pre_cloth,lease);
        return lease->code=retired?retired:code;
    }
    ++lease->updates; lease->stage=0;
    return lease->code=0;
}
#endif
