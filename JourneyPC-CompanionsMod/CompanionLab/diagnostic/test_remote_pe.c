#include "remote_pe.h"
#include <assert.h>
#include <stdio.h>
static unsigned char image[4096];
static int readable = 1;
static int read_image(void *unused, uint32_t rva, void *out, size_t length) {
    (void)unused;
    if (!readable || length > sizeof(image) || rva > sizeof(image) - length) return 0;
    memcpy(out, image + rva, length);
    return 1;
}
static void put32(size_t offset, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) image[offset+i] = (unsigned char)(value >> (i*8));
}
static void fixture(void) {
    memset(image, 0, sizeof(image));
    image[0]='M'; image[1]='Z'; put32(60,128);
    memcpy(image+128,"PE\0\0",4);
    image[132]=0x64; image[133]=0x86; image[134]=1;
    image[148]=240; image[152]=0x0b; image[153]=2;
    put32(208,sizeof(image)); put32(260,1);
    put32(264,512); put32(268,256);
    put32(400,256); put32(404,1024); put32(428,0x20000000);
    put32(528,1); put32(532,1); put32(536,1);
    put32(540,560); put32(544,564); put32(548,568);
    put32(560,1024); put32(564,600); memcpy(image+600,"LoadLibraryW",13);
}
int main(void) {
    struct jc_export_result out;
    fixture(); assert(jc_pe_export(read_image,NULL,sizeof(image),"LoadLibraryW",&out)==JC_PE_FUNCTION && out.rva==1024);
    assert(jc_pe_export(read_image,NULL,sizeof(image),"#1",&out)==JC_PE_FUNCTION);
    assert(jc_pe_export(read_image,NULL,sizeof(image),"Absent",&out)==JC_PE_NOT_FOUND);
    assert(jc_pe_export(read_image,NULL,sizeof(image),"#2",&out)==JC_PE_NOT_FOUND);
    assert(jc_pe_export(read_image,NULL,sizeof(image),"#65536",&out)==JC_PE_INVALID);
    put32(560,640); memcpy(image+640,"KERNELBASE.LoadLibraryW",23);
    assert(jc_pe_export(read_image,NULL,sizeof(image),"LoadLibraryW",&out)==JC_PE_FORWARD && !strcmp(out.forward,"KERNELBASE.LoadLibraryW"));
    memset(image+640,'X',128); assert(jc_pe_export(read_image,NULL,sizeof(image),"LoadLibraryW",&out)==JC_PE_INVALID);
    fixture(); image[568]=1; assert(jc_pe_export(read_image,NULL,sizeof(image),"LoadLibraryW",&out)==JC_PE_INVALID);
    fixture(); put32(428,0); assert(jc_pe_export(read_image,NULL,sizeof(image),"LoadLibraryW",&out)==JC_PE_INVALID);
    fixture(); put32(544,UINT32_MAX); assert(jc_pe_export(read_image,NULL,sizeof(image),"LoadLibraryW",&out)==JC_PE_INVALID);
    fixture(); readable=0; assert(jc_pe_export(read_image,NULL,sizeof(image),"LoadLibraryW",&out)==JC_PE_INVALID);
    puts("Remote PE resolver: 11 assertions passed (synthetic images; not a Wine runtime test).");
    return 0;
}
