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

#ifndef H_COMMON
#define H_COMMON

//////////////////////////////////////////////
// Defines
//////////////////////////////////////////////

#define AVSM_FILTERING_ENABLED

#define LT_BILINEAR_FILTERING

#define EARLY_DEPTH_CULL_ALPHA (0x1UL)

///////////////////////
// Resources
///////////////////////

TextureCube  gIBLTexture;
SamplerState gIBLSampler;

Texture2DMS<float4>       gCompositingBuffer;

//////////////////////////////////////////////
// Constants
//////////////////////////////////////////////

struct UIConstants
{
    uint faceNormals;
    uint avsmSortingMethod;
    uint volumeShadowMethod;
    uint enableVolumeShadowLookup;
    uint pauseParticleAnimaton;
    float particleSize;
    uint particleOpacity;
    uint hairThickness;
    uint hairShadowThickness;
    uint hairOpacity;
    uint lightingOnly;
    uint stNumSamples;
};

cbuffer PerFrameConstants
{
    float4x4    mCameraWorldViewProj;
    float4x4    mCameraWorldView;
    float4x4    mCameraViewProj;
    float4x4    mCameraProj;
    float4      mCameraPos;
    float4x4    mLightWorldViewProj;
    float4x4    mAvsmLightWorldViewProj;    
    float4x4    mCameraViewToWorld;
    float4x4    mCameraViewToLightProj;
    float4x4    mCameraViewToLightView;    
    float4x4    mCameraViewToAvsmLightProj;
    float4x4    mCameraViewToAvsmLightView;        
    float4      mLightDir;
    float4      mMSAACoverageNorm;
    float4      mGeometryAlpha;
    float4      mAlphaThreshold;
    
    UIConstants mUI;
};


// data that we can read or derived from the surface shader outputs
struct SurfaceData
{
    float3 positionView;         // View space position
    float3 positionViewDX;       // Screen space derivatives
    float3 positionViewDY;       // of view space position
    float3 normal;               // View space normal
    float4 albedo;
    float2 lightTexCoord;        // Texture coordinates in light space, [0, 1]
    float2 lightTexCoordDX;      // Screen space partial derivatives
    float2 lightTexCoordDY;      // of light space texture coordinates.
    float  lightSpaceZ;          // Z coordinate (depth) of surface in light space
};


//////////////////////////////////////////////
// Full screen pass
//////////////////////////////////////////////

struct FullScreenTriangleVSOut
{
    float4 positionViewport : SV_Position;
    float4 positionClip     : positionClip;
    float2 texCoord         : texCoord;
};

//////////////////////////////////////////////
// Helper Functions
//////////////////////////////////////////////

float linstep(float min, float max, float v)
{
    return saturate((v - min) / (max - min));
}

float2 ProjectIntoLightTexCoord(float3 positionView)
{
    float4 positionLight = mul(float4(positionView, 1.0f), mCameraViewToLightProj);
    float2 texCoord = (positionLight.xy / positionLight.w) * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
    return texCoord;
}

float2 ProjectIntoAvsmLightTexCoord(float3 positionView)
{
    float4 positionLight = mul(float4(positionView, 1.0f), mCameraViewToAvsmLightProj);
    float2 texCoord = (positionLight.xy / positionLight.w) * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
    return texCoord;
}

#endif // H_COMMON