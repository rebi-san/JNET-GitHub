#ifndef JC_TRAVELER_SAND_GUARD_H
#define JC_TRAVELER_SAND_GUARD_H
#include <stdatomic.h>
static void *original_sand_inline __attribute__((used));
static void *sand_inline_resume __attribute__((used));
static atomic_uint traveler_sand_inline_dropped __attribute__((used));
#ifdef __APPLE__
#define JC_SAND_SYMBOL(name) "_" #name
#else
#define JC_SAND_SYMBOL(name) #name
#endif
__attribute__((naked,used)) static void traveler_sand_inline(void) {
    __asm__("cmpb %r15b,0x81(%rdi)\n"
        "movl $0x2e40,%edx\n"
        "cmovneq %r11,%rdx\n"
        "cmpl $3,0xb318(%rbx,%rdx)\n"
        "ja 1f\n"
        "jmp *" JC_SAND_SYMBOL(original_sand_inline) "(%rip)\n"
        "1: lock incl " JC_SAND_SYMBOL(traveler_sand_inline_dropped) "(%rip)\n"
        "jmp *" JC_SAND_SYMBOL(sand_inline_resume) "(%rip)\n");
}
#undef JC_SAND_SYMBOL
typedef void (JC_SAND_ABI *jc_sand_append_fn)(uintptr_t,unsigned,const void *,const void *,
    float,float,float,float,float,float);
static jc_sand_append_fn original_sand_append;
static atomic_uint traveler_sand_dropped,traveler_sand_unreadable;
static void JC_SAND_ABI traveler_sand_append(uintptr_t renderer,unsigned kind,
        const void *position,const void *effect,float a,float b,float c,float d,float e,float f) {
    if (kind==8 || kind==9) {
        uint32_t count=0;
        int read=own_read(NULL,renderer+0xb318+(uint64_t)kind*0x5c8,&count,4);
        if (!read || count>=6) {
            unsigned dropped=atomic_fetch_add(&traveler_sand_dropped,1)+1;
            unsigned unreadable=atomic_fetch_add(&traveler_sand_unreadable,!read)+!read;
            if (dropped<=4 || dropped%300==0) {
                char line[192];
                snprintf(line,sizeof(line),"[JC] sand capacity kind=%u count=%u dropped=%u unreadable=%u",
                    kind,count,dropped,unreadable);message(line);
            }
            return;
        }
    }
    original_sand_append(renderer,kind,position,effect,a,b,c,d,e,f);
}
typedef uintptr_t (JC_SAND_ABI *jc_sand_consume_fn)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
static jc_sand_consume_fn original_sand_consume;
static atomic_uint traveler_sand_trimmed;
static uintptr_t JC_SAND_ABI traveler_sand_consume(uintptr_t renderer,uintptr_t a,uintptr_t b,
        uintptr_t c,uintptr_t d) {
    static unsigned reported_inline;
    unsigned dropped=atomic_load(&traveler_sand_inline_dropped);
    if(dropped && (!reported_inline || dropped-reported_inline>=300)) {
        char line[144];snprintf(line,sizeof(line),"[JC] sand inline producer bounded before write; dropped loops=%u",dropped);
        message(line);reported_inline=dropped;
    }
    for(unsigned kind=8;kind<=9;++kind) {
        uint64_t address=renderer+0xb318+(uint64_t)kind*0x5c8;
        uint32_t count=0,limit=6;
        if(!own_read(NULL,address,&count,4))return 0;
        if(count>limit) {
            if(!jc_local_windows_write(address,&limit,4))return 0;
            unsigned trimmed=atomic_fetch_add(&traveler_sand_trimmed,1)+1;
            if(trimmed<=4 || trimmed%300==0) {
                char line[160];snprintf(line,sizeof(line),"[JC] sand consumer bounded kind=%u count=%u batches=%u",kind,count,trimmed);
                message(line);
            }
        }
    }
    return original_sand_consume(renderer,a,b,c,d);
}
#endif
