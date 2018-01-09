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

#ifndef H_DEBUG_CPP
#define H_DEBUG_CPP

#define DEBUG_MAX_NODE_COUNT (1024)

#define DEBUG_DRAW_ABUFFER_COLOR_IDX    (0)
#define DEBUG_DRAW_AOIT_COLOR_IDX       (1)

#ifdef DEBUG_SHADER_CODE
typedef uint uint32;
#else
typedef unsigned int uint32;
#endif // DEBUG_SHADER_CODE

#ifndef DEBUG_SHADER_CODE
__declspec(align(16))
struct 
#else
cbuffer
#endif // DEBUG_SHADER_CODE
DebugDrawShaderConstants
{
#ifdef DEBUG_SHADER_CODE
    float4x4      cameraProj;
    float4        lineColor[8];
#else
    D3DXMATRIXA16 cameraProj;
    float         lineColor[32];
#endif
    float         pixelCoordX;
    float         pixelCoordY;
    uint32        enableABuffer;
    uint32        enableAOIT;
};

struct DebugNodeData
{
    float   depth;
    float   trans;
#ifdef DEBUG_SHADER_CODE
    float4  color;
#else
    float   color[4];
#endif
};

struct DebugDrawIndirectArgs
{
    uint32  vertexCountPerInstance;
    uint32  instanceCount;
    uint32  startVertexLocation;
    uint32  startInstanceLocation;
};

#ifdef DEBUG_SHADER_CODE
cbuffer
#else
__declspec(align(16))
struct
#endif
PIPBGCB
{
#ifdef DEBUG_SHADER_CODE
    float2 viewportTopLeft;
    float2 viewportDim;
    float2 viewportZRange;
#else
    D3D11_VIEWPORT viewport;
#endif
#ifdef DEBUG_SHADER_CODE
    uint2 pickedTarget;
#else
    uint32 pickedTargetX;
    uint32 pickedTargetY;
#endif
    float backgroundAlpha;
};

#endif // H_DEBUG_CPP