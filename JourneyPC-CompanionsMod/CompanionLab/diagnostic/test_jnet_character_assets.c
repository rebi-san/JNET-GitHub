#include <assert.h>
#include <stdio.h>
struct jnet_character_asset_alias { const char *alias,*native; };
static const struct jnet_character_asset_alias jnet_character_asset_paths[]={
    {"/Data/Textures/Bin/JNETCharacter_WhiteWhite.dds.D3D11x64",
     "/Data/Textures/Bin/WhiteWhite.dds.D3D11x64"}
};
#include "jnet_character_assets.h"
int main(void) {
    char out[256];
    const char *path="C:/Journey/Data/Textures/Bin/JNETCharacter_WhiteWhite.dds.D3D11x64";
    assert(jnet_character_asset_path(path,out,sizeof(out)));
    assert(!strcmp(out,"C:/Journey/Data/Textures/Bin/WhiteWhite.dds.D3D11x64"));
    assert(!jnet_character_asset_path(out,out,sizeof(out)));
    assert(!jnet_character_asset_path("C:/Journey/Data/Textures/Bin/JNETCharacter_Unknown.dds.D3D11x64",out,sizeof(out)));
    assert(!jnet_character_asset_path("C:/Journey/Data/Textures/Bin/JNETCharacter_WhiteWhite.dds.D3D11x64.extra",out,sizeof(out)));
    assert(!jnet_character_asset_path(path,out,8));
    assert(!jnet_character_asset_path(NULL,out,sizeof(out)));
    puts("Character texture paths: exact aliases mapped, native/unrelated paths unchanged.");
    return 0;
}
