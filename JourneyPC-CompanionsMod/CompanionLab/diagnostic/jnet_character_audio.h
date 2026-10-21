#ifndef JNET_CHARACTER_AUDIO_H
#define JNET_CHARACTER_AUDIO_H
#include <stddef.h>
#include <string.h>
static inline int jnet_character_bank_path(const char *path,char *out,size_t capacity) {
    static const char *const aliases[]={"Data/Sounds/JNTBioSummit-SFX.bnk","Data/Sounds/JNTMechCave-SFX.bnk","Data/Sounds/JNTFlowRuins-SFX.bnk"};
    static const char *const native[]={"Data/Sounds/LvlSfxSummit-SFX.bnk","Data/Sounds/LvlSfxCave-SFX.bnk","Data/Sounds/LvlSfxRuins-SFX.bnk"};
    if(!path || !out || !capacity)return 0;
    size_t length=0;while(length<capacity && path[length])++length;
    if(length==capacity)return 0;
    for(unsigned i=0;i<3;++i){
        size_t suffix=strlen(aliases[i]);
        if(length<suffix)continue;
        size_t prefix=length-suffix;
        if(prefix && path[prefix-1]!='/' && path[prefix-1]!='\\')continue;
        size_t j=0;
        for(;j<suffix;++j){char c=path[prefix+j];if(c=='\\')c='/';if(c!=aliases[i][j])break;}
        if(j!=suffix || prefix+strlen(native[i])>=capacity)continue;
        memcpy(out,path,prefix);strcpy(out+prefix,native[i]);return 1;
    }
    return 0;
}
static inline unsigned jnet_character_chirp_variant(unsigned kind,unsigned sequence,unsigned charge) {
    if(kind==1)return (charge==4?14:12)+(sequence%2);
    if(kind==2){static const unsigned sounds[]={16,17,19,20};return sounds[sequence%4];}
    if(kind==3)return 18;
    if(kind==5 || kind==6)return 21+sequence%2;
    return 0;
}
#endif
