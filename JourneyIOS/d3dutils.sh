
#if !defined(D_D3DUTILS_H) && defined(D_PLATFORM_D3D11)
#define D_D3DUTILS_H

	#ifdef D_D3D_STENCIL

		float4				stencil_texDim_depthScale_mask;
		float4				stencil_testColors;

		#ifdef MSAA
			Texture2DMS<uint2>	stencilTexMSAA;

			float GetStencilMask(float2 uv)
			{
				float2	uvScaled = uv * stencil_texDim_depthScale_mask.xy;
				uint3	uvInt = uint3(uint2(uvScaled), 0);
				uint	mask = uint(stencil_texDim_depthScale_mask.w);
				uint	stencil = stencilTexMSAA.Load(uvInt, 0).g;
				uint	stencilMasked = stencil&mask;
				float	d = float(stencilMasked);

				return d * stencil_texDim_depthScale_mask.z;
			}

		#else

			Texture2D<uint2>	stencilTex;

			float GetStencilMask(float2 uv)
			{
				float2	uvScaled = uv * stencil_texDim_depthScale_mask.xy;
				uint3	uvInt = uint3(uint2(uvScaled), 0);
				uint	mask = uint(stencil_texDim_depthScale_mask.w);
				uint	stencil = stencilTex.Load(uvInt).g;
				uint	stencilMasked = stencil&mask;
				float	d = float(stencilMasked);

				return d * stencil_texDim_depthScale_mask.z;
			}

		#endif

	
	#endif//D_D3D_STENCIL

#endif//!defined(D_D3DUTILS_H) && defined(D_PLATFORM_D3D11)