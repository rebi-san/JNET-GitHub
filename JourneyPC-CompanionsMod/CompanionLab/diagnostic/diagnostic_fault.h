#ifndef JC_DIAGNOSTIC_FAULT_H
#define JC_DIAGNOSTIC_FAULT_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
enum { JC_FLIGHT_FIELDS=12, JC_FAULT_STACK_BYTES=512, JC_FAULT_STACK_FIELD=84, JC_FAULT_FIELDS=JC_FAULT_STACK_FIELD+JC_FAULT_STACK_BYTES/4, JC_FAULT_BYTES=JC_FAULT_FIELDS*9+14 };
static inline int jc_fault_interesting(uint32_t code) {
    return code==0x80000003u || code==0xc0000005u || code==0xc0000006u || code==0xc000001du ||
        code==0xc0000094u || code==0xc00000fdu || code==0xc0000409u || code==0xc0000374u;
}
static inline uint32_t jc_fault_rva(uint64_t address,uint64_t base,uint32_t size) {
    return address>=base && address-base<size ? (uint32_t)(address-base) : UINT32_MAX;
}
static inline size_t jc_fault_record(char *out,size_t cap,const uint32_t fields[JC_FAULT_FIELDS]) {
    static const char hex[]="0123456789abcdef";
    if (cap<JC_FAULT_BYTES) return 0;
    memcpy(out,"[JNET-fault]",12); size_t n=12;
    for (unsigned i=0;i<JC_FAULT_FIELDS;++i) {
        out[n++]=' ';
        for (int shift=28;shift>=0;shift-=4) out[n++]=hex[(fields[i]>>shift)&15];
    }
    out[n++]='\r'; out[n++]='\n'; return n;
}
#endif
