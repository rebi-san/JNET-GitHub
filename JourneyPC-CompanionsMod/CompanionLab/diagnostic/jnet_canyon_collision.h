#ifndef JNET_CANYON_COLLISION_H
#define JNET_CANYON_COLLISION_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdatomic.h>

struct jnet_canyon_node {float lo[3],hi[3];uint32_t first,last;};
struct jnet_canyon_triangle {float v[3][3];uint32_t owner;};
struct jnet_canyon_hull {
    uintptr_t vtable,reserved;
    float lo[4],hi[4],direction[4],light[4];
    const unsigned char *triangles,*planes,*edges;
    float (*vertices)[4];
    uint32_t tri_count,plane_count,edge_count,vert_count,active,flags;
    float strength;uint32_t pad;
    _Atomic uintptr_t owner;uintptr_t next;
    char name[40];uintptr_t cached;
    float points[6][4];
};
_Static_assert(offsetof(struct jnet_canyon_hull,lo)==0x10,"Hull bounds layout");
_Static_assert(offsetof(struct jnet_canyon_hull,triangles)==0x50,"Hull triangles layout");
_Static_assert(offsetof(struct jnet_canyon_hull,vert_count)==0x7c,"Hull counts layout");
_Static_assert(offsetof(struct jnet_canyon_hull,name)==0xa0,"Hull name layout");
_Static_assert(offsetof(struct jnet_canyon_hull,points)==0xd0,"Hull storage layout");
static const unsigned char jnet_canyon_triangles[]={0,1,2,5,4,3,0,3,1,1,3,4,1,4,2,2,4,5,2,5,0,0,5,3};
static const unsigned char jnet_canyon_planes[]={1,1,2,2,2};
static const unsigned char jnet_canyon_edges[]={0,1,1,2,2,0,3,4,4,5,5,3,0,3,1,4,2,5};

static int jnet_canyon_make_hull(struct jnet_canyon_hull *h,const struct jnet_canyon_triangle *t,uintptr_t vtable,unsigned index) {
    double u[3],v[3],n[3],length=0;
    for(unsigned a=0;a<3;++a){
        for(unsigned i=0;i<3;++i)if(!isfinite(t->v[i][a]) || fabsf(t->v[i][a])>100000)return 0;
        u[a]=(double)t->v[1][a]-t->v[0][a];v[a]=(double)t->v[2][a]-t->v[0][a];
    }
    for(unsigned a=0;a<3;++a){n[a]=u[(a+1)%3]*v[(a+2)%3]-u[(a+2)%3]*v[(a+1)%3];length+=n[a]*n[a];}
    if(!isfinite(length) || length<1e-10)return 0;
    length=sqrt(length);
    memset(h,0,sizeof(*h));h->vtable=vtable;
    for(unsigned a=0;a<3;++a){
        h->lo[a]=INFINITY;h->hi[a]=-INFINITY;
        for(unsigned i=0;i<6;++i){
            h->points[i][a]=(float)(t->v[i%3][a]+(i<3?0.005:-0.005)*n[a]/length);
            if(h->points[i][a]<h->lo[a])h->lo[a]=h->points[i][a];
            if(h->points[i][a]>h->hi[a])h->hi[a]=h->points[i][a];
        }
    }
    h->triangles=jnet_canyon_triangles;h->planes=jnet_canyon_planes;h->edges=jnet_canyon_edges;h->vertices=h->points;
    h->tri_count=8;h->plane_count=5;h->edge_count=9;h->vert_count=6;h->active=1;h->flags=2;h->strength=1;
    snprintf(h->name,sizeof(h->name),"JNETCanyon%08u",index);
    return 1;
}

static double jnet_canyon_distance2(const float lo[3],const float hi[3],const float p[3]) {
    double squared=0;
    for(unsigned a=0;a<3;++a){double d=p[a]<lo[a]?lo[a]-p[a]:p[a]>hi[a]?p[a]-hi[a]:0;squared+=d*d;}
    return squared;
}

static double jnet_canyon_candidate_distance(uintptr_t candidate,const float p[3]) {
    const struct jnet_canyon_hull *h=(const void *)candidate;
    return jnet_canyon_distance2(h->lo,h->hi,p);
}

static void jnet_canyon_heap_down(uintptr_t *heap,unsigned count,unsigned at,const float p[3]) {
    for(;;){
        unsigned child=at*2+1;if(child>=count)return;
        if(child+1<count && jnet_canyon_candidate_distance(heap[child+1],p)>jnet_canyon_candidate_distance(heap[child],p))++child;
        if(jnet_canyon_candidate_distance(heap[at],p)>=jnet_canyon_candidate_distance(heap[child],p))return;
        uintptr_t swap=heap[at];heap[at]=heap[child];heap[child]=swap;at=child;
    }
}

static unsigned jnet_canyon_query(const struct jnet_canyon_node *nodes,unsigned node_count,
    struct jnet_canyon_hull *hulls,unsigned hull_count,const float p[3],float radius,
    uintptr_t owner,uintptr_t *output,unsigned used,unsigned capacity,unsigned *overflow) {
    if(!nodes || !node_count || !hulls || !output || used>=capacity || !isfinite(radius) || radius<0)return used;
    for(unsigned a=0;a<3;++a)if(!isfinite(p[a]))return used;
    unsigned stack[64],pending=1,first=used,heap_ready=0;stack[0]=0;
    while(pending){
        unsigned index=stack[--pending];if(index>=node_count)return used;
        const struct jnet_canyon_node *n=&nodes[index];
        double limit=heap_ready?jnet_canyon_candidate_distance(output[first],p):(double)radius*radius;
        if(jnet_canyon_distance2(n->lo,n->hi,p)>limit)continue;
        if(n->last&0x80000000u){
            if(pending>62)return used;
            stack[pending++]=n->first;stack[pending++]=n->last&0x7fffffffu;
        }else{
            if(n->first>hull_count || n->last>hull_count-n->first)return used;
            for(unsigned i=0;i<n->last;++i){
                struct jnet_canyon_hull *h=&hulls[n->first+i];
                double distance=jnet_canyon_distance2(h->lo,h->hi,p);
                if(distance>(double)radius*radius)continue;
                if(used==capacity){
                    if(overflow)++*overflow;
                    if(!heap_ready){
                        for(unsigned at=(used-first)/2;at>0;--at)jnet_canyon_heap_down(output+first,used-first,at-1,p);
                        heap_ready=1;
                    }
                    if(distance<jnet_canyon_candidate_distance(output[first],p)){
                        atomic_store_explicit(&h->owner,owner,memory_order_relaxed);output[first]=(uintptr_t)h;
                        jnet_canyon_heap_down(output+first,used-first,0,p);
                    }
                    continue;
                }
                atomic_store_explicit(&h->owner,owner,memory_order_relaxed);output[used++]=(uintptr_t)h;
            }
        }
    }
    return used;
}
#endif
