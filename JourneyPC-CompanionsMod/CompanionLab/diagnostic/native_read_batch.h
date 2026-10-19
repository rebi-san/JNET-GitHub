#ifndef JC_NATIVE_READ_BATCH_H
#define JC_NATIVE_READ_BATCH_H
#include "native_layout.h"
#include "native_pose_writes.h"
#define JC_READ_PAGES 8u
#define JC_READ_PAGE 4096u
struct jc_read_page { uint64_t address; unsigned char bytes[JC_READ_PAGE],used[JC_READ_PAGE/8]; };
struct jc_read_batch { struct jc_read_page pages[JC_READ_PAGES]; unsigned next; };
typedef int (*jc_span_consumer)(void *,unsigned,const unsigned char *,size_t);
/* A fresh bounded snapshot per call; failed gap reads fall back to exact spans. */
static inline int jc_read_spans(jc_layout_reader read,void *ctx,struct jc_pose_span *spans,
        unsigned count,jc_span_consumer consume,void *output) {
    unsigned char scratch[4096]; uint64_t end;
    if (count>JC_POSE_SPANS || !consume) return 4;
    for (unsigned i=0;i<count;++i)
        if (!spans[i].size || spans[i].size>sizeof(scratch) ||
            !jc_layout_address(spans[i].address,spans[i].size,&end)) return 4;
    jc_pose_sort(spans,count);
    for (unsigned i=0;i<count;) {
        unsigned last=i+1; uint64_t start=spans[i].address;
        end=start+spans[i].size;
        while (last<count && spans[last].address+spans[last].size-start<=sizeof(scratch) &&
               (spans[last].address<=end || spans[last].address-end<=256)) {
            if (spans[last].address+spans[last].size>end) end=spans[last].address+spans[last].size;
            ++last;
        }
        if (read(ctx,start,scratch,(size_t)(end-start))) {
            for (unsigned j=i;j<last;++j) {
                int code=consume(output,spans[j].owner,scratch+(size_t)(spans[j].address-start),spans[j].size);
                if (code) return code;
            }
        } else for (unsigned j=i;j<last;++j) {
            if (!read(ctx,spans[j].address,scratch,spans[j].size)) return 1;
            int code=consume(output,spans[j].owner,scratch,spans[j].size);
            if (code) return code;
        }
        i=last;
    }
    return 0;
}
static inline void jc_read_batch_reset(struct jc_read_batch *batch) {
    for (unsigned i=0;i<JC_READ_PAGES;++i) batch->pages[i].address=0;
    batch->next=0;
}
static inline int jc_read_batch_get(struct jc_read_batch *batch,jc_layout_reader read,void *ctx,
        uint64_t address,void *out,size_t size) {
    uint64_t end;
    if (!size || !jc_layout_address(address,size,&end)) return 0;
    unsigned offset=(unsigned)(address&(JC_READ_PAGE-1));
    if (size>JC_READ_PAGE-offset) { jc_read_batch_reset(batch); return read(ctx,address,out,size); }
    uint64_t page=address-offset; unsigned slot=0;
    while (slot<JC_READ_PAGES && batch->pages[slot].address!=page) ++slot;
    int missing=slot==JC_READ_PAGES,fresh=missing;
    if (missing) { slot=batch->next; batch->next=(slot+1)%JC_READ_PAGES; }
    struct jc_read_page *p=batch->pages+slot;
    if (!fresh) for (unsigned i=offset;i<offset+size;++i)
        if (p->used[i/8]&(1u<<(i%8))) { fresh=1; break; }
    if (fresh) {
        p->address=0;
        if (!read(ctx,page,p->bytes,JC_READ_PAGE)) return read(ctx,address,out,size);
        if (missing) memset(p->used,0,sizeof(p->used));
        p->address=page;
    }
    memcpy(out,p->bytes+offset,size);
    for (unsigned i=offset;i<offset+size;++i) p->used[i/8]|=(unsigned char)(1u<<(i%8));
    return 1;
}
#endif
