// Copyright 2011 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies.  Intel makes no representations about the
// suitability of this software for any purpose.  THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.

#ifndef RENDERING_HLSL
#define RENDERING_HLSL

#include "Common.hlsl"
#include "AVSM.hlsl"
#include "FragmentList.hlsl"

//--------------------------------------------------------------------------------------
// Defines
//--------------------------------------------------------------------------------------

//#define SPECULAR_ENABLED

//--------------------------------------------------------------------------------------
// Resources
//--------------------------------------------------------------------------------------

Texture2D    gDiffuseTexture;
SamplerState gDiffuseSampler;

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------

struct GeometryVSIn
{
    float3 position : position;
    float3 normal   : normal;
    float2 texCoord : texCoord;
};

struct GeometryVSOut
{
    float4 position     : SV_position;
    float3 color        : color;
    float3 positionView : positionView;      // View space position
    float3 normal       : normal;
    float2 texCoord     : texCoord;
};

//--------------------------------------------------------------------------------------
// Vertex shaders
//--------------------------------------------------------------------------------------

GeometryVSOut GeometryVS(GeometryVSIn input, uint vertexID : SV_VertexID)
{
    GeometryVSOut output;

    output.position     = mul(float4(input.position, 1.0f), mCameraWorldViewProj);
    output.positionView = mul(float4(input.position, 1.0f), mCameraWorldView).xyz;
    output.normal       = mul(float4(input.normal, 0.0f), mCameraWorldView).xyz;
    output.texCoord     = input.texCoord;
    output.color        = float3(1, 1, 1);

    return output;


    // Commented out hacks to render colored teapots
    /*%if (!((vertexID / 18960) & 0x1U)) {
        output.color        = float3(1, 0, 0);
    }*/

    /*
    uint idx = (vertexID / 18960) & 0x3U;
    switch (idx) {
        case 1:
            output.color = float3(1, 1, 1); break;
        case 0:
            output.color = float3(0.96, 0.58, 0.13); break;
        case 3:
            output.color = float3(0.75, 0.13, 0.18); break;
        case 2:
            output.color = float3(0.09, 0.12, 0.37); break;
    };*/

    /*
    uint idx = (vertexID / 18960) & 0x1U;
    switch (idx) {
        case 0:
            output.color = float3(0.75, 0.13, 0.18); break;
        case 1:
            output.color = float3(0.09, 0.12, 0.37); break;
    };*/
}


//--------------------------------------------------------------------------------------
// Lighting phase
//--------------------------------------------------------------------------------------

float4 Ambient(in GeometryVSOut input)
{
    return float4(0.15f, 0.15, 0.15f, 1.0f);
}

float4 Albedo(in GeometryVSOut input)
{
	uint2 dim;
    float4 albedo;
	gDiffuseTexture.GetDimensions(dim.x, dim.y);

    if (mUI.lightingOnly || dim.x == 0U) {
    	albedo = float4(1.0f, 1.0f, 1.0f, 1.0f);
    } else {
    	albedo = gDiffuseTexture.Sample(gDiffuseSampler, input.texCoord);
    }

    return input.color.xyzz * albedo;
}

float4 Lambert(in GeometryVSOut input)
{
    return saturate(dot(-mLightDir.xyz, normalize(input.normal)));
}

float BlinnPhong(in GeometryVSOut input)
{    
#ifdef SPECULAR_ENABLED
    float3 h = normalize(-mLightDir.xyz - input.positionView.xyz);
    return 4 * pow(saturate(dot(normalize(input.normal), h)), 128);	
#else
    return 0;
#endif
}

//--------------------------------------------------------------------------------------
// Pixel shaders
//--------------------------------------------------------------------------------------

float4 GeometryPS(GeometryVSOut input) : SV_Target0
{
    float  surfaceDepth = input.positionView.z;
    float  receiverDepth = mul(float4(input.positionView.xyz, 1.0f), mCameraViewToAvsmLightView).z;       
    float2 lightTexCoord = ProjectIntoAvsmLightTexCoord(input.positionView.xyz);

    float shadowTerm = 1.0f;
    [branch]if (mUI.enableVolumeShadowLookup) {
        shadowTerm = VolumeSample(mUI.volumeShadowMethod, lightTexCoord, receiverDepth);
    }

    float4 albedo = Albedo(input);
#ifndef ALPHA_FROM_TEXTURE
    albedo.w = mGeometryAlpha.x;
#else
    [flatten] if (0 != mAlphaThreshold.x) {
        albedo.w = albedo.w >= mAlphaThreshold.x ? 1 : 0;
        [flatten] if (albedo.w == 0) discard;
    }
#endif
    
    //return saturate(1 - ((surfaceDepth + 20) / 300)).xxxx;
    return float4(saturate(albedo * (Ambient(input) + shadowTerm.xxxx * (BlinnPhong(input).xxxx + Lambert(input)))).xyz, albedo.w);
}

//--------------------------------------------------------------------------------------
// Capture fragments phase
//--------------------------------------------------------------------------------------

[earlydepthstencil]
void GeometryCapturePS(GeometryVSOut input, in uint coverageMask : SV_Coverage)
{
    float  surfaceDepth = input.positionView.z;
    float  receiverDepth = mul(float4(input.positionView.xyz, 1.0f), mCameraViewToAvsmLightView).z;                        
    float2 lightTexCoord = ProjectIntoAvsmLightTexCoord(input.positionView.xyz);

    float shadowTerm = 1.0f;
    [branch]if (mUI.enableVolumeShadowLookup) {
        shadowTerm = VolumeSample(mUI.volumeShadowMethod, lightTexCoord, receiverDepth);
    }

    float4 albedo = Albedo(input);

#ifdef ALPHA_FROM_TEXTURE
    float alpha = albedo.w;
    [flatten] if (0 != mAlphaThreshold.x) {
        alpha = alpha >= mAlphaThreshold.x ? 1 : 0;
    }
#else
    float alpha = mGeometryAlpha.x;
#endif

    float4 surfaceColor =  saturate(albedo * (Ambient(input) + shadowTerm.xxxx * (BlinnPhong(input).xxxx + Lambert(input))));
	surfaceColor.w = alpha;

    // can't use discard here because it crashes ATI shader compiler    
    if (0 != alpha) 
    {
        // Get fragment viewport coordinates
        uint newNodeAddress;    
        int2 screenAddress = int2(input.position.xy);     

    #ifdef LIST_EARLY_DEPTH_CULL
        uint packedCullData = gFragmentListDepthCullUAV[screenAddress];
        float cullDepth = asfloat(packedCullData & 0xFFFFFF00UL);
        uint  cullTrans = packedCullData & 0xFFUL;
        if ((cullTrans > EARLY_DEPTH_CULL_ALPHA) || (cullDepth > surfaceDepth)) 
        {	
    #endif 
            if (FL_AllocNode(newNodeAddress)) {       
                // Fill node
                FragmentListNode node;            
                node.color    = FL_PackColor(surfaceColor);
                node.depth    = FL_PackDepthAndCoverage(surfaceDepth, coverageMask);                 
            
                // Insert node!
                FL_InsertNode(screenAddress, newNodeAddress, node);         
                      
    #ifdef LIST_EARLY_DEPTH_CULL
                // Update culling data
                float trans = (1.0f - surfaceColor.w) * float(cullTrans);
                packedCullData = asuint(max(surfaceDepth, cullDepth)) & 0xFFFFFF00UL;
                packedCullData = packedCullData | (uint(ceil(trans)) & 0xFFUL);
                gFragmentListDepthCullUAV[screenAddress] = packedCullData;
            }  
    #endif
        } 
    }
}

//--------------------------------------------------------------------------------------
// Full-screen pass shaders
//--------------------------------------------------------------------------------------

FullScreenTriangleVSOut FullScreenTriangleVS(uint vertexID : SV_VertexID)
{
    FullScreenTriangleVSOut output;

    // Parametrically work out vertex location for full screen triangle
    float2 grid = float2((vertexID << 1) & 2, vertexID & 2);
    output.positionClip = float4(grid * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    output.positionViewport = output.positionClip;
    output.texCoord = grid;
    
    return output;
}

//--------------------------------------------------------------------------------------
// Skybox
//--------------------------------------------------------------------------------------
TextureCube gSkyboxTexture : register(t0);

struct SkyboxVSOut
{
    float4 position : SV_Position;
    float3 texCoord : texCoord;
};

SkyboxVSOut SkyboxVS(GeometryVSIn input)
{
    SkyboxVSOut output;
    
    // NOTE: Don't translate skybox, and make sure depth == 1
    output.position = mul(float4(input.position, 0.0f), mCameraViewProj).xyww;
    output.texCoord = input.position;
    
    return output;
}

float4 SkyboxPS(SkyboxVSOut input) : SV_Target0
{
    return float4(gSkyboxTexture.Sample(gDiffuseSampler, input.texCoord).rgb, 1.0);
}

#endif // ifndef RENDERING_HLSL
