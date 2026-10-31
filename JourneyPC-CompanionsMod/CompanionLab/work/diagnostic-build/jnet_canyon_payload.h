struct jnet_collision_resource {
const wchar_t *filename; const char *name; int level;
uint32_t bytes,nodes,triangles,instances; unsigned char hash[32];
};
static const struct jnet_collision_resource jnet_collision_resources[]={
{L"JNET-canyon-collision.bin","Canyon",3,20151016u,131071u,398918u,84u,{90,198,36,238,131,158,52,164,88,235,141,167,55,225,44,164,121,117,64,241,14,132,86,33,72,55,119,200,11,226,81,8}},
{L"JNET-barrens-collision.bin","Barrens",1,826464u,4865u,16769u,6u,{137,103,233,129,42,2,197,18,167,150,222,84,157,41,252,175,165,77,121,188,198,3,231,212,58,217,44,140,221,168,20,161}},
{L"JNET-desert-collision.bin","Desert",2,1024936u,8191u,19070u,9u,{241,58,150,151,114,33,42,135,240,100,141,52,138,25,96,185,187,139,137,87,53,165,92,104,28,85,21,206,189,80,145,88}},
{L"JNET-cave-collision.bin","Cave",4,1754232u,11679u,34512u,11u,{184,152,173,229,157,184,189,169,33,127,39,247,114,125,214,36,88,62,245,224,190,40,188,68,160,68,7,235,247,51,170,179}},
{L"JNET-mountain-collision.bin","Mountain",6,1459816u,8191u,29942u,10u,{214,39,249,81,50,232,77,246,36,59,64,131,29,88,38,16,128,246,150,151,131,167,51,37,27,140,3,118,63,224,234,177}},
};
