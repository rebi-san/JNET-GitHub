
#define main cloth_header_main
#include "../diagnostic/test_native_companion_cloth.c"
#undef main
#define native_cloth_lease lease
struct callback_read_budget { int unused; };
#define ATLAS UINT64_C(0x2000000)
static unsigned char atlas_memory[6][0x200];
static unsigned resolved,selected,flag_reads[6];
static int atlas_failure;
static int JCProbeCompanionFeatures[40];
typedef int LONG;
static int InterlockedExchange(int *p,int n) { int old=*p;*p=n;return old; }
static int callback_read(void *c,uint64_t at,void *out,size_t bytes) {
    if (at==IMAGE+0x403f050 && bytes==8) { uint64_t type=IMAGE+0x403f060;memcpy(out,&type,8);return 1; }
    if (at==IMAGE+0x403f060 && bytes==8) { uint64_t type=IMAGE+0x5cd4c8;memcpy(out,&type,8);return 1; }
    if (at>=ATLAS && at-ATLAS<sizeof(atlas_memory) && bytes<=sizeof(atlas_memory)-(at-ATLAS)) {
        memcpy(out,(unsigned char *)atlas_memory+(at-ATLAS),bytes);
        if (atlas_failure==9 && (at-ATLAS)%0x200==0x1a4 && bytes==4 && ++flag_reads[(at-ATLAS)/0x200]==2) {
            uint32_t changed=1;memcpy(out,&changed,4);
        }
        return 1;
    }
    return read_memory(c,at,out,bytes);
}
static int evaluation_writable(void *c,uint64_t at,size_t bytes) {
    (void)c;(void)at;(void)bytes;return atlas_failure!=1;
}
static uintptr_t fake_lookup(uintptr_t manager,uintptr_t type,const char *name) {
    assert(manager==MATERIALS && type==IMAGE+0x403f060);
    const char *names[2][3];uint32_t outfit;memcpy(&outfit,dude+0x70,4);assert(jc_cloth_atlas_names(outfit,names));
    if (atlas_failure==8) return 0;
    for (unsigned i=0;i<6;++i) if (!strcmp(name,names[i/3][i%3])) return ATLAS+i*0x200;
    assert(0);return 0;
}
static void fake_resolve(uintptr_t manager,const char *name,uintptr_t holder) {
    assert(manager==MATERIALS && resolved<6);
    unsigned i=resolved++;uint64_t at=fake_lookup(manager,IMAGE+0x403f060,name);unsigned char *p=atlas_memory[i];
    assert(at==ATLAS+i*0x200 && holder==BARN+0x666d68+(4+i/3)*0x630+(i%3)*0x28);
    put64(resolve(holder,16),0,at);put64(resolve(holder,16),8,at+0x28);
    put64(p,0x38,at+0x40);put64(p,0x108,at+0x180);
    /* No fabricated name string in the payload: native binding proves identity. */
    if (atlas_failure==2) put64(resolve(holder,16),8,at+0x30);
    if (atlas_failure==3) put64(p,0x108,0);
    if (atlas_failure==4 || (atlas_failure==7 && i<3)) put32(p,0x1a4,0x80);
}
static void fake_select(uintptr_t barn_ptr,unsigned role,uintptr_t manager,const char *a,const char *b,const char *c) {
    assert(barn_ptr==BARN && manager==MATERIALS && role==4+selected && a && b && c);++selected;
    if (atlas_failure==5 && selected==1) barn[0x666d68+2*0x630]^=1;
    if (atlas_failure==6) put64(game,0x28,MATERIALS+8);
}
static void reset_atlas(void) {
    reset();assert(!step(1,1));put64(game,0x28,MATERIALS);
    resolved=selected=0;atlas_failure=0;memset(flag_reads,0,sizeof(flag_reads));memset(atlas_memory,0,sizeof(atlas_memory));
}
static int companion_cloth_atlas(struct callback_read_budget *budget,uintptr_t image,uint64_t dude,uint64_t barn) {
    const char *names[2][3]; uint32_t outfit; uint64_t manager,again,type,vtable,entries[2][3];
    unsigned char local_before[2][3][0x28],local_after[0x28];
    InterlockedExchange(&JCProbeCompanionFeatures[11],30);
    if (dude!=native_cloth_lease.dude || barn!=native_cloth_lease.barn ||
        native_cloth_lease.state!=JC_CLOTH_LIVE ||
        !callback_read(budget,dude+0x70,&outfit,4) || !jc_cloth_atlas_names(outfit,names) ||
        !callback_read(budget,native_cloth_lease.game+0x28,&manager,8) || !manager ||
        !callback_read(budget,image+0x403f050,&type,8) || !type ||
        !callback_read(budget,type,&vtable,8) || vtable!=image+0x5cd4c8) return 0;
    uintptr_t (*lookup)(uintptr_t,uintptr_t,const char *)=(void *)(uintptr_t)fake_lookup;
    for (unsigned i=0;i<2;++i) for (unsigned j=0;j<3;++j) {
        InterlockedExchange(&JCProbeCompanionFeatures[11],40+(LONG)(i*3+j));
        if (!evaluation_writable(budget,barn+0x666d68+(4+i)*0x630+j*0x28,0x28) ||
            !callback_read(budget,barn+0x666d68+(2+i)*0x630+j*0x28,local_before[i][j],0x28)) return 0;
        entries[i][j]=lookup((uintptr_t)manager,(uintptr_t)type,names[i][j]);
        if (!entries[i][j]) return 0;
    }
    void (*resolve)(uintptr_t,const char *,uintptr_t)=(void *)(uintptr_t)fake_resolve;
    for (unsigned i=0;i<2;++i) for (unsigned j=0;j<3;++j) {
        uintptr_t holder=(uintptr_t)barn+0x666d68+(4+i)*0x630+j*0x28;
        resolve((uintptr_t)manager,names[i][j],holder);
        uint64_t texture;
        int code=jc_cloth_atlas_loaded(callback_read,budget,holder,entries[i][j],i==1,&texture);
        InterlockedExchange(&JCProbeCompanionFeatures[11],100+(LONG)(i*3+j)*10+code);
        if (code || (i==1 && !evaluation_writable(budget,texture+0xa,2))) return 0;
    }
    void (*select)(uintptr_t,unsigned,uintptr_t,const char *,const char *,const char *)=(void *)(uintptr_t)fake_select;
    for (unsigned i=0;i<2;++i) select((uintptr_t)barn,4+i,(uintptr_t)manager,names[i][0],names[i][1],names[i][2]);
    for (unsigned i=0;i<2;++i) for (unsigned j=0;j<3;++j) {
        int code=jc_cloth_atlas_loaded(callback_read,budget,barn+0x666d68+(4+i)*0x630+j*0x28,entries[i][j],i==1,NULL);
        InterlockedExchange(&JCProbeCompanionFeatures[11],200+(LONG)(i*3+j)*10+code);
        if (code || lookup((uintptr_t)manager,(uintptr_t)type,names[i][j])!=entries[i][j] ||
            !callback_read(budget,barn+0x666d68+(2+i)*0x630+j*0x28,local_after,0x28) ||
            memcmp(local_before[i][j],local_after,0x28)) return 0;
    }
    InterlockedExchange(&JCProbeCompanionFeatures[11],31);
    return callback_read(budget,native_cloth_lease.game+0x28,&again,8) && again==manager;
}

int main(void) {
    struct callback_read_budget budget={0};unsigned scenarios=0;
    for (unsigned outfit=0;outfit<7;++outfit) {
        reset_atlas();put32(dude,0x70,outfit);
        assert(companion_cloth_atlas(&budget,IMAGE,DUDE,BARN) && resolved==6 && selected==2);++scenarios;
    }
    for (int bad=1;bad<=6;++bad) {
        reset_atlas();atlas_failure=bad;
        assert(!companion_cloth_atlas(&budget,IMAGE,DUDE,BARN));++scenarios;
        if (bad<=4) assert(!selected);
    }
    reset_atlas();put32(dude,0x70,7);assert(!companion_cloth_atlas(&budget,IMAGE,DUDE,BARN) && !resolved);++scenarios;
    reset_atlas();lease.state=JC_CLOTH_RETIRED;assert(!companion_cloth_atlas(&budget,IMAGE,DUDE,BARN) && !resolved);++scenarios;
    reset_atlas();failed_read=GAME+0x28;assert(!companion_cloth_atlas(&budget,IMAGE,DUDE,BARN) && !resolved);++scenarios;
    reset_atlas();atlas_failure=7;assert(companion_cloth_atlas(&budget,IMAGE,DUDE,BARN) && selected==2);++scenarios;
    reset_atlas();atlas_failure=8;assert(!companion_cloth_atlas(&budget,IMAGE,DUDE,BARN) && !resolved && JCProbeCompanionFeatures[11]==40);++scenarios;
    reset_atlas();atlas_failure=9;assert(!companion_cloth_atlas(&budget,IMAGE,DUDE,BARN) && !selected && JCProbeCompanionFeatures[11]==106);++scenarios;
    printf("Native atlas: %u production scenarios; all seven outfits, exact resource names, protected textures, missing resources, original holders and manager mutation checked. Native loaders simulated.\n",scenarios);
}
