#ifndef JC_NATIVE_CONNECTION_SCAN_H
#define JC_NATIVE_CONNECTION_SCAN_H
#include "native_pose_storage.h"
#include "native_trial_shader.h"
#include "native_render_queue.h"
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#define JC_SCAN_CLIPS 16u
struct jc_scan_clip {
    unsigned row,descriptor,additive;
    float frame,duration,speed,weight,clock,range[2];
    int state_code;
    char name[128];
};
struct jc_scan_dude {
    int code,animation_code,spine_code;
    unsigned symbol,total,active,stored,dust,chunk;
    float position[3],dissolve_target,dissolve_render,spine[16];
    struct jc_scan_clip clips[JC_SCAN_CLIPS];
};
struct jc_connection_snapshot {
    uint64_t barn; /* Only for local comparison; never written to the log. */
    int code,level,remote_gate,shout_code,cloth_code;
    unsigned connected,gates[3],blocked,chapter[2],shout_counter,shout_active,shout_local,shout_remote,cloth_role;
    int cloth_dissolve_milli;
    unsigned cloth_stage;
    struct jc_scan_dude dude[2];
};
static inline int jc_scan_name(jc_layout_reader read,void *ctx,uint64_t entry,const unsigned char row[80],char out[128]) {
    uint64_t length,cap,pointer=entry+32;
    memcpy(&length,row+48,8); memcpy(&cap,row+56,8);
    if (length>=128 || cap<length || cap>4096) return 4;
    if (cap>=16) memcpy(&pointer,row+32,8);
    if (!jc_layout_field(read,ctx,pointer,0,out,(size_t)length+1)) return 1;
    if (out[length] || memchr(out,0,(size_t)length)) return 4;
    /* Names are asset identifiers, not arbitrary bytes or log delimiters. */
    for (unsigned i=0;i<length;++i)
        if ((unsigned char)out[i]<32 || (unsigned char)out[i]>126 || out[i]=='"' || out[i]=='\\') out[i]='?';
    return 0;
}
static inline int jc_scan_animation(jc_layout_reader read,void *ctx,uint64_t image,uint64_t mesh,struct jc_scan_dude *out) {
    uint64_t fields[2],after[2],states=0,state_count=0;
    unsigned char rows[512][80];
    if (!jc_layout_field(read,ctx,mesh,0x578,fields,sizeof(fields))) return 1;
    if (!fields[1] || fields[1]>512) return 4;
    out->total=(unsigned)fields[1];
    if (!jc_layout_field(read,ctx,fields[0],0,rows,out->total*80)) return 1;
    struct jc_clone_snapshot clone;
    int clone_code=jc_clone_snapshot_read(read,ctx,image,mesh,&clone);
    if (!clone_code && clone.payload) {
        uint64_t pair[2];
        if (jc_layout_field(read,ctx,clone.payload,0x40,pair,sizeof(pair)) && pair[1]<=512) {
            states=pair[0]; state_count=pair[1];
        }
    }
    for (unsigned i=0;i<out->total;++i) {
        float weight; memcpy(&weight,rows[i]+16,4);
        if (!isfinite(weight)) return 4;
        if (!weight) continue;
        ++out->active;
        if (out->stored==JC_SCAN_CLIPS) continue;
        struct jc_scan_clip *c=out->clips+out->stored++;
        c->row=i; c->additive=rows[i][2]; c->weight=weight;
        memcpy(&c->descriptor,rows[i]+20,4);
        memcpy(&c->frame,rows[i]+4,4); memcpy(&c->duration,rows[i]+8,4); memcpy(&c->speed,rows[i]+12,4);
        if (!isfinite(c->frame) || !isfinite(c->duration) || !isfinite(c->speed)) return 4;
        int code=jc_scan_name(read,ctx,fields[0]+i*80,rows[i],c->name); if (code) return code;
        c->state_code=1;
        if (states && c->descriptor<state_count) {
            uint64_t state; float values[6];
            if (jc_layout_field(read,ctx,states,c->descriptor*8,&state,8) &&
                jc_layout_field(read,ctx,state,0x10,values,sizeof(values))) {
                c->clock=values[0]; c->range[0]=values[4]; c->range[1]=values[5];
                c->state_code=isfinite(c->clock) && isfinite(c->range[0]) && isfinite(c->range[1]) ? 0 : 4;
            }
        }
    }
    if (!jc_layout_field(read,ctx,mesh,0x578,after,sizeof(after))) return 1;
    return memcmp(fields,after,sizeof(fields)) ? 3 : 0;
}
static inline int jc_scan_dude_read(jc_layout_reader read,void *ctx,uint64_t image,uint64_t dude,int deep,struct jc_scan_dude *out) {
    uint64_t type; unsigned char flags[2];
    out->spine_code=-1; out->animation_code=-1;
    if (!jc_layout_field(read,ctx,dude,0,&type,8)) return 1;
    if (type!=image+0x5d0380) return 2;
    if (!jc_layout_field(read,ctx,dude,0x10,out->position,sizeof(out->position)) ||
        !jc_layout_field(read,ctx,dude,0x74,&out->symbol,4) ||
        !jc_layout_field(read,ctx,dude,0x1e00,&out->dissolve_target,4) ||
        !jc_layout_field(read,ctx,dude,0x2000,&out->dissolve_render,4) ||
        !jc_layout_field(read,ctx,dude,0xa264,flags,1) ||
        !jc_layout_field(read,ctx,dude,0xa2f4,flags+1,1)) return 1;
    out->dust=flags[0]; out->chunk=flags[1];
    for (unsigned i=0;i<3;++i) if (!isfinite(out->position[i])) return 4;
    if (!isfinite(out->dissolve_target) || !isfinite(out->dissolve_render)) return 4;
    if (deep) {
        out->animation_code=jc_scan_animation(read,ctx,image,dude+0x160,out);
        struct jc_pose_storage storage;
        out->spine_code=jc_pose_storage_read(read,ctx,image,dude+0x160,&storage);
        if (!out->spine_code) {
            float world[16],root[16],inverse[16],check[16];
            if (!jc_layout_field(read,ctx,storage.root,0x20,root,sizeof(root))) out->spine_code=1;
            if (!out->spine_code) out->spine_code=jc_trial_spine_lookup(read,ctx,storage.model,storage.root,world);
            if (!out->spine_code && (!jc_layout_field(read,ctx,storage.root,0x20,check,sizeof(check)) || memcmp(root,check,sizeof(root)))) out->spine_code=3;
            if (!out->spine_code) out->spine_code=jc_trial_inverse(root,inverse);
            if (!out->spine_code) {
                jc_trial_multiply(inverse,world,out->spine);
                if (!jc_trial_affine(out->spine)) out->spine_code=4;
            }
        }
    }
    return 0;
}
static inline int jc_connection_read(jc_layout_reader read,void *ctx,uint64_t image,uint64_t game,
                                    int deep,unsigned cloth_role,struct jc_connection_snapshot *out) {
    memset(out,0,sizeof(*out)); out->level=-1; out->cloth_code=out->shout_code=-1;
    out->dude[0].code=out->dude[1].code=-1;
    uint64_t barn,type,match,shout,cloth,again; unsigned char gates[3],check[3],connected,blocked;
    if (!jc_layout_field(read,ctx,game,0x1e0,&barn,8) || !jc_layout_field(read,ctx,barn,0,&type,8)) return 1;
    if (type!=image+0x5da340) return 2;
    out->barn=barn;
    if (!jc_layout_field(read,ctx,game,0x180,&match,8) ||
        !jc_layout_field(read,ctx,match,0x11db4,&connected,1) ||
        !jc_layout_field(read,ctx,barn,0xa2,gates,3) ||
        !jc_layout_field(read,ctx,barn,0x1670c,&blocked,1) ||
        !jc_layout_field(read,ctx,barn,0x14,out->chapter,4) ||
        !jc_layout_field(read,ctx,barn,0x64,out->chapter+1,4)) return 1;
    out->connected=connected; out->blocked=blocked;
    for (unsigned i=0;i<3;++i) out->gates[i]=gates[i];
    out->remote_gate=(gates[0] || gates[2]) && !blocked && out->chapter[0]==out->chapter[1];
    jc_render_level(read,ctx,game,&out->level);
    for (unsigned i=0;i<2;++i)
        out->dude[i].code=jc_scan_dude_read(read,ctx,image,barn+(i ? 0x146f0 : 0xb0),deep,out->dude+i);
    if (jc_layout_field(read,ctx,game,0x1b8,&shout,8)) {
        unsigned char slots[0x718];
        if (jc_layout_field(read,ctx,shout,0,slots,sizeof(slots))) {
            memcpy(&out->shout_counter,slots+0x714,4); out->shout_code=0;
            for (unsigned i=0;i<16;++i) if (slots[i*0x70+0x68]) {
                ++out->shout_active;
                out->shout_local+=slots[i*0x70+0x4c]!=0;
                out->shout_remote+=slots[i*0x70+0x4d]!=0;
            }
        } else out->shout_code=1;
    }
    if (deep && cloth_role>=2 && cloth_role<=5) {
        out->cloth_role=cloth_role;
        struct jc_scalar_observation scalar;
        out->cloth_code=1;
        out->cloth_stage=1;
        uint64_t material;
        if (jc_layout_field(read,ctx,game,0x108,&cloth,8) && jc_layout_address(cloth,0x666ee8+cloth_role*0x630,&material)) {
            out->cloth_code=jc_material_scalar_observe(read,ctx,material,"dudeDissolve",&scalar);
            out->cloth_stage=scalar.stage;
            if (!out->cloth_code) {
                if (scalar.value>=-1000 && scalar.value<=1000) out->cloth_dissolve_milli=(int)(scalar.value*1000);
                else out->cloth_code=4;
            }
        }
    }
    if (!jc_layout_field(read,ctx,game,0x1e0,&again,8) || !jc_layout_field(read,ctx,barn,0xa2,check,3)) return 1;
    if (again!=barn || memcmp(gates,check,3)) return 3;
    return 0;
}
static inline int jc_scan_append(char *text,size_t cap,size_t *used,const char *format,...) {
    if (*used>=cap) return 0;
    va_list args; va_start(args,format); int n=vsnprintf(text+*used,cap-*used,format,args); va_end(args);
    if (n<0 || (size_t)n>=cap-*used) return 0;
    *used+=(size_t)n; return 1;
}
static inline size_t jc_connection_format(char *text,size_t cap,const struct jc_connection_snapshot *s,
                                         uint64_t ms,unsigned frame,unsigned after,unsigned cost,unsigned reads,int deep) {
    size_t used=0;
#define JC_SCAN_ADD(...) do { if (!jc_scan_append(text,cap,&used,__VA_ARGS__)) return 0; } while (0)
    JC_SCAN_ADD("{\"ms\":%llu,\"frame\":%u,\"frame_after\":%u,\"read_ms\":%u,\"reads\":%u,\"code\":%d",(unsigned long long)ms,frame,after,cost,reads,s->code);
    if (!s->code) {
        JC_SCAN_ADD(",\"level\":%d,\"connected\":%u,\"gate\":%d,\"gates\":[%u,%u,%u],\"blocked\":%u,\"chapters\":[%u,%u],\"dudes\":[",
            s->level,s->connected,s->remote_gate,s->gates[0],s->gates[1],s->gates[2],s->blocked,s->chapter[0],s->chapter[1]);
        for (unsigned i=0;i<2;++i) {
            const struct jc_scan_dude *d=s->dude+i;
            JC_SCAN_ADD("%s{\"code\":%d",i ? "," : "",d->code);
            if (!d->code) {
                JC_SCAN_ADD(",\"symbol_zero_based\":%u,\"position\":[%.6g,%.6g,%.6g],\"dissolve\":[%.6g,%.6g],\"emitters\":[%u,%u],\"animation_code\":%d,\"active\":%u,\"total\":%u,\"spine_code\":%d",
                    d->symbol,(double)d->position[0],(double)d->position[1],(double)d->position[2],(double)d->dissolve_target,(double)d->dissolve_render,d->dust,d->chunk,d->animation_code,d->active,d->total,d->spine_code);
                if (deep && !d->animation_code) {
                    JC_SCAN_ADD(",\"clips\":[");
                    for (unsigned j=0;j<d->stored;++j) {
                        const struct jc_scan_clip *c=d->clips+j;
                        JC_SCAN_ADD("%s{\"name\":\"%s\",\"row\":%u,\"descriptor\":%u,\"additive\":%u,\"frame\":%.7g,\"duration\":%.7g,\"speed\":%.7g,\"weight\":%.7g,\"state_code\":%d",
                            j ? "," : "",c->name,c->row,c->descriptor,c->additive,(double)c->frame,(double)c->duration,(double)c->speed,(double)c->weight,c->state_code);
                        if (!c->state_code) JC_SCAN_ADD(",\"clock\":%.7g,\"range\":[%.7g,%.7g]",(double)c->clock,(double)c->range[0],(double)c->range[1]);
                        JC_SCAN_ADD("}");
                    }
                    JC_SCAN_ADD("]");
                }
                if (deep && !d->spine_code) {
                    JC_SCAN_ADD(",\"relative_spine\":[");
                    for (unsigned j=0;j<16;++j) JC_SCAN_ADD("%s%.7g",j ? "," : "",(double)d->spine[j]);
                    JC_SCAN_ADD("]");
                }
            }
            JC_SCAN_ADD("}");
        }
        JC_SCAN_ADD("],\"shout\":[%d,%u,%u,%u,%u],\"cloth\":[%d,%u,%d]",s->shout_code,s->shout_counter,s->shout_active,s->shout_local,s->shout_remote,s->cloth_code,s->cloth_role,s->cloth_dissolve_milli);
        JC_SCAN_ADD(",\"cloth_stage\":%u",s->cloth_stage);
    }
    JC_SCAN_ADD("}\n");
#undef JC_SCAN_ADD
    return used;
}
#endif
