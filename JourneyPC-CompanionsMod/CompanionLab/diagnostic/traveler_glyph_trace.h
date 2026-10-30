#ifndef JC_TRAVELER_GLYPH_TRACE_H
#define JC_TRAVELER_GLYPH_TRACE_H
static struct {
    PVOID volatile material,model,shader,queue,item[4];
    volatile LONG commits,denied,queued,dispatch,dispatch_result,built,build_result,commands,passes;
} traveler_glyph_trace[2];
static uintptr_t traveler_glyph_trace_pointer(PVOID volatile *p) {
    return (uintptr_t)InterlockedCompareExchangePointer(p,NULL,NULL);
}
static void traveler_glyph_trace_bind(unsigned n,uintptr_t material,uintptr_t model,uintptr_t shader,uintptr_t render) {
    if(n>=2)return;
    uint64_t queue=0,begin=0,end=0,items[4]={0};
    own_read(NULL,render+0x60,&queue,8);
    if(own_read(NULL,model+0x60,&begin,8) && own_read(NULL,model+0x68,&end,8) &&
       end>=begin && end-begin<=sizeof(items) && !((end-begin)%8))
        own_read(NULL,begin,items,(size_t)(end-begin));
    InterlockedExchangePointer(&traveler_glyph_trace[n].material,(void *)material);
    InterlockedExchangePointer(&traveler_glyph_trace[n].model,(void *)model);
    InterlockedExchangePointer(&traveler_glyph_trace[n].shader,(void *)shader);
    InterlockedExchangePointer(&traveler_glyph_trace[n].queue,(void *)(uintptr_t)queue);
    for(unsigned i=0;i<4;++i)InterlockedExchangePointer(&traveler_glyph_trace[n].item[i],(void *)(uintptr_t)items[i]);
}
static void traveler_glyph_trace_commit(uintptr_t material,int allowed) {
    for(unsigned n=0;n<2;++n)if(material && material==traveler_glyph_trace_pointer(&traveler_glyph_trace[n].material))
        InterlockedIncrement(allowed?&traveler_glyph_trace[n].commits:&traveler_glyph_trace[n].denied);
}
static void traveler_glyph_trace_queue(uintptr_t queue) {
    uintptr_t models[2]={0};int matching=0;
    for(unsigned n=0;n<2;++n)if(queue && queue==traveler_glyph_trace_pointer(&traveler_glyph_trace[n].queue)) {
        models[n]=traveler_glyph_trace_pointer(&traveler_glyph_trace[n].model);matching=1;
    }
    if(!matching)return;
    uint32_t count=0;uint64_t rows[64][2];
    if(!own_read(NULL,queue+0x10080,&count,4) || count>4096)return;
    for(unsigned at=0;at<count;at+=64) {
        unsigned take=count-at<64?count-at:64;
        if(!own_read(NULL,queue+0x80+16*at,rows,take*16))return;
        for(unsigned i=0;i<take;++i)for(unsigned n=0;n<2;++n)
            if(models[n] && rows[i][0]==models[n])InterlockedIncrement(&traveler_glyph_trace[n].queued);
    }
}
static void traveler_glyph_trace_dispatch(uintptr_t item,uint32_t result) {
    for(unsigned n=0;n<2;++n)for(unsigned i=0;i<4;++i)
        if(item && item==traveler_glyph_trace_pointer(&traveler_glyph_trace[n].item[i])) {
            InterlockedIncrement(&traveler_glyph_trace[n].dispatch);
            InterlockedExchange(&traveler_glyph_trace[n].dispatch_result,(LONG)result);break;
        }
}
static int traveler_glyph_trace_build(uintptr_t shader) {
    for(unsigned n=0;n<2;++n)if(shader && shader==traveler_glyph_trace_pointer(&traveler_glyph_trace[n].shader))return (int)n;
    return -1;
}
static void traveler_glyph_trace_built(int n,uint32_t result,uint64_t before,uint64_t after,uint32_t passes) {
    if(n<0 || n>=2)return;
    InterlockedIncrement(&traveler_glyph_trace[n].built);
    InterlockedExchange(&traveler_glyph_trace[n].build_result,(LONG)result);
    InterlockedExchange(&traveler_glyph_trace[n].passes,(LONG)passes);
    if(before && after>before)InterlockedIncrement(&traveler_glyph_trace[n].commands);
}
static void traveler_glyph_trace_report(void) {
    char line[384];
    snprintf(line,sizeof(line),"[JNET] floating ownership guard/detail/pool=%u/%u/%u first-detail/pool=%u/%u slots=%u retries/recovered=%u/%u slot/link/head/tail=%llx/%llx/%llx/%llx",
        traveler_glyph_init.failed,traveler_glyph_init.slot_detail,traveler_glyph_init.slot_result,
        traveler_glyph_init.first_detail,traveler_glyph_init.first_pool,traveler_glyph_init.live_slots,
        traveler_glyph_init.retries,traveler_glyph_init.recovered,
        (unsigned long long)traveler_glyph_init.slot,(unsigned long long)traveler_glyph_init.link,
        (unsigned long long)traveler_glyph_init.head,(unsigned long long)traveler_glyph_init.tail);
    message(line);
    for(unsigned n=0;n<2;++n) {
        snprintf(line,sizeof(line),"[JNET] floating pipeline actor=%u commit/denied/queued/dispatch/build/commands=%ld/%ld/%ld/%ld/%ld/%ld results=%ld/%ld passes=%ld glyph-guard=%u",
            n,(long)InterlockedCompareExchange(&traveler_glyph_trace[n].commits,0,0),
            (long)InterlockedCompareExchange(&traveler_glyph_trace[n].denied,0,0),
            (long)InterlockedCompareExchange(&traveler_glyph_trace[n].queued,0,0),
            (long)InterlockedCompareExchange(&traveler_glyph_trace[n].dispatch,0,0),
            (long)InterlockedCompareExchange(&traveler_glyph_trace[n].built,0,0),
            (long)InterlockedCompareExchange(&traveler_glyph_trace[n].commands,0,0),
            (long)InterlockedCompareExchange(&traveler_glyph_trace[n].dispatch_result,0,0),
            (long)InterlockedCompareExchange(&traveler_glyph_trace[n].build_result,0,0),
            (long)InterlockedCompareExchange(&traveler_glyph_trace[n].passes,0,0),traveler_glyph_init.failed);
        message(line);
    }
}
#endif
