/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.6.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include "PhyreShaderPlatform.h"
#include "PhyreShaderCommonD3D.h"

// An implementation of the default COLLADA shader - for Maya lamberts.

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The maximum number of lights this shader supports.
#define MAX_NUM_LIGHTS 3

#ifndef __ORBIS__
// Switches. 
// Context switches
bool PhyreContextSwitches 
< 
string ContextSwitchNames[] = {"NUM_LIGHTS", "LOD_BLEND"}; 
int MaxNumLights = MAX_NUM_LIGHTS; 
string SupportedLightTypes[] = {"DirectionalLight","PointLight","SpotLight"};
string SupportedShadowTypes[] = {"PCFShadowMap", "CascadedShadowMap", "CombinedCascadedShadowMap"};
>;

// Material switch definitions. These are the material switches this shader exposes.
bool PhyreMaterialSwitches 
< 
string MaterialSwitchNames[] = {"VERTEX_COLOR_ENABLED", "TEXTURE_ENABLED","NORMAL_MAPPING_ENABLED", "SPECULAR_ENABLED", "CASTS_SHADOWS", "ALPHA_ENABLED", "RECEIVE_SHADOWS"}; 
string MaterialSwitchUiNames[] = {"Enable Vertex Color", "Enable Texture",  "Enable Normal Mapping", "Enable Specular", "Casts Shadows", "Alpha Enabled", "Receive Shadows"}; 
>;
#endif //! __ORBIS__

#include "PhyreSceneWideParametersD3D.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Preprocessing of switches. 
// Setup macros for common combinations of switches
#ifdef NUM_LIGHTS
#if NUM_LIGHTS > MAX_NUM_LIGHTS
#error Maximum number of supported lights exceeded.
#endif
#else
#define NUM_LIGHTS 0
#endif

#if defined(NORMAL_MAPPING_ENABLED)
#define USE_TANGENTS
#endif

#if defined(ALPHA_ENABLED) && defined(TEXTURE_ENABLED)
#define SHADOW_TEXTURE_SAMPLING_ENABLED
#define ZPREPASS_TEXTURE_SAMPLING_ENABLED
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global shader parameters.

// Un-tweakables
float4x4 World					: World;		
float4x4 WorldView				: WorldView;		
float4x4 WorldInverse			: WorldInverse;
float4x4 WorldViewProjection	: WorldViewProjection;	
float4x4 WorldViewInverse		: WorldViewInverse;		

#ifdef SKINNING_ENABLED
#define NUM_SKIN_TRANSFORMS 80 // Note: This number is mirrored in Core as PD_MATERIAL_SKINNING_MAX_GPU_BONE_COUNT
float4x4 BoneTransforms[NUM_SKIN_TRANSFORMS] : BONETRANSFORMS;
#endif

// Material Parameters
float4 MaterialColor : MATERIALCOLOR = float4(1.0f,1.0f,1.0f,1.0f);
float4 MaterialTransparency : MATERIALTRANSPARENCY = float4(0,0,0,0);
float4 MaterialAmbient : MATERIALAMBIENT = float4(0,0,0,0);
float4 MaterialEmission : MATERIALEMISSION = float4(0,0,0,0);
float4 MaterialDiffuse : MATERIALDIFFUSE = float4(1,1,1,1);

#ifdef SPECULAR_ENABLED
float4 MaterialSpecular : MATERIALSPECULAR = float4(2,2,2,2);

float SpecularPower : SpecularPower = 32.0f;
float FresnelPower : FresnelPower = 1.0f;
#endif // SPECULAR_ENABLED

#if defined(TEXTURE_ENABLED) || defined(NORMAL_MAPPING_ENABLED)
// UV animation
float TextureOffsetU : TEXTUREOFFSETU = 0.0f;
float TextureOffsetV : TEXTUREOFFSETV = 0.0f;
float TextureScaleU	 : TEXTURESCALEU = 1.0f;
float TextureScaleV	 : TEXTURESCALEV = 1.0f;
float AlphaGain : ALPHAGAIN = 1.0f;
#endif //! defined(TEXTURE_ENABLED) || defined(NORMAL_MAPPING_ENABLED)

// Engine-supplied lighting parameters

#if NUM_LIGHTS > 0
	#ifdef __ORBIS__
		LIGHTTYPE_0 Light0;
	#else //! __ORBIS__
		LIGHTTYPE_0 Light0 : LIGHT0;
	#endif //! __ORBIS__
	#ifndef SHADOWTYPE_0
		#define LightShadow0 0.0f
		#define LightShadowMap0 0.0f
	#else
		#ifdef __ORBIS__
			SHADOWTYPE_0 LightShadow0;
			Texture2D <float> LightShadowMap0;
		#else //! __ORBIS__
			SHADOWTYPE_0 LightShadow0 : LIGHTSHADOW0;
			Texture2D <float> LightShadowMap0 : LIGHTSHADOWMAP0;
		#endif //! __ORBIS__
	#endif
#endif

#if NUM_LIGHTS > 1
	#ifdef __ORBIS__
		LIGHTTYPE_1 Light1;
	#else //! __ORBIS__
		LIGHTTYPE_1 Light1 : LIGHT1;
	#endif //! __ORBIS__
	#ifndef SHADOWTYPE_1
		#define LightShadow1 0.0f
		#define LightShadowMap1 0.0f
	#else
		#ifdef __ORBIS__
			SHADOWTYPE_1 LightShadow1;
			Texture2D <float> LightShadowMap1;
		#else //! __ORBIS__
			SHADOWTYPE_1 LightShadow1 : LIGHTSHADOW1;
			Texture2D <float> LightShadowMap1 : LIGHTSHADOWMAP1;
		#endif //! __ORBIS__
	#endif
#endif

#if NUM_LIGHTS > 2
	#ifdef __ORBIS__
		LIGHTTYPE_2 Light2;
	#else //! __ORBIS__
		LIGHTTYPE_2 Light2 : LIGHT2;
	#endif //! __ORBIS__
	#ifndef SHADOWTYPE_2
		#define LightShadow2 0.0f
		#define LightShadowMap2 0.0f
	#else
		#ifdef __ORBIS__
			SHADOWTYPE_2 LightShadow2;
			Texture2D <float> LightShadowMap2;
		#else //! __ORBIS__
			SHADOWTYPE_2 LightShadow2 : LIGHTSHADOW2;
			Texture2D <float> LightShadowMap2 : LIGHTSHADOWMAP2;
		#endif //! __ORBIS__
	#endif
#endif

#if NUM_LIGHTS > 3
	#ifdef __ORBIS__
		LIGHTTYPE_3 Light3;
	#else //! __ORBIS__
		LIGHTTYPE_3 Light3 : LIGHT3;
	#endif //! __ORBIS__
	#ifndef SHADOWTYPE_3
		#define LightShadow3 0.0f
		#define LightShadowMap3 0.0f
	#else
		#ifdef __ORBIS__
			SHADOWTYPE_3 LightShadow3;
			Texture2D <float> LightShadowMap3;
		#else //! __ORBIS__
			SHADOWTYPE_3 LightShadow3 : LIGHTSHADOW3;
			Texture2D <float> LightShadowMap3 : LIGHTSHADOWMAP3;
		#endif //! __ORBIS__
	#endif
#endif

// Textures
#ifdef TEXTURE_ENABLED
Texture2D<float4> TextureSampler;
#endif // TEXTURE_ENABLED

#ifdef NORMAL_MAPPING_ENABLED
Texture2D<float4> NormalMapSampler;
#endif // NORMAL_MAPPING_ENABLED

#ifdef LIGHTPREPASS_ENABLED
Texture2D<float4> LightPrepassSampler;
#endif // LIGHTPREPASS_ENABLED

#ifdef LOD_BLEND
float LodBlendValue : LodBlendValue;
#endif


sampler LinearWrapSampler
{
	Filter = Min_Mag_Mip_Linear;
    AddressU = Wrap;
    AddressV = Wrap;
};
sampler TextureSamplerSampler
{
	Filter = Min_Mag_Mip_Linear;
    AddressU = Wrap;
    AddressV = Wrap;
};
sampler NormalMapSamplerSampler
{
	Filter = Min_Mag_Mip_Linear;
    AddressU = Wrap;
    AddressV = Wrap;
};
sampler PointWrapSampler
{
	Filter = Min_Mag_Mip_Point;
    AddressU = Wrap;
    AddressV = Wrap;
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global shader parameters.

struct ZVSInput
{
#ifdef SKINNING_ENABLED
	#ifdef __ORBIS__
		float4 SkinnableVertex VS_ATTR(POSITION);
	#else //! __ORBIS__
		float3 SkinnableVertex VS_ATTR(POSITION);
	#endif //! __ORBIS__
#else //! SKINNING_ENABLED
	#ifdef __ORBIS__
		float4 Position	VS_ATTR(POSITION);
	#else //! __ORBIS__
		float3 Position	VS_ATTR(POSITION);
	#endif //! __ORBIS__
#endif //! SKINNING_ENABLED
#ifdef SKINNING_ENABLED
	uint4	SkinIndices			VS_ATTR(BLENDINDICES);
	float4	SkinWeights			VS_ATTR(BLENDWEIGHT);
#endif //! SKINNING_ENABLED
};


struct DefaultVSInput
{
#ifdef VERTEX_COLOR_ENABLED
	float4 Color VS_ATTR(COLOR0);
#endif
#ifdef SKINNING_ENABLED
	#ifdef __ORBIS__
		float4 SkinnableVertex VS_ATTR(POSITION);
	#else //! __ORBIS__
		float3 SkinnableVertex VS_ATTR(POSITION);
	#endif //! __ORBIS
	float3 SkinnableNormal	VS_ATTR(NORMAL); 	
#else //! SKINNING_ENABLED
	#ifdef __ORBIS__
		float4 Position	VS_ATTR(POSITION);
	#else //! __ORBIS__
		float3 Position	VS_ATTR(POSITION);
	#endif //! __ORBIS__
	float3 Normal	VS_ATTR(NORMAL); 	
#endif //! SKINNING_ENABLED
	
#if defined(TEXTURE_ENABLED) || defined(NORMAL_MAPPING_ENABLED)
	float2 Uv	VS_ATTR(TEXCOORD0);
#ifdef USE_TANGENTS
	#ifdef SKINNING_ENABLED
		float3 SkinnableTangent	VS_ATTR(TEXCOORD1);
	#else //! SKINNING_ENABLED
		float3 Tangent	VS_ATTR(TEXCOORD1);
	#endif //! SKINNING_ENABLED
#endif //! USE_TANGENTS
#endif //! defined(TEXTURE_ENABLED) || defined(NORMAL_MAPPING_ENABLED)
#ifdef SKINNING_ENABLED
	uint4	SkinIndices			VS_ATTR(BLENDINDICES);
	float4	SkinWeights			VS_ATTR(BLENDWEIGHT);
#endif //! SKINNING_ENABLED
};


struct DefaultVSForwardRenderOutput
{
#ifdef VERTEX_COLOR_ENABLED
	float4 Color VS_ATTR(COLOR0);
#endif
	float4 Position	: SV_POSITION;	
	
	float2 Uv	VS_ATTR(TEXCOORD0);
	float4 NormalDepth	VS_ATTR(TEXCOORD1); 	
	float3 WorldPosition VS_ATTR(TEXCOORD2);
#ifdef USE_TANGENTS
	float3 Tangent	VS_ATTR(TEXCOORD3);
#endif //! USE_TANGENTS
};

struct DefaultPSLightPrepassRenderInput
{
#ifdef VERTEX_COLOR_ENABLED
	float4 Color VS_ATTR(COLOR0);
#endif
	float4 Position	: SV_POSITION;	
	float2 Uv	VS_ATTR(TEXCOORD0);
	float4 NormalDepth	VS_ATTR(TEXCOORD1); 	
	float3 WorldPosition VS_ATTR(TEXCOORD2);
#ifdef USE_TANGENTS
	float3 Tangent	VS_ATTR(TEXCOORD3);
#endif //! USE_TANGENTS
};


// Skinning code
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef SKINNING_ENABLED
// Evaluate skin for position, normal and tangent, for 4 bone weights.
void EvaluateSkinPositionNormalTangent4Bones( inout float3 position, inout float3 normal, inout float3 tangent, float4 weights, uint4 boneIndices)
{
	uint indexArray[4] = {boneIndices.x,boneIndices.y,boneIndices.z,boneIndices.w};

	float4 inPosition = float4(position,1);
	float4 inNormal = float4(normal,0);
	float4 inTangent = float4(tangent,0);
	
 	position = 
		mul(inPosition, BoneTransforms[indexArray[0]]).xyz * weights.x
	+	mul(inPosition, BoneTransforms[indexArray[1]]).xyz * weights.y
	+	mul(inPosition, BoneTransforms[indexArray[2]]).xyz * weights.z
	+	mul(inPosition, BoneTransforms[indexArray[3]]).xyz * weights.w;
	
	normal = 
		mul(inNormal, BoneTransforms[indexArray[0]]).xyz * weights.x
	+	mul(inNormal, BoneTransforms[indexArray[1]]).xyz * weights.y
	+	mul(inNormal, BoneTransforms[indexArray[2]]).xyz * weights.z
	+	mul(inNormal, BoneTransforms[indexArray[3]]).xyz * weights.w;

	tangent = 
		mul(inTangent, BoneTransforms[indexArray[0]]).xyz * weights.x
	+	mul(inTangent, BoneTransforms[indexArray[1]]).xyz * weights.y
	+	mul(inTangent, BoneTransforms[indexArray[2]]).xyz * weights.z
	+	mul(inTangent, BoneTransforms[indexArray[3]]).xyz * weights.w;
		
	
}


void EvaluateSkinPositionNormal4Bones( inout float3 position, inout float3 normal, float4 weights, uint4 boneIndices )
{
	uint indexArray[4] = {boneIndices.x,boneIndices.y,boneIndices.z,boneIndices.w};

	float4 inPosition = float4(position,1);
	float4 inNormal = float4(normal,0);
	
 	position = 
		mul(inPosition, BoneTransforms[indexArray[0]]).xyz * weights.x
	+	mul(inPosition, BoneTransforms[indexArray[1]]).xyz * weights.y
	+	mul(inPosition, BoneTransforms[indexArray[2]]).xyz * weights.z
	+	mul(inPosition, BoneTransforms[indexArray[3]]).xyz * weights.w;
	
	normal = 
		mul(inNormal, BoneTransforms[indexArray[0]]).xyz * weights.x
	+	mul(inNormal, BoneTransforms[indexArray[1]]).xyz * weights.y
	+	mul(inNormal, BoneTransforms[indexArray[2]]).xyz * weights.z
	+	mul(inNormal, BoneTransforms[indexArray[3]]).xyz * weights.w;
	
}

void EvaluateSkinPosition4Bones( inout float3 position, float4 weights, uint4 boneIndices )
{
	uint indexArray[4] = {boneIndices.x,boneIndices.y,boneIndices.z,boneIndices.w};
	float4 inPosition = float4(position,1);
	
 	position = 
		mul(inPosition, BoneTransforms[indexArray[0]]).xyz * weights.x
	+	mul(inPosition, BoneTransforms[indexArray[1]]).xyz * weights.y
	+	mul(inPosition, BoneTransforms[indexArray[2]]).xyz * weights.z
	+	mul(inPosition, BoneTransforms[indexArray[3]]).xyz * weights.w;
}

#endif //! SKINNING_ENABLED

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vertex shaders

// Default shadow vertex shader.
float4 DefaultShadowVS(ZVSInput IN) : SV_POSITION
{
#ifdef SKINNING_ENABLED
	float3 position = IN.SkinnableVertex.xyz;
	EvaluateSkinPosition4Bones(position.xyz, IN.SkinWeights, IN.SkinIndices );
	return mul(float4(position.xyz,1), ViewProjection);	
#else //! SKINNING_ENABLED
	float3 position = IN.Position.xyz;
	return mul(float4(position.xyz,1), WorldViewProjection);
#endif //! SKINNING_ENABLED
}

// Default Z prepass vertex shader.
float4 DefaultZPrePassVS(ZVSInput IN) : SV_POSITION
{
#ifdef SKINNING_ENABLED
	float3 position = IN.SkinnableVertex.xyz;
	EvaluateSkinPosition4Bones(position.xyz, IN.SkinWeights, IN.SkinIndices );
	return mul(float4(position.xyz,1), ViewProjection);	
#else //! SKINNING_ENABLED
	float3 position = IN.Position.xyz;
	return mul(float4(position.xyz,1), WorldViewProjection);
#endif //! SKINNING_ENABLED
}

// Default forward render vertex shader
DefaultVSForwardRenderOutput DefaultForwardRenderVS(DefaultVSInput IN)
{
	DefaultVSForwardRenderOutput OUT;

#ifdef SKINNING_ENABLED
	float3 position = IN.SkinnableVertex.xyz;
#ifdef USE_TANGENTS
	EvaluateSkinPositionNormalTangent4Bones(position.xyz, IN.SkinnableNormal.xyz, IN.SkinnableTangent.xyz, IN.SkinWeights, IN.SkinIndices );
	float3 tangent = IN.SkinnableTangent;
#else //! USE_TANGENTS
	EvaluateSkinPositionNormal4Bones(position.xyz, IN.SkinnableNormal.xyz, IN.SkinWeights, IN.SkinIndices );
#endif //! USE_TANGENTS
	float3 normal = IN.SkinnableNormal;
#else //! SKINNING_ENABLED
	float3 position = IN.Position.xyz;
	float3 normal = IN.Normal;
#ifdef USE_TANGENTS
	float3 tangent = IN.Tangent;
#endif //! USE_TANGENTS
#endif //! SKINNING_ENABLED

#ifdef SKINNING_ENABLED
	OUT.WorldPosition = position;
#else //! SKINNING_ENABLED
	OUT.WorldPosition = mul(float4(position,1.0f), World).xyz;
#endif //! SKINNING_ENABLED
	OUT.Position = mul(float4(OUT.WorldPosition,1.0f), ViewProjection);
#if defined(TEXTURE_ENABLED) || defined(NORMAL_MAPPING_ENABLED)
	OUT.Uv.xy = IN.Uv * float2(TextureScaleU, TextureScaleV) + float2(TextureOffsetU, TextureOffsetV);
#else //! defined(TEXTURE_ENABLED) || defined(NORMAL_MAPPING_ENABLED)
	OUT.Uv.xy = 0;
#endif //! defined(TEXTURE_ENABLED) || defined(NORMAL_MAPPING_ENABLED)
		
	OUT.NormalDepth = float4( normalize(mul(float4(normal,0), World).xyz), -mul(float4(position,1.0f), WorldView).z );
#ifdef USE_TANGENTS
	OUT.Tangent = normalize(mul(float4(tangent,0), World).xyz);
#endif //! USE_TANGENTS
    
#ifdef VERTEX_COLOR_ENABLED
	OUT.Color = IN.Color;
#endif
	return OUT;
}


// Default forward render vertex shader
DefaultVSForwardRenderOutput DefaultDeferredRenderVS(DefaultVSInput IN)
{
	DefaultVSForwardRenderOutput OUT;

#ifdef SKINNING_ENABLED
	float3 position = IN.SkinnableVertex.xyz;
#ifdef USE_TANGENTS
	EvaluateSkinPositionNormalTangent4Bones(position.xyz, IN.SkinnableNormal.xyz, IN.SkinnableTangent.xyz, IN.SkinWeights, IN.SkinIndices );
	float3 tangent = IN.SkinnableTangent;
#else //! USE_TANGENTS
	EvaluateSkinPositionNormal4Bones(position.xyz, IN.SkinnableNormal.xyz, IN.SkinWeights, IN.SkinIndices );
#endif //! USE_TANGENTS
	float3 normal = IN.SkinnableNormal;
#else //! SKINNING_ENABLED
	float3 position = IN.Position.xyz;
	float3 normal = IN.Normal;
#ifdef USE_TANGENTS
	float3 tangent = IN.Tangent;
#endif //! USE_TANGENTS
#endif //! SKINNING_ENABLED

#ifdef SKINNING_ENABLED
	OUT.WorldPosition = position;
#else //! SKINNING_ENABLED
	OUT.WorldPosition = mul(float4(position,1.0f), World).xyz;
#endif //! SKINNING_ENABLED
	OUT.Position = mul(float4(OUT.WorldPosition,1.0f), ViewProjection);
#if defined(TEXTURE_ENABLED) || defined(NORMAL_MAPPING_ENABLED)
	OUT.Uv.xy = IN.Uv;
#else //! defined(TEXTURE_ENABLED) || defined(NORMAL_MAPPING_ENABLED)
	OUT.Uv.xy = 0;
#endif //! defined(TEXTURE_ENABLED) || defined(NORMAL_MAPPING_ENABLED)
		
	OUT.NormalDepth = float4( normalize(mul(float4(normal,0), World).xyz), -mul(float4(position,1.0f), WorldView).z );
#ifdef USE_TANGENTS
	OUT.Tangent = normalize(mul(float4(tangent,0), World).xyz);
#endif //! USE_TANGENTS
    
#ifdef VERTEX_COLOR_ENABLED
	OUT.Color = IN.Color;
#endif
	return OUT;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fragment utility macros. Just make the fragment shader code a bit neater by hiding some of the combination handling / argument passing here.
#if defined(NORMAL_MAPPING_ENABLED)
float3 EvaluateNormalMapNormal(float3 inNormal, float2 inUv, float3 inTangent)
{
	float4 normalMapValue = NormalMapSampler.Sample(NormalMapSamplerSampler, inUv);
	float3 normalMapNormal = normalize(normalMapValue.xyz * 2.0f - 1.0f);

	// Evaluate tangent basis
	float3 basis0 = normalize(inTangent);
	float3 basis2 = normalize(inNormal);
	float3 basis1 = cross(basis0, basis2);

	float3 normal = (normalMapNormal.x * basis0) + (normalMapNormal.y * basis1) + (normalMapNormal.z * basis2);	
	return normal;
}
#endif // NORMAL_MAPPING_ENABLED

#if defined(NORMAL_MAPPING_ENABLED)
#define EvaluateNormal(In) EvaluateNormalMapNormal(In.NormalDepth.xyz,In.Uv,In.Tangent)
#else
#define EvaluateNormal(In) EvaluateStandardNormal(In.NormalDepth.xyz)
#endif

#ifdef RECEIVE_SHADOWS
#define EvaluateShadowValue(LightId, LightShadowId, ShadowMapId, worldPos, viewDepth) EvaluateShadow(LightId, LightShadowId, ShadowMapId, worldPos, viewDepth)
#else
#define EvaluateShadowValue(LightId, LightShadowId, ShadowMapId, worldPos, viewDepth) 1.0f
#endif

#ifdef SPECULAR_ENABLED
#define EvaluateLightFunction(LightIndex) \
	{ \
		float shad = EvaluateShadowValue(Light##LightIndex, LightShadow##LightIndex, LightShadowMap##LightIndex, worldPosition, In.NormalDepth.w); \
		lightResult += EvaluateLight(Light##LightIndex, worldPosition,normal,-eyeDirection, shad, MaterialSpecular.x,SpecularPower /*,FresnelPower*/); \
	}
#else 
#define EvaluateLightFunction(LightIndex) \
	{ \
		float shad = EvaluateShadowValue(Light##LightIndex, LightShadow##LightIndex, LightShadowMap##LightIndex, worldPosition, In.NormalDepth.w); \
		lightResult += EvaluateLight(Light##LightIndex, worldPosition,normal, shad); \
	}
#endif

#ifdef LOD_BLEND
float GetLODDitherValue(float2 screenUv)
{	
	float4 ditherValue = DitherNoiseTexture.Load(int3(screenUv,0) & 255);
	float threshold = (1.0f - abs(LodBlendValue));

	float lodBlendValueSign = sign(LodBlendValue);
	float rslt = ((ditherValue.x >= threshold) ? 1.0f : -1.0f) * lodBlendValueSign;
	return rslt;
}
#endif //! LOD_BLEND



float4 PackNormalAndViewSpaceDepth(float3 normal, float viewSpaceZ)
{
	float normalizedViewZ = viewSpaceZ / cameraNearFar.y;
	float2 depthPacked = float2( floor(normalizedViewZ * 256.0f) / 255.0f,  frac(normalizedViewZ * 256.0f) );
	float4 rslt = float4(normal.xy, depthPacked);
	return rslt;
}
float4 PackNormalAndDepth(float3 normal, float depth)
{
	float viewSpaceZ = -(cameraNearTimesFar / (depth * cameraFarMinusNear - cameraNearFar.y));
	return PackNormalAndViewSpaceDepth(normal,viewSpaceZ);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fragment shaders

// Default fragment shader. 
float4 DefaultUnshadedFP(float4 ScreenPosition : SV_POSITION) : FRAG_OUTPUT_COLOR0
{
#ifdef LOD_BLEND
	clip(GetLODDitherValue(ScreenPosition.xy));
#endif
	return 1;
}


// Default light pre pass first pass shader. Outputs normal and defaulted specular power only.
float4 DefaultLightPrepassFP(DefaultVSForwardRenderOutput In) : FRAG_OUTPUT_COLOR0
{		
	float3 normal = EvaluateNormal(In);

#ifdef SPECULAR_ENABLED
	// could vary by pixel with a texture lookup
	float specPower = SpecularPower;
	float gloss = 1;
#else
	float specPower = 0;
	float gloss = 0;
#endif

	float3 viewSpaceNormal = normalize(mul(float4(normal.xyz,0), View).xyz);

	float4 Out = PackNormalAndViewSpaceDepth(float3(viewSpaceNormal.xy*0.5f+0.5f,0), In.NormalDepth.w);
	return Out;	

}

float3 EvaluateLightingDefault( DefaultVSForwardRenderOutput In, float3 worldPosition, float3 normal )
{		
	// Lighting
	float3 lightResult = 1;
	float3 eyeDirection = normalize(worldPosition - EyePosition.xyz);
	lightResult = GlobalAmbientColor;
	
#if NUM_LIGHTS > 0
	EvaluateLightFunction(0);
#endif
#if NUM_LIGHTS > 1
	EvaluateLightFunction(1);
#endif
#if NUM_LIGHTS > 2
	EvaluateLightFunction(2);
#endif
#if NUM_LIGHTS > 3
	EvaluateLightFunction(3);
#endif

	return lightResult;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Structures
// Dont need any - use the defaults.

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vertex shaders
// Dont need any - use the defaults.

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fragment shaders.

// Forward render fragment shader
float4 ForwardRenderFP(DefaultVSForwardRenderOutput In) : FRAG_OUTPUT_COLOR0
{	
	float3 normal = EvaluateNormal(In);
		

#ifdef VERTEX_COLOR_ENABLED
	float4 shadingResult = In.Color;
#else
	float4 shadingResult = MaterialColor;
#endif
#ifdef TEXTURE_ENABLED
	shadingResult *= TextureSampler.Sample(TextureSamplerSampler, In.Uv) * float4(1.0,1.0,1.0,AlphaGain);
#endif

	// Lighting
	float3 lightResult = EvaluateLightingDefault(In, In.WorldPosition, normal );

#ifndef VERTEX_COLOR_ENABLED
	lightResult *= MaterialDiffuse.xyz;
#endif
	lightResult += MaterialAmbient.xyz;
	shadingResult.xyz *= lightResult.xyz;
	shadingResult.xyz += MaterialEmission.xyz;
	
#ifdef LOD_BLEND
	clip(GetLODDitherValue(In.Position.xy));
#endif

#ifdef ALPHA_ENABLED
	shadingResult.w *= 1 - MaterialTransparency.x;
#endif //! ALPHA_ENABLED
	
	return shadingResult;
}

// Light pre pass second pass shader. Samples the light prepass buffer.
float4 LightPrepassApplyFP(DefaultPSLightPrepassRenderInput In) : FRAG_OUTPUT_COLOR0
{		
	float3 normal = EvaluateNormal(In);
	float3 worldPosition = In.WorldPosition;
	float3 eyeDirection = normalize(worldPosition - EyePosition.xyz);
	
#ifdef VERTEX_COLOR_ENABLED
	float4 shadingResult = In.Color;
#else
	float4 shadingResult = MaterialColor;
#endif
#ifdef TEXTURE_ENABLED
	shadingResult *= TextureSampler.Sample(TextureSamplerSampler, In.Uv);
#endif
	float alphaValue = shadingResult.w;

#ifdef LIGHTPREPASS_ENABLED
	// Lighting
	float2 screenUv = In.ScreenPosition.xy * screenWidthHeightInv;
	float3 lightResult = LightPrepassSampler.Sample(LinearWrapSampler,screenUv).xyz;
#else 
	float3 lightResult = 1;
#endif

	lightResult *= MaterialDiffuse.xyz;
	lightResult += MaterialAmbient.xyz;
	shadingResult.xyz *= lightResult;
	shadingResult.xyz += MaterialEmission.xyz;

	return shadingResult;
}

// Deferred rendering
PSDeferredOutput DefaultDeferredRenderFP(DefaultVSForwardRenderOutput In) 
{		
	float3 normal = EvaluateNormal(In);

#ifdef VERTEX_COLOR_ENABLED
	float4 colour = In.Color;
#else
	float4 colour = MaterialColor;
#endif

	colour.xyz *= MaterialDiffuse.xyz;
#ifdef TEXTURE_ENABLED
	colour *= TextureSampler.Sample(TextureSamplerSampler, In.Uv);
#endif //! TEXTURE_ENABLED

#ifndef SPECULAR_ENABLED
	colour.w = 0.0;
#else
	colour.w *= MaterialSpecular.x;
#endif //! SPECULAR_ENABLED
	
	float3 viewSpaceNormal = normalize(mul(float4(normal.xyz,0), View).xyz);
	
#ifdef LOD_BLEND
	clip(GetLODDitherValue(In.Position.xy));
#endif //! LOD_BLEND
	PSDeferredOutput Out;
	Out.Colour = float4(colour.xyz, MaterialEmission.x);
	Out.NormalDepth = float4(viewSpaceNormal.xyz * 0.5f + 0.5f, colour.w);
	return Out;	

}

#ifndef __ORBIS__

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// State blocks


BlendState NoBlend 
{
  BlendEnable[0] = FALSE;
};
BlendState LinearBlend 
{
	BlendEnable[0] = TRUE;
	SrcBlend[0] = SRC_ALPHA;
	DestBlend[0] = INV_SRC_ALPHA;
	BlendOp[0] = ADD;
	SrcBlendAlpha[0] = ONE;
	DestBlendAlpha[0] = ONE;
	BlendOpAlpha[0] = ADD;
};
DepthStencilState DepthState {
  DepthEnable = TRUE;
  DepthWriteMask = All;
  DepthFunc = Less;
  StencilEnable = FALSE; 
};

RasterizerState DefaultRasterState 
{
	CullMode = None;
};
RasterizerState CullRasterState 
{
	CullMode = Front;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Techniques.

#ifndef ALPHA_ENABLED


technique11 ForwardRender
<
	string PhyreRenderPass = "Opaque";
	string VpIgnoreContextSwitches[] = {"NUM_LIGHTS", "LOD_BLEND"};
>
{
	pass pass0
	{
		SetVertexShader( CompileShader( vs_4_0, DefaultForwardRenderVS() ) );
		SetPixelShader( CompileShader( ps_4_0, ForwardRenderFP() ) );
	
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );		
	}
}

#endif

#ifdef ALPHA_ENABLED

technique11 ForwardRenderAlpha
<
	string PhyreRenderPass = "Transparent";
	string VpIgnoreContextSwitches[] = {"NUM_LIGHTS", "LOD_BLEND"};
>
{
	pass pass0
	{
		SetVertexShader( CompileShader( vs_4_0, DefaultForwardRenderVS() ) );
		SetPixelShader( CompileShader( ps_4_0, ForwardRenderFP() ) );
	
		SetBlendState( LinearBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( CullRasterState );		
	}
}

#endif

#ifdef CASTS_SHADOWS

technique11 Shadow
<
	string PhyreRenderPass = "Shadow";
	string VpIgnoreContextSwitches[] = {"NUM_LIGHTS", "LOD_BLEND"};
	string FpIgnoreContextSwitches[] = {"NUM_LIGHTS"};
>
{
	pass p0
	{
		SetVertexShader( CompileShader( vs_4_0, DefaultShadowVS() ) );
		SetPixelShader( CompileShader( ps_4_0, DefaultUnshadedFP() ) );
	
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );		
	}
}

#endif


#ifndef ALPHA_ENABLED

technique11 ZPrePass
<
	string PhyreRenderPass = "ZPrePass";
	string VpIgnoreContextSwitches[] = {"NUM_LIGHTS", "LOD_BLEND"};
	string FpIgnoreContextSwitches[] = {"NUM_LIGHTS"};
>
{
	pass p0
	{
		SetVertexShader( CompileShader( vs_4_0, DefaultShadowVS() ) );
		SetPixelShader( CompileShader( ps_4_0, DefaultUnshadedFP() ) );
	
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );		
	}
}

#endif


#ifndef ALPHA_ENABLED

// Techniques
technique11 DeferredRender
<
	string PhyreRenderPass = "DeferredRender";
	string VpIgnoreContextSwitches[] = {"NUM_LIGHTS", "LOD_BLEND"};
	string FpIgnoreContextSwitches[] = {"NUM_LIGHTS", "INSTANCING_ENABLED"};
>
{
	pass p0
	{
		SetVertexShader( CompileShader( vs_4_0, DefaultDeferredRenderVS() ) );
		SetPixelShader( CompileShader( ps_4_0, DefaultDeferredRenderFP() ) );
		
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );	
	}
}

#endif

#if 0 // Note: These techniques are disabled until future support is added

#ifndef ALPHA_ENABLED

technique11 LightPrePass
<
	string PhyreRenderPass = "LightPrePass";
	string VpIgnoreContextSwitches[] = {"NUM_LIGHTS", "LOD_BLEND"};
	string FpIgnoreContextSwitches[] = {"NUM_LIGHTS"};
>
{
	pass p0
	{
		DepthTestEnable=true;
#ifdef ZPREPASS_ENABLED
		DepthMask = false;
#else
		DepthMask = true;	
#endif
		DepthFunc = LEqual;
		ColorMask = bool4(true,true,true,true);
		VertexProgram = compile vp40 DefaultForwardRenderVS();
		FragmentProgram = compile fp40 DefaultLightPrepassFP();
	}
}

#endif


#ifndef ALPHA_ENABLED

technique11 LightPreMaterialPass
<
	string PhyreRenderPass = "LightPrePassMaterial";
	string VpIgnoreContextSwitches[] = {"NUM_LIGHTS", "LOD_BLEND"};
	string FpIgnoreContextSwitches[] = {"NUM_LIGHTS"};
>
{
	pass
	{
		DepthTestEnable=true;
		DepthMask = false;
		DepthFunc = LEqual;
		ColorMask = bool4(true,true,true,true);
		VertexProgram = compile vp40 DefaultForwardRenderVS();
		FragmentProgram = compile fp40 LightPrepassApplyFP();
	}
}

#endif


#endif // Disabled techniques

#endif //! __ORBIS__
