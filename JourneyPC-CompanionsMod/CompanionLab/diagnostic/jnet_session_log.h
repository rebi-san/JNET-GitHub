#ifndef JNET_SESSION_LOG_H
#define JNET_SESSION_LOG_H
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>
#define JNET_LOG_ROWS 2048
#define JNET_LOG_BYTES 1024
struct jnet_log_row { _Atomic uint32_t busy,length; uint32_t sequence; char text[JNET_LOG_BYTES]; };
struct jnet_memory_log { _Atomic uint32_t count,dropped; struct jnet_log_row rows[JNET_LOG_ROWS]; };
struct jnet_session_logs { _Atomic uint32_t ready; struct jnet_memory_log runtime,fault; };
_Static_assert(ATOMIC_INT_LOCK_FREE==2,"Session logging requires lock-free publication");
static inline int jnet_log_append(struct jnet_memory_log *log,const char *text,size_t size) {
    if(!log || (!text && size))return 0;
    while(size){
        uint32_t ticket=atomic_load(&log->count);
        do {
            if(ticket==UINT32_MAX){atomic_fetch_add(&log->dropped,1);return 1;}
        }while(!atomic_compare_exchange_weak(&log->count,&ticket,ticket+1));
        size_t n=size<JNET_LOG_BYTES?size:JNET_LOG_BYTES;
        struct jnet_log_row *row=&log->rows[ticket%JNET_LOG_ROWS];
        uint32_t idle=0;
        if(atomic_compare_exchange_strong(&row->busy,&idle,1)){
            if(row->sequence<=ticket){
                if(ticket>=JNET_LOG_ROWS)atomic_fetch_add(&log->dropped,1);
                memcpy(row->text,text,n);row->sequence=ticket+1;
                atomic_store(&row->length,(uint32_t)n);
            }else atomic_fetch_add(&log->dropped,1);
            atomic_store(&row->busy,0);
        }else atomic_fetch_add(&log->dropped,1);
        text+=n;size-=n;
    }
    return 1;
}
/* Never wait on a writer: a fault may have interrupted it permanently. */
static inline size_t jnet_log_read_row(struct jnet_memory_log *log,uint32_t ticket,char *out) {
    struct jnet_log_row *row=&log->rows[ticket%JNET_LOG_ROWS];
    uint32_t idle=0;
    if(!atomic_compare_exchange_strong(&row->busy,&idle,1))return 0;
    size_t n=atomic_load(&row->length);
    if(row->sequence!=ticket+1 || n>JNET_LOG_BYTES)n=0;
    if(n)memcpy(out,row->text,n);
    atomic_store(&row->busy,0);return n;
}
static inline size_t jnet_log_tail(struct jnet_memory_log *log,char *out,size_t cap) {
    if(!cap)return 0;
    out[0]=0;if(!log)return 0;
    uint32_t count=atomic_load(&log->count),start=count>JNET_LOG_ROWS?count-JNET_LOG_ROWS:0;
    size_t used=0;char bytes[JNET_LOG_BYTES];
    for(uint32_t i=count;i>start && used<cap-1;--i){
        size_t n=jnet_log_read_row(log,i-1,bytes);
        size_t take=n<cap-1-used?n:cap-1-used;
        memcpy(out+cap-1-used-take,bytes+n-take,take);used+=take;
    }
    memmove(out,out+cap-1-used,used);
    out[used]=0;return used;
}
#if defined(_WIN32)
static inline struct jnet_session_logs *jnet_session_open(HANDLE process,DWORD pid,HANDLE *mapping) {
    FILETIME created,ended,kernel,user;
    if(!GetProcessTimes(process,&created,&ended,&kernel,&user))return NULL;
    wchar_t name[128];
    swprintf(name,128,L"Local\\JNET-session-v2-%lu-%08lx%08lx",(unsigned long)pid,
             (unsigned long)created.dwHighDateTime,(unsigned long)created.dwLowDateTime);
    HANDLE handle=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(struct jnet_session_logs),name);
    if(!handle)return NULL;
    struct jnet_session_logs *logs=MapViewOfFile(handle,FILE_MAP_ALL_ACCESS,0,0,sizeof(*logs));
    if(!logs){CloseHandle(handle);return NULL;}
    *mapping=handle;return logs;
}
#endif
#endif
