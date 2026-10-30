#ifndef JC_NATIVE_RENDER_RETIREMENT_H
#define JC_NATIVE_RENDER_RETIREMENT_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#define JC_RETIRE_SLOTS 4u
enum { JC_RETIRE_FREE,JC_RETIRE_ARMED,JC_RETIRE_EXECUTING,JC_RETIRE_GPU,JC_RETIRE_DONE,JC_RETIRE_FAILED };
struct jc_retirement_entry { uint64_t ticket,executor,begin,end,thread; unsigned phase; };
struct jc_retirement { struct jc_retirement_entry entries[JC_RETIRE_SLOTS]; uint64_t last_ticket; };
static inline int jc_retirement_arm(struct jc_retirement *r,uint64_t executor,uint64_t begin,uint64_t end,
        uint64_t *ticket) {
    if (!r || !ticket || !executor || !begin || end<=begin || (end-begin)%128 ||
        end-begin>64u*1024u*1024u || r->last_ticket==UINT64_MAX) return 0;
    unsigned free_slot=JC_RETIRE_SLOTS;
    for (unsigned i=0;i<JC_RETIRE_SLOTS;++i) {
        const struct jc_retirement_entry *e=r->entries+i;
        if (!e->phase && free_slot==JC_RETIRE_SLOTS) free_slot=i;
        if ((e->phase==JC_RETIRE_ARMED || e->phase==JC_RETIRE_EXECUTING || e->phase==JC_RETIRE_FAILED) &&
            e->executor==executor && begin<e->end && e->begin<end) return 0;
    }
    if (free_slot==JC_RETIRE_SLOTS) return 0;
    uint64_t next=++r->last_ticket;
    r->entries[free_slot]=(struct jc_retirement_entry){next,executor,begin,end,0,JC_RETIRE_ARMED};
    *ticket=next; return 1;
}
static inline unsigned jc_retirement_enter(struct jc_retirement *r,uint64_t executor,
        uint64_t begin,uint64_t end,uint64_t thread) {
    unsigned mask=0;
    if (!thread || !begin || end<=begin || (end-begin)%128) return 0;
    for (unsigned i=0;i<JC_RETIRE_SLOTS;++i) {
        struct jc_retirement_entry *e=r->entries+i;
        if (e->phase!=JC_RETIRE_ARMED || e->executor!=executor || end<=e->begin || begin>=e->end) continue;
        if (begin>e->begin || end<e->end) { e->phase=JC_RETIRE_FAILED; continue; }
        e->thread=thread; e->phase=JC_RETIRE_EXECUTING; mask|=1u<<i;
    }
    return mask;
}
static inline int jc_retirement_result(const struct jc_retirement *r,uint64_t ticket) {
    if (!ticket) return -1;
    for (unsigned i=0;i<JC_RETIRE_SLOTS;++i) if (r->entries[i].ticket==ticket && r->entries[i].phase) {
        unsigned p=r->entries[i].phase;
        return p==JC_RETIRE_DONE ? 1 : p==JC_RETIRE_FAILED ? -1 : 0;
    }
    return -1;
}

#ifdef _WIN32
#include <windows.h>
#include <d3d11.h>
#endif
#if defined(_WIN32) || defined(JC_RETIREMENT_TEST_PLATFORM)
struct jc_retirement_gpu { ID3D11DeviceContext *context; ID3D11Query *query; unsigned flushed; };
struct jc_live_retirement {
    SRWLOCK lock;
    struct jc_retirement state;
    struct jc_retirement_gpu gpu[JC_RETIRE_SLOTS];
};
/* Device calls occur only on the thread that executed the matching commands.
   An event query is ended AFTER native execution, never after handoff. */
static inline int jc_live_retirement_event(struct jc_retirement_gpu *g,ID3D11DeviceContext *context) {
    if (!context || g->query || g->context ||
        context->lpVtbl->GetType(context)!=D3D11_DEVICE_CONTEXT_IMMEDIATE) return 0;
    ID3D11Device *device=NULL; ID3D11Query *query=NULL;
    context->lpVtbl->GetDevice(context,&device);
    if (!device) return 0;
    D3D11_QUERY_DESC desc={D3D11_QUERY_EVENT,0};
    HRESULT result=device->lpVtbl->CreateQuery(device,&desc,&query);
    device->lpVtbl->Release(device);
    if (FAILED(result) || !query) { if (query) query->lpVtbl->Release(query); return 0; }
    context->lpVtbl->AddRef(context);
    g->context=context; g->query=query;
    context->lpVtbl->End(context,(ID3D11Asynchronous *)query); return 1;
}
static inline uint64_t jc_live_retirement_arm(struct jc_live_retirement *r,uint64_t executor,
        uint64_t begin,uint64_t end) {
    uint64_t ticket=0;
    AcquireSRWLockExclusive(&r->lock);
    jc_retirement_arm(&r->state,executor,begin,end,&ticket);
    ReleaseSRWLockExclusive(&r->lock); return ticket;
}
/* Only when publication is known to have queued nothing. */
static inline int jc_live_retirement_cancel(struct jc_live_retirement *r,uint64_t ticket) {
    int canceled=0;
    AcquireSRWLockExclusive(&r->lock);
    for (unsigned i=0;i<JC_RETIRE_SLOTS;++i) if (ticket && r->state.entries[i].ticket==ticket &&
            r->state.entries[i].phase==JC_RETIRE_ARMED) {
        memset(r->state.entries+i,0,sizeof(r->state.entries[i])); canceled=1; break;
    }
    ReleaseSRWLockExclusive(&r->lock); return canceled;
}
static inline unsigned jc_live_retirement_enter(struct jc_live_retirement *r,uint64_t executor,
        uint64_t begin,uint64_t end) {
    AcquireSRWLockExclusive(&r->lock);
    unsigned mask=jc_retirement_enter(&r->state,executor,begin,end,GetCurrentThreadId());
    ReleaseSRWLockExclusive(&r->lock); return mask;
}
static inline void jc_live_retirement_poll(struct jc_live_retirement *r) {
    AcquireSRWLockExclusive(&r->lock);
    for (unsigned i=0;i<JC_RETIRE_SLOTS;++i) {
        struct jc_retirement_entry *e=r->state.entries+i; struct jc_retirement_gpu *g=r->gpu+i;
        if (e->phase!=JC_RETIRE_GPU || e->thread!=GetCurrentThreadId()) continue;
        BOOL complete=FALSE;
        HRESULT hr=g->context->lpVtbl->GetData(g->context,(ID3D11Asynchronous *)g->query,
            &complete,sizeof(complete),D3D11_ASYNC_GETDATA_DONOTFLUSH);
        if (hr==S_OK && complete) {
            g->query->lpVtbl->Release(g->query); g->context->lpVtbl->Release(g->context);
            memset(g,0,sizeof(*g)); e->phase=JC_RETIRE_DONE;
        } else if (FAILED(hr)) e->phase=JC_RETIRE_FAILED;
    }
    ReleaseSRWLockExclusive(&r->lock);
}
static inline int jc_live_retirement_needs_poll(struct jc_live_retirement *r) {
    int pending=0;
    AcquireSRWLockExclusive(&r->lock);
    for (unsigned i=0;i<JC_RETIRE_SLOTS;++i)
        if (r->state.entries[i].phase==JC_RETIRE_GPU &&
            r->state.entries[i].thread==GetCurrentThreadId()) {pending=1;break;}
    ReleaseSRWLockExclusive(&r->lock);
    return pending;
}
static inline void jc_live_retirement_progress(struct jc_live_retirement *r) {
    AcquireSRWLockExclusive(&r->lock);
    for (unsigned i=0;i<JC_RETIRE_SLOTS;++i) {
        struct jc_retirement_entry *e=r->state.entries+i;
        struct jc_retirement_gpu *g=r->gpu+i;
        if (e->phase!=JC_RETIRE_GPU || e->thread!=GetCurrentThreadId() || g->flushed) continue;
        g->context->lpVtbl->Flush(g->context);
        g->flushed=1;
    }
    ReleaseSRWLockExclusive(&r->lock);
    jc_live_retirement_poll(r);
}
static inline uint64_t jc_live_retirement_pending_executor(struct jc_live_retirement *r) {
    uint64_t executor=0;
    AcquireSRWLockExclusive(&r->lock);
    for (unsigned i=0;i<JC_RETIRE_SLOTS;++i) {
        const struct jc_retirement_entry *e=r->state.entries+i;
        if (e->phase==JC_RETIRE_ARMED || e->phase==JC_RETIRE_EXECUTING || e->phase==JC_RETIRE_GPU) {
            if (executor && executor!=e->executor) { executor=0; break; }
            executor=e->executor;
        }
    }
    ReleaseSRWLockExclusive(&r->lock); return executor;
}
static inline void jc_live_retirement_leave(struct jc_live_retirement *r,unsigned mask,uint32_t native_result,
        ID3D11DeviceContext *context) {
    AcquireSRWLockExclusive(&r->lock);
    for (unsigned i=0;i<JC_RETIRE_SLOTS;++i) if (mask&(1u<<i)) {
        struct jc_retirement_entry *e=r->state.entries+i;
        if (e->phase!=JC_RETIRE_EXECUTING || e->thread!=GetCurrentThreadId() || native_result ||
            !jc_live_retirement_event(r->gpu+i,context)) e->phase=JC_RETIRE_FAILED;
        else e->phase=JC_RETIRE_GPU;
    }
    ReleaseSRWLockExclusive(&r->lock);
    jc_live_retirement_poll(r);
}
/* Read on the producer thread; DONE remains until explicitly acknowledged, so
   a polling client cannot accidentally consume another client's completion. */
static inline int jc_live_retirement_result(struct jc_live_retirement *r,uint64_t ticket,int acknowledge) {
    AcquireSRWLockExclusive(&r->lock);
    int result=jc_retirement_result(&r->state,ticket);
    if (result==1 && acknowledge) for (unsigned i=0;i<JC_RETIRE_SLOTS;++i)
        if (r->state.entries[i].ticket==ticket) memset(r->state.entries+i,0,sizeof(r->state.entries[i]));
    ReleaseSRWLockExclusive(&r->lock); return result;
}
#endif
#endif
