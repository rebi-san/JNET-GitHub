#include <assert.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "jnet_canyon_collision.h"
#include "canyon_native_fixture.h"

struct contact {float normal[4],distance;uint32_t pad;uintptr_t hull;};
typedef void __attribute__((ms_abi)) native_distance(const struct jnet_canyon_hull *,const float *,struct contact *);
static void dataset(native_distance *distance,const char *path) {
    FILE *file=fopen(path,"rb");assert(file);
    assert(!fseek(file,0,SEEK_END));long size=ftell(file);assert(size>24);rewind(file);
    unsigned char *bytes=malloc((size_t)size);assert(bytes && fread(bytes,1,(size_t)size,file)==(size_t)size);fclose(file);
    assert(!memcmp(bytes,"JNCANY1\0",8));uint32_t counts[4];memcpy(counts,bytes+8,sizeof(counts));
    assert(24ull+counts[0]*sizeof(struct jnet_canyon_node)+(uint64_t)counts[1]*sizeof(struct jnet_canyon_triangle)==(size_t)size);
    assert(counts[0] && counts[1] && counts[2] && counts[2]<=counts[1] && !counts[3]);
    const struct jnet_canyon_node *nodes=(void *)(bytes+24);
    const struct jnet_canyon_triangle *tri=(void *)(bytes+24+counts[0]*sizeof(*nodes));
    struct jnet_canyon_hull *hulls=calloc(counts[1],sizeof(*hulls));assert(hulls);
    unsigned *owners=calloc(counts[2],sizeof(*owners));assert(owners);
    unsigned invalid=0;float maximum_error=0;
    for(unsigned i=0;i<counts[1];++i){
        assert(tri[i].owner<counts[2]);++owners[tri[i].owner];assert(jnet_canyon_make_hull(&hulls[i],&tri[i],0,i));
        double u[3],v[3],n[3],length=0;
        for(unsigned a=0;a<3;++a){u[a]=(double)tri[i].v[1][a]-tri[i].v[0][a];v[a]=(double)tri[i].v[2][a]-tri[i].v[0][a];}
        for(unsigned a=0;a<3;++a){n[a]=u[(a+1)%3]*v[(a+2)%3]-u[(a+2)%3]*v[(a+1)%3];length+=n[a]*n[a];}
        length=sqrt(length);
        for(int side=-1;side<=1;side+=2){
            float point[4]={0};struct contact contact={0};
            for(unsigned a=0;a<3;++a)point[a]=(float)(((double)tri[i].v[0][a]+tri[i].v[1][a]+tri[i].v[2][a])/3+side*.5*n[a]/length);
            distance(&hulls[i],point,&contact);
            float error=fabsf(contact.distance-.495f);if(error>maximum_error)maximum_error=error;
            if(!isfinite(contact.distance) || !isfinite(contact.normal[0]) || !isfinite(contact.normal[1]) || !isfinite(contact.normal[2]) || error>.05f){
                if(invalid<8)fprintf(stderr,"triangle %u owner %u side %d distance %g normal %g %g %g\n",i,tri[i].owner,side,contact.distance,contact.normal[0],contact.normal[1],contact.normal[2]);
                ++invalid;
            }
        }
    }
    for(unsigned i=0;i<counts[2];++i)assert(owners[i]);
    const float radii[]={1,5,25};
    for(unsigned q=0;q<sizeof(radii)/sizeof(radii[0]);++q){
        unsigned capped=0,queries=0,peak=0;
        for(unsigned i=0;i<counts[1];i+=127){
            float point[3];for(unsigned a=0;a<3;++a)point[a]=(tri[i].v[0][a]+tri[i].v[1][a]+tri[i].v[2][a])/3;
            uintptr_t output[4097]={0};unsigned overflow=0;output[0]=123;output[4096]=456;
            unsigned n=jnet_canyon_query(nodes,counts[0],hulls,counts[1],point,radii[q],77,output,1,4096,&overflow);
            assert(output[0]==123 && output[4096]==456 && n<=4096);
            assert(!overflow || n==4096);
            int found=0;for(unsigned j=1;j<n;++j)if(output[j]==(uintptr_t)&hulls[i])found=1;
            assert(found);if(overflow)++capped;if(n-1>peak)peak=n-1;++queries;
        }
        printf("Radius %g: %u queries, peak %u added candidates, %u capped; source triangles retained and guards intact.\n",radii[q],queries,peak,capped);
    }
    printf("Full Canyon dataset: %u instances, %u prisms; native distance failures=%u, maximum error=%g.\n",counts[2],counts[1],invalid,maximum_error);
    assert(!invalid);free(owners);free(hulls);free(bytes);
}

int main(int argc,char **argv) {
    unsigned char *image=mmap(NULL,0x700000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
    assert(image!=MAP_FAILED);
    for(unsigned i=0;i<sizeof(canyon_native_parts)/sizeof(canyon_native_parts[0]);++i)
        memcpy(image+canyon_native_parts[i].rva,canyon_native_parts[i].data,canyon_native_parts[i].size);
    assert(!mprotect(image,0x700000,PROT_READ|PROT_EXEC));
    native_distance *distance=(native_distance *)(image+0xe82c0);
    struct jnet_canyon_triangle tri={.v={{0,0,0},{8,0,0},{0,0,8}}};
    struct jnet_canyon_hull hulls[2];assert(jnet_canyon_make_hull(&hulls[0],&tri,0,1));
    float point[4]={2,1,2,0};struct contact c={0};distance(&hulls[0],point,&c);
    assert(isfinite(c.distance) && fabsf(c.distance-0.995f)<0.001f && c.hull==(uintptr_t)&hulls[0]);
    point[1]=-1;distance(&hulls[0],point,&c);assert(fabsf(c.distance-0.995f)<0.001f);
    point[1]=0;distance(&hulls[0],point,&c);assert(c.distance<=0);
    point[0]=7;point[2]=7;distance(&hulls[0],point,&c);assert(c.distance>4);
    for(unsigned i=0;i<3;++i)tri.v[i][0]+=20;
    assert(jnet_canyon_make_hull(&hulls[1],&tri,0,2));
    struct jnet_canyon_node nodes[3]={
        {.lo={0,-.01f,0},.hi={28,.01f,8},.first=1,.last=0x80000002u},
        {.lo={0,-.01f,0},.hi={8,.01f,8},.first=0,.last=1},
        {.lo={20,-.01f,0},.hi={28,.01f,8},.first=1,.last=1}};
    uintptr_t out[4]={123,0,0,456};unsigned overflow=0;
    float opening[3]={14,0,2};assert(jnet_canyon_query(nodes,3,hulls,2,opening,1,77,out,1,3,&overflow)==1);
    point[0]=2;point[2]=2;
    assert(jnet_canyon_query(nodes,3,hulls,2,point,1,77,out,1,3,&overflow)==2 && out[0]==123 && out[3]==456 && hulls[0].owner==77);
    assert(jnet_canyon_query(nodes,3,hulls,2,opening,50,77,out,1,2,&overflow)==2 && overflow==1 && out[3]==456);
    struct jnet_canyon_node leaf={.lo={0,-.01f,0},.hi={28,.01f,8},.first=0,.last=2};
    point[0]=22;overflow=0;
    assert(jnet_canyon_query(&leaf,1,hulls,2,point,50,77,out,1,2,&overflow)==2 && overflow==1);
    assert(out[0]==123 && out[1]==(uintptr_t)&hulls[1] && out[3]==456);
    tri.v[1][0]=NAN;assert(!jnet_canyon_make_hull(&hulls[1],&tri,0,2));
    puts("Canyon collision: native point distance on both sides, triangle edges, open passage, bounded BVH query and original candidates passed.");
    assert(argc==2);dataset(distance,argv[1]);
    munmap(image,0x700000);
    return 0;
}
