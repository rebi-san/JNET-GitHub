#include "native_mesh_callbacks.h"
#include <assert.h>
#include <stdio.h>
static struct jc_callback_workspace workspace;
static unsigned char memory[0x82000];
static const uint64_t base=0x100000,meshes[3]={0x200000,0x210000,0x220000};
static int changing,head_reads;
static void put(unsigned off,uint64_t value) { memcpy(memory+off,&value,8); }
static int reader(void *ctx,uint64_t addr,void *out,size_t n) {
    (void)ctx;
    if (changing && addr==base+0x80030 && head_reads++==1) put(0x80030,0);
    if (addr<base || addr-base>sizeof(memory) || n>sizeof(memory)-(addr-base)) return 0;
    memcpy(out,memory+(addr-base),n); return 1;
}
static void fixture(unsigned count) {
    memset(memory,0,sizeof(memory)); changing=head_reads=0;
    put(0x80030,count ? base+0x100 : 0); put(0x81000,0x140600000);
    for (unsigned i=0;i<count;++i) {
        unsigned o=0x100+i*0x28;
        put(o,i+1<count ? base+o+0x28 : 0); put(o+8,meshes[i%3]); put(o+0x20,base+0x81000);
    }
}
int main(void) {
    struct jc_callback_snapshot s;
    fixture(3); assert(!jc_mesh_callbacks(reader,NULL,base,meshes,&s,&workspace));
    assert(s.owners[0].total==1 && jc_callback_equal(&s.owners[1],&s.owners[2]));
    fixture(4); assert(!jc_mesh_callbacks(reader,NULL,base,meshes,&s,&workspace));
    assert(!jc_callback_equal(&s.owners[0],&s.owners[1]));
    fixture(0); assert(!jc_mesh_callbacks(reader,NULL,base,meshes,&s,&workspace) && !s.owners[2].total);
    fixture(1); put(0x100,base+0x100); assert(jc_mesh_callbacks(reader,NULL,base,meshes,&s,&workspace)==JC_CALLBACK_CYCLE);
    fixture(8193); assert(jc_mesh_callbacks(reader,NULL,base,meshes,&s,&workspace)==JC_CALLBACK_NODE_CAP && s.visited==8192);
    fixture(27);
    for (unsigned i=0;i<27;++i) {
        unsigned callback=0x81000+(i/3)*8;
        put(0x100+i*0x28+0x20,base+callback); put(callback,0x140600000+(i/3)*0x100);
    }
    assert(jc_mesh_callbacks(reader,NULL,base,meshes,&s,&workspace)==JC_CALLBACK_TYPE_CAP);
    fixture(8192); assert(!jc_mesh_callbacks(reader,NULL,base,meshes,&s,&workspace) && s.visited==8192);
    fixture(257); assert(!jc_mesh_callbacks(reader,NULL,base,meshes,&s,&workspace));
    fixture(1); assert(jc_mesh_callbacks(reader,NULL,base,meshes,&s,NULL)==JC_CALLBACK_STORAGE);
    fixture(1); put(0x120,0); assert(jc_mesh_callbacks(reader,NULL,base,meshes,&s,&workspace)==JC_LAYOUT_READ);
    fixture(1); changing=1; assert(jc_mesh_callbacks(reader,NULL,base,meshes,&s,&workspace)==JC_LAYOUT_CHANGED);
    puts("Mesh callbacks: 11 synthetic checks passed; no game callbacks invoked.");
}
