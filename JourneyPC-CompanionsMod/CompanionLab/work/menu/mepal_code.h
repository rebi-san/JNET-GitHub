static int jnet_mepal_site(uintptr_t at,const unsigned char *normal,const unsigned char *enabled,size_t bytes,int on) {
    unsigned char current[16];DWORD previous,ignored;
    if(bytes>sizeof(current) || !own_read(NULL,at,current,bytes) ||
        (memcmp(current,normal,bytes) && memcmp(current,enabled,bytes)))return 0;
    const unsigned char *desired=on?enabled:normal;
    if(!memcmp(current,desired,bytes))return 1;
    if(!VirtualProtect((void *)at,bytes,PAGE_EXECUTE_READWRITE,&previous))return 0;
    memcpy((void *)at,desired,bytes);
    int ok=FlushInstructionCache(GetCurrentProcess(),(void *)at,bytes)!=0;
    ok=VirtualProtect((void *)at,bytes,previous,&ignored)!=0 && ok;
    return ok;
}
static int jnet_mepal_prepare(unsigned level) {
    static const unsigned char a[]={0x66,0xc7,0x41,8,1,1},b[]={0x66,0xc7,0x41,8,0,0};
    static const unsigned char c[]={0x66,0xc7,0x42,9,0,1},d[]={0x66,0xc7,0x42,9,0,0};
    static const unsigned char e[]={0x0f,0x92,0xc0,0x88,0x43,9,0x48,0x8b,0x5c,0x24,0x30},f[]={0x31,0xc0,0x88,0x43,9,0x90,0x48,0x8b,0x5c,0x24,0x30};
    static const unsigned char g[]={0xc6,0x43,0x0a,1},h[]={0xc6,0x43,0x0a,0};
    const uintptr_t sites[]={fault_image+0x187f67,fault_image+0x170e22,fault_image+0x170e9c,fault_image+0x17dab7};
    const unsigned char *normal[]={a,c,g,e},*enabled[]={b,d,h,f};
    const size_t sizes[]={sizeof(a),sizeof(c),sizeof(g),sizeof(e)};
    unsigned char before[4][16];int old[4];
    const unsigned meet=1,play=jnet_mepal_play;
    if(level>=12)return 0;
    for(unsigned i=0;i<4;++i){
        if(!own_read(NULL,sites[i],before[i],sizes[i]))return 0;
        old[i]=!memcmp(before[i],enabled[i],sizes[i]);
        if(!old[i] && memcmp(before[i],normal[i],sizes[i]))return 0;
    }
    for(unsigned i=0;i<4;++i)if(!jnet_mepal_site(sites[i],normal[i],enabled[i],sizes[i],i<3?meet:play)){
        int restored=1;
        for(unsigned j=0;j<=i;++j)restored=jnet_mepal_site(sites[j],normal[j],enabled[j],sizes[j],old[j]) && restored;
        if(!restored)message("[JC] MEPAL patch rollback failed; restart Journey before further travel");
        return 0;
    }
    return 1;
}