#include <assert.h>
#include <stddef.h>
#include <string.h>
typedef unsigned UINT;
#define D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE 16
struct jnet_image_saved { UINT viewport_count; void *vs,*ps,*gs,*hs,*ds; void *vc[256],*pc[256],*gc[256],*hc[256],*dc[256]; UINT vn,pn,gn,hn,dn; };
struct context;
static unsigned mode,token;
static void get(struct context *c,void **shader,void **classes,UINT *count) {
 (void)c;
 if(shader)*shader=&token;
 if(mode==0)return;
 if(mode==1){if(classes){for(UINT i=0;i<3;++i)classes[i]=&token;}*count=3;}
 if(mode==2)*count=257;
 if(mode==3)*count=1;
}
struct vtable { void (*VSGetShader)(struct context*,void**,void**,UINT*); void (*PSGetShader)(struct context*,void**,void**,UINT*); void (*GSGetShader)(struct context*,void**,void**,UINT*); void (*HSGetShader)(struct context*,void**,void**,UINT*); void (*DSGetShader)(struct context*,void**,void**,UINT*); };
struct context {struct vtable *lpVtbl;};
static void capture(struct context *c,struct jnet_image_saved *s) {
 memset(s,0,sizeof(*s));
#define JNET_GET_SHADER(stage,shader,classes,count) do { \
    c->lpVtbl->stage##GetShader(c,&s->shader,NULL,&s->count); \
    if(s->shader && s->count && s->count<=256) \
        c->lpVtbl->stage##GetShader(c,NULL,s->classes,&s->count); \
    if(!s->shader)s->count=0; \
} while(0)
    JNET_GET_SHADER(VS,vs,vc,vn);JNET_GET_SHADER(PS,ps,pc,pn);
    JNET_GET_SHADER(GS,gs,gc,gn);JNET_GET_SHADER(HS,hs,hc,hn);JNET_GET_SHADER(DS,ds,dc,dn);

#undef JNET_GET_SHADER
}
static int jnet_image_saved_valid(const struct jnet_image_saved *s) {
    if(s->viewport_count>D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)return 0;
#define JNET_CLASSES_VALID(classes,count) do { \
    if(s->count>256)return 0; \
    for(UINT i=0;i<s->count;++i)if(!s->classes[i])return 0; \
} while(0)
    JNET_CLASSES_VALID(vc,vn);JNET_CLASSES_VALID(pc,pn);JNET_CLASSES_VALID(gc,gn);
    JNET_CLASSES_VALID(hc,hn);JNET_CLASSES_VALID(dc,dn);
#undef JNET_CLASSES_VALID
    return 1;
}

int main(void){
 struct vtable vt={get,get,get,get,get};struct context c={&vt};struct jnet_image_saved s;
 mode=0;capture(&c,&s);assert(jnet_image_saved_valid(&s)&&s.vn==0&&s.dn==0);
 mode=1;capture(&c,&s);assert(jnet_image_saved_valid(&s)&&s.vn==3&&s.dc[2]==&token);
 mode=2;capture(&c,&s);assert(!jnet_image_saved_valid(&s));
 mode=3;capture(&c,&s);assert(!jnet_image_saved_valid(&s));
 return 0;
}
