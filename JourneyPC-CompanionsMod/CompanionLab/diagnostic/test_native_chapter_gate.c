#include "native_chapter_gate.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    struct jc_chapter_gate g={0}; char source[64]="Level_Desert",out[64]={0};
    assert(jc_chapter_defer(&g,1,2,source));
    memset(source,0,sizeof(source));
    for (unsigned i=0;i<100;++i) assert(!jc_chapter_take(&g,1,2,0,out) && g.pending);
    assert(jc_chapter_take(&g,1,2,1,out) && !strcmp(out,"Level_Desert"));
    assert(!jc_chapter_take(&g,1,2,1,out));
    strcpy(source,"Level_Cave"); assert(jc_chapter_defer(&g,1,2,source));
    strcpy(source,"Level_Graveyard"); assert(jc_chapter_defer(&g,1,2,source));
    assert(jc_chapter_take(&g,1,2,1,out) && !strcmp(out,"Level_Graveyard"));
    assert(jc_chapter_defer(&g,1,2,source));
    assert(!jc_chapter_take(&g,1,2,-1,out) && g.failed && g.pending);
    assert(!jc_chapter_take(&g,1,2,1,out));
    memset(&g,0,sizeof(g)); assert(jc_chapter_defer(&g,1,2,source));
    assert(!jc_chapter_take(&g,3,2,1,out) && g.failed);
    memset(&g,0,sizeof(g)); assert(jc_chapter_defer(&g,1,2,source));
    assert(!jc_chapter_take(&g,1,3,1,out) && g.failed);
    memset(&g,0,sizeof(g)); memset(source,'x',sizeof(source));
    assert(!jc_chapter_defer(&g,1,2,source) && g.failed);
    puts("Chapter gate: retained request after native clear, delayed drain, once-only replay, latest target, failed drain, changed game/thread and unterminated target rejected; no game runtime");
    return 0;
}
