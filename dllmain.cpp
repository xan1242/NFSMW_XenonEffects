// NFS Most Wanted - Xenon Effects implementation
// A port of the XenonEffect particle effect emitters from NFS Carbon
// by Xan/Tenjoin

// BUG LIST:
// - particles stay in the world after restart - MAKE A XENON EFFECT RESET
// - contrails get overwritten by sparks 
// - Reconfigurable limits
// - hookbacks for call hooks
// - reconfigurable texture
// - loading via TextureInfo?
// - catch FPS dynamically for the rate limiters!
// - particle collision & bouncing
//

#include "stdafx.h"
#include "stdio.h"
#include "includes\injector\injector.hpp"
#include "includes\mINI\src\mini\ini.h"
#include <d3d9.h>
#include <cmath>
#pragma comment(lib, "d3d9.lib")

#include <d3dx9.h>
#pragma comment(lib, "d3dx9.lib")
#pragma runtime_checks( "", off )

//#define CONTRAIL_TEST

bool bDebugTexture = false;
bool bContrails = true;
bool bLimitContrailRate = true;
bool bLimitSparkRate = true;
bool bNISContrails = false;
bool bUseCGStyle = false;
bool bPassShadowMap = false;
float ContrailTargetFPS = 60.0f;
float SparkTargetFPS = 60.0f;
float ContrailSpeed = 44.0f;
float ContrailMinIntensity = 0.1f;
float ContrailMaxIntensity = 0.75f;

uint32_t ContrailFrameDelay = 1;
uint32_t SparkFrameDelay = 1;

#define GLOBAL_D3DDEVICE 0x00982BDC
#define GAMEFLOWSTATUS_ADDR 0x00925E90
#define FASTMEM_ADDR 0x00925B30
#define NISINSTANCE_ADDR 0x009885C8
#define WORLDPRELITSHADER_OBJ_ADDR 0x0093DEBC
#define CURRENTSHADER_OBJ_ADDR 0x00982C80

#define FRAMECOUNTER_ADDR 0x00982B78
#define eFrameCounter *(uint32_t*)FRAMECOUNTER_ADDR

#define MAX_PARTICLES 10000
#define NGEFFECT_LIST_COUNT 500

#define SIZEOF_NGPARTICLE 0x48
#define PARTICLELIST_SIZE SIZEOF_NGPARTICLE * MAX_PARTICLES


struct bVector3
{
    float x;
    float y;
    float z;
};

struct bVector4
{
    float x;
    float y;
    float z;
    float w;
};

struct bMatrix4
{
    bVector4 v0;
    bVector4 v1;
    bVector4 v2;
    bVector4 v3;
};

void*(__thiscall* FastMem_Alloc)(void* FastMem, unsigned int bytes, char* kind) = (void*(__thiscall*)(void*, unsigned int, char*))0x005D29D0;
void* (__thiscall* FastMem_Free)(void* FastMem, void* ptr, unsigned int bytes, char* kind) = (void* (__thiscall*)(void*, void*, unsigned int, char*))0x005D0370;
void (__stdcall* __CxxThrowException)(int arg1, int arg2) = (void (__stdcall*)(int, int))0x007C56B0;
void* (__thiscall* Attrib_Instance_MW)(void* Attrib, void* AttribCollection, unsigned int unk, void* ucomlist) = (void* (__thiscall*)(void*, void*, unsigned int, void*))0x00452380;
void*(__cdecl* Attrib_DefaultDataArea)(unsigned int size) = (void*(__cdecl*)(unsigned int))0x006269B0;
void* (__thiscall* Attrib_Instance_Get)(void* AttribCollection, unsigned int unk, unsigned int hash) = (void* (__thiscall*)(void*, unsigned int, unsigned int))0x004546C0;
void* (__thiscall* Attrib_Attribute_GetLength)(void* AttribCollection) = (void* (__thiscall*)(void*))0x00452D40;
void* (__thiscall* Attrib_Dtor)(void* AttribCollection) = (void* (__thiscall*)(void*))0x00452BD0;
void* (__thiscall* Attrib_Instance_GetAttributePointer)(void* AttribCollection, unsigned int hash, unsigned int unk) = (void* (__thiscall*)(void*, unsigned int, unsigned int))0x00454810;
void* (__thiscall* Attrib_RefSpec_GetCollection)(void* Attrib) = (void* (__thiscall*)(void*))0x004560D0;
void* (__thiscall* Attrib_Instance_Dtor)(void* AttribInstance) = (void* (__thiscall*)(void*))0x0045A430;
void* (__thiscall* Attrib_Instance_Refspec)(void* AttribCollection, void* refspec, unsigned int unk, void* ucomlist) = (void* (__thiscall*)(void*, void*, unsigned int, void*))0x00456CB0;
void* (__cdecl* Attrib_FindCollection)(uint32_t param1, uint32_t param2) = (void* (__cdecl*)(uint32_t, uint32_t))0x00455FD0;
float (__cdecl* bRandom_Float_Int)(float range, int unk) = (float (__cdecl*)(float, int))0x0045D9E0;
int(__cdecl* bRandom_Int_Int)(int range, uint32_t* unk) = (int(__cdecl*)(int, uint32_t*))0x0045D9A0;
unsigned int(__cdecl* bStringHash)(char* str) = (unsigned int(__cdecl*)(char*))0x00460BF0;
void*(__cdecl* GetTextureInfo)(unsigned int name_hash, int return_default_texture_if_not_found, int include_unloaded_textures) = (void*(__cdecl*)(unsigned int, int, int))0x00503400;
void (__thiscall* EmitterSystem_UpdateParticles)(void* EmitterSystem, float dt) = (void (__thiscall*)(void*, float))0x00508C30;
void(__thiscall* EmitterSystem_Render)(void* EmitterSystem, void* eView) = (void(__thiscall*)(void*, void*))0x00503D00;
void(__stdcall* sub_7286D0)() = (void(__stdcall*)())0x007286D0;
void* (__cdecl* FastMem_CoreAlloc)(uint32_t size, char* debug_line) = (void* (__cdecl*)(uint32_t, char*))0x00465A70;
void(__stdcall* sub_739600)() = (void(__stdcall*)())0x739600;
void(__thiscall* CarRenderConn_UpdateEngineAnimation)(void* CarRenderConn, float param1, void* PktCarService) = (void(__thiscall*)(void*, float, void*))0x00745F20;
void(__stdcall* sub_6CFCE0)() = (void(__stdcall*)())0x6CFCE0;
void(__cdecl* ParticleSetTransform)(D3DXMATRIX* worldmatrix, uint32_t EVIEW_ID) = (void(__cdecl*)(D3DXMATRIX*, uint32_t))0x6C8000;
bool(__thiscall* WCollisionMgr_CheckHitWorld)(void* WCollisionMgr, bMatrix4* inputSeg, void* cInfo, uint32_t primMask) = (bool(__thiscall*)(void*, bMatrix4*, void*, uint32_t))0x007854B0;

void InitTex();

// bridge the difference between MW and Carbon
void* __stdcall Attrib_Instance(void* collection, uint32_t msgPort)
{
    uint32_t that;
    _asm mov that, ecx
    auto result = Attrib_Instance_MW((void*)that, collection, msgPort, NULL);

    // shift the data back to be compatible with carbon's struct
    //memcpy((void*)that, (void*)(that + 4), 16);

    return result;
}

uint32_t FuelcellEmitterAddr = 0;

// function maps from Carbon to MW
unsigned int __ftol2 = 0x007C4B80;
unsigned int sub_6016B0 = 0x5C5E80;
unsigned int sub_404A20 = 0x004048C0;
unsigned int rsqrt = 0x00410220;
unsigned int sub_478200 = 0x466520;
unsigned int sub_6012B0 = 0x4FA510;

char gNGEffectList[64];

struct NGParticle
{
    /* 0x0000 */ struct bVector3 initialPos;
    /* 0x000c */ unsigned int color;
    /* 0x0010 */ struct bVector3 vel;
    /* 0x001c */ float gravity;
    /* 0x0020 */ struct bVector3 impactNormal;
    /* 0x002c */ float remainingLife;
    /* 0x0030 */ float life;
    /* 0x0034 */ float age;
    /* 0x0038 */ unsigned char elasticity;
    /* 0x0039 */ unsigned char pad[3];
    /* 0x003c */ unsigned char flags;
    /* 0x003d */ unsigned char rotX;
    /* 0x003e */ unsigned char rotY;
    /* 0x003f */ unsigned char rotZ;
    /* 0x0040 */ unsigned char size;
    /* 0x0041 */ unsigned char startX;
    /* 0x0042 */ unsigned char startY;
    /* 0x0043 */ unsigned char startZ;
    /* 0x0044 */ unsigned char uv[4];
}; /* size: 0x0048 */

struct ParticleList
{
    NGParticle mParticles[MAX_PARTICLES];
    unsigned int mNumParticles;
}gParticleList;

//char gParticleList[PARTICLELIST_SIZE + 4];
void* off_468094 = NULL;
bool byte_468098 = false;

#define numParticles dword ptr gParticleList[PARTICLELIST_SIZE]

float flt_9C92F0 = 255.0f;
float flt_9C2478 = 1.0f;
float flt_9EA540 = 42.0f;
float flt_9C2C44 = 100.0f;
float flt_9C77C8 = 0.0039215689f;
float flt_9EAFFC = 0.00048828125f;
float flt_9C248C = 0.0f;
float flt_A6C230 = 0.15000001f;
float flt_9C2A3C = 4.0f;
float flt_9C2888 = 0.5f;
unsigned int randomSeed = 0xDEADBEEF;
const char* TextureName = "MAIN";
float EmitterDeltaTime = 0.0f;

LPDIRECT3DDEVICE9 g_D3DDevice;

struct fuelcell_emitter_mw
{
    bVector4 VolumeCenter;
    bVector4 VelocityDelta;
    bVector4 VolumeExtent;
    bVector4 VelocityInherit;
    bVector4 VelocityStart;
    bVector4 Colour1;
    uint32_t emitteruv_class;
    uint32_t emitteruv_collection;
    uint32_t unk;
    float life;
    float NumParticlesVariance;
    float GravityStart;
    float HeightStart;
    float GravityDelta;
    float LengthStart;
    float LengthDelta;
    float LifeVariance;
    float NumParticles;
    uint16_t Spin;
    uint8_t unk_0xD8782949;
    uint8_t zContrail;
};

struct fuelcell_emitter_carbon
{
    bVector4 VolumeCenter;
    bVector4 VelocityDelta;
    bVector4 VolumeExtent;
    bVector4 VelocityInherit;
    bVector4 VelocityStart;
    bVector4 Colour1;
    uint32_t emitteruv_class;
    uint32_t emitteruv_collection;
    uint32_t unk;
    float life;
    float NumParticlesVariance;
    float GravityStart;
    float HeightStart;
    float GravityDelta;
    float Elasticity;
    float LengthStart;
    float LengthDelta;
    float LifeVariance;
    float NumParticles;
    uint8_t zDebrisType;
    uint8_t zContrail;
}bridge_instance;

class ViewTransform
{
public:
    bMatrix4 m_mViewMatrix;
    bMatrix4 m_mProjectionMatrix;
    bMatrix4 m_mProjectionZBiasMatrix;
    bMatrix4 m_mViewProjectionMatrix;
    bMatrix4 m_mViewProjectionZBiasMatrix;
};

int NGSpriteManager[32];
char NGSpriteManager_ClassData[128];

float GetTargetFrametime()
{
    return **(float**)0x6612EC;
}

uint32_t bridge_oldaddr = 0;
uint32_t bridge_instance_addr = 0;
void __stdcall fuelcell_emitter_bridge()
{
    uint32_t that;
    _asm mov that, ecx
    bridge_instance_addr = that;

    uint32_t instance_pointer = *(uint32_t*)(that + 4);
    fuelcell_emitter_mw* mw_emitter = (fuelcell_emitter_mw*)instance_pointer;
    memset(&bridge_instance, 0, sizeof(fuelcell_emitter_carbon));
    // copy the matching data first (first 0x80 bytes)
    memcpy(&bridge_instance, mw_emitter, 0x80);
    // adapt
    bridge_instance.LengthStart = (*mw_emitter).LengthStart;
    bridge_instance.LengthDelta = (*mw_emitter).LengthDelta;
    bridge_instance.LifeVariance = (*mw_emitter).LifeVariance;
    bridge_instance.NumParticles = (*mw_emitter).NumParticles;
    bridge_instance.zContrail = (*mw_emitter).zContrail;
    // no idea if this is right
    //bridge_instance.zDebrisType = (*mw_emitter).unk_0xD8782949;

    // save & write back the pointer
    bridge_oldaddr = instance_pointer;
    *(uint32_t*)(that + 4) = (uint32_t)&bridge_instance;
}

void __stdcall fuelcell_emitter_bridge_restore()
{
    *(uint32_t*)(bridge_instance_addr + 4) = bridge_oldaddr;
}

void* FastMem_CoreAlloc_Wrapper(uint32_t size)
{
    return FastMem_CoreAlloc(size, 0);
}

void __declspec(naked) sub_736EA0()
{
    _asm
    {
        //push    0FFFFFFFFh
        //push    offset SEH_736EA0
        //mov     eax, large fs : 0
        //push    eax
        //mov     large fs : 0, esp
        sub     esp, 10h
        push    esi
        push    0FE40E637h
        lea     eax, [esp + 8]
        push    eax
        call    Attrib_Instance_Get ; Attrib::Instance::Get(const(ulong))
        mov     ecx, eax
        mov     dword ptr[esp + 1Ch], 0
        call    Attrib_Attribute_GetLength ; Attrib::Attribute::GetLength(const(void))
        lea     ecx, [esp + 4]
        mov     esi, eax
        mov     dword ptr[esp + 1Ch], 0FFFFFFFFh
        //call    nullsub_3
        //mov     ecx, [esp + 14h]
        mov     eax, esi
        pop     esi
        //mov     large fs : 0, ecx
        add     esp, 10h
        retn
    }
}

// EASTL List stuff Start
void __declspec(naked) eastl_vector_uninitialized_copy_impl_XenonEffectDef()
{
    _asm
    {
                mov     eax, [esp+8]
                mov     edx, [esp+10h]
                push    ebx
                mov     ebx, [esp+10h]
                cmp     eax, ebx
                jz      short loc_74C80E
                push    esi
                push    edi

loc_74C7F3:                             ; CODE XREF: eastl::uninitialized_copy_impl<eastl::generic_iterator<XenonEffectDef *,void>,eastl::generic_iterator<XenonEffectDef *,void>>(eastl::generic_iterator<XenonEffectDef *,void>,eastl::generic_iterator<XenonEffectDef *,void>,eastl::generic_iterator<XenonEffectDef *,void>,eastl::integral_constant<bool,0>)+2A↓j
                test    edx, edx
                jz      short loc_74C802
                mov     ecx, 17h
                mov     esi, eax
                mov     edi, edx
                rep movsd

loc_74C802:                             ; CODE XREF: eastl::uninitialized_copy_impl<eastl::generic_iterator<XenonEffectDef *,void>,eastl::generic_iterator<XenonEffectDef *,void>>(eastl::generic_iterator<XenonEffectDef *,void>,eastl::generic_iterator<XenonEffectDef *,void>,eastl::generic_iterator<XenonEffectDef *,void>,eastl::integral_constant<bool,0>)+15↑j
                add     eax, 5Ch ; '\'
                add     edx, 5Ch ; '\'
                cmp     eax, ebx
                jnz     short loc_74C7F3
                pop     edi
                pop     esi

loc_74C80E:                             ; CODE XREF: eastl::uninitialized_copy_impl<eastl::generic_iterator<XenonEffectDef *,void>,eastl::generic_iterator<XenonEffectDef *,void>>(eastl::generic_iterator<XenonEffectDef *,void>,eastl::generic_iterator<XenonEffectDef *,void>,eastl::generic_iterator<XenonEffectDef *,void>,eastl::integral_constant<bool,0>)+F↑j
                mov     eax, [esp+8]
                mov     [eax], edx
                pop     ebx
                retn
    }
}

void __declspec(naked) eastl_vector_erase_XenonEffectDef()
{
    _asm
    {
                push    ecx
                push    ebx
                mov     ebx, [ecx+4]
                push    ebp
                mov     ebp, [esp+14h]
                cmp     ebp, ebx
                push    esi
                mov     esi, [esp+14h]
                mov     [esp+0Ch], ecx
                mov     edx, esi
                mov     eax, ebp
                jz      short loc_752BFE
                push    edi
                lea     esp, [esp+0]

loc_752BE0:                             ; CODE XREF: eastl::vector<XenonEffectDef,bstl::allocator>::erase(XenonEffectDef *,XenonEffectDef *)+33↓j
                mov     esi, eax
                mov     edi, edx
                add     eax, 5Ch ; '\'
                mov     ecx, 17h
                add     edx, 5Ch ; '\'
                cmp     eax, ebx
                rep movsd
                jnz     short loc_752BE0
                mov     ecx, [esp+10h]
                mov     esi, [esp+18h]
                pop     edi

loc_752BFE:                             ; CODE XREF: eastl::vector<XenonEffectDef,bstl::allocator>::erase(XenonEffectDef *,XenonEffectDef *)+19↑j
                sub     ebp, esi
                mov     eax, 4DE9BD37h
                imul    ebp
                sub     edx, ebp
                sar     edx, 6
                mov     eax, edx
                shr     eax, 1Fh
                add     eax, edx
                mov     edx, [ecx+4]
                imul    eax, 5Ch ; '\'
                add     edx, eax
                mov     eax, esi
                pop     esi
                pop     ebp
                mov     [ecx+4], edx
                pop     ebx
                pop     ecx
                retn    8
    }
}

void __declspec(naked) eastl_vector_DoInsertValue_XenonEffectDef()
{
    _asm
    {
                sub     esp, 8
                push    ebx
                push    ebp
                push    esi
                mov     ebx, ecx
                mov     eax, [ebx+8]
                push    edi
                mov     edi, [ebx+4]
                cmp     edi, eax
                jz      short loc_752CAB
                mov     eax, [esp+20h]
                mov     ebp, [esp+1Ch]
                cmp     eax, ebp
                mov     [esp+20h], eax
                jb      short loc_752C5E
                cmp     eax, edi
                jnb     short loc_752C5E
                add     eax, 5Ch ; '\'
                mov     [esp+20h], eax

loc_752C5E:                             ; CODE XREF: eastl::vector<XenonEffectDef,bstl::allocator>::DoInsertValue(XenonEffectDef *,XenonEffectDef const &)+21↑j
                                        ; eastl::vector<XenonEffectDef,bstl::allocator>::DoInsertValue(XenonEffectDef *,XenonEffectDef const &)+25↑j
                test    edi, edi
                jz      short loc_752C6C
                lea     esi, [edi-5Ch]
                mov     ecx, 17h
                rep movsd

loc_752C6C:                             ; CODE XREF: eastl::vector<XenonEffectDef,bstl::allocator>::DoInsertValue(XenonEffectDef *,XenonEffectDef const &)+30↑j
                mov     edx, [ebx+4]
                lea     eax, [edx-5Ch]
                cmp     eax, ebp
                jz      short loc_752C8B

loc_752C76:                             ; CODE XREF: eastl::vector<XenonEffectDef,bstl::allocator>::DoInsertValue(XenonEffectDef *,XenonEffectDef const &)+59↓j
                sub     eax, 5Ch ; '\'
                sub     edx, 5Ch ; '\'
                cmp     eax, ebp
                mov     ecx, 17h
                mov     esi, eax
                mov     edi, edx
                rep movsd
                jnz     short loc_752C76

loc_752C8B:                             ; CODE XREF: eastl::vector<XenonEffectDef,bstl::allocator>::DoInsertValue(XenonEffectDef *,XenonEffectDef const &)+44↑j
                mov     esi, [esp+20h]
                mov     edi, ebp
                mov     ecx, 17h
                rep movsd
                mov     eax, [ebx+4]
                pop     edi
                pop     esi
                add     eax, 5Ch ; '\'
                pop     ebp
                mov     [ebx+4], eax
                pop     ebx
                add     esp, 8
                retn    8
; ---------------------------------------------------------------------------

loc_752CAB:                             ; CODE XREF: eastl::vector<XenonEffectDef,bstl::allocator>::DoInsertValue(XenonEffectDef *,XenonEffectDef const &)+11↑j
                sub     edi, [ebx]
                mov     eax, 0B21642C9h
                imul    edi
                add     edx, edi
                sar     edx, 6
                mov     eax, edx
                shr     eax, 1Fh
                add     eax, edx
                jz      short loc_752CE9
                lea     edi, [eax+eax]
                test    edi, edi
                mov     [esp+14h], edi
                jz      short loc_752CF7

loc_752CCD:                             ; CODE XREF: eastl::vector<XenonEffectDef,bstl::allocator>::DoInsertValue(XenonEffectDef *,XenonEffectDef const &)+C5↓j
                mov     eax, edi
                imul    eax, 5Ch ; '\'
                test    eax, eax
                jz      short loc_752CF7
                push    0
                push    eax
                mov     ecx, FASTMEM_ADDR
                call    FastMem_Alloc ; FastMem::Alloc((uint,char const *))
                mov     [esp+10h], eax
                jmp     short loc_752CFF
; ---------------------------------------------------------------------------

loc_752CE9:                             ; CODE XREF: eastl::vector<XenonEffectDef,bstl::allocator>::DoInsertValue(XenonEffectDef *,XenonEffectDef const &)+90↑j
                mov     dword ptr [esp+14h], 1
                mov     edi, [esp+14h]
                jmp     short loc_752CCD
; ---------------------------------------------------------------------------

loc_752CF7:                             ; CODE XREF: eastl::vector<XenonEffectDef,bstl::allocator>::DoInsertValue(XenonEffectDef *,XenonEffectDef const &)+9B↑j
                                        ; eastl::vector<XenonEffectDef,bstl::allocator>::DoInsertValue(XenonEffectDef *,XenonEffectDef const &)+A4↑j
                mov     dword ptr [esp+10h], 0

loc_752CFF:                             ; CODE XREF: eastl::vector<XenonEffectDef,bstl::allocator>::DoInsertValue(XenonEffectDef *,XenonEffectDef const &)+B7↑j
                mov     ecx, [esp+1Ch]
                mov     edx, [esp+10h]
                mov     ebp, [esp+1Ch]
                mov     eax, [ebx]
                push    ecx
                push    edx
                push    ebp
                push    eax
                lea     eax, [esp+2Ch]
                push    eax
                call    eastl_vector_uninitialized_copy_impl_XenonEffectDef
                mov     eax, [esp+30h]
                add     esp, 14h
                test    eax, eax
                jz      short loc_752D37
                mov     esi, [esp+20h]
                mov     ecx, 17h
                mov     edi, eax
                rep movsd
                mov     edi, [esp+14h]

loc_752D37:                             ; CODE XREF: eastl::vector<XenonEffectDef,bstl::allocator>::DoInsertValue(XenonEffectDef *,XenonEffectDef const &)+F4↑j
                mov     edx, [esp+1Ch]
                mov     ecx, [ebx+4]
                push    edx
                add     eax, 5Ch ; '\'
                push    eax
                push    ecx
                lea     eax, [esp+28h]
                push    ebp
                push    eax
                call    eastl_vector_uninitialized_copy_impl_XenonEffectDef
                mov     esi, [ebx]
                add     esp, 14h
                test    esi, esi
                jz      short loc_752D81
                mov     ecx, [ebx+8]
                sub     ecx, esi
                mov     eax, 0B21642C9h
                imul    ecx
                add     edx, ecx
                sar     edx, 6
                mov     ecx, edx
                shr     ecx, 1Fh
                add     ecx, edx
                imul    ecx, 5Ch ; '\'
                push    0
                push    ecx
                push    esi
                mov     ecx, FASTMEM_ADDR
                call    FastMem_Free ; FastMem::Free((void *,uint,char const *))

loc_752D81:                             ; CODE XREF: eastl::vector<XenonEffectDef,bstl::allocator>::DoInsertValue(XenonEffectDef *,XenonEffectDef const &)+126↑j
                mov     eax, [esp+10h]
                imul    edi, 5Ch ; '\'
                mov     edx, [esp+1Ch]
                add     edi, eax
                mov     [ebx+8], edi
                pop     edi
                pop     esi
                pop     ebp
                mov     [ebx], eax
                mov     [ebx+4], edx
                pop     ebx
                add     esp, 8
                retn    8
    }
}

void __declspec(naked) eastl_vector_reserve_XenonEffectDef()
{
    _asm
    {
                push    ebx
                mov     ebx, [esp+8]
                push    ebp
                push    esi
                mov     esi, ecx
                mov     ebp, [esi]
                mov     ecx, [esi+8]
                sub     ecx, ebp
                mov     eax, 0B21642C9h
                imul    ecx
                add     edx, ecx
                sar     edx, 6
                mov     eax, edx
                shr     eax, 1Fh
                add     eax, edx
                cmp     ebx, eax
                jbe     loc_752858
                test    ebx, ebx
                mov     ecx, [esi+4]
                push    edi
                mov     [esp+14h], ecx
                jz      short loc_7527E1
                mov     eax, ebx
                imul    eax, 5Ch ; '\'
                test    eax, eax
                jz      short loc_7527E1
                push    0
                push    eax
                mov     ecx, FASTMEM_ADDR
                call    FastMem_Alloc ; FastMem::Alloc((uint,char const *))
                mov     edi, eax
                jmp     short loc_7527E3
; ---------------------------------------------------------------------------

loc_7527E1:                             ; CODE XREF: eastl::vector<XenonEffectDef,bstl::allocator>::reserve(uint)+35↑j
                                        ; eastl::vector<XenonEffectDef,bstl::allocator>::reserve(uint)+3E↑j
                xor     edi, edi

loc_7527E3:                             ; CODE XREF: eastl::vector<XenonEffectDef,bstl::allocator>::reserve(uint)+4F↑j
                mov     edx, [esp+14h]
                mov     eax, [esp+14h]
                push    edx
                push    edi
                push    eax
                lea     ecx, [esp+20h]
                push    ebp
                push    ecx
                call    eastl_vector_uninitialized_copy_impl_XenonEffectDef ; eastl::uninitialized_copy_impl<eastl::generic_iterator<XenonEffectDef *,void>,eastl::generic_iterator<XenonEffectDef *,void>>(eastl::generic_iterator<XenonEffectDef *,void>,eastl::generic_iterator<XenonEffectDef *,void>,eastl::generic_iterator<XenonEffectDef *,void>,eastl::integral_constant<bool,0>)
                mov     ebp, [esi]
                add     esp, 14h
                test    ebp, ebp
                jz      short loc_75282B
                mov     ecx, [esi+8]
                sub     ecx, ebp
                mov     eax, 0B21642C9h
                imul    ecx
                add     edx, ecx
                sar     edx, 6
                mov     eax, edx
                shr     eax, 1Fh
                add     eax, edx
                imul    eax, 5Ch ; '\'
                push    0
                push    eax
                push    ebp
                mov     ecx, FASTMEM_ADDR
                call    FastMem_Free ; FastMem::Free((void *,uint,char const *))

loc_75282B:                             ; CODE XREF: eastl::vector<XenonEffectDef,bstl::allocator>::reserve(uint)+70↑j
                mov     edx, [esi]
                imul    ebx, 5Ch ; '\'
                mov     ecx, [esi+4]
                sub     ecx, edx
                mov     eax, 0B21642C9h
                imul    ecx
                add     edx, ecx
                sar     edx, 6
                mov     eax, edx
                shr     eax, 1Fh
                add     eax, edx
                imul    eax, 5Ch ; '\'
                add     eax, edi
                add     ebx, edi
                mov     [esi], edi
                mov     [esi+4], eax
                mov     [esi+8], ebx
                pop     edi

loc_752858:                             ; CODE XREF: eastl::vector<XenonEffectDef,bstl::allocator>::reserve(uint)+25↑j
                pop     esi
                pop     ebp
                pop     ebx
                retn    4
    }
}

void(__thiscall* eastl_vector_reserve_XenonEffectDef_Abstract)(void* vector, uint32_t n) = (void(__thiscall*)(void*, uint32_t))&eastl_vector_reserve_XenonEffectDef;
void(__thiscall* eastl_vector_erase_XenonEffectDef_Abstract)(void* vector, void* first, void* last) = (void(__thiscall*)(void*, void*, void*))&eastl_vector_erase_XenonEffectDef;

void __stdcall XenonEffectList_Initialize()
{
    eastl_vector_reserve_XenonEffectDef_Abstract(&gNGEffectList, NGEFFECT_LIST_COUNT);
    //eastl_vector_erase_XenonEffectDef_Abstract(&gNGEffectList, gNGEffectList, (void*)((uint32_t)(gNGEffectList + 4)));
}

// EASTL List stuff End

void sub_739600_hook()
{
    sub_739600();
    //_asm mov ecx, offset gNGEffectList
    XenonEffectList_Initialize();
    InitTex();
}

// note: unk_9D7880 == unk_8A3028

void __declspec(naked) AddXenonEffect() // (AcidEffect *piggyback_fx, Attrib::Collection *spec, UMath::Matrix4 *mat, UMath::Vector4 *vel, float intensity)
{
	_asm
	{
                sub     esp, 5Ch
                push    ebx
                mov     ebx, dword ptr gNGEffectList[0]
                push    edi
                mov     edi, dword ptr gNGEffectList[4]
                mov     ecx, ebx
                sub     ecx, edi
                mov     eax, 0B21642C9h
                imul    ecx
                add     edx, ecx
                sar     edx, 6
                mov     eax, edx
                shr     eax, 1Fh
                add     eax, edx
                cmp     eax, NGEFFECT_LIST_COUNT ; 'd'
                jnb     loc_754DA5
                push    esi
                mov     ecx, 10h
                mov     esi, 0x8A3028
                lea     edi, [esp+20h]
                rep movsd
                mov     ecx, [esp+74h]
                add     ecx, 30h ; '0'
                mov     edx, [ecx]
                mov     eax, [ecx+4]
                mov     [esp+50h], edx
                mov     edx, [ecx+8]
                mov     [esp+54h], eax
                mov     eax, [ecx+0Ch]
                mov     ecx, [esp+70h]
                mov     [esp+58h], edx
                mov     edx, [esp+78h]
                mov     [esp+5Ch], eax
                mov     eax, [edx]
                mov     [esp+60h], ecx
                mov     ecx, [edx+4]
                mov     [esp+10h], eax
                mov     eax, [edx+8]
                mov     [esp+18h], eax
                mov     eax, [esp+6Ch]
                mov     [esp+14h], ecx
                mov     ecx, [edx+0Ch]
                mov     edx, [esp+7Ch]
                mov     [esp+64h], eax
                cmp     ebx, dword ptr gNGEffectList[8]
                mov     [esp+1Ch], ecx
                mov     [esp+0Ch], edx
                jnb     loc_754D94
                mov     edi, ebx
                add     ebx, 5Ch ; '\'
                test    edi, edi
                mov     dword ptr gNGEffectList[4], ebx
                jz      loc_754DA4
                mov     ecx, 17h
                lea     esi, [esp+0Ch]
                rep movsd
                pop     esi
                pop     edi
                pop     ebx
                add     esp, 5Ch
                retn
; ---------------------------------------------------------------------------

loc_754D94:                             ; CODE XREF: AddXenonEffect(AcidEffect *,Attrib::Collection const *,UMath::Matrix4 const *,UMath::Vector4 const *,float)+A1↑j
                lea     ecx, [esp+0Ch]
                push    ecx
                push    ebx
                mov     ecx, offset gNGEffectList
                call    eastl_vector_DoInsertValue_XenonEffectDef ; eastl::vector<XenonEffectDef,bstl::allocator>::DoInsertValue(XenonEffectDef *,XenonEffectDef const &)

loc_754DA4:                             ; CODE XREF: AddXenonEffect(AcidEffect *,Attrib::Collection const *,UMath::Matrix4 const *,UMath::Vector4 const *,float)+B0↑j
                pop     esi

loc_754DA5:                             ; CODE XREF: AddXenonEffect(AcidEffect *,Attrib::Collection const *,UMath::Matrix4 const *,UMath::Vector4 const *,float)+2B↑j
                pop     edi
                pop     ebx
                add     esp, 5Ch
                retn
	}
}

void(__cdecl* AddXenonEffect_Abstract)(void* piggyback_fx, void* spec, bMatrix4* mat, bVector4* vel, float intensity) = (void(__cdecl*)(void*, void*, bMatrix4*, bVector4*, float)) & AddXenonEffect;

void __declspec(naked) CalcCollisiontime()
{
    _asm
    {
        sub     esp, 9Ch
        fld     flt_A6C230
        push    esi
        mov     esi, [esp + 0A4h]
        fadd    dword ptr[esi + 8]
        mov     eax, [esi + 30h]
        mov[esp + 4], eax
        mov     ecx, [esi]
        fst     dword ptr[esi + 8]
        mov     edx, ecx
        fld     dword ptr[esp + 4]
        mov[esp + 24h], ecx
        fmul    dword ptr[esi + 10h]
        mov     dword ptr[esp + 18h], 3F800000h
        mov[esp + 14h], edx
        mov[esp + 30h], edx
        fadd    dword ptr[esi]
        fld     dword ptr[esp + 4]
        fmul    dword ptr[esi + 14h]
        fadd    dword ptr[esi + 4]
        fstp    dword ptr[esp + 20h]
        fld     dword ptr[esi + 30h]
        fld     dword ptr[esp + 4]
        fmul    dword ptr[esi + 18h]
        fadd    st, st(3)
        fld     st(1)
        fmul    st, st(2)
        fmul    dword ptr[esi + 1Ch]
        faddp   st(1), st
        fstp    st(1)
        fld     dword ptr[esp + 20h]
        fchs
        fld     dword ptr[esi + 4]
        fchs
        fstp    dword ptr[esp + 0Ch]
        mov     eax, [esp + 0Ch]
        fxch    st(3)
        mov[esp + 28h], eax
        mov     eax, [esp + 18h]
        fstp    dword ptr[esp + 10h]
        mov     ecx, [esp + 10h]
        fxch    st(2)
        fstp    dword ptr[esp + 0Ch]
        mov[esp + 2Ch], ecx
        mov     ecx, [esp + 0Ch]
        fxch    st(1)
        fstp    dword ptr[esp + 10h]
        mov     edx, [esp + 10h]
        mov[esp + 38h], ecx
        mov     dword ptr[esp + 18h], 3F800000h
        fstp    dword ptr[esp + 14h]
        mov     ecx, [esp + 18h]
        mov[esp + 34h], eax
        mov     eax, [esp + 14h]
        mov[esp + 44h], ecx
        lea     ecx, [esp + 48h]
        mov[esp + 3Ch], edx
        mov[esp + 40h], eax
        call    sub_404A20
        fld     flt_A6C230
        fadd    dword ptr[esp + 2Ch]
        push    3
        lea     edx, [esp + 4Ch]
        push    edx
        lea     eax, [esp + 30h]
        fstp    dword ptr[esp + 34h]
        push    eax
        lea     ecx, [esp + 28h]
        mov     dword ptr[esp + 28h], 0
        mov     dword ptr[esp + 2Ch], 3
        call    WCollisionMgr_CheckHitWorld
        test    eax, eax
        jz      loc_73F30F
        mov     ecx, [esi + 18h]
        mov[esp + 4], ecx
        fld     dword ptr[esp + 4]
        fmul    dword ptr[esp + 4]
        fld     dword ptr[esi + 8]
        fsub    dword ptr[esp + 4Ch]
        fmul    dword ptr[esi + 1Ch]
        fmul    ds : flt_9C2A3C
        fsubp   st(1), st
        fst     dword ptr[esp + 8]
        fcomp   ds : flt_9C248C
        fnstsw  ax
        test    ah, 41h
        jp      loc_73F2AA
        fld     ds : flt_9C248C
        jmp     loc_73F2ED
        ; -------------------------------------------------------------------------- -

        loc_73F2AA:; CODE XREF : CalcCollisiontime + 140↑j
        mov     edx, [esp + 8]
        push    edx; float
        call    sub_6016B0
        fstp    dword ptr[esp + 0Ch]
        fld     dword ptr[esi + 1Ch]
        add     esp, 4
        fadd    st, st
        fstp    dword ptr[esp + 1Ch]
        fld     dword ptr[esp + 8]
        fsub    dword ptr[esp + 4]
        fdiv    dword ptr[esp + 1Ch]
        fcom    ds : flt_9C248C
        fnstsw  ax
        test    ah, 5
        jp      loc_73F2ED
        fstp    st
        fld     dword ptr[esp + 4]
        fchs
        fsub    dword ptr[esp + 8]
        fdiv    dword ptr[esp + 1Ch]

        loc_73F2ED:; CODE XREF : CalcCollisiontime + 148↑j
        ; CalcCollisiontime + 17B↑j
        mov     al, [esi + 3Ch]
        fstp    dword ptr[esi + 30h]
        fld     dword ptr[esp + 58h]
        mov     ecx, [esp + 5Ch]
        or al, 2
        fchs
        mov[esi + 3Ch], al
        fstp    dword ptr[esi + 24h]
        mov     eax, [esp + 60h]
        mov[esi + 20h], eax
        mov[esi + 28h], ecx

        loc_73F30F : ; CODE XREF : CalcCollisiontime + 10A↑j
        pop     esi
        add     esp, 9Ch
        retn
    }
}

void __declspec(naked) BounceParticle()
{
    _asm
    {
                sub     esp, 18h
                push    esi
                mov     esi, [esp+20h]
                fld     dword ptr [esi+30h]
                push    edi
                fld     st
                lea     edi, [esi+10h]
                fmul    dword ptr [edi]
                mov     eax, esi
                fadd    dword ptr [esi]
                fstp    dword ptr [esp+14h]
                mov     ecx, [esp+14h]
                fld     st
                fmul    dword ptr [edi+4]
                fadd    dword ptr [esi+4]
                fstp    dword ptr [esp+18h]
                mov     edx, [esp+18h]
                fld     st
                fmul    dword ptr [esi+1Ch]
                fld     st(1)
                fmul    dword ptr [edi+8]
                fadd    dword ptr [esi+8]
                fld     st(1)
                mov     [eax], ecx
                fmul    st, st(3)
                mov     [eax+4], edx
                mov     edx, edi
                faddp   st(1), st
                fstp    dword ptr [esp+1Ch]
                mov     ecx, [esp+1Ch]
                mov     [eax+8], ecx
                mov     eax, [edx]
                fadd    st, st
                mov     ecx, [edx+4]
                mov     edx, [edx+8]
                mov     [esp+1Ch], edx
                fadd    dword ptr [esp+1Ch]
                mov     [esp+0Ch], ecx
                mov     [esp+8], eax
                fstp    dword ptr [esp+1Ch]
                mov     eax, [esp+1Ch]
                lea     ecx, [esp+8]
                push    ecx             ; float
                fstp    st
                mov     [esp+14h], eax
                call    rsqrt
                fld     dword ptr [esp+14h]
                fmul    dword ptr [esi+28h]
                movzx   edx, byte ptr [esi+38h]
                fld     dword ptr [esp+10h]
                fmul    dword ptr [esi+24h]
                faddp   st(1), st
                fld     dword ptr [esp+0Ch]
                fmul    dword ptr [esi+20h]
                faddp   st(1), st
                fadd    st, st
                fld     st
                fmul    dword ptr [esi+20h]
                fld     st(1)
                fmul    dword ptr [esi+24h]
                fstp    dword ptr [esp+1Ch]
                fxch    st(1)
                fmul    dword ptr [esi+28h]
                fstp    dword ptr [esp+20h]
                fld     dword ptr [esp+0Ch]
                fsub    st, st(1)
                fstp    dword ptr [esp+18h]
                fstp    st
                fld     dword ptr [esp+10h]
                fsub    dword ptr [esp+1Ch]
                fstp    dword ptr [esp+1Ch]
                fld     dword ptr [esp+14h]
                fsub    dword ptr [esp+20h]
                fstp    dword ptr [esp+20h]
                mov     [esp+28h], edx
                fimul   dword ptr [esp+28h]
                push    esi
                mov     dword ptr [esi+34h], 0
                fmul    ds:flt_9C77C8
                fld     dword ptr [esp+1Ch]
                fmul    st, st(1)
                fld     dword ptr [esp+20h]
                fmul    st, st(2)
                fld     dword ptr [esp+24h]
                fmul    st, st(3)
                fstp    dword ptr [esp+18h]
                mov     eax, [esp+18h]
                fxch    st(1)
                mov     [esp+24h], eax
                fstp    dword ptr [esp+1Ch]
                mov     ecx, [esp+1Ch]
                mov     [edi], ecx
                mov     cl, [esi+3Ch]
                fstp    dword ptr [esp+20h]
                mov     edx, [esp+20h]
                mov     [edi+4], edx
                fstp    st
                mov     edx, [esi+2Ch]
                and     cl, 1
                or      cl, 4
                mov     [edi+8], eax
                mov     [esi+3Ch], cl
                mov     [esi+30h], edx
                call    CalcCollisiontime
                add     esp, 8
                pop     edi
                mov     al, 1
                pop     esi
                add     esp, 18h
                retn
    }
}

void __declspec(naked) ParticleList_AgeParticles()
{
    _asm
    {
                sub     esp, 10h
                push    ebx
                push    ebp
                mov     ebp, ecx
                mov     ecx, [ebp+PARTICLELIST_SIZE]
                lea     edx, [ebp+PARTICLELIST_SIZE]
                xor     eax, eax
                cmp     ecx, eax
                mov     ebx, ebp
                mov     [esp+8], eax
                mov     [esp+14h], eax
                mov     [esp+10h], edx
                jle     loc_74A24F
                push    esi
                push    edi
                lea     ecx, [ecx+0]

loc_74A1C0:                             ; CODE XREF: ParticleList::AgeParticles((float))+B3↓j
                fld     dword ptr [esp+24h]
                fadd    dword ptr [ebp+34h]
                fst     dword ptr [esp+14h]
                fcomp   dword ptr [ebp+30h]
                fnstsw  ax
                test    ah, 41h
                jnz     short loc_74A212
                test    byte ptr [ebp+3Ch], 2
                jz      short loc_74A20D
                fld     dword ptr [ebp+2Ch]
                fsub    dword ptr [esp+14h]
                fst     dword ptr [ebp+2Ch]
                fcomp   dword ptr [esp+24h]
                fnstsw  ax
                test    ah, 41h
                jnz     short loc_74A20D
                push    ebp
                call    BounceParticle
; ---------------------------------------------------------------------------
                mov     edx, [esp+1Ch]
                add     esp, 4
                test    al, al
                jz      short loc_74A20D
                mov     eax, [esp+10h]
                add     ebx, 48h ; 'H'
                inc     eax
                mov     [esp+10h], eax

loc_74A20D:                             ; CODE XREF: ParticleList::AgeParticles((float))+49↑j
                                        ; ParticleList::AgeParticles((float))+5E↑j ...
                add     ebp, 48h ; 'H'
                jmp     short loc_74A236
; ---------------------------------------------------------------------------

loc_74A212:                             ; CODE XREF: ParticleList::AgeParticles((float))+43↑j
                fld     dword ptr [esp+24h]
                mov     eax, [esp+10h]
                mov     esi, ebp
                mov     edi, ebx
                mov     ecx, 12h
                rep movsd
                fadd    dword ptr [ebx+34h]
                fstp    dword ptr [ebx+34h]
                add     ebp, 48h ; 'H'
                add     ebx, 48h ; 'H'
                inc     eax
                mov     [esp+10h], eax

loc_74A236:                             ; CODE XREF: ParticleList::AgeParticles((float))+80↑j
                mov     eax, [esp+1Ch]
                mov     ecx, [edx]
                inc     eax
                cmp     eax, ecx
                mov     [esp+1Ch], eax
                jl      loc_74A1C0
                mov     eax, [esp+10h]
                pop     edi
                pop     esi

loc_74A24F:                             ; CODE XREF: ParticleList::AgeParticles((float))+25↑j
                pop     ebp
                mov     [edx], eax
                pop     ebx
                add     esp, 10h
                retn    4
    }
}

char fuelcell_attrib_buffer5[20];
void* __stdcall Attrib_Instance_GetAttributePointer_Shim(uint32_t hash, uint32_t unk)
{
    uint32_t that;
    _asm mov that, ecx

    memset(fuelcell_attrib_buffer5, 0, 20);
    memcpy(&(fuelcell_attrib_buffer5[4]), (void*)that, 16);

    return Attrib_Instance_GetAttributePointer((void*)fuelcell_attrib_buffer5, hash, unk);
}

void __declspec(naked) CGEmitter_SpawnParticles()
{
    _asm
    {
                fld     dword ptr [esp+8]
                sub     esp, 0ACh
                fcomp   ds:flt_9C248C
                push    ebp
                mov     ebp, ecx
                fnstsw  ax
                test    ah, 41h
                jnp     loc_73F920
                fld     dword ptr [esp+0B4h]
                fcomp   ds:flt_9C248C
                fnstsw  ax
                test    ah, 41h
                jnp     loc_73F920
                mov     eax, randomSeed
                push    ebx
                push    esi
                push    edi
                lea     esi, [ebp+30h]
                mov     ecx, 10h
                lea     edi, [esp+7Ch]
                rep movsd
                mov     esi, [ebp+4]
                mov     [esp+10h], eax
                fld     dword ptr [esi+6Ch]
                fld     st
                fmul    dword ptr [esi+8Ch]
                fsubr   st, st(1)
                fstp    dword ptr [esp+3Ch]
                fstp    st
                fld     dword ptr [esi+50h]
                fmul    ds:flt_9C92F0
                call    __ftol2
                fld     dword ptr [esi+54h]
                fmul    ds:flt_9C92F0
                mov     edi, eax
                call    __ftol2
                fld     dword ptr [esi+58h]
                fmul    ds:flt_9C92F0
                mov     ebx, eax
                call    __ftol2
                fld     dword ptr [esi+5Ch]
                fmul    ds:flt_9C92F0
                mov     [esp+24h], eax
                call    __ftol2
                fld     ds:flt_9C2478
                fld     dword ptr [esp+0C4h]
                mov     ecx, eax
                fucompp
                fnstsw  ax
                test    ah, 44h
                jnp     short loc_73F409
                fld     dword ptr [esp+0C4h]
                fmul    ds:flt_9EA540
                fld     ds:flt_9EA540
                fcomp   st(1)
                fnstsw  ax
                test    ah, 5
                jp      short loc_73F402
                fstp    st
                fld     ds:flt_9EA540

loc_73F402:                             ; CODE XREF: CGEmitter::SpawnParticles(float,float)+D8↑j
                call    __ftol2
                mov     ecx, eax

loc_73F409:                             ; CODE XREF: CGEmitter::SpawnParticles(float,float)+BC↑j
                fld     dword ptr [esp+0C4h]
                mov     eax, [esp+24h]
                fcomp   ds:flt_9C2478
                shl     ecx, 8
                or      ecx, edi
                shl     ecx, 8
                or      ecx, ebx
                shl     ecx, 8
                or      ecx, eax
                fnstsw  ax
                mov     [esp+50h], ecx
                test    ah, 41h
                jnz     short loc_73F43D
                fld     dword ptr [esp+0C4h]
                jmp     short loc_73F443
; ---------------------------------------------------------------------------

loc_73F43D:                             ; CODE XREF: CGEmitter::SpawnParticles(float,float)+112↑j
                fld     ds:flt_9C2478

loc_73F443:                             ; CODE XREF: CGEmitter::SpawnParticles(float,float)+11B↑j
                mov     eax, [ebp+4]
                fmul    dword ptr [eax+90h]
                fmul    dword ptr [eax+70h]
                fld     dword ptr [esp+0C4h]
                fcomp   ds:flt_9C2478
                fnstsw  ax
                test    ah, 41h
                jnz     short loc_73F46C
                fld     dword ptr [esp+0C4h]
                jmp     short loc_73F472
; ---------------------------------------------------------------------------

loc_73F46C:                             ; CODE XREF: CGEmitter::SpawnParticles(float,float)+141↑j
                fld     ds:flt_9C2478

loc_73F472:                             ; CODE XREF: CGEmitter::SpawnParticles(float,float)+14A↑j
                mov     ecx, [ebp+4]
                fmul    dword ptr [ecx+90h]
                xor     ebx, ebx
                fxch    st(1)
                push    ebx
                fmul    ds:flt_9C2C44
                push    28638D89h
                mov     ecx, ebp
                //sub ecx, 4
                fsubp   st(1), st
                fstp    dword ptr [esp+28h]
                call    Attrib_Instance_GetAttributePointer_Shim; Attrib::Instance::GetAttributePointer(const(ulong,uint))
                test    eax, eax
                jnz     short loc_73F4A6
                push    1
                call    Attrib_DefaultDataArea; Attrib::DefaultDataArea((uint))
                add     esp, 4

loc_73F4A6:                             ; CODE XREF: CGEmitter::SpawnParticles(float,float)+17A↑j
                mov     dl, [eax]
                push    0
                push    0D2603865h
                mov     ecx, ebp
                //sub ecx, 4
                mov     [esp+22h], dl
                call    Attrib_Instance_GetAttributePointer_Shim; Attrib::Instance::GetAttributePointer(const(ulong,uint))
                test    eax, eax
                jnz     short loc_73F4C8
                push    1
                call    Attrib_DefaultDataArea; Attrib::DefaultDataArea((uint))
                add     esp, 4

loc_73F4C8:                             ; CODE XREF: CGEmitter::SpawnParticles(float,float)+19C↑j
                mov     al, [eax]
                push    0
                push    0E2CC8106h
                mov     ecx, ebp
                //sub ecx, 4
                mov     [esp+21h], al
                call    Attrib_Instance_GetAttributePointer_Shim; Attrib::Instance::GetAttributePointer(const(ulong,uint))
                test    eax, eax
                jnz     short loc_73F4EA
                push    1
                call    Attrib_DefaultDataArea ; Attrib::DefaultDataArea((uint))
                add     esp, 4

loc_73F4EA:                             ; CODE XREF: CGEmitter::SpawnParticles(float,float)+1BE↑j
                mov     cl, [eax]
                mov     edx, [ebp+4]
                movzx   eax, byte ptr [edx+94h]
                test    eax, eax
                mov     [esp+1Bh], cl
                mov     [esp+24h], eax
                jz      short loc_73F50C
                dec     eax
                mov     ebx, 1
                mov     [esp+24h], eax

loc_73F50C:                             ; CODE XREF: CGEmitter::SpawnParticles(float,float)+1E0↑j
                fld     dword ptr [esp+0C0h]
                mov     dword ptr [esp+14h], 0
                fdiv    dword ptr [esp+20h]
                fstp    dword ptr [esp+58h]
                fld     ds:flt_9C248C
                fld     dword ptr [esp+20h]
                fucompp
                fnstsw  ax
                test    ah, 44h
                jnp     loc_73F913
                lea     ebx, [ebx+0]

loc_73F540:                             ; CODE XREF: CGEmitter::SpawnParticles(float,float)+5ED↓j
                fld     dword ptr [esp+20h]
                mov     eax, numParticles
                cmp     eax, MAX_PARTICLES
                fsub    ds:flt_9C2478
                fstp    dword ptr [esp+20h]
                jnb     loc_73F913
                lea     esi, [eax+eax*8]
                lea     esi, gParticleList[esi*8] ; ParticleList gParticleList
                inc     eax
                test    esi, esi
                mov     numParticles, eax
                jz      loc_73F913
                mov     eax, [ebp+4]
                mov     ecx, [eax+84h]
                mov     edx, [eax+88h]
                lea     eax, [esp+10h]
                mov     [esp+1Ch], ecx
                push    eax             ; int
                mov     ecx, edx
                push    ecx             ; float
                mov     [esp+5Ch], edx
                call    bRandom_Float_Int; bRandom(float,uint *)
                fadd    dword ptr [esp+24h]
                add     esp, 8
                fst     dword ptr [esp+1Ch]
                fcomp   ds:flt_9C248C
                fnstsw  ax
                test    ah, 5
                jnp     loc_73F913
                fld     dword ptr [esp+1Ch]
                fcomp   ds:flt_9C92F0
                fnstsw  ax
                test    ah, 5
                jnp     short loc_73F5CF
                mov     dword ptr [esp+1Ch], 437F0000h

loc_73F5CF:                             ; CODE XREF: CGEmitter::SpawnParticles(float,float)+2A5↑j
                mov     edi, [ebp+4]
                mov     eax, [edi+10h]
                lea     edx, [esp+10h]
                push    edx             ; int
                push    eax             ; float
                call    bRandom_Float_Int; bRandom(float,uint *)
                fadd    st, st
                lea     ecx, [esp+18h]
                push    ecx             ; int
                fsubr   dword ptr [edi+10h]
                mov     edi, [ebp+4]
                mov     edx, [edi+14h]
                push    edx             ; float
                fsubr   ds:flt_9C2478
                fstp    dword ptr [esp+6Ch]
                call    bRandom_Float_Int; bRandom(float,uint *)
                fadd    st, st
                lea     eax, [esp+20h]
                push    eax             ; int
                fsubr   dword ptr [edi+14h]
                mov     edi, [ebp+4]
                mov     ecx, [edi+18h]
                push    ecx             ; float
                fsubr   ds:flt_9C2478
                fstp    dword ptr [esp+78h]
                call    bRandom_Float_Int  ; bRandom(float,uint *)
                mov     eax, [ebp+4]
                fadd    st, st
                lea     edx, [esp+84h]
                push    edx
                fsubr   dword ptr [edi+18h]
                lea     ecx, [ebp+30h]
                push    ecx
                add     eax, 40h ; '@'
                fsubr   ds:flt_9C2478
                push    eax
                fstp    dword ptr [esp+88h]
                fld     dword ptr [eax-10h]
                fmul    dword ptr [ebp+20h]
                fstp    dword ptr [esp+4Ch]
                fld     dword ptr [eax-0Ch]
                fmul    dword ptr [ebp+24h]
                fstp    dword ptr [esp+50h]
                fld     dword ptr [eax-8]
                fmul    dword ptr [ebp+28h]
                fstp    dword ptr [esp+54h]
                call    sub_478200
                fld     dword ptr [esp+90h]
                mov     edi, [ebp+4]
                fadd    dword ptr [esp+4Ch]
                mov     ecx, [edi+7Ch]
                fld     dword ptr [esp+94h]
                lea     eax, [esp+34h]
                fadd    dword ptr [esp+50h]
                push    eax             ; int
                fld     dword ptr [esp+9Ch]
                push    ecx             ; float
                fadd    dword ptr [esp+5Ch]
                fstp    dword ptr [esp+5Ch]
                fld     dword ptr [esp+0A4h]
                fadd    dword ptr [esp+60h]
                fstp    dword ptr [esp+60h]
                fxch    st(1)
                fmul    dword ptr [esp+88h]
                fstp    dword ptr [esp+54h]
                fmul    dword ptr [esp+8Ch]
                fstp    dword ptr [esp+58h]
                fld     dword ptr [esp+5Ch]
                fmul    dword ptr [esp+90h]
                fstp    dword ptr [esp+5Ch]
                call    bRandom_Float_Int; bRandom(float,uint *)
                fadd    st, st
                lea     edx, [esp+3Ch]
                fld     dword ptr [edi+74h]
                fsub    dword ptr [edi+7Ch]
                mov     edi, [ebp+4]
                faddp   st(1), st
                fstp    dword ptr [esp+64h]
                mov     eax, [edi+20h]
                push    edx             ; int
                push    eax             ; float
                call    bRandom_Float_Int; bRandom(float,uint *)
                fld     dword ptr [edi+20h]
                fmul    ds:flt_9C2888
                lea     ecx, [esp+44h]
                push    ecx             ; int
                fsubp   st(1), st
                fadd    dword ptr [edi]
                mov     edi, [ebp+4]
                mov     edx, [edi+24h]
                push    edx             ; float
                fstp    dword ptr [esp+7Ch]
                call    bRandom_Float_Int; bRandom(float,uint *)
                fld     dword ptr [edi+24h]
                fmul    ds:flt_9C2888
                lea     eax, [esp+4Ch]
                push    eax             ; int
                fsubp   st(1), st
                fadd    dword ptr [edi+4]
                mov     edi, [ebp+4]
                mov     ecx, [edi+28h]
                push    ecx             ; float
                fstp    dword ptr [esp+88h]
                call    bRandom_Float_Int  ; bRandom(float,uint *)
                fld     dword ptr [edi+28h]
                fmul    ds:flt_9C2888
                add     esp, 44h
                lea     edx, [esp+40h]
                push    edx
                fsubp   st(1), st
                lea     eax, [esp+80h]
                push    eax
                fadd    dword ptr [edi+8]
                mov     ecx, edx
                push    ecx
                fstp    dword ptr [esp+54h]
                mov     dword ptr [esp+58h], 3F800000h
                call    sub_6012B0
                fld     dword ptr [esp+34h]
                fmul    dword ptr [esp+20h]
                mov     edx, [esp+34h]
                mov     eax, [esp+38h]
                mov     ecx, [esp+3Ch]
                fadd    dword ptr [esp+4Ch]
                mov     [esi+10h], edx
                mov     edx, [esp+48h]
                fstp    dword ptr [esi]
                mov     [esi+14h], eax
                fld     dword ptr [esp+38h]
                mov     eax, edx
                fmul    dword ptr [esp+20h]
                mov     [esi+18h], ecx
                mov     ecx, [esp+20h]
                mov     [esi+2Ch], edx
                mov     edx, [esp+44h]
                fadd    dword ptr [esp+50h]
                mov     [esi+30h], eax
                mov     [esi+34h], ecx
                fstp    dword ptr [esi+4]
                mov     [esi+1Ch], edx
                fld     dword ptr [esp+44h]
                add     esp, 0Ch
                fmul    dword ptr [esp+14h]
                fmul    dword ptr [esp+14h]
                fld     dword ptr [esp+30h]
                fmul    dword ptr [esp+14h]
                faddp   st(1), st
                fadd    dword ptr [esp+48h]
                fstp    dword ptr [esi+8]
                mov     eax, [ebp+4]
                fld     dword ptr [eax+80h]
                call    __ftol2
                mov     [esi+38h], al
                mov     ecx, [ebp+4]
                fld     dword ptr [ecx+78h]
                call    __ftol2
                test    bl, 1
                mov     edx, [esp+50h]
                mov     [esi+40h], al
                mov     [esi+0Ch], edx
                mov     [esi+3Ch], bl
                jz      short loc_73F87A
                mov     al, [esp+24h]
                mov     [esi+44h], al
                mov     ecx, [ebp+14h]
                fld     dword ptr [ecx+4] // emitteruv StartU
                call    __ftol2
                mov     [esi+45h], al
                mov     edx, [ebp+14h]
                fld     dword ptr [edx+0Ch] // emitteruv StartV
                call    __ftol2
                mov     [esi+46h], al
                lea     eax, [esp+10h]
                push    eax
                push    0FFh
                call    bRandom_Int_Int; bRandom(int,uint *)
                lea     ecx, [esp+18h]
                push    ecx
                push    0FFh
                mov     [esi+41h], al
                call    bRandom_Int_Int; bRandom(int,uint *)
                lea     edx, [esp+20h]
                push    edx
                push    0FFh
                mov     [esi+42h], al
                call    bRandom_Int_Int  ; bRandom(int,uint *)
                mov     cl, [esp+31h]
                mov     dl, [esp+33h]
                mov     [esi+43h], al
                mov     al, [esp+32h]
                add     esp, 18h
                mov     [esi+3Dh], al
                mov     [esi+3Eh], cl
                mov     [esi+3Fh], dl
                jmp     short loc_73F8D5
; ---------------------------------------------------------------------------

loc_73F87A:                             ; CODE XREF: CGEmitter::SpawnParticles(float,float)+4E5↑j
                mov     eax, [ebp+14h]
                fld     dword ptr [eax+4] // emitteruv StartU
                fmul    ds:flt_9C92F0
                call    __ftol2
                mov     [esi+44h], al
                mov     ecx, [ebp+14h]
                fld     dword ptr [ecx+0Ch] // emitteruv StartV
                fmul    ds:flt_9C92F0
                call    __ftol2
                mov     [esi+45h], al
                mov     edx, [ebp+14h]
                fld     dword ptr [edx+8] // emitteruv EndU
                fmul    ds:flt_9C92F0
                call    __ftol2
                mov     [esi+46h], al
                mov     eax, [ebp+14h]
                fld     dword ptr [eax] // emitteruv EndV
                fmul    ds:flt_9C92F0
                call    __ftol2
                fld     dword ptr [esp+1Ch]
                mov     [esi+47h], al
                call    __ftol2
                mov     [esi+41h], al

loc_73F8D5:                             ; CODE XREF: CGEmitter::SpawnParticles(float,float)+558↑j
                fld     dword ptr [esp+14h]
                mov     al, [esi+3Ch]
                test    al, 4
                fadd    dword ptr [esp+58h]
                fstp    dword ptr [esp+14h]
                jnz     short loc_73F8FC
                mov     al, [esp+0C8h]
                test    al, al
                jnz     short loc_73F8FC
                push    esi
                call    CalcCollisiontime ; CalcCollisiontime(NGParticle *)
                add     esp, 4

loc_73F8FC:                             ; CODE XREF: CGEmitter::SpawnParticles(float,float)+5C6↑j
                                        ; CGEmitter::SpawnParticles(float,float)+5D1↑j
                fld     ds:flt_9C248C
                fld     dword ptr [esp+20h]
                fucompp
                fnstsw  ax
                test    ah, 44h
                jp      loc_73F540

loc_73F913:                             ; CODE XREF: CGEmitter::SpawnParticles(float,float)+214↑j
                                        ; CGEmitter::SpawnParticles(float,float)+238↑j ...
                mov     ecx, [esp+10h]
                pop     edi
                pop     esi
                mov     randomSeed, ecx
                pop     ebx

loc_73F920:                             ; CODE XREF: CGEmitter::SpawnParticles(float,float)+18↑j
                                        ; CGEmitter::SpawnParticles(float,float)+30↑j
                pop     ebp
                add     esp, 0ACh
                retn    0Ch
    }
}

void __declspec(naked) Attrib_Gen_fuelcell_emitter_constructor()
{
    _asm
    {
                //push    0FFFFFFFFh
                //push    offset SEH_73EEB0
                //mov     eax, large fs:0
                //push    eax
                //mov     large fs:0, esp
                //push    ecx
                sub esp, 10h
                mov     eax, [esp+18h]
                push    esi
                mov     esi, ecx
                mov     ecx, [esp+18h]
                push    eax
                push    ecx
                mov     ecx, esi
                mov     [esp+0Ch], esi
                call    Attrib_Instance ; Attrib::Instance::Instance((Attrib::Collection const *,ulong))
                mov     eax, [esi+8]
                test    eax, eax
                mov     dword ptr [esp+10h], 0
                jnz     loc_73EEFD
                push    98h ; '˜'
                call    Attrib_DefaultDataArea ; Attrib::DefaultDataArea((uint))
                add     esp, 4
                mov     [esi+8], eax

loc_73EEFD:                             ; CODE XREF: Attrib::Gen::fuelcell_emitter::fuelcell_emitter(Attrib::Collection const *,uint)+3B↑j
                //mov     ecx, [esp+8]
                mov     eax, esi
                pop     esi
                //mov     large fs:0, ecx
                add     esp, 10h
                retn    8
    }
}


char fuelcell_attrib_buffer[20];
void*(__thiscall* Attrib_Gen_fuelcell_emitter_constructor_Abstract)(void* AttribInstance, uint32_t unk1, uint32_t unk2) = (void*(__thiscall*)(void*, uint32_t, uint32_t)) & Attrib_Gen_fuelcell_emitter_constructor;
void* __stdcall Attrib_Gen_fuelcell_emitter_constructor_shim(uint32_t unk1, uint32_t unk2)
{
    uint32_t that;
    _asm mov that, ecx

    memset(fuelcell_attrib_buffer, 0, 20);
    auto result = Attrib_Gen_fuelcell_emitter_constructor_Abstract((void*)fuelcell_attrib_buffer, unk1, unk2);
    
    memcpy((void*)that, (void*)(&fuelcell_attrib_buffer[4]), 16);
    
    return result;
}

void __declspec(naked) sub_737610()
{
    _asm
    {
                //push    0FFFFFFFFh
                //push    offset SEH_737610
               // mov     eax, large fs:0
                //push    eax
                //mov     large fs:0, esp
                //push    ecx
                sub esp, 10h
                mov     eax, [esp+18h]
                push    esi
                mov     esi, ecx
                mov     ecx, [esp+18h]
                push 0
                push    eax
                push    ecx
                mov     ecx, esi
                mov     [esp+0Ch], esi
                call    Attrib_Instance_Refspec; Attrib::Instance::Instance((Attrib::RefSpec const &,ulong))
                mov     eax, [esi+8]
                test    eax, eax
                mov     dword ptr [esp+10h], 0
                jnz     loc_73765A
                push    10h
                call    Attrib_DefaultDataArea ; Attrib::DefaultDataArea((uint))
                add     esp, 4
                mov     [esi+8], eax

loc_73765A:                             ; CODE XREF: sub_737610+3B↑j
                //mov     ecx, [esp+8]
                mov     eax, esi
                pop     esi
                //mov     large fs:0, ecx
                add     esp, 10h
                retn    8
    }
}

char fuelcell_attrib_buffer2[20];

void* (__thiscall* sub_737610_Abstract)(void* AttribInstance, uint32_t unk1, uint32_t unk2) = (void* (__thiscall*)(void*, uint32_t, uint32_t)) & sub_737610;
void* __stdcall sub_737610_shim(uint32_t unk1, uint32_t unk2)
{
    uint32_t that;
    _asm mov that, ecx

    memset(fuelcell_attrib_buffer2, 0, 20);
    auto result = sub_737610_Abstract((void*)fuelcell_attrib_buffer2, unk1, unk2);

    memcpy((void*)that, (void*)(&fuelcell_attrib_buffer2[4]), 16);

    return result;
}

void __declspec(naked) CGEmitter_CGEmitter()
{
    _asm
    {
                 //push    0FFFFFFFFh
                 //push    offset SEH_73F0B0
                 //mov     eax, large fs:0
                 //push    eax
                 //mov     large fs:0, esp
                 //push    ecx
                 sub esp, 10h
                 mov     eax, [esp+14h]
                 push    ebx
                 push    esi
                 push    edi
                 push    0
                 mov     ebx, ecx
                 push    eax
                 mov     [esp+14h], ebx
                 call    Attrib_Gen_fuelcell_emitter_constructor_shim; Attrib::Gen::fuelcell_emitter::fuelcell_emitter(Attrib::Collection const *,uint)
                 mov     ecx, [ebx+4]
                 add     ecx, 60h ; '`'
                 push    0
                 push    ecx
                 lea     ecx, [ebx+10h]
                 mov     dword ptr [esp+20h], 0
                 call    sub_737610_shim
                 mov     eax, [esp+24h]
                 lea     esi, [eax+14h]
                 add     eax, 4
                 lea     edi, [ebx+30h]
                 mov     ecx, 10h
                 rep movsd
                 mov     ecx, [eax]
                 lea     edx, [ebx+20h]
                 mov     [edx], ecx
                 mov     ecx, [eax+4]
                 mov     [edx+4], ecx
                 mov     ecx, [eax+8]
                 mov     [edx+8], ecx
                 mov     eax, [eax+0Ch]
                 //mov     ecx, [esp+10h]
                 pop     edi
                 mov     [edx+0Ch], eax
                 pop     esi
                 mov     eax, ebx
                 pop     ebx
                 //mov     large fs:0, ecx
                 add     esp, 10h
                 retn    8
    }
}


char fuelcell_attrib_buffer3[20];
void __stdcall Attrib_Gen_fuelcell_effect_constructor(void* collection, unsigned int msgPort)
{
    uint32_t that;
    _asm mov that, ecx
    //_asm int 3

    memset(fuelcell_attrib_buffer3, 0, 20);

    Attrib_Instance_MW((void*)fuelcell_attrib_buffer3, collection, msgPort, NULL);

    memcpy((void*)that, (void*)(&fuelcell_attrib_buffer3[4]), 16);

    //Attrib_Instance_MW((void*)that, collection, msgPort, NULL);

    if (!*(uint32_t*)(that + 4))
        *(uint32_t*)(that + 4) = (uint32_t)Attrib_DefaultDataArea(1);
}

unsigned int __stdcall Attrib_Gen_fuelcell_effect_Num_NGEmitter()
{
    uint32_t that;
    _asm mov that, ecx

    uint32_t v1; // eax
    uint32_t v2; // esi
    char v4[16]; // [esp+4h] [ebp-1Ch] BYREF
    //int v5; // [esp+1Ch] [ebp-4h]

    memset(fuelcell_attrib_buffer3, 0, 20);
    memcpy(&(fuelcell_attrib_buffer3[4]), (void*)that, 16);

    v1 = (uint32_t)Attrib_Instance_Get(fuelcell_attrib_buffer3, (unsigned int)v4, 0xB0D98A89);
    //v5 = 0;
    v2 = (uint32_t)Attrib_Attribute_GetLength((void*)v1);
    //v5 = -1;

    return v2;
}

// copies the object back to MW's format to avoid crashing during Attrib garbage collection
char fuelcell_attrib_buffer4[20];
void __stdcall Attrib_Instance_Dtor_Shim()
{
    uint32_t that;
    _asm mov that, ecx

    memset(fuelcell_attrib_buffer4, 0, 20);
    memcpy(&(fuelcell_attrib_buffer4[4]), (void*)that, 16);
    memcpy((void*)that, fuelcell_attrib_buffer4, 20);

    Attrib_Instance_Dtor((void*)that);
}


void __declspec(naked) NGEffect_NGEffect()
{
    _asm
    {
                //push    0FFFFFFFFh
                //push    offset SEH_74A260
                //mov     eax, large fs:0
                //push    eax
                //mov     large fs:0, esp
                sub     esp, 84h
                push    ebp
                push    edi
                mov     edi, [esp+90h]
                mov     eax, [edi+54h]
                push    0
                mov     ebp, ecx
                push    eax
                mov     [esp+14h], ebp
                call    Attrib_Gen_fuelcell_effect_constructor ; Attrib::Gen::fuelcell_effect::fuelcell_effect(Attrib::Collection const *,uint)
                cmp     dword ptr [ebp+4], 0
                mov     dword ptr[esp + 88h], 0
                jz      loc_74A36E
                push    esi
                mov     ecx, ebp
                call    Attrib_Gen_fuelcell_effect_Num_NGEmitter ; Attrib::Gen::fuelcell_effect::Num_NGEmitter(void)
                xor     esi, esi
                test    eax, eax
                mov     [esp+0Ch], eax
                jle     loc_74A352
                lea     ecx, [ecx+0]

loc_74A2C0:                             ; CODE XREF: NGEffect::NGEffect(XenonEffectDef const &,float)+EC↓j
                push    esi
                push    0B0D98A89h
                mov     ecx, ebp
                //sub ecx, 4
                call    Attrib_Instance_GetAttributePointer_Shim
                test    eax, eax
                jnz     loc_74A2DB
                push    0Ch
                call    Attrib_DefaultDataArea
                add     esp, 4

loc_74A2DB:                             ; CODE XREF: NGEffect::NGEffect(XenonEffectDef const &,float)+6F↑j
                mov     ecx, eax
                call    Attrib_RefSpec_GetCollection
                push    edi
                push    eax
                lea     ecx, [esp+1Ch]
                call    CGEmitter_CGEmitter ; CGEmitter::CGEmitter(Attrib::Collection const *,XenonEffectDef const &)
                lea     ecx, [esp+14h]
                call fuelcell_emitter_bridge
                mov     eax, [edi+58h]
                test    eax, eax
                mov     byte ptr [esp+8Ch], 1
                jnz     loc_74A30B // jnz
                mov     ecx, [edi]
                mov     edx, [esp+98h]
                push    1
                push    ecx
                //push    3F800000h
                push    edx
                jmp     loc_74A31A
; ---------------------------------------------------------------------------

loc_74A30B:                             ; CODE XREF: NGEffect::NGEffect(XenonEffectDef const &,float)+9A↑j
                mov     eax, [esp+98h]
                push    0               ; char
                push    3F800000h       ; float
                push    eax             ; float

loc_74A31A:                             ; CODE XREF: NGEffect::NGEffect(XenonEffectDef const &,float)+A9↑j
                //lea     ecx, [esp+20h]
                //int 3
                //call fuelcell_emitter_bridge
                lea     ecx, [esp + 20h]
                call    CGEmitter_SpawnParticles ; CGEmitter::SpawnParticles(float,float)
                call fuelcell_emitter_bridge_restore
                lea     ecx, [esp+24h]
                mov     byte ptr [esp+8Ch], 2
                //sub ecx, 4
                call    Attrib_Instance_Dtor_Shim; Attrib::Instance::~Instance((void))
                lea     ecx, [esp+14h]
                mov     byte ptr [esp+8Ch], 0
                //sub ecx, 4
                call    Attrib_Instance_Dtor_Shim; Attrib::Instance::~Instance((void))
                mov     eax, [esp+0Ch]
                inc     esi
                cmp     esi, eax
                jl      loc_74A2C0

loc_74A352:                             ; CODE XREF: NGEffect::NGEffect(XenonEffectDef const &,float)+57↑j
                mov     eax, ebp
                pop     esi

loc_74A355:                             ; CODE XREF: NGEffect::NGEffect(XenonEffectDef const &,float)+110↓j
                //mov     ecx, [esp+8Ch+var_C]
                pop     edi
                pop     ebp
                //mov     large fs:0, ecx
                //int 3
                add     esp, 84h
                
                retn    8
loc_74A36E:
                mov     eax, ebp
                jmp     short loc_74A355
    }
}

void __declspec(naked) XSpriteManager_AddParticle()
{
    _asm
    {
        sub     esp, 1Ch
        push    ebx
        push    ebp
        mov     ebp, ecx
        push    esi
        mov     esi, [ebp + 0]
        push    2000h // Flags D3DLOCK_DISCARD
        lea     ecx, [esi + 1Ch]
        push    ecx // **ppbData
        mov     bl, 1
        mov[ebp + 4], bl
        mov     ecx, [esi + 0Ch]
        mov     eax, [esi]
        mov     edx, [eax]
        lea     ecx, [ecx + ecx * 2]
        shl     ecx, 5
        push    ecx // SizeToLock
        push    0 // OffsetToLock
        push    eax
        mov[esp + 20h], ebp
        call    dword ptr[edx + 2Ch]; DriverVertexBuffer::Lock
        mov     ecx, [esp + 34h]
        test    ecx, ecx
        mov     dword ptr[esi + 10h], 0
        mov[esi + 18h], bl
        jbe     loc_74EDF3
        push    edi
        mov     edi, [esp + 34h]
        lea     eax, [edi + 18h]
        mov[esp + 38h], ecx

        loc_74EC13 : ; CODE XREF : XSpriteManager::AddParticle(eView*, NGParticle const*, uint) + 22C↓j
        fld     dword ptr[eax + 1Ch]
        mov     edx, [ebp + 0]
        fld     st
        mov     esi, [edx + 10h]
        fmul    dword ptr[eax - 8]
        mov     ecx, [edx + 0Ch]
        cmp     esi, ecx
        fadd    dword ptr[edi]
        fstp    dword ptr[esp + 14h]
        fld     st
        fmul    dword ptr[eax - 4]
        fadd    dword ptr[eax - 14h]
        fstp    dword ptr[esp + 18h]
        fld     dword ptr[eax + 1Ch]
        fxch    st(1)
        fmul    dword ptr[eax]
        fadd    dword ptr[eax - 10h]
        fld     st(1)
        fmul    st, st(2)
        fmul    dword ptr[eax + 4]
        faddp   st(1), st
        fstp    dword ptr[esp + 1Ch]
        fstp    st
        jnb     loc_74EDDD
        mov     ebx, [edx + 1Ch]
        lea     ecx, [esi + esi * 2]
        shl     ecx, 5
        add     ecx, ebx
        lea     esi, [esi + 1]
        mov[edx + 10h], esi
        jz      loc_74EDDD
        mov     edx, [esp + 14h]
        mov     esi, [esp + 18h]
        mov     ebx, [esp + 1Ch]
        mov     ebp, ecx
        mov[ebp + 0], edx
        mov[ebp + 4], esi
        mov[ebp + 8], ebx
        mov     ebp, [eax - 0Ch]
        mov[ecx + 0Ch], ebp
        movzx   ebp, byte ptr[eax + 2Ch]
        mov[esp + 34h], ebp
        fild    dword ptr[esp + 34h]
        fmul    ds : flt_9C77C8
        fstp    dword ptr[ecx + 10h]
        movzx   ebp, byte ptr[eax + 2Dh]
        mov[esp + 34h], ebp
        fild    dword ptr[esp + 34h]
        fmul    ds : flt_9C77C8
        fstp    dword ptr[ecx + 14h]
        movzx   ebp, byte ptr[eax + 28h]
        mov[esp + 34h], ebp
        lea     ebp, [ecx + 18h]
        mov[ebp + 0], edx
        fild    dword ptr[esp + 34h]
        mov[ebp + 4], esi
        mov[ebp + 8], ebx
        fmul    ds : flt_9EAFFC
        fld     st
        fadd    dword ptr[ecx + 20h]
        fstp    dword ptr[ecx + 20h]
        mov     edx, [eax - 0Ch]
        mov[ecx + 24h], edx
        movzx   edx, byte ptr[eax + 2Eh]
        mov[esp + 34h], edx
        fild    dword ptr[esp + 34h]
        fmul    ds : flt_9C77C8
        fstp    dword ptr[ecx + 28h]
        movzx   edx, byte ptr[eax + 2Dh]
        mov[esp + 34h], edx
        fild    dword ptr[esp + 34h]
        fmul    ds : flt_9C77C8
        fstp    dword ptr[ecx + 2Ch]
        movzx   edx, byte ptr[eax + 29h]
        mov[esp + 34h], edx
        lea     edx, [ecx + 30h]
        mov     ebp, edx
        fild    dword ptr[esp + 34h]
        fmul    ds : flt_9EAFFC
        fadd    dword ptr[eax + 1Ch]
        fld     st
        fmul    dword ptr[eax - 8]
        fadd    dword ptr[edi]
        fstp    dword ptr[esp + 20h]
        mov     edx, [esp + 20h]
        fld     st
        fmul    dword ptr[eax - 4]
        fadd    dword ptr[eax - 14h]
        fstp    dword ptr[esp + 24h]
        mov     esi, [esp + 24h]
        fld     st
        fmul    dword ptr[eax]
        fadd    dword ptr[eax - 10h]
        fld     st(1)
        fmul    dword ptr[eax + 4]
        mov[ebp + 0], edx
        mov[ebp + 4], esi
        fmul    st, st(2)
        faddp   st(1), st
        fstp    dword ptr[esp + 28h]
        mov     ebx, [esp + 28h]
        mov[ebp + 8], ebx
        fstp    st
        fadd    dword ptr[ecx + 38h]
        fstp    dword ptr[ecx + 38h]
        mov     ebp, [eax - 0Ch]
        mov[ecx + 3Ch], ebp
        movzx   ebp, byte ptr[eax + 2Eh]
        mov[esp + 34h], ebp
        fild    dword ptr[esp + 34h]
        fmul    ds : flt_9C77C8
        fstp    dword ptr[ecx + 40h]
        movzx   ebp, byte ptr[eax + 2Fh]
        mov[esp + 34h], ebp
        lea     ebp, [ecx + 48h]
        mov[ebp + 0], edx
        fild    dword ptr[esp + 34h]
        mov[ebp + 4], esi
        mov[ebp + 8], ebx
        mov     ebp, [esp + 10h]
        fmul    ds : flt_9C77C8
        fstp    dword ptr[ecx + 44h]
        mov     edx, [eax - 0Ch]
        mov[ecx + 54h], edx
        movzx   edx, byte ptr[eax + 2Ch]
        mov[esp + 34h], edx
        fild    dword ptr[esp + 34h]
        fmul    ds : flt_9C77C8
        fstp    dword ptr[ecx + 58h]
        movzx   edx, byte ptr[eax + 2Fh]
        mov[esp + 34h], edx
        fild    dword ptr[esp + 34h]
        fmul    ds : flt_9C77C8
        fstp    dword ptr[ecx + 5Ch]

        loc_74EDDD : ; CODE XREF : XSpriteManager::AddParticle(eView*, NGParticle const*, uint) + 91↑j
        ; XSpriteManager::AddParticle(eView*, NGParticle const*, uint) + A8↑j
        mov     ecx, [esp + 38h]
        add     edi, 48h; 'H'
        add     eax, 48h; 'H'
        dec     ecx
        mov[esp + 38h], ecx
        jnz     loc_74EC13
        pop     edi

        loc_74EDF3 : ; CODE XREF : XSpriteManager::AddParticle(eView*, NGParticle const*, uint) + 41↑j
        mov     byte ptr[ebp + 4], 0
        mov     ebp, [ebp + 0]
        mov     al, [ebp + 18h]
        test    al, al
        jz      short loc_74EE0B
        cmp     dword ptr[ebp + 0], 0
        jz      short loc_74EE0B
        mov     byte ptr[ebp + 18h], 0

        loc_74EE0B:; CODE XREF : XSpriteManager::AddParticle(eView*, NGParticle const*, uint) + 23F↑j
        ; XSpriteManager::AddParticle(eView*, NGParticle const*, uint) + 245↑j
        mov     eax, [ebp + 0]
        mov     ecx, [eax]
        push    eax
        call    dword ptr[ecx + 30h]
        pop     esi
        mov     dword ptr[ebp + 1Ch], 0
        pop     ebp
        pop     ebx
        add     esp, 1Ch
        retn    0Ch
    }
}

void __declspec(naked) DrawXenonEmitters(void* eView)
{
    _asm
    {
        push    ebp
        mov     ebp, esp
        and esp, 0FFFFFFF8h
        sub     esp, 74h
        mov     eax, EmitterDeltaTime
        push    ebx
        push    esi
        mov     ecx, eax
        push    edi
        push    ecx; float
        mov     ecx, offset gParticleList
        mov[esp + 10h], eax
        mov     EmitterDeltaTime, 0
        call    ParticleList_AgeParticles
        mov     ebx, dword ptr gNGEffectList
        mov     edx, dword ptr gNGEffectList[4]
        cmp     ebx, edx
        jz      loc_754C78
        lea     esp, [esp + 0]

        loc_754C30:
        mov     ecx, 17h
        mov     esi, ebx
        lea     edi, [esp + 20h]
        rep movsd
        mov     eax, [esp + 78h]
        test    eax, eax
        jz      loc_754C4F
        mov     eax, [eax + 18h]
        shr     eax, 4
        test    al, 1
        jz      loc_754C71

        loc_754C4F:
        mov     ecx, [esp + 0Ch]
        push    ecx
        lea     edx, [esp + 24h]
        push    edx
        lea     ecx, [esp + 18h]
        call    NGEffect_NGEffect
        lea     ecx, [esp + 10h]
        call    Attrib_Instance_Dtor_Shim
        mov     edx, dword ptr gNGEffectList[4]

        loc_754C71:
        add     ebx, 5Ch; '\'
        cmp     ebx, edx
        jnz     loc_754C30

        loc_754C78 : ; CODE XREF : DrawXenonEmitters(eView*) + 3A↑j
        mov     eax, dword ptr gNGEffectList
        push    edx
        push    eax
        mov     ecx, offset gNGEffectList
        call    eastl_vector_erase_XenonEffectDef
        mov     eax, numParticles
        test    eax, eax
        jz      loc_754CA6
        mov     ecx, [ebp + 8]
        push    eax
        push    offset gParticleList
        push    ecx
        mov     ecx, offset NGSpriteManager
        call    XSpriteManager_AddParticle

        loc_754CA6:; CODE XREF : DrawXenonEmitters(eView*) + A0↑j
        pop     edi
        pop     esi
        pop     ebx
        mov     esp, ebp
        pop     ebp
        retn
    }
}
// RENDERER STUFF START
void __declspec(naked) sub_743DF0()
{
    _asm
    {
        push    ebx
        push    ebp
        mov     ebp, ds:[GLOBAL_D3DDEVICE]
        push    esi
        mov     esi, ecx
        push    edi
        mov     edi, [esp + 14h]
        xor eax, eax
        push    eax // pSharedHandle
        mov[esi + 10h], eax
        mov[esi + 14h], eax
        mov[esi + 18h], al
        mov     ecx, [edi]
        push    esi // ppVertexBuffer
        push    eax // Pool
        shl     ecx, 2
        mov[esi + 8], ecx
        mov     edx, [edi]
        push    eax // FVF
        mov[esi + 0Ch], edx
        mov     eax, [edi]
        mov     ecx, [ebp + 0]
        add     eax, 3
        lea     edx, [eax + eax * 2]
        push    208h // Usage D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY
        shl     edx, 5
        push    edx // Length
        push    ebp
        call    dword ptr[ecx + 68h] // CreateVertexBuffer
        mov     eax, [edi]
        mov     ecx, [ebp + 0]
        push    0 // pSharedHandle
        lea     ebx, [esi + 4]
        push    ebx // ppIndexBuffer
        push    1 // Pool
        push    65h // Format = D3DFMT_INDEX16
        lea     edx, [eax + eax * 2]
        push    8 // Usage = D3DUSAGE_WRITEONLY
        shl     edx, 2
        push    edx // Length
        push    ebp
        call    dword ptr[ecx + 6Ch] // CreateIndexBuffer
        mov     ebx, [ebx]
        mov     eax, [ebx]
        xor ebp, ebp
        push    ebp
        lea     ecx, [esp + 18h]
        push    ecx
        push    ebp
        push    ebp
        push    ebx
        call    dword ptr[eax + 2Ch]
        test    eax, eax
        jnz     loc_743ED3
        cmp[edi], ebp
        jbe     loc_743EBC
        xor ecx, ecx
        mov     eax, 6

        loc_743E71:; CODE XREF : sub_743DF0 + CA↓j
        mov     edx, [esp + 14h]
        mov[eax + edx - 6], cx
        mov     ebx, [esp + 14h]
        lea     edx, [ecx + 1]
        mov[eax + ebx - 4], dx
        mov     ebx, [esp + 14h]
        lea     edx, [ecx + 2]
        mov[eax + ebx - 2], dx
        mov     ebx, [esp + 14h]
        mov[eax + ebx], cx
        mov     ebx, [esp + 14h]
        mov[eax + ebx + 2], dx
        mov     ebx, [esp + 14h]
        lea     edx, [ecx + 3]
        mov[eax + ebx + 4], dx
        mov     edx, [edi]
        inc     ebp
        add     eax, 0Ch
        add     ecx, 4
        cmp     ebp, edx
        jb      loc_743E71

        loc_743EBC : ; CODE XREF : sub_743DF0 + 78↑j
        mov     eax, [esi + 4]
        mov     ecx, [eax]
        push    eax
        call    dword ptr[ecx + 30h]
        pop     edi
        mov     dword ptr[esi + 1Ch], 0
        pop     esi
        pop     ebp
        pop     ebx
        retn    4

        loc_743ED3:; CODE XREF : sub_743DF0 + 74↑j
        pop     edi
        mov[esi + 1Ch], ebp
        pop     esi
        pop     ebp
        pop     ebx
        retn    4
    }
}


LPDIRECT3DTEXTURE9 texMain;
const BYTE bPink[58] =
{
    0x42, 0x4D, 0x3A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x00, 0xFF, 0x00
};

void InitTex()
{
    if (bDebugTexture)
        D3DXCreateTextureFromFileInMemory(g_D3DDevice, (LPCVOID)&bPink, 58, &texMain);
    else if (D3DXCreateTextureFromFile(g_D3DDevice, "MAIN.dds", &texMain) != D3D_OK)
        D3DXCreateTextureFromFile(g_D3DDevice, "scripts\\MAIN.dds", &texMain);
}

struct SpriteManager
{
    IDirect3DVertexBuffer9* vertex_buffer;
    IDirect3DIndexBuffer9* index_buffer;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t vert_count;
};


struct eViewPlatInfo
{
    D3DXMATRIX m_mViewMatrix;
    D3DXMATRIX m_mProjectionMatrix;
    D3DXMATRIX m_mProjectionZBiasMatrix;
    D3DXMATRIX m_mViewProjectionMatrix;
    D3DXMATRIX m_mViewProjectionZBiasMatrix;
};

struct eView
{
    eViewPlatInfo* PlatInfo;
    uint32_t EVIEW_ID;
};


void __declspec(naked) InitializeRenderObj()
{
    _asm
    {
        sub esp, 4
        push    ecx
        lea     eax, [esp]
        push    eax
        mov     ecx, offset NGSpriteManager_ClassData
        mov     dword ptr[esp + 4], MAX_PARTICLES
        call    sub_743DF0
        push    0
        push    1
        push    TextureName
        call    bStringHash
        add     esp, 4
        push    eax
        call    GetTextureInfo
        mov     ecx, offset NGSpriteManager_ClassData
        add     ecx, 0x14
        mov     [ecx], eax
        mov ecx, ds:[GLOBAL_D3DDEVICE]
        mov g_D3DDevice, ecx
        xor al, al
        add     esp, 14h
        retn
    }
}

void __stdcall ReleaseRenderObj()
{
    SpriteManager* sm = (SpriteManager*)NGSpriteManager_ClassData;

    (*sm).vertex_buffer->Release();
    (*sm).index_buffer->Release();
}

void __stdcall XSpriteManager_DrawBatch(eView* view)
{
    SpriteManager* sm = (SpriteManager*)NGSpriteManager_ClassData;
    // init shader stuff here...
    uint32_t CurrentShaderObj = WORLDPRELITSHADER_OBJ_ADDR;
    if (bPassShadowMap)
        CurrentShaderObj = CURRENTSHADER_OBJ_ADDR;
    LPD3DXEFFECT effect = *(LPD3DXEFFECT*)(*(uint32_t*)CurrentShaderObj + 0x48);

    effect->Begin(NULL, 0);
    ParticleSetTransform((D3DXMATRIX*)0x00987AB0, view->EVIEW_ID);
    effect->BeginPass(0);

    g_D3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    if ((*sm).vert_count && gParticleList.mNumParticles)
    {
        //g_D3DDevice->SetFVF((D3DFVF_XYZRHW | D3DFVF_DIFFUSE)); -- don't touch FVF
        //uint32_t pStreamData = *(uint32_t*)(SpriteMgrBuffer);
        g_D3DDevice->SetStreamSource(0, (*sm).vertex_buffer, 0, 0x18);
        g_D3DDevice->SetIndices((*sm).index_buffer);
        // then shader stuff again... check SpriteMgrBuffer + 0x14 like in NFSC 0x00749BBB

        // this is a temporary hack until the real texture is loaded into memory
        g_D3DDevice->SetTexture(0, texMain);
        g_D3DDevice->SetRenderState(D3DRS_ALPHAREF, (DWORD)0x000000B0);
        g_D3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        g_D3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        g_D3DDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
        g_D3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        g_D3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
        // g_D3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA); -- Carbon does it wrong
        g_D3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);

        g_D3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

        effect->CommitChanges();

        g_D3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4 * (*sm).vert_count, 0, 2 * (*sm).vert_count);
    }
    g_D3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

    effect->EndPass();
    effect->End();
}

void __stdcall EmitterSystem_Render_Hook(eView* view)
{
    void* that;
    _asm mov that, ecx

    EmitterSystem_Render(that, view);
    if (*(uint32_t*)GAMEFLOWSTATUS_ADDR == 6)
    {
        DrawXenonEmitters(view);
        XSpriteManager_DrawBatch(view);
    }
    //printf("VertexBuffer: 0x%X\n", vertex_buffer);
}

// D3D Reset Hook
void __stdcall sub_6CFCE0_hook()
{
    sub_6CFCE0();
    InitializeRenderObj();
}

// RENDERER STUFF END

uint32_t SparkFC = 0;
void AddXenonEffect_Spark_Hook(void* piggyback_fx, void* spec, bMatrix4* mat, bVector4* vel, float intensity)
{
    if (!bLimitSparkRate)
        return AddXenonEffect_Abstract(piggyback_fx, spec, mat, vel, intensity);

    if ((SparkFC + SparkFrameDelay) <= eFrameCounter)
    {
        if (SparkFC != eFrameCounter)
        {
            SparkFC = eFrameCounter;
            AddXenonEffect_Abstract(piggyback_fx, spec, mat, vel, intensity);
        }
    }
}

uint32_t ContrailFC = 0;
void AddXenonEffect_Contrail_Hook(void* piggyback_fx, void* spec, bMatrix4* mat, bVector4* vel, float intensity)
{
#ifdef CONTRAIL_TEST
    // TEST CODE
    bVector4 newvel = { -40.6f, 29.3f, -2.3f, 0.0f };
    bMatrix4 newmat;

    newmat.v0.x = 0.60f;
    newmat.v0.y = 0.80f;
    newmat.v0.z = -0.03f;
    newmat.v0.w = 0.00f;

    newmat.v1.x = -0.80f;
    newmat.v1.y = 0.60f;
    newmat.v1.z = 0.01f;
    newmat.v1.w = 0.00f;

    newmat.v2.x = 0.03f;
    newmat.v2.y = 0.02f;
    newmat.v2.z = 1.00f;
    newmat.v2.w = 0.00f;

    newmat.v3.x = 981.90f;
    newmat.v3.y = 2148.45f;
    newmat.v3.z = 153.05f;
    newmat.v3.w = 1.00f;

    if (!bLimitContrailRate)
        AddXenonEffect_Abstract(piggyback_fx, spec, &newmat, &newvel, 1.0f);

    if ((ContrailFC + ContrailFrameDelay) <= eFrameCounter)
    {
        if (ContrailFC != eFrameCounter)
        {
            ContrailFC = eFrameCounter;
            AddXenonEffect_Abstract(piggyback_fx, spec, &newmat, &newvel, 1.0f);
        }
    }
    // TEST CODE
#else

    float newintensity = ContrailMaxIntensity;

    if (!bUseCGStyle)
    {
        double carspeed = ((sqrt((*vel).x * (*vel).x + (*vel).y * (*vel).y + (*vel).z * (*vel).z) - ContrailSpeed)) / ContrailSpeed;
        newintensity = std::lerp(ContrailMinIntensity, ContrailMaxIntensity, carspeed);
        if (newintensity > ContrailMaxIntensity)
            newintensity = ContrailMaxIntensity;
    }

    if (!bLimitContrailRate)
        return AddXenonEffect_Abstract(piggyback_fx, spec, mat, vel, newintensity);

    if ((ContrailFC + ContrailFrameDelay) <= eFrameCounter)
    {
        if (ContrailFC != eFrameCounter)
        {
            ContrailFC = eFrameCounter;
            AddXenonEffect_Abstract(piggyback_fx, spec, mat, vel, newintensity);
        }
    }
#endif
}


void __stdcall EmitterSystem_Update_Hook(float dt)
{
    uint32_t that;
    _asm mov that, ecx
    EmitterSystem_UpdateParticles((void*)that, dt);
    EmitterDeltaTime = dt;
    //printf("EmitterDeltaTime = %.2f\n", EmitterDeltaTime);
    //printf("FuelcellEmitterAddr = 0x%X\n", FuelcellEmitterAddr);
}

uint32_t RescueESI = 0;
void __declspec(naked) Emitter_SpawnParticles_Cave()
{
    _asm
    {
        //int 3
        mov ecx, [edi+0x7C]
        call sub_736EA0
        mov RescueESI, edi
        xor esi, esi
        test eax, eax
        mov ebx, eax
        jle loc_755CA6
    loc_D9D30:
        //int 3
        mov     ecx, [edi + 7Ch]
        push    esi
        push    0FE40E637h
        call    Attrib_Instance_GetAttributePointer
        test    eax, eax
        jnz     loc_D9D4C

        push    0Ch
        call    Attrib_DefaultDataArea
        add     esp, 4

    loc_D9D4C:
        mov     ecx, eax
        call    Attrib_RefSpec_GetCollection
        test    eax, eax
        jz      loc_D9D6F

        push    3F800000h
        lea     ecx, [edi+60h]
        push    ecx
        lea     edx, [edi+20h]
        push    edx
        push    eax
        mov     eax, [edi+8Ch]
        push    eax
        call    AddXenonEffect_Spark_Hook
        add     esp, 14h

    loc_D9D6F:
        //mov     eax, ebx
        inc     esi
        cmp     esi, ebx
        jl      loc_D9D30

    loc_755CA6:
        mov ecx, [esp+0x18]
        mov [edi+0x14], ecx
        pop edi
        pop esi
        pop ebx
        mov esp, ebp
        pop ebp
        mov esi, RescueESI
        retn 8
    }
}

// entry point: 0x750F48
// exit points: 0x750F6D & 0x750F4E

uint32_t loc_750F4E = 0x750F4E;
uint32_t loc_750F6D = 0x750F6D;
uint32_t loc_750FB0 = 0x750FB0;

void __declspec(naked) CarRenderConn_OnRender_Cave()
{
    _asm
    {
#ifndef CONTRAIL_TEST
                mov     al, [edi+400h]
                test    al, al
                jz      loc_7E13A5
#endif
                test    ebx, ebx
                jz      loc_7E140C
                mov     eax, [esi+8]
                cmp     eax, 1
                jz      short loc_7E1346
                cmp     eax, 2
                jnz     loc_7E13A5
; ---------------------------------------------------------------------------

loc_7E1346:                             ; CODE XREF: sub_7E1160+169↑j
                mov     edx, [ebx]
                mov     ecx, ebx
                call    dword ptr [edx+24h]
                test    al, al
                jz      short loc_7E13A5
                push    16AFDE7Bh
                push    6F5943F1h
                call    Attrib_FindCollection ; Attrib::FindCollection((ulong,ulong))
                add     esp, 8
                push    3F400000h       ; float
                mov     esi, eax
                mov     eax, [edi+38h]
                mov     ecx, [edi+34h]
                push    eax
                push    ecx

loc_7E1397:                             ; CODE XREF: sub_7E1160+1DD↑j
                push    esi
                push    0
                call    AddXenonEffect_Contrail_Hook ; AddXenonEffect(AcidEffect *,Attrib::Collection const *,UMath::Matrix4 const *,UMath::Vector4 const *,float)
                mov     esi, [ebp+8]
                add     esp, 14h

loc_7E13A5:
                cmp dword ptr [esi+4], 3
                jnz exitpoint_nz

                jmp loc_750F4E

exitpoint_nz:
                jmp loc_750F6D

loc_7E140C:
                jmp loc_750FB0
    }
}

void __stdcall CarRenderConn_UpdateContrails(void* CarRenderConn, void* PktCarService, float param)
{
    float* v3 = (float*)*((uint32_t*)CarRenderConn + 0xE);
    float v4 = sqrt(*v3 * *v3 + v3[1] * v3[1] + v3[2] * v3[2]);

    *((bool*)CarRenderConn + 0x400) = (
        (v3 = (float*)*((uint32_t*)CarRenderConn + 0xE),
        sqrt(*v3 * *v3 + v3[1] * v3[1] + v3[2] * v3[2]) >= ContrailSpeed)
        );

    if (*((bool*)PktCarService + 0x71) && bUseCGStyle)
        *((bool*)CarRenderConn + 0x400) = true;

    if (*(uint32_t*)NISINSTANCE_ADDR && !bNISContrails)
        *((bool*)CarRenderConn + 0x400) = false;

    if (*(bool*)((uint32_t)(CarRenderConn)+0x400))
        *(float*)((uint32_t)(CarRenderConn) + 0x404) = *(float*)((uint32_t)(PktCarService)+0x6C);
    else
        *(float*)((uint32_t)(CarRenderConn)+0x404) = 0;
}

void __stdcall CarRenderConn_UpdateEngineAnimation_Hook(float param, void* PktCarService)
{
    uint32_t that;
    _asm mov that, ecx

    CarRenderConn_UpdateEngineAnimation((void*)that, param, PktCarService);

    *(uint32_t*)(that + 0x400) = 0;
    *(uint32_t*)(that + 0x404) = 0;

    if ((*(char*)(that + 0x3F8) & 4) != 0)
    {
        CarRenderConn_UpdateContrails((void*)that, PktCarService, param);
    }
}

void InitConfig()
{
    mINI::INIFile inifile("NFSMW_XenonEffects.ini");
    mINI::INIStructure ini;
    inifile.read(ini);

    if (ini.has("MAIN"))
    {
        if (ini["MAIN"].has("DebugTexture"))
            bDebugTexture = std::stol(ini["MAIN"]["DebugTexture"]) != 0;
        if (ini["MAIN"].has("Contrails"))
            bContrails = std::stol(ini["MAIN"]["Contrails"]) != 0;
        if (ini["MAIN"].has("UseCGStyle"))
            bUseCGStyle = std::stol(ini["MAIN"]["UseCGStyle"]) != 0;
        if (ini["MAIN"].has("NISContrails"))
            bNISContrails = std::stol(ini["MAIN"]["NISContrails"]) != 0;
        if (ini["MAIN"].has("PassShadowMap"))
            bPassShadowMap = std::stol(ini["MAIN"]["PassShadowMap"]) != 0;
        if (ini["MAIN"].has("ContrailSpeed"))
            ContrailSpeed = std::stof(ini["MAIN"]["ContrailSpeed"]);
        if (ini["MAIN"].has("LimitContrailRate"))
            bLimitContrailRate = std::stol(ini["MAIN"]["LimitContrailRate"]) != 0;
        if (ini["MAIN"].has("LimitSparkRate"))
            bLimitSparkRate = std::stol(ini["MAIN"]["LimitSparkRate"]) != 0;
    }

    if (ini.has("Limits"))
    {
        if (ini["Limits"].has("ContrailTargetFPS"))
            ContrailTargetFPS = std::stof(ini["Limits"]["ContrailTargetFPS"]);
        if (ini["Limits"].has("SparkTargetFPS"))
            SparkTargetFPS = std::stof(ini["Limits"]["SparkTargetFPS"]);
        if (ini["Limits"].has("ContrailMinIntensity"))
            ContrailMinIntensity = std::stof(ini["Limits"]["ContrailMinIntensity"]);
        if (ini["Limits"].has("ContrailMaxIntensity"))
            ContrailMaxIntensity = std::stof(ini["Limits"]["ContrailMaxIntensity"]);
    }


    static float fGameTargetFPS = 1.0f / GetTargetFrametime();

    static float fContrailFrameDelay = (fGameTargetFPS / ContrailTargetFPS);
    ContrailFrameDelay = (uint32_t)round(fContrailFrameDelay);

    static float fSparkFrameDelay = (fGameTargetFPS / SparkTargetFPS);
    SparkFrameDelay = (uint32_t)round(fSparkFrameDelay);
}

int Init()
{
    NGSpriteManager[0] = (int)(&NGSpriteManager_ClassData);

    // delta time stealer
    injector::MakeCALL(0x0050D43C, EmitterSystem_Update_Hook, true);

    // temp. injection point in LoadGlobalChunks()
    injector::MakeCALL(0x006648BC, InitializeRenderObj, true);

    // render objects release
    injector::MakeCALL(0x006BD622, ReleaseRenderObj, true);

    // D3D reset inject
    injector::MakeCALL(0x006DB293, sub_6CFCE0_hook, true);

    // render inject, should be hooked to EmitterSystem::Render or somewhere after it
    injector::MakeCALL(0x006DF4BB, EmitterSystem_Render_Hook, true);

    // xenon effect inject at loc_50A5D7 for Emitter::SpawnParticles
    injector::MakeJMP(0x50A5D7, Emitter_SpawnParticles_Cave, true);

    // xenon effect inject at CarRenderConn::OnRender for contrails
    if (bContrails)
      injector::MakeJMP(0x750F48, CarRenderConn_OnRender_Cave, true); // ExOpts was the cause of the Debug Cam crash due to an ancient workaround.

    // update contrail inject
    injector::MakeCALL(0x00756629, CarRenderConn_UpdateEngineAnimation_Hook, true);

    // constructor for the NGEffectList
    //injector::MakeCALL(0x006D1DA3, bStringHash_Hook1, true);;
    injector::MakeCALL(0x0066616E, sub_739600_hook, true);


    // extend CarRenderConn by 8 bytes to accommodate a float and a bool
    injector::WriteMemory<uint32_t>(0x0075E6FC, 0x408, true);
    injector::WriteMemory<uint32_t>(0x0075E766, 0x408, true);

    //freopen("CON", "w", stdout);
    //freopen("CON", "w", stderr);

	return 0;
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
        InitConfig();
		Init();
	}
	return TRUE;
}

