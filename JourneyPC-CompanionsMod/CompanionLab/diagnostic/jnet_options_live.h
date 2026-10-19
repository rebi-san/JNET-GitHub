#ifndef JNET_OPTIONS_LIVE_H
#define JNET_OPTIONS_LIVE_H
static void jnet_options_apply(void) {
    static unsigned windwalls=1,sliding=1,freezing=1;
    unsigned wanted_wind=jnet_windwalls;
    unsigned wanted_slide=jnet_sliding,wanted_freeze=jnet_freezing;
    if(wanted_wind==windwalls && wanted_slide==sliding && wanted_freeze==freezing)return;
    static const unsigned char slide[]={0x0f,0xb6,0x82,0,1,0,0},slide_off[]={0xb8,0,0,0,0,0x90,0x90};
    static const unsigned char cold[]={0xf3,0x0f,0x11,0x43,0x14,0xf3,0x0f,0x11,0x4b,0x18};
    static const unsigned char nop10[]={0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
    static const unsigned char wind[]={0x40,0x55},wind_off[]={0xc3,0x90};
    unsigned char before[10];
    if(!own_read(NULL,fault_image+0x15b240,before,2) || (memcmp(before,wind,2) && memcmp(before,wind_off,2)) ||
       !own_read(NULL,fault_image+0xaa239,before,7) || (memcmp(before,slide,7) && memcmp(before,slide_off,7)) ||
       !own_read(NULL,fault_image+0x184920,before,10) || (memcmp(before,cold,10) && memcmp(before,nop10,10)))goto failed;
    const uintptr_t sites[]={fault_image+0xaa239,fault_image+0x184920,fault_image+0x15b240};
    const unsigned char *normal[]={slide,cold,wind},*disabled[]={slide_off,nop10,wind_off};
    const size_t sizes[]={7,10,2};
    const unsigned desired[]={wanted_slide,wanted_freeze,wanted_wind},previous[]={sliding,freezing,windwalls};
    for(unsigned i=0;i<3;++i)if(!jnet_mepal_site(sites[i],normal[i],disabled[i],sizes[i],!desired[i])){
        int restored=1;
        for(unsigned j=0;j<=i;++j)restored=jnet_mepal_site(sites[j],normal[j],disabled[j],sizes[j],!previous[j]) && restored;
        if(!restored)message("[JNET] Options rollback failed; restart Journey");
        goto failed;
    }
    windwalls=wanted_wind;sliding=wanted_slide;freezing=wanted_freeze;jnet_options_failed=0;return;
failed:
    jnet_windwalls=windwalls;jnet_sliding=sliding;jnet_freezing=freezing;jnet_options_failed=1;
    message("[JNET] Options patch refused: native bytes or protection unavailable");
}
#endif
