#include "diagnostic_checkpoint.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
struct sink { unsigned writes,flushes,bytes,fail; };
static int write_record(void *context,const char *line,size_t length) {
    struct sink *sink=context;
    ++sink->writes;
    assert(line && length<=JC_CHECKPOINT_LENGTH);
    if (sink->fail==sink->writes) return 0;
    sink->bytes+=(unsigned)length;
    return 1;
}
static int flush_record(void *context) {
    struct sink *sink=context;
    ++sink->flushes;
    return sink->fail!=3;
}
static void appearance_records(void) {
    char line[512]; unsigned attempts=0; struct sink sink={0};
    int n=snprintf(line,sizeof(line),"[JC] appearance source reason=0 parameter=-1 code=0 found=8191 trial_found=8191 definition_match=8191 missing_source=0 missing_trial=0 definition_difference=4096");
    assert(n==161);
    assert(jc_checkpoint_record(&attempts,line,(size_t)n,&sink,write_record,flush_record)==JC_CHECKPOINT_SIZE);
    assert(!attempts && !sink.writes);
    for (unsigned scenario=0;scenario<3;++scenario) {
        unsigned u=scenario==0 ? 891u : scenario==1 ? 8191u : UINT_MAX;
        int i=scenario==2 ? INT_MIN : -1;
        for (unsigned row=0;row<5;++row) {
            if (row==0) n=snprintf(line,sizeof(line),JC_CP_APPEARANCE_RESULT,i,u,u,i,u,u,u,u);
            else if (row==1) n=snprintf(line,sizeof(line),JC_CP_APPEARANCE_SOURCE,i,i,i,u,u,u);
            else if (row==2) n=snprintf(line,sizeof(line),JC_CP_APPEARANCE_TEXTURES,u,u,u,u);
            else if (row==3) n=snprintf(line,sizeof(line),JC_CP_APPEARANCE_LAYOUT,i,i,u,u,u);
            else n=snprintf(line,sizeof(line),JC_CP_APPEARANCE_WIDTH,i,i,u,u,u);
            assert(n>0 && (unsigned)n<=JC_CHECKPOINT_LENGTH && (size_t)n==strlen(line));
            assert(!jc_checkpoint_record(&attempts,line,(size_t)n,&sink,write_record,flush_record));
        }
    }
    assert(attempts==15 && sink.flushes==15);
    puts("appearance checkpoints: reproduced 161-byte rejection; all five production formats fit for 891/8191 masks and full signed/unsigned extremes");
}
int main(void) {
    appearance_records();
    unsigned attempts=0; struct sink sink={0}; char line[JC_CHECKPOINT_LENGTH+1];
    memset(line,'x',sizeof(line));
    for (unsigned i=0;i<64;++i)
        assert(!jc_checkpoint_record(&attempts,line,160,&sink,write_record,flush_record));
    assert(attempts==64 && sink.flushes==64);
    assert(!jc_checkpoint_record(&attempts,"draw returned",13,&sink,write_record,flush_record));
    assert(!jc_checkpoint_record(&attempts,"postchecks passed",17,&sink,write_record,flush_record));
    assert(attempts==66 && sink.flushes==66);
    while (attempts<JC_CHECKPOINT_LIMIT)
        assert(!jc_checkpoint_record(&attempts,line,160,&sink,write_record,flush_record));
    struct sink before=sink;
    assert(jc_checkpoint_record(&attempts,line,160,&sink,write_record,flush_record)==JC_CHECKPOINT_CAP);
    assert(attempts==JC_CHECKPOINT_LIMIT && !memcmp(&before,&sink,sizeof(sink)));
    assert(sink.bytes<=JC_CHECKPOINT_LIMIT*(JC_CHECKPOINT_LENGTH+2));
    attempts=0; sink=(struct sink){0};
    assert(jc_checkpoint_record(&attempts,line,161,&sink,write_record,flush_record)==JC_CHECKPOINT_SIZE);
    assert(!attempts && !sink.writes && !sink.flushes);
    for (unsigned failure=1;failure<=3;++failure) {
        attempts=0; sink=(struct sink){.fail=failure};
        int expected=failure==1 ? JC_CHECKPOINT_LINE : failure==2 ? JC_CHECKPOINT_NEWLINE : JC_CHECKPOINT_FLUSH;
        assert(jc_checkpoint_record(&attempts,line,160,&sink,write_record,flush_record)==expected);
        assert(attempts==1 && sink.writes==(failure==1 ? 1u : 2u) && sink.flushes==(failure==3));
    }
    puts("checkpoint: 64-record run continues through return/postchecks; 128-record/20,736-byte cap, oversize and each I/O failure passed");
}
