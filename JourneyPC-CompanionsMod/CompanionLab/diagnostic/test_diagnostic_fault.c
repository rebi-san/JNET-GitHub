#include "diagnostic_fault.h"
#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#endif
#include "jnet_session_log.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    uint32_t fields[JC_FAULT_FIELDS];
    char record[JC_FAULT_BYTES+1];
    for (unsigned i=0;i<JC_FAULT_FIELDS;++i) fields[i]=UINT32_MAX;
    memset(record,'!',sizeof(record));
    assert(!jc_fault_record(record,JC_FAULT_BYTES-1,fields));
    assert(record[0]=='!' && record[JC_FAULT_BYTES]=='!');
    assert(jc_fault_record(record,JC_FAULT_BYTES,fields)==JC_FAULT_BYTES);
    assert(!memcmp(record,"[JNET-fault] ffffffff",21));
    assert(record[JC_FAULT_BYTES-2]=='\r' && record[JC_FAULT_BYTES-1]=='\n' && record[JC_FAULT_BYTES]=='!');
    fields[82]=1;fields[83]=0xff;
    fields[JC_FAULT_STACK_FIELD+0x58/4]=0x40284414;
    fields[JC_FAULT_STACK_FIELD+0x58/4+1]=1;
    assert(jc_fault_record(record,JC_FAULT_BYTES,fields)==JC_FAULT_BYTES);
    assert(!memcmp(record+13+(JC_FAULT_STACK_FIELD+0x58/4)*9,"40284414 00000001",17));
    static struct jnet_memory_log log;
    char copied[JC_FAULT_BYTES+1];
    assert(jnet_log_append(&log,record,JC_FAULT_BYTES));
    assert(jnet_log_tail(&log,copied,sizeof(copied))==JC_FAULT_BYTES);
    assert(!memcmp(copied,record,JC_FAULT_BYTES));
    fields[0]=0xc0000005; fields[1]=0;
    assert(jc_fault_record(record,JC_FAULT_BYTES,fields)==JC_FAULT_BYTES);
    assert(!memcmp(record+13,"c0000005 00000000",17));
    assert(jc_fault_interesting(0xc0000005) && jc_fault_interesting(0xc0000409));
    assert(jc_fault_interesting(0x80000003) && !jc_fault_interesting(0xe06d7363));
    assert(jc_fault_rva(0x140001234,0x140000000,0x2000)==0x1234);
    assert(jc_fault_rva(0x13fffffff,0x140000000,0x2000)==UINT32_MAX);
    assert(jc_fault_rva(0x140002000,0x140000000,0x2000)==UINT32_MAX);
    assert(jc_fault_rva(UINT64_MAX,UINT64_MAX-1,2)==1);
    puts("Fault records: exact bounds, full-width codes, relative module offsets and exception filtering passed; no faults raised.");
}
