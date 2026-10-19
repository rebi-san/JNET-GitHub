#ifndef JNET_CHARACTER_ASSETS_H
#define JNET_CHARACTER_ASSETS_H
#include <stddef.h>
#include <string.h>
static inline int jnet_character_asset_path(const char *path,char *out,size_t capacity) {
    if(!path || !out || !capacity)return 0;
    size_t length=0;
    while(length<capacity && path[length])++length;
    if(length==capacity)return 0;
    for(size_t i=0;i<sizeof(jnet_character_asset_paths)/sizeof(jnet_character_asset_paths[0]);++i){
        const struct jnet_character_asset_alias *row=&jnet_character_asset_paths[i];
        size_t suffix=strlen(row->alias),replacement=strlen(row->native);
        if(length<suffix || strcmp(path+length-suffix,row->alias))continue;
        size_t prefix=length-suffix;
        if(prefix+replacement>=capacity)return 0;
        memcpy(out,path,prefix);
        memcpy(out+prefix,row->native,replacement+1);
        return 1;
    }
    return 0;
}
#endif
