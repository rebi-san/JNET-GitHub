
#ifndef	MIGRATORY_SH
#define MIGRATORY_SH

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef D_PLATFORM_D3D11x64
	#define D_PLATFORM_D3D11
#endif

#ifdef D_PLATFORM_D3D11

//#pragma pack_matrix(column_major)

#define	D_D3D11_OTHER(x,y)	x
#define	D_D3D11_ONLY(x)		x


#define	POSITION_OUT						SV_POSITION
#define	FINAL_COLOR		D_D3D11_OTHER(SV_TARGET0,COLOR)


#elif defined(D_PLATFORM_GNM)

#ifdef __ORBIS__
	#pragma warning (disable:5203) // parameter unreferenced
	#pragma warning (disable:5206) // local variable unreferenced
	#pragma warning (disable:6204) // does not support uniform default values
	#pragma warning (disable:6459) // __user_defined__ is not a valid semantic for uniform
	#pragma warning (disable:5581) // PSSL treats 'half' type as 'float'
	#pragma warning (disable:5609) // PSSL treats 'half' type as 'float'
	#pragma warning (disable:5583) // PSSL treats 'half' literals as 'float'
	#pragma warning (disable:5524) // unsupported compiler hint

	//! Some evil #defines to sort out matrix multiplies.
	#define float4x4 row_major float4x4
	#define float3x4 row_major float3x4
	#define float4x3 row_major float4x3

	//! Semantics for VS/PS inputs and outputs.
	#define POSITION S_POSITION
	#define SV_POSITION S_POSITION
	#define SV_GroupID S_GROUP_ID
	#define SV_VertexID S_VERTEX_ID
	#define SV_DispatchThreadID S_DISPATCH_THREAD_ID
	#define SV_GroupThreadID S_GROUP_THREAD_ID
	#define SV_GroupIndex S_GROUP_INDEX
	#define SV_PRIMITIVEID S_PRIMITIVE_ID
	#define SV_IsFrontFace S_FRONT_FACE
	#define SV_Depth FRAG_OUTPUT_DEPTH
	#define FRAG_OUTPUT_COLOR S_TARGET_OUTPUT0
	#define FRAG_OUTPUT_COLOR0 S_TARGET_OUTPUT0
	#define FRAG_OUTPUT_COLOR1 S_TARGET_OUTPUT1
	#define FRAG_OUTPUT_COLOR2 S_TARGET_OUTPUT2
	#define FRAG_OUTPUT_COLOR3 S_TARGET_OUTPUT3
	#define FRAG_OUTPUT_DEPTH S_DEPTH_OUTPUT

	//! Some defines to make textures compile with PSSL for now (sort out later)
	SamplerState globalSamplerState;
	SamplerComparisonState globalSamplerComparisonState
	{
		// Sampler state
		Filter = MIN_MAG_LINEAR_MIP_POINT;
		AddressU = CLAMP;
		AddressV = CLAMP;
		// Sampler comparison state
		ComparisonFunc = LESS;
		ComparisonFilter = COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	};

	#define cbuffer ConstantBuffer
	#define sampler SamplerState
	#define sampler2D Texture2D
	#define sampler3D Texture3D
	#define samplerCUBE TextureCube
	#define tex2Dproj(a,b) a.Sample(globalSamplerState, (b).xy / (b).w)
	#define h1tex2Dproj(a,b) ((half)(a.Sample(globalSamplerState, (b).xy / (b).w).x))
	#define h4tex2D(a,b) ((half4)a.Sample(globalSamplerState, (b).xy))
	#define h4tex2Dlod(a,b) ((half4)a.SampleLOD(globalSamplerState, (b).xy, (b).w))
	#define tex2D(a,b) a.Sample(globalSamplerState, (b).xy)

	#define SampleLevel SampleLOD
	#define SampleCmpLevelZero SampleCmpLOD0
	#define StructuredBuffer RegularBuffer
	#define RWStructuredBuffer RW_RegularBuffer
	#define numthreads NUM_THREADS
	#define maxvertexcount MAX_VERTEX_COUNT
	#define groupshared thread_group_memory
	#define TriangleStream TriangleBuffer
	#define PointStream PointBuffer
	#define triangle Triangle
	#define IncrementCounter IncrementCount
	#define DecrementCounter DecrementCount
	
	#define GroupMemoryBarrierWithGroupSync ThreadGroupMemoryBarrierSync
	#define InterlockedMin AtomicMin
	#define InterlockedMax AtomicMax
	#define InterlockedAdd AtomicAdd
#endif //! __ORBIS__
#if defined(PHYRE_D3DFX) && !defined(__ORBIS__)
	#define FRAG_OUTPUT_COLOR SV_TARGET
	#define FRAG_OUTPUT_COLOR0 SV_TARGET0
	#define FRAG_OUTPUT_COLOR1 SV_TARGET1
	#define FRAG_OUTPUT_COLOR2 SV_TARGET2
	#define FRAG_OUTPUT_COLOR3 SV_TARGET3
	#define FRAG_OUTPUT_DEPTH SV_DEPTH

	#define SYSTEM_PRIMITIVE_INDEX SV_PRIMITIVEID
#endif //! defined(PHYRE_D3DFX) && !defined(__ORBIS__)

#define	D_D3D11_OTHER(x,y)	x
#define	D_D3D11_ONLY(x)		x

#define	POSITION_OUT						POSITION
#define	FINAL_COLOR		FRAG_OUTPUT_COLOR

#elif !defined(_IOS64)

#define	D_D3D11_OTHER(x,y)	y
#define	D_D3D11_ONLY(x)

#define	HALF								float
#define	HALF2								float2
#define	HALF3								float3
#define	HALF4								float4
#define	HALF3x3							float3x3
#define	HALF3x4							float3x4
#define	HALF4x4							float4x4

#define	POSITION_OUT						POSITION
#define	FINAL_COLOR		D_D3D11_OTHER(SV_TARGET0,COLOR)


#endif

#if defined(__psp2__)
#define NATIVECOLOR __nativecolor
#define REGFORMAT __regformat
#else
#define NATIVECOLOR
#define REGFORMAT 
#endif



#ifndef _IOS64

#if !defined(__ORBIS__) && !defined(D_PLATFORM_GXM)

BlendState NoBlend_BlendState 
{
  BlendEnable[0] = FALSE;
};

BlendState Alpha_BlendState 
{
  BlendEnable[0] = TRUE;
	SrcBlend[0] = Src_Alpha;
	DestBlend[0] = Inv_Src_Alpha;
	BlendOp[0] = ADD;
	SrcBlendAlpha[0] = Src_Alpha;
	DestBlendAlpha[0] = Inv_Src_Alpha;
	BlendOpAlpha[0] = ADD;
	BlendEnable[1] = FALSE;
};

BlendState Additive_BlendState 
{
  BlendEnable[0] = TRUE;
	SrcBlend[0] = Src_Alpha;
	DestBlend[0] = ONE;
	BlendOp[0] = ADD;
	SrcBlendAlpha[0] = Src_Alpha;
	DestBlendAlpha[0] = ONE;
	BlendOpAlpha[0] = ADD;
	BlendEnable[1] = FALSE;
};



BlendState ParticleBlended_BlendState 
{
  BlendEnable[0] = TRUE;
	SrcBlend[0] = ONE;
	DestBlend[0] = Inv_Src_Alpha;
	BlendOp[0] = ADD;
	SrcBlendAlpha[0] = ONE;
	DestBlendAlpha[0] = Inv_Src_Alpha;
	BlendOpAlpha[0] = ADD;
	BlendEnable[1] = FALSE;
};

DepthStencilState  NoDepthTest_NoDepthWrite_DepthState 
{
  DepthEnable = FALSE;
  DepthWriteMask = Zero;
  DepthFunc = Less;
  StencilEnable = FALSE; 
};

DepthStencilState  NoDepthTest_DepthWrite_DepthState 
{
  DepthEnable = FALSE;
  DepthWriteMask = All;
  DepthFunc = Less;
  StencilEnable = FALSE; 
};

DepthStencilState  DepthTest_NoDepthWrite_DepthState 
{
  DepthEnable = TRUE;
  DepthWriteMask = Zero;
  DepthFunc = Less;
  StencilEnable = FALSE; 
};

DepthStencilState  DepthTest_DepthWrite_DepthState 
{
  DepthEnable = TRUE;
  DepthWriteMask = All;
  DepthFunc = Less;
  StencilEnable = FALSE; 
};

RasterizerState  NoCull_RasterizerState
{
	CullMode=None;
};

RasterizerState  BackFaceCull_RasterizerState
{
	CullMode=Back;
	FrontCounterClockwise = TRUE;
};

RasterizerState  FrontFaceCull_RasterizerState
{
	CullMode=Front;
	FrontCounterClockwise = TRUE;
};

#endif

#endif


#if defined(D_PLATFORM_D3D11) || defined(D_PLATFORM_GNM)	
#define MatrixMul(a,b) mul(b,a)

#define	HALF								float
#define	HALF2								float2
#define	HALF3								float3
#define	HALF4								float4
#define	HALF3x3								float3x3
#define	HALF3x4								float3x4
#define	HALF4x4								float4x4

#define half								float
#define half2								float2
#define half3								float3
#define half4								float4

#else
#define MatrixMul(a,b) mul(a,b)
#endif


// samplers

#if defined(D_PLATFORM_D3D11) || defined(D_PLATFORM_GNM)

sampler PointClampSampler : register(s8)
{
	Filter = Min_Mag_Mip_Point;
	AddressU = Clamp;
	AddressV = Clamp;
	AddressW = Clamp;
};

sampler LinearClampSampler : register(s9)
{
	Filter = Min_Mag_Mip_Linear;
	AddressU = Clamp;
	AddressV = Clamp;
	AddressW = Clamp;

};

sampler LinearWrapTSampler : register(s10)
{
	Filter = Min_Mag_Mip_Linear;
	AddressU = Clamp;
	AddressV = Wrap;
	AddressW = Clamp;
};

sampler LinearWrapSampler : register(s11)
{
	Filter = Min_Mag_Mip_Linear;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;
};

sampler PointWrapSampler : register(s12)
{
	Filter = Min_Mag_Mip_Point;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;	
};

sampler AnisoClampSampler : register(s13)
{
	Filter = ANISOTROPIC;
	AddressU = Clamp;
	AddressV = Clamp;
	AddressW = Clamp;	
	MaxAnisotropy = 16;	
};

sampler LinearWrapAnisoSampler : register(s14)
{
	Filter = ANISOTROPIC;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;	
	MaxAnisotropy = 16;	
};

sampler PointWrapAnisoSampler : register(s15)
{
	Filter = Min_Mag_Point_Mip_Linear;
	AddressU = Wrap;
	AddressV = Wrap;
	AddressW = Wrap;	
	MaxAnisotropy = 16;	
};

#define SAMPLER_WRAP_ANISO		LinearWrapAnisoSampler
#define SAMPLER_WRAP_LINEAR		LinearWrapSampler
#define SAMPLER_WRAP_POINT		PointWrapSampler

#define SAMPLER_CLAMP_ANISO		AnisoClampSampler
#define SAMPLER_CLAMP_LINEAR	LinearClampSampler
#define SAMPLER_CLAMP_POINT		PointClampSampler

static const float kSRGB = 2.2f;

#define SRGB_ON(x)				pow(x,kSRGB)
#define SRGB_OFF(x)				x

#define SAMPLETEX(tex,uv,filter,wrap,srgb)		SRGB_##srgb(tex.Sample(SAMPLER_##wrap##_##filter, uv))



#ifdef __ORBIS__
#define	D_DECLARE_TEXTURE2DMS_4(x)						MS_Texture2D<float4> x
#else
#define	D_DECLARE_TEXTURE2DMS_4(x)						Texture2DMS<float4,4> x
#endif
#define	D_DECLARE_TEXTURE2D_4(x)						Texture2D<float4> x
#define	D_DECLARE_TEXTURE3D_4(x)						Texture3D<float4> x
#define	D_DECLARE_TEXTURE_CUBE_4(x)						TextureCube<float4> x

#define	D_SAMPLE_TEXTURE(tex,uv)						tex.Sample(LinearClampSampler,uv)
#define	D_POINT_SAMPLE_TEXTURE(tex,uv)					tex.Sample(PointClampSampler,uv)
#define	D_SAMPLE_TEXTURE_PROJ(tex,uv)					tex.Sample(LinearClampSampler,(float2) (uv.xy / uv.w) )
#define	D_POINT_SAMPLE_TEXTURE_PROJ(tex,uv)				tex.Sample(PointClampSampler,(float2) (uv.xy / uv.w) )
#define	D_SAMPLE_TEXTURE_3D(tex,uv)						tex.Sample(LinearClampSampler,uv)
#define	D_SAMPLE_TEXTURE_WRAP_3D(tex,uv)				tex.Sample(LinearWrapSampler,uv)


#define	D_H1_SAMPLE_TEXTURE(tex,uv)						tex.Sample(LinearClampSampler,uv).x
#define	D_H2_SAMPLE_TEXTURE(tex,uv)						tex.Sample(LinearClampSampler,uv).xy
#define	D_H3_SAMPLE_TEXTURE(tex,uv)						tex.Sample(LinearClampSampler,uv).xyz
#define	D_H4_SAMPLE_TEXTURE(tex,uv)						tex.Sample(LinearClampSampler,uv)

#define	D_H1_POINTSAMPLELEVEL_TEXTURE(tex,uv,lod)		tex.SampleLevel(PointClampSampler,uv,lod).x
#define	D_H1_SAMPLELEVEL_TEXTURE(tex,uv,lod)			tex.SampleLevel(LinearClampSampler,uv,lod).x
#define	D_H4_SAMPLELEVEL_TEXTURE(tex,uv,lod)			tex.SampleLevel(LinearClampSampler,uv,lod)

#if defined(D_PLATFORM_D3D11) 
#define	D_H1_SAMPLE_SRGBTEXTURE(tex,uv)					pow(tex.Sample(LinearClampSampler,uv).x,2.2f)
#define	D_H2_SAMPLE_SRGBTEXTURE(tex,uv)					pow(tex.Sample(LinearClampSampler,uv).xy,2.2f)
#define	D_H3_SAMPLE_SRGBTEXTURE(tex,uv)					pow(tex.Sample(LinearClampSampler,uv).xyz,2.2f)
#define	D_H4_SAMPLE_SRGBTEXTURE(tex,uv)					float4( pow(tex.Sample(LinearClampSampler,uv).xyz,2.2f), tex.Sample(LinearClampSampler,uv).a )
#define	D_H1_SAMPLE_WRAP_SRGBTEXTURE(tex,uv)			pow(tex.Sample(LinearWrapSampler,uv).x,2.2f)
#define	D_H2_SAMPLE_WRAP_SRGBTEXTURE(tex,uv)			pow(tex.Sample(LinearWrapSampler,uv).xy,2.2f)
#define	D_H3_SAMPLE_WRAP_SRGBTEXTURE(tex,uv)			pow(tex.Sample(LinearWrapSampler,uv).xyz,2.2f)
#define	D_H4_SAMPLE_WRAP_SRGBTEXTURE(tex,uv)			float4( pow(tex.Sample(LinearWrapSampler,uv).xyz,2.2f), tex.Sample(LinearWrapSampler,uv).a )

#define	D_H4_SAMPLE_WRAPT_SRGBTEXTURE(tex,uv)			float4( pow(tex.Sample(LinearWrapTSampler,uv).xyz,2.2f), tex.Sample(LinearWrapTSampler,uv).a)

#define	D_H3_SAMPLE_WRAP_ANISO_SRGBTEXTURE(tex,uv)			pow( tex.Sample(LinearWrapAnisoSampler,uv).xyz, 2.2f)

#else
#define	D_H1_SAMPLE_SRGBTEXTURE(tex,uv)					tex.Sample(LinearClampSampler,uv).x
#define	D_H2_SAMPLE_SRGBTEXTURE(tex,uv)					tex.Sample(LinearClampSampler,uv).xy
#define	D_H3_SAMPLE_SRGBTEXTURE(tex,uv)					tex.Sample(LinearClampSampler,uv).xyz
#define	D_H4_SAMPLE_SRGBTEXTURE(tex,uv)					tex.Sample(LinearClampSampler,uv)
#define	D_H1_SAMPLE_WRAP_SRGBTEXTURE(tex,uv)			tex.Sample(LinearWrapSampler,uv).x
#define	D_H2_SAMPLE_WRAP_SRGBTEXTURE(tex,uv)			tex.Sample(LinearWrapSampler,uv).xy
#define	D_H3_SAMPLE_WRAP_SRGBTEXTURE(tex,uv)			tex.Sample(LinearWrapSampler,uv).xyz
#define	D_H4_SAMPLE_WRAP_SRGBTEXTURE(tex,uv)			tex.Sample(LinearWrapSampler,uv)

#define	D_H4_SAMPLE_WRAPT_SRGBTEXTURE(tex,uv)			tex.Sample(LinearWrapTSampler,uv)

#endif

#define	D_H1_SAMPLE_WRAP_TEXTURE(tex,uv)				tex.Sample(LinearWrapSampler,uv).x
#define	D_H2_SAMPLE_WRAP_TEXTURE(tex,uv)				tex.Sample(LinearWrapSampler,uv).xy
#define	D_H3_SAMPLE_WRAP_TEXTURE(tex,uv)				tex.Sample(LinearWrapSampler,uv).xyz
#define	D_H4_SAMPLE_WRAP_TEXTURE(tex,uv)				tex.Sample(LinearWrapSampler,uv)
#define	D_H1_SAMPLE_WRAP_ANISO_TEXTURE(tex,uv)			tex.Sample(LinearWrapAnisoSampler,uv).x
#define	D_H2_SAMPLE_WRAP_ANISO_TEXTURE(tex,uv)			tex.Sample(LinearWrapAnisoSampler,uv).xy
#define	D_H3_SAMPLE_WRAP_ANISO_TEXTURE(tex,uv)			tex.Sample(LinearWrapAnisoSampler,uv).xyz
#define	D_H4_SAMPLE_WRAP_ANISO_TEXTURE(tex,uv)			tex.Sample(LinearWrapAnisoSampler,uv)
#define	D_H1_SAMPLE_POINT_WRAP_ANISO_TEXTURE(tex,uv)	tex.Sample(PointWrapAnisoSampler,uv).x
#define	D_H2_SAMPLE_POINT_WRAP_ANISO_TEXTURE(tex,uv)	tex.Sample(PointWrapAnisoSampler,uv).xy
#define	D_H3_SAMPLE_POINT_WRAP_ANISO_TEXTURE(tex,uv)	tex.Sample(PointWrapAnisoSampler,uv).xyz
#define	D_H4_SAMPLE_POINT_WRAP_ANISO_TEXTURE(tex,uv)	tex.Sample(PointWrapAnisoSampler,uv)

#define	D_H2_SAMPLE_TEXTURE_3D(tex,uv)					tex.Sample(LinearClampSampler,uv).xy
#define	D_H2_SAMPLE_WRAP_TEXTURE_3D(tex,uv)				tex.Sample(LinearWrapSampler,uv).xy


#if defined(D_PLATFORM_D3D11) 
#define	D_H3_SAMPLE_TEXTURE_CUBE(tex,uv)				pow(tex.Sample(LinearClampSampler,uv).xyz,2.2f)
#define	D_H4_SAMPLE_TEXTURE_CUBE(tex,uv)				float4( pow(tex.Sample(LinearClampSampler,uv).xyz,2.2f), tex.Sample(LinearClampSampler,uv).a )
#define	D_SAMPLE_TEXTURE_CUBE(tex,uv)					float4( pow(tex.Sample(LinearClampSampler,uv).xyz,2.2f), tex.Sample(LinearClampSampler,uv).a )
#else
#define	D_H3_SAMPLE_TEXTURE_CUBE(tex,uv)				tex.Sample(LinearClampSampler,uv).xyz
#define	D_H4_SAMPLE_TEXTURE_CUBE(tex,uv)				tex.Sample(LinearClampSampler,uv)
#define	D_SAMPLE_TEXTURE_CUBE(tex,uv)					tex.Sample(LinearClampSampler,uv)
#endif

#else

#define	D_DECLARE_TEXTURE2D_4(x)						sampler2D x
#define	D_DECLARE_TEXTURE3D_4(x)						sampler3D x
#define	D_DECLARE_TEXTURE_CUBE_4(x)						samplerCUBE x

#define	D_SAMPLE_TEXTURE(tex,uv)						h4tex2D(tex,uv)
#define	D_POINT_SAMPLE_TEXTURE(tex,uv)					h4tex2D(tex,uv)
#define	D_SAMPLE_TEXTURE_PROJ(tex,uv)					h4tex2Dproj(tex,uv.xyw)
#define	D_POINT_SAMPLE_TEXTURE_PROJ(tex,uv)				h4tex2Dproj(tex,uv.xyw)
#define	D_SAMPLE_TEXTURE_CUBE(tex,uv)					h4texCUBE(tex,uv)
#define	D_SAMPLE_TEXTURE_3D(tex,uv)						h4tex3D(tex,uv)
#define	D_SAMPLE_TEXTURE_WRAP_3D(tex,uv)				h4tex3D(tex,uv)

#define	D_H1_SAMPLE_TEXTURE(tex,uv)						h1tex2D(tex,uv).x
#define	D_H2_SAMPLE_TEXTURE(tex,uv)						h2tex2D(tex,uv).xy
#define	D_H3_SAMPLE_TEXTURE(tex,uv)						h3tex2D(tex,uv).xyz
#define	D_H4_SAMPLE_TEXTURE(tex,uv)						h4tex2D(tex,uv)

#define	D_H1_POINTSAMPLELEVEL_TEXTURE(tex,uv,lod)		h1tex2D(tex,uv).x
#define	D_H1_SAMPLELEVEL_TEXTURE(tex,uv,lod)			h1tex2D(tex,uv).x
#define	D_H4_SAMPLELEVEL_TEXTURE(tex,uv,lod)			h4tex2D(tex,uv)

#define	D_H1_SAMPLE_SRGBTEXTURE(tex,uv)					h1tex2D(tex,uv).x
#define	D_H2_SAMPLE_SRGBTEXTURE(tex,uv)					h2tex2D(tex,uv).xy
#define	D_H3_SAMPLE_SRGBTEXTURE(tex,uv)					h3tex2D(tex,uv).xyz
#define	D_H4_SAMPLE_SRGBTEXTURE(tex,uv)					h4tex2D(tex,uv)

#define	D_H1_SAMPLE_WRAP_TEXTURE(tex,uv)				h1tex2D(tex,uv).x		
#define	D_H2_SAMPLE_WRAP_TEXTURE(tex,uv)				h2tex2D(tex,uv).xy
#define	D_H3_SAMPLE_WRAP_TEXTURE(tex,uv)				h3tex2D(tex,uv).xyz
#define	D_H4_SAMPLE_WRAP_TEXTURE(tex,uv)				h4tex2D(tex,uv)

#define	D_H1_SAMPLE_WRAP_ANISO_TEXTURE(tex,uv)			h1tex2D(tex,uv).x		
#define	D_H2_SAMPLE_WRAP_ANISO_TEXTURE(tex,uv)			h2tex2D(tex,uv).xy
#define	D_H3_SAMPLE_WRAP_ANISO_TEXTURE(tex,uv)			h3tex2D(tex,uv).xyz
#define	D_H4_SAMPLE_WRAP_ANISO_TEXTURE(tex,uv)			h4tex2D(tex,uv)

#define	D_H1_SAMPLE_ANISO_TEXTURE(tex,uv)				h1tex2D(tex,uv).x		
#define	D_H2_SAMPLE_ANISO_TEXTURE(tex,uv)				h2tex2D(tex,uv).xy
#define	D_H3_SAMPLE_ANISO_TEXTURE(tex,uv)				h3tex2D(tex,uv).xyz
#define	D_H4_SAMPLE_ANISO_TEXTURE(tex,uv)				h4tex2D(tex,uv)

#define	D_H1_SAMPLE_POINT_WRAP_ANISO_TEXTURE(tex,uv)	h1tex2D(tex,uv).x		
#define	D_H2_SAMPLE_POINT_WRAP_ANISO_TEXTURE(tex,uv)	h2tex2D(tex,uv).xy
#define	D_H3_SAMPLE_POINT_WRAP_ANISO_TEXTURE(tex,uv)	h3tex2D(tex,uv).xyz
#define	D_H4_SAMPLE_POINT_WRAP_ANISO_TEXTURE(tex,uv)	h4tex2D(tex,uv)

#define	D_H1_SAMPLE_WRAP_SRGBTEXTURE(tex,uv)			h1tex2D(tex,uv).x
#define	D_H2_SAMPLE_WRAP_SRGBTEXTURE(tex,uv)			h2tex2D(tex,uv).xy
#define	D_H3_SAMPLE_WRAP_SRGBTEXTURE(tex,uv)			h3tex2D(tex,uv).xyz
#define	D_H4_SAMPLE_WRAP_SRGBTEXTURE(tex,uv)			h4tex2D(tex,uv)

#define	D_H4_SAMPLE_WRAPT_SRGBTEXTURE(tex,uv)			h4tex2D(tex,uv)


#define	D_H2_SAMPLE_TEXTURE_3D(tex,uv)					h2tex3D(tex,uv).xy
#define	D_H2_SAMPLE_WRAP_TEXTURE_3D(tex,uv)				h2tex3D(tex,uv).xy
#define	D_H3_SAMPLE_TEXTURE_CUBE(tex,uv)				h3texCUBE(tex,uv).xyz
#define	D_H4_SAMPLE_TEXTURE_CUBE(tex,uv)				h4texCUBE(tex,uv)

#endif

#ifdef _IOS64

struct UniformGlobal
{
    float4x4    view;
    float4x4    projection;
    float4x4    viewprojection;
    float4      eyeDirectionWS;
    float4      eyePositionWS;

    // depth conversions
    float cameraNear;
    float cameraNearFar;
    float cameraFarMinusNear;
    float cameraFar;

    // lighting
    float4 l_d;//dir
    float4 l_c;//dirColor
    float4 l_s;//spec
    float4 l_a;//amb

    // fog
    float4 f_se;       // fogStart/fogEnd
    float4 f_cc, f_fc; // fogCloseColor, fogFarColor

    // velvet factors
    float velvetInt;
    float velvetRatio;

    // For converting from Z buffer value to linear depth.
    float depthBias;
    float depthScale;

    float lodBias;

#ifdef D_ENABLE_DEBUG_PARAMS
    float4 debugVars0;
    float4 debugVars1;
    float4 debugVars2;
    float4 debugVars3;
    float4 debugVars4;
    float4 debugVars5;
    float4 debugVars6;
    float4 debugVars7;
#endif
};

#define GET_GLOBAL(xxx) UG.xxx
#define GET_LOCAL(xxx) UL.xxx

#else

#define GET_GLOBAL(xxx) xxx
#define GET_LOCAL(xxx) xxx

#if !defined(GLOBAL_LEAN_AND_MEAN)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// these constants are system constants; uploaded by Phyre itself.
// we can't do this, but we don't want to upload them all the time, either. consequently, we make these global
// i guess we can eventually stuff these into a separate uniform buffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	float4x4	view				: view				;		// uploaded once per camera
	float4x4	projection			: projection		;					// view inverse
	float4x4	viewprojection		: viewprojection	;
	float4		eyeDirectionWS		: eyeDirectionWS	;
	float4		eyePositionWS		: eyePositionWS	;
	float4		eyePositionOS		: eyePositionOS		;
	float4		cameraNear			: cameraNearFar		;
	float4x4	modelIT				: modelit			;	// uploaded per primitive		// model inverse transpose
	float4x4	model				: model				;
	float4x4	modelViewProj		: modelviewproj		;
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// depth conversion code, stolen from the sample code
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	float		cameraNearTimesFar;
	float		cameraFarMinusNear;
	float4		cameraNearFar;
#endif

#ifndef _IOS64

// Convert a depth value from post projection space to view space. 
inline float ConvertDepth(
    UniformGlobal UG,
    float depth)
{	
//	float viewSpaceZ = -(scene.cameraNearTimesFar / (depth * scene.cameraFarMinusNear - scene.cameraNearFar.y));
#if defined(D_PLATFORM_GXM)
	float viewSpaceZ = -(cameraNearTimesFar / (depth * cameraFarMinusNear - cameraNearFar.y));
#else
	float viewSpaceZ = -(GET_GLOBAL(cameraNearTimesFar) / (depth * GET_GLOBAL(cameraFarMinusNear) - GET_GLOBAL(cameraNearFar.y)));
#endif
	return viewSpaceZ;
}

#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// skinning
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef SKINNING_ENABLED

// grabbed off [PhyreCoreShaderShared.h]

// Description:
// Encapsulates the skinning transformation matrices.

#if defined(D_PLATFORM_GXM)

#define PE_MAX_SKIN_CB_TRANSFORMS 32

float4x4 boneTransforms[PE_MAX_SKIN_CB_TRANSFORMS];

float4x4 GenerateBoneMatrix(int4 SkinIndices,float4 SkinWeights)
{
	return	boneTransforms[SkinIndices[0]]*SkinWeights[0]
		+	boneTransforms[SkinIndices[1]]*SkinWeights[1]
		+	boneTransforms[SkinIndices[2]]*SkinWeights[2]
		+	boneTransforms[SkinIndices[3]]*SkinWeights[3];
}

#elif defined(_IOS64)


#else

#define PE_MAX_SKIN_CB_TRANSFORMS 32

struct PSkinTransforms
{
	float4x4 Mtxs[PE_MAX_SKIN_CB_TRANSFORMS];	// The skinning transformation matrices.
};

cbuffer BoneTransformConstantBuffer
{
	PSkinTransforms SkinTransforms; 
}

float4x4 GenerateBoneMatrix(uint4 SkinIndices,float4 SkinWeights)
{
	return	SkinTransforms.Mtxs[SkinIndices[0]]*SkinWeights[0]
		+	SkinTransforms.Mtxs[SkinIndices[1]]*SkinWeights[1]
		+	SkinTransforms.Mtxs[SkinIndices[2]]*SkinWeights[2]
		+	SkinTransforms.Mtxs[SkinIndices[3]]*SkinWeights[3];
}

#endif

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _IOS64

static const float4x4 SCALE_BIAS = float4x4(
	float4( 0.5, 0.0, 0.0, 0.5 ),
#if defined(D_PLATFORM_D3D11) 
	float4( 0.0, -0.5, 0.0, 0.5 ),
#else
	float4( 0.0, 0.5, 0.0, 0.5 ),
#endif
	
	float4( 0.0, 0.0, 0.5, 0.5 ),
	float4( 0.0, 0.0, 0.0, 1.0 )
	);

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef D_PLATFORM_GXM
	#define D_INT_MOD(val,mod)		(frac((floor((float)val)/mod))*mod)
	#define D_UINT_MOD(val,mod)		(frac((floor((float)val)/mod))*mod)
#elif defined(D_PLATFORM_GNM)
	#define D_INT_MOD(val,mod)		((int)floor(val) % mod)
	#define D_UINT_MOD(val,mod)		((uint)floor(val) % mod)
#elif defined(_IOS64)
    #define D_INT_MOD(val,mod)        (frac((floor((float)val)/mod))*mod)
    #define D_UINT_MOD(val,mod)       (frac((floor((float)val)/mod))*mod)
#else
	#define D_INT_MOD(val,mod)		((int)floor(val) % mod)
	#define D_UINT_MOD(val,mod)		((int)floor(val) % mod)
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef D_ENABLE_DEBUG_PARAMS

	float4 debugVars0;
	float4 debugVars1;
	float4 debugVars2;
	float4 debugVars3;
	float4 debugVars4;
	float4 debugVars5;
	float4 debugVars6;
	float4 debugVars7;

	#define GETDEBUGVAR(dv)			(debugVars##dv!=0) ? (debugVars##dv) : 
	#define GETDEBUGVAR2(dv,scl)	(debugVars##dv!=0) ? (scl*debugVars##dv) : 

#endif

#define powsafe(_x,_y) pow(max(_x,0.00000000000001),_y)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif//MIGRATORY_SH
