#include "native_traveler_trial.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    struct jc_traveler_identity original[2]={0},added={0};
    const uint64_t barn=0x100000;
    original[0].glyph=barn+0x10; original[1].glyph=barn+0x10+0x10d0;
    for (unsigned p=0;p<2;++p) {
        original[p].mesh.entries=0x10000+p*0x1000; original[p].mesh.count=4;
        for (unsigned i=0;i<14;++i) original[p].sounds[i]=p*100+i;
    }
    added.mesh.entries=0x12000; added.mesh.count=4;
    added.glyph=barn+0x10+2*0x10d0;
    for (unsigned i=0;i<14;++i) added.sounds[i]=200+i;
    struct jc_traveler_identity playing=original[0];playing.sounds[0]=9999;
    assert(jc_traveler_storage_same(original,&playing));
    playing.mesh.entries+=8;assert(!jc_traveler_storage_same(original,&playing));
    playing=original[0];++playing.glyph;assert(!jc_traveler_storage_same(original,&playing));
    unsigned sounds=0;
    assert(!jc_traveler_added(original,&added,barn,2,3,&sounds) && sounds==14);
    added.sounds[3]=UINT64_MAX;
    assert(!jc_traveler_added(original,&added,barn,2,3,&sounds) && sounds==13);
    added.sounds[3]=original[1].sounds[7];
    assert(jc_traveler_added(original,&added,barn,2,3,&sounds)==7);
    added.sounds[3]=added.sounds[4];
    assert(jc_traveler_added(original,&added,barn,2,3,&sounds)==7);
    added.sounds[3]=203;
    assert(jc_traveler_added(original,&added,barn,1024,1025,&sounds)==5);
    assert(jc_traveler_added(original,&added,barn,2,4,&sounds)==5);
    added.glyph=original[1].glyph;
    assert(jc_traveler_added(original,&added,barn,2,3,&sounds)==5);
    added.glyph=barn+0x10+2*0x10d0+1;
    assert(jc_traveler_added(original,&added,barn,2,3,&sounds)==5);
    added.glyph=barn+0x10+2*0x10d0;
    added.mesh.entries=original[1].mesh.entries+0x50;
    assert(jc_traveler_added(original,&added,barn,2,3,&sounds)==6);
    assert(!jc_traveler_glyph_slot(UINT64_MAX-7,2,16));
    puts("Full traveler ownership: glyph append/bounds, source aliasing, animation tables and sound handles passed");
    return 0;
}
