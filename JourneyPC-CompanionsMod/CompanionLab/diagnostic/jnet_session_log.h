#ifndef JNET_SESSION_LOG_H
#define JNET_SESSION_LOG_H
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>
#define JNET_LOG_ROWS 2048
#define JNET_LOG_BYTES 1024
struct jnet_log_row { _Atomic uint32_t length; char text[JNET_LOG_BYTES]; };
struct jnet_memory_log { _Atomic uint32_t count,dropped; struct jnet_log_row rows[JNET_LOG_ROWS]; };
struct jnet_session_logs { _Atomic uint32_t ready; struct jnet_memory_log runtime,fault; };
_Static_assert(ATOMIC_INT_LOCK_FREE==2,"Session logging requires lock-free publication");
static inline int jnet_log_append(struct jnet_memory_log *log,const char *text,size_t size) {
    if(!log || (!text && size))return 0;
    while(size){
        uint32_t slot=atomic_load(&log->count);
        do {
            if(slot>=JNET_LOG_ROWS){atomic_fetch_add(&log->dropped,1);return 1;}
        }while(!atomic_compare_exchange_weak(&log->count,&slot,slot+1));
        size_t n=size<JNET_LOG_BYTES?size:JNET_LOG_BYTES;
        memcpy(log->rows[slot].text,text,n);
        atomic_store_explicit(&log->rows[slot].length,(uint32_t)n,memory_order_release);
        text+=n;size-=n;
    }
    return 1;
}
static inline size_t jnet_log_tail(const struct jnet_memory_log *log,char *out,size_t cap) {
    if(!cap)return 0;
    out[0]=0;if(!log)return 0;
    uint32_t count=atomic_load(&log->count);if(count>JNET_LOG_ROWS)count=JNET_LOG_ROWS;
    size_t used=0;
    for(uint32_t i=count;i>0 && used<cap-1;--i){
        const struct jnet_log_row *row=&log->rows[i-1];
        size_t n=atomic_load_explicit(&row->length,memory_order_acquire);
        if(n>JNET_LOG_BYTES)continue;
        size_t take=n<cap-1-used?n:cap-1-used;
        memmove(out+take,out,used);memcpy(out,row->text+n-take,take);used+=take;
    }
    out[used]=0;return used;
}
#if defined(_WIN32)
static inline struct jnet_session_logs *jnet_session_open(HANDLE process,DWORD pid,HANDLE *mapping) {
    FILETIME created,ended,kernel,user;
    if(!GetProcessTimes(process,&created,&ended,&kernel,&user))return NULL;
    wchar_t name[128];
    swprintf(name,128,L"Local\\JNET-session-v1-%lu-%08lx%08lx",(unsigned long)pid,
             (unsigned long)created.dwHighDateTime,(unsigned long)created.dwLowDateTime);
    HANDLE handle=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(struct jnet_session_logs),name);
    if(!handle)return NULL;
    struct jnet_session_logs *logs=MapViewOfFile(handle,FILE_MAP_ALL_ACCESS,0,0,sizeof(*logs));
    if(!logs){CloseHandle(handle);return NULL;}
    *mapping=handle;return logs;
}
#endif
#endif
