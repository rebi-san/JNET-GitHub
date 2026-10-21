
#include "native_local_read.h"
#include "native_read_batch.h"
#include <assert.h>
#include <stdio.h>
typedef uint64_t ULONGLONG;
static unsigned char memory[8192];
static unsigned queries, copies, fallback, denied;
static int JCProbeCompanionFeatures[40];
static uint64_t now;
static uint64_t GetTickCount64(void) {return now;}
static int jc_local_windows_query(void *ctx,uint64_t at,struct jc_local_region *r) {
 (void)ctx;++queries;if(at<0x100000 || at>=0x102000 || denied)return 0;
 *r=(struct jc_local_region){0x100000,0x102000};return 1;
}
static int jc_local_windows_copy(void *ctx,uint64_t at,void *out,size_t n) {
 (void)ctx;++copies;if(denied || at<0x100000 || n>sizeof(memory) || at-0x100000>sizeof(memory)-n)return 0;
 memcpy(out,memory+(at-0x100000),n);return 1;
}
static int own_read(void *ctx,uint64_t at,void *out,size_t n) {
 ++fallback;return jc_local_windows_copy(ctx,at,out,n);
}
struct callback_read_budget { ULONGLONG deadline; int expired,read_failed; unsigned reads,fast; struct jc_local_read_cache local; struct jc_read_batch *batch; };
static int callback_raw(void *opaque,uint64_t address,void *out,size_t size) {
    struct callback_read_budget *budget=opaque;
    if (GetTickCount64()>=budget->deadline) { budget->expired=1; return 0; }
    ++budget->reads;
    int ok=(budget->fast || JCProbeCompanionFeatures[8]==1) ? jc_local_read(&budget->local,1,jc_local_windows_query,jc_local_windows_copy,NULL,address,out,size) :
        own_read(NULL,address,out,size);

    return ok;
}
static int callback_read(void *opaque,uint64_t address,void *out,size_t size) {
    struct callback_read_budget *budget=opaque;
    if (GetTickCount64()>=budget->deadline) { budget->expired=1; return 0; }
    int ok=budget->batch && !budget->fast && JCProbeCompanionFeatures[8]!=1 ? jc_read_batch_get(budget->batch,callback_raw,budget,address,out,size) :
        callback_raw(budget,address,out,size);
    if (!ok && !budget->expired) budget->read_failed=1;
    return ok;
}

int main(void) {
 struct callback_read_budget b={.deadline=100,.fast=1};unsigned char out;
 for(unsigned i=0;i<1000;++i) {memory[0]=(unsigned char)i;assert(callback_read(&b,0x100000,&out,1) && out==(unsigned char)i);}
 assert(queries==1 && copies==1000 && !fallback && b.reads==1000);
 denied=1;assert(!callback_read(&b,0x100000,&out,1) && b.read_failed && !b.local.used);
 denied=0;b=(struct callback_read_budget){.deadline=100,.fast=1};
 assert(callback_read(&b,0x100000,&out,1) && queries==2);
 b=(struct callback_read_budget){.deadline=100};
 assert(callback_read(&b,0x100000,&out,1) && fallback==1 && queries==2);
 JCProbeCompanionFeatures[8]=1;b=(struct callback_read_budget){.deadline=100};
 assert(callback_read(&b,0x100000,&out,1) && fallback==1 && queries==3);
 JCProbeCompanionFeatures[8]=2;
 now=100;unsigned previous=copies;
 assert(!callback_read(&b,0x100000,&out,1) && b.expired && copies==previous);
 b=(struct callback_read_budget){.deadline=200};denied=1;
 assert(!callback_read(&b,0x100000,&out,1) && b.read_failed && fallback==2);
 denied=0;now=0;struct jc_read_batch batch={0};b=(struct callback_read_budget){.deadline=200,.batch=&batch};
 unsigned start=fallback;unsigned char block[8];
 for(unsigned i=0;i<100;++i)assert(callback_read(&b,0x100000+i*8,block,8));
 assert(fallback==start+1 && b.reads==1);
 memory[0]=77;assert(callback_read(&b,0x100000,block,8) && block[0]==77 && b.reads==2);
 jc_read_batch_reset(&batch);memory[8]=88;
 assert(callback_read(&b,0x100008,block,8) && block[0]==88 && b.reads==3);
 puts("Production callback reader: 1000 fresh reads, one permission query; fault rejection, fresh inspection, fallback and deadline checks passed.");
}
