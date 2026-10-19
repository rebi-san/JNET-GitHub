#ifndef JC_NATIVE_LOCAL_READ_H
#define JC_NATIVE_LOCAL_READ_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
struct jc_local_region { uint64_t begin,end; };
struct jc_local_read_cache { struct jc_local_region regions[16]; unsigned used,next; uint64_t epoch; };
typedef int (*jc_local_query)(void *,uint64_t,struct jc_local_region *);
typedef int (*jc_local_copy)(void *,uint64_t,void *,size_t);
/* Cache permissions for one inspection only, never the inspected bytes. A fault
   protected copy is required because another thread can change the mapping. */
static inline int jc_local_read(struct jc_local_read_cache *cache,uint64_t epoch,
    jc_local_query query,jc_local_copy copy,void *ctx,uint64_t address,void *out,size_t bytes) {
    if (!out || !bytes || address<65536 || address>UINT64_C(0x7fffffffffff) ||
        bytes>UINT64_C(0x800000000000)-address) return 0;
    if (cache->epoch!=epoch) { memset(cache,0,sizeof(*cache)); cache->epoch=epoch; }
    uint64_t end=address+bytes,cursor=address;
    while (cursor<end) {
        uint64_t stop=0;
        for (unsigned i=0;i<cache->used;++i) if (cursor>=cache->regions[i].begin && cursor<cache->regions[i].end) {
            stop=cache->regions[i].end; break;
        }
        if (!stop) {
            struct jc_local_region r;
            if (!query(ctx,cursor,&r) || r.begin>cursor || r.end<=cursor || r.end>UINT64_C(0x800000000000)) return 0;
            unsigned slot=cache->used<16 ? cache->used++ : cache->next++%16;
            cache->regions[slot]=r; stop=r.end;
        }
        cursor=stop<end ? stop : end;
    }
    if (!copy(ctx,address,out,bytes)) { memset(cache,0,sizeof(*cache)); return 0; }
    return 1;
}
#ifdef _WIN32
#include <windows.h>
static int jc_local_windows_query(void *ctx,uint64_t address,struct jc_local_region *out) {
    (void)ctx; MEMORY_BASIC_INFORMATION info;
    if (VirtualQuery((void *)(uintptr_t)address,&info,sizeof(info))!=sizeof(info) ||
        info.State!=MEM_COMMIT || (info.Protect&(PAGE_GUARD|PAGE_NOACCESS)) ||
        !(info.Protect&(PAGE_READONLY|PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))) return 0;
    uint64_t start=(uintptr_t)info.BaseAddress;
    if (!info.RegionSize || start>UINT64_C(0x800000000000) || info.RegionSize>UINT64_C(0x800000000000)-start) return 0;
    *out=(struct jc_local_region){start,start+info.RegionSize}; return 1;
}
/* Recovery is limited to this leaf's load and store. No stack or
   nonvolatile registers change, so recovery needs no compiler SEH unwinding. */
extern const unsigned char jc_local_copy_load[],jc_local_copy_store[],jc_local_copy_failed[];
__attribute__((naked,noinline,used)) static int jc_local_copy_leaf(void *ctx,uint64_t address,void *out,size_t bytes) {
    __asm__ volatile(
        "testq %r9, %r9\n\t"
        "jz jc_local_copy_failed\n\t"
        ".globl jc_local_copy_load\n"
        "jc_local_copy_load:\n\t"
        "movb (%rdx), %al\n\t"
        ".globl jc_local_copy_store\n"
        "jc_local_copy_store:\n\t"
        "movb %al, (%r8)\n\t"
        "incq %rdx\n\t"
        "incq %r8\n\t"
        "decq %r9\n\t"
        "jnz jc_local_copy_load\n\t"
        "movl $1, %eax\n\t"
        "retq\n\t"
        ".globl jc_local_copy_failed\n"
        "jc_local_copy_failed:\n\t"
        "xorl %eax, %eax\n\t"
        "retq");
}
static LONG CALLBACK jc_local_read_exception(EXCEPTION_POINTERS *info) {
    EXCEPTION_RECORD *record=info->ExceptionRecord; CONTEXT *cpu=info->ContextRecord;
    if ((record->ExceptionCode==EXCEPTION_ACCESS_VIOLATION || record->ExceptionCode==EXCEPTION_IN_PAGE_ERROR ||
         record->ExceptionCode==EXCEPTION_GUARD_PAGE) && record->NumberParameters>=2 && record->ExceptionInformation[0]==1 &&
        cpu->Rip==(uintptr_t)jc_local_copy_store && record->ExceptionAddress==(void *)jc_local_copy_store &&
        cpu->R8>=65536 && cpu->R8<UINT64_C(0x800000000000) && cpu->R9 &&
        cpu->R9<=UINT64_C(0x800000000000)-cpu->R8 && record->ExceptionInformation[1]==cpu->R8) {
        cpu->Rip=(uintptr_t)jc_local_copy_failed;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if ((record->ExceptionCode==EXCEPTION_ACCESS_VIOLATION || record->ExceptionCode==EXCEPTION_IN_PAGE_ERROR ||
         record->ExceptionCode==EXCEPTION_GUARD_PAGE) && record->NumberParameters>=2 && record->ExceptionInformation[0]==0 &&
        cpu->Rip==(uintptr_t)jc_local_copy_load && record->ExceptionAddress==(void *)jc_local_copy_load &&
        cpu->Rdx>=65536 && cpu->Rdx<UINT64_C(0x800000000000) && cpu->R9 &&
        cpu->R9<=UINT64_C(0x800000000000)-cpu->Rdx && record->ExceptionInformation[1]==cpu->Rdx) {
        cpu->Rip=(uintptr_t)jc_local_copy_failed;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
static INIT_ONCE jc_local_reader_once=INIT_ONCE_STATIC_INIT;
static PVOID jc_local_reader_handler;
static BOOL CALLBACK jc_local_reader_initialize(PINIT_ONCE once,PVOID parameter,PVOID *context) {
    (void)once; (void)parameter; (void)context;
    jc_local_reader_handler=AddVectoredExceptionHandler(1,jc_local_read_exception);
    return jc_local_reader_handler!=NULL;
}
static int jc_local_windows_copy(void *ctx,uint64_t address,void *out,size_t bytes) {
    if (!InitOnceExecuteOnce(&jc_local_reader_once,jc_local_reader_initialize,NULL,NULL)) return 0;
    return jc_local_copy_leaf(ctx,address,out,bytes);
}
static inline int jc_local_windows_write(uint64_t address,const void *value,size_t bytes) {
    if (!value || !bytes || address<65536 || address>=UINT64_C(0x800000000000) ||
        bytes>UINT64_C(0x800000000000)-address) return 0;
    uint64_t cursor=address,end=address+bytes;
    while (cursor<end) {
        MEMORY_BASIC_INFORMATION info;
        if (VirtualQuery((void *)(uintptr_t)cursor,&info,sizeof(info))!=sizeof(info) ||
            info.State!=MEM_COMMIT || (info.Protect&(PAGE_GUARD|PAGE_NOACCESS)) ||
            !(info.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))) return 0;
        uint64_t start=(uintptr_t)info.BaseAddress;
        if (start>cursor || !info.RegionSize || info.RegionSize>UINT64_C(0x800000000000)-start ||
            start+info.RegionSize<=cursor) return 0;
        cursor=start+info.RegionSize;
    }
    return jc_local_windows_copy(NULL,(uintptr_t)value,(void *)(uintptr_t)address,bytes);
}
/* Standalone fixture only: deliberate faults must not gate game startup. */
static inline int jc_local_windows_test(void) {
    SYSTEM_INFO system; GetSystemInfo(&system); size_t page=system.dwPageSize;
    unsigned char *memory=VirtualAlloc(NULL,page*3,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    if (!memory) return 1;
    unsigned char out[32]; struct jc_local_read_cache cache={0}; DWORD previous; int code=2;
    CONTEXT cpu={0}; EXCEPTION_RECORD record={0}; EXCEPTION_POINTERS fault={&record,&cpu};
    cpu.Rip=(uintptr_t)jc_local_copy_load; cpu.Rdx=(uintptr_t)memory; cpu.R9=16;
    record.ExceptionCode=EXCEPTION_ACCESS_VIOLATION; record.NumberParameters=2;
    record.ExceptionAddress=(void *)jc_local_copy_load; record.ExceptionInformation[1]=(uintptr_t)memory;
    if (jc_local_read_exception(&fault)!=EXCEPTION_CONTINUE_EXECUTION || cpu.Rip!=(uintptr_t)jc_local_copy_failed) goto done;
    cpu.Rip=(uintptr_t)jc_local_copy_load; record.ExceptionInformation[0]=1;
    if (jc_local_read_exception(&fault)!=EXCEPTION_CONTINUE_SEARCH || cpu.Rip!=(uintptr_t)jc_local_copy_load) goto done;
    record.ExceptionInformation[0]=0; ++record.ExceptionInformation[1];
    if (jc_local_read_exception(&fault)!=EXCEPTION_CONTINUE_SEARCH) goto done;
    --record.ExceptionInformation[1]; ++cpu.Rip;
    if (jc_local_read_exception(&fault)!=EXCEPTION_CONTINUE_SEARCH) goto done;
    --cpu.Rip; cpu.R9=0;
    if (jc_local_read_exception(&fault)!=EXCEPTION_CONTINUE_SEARCH) goto done;

    memset(memory,17,page*3);
    memset(out,29,sizeof(out));
    if (!jc_local_windows_write((uintptr_t)memory+page-16,out,sizeof(out)) ||
        memory[page-16]!=29 || memory[page+15]!=29 ||
        jc_local_windows_write(0,out,sizeof(out)) ||
        jc_local_windows_write(UINT64_C(0x7fffffffffff),out,sizeof(out))) goto done;
    if (!VirtualProtect(memory+page,page,PAGE_READONLY,&previous)) goto done;
    memset(out,31,sizeof(out));
    if (jc_local_windows_write((uintptr_t)memory+page-16,out,sizeof(out)) || memory[page-16]!=29) goto done;
    if (jc_local_windows_copy(NULL,(uintptr_t)out,memory+page,sizeof(out))) goto done;
    if (!VirtualProtect(memory+page,page,PAGE_READWRITE,&previous)) goto done;
    memset(memory,17,page*3);
    if (!jc_local_read(&cache,1,jc_local_windows_query,jc_local_windows_copy,NULL,(uintptr_t)memory,out,sizeof(out)) || out[0]!=17) goto done;
    memory[0]=23;
    if (!jc_local_read(&cache,1,jc_local_windows_query,jc_local_windows_copy,NULL,(uintptr_t)memory,out,sizeof(out)) || out[0]!=23) goto done;
    if (!jc_local_read(&cache,1,jc_local_windows_query,jc_local_windows_copy,NULL,(uintptr_t)memory+page-16,out,sizeof(out))) goto done;
    code=3;
    if (!VirtualProtect(memory+page,page,PAGE_NOACCESS,&previous)) goto done;
    /* The positive permission cache is deliberately stale; the leaf handler must catch this read. */
    if (jc_local_read(&cache,1,jc_local_windows_query,jc_local_windows_copy,NULL,(uintptr_t)memory+page,out,sizeof(out))) goto done;
    if (jc_local_read(&cache,2,jc_local_windows_query,jc_local_windows_copy,NULL,(uintptr_t)memory+page-16,out,sizeof(out))) goto done;
    code=4;
    if (!VirtualProtect(memory+page*2,page,PAGE_READWRITE|PAGE_GUARD,&previous)) goto done;
    if (jc_local_windows_write((uintptr_t)memory+page*2,out,sizeof(out))) goto done;
    if (jc_local_read(&cache,3,jc_local_windows_query,jc_local_windows_copy,NULL,(uintptr_t)memory+page*2,out,sizeof(out))) goto done;
    MEMORY_BASIC_INFORMATION info;
    if (!VirtualQuery(memory+page*2,&info,sizeof(info)) || !(info.Protect&PAGE_GUARD)) goto done;
    code=0;
done:
    VirtualFree(memory,0,MEM_RELEASE);
    /* This test runs alone in a disposable process; its DLL is unloaded next. */
    if (jc_local_reader_handler) {
        if (!RemoveVectoredExceptionHandler(jc_local_reader_handler)) return 4;
        jc_local_reader_handler=NULL; jc_local_reader_once=(INIT_ONCE)INIT_ONCE_STATIC_INIT;
    }
    return code;
}
#endif
#endif
