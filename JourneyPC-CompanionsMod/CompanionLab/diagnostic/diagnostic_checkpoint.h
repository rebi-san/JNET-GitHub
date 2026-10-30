#ifndef JC_DIAGNOSTIC_CHECKPOINT_H
#define JC_DIAGNOSTIC_CHECKPOINT_H
#include <stddef.h>
#define JC_CHECKPOINT_LIMIT 128u
#define JC_CHECKPOINT_LENGTH 160u
#define JC_CP_APPEARANCE_RESULT "[JNET] appearance rc=%d stage=%u calls=%u orig=%d check=%u src=%u before=%u after=%u"
#define JC_CP_APPEARANCE_SOURCE "[JNET] appearance source reason=%d param=%d rc=%d found=%u trial=%u match=%u"
#define JC_CP_APPEARANCE_TEXTURES "[JNET] appearance textures missing_src=%u missing_trial=%u different=%u refreshed=%u"
#define JC_CP_APPEARANCE_LAYOUT "[JNET] appearance layout field=%d param=%d variant=%u count=%u bytes=%u"
#define JC_CP_APPEARANCE_WIDTH "[JNET] appearance descriptor table=%d row=%d offset=%u declared=%u needed=%u"
enum { JC_CHECKPOINT_OK=0, JC_CHECKPOINT_BUSY=1, JC_CHECKPOINT_HANDLE=2,
       JC_CHECKPOINT_CAP=3, JC_CHECKPOINT_SIZE=4, JC_CHECKPOINT_LINE=5,
       JC_CHECKPOINT_NEWLINE=6, JC_CHECKPOINT_FLUSH=7 };
typedef int (*jc_checkpoint_write)(void *,const char *,size_t);
typedef int (*jc_checkpoint_flush)(void *);
static inline int jc_checkpoint_record(unsigned *attempts,const char *line,size_t length,
    void *context,jc_checkpoint_write write,jc_checkpoint_flush flush) {
    if (*attempts>=JC_CHECKPOINT_LIMIT) return JC_CHECKPOINT_CAP;
    if (length>JC_CHECKPOINT_LENGTH) return JC_CHECKPOINT_SIZE;
    ++*attempts;
    if (!write(context,line,length)) return JC_CHECKPOINT_LINE;
    if (!write(context,"\r\n",2)) return JC_CHECKPOINT_NEWLINE;
    if (!flush(context)) return JC_CHECKPOINT_FLUSH;
    return JC_CHECKPOINT_OK;
}
#endif
