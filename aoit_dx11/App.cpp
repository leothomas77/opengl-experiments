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

#include "App.h"
#include "AVSM_def.h"
#include "Debug.h"
#include "ListTexture.h"
#include "MathUtils.h"
#include "Partitions.h"
#include "ShaderUtils.h"
#include "Stats.h"
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
 
using std::tr1::shared_ptr;

using namespace MathUtils;
using namespace ShaderUtils; 

static const float EMPTY_NODE = 65504.0f; // max half prec

template<typename ShaderType>
struct CreateShaderFromObjArgs {
    char* pFile;
    char* pFileAppend;
    int shaderCount;
    ShaderType** shader;
    ID3D11ShaderReflection** reflector;
    void Create(ID3D11Device* d3dDevice,
                void (*CreateShaderFromCompiledObj)(ID3D11Device*, LPCTSTR, ShaderType**, ID3D11ShaderReflection**),
                ShaderList<ShaderType>& loadedShaders) const 
    {
        if (0 == pFileAppend) {
            std::string objName = std::string("ShaderObjs\\") + std::string(pFile);
            std::wstring objWName(objName.length(), L'');
            std::copy(objName.begin(), objName.end(), objWName.begin());
            CreateShaderFromCompiledObj(d3dDevice, objWName.c_str(), shader, reflector);
            loadedShaders.push_back(*shader, *reflector);
        } else {
            for (int i = 0; i < shaderCount; i++) 
            {
                char cNodeCount[32];
                _itoa_s((i + 1) * 4, cNodeCount, sizeof(cNodeCount), 10);  
                std::string objName =   std::string("ShaderObjs\\") +
                                        std::string(pFile) + 
                                        std::string(cNodeCount) + 
                                        std::string(pFileAppend);
                std::wstring objWName(objName.length(), L'');
                std::copy(objName.begin(), objName.end(), objWName.begin());
                CreateShaderFromCompiledObj(d3dDevice, (LPCTSTR)objWName.c_str(), &shader[i], &reflector[i]);
                loadedShaders.push_back(shader[i], reflector[i]);
            }
        }
    }
};

App::App(ID3D11Device *d3dDevice,
         ID3D11DeviceContext* d3dDeviceContext,
         unsigned int msaaSampleCount,
         unsigned int shadowNodeCount,      
         unsigned int aoitNodeCount,
         unsigned int avsmShadowTextureDim)
    : mAVSMShadowTextureDim(avsmShadowTextureDim)
    , mLisTexNodeCount(1 << 22)
    // 50% coverage of screen (assuming resolution of 1680x1050
    // with average depth complexity of 50. Should be more than plenty.
    , mHairLTMaxNodeCount(unsigned int(1680 * 1050 * 0.5f * 15)) 
    , mHair(NULL)
    , mLastTime((float)DXUTGetGlobalTimer()->GetAbsoluteTime())
    , mHairMeshDirty(false)
    , mDepthDisabledDepthStencilState(0)
    , mDepthWritesDisabledDepthStencilState(0)
    , mFullScreenTrianglePS(0)
    , mFullScreenTrianglePSReflector(0)
{
    SetShadowNodeCount(shadowNodeCount);
    SetAOITNodeCount(aoitNodeCount);
    SetMSAASampleCount(msaaSampleCount);

    UINT shaderFlags = D3D10_SHADER_ENABLE_STRICTNESS | D3D10_SHADER_PACK_MATRIX_ROW_MAJOR;

    // Set up macros

    std::string avsmNodeCountStr;
    {
        std::ostringstream oss;
        oss << mAVSMShadowNodeCount;
        avsmNodeCountStr = oss.str();
    }

    D3D10_SHADER_MACRO shaderDefines[] = {
        {"AVSM_NODE_COUNT", avsmNodeCountStr.c_str()},
        {0, 0}
    };

    // Create geometry pass vertex shaders and input layout
    {
        HRESULT hr;

        // Vertex shader
        ID3D10Blob *vertexShaderBlob = 0;
        hr = D3DX11CompileFromFile(L"Rendering.hlsl", shaderDefines, 0,
                                   "GeometryVS", "vs_5_0",
                                   shaderFlags,
                                   0, 0, &vertexShaderBlob, 0, 0);
        // Do something better here on shader compile failure...
        assert(SUCCEEDED(hr));
        
        hr = d3dDevice->CreateVertexShader(vertexShaderBlob->GetBufferPointer(),
                                           vertexShaderBlob->GetBufferSize(),
                                           0,
                                           &mGeometryVS);
        assert(SUCCEEDED(hr));

        // Create input layout
        const D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            {"position",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"normal",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"texCoord",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        
        d3dDevice->CreateInputLayout( 
            layout, ARRAYSIZE(layout), 
            vertexShaderBlob->GetBufferPointer(),
            vertexShaderBlob->GetBufferSize(), 
            &mMeshVertexLayout);

        D3DReflect(vertexShaderBlob->GetBufferPointer(),
                   vertexShaderBlob->GetBufferSize(), 
                   IID_ID3D11ShaderReflection, 
                   (void**) &mGeometryVSReflector);

        vertexShaderBlob->Release();      
    }

    ////////////////////////////////////////////
    // Create vertex shaders
    ////////////////////////////////////////////

    CreateShaderFromObjArgs<ID3D11VertexShader> vsArray[] =
    {   // pFile, pFileAppend, shaderCount, shader, reflector
        { "FullScreenTriangleVS.fxo",       0, 1, &mFullScreenTriangleVS,     &mFullScreenTriangleVSReflector}, 
        { "DynamicParticlesShadingVS.fxo",  0, 1, &mParticleShadingVS,        &mParticleShadingVSReflector}, 
        { "HairVS.fxo",                     0, 1, &mHairVS,                   &mHairVSReflector}, 
        { "SkyboxVS.fxo",                   0, 1, &mSkyboxVS,                 &mSkyboxVSReflector}, 
        { "DebugDataVS.fxo",                0, 1, &mDebugDataVS,              &mDebugDataVSReflector}, 
    };

    for (int i = 0; i < sizeof(vsArray) / sizeof(vsArray[0]); i++) {
        vsArray[i].Create(d3dDevice, CreateVertexShaderFromCompiledObj, mLoadedVS);
    }

    ////////////////////////////////////////////
    // Create geometry shaders
    ////////////////////////////////////////////

    CreateShaderFromObjArgs<ID3D11GeometryShader> gsArray[] =
    {   // pFile, pFileAppend, shaderCount, shader, reflector
        { "HairGS.fxo", 0, 1,               &mHairGS,                   &mHairGSReflector}, 
    };

    for (int i = 0; i < sizeof(gsArray) / sizeof(gsArray[0]); i++) {
        gsArray[i].Create(d3dDevice, CreateGeometryShaderFromCompiledObj, mLoadedGS);
    }

    ////////////////////////////////////////////
    // Create pixel shaders
    ////////////////////////////////////////////

    CreateShaderFromObjArgs<ID3D11PixelShader> psArray[] = 
    {   // pFile, pFileAppend, shaderCount, shader, reflector
        //{"MSAAResolvePassPS.fxo",          0, 1, &mMSAAResolvePassPS,        &mMSAAResolvePassPSReflector}, 
        {"GeometryPS.fxo",                 0, 1, &mGeometryPS[0],            &mGeometryPSReflector[0]}, 
        {"GeometryAlphaPS.fxo",            0, 1, &mGeometryPS[1],            &mGeometryPSReflector[1]}, 
        {"GeometryCapture8PS.fxo",         0, 1, &mGeometryCapturePS[0],     &mGeometryCapturePSReflector[0]}, 
        {"GeometryAlphaCapture8PS.fxo",    0, 1, &mGeometryCapturePS[2],     &mGeometryCapturePSReflector[2]}, 
        {"ABufferSortedResolvePS.fxo",     0, 1, &mABufferSortedResolvePS[0],&mABufferSortedResolvePSReflector[0]}, 
        {"ABufferSortedResolveAAPS.fxo",   0, 1, &mABufferSortedResolvePS[1],&mABufferSortedResolvePSReflector[1]}, 
        {"ABufferResolvePS.fxo",           0, 1, &mABufferResolvePS[0],      &mABufferResolvePSReflector[0]}, 
        {"ABufferResolveAAPS.fxo",         0, 1, &mABufferResolvePS[1],      &mABufferResolvePSReflector[1]}, 
        {"ParticleAVSMCapturePS.fxo",      0, 1, &mParticleAVSMCapturePS,    &mParticleAVSMCapturePSReflector}, 
        {"ShadowHairCapturePS.fxo",        0, 1, &mShadowHairCapturePS,      &mShadowHairCapturePSReflector}, 
        {"SkyboxPS.fxo",                   0, 1, &mSkyboxPS,                 &mSkyboxPSReflector},  
        {"DebugDataPS.fxo",                0, 1, &mDebugDataPS,              &mDebugDataPSReflector}, 
        {"AOITResolve8PS.fxo",             0, 1, &mAOITResolvePS[0],         &mAOITResolvePSReflector[0]}, 
        {"AOITResolve16PS.fxo",            0, 1, &mAOITResolvePS[1],         &mAOITResolvePSReflector[1]}, 
        {"AOITResolve24PS.fxo",            0, 1, &mAOITResolvePS[2],         &mAOITResolvePSReflector[2]}, 
        {"AOITResolve32PS.fxo",            0, 1, &mAOITResolvePS[3],         &mAOITResolvePSReflector[3]}, 
        {"AOITResolve8AAPS.fxo",           0, 1, &mAOITResolvePS[4],         &mAOITResolvePSReflector[4]}, 
        {"AOITResolve16AAPS.fxo",          0, 1, &mAOITResolvePS[5],         &mAOITResolvePSReflector[5]}, 
        {"AOITResolve24AAPS.fxo",          0, 1, &mAOITResolvePS[6],         &mAOITResolvePSReflector[6]}, 
        {"AOITResolve32AAPS.fxo",          0, 1, &mAOITResolvePS[7],         &mAOITResolvePSReflector[7]}, 
        {"AVSM",                           "UnsortedResolvePS.fxo", 4, mAVSMUnsortedResolvePS,     mAVSMUnsortedResolvePSReflector}, 
        {"AVSM",                           "InsertionSortResolvePS.fxo", 4,  mAVSMInsertionSortResolvePS,     mAVSMInsertionSortResolvePSReflector}, 
        {"DynamicParticlesShading",        "PS.fxo", 4, mParticleShadingPS,             mParticleShadingPSReflector}, 
        {"DynamicParticlesShadingCapture8PS.fxo", 0, 1, &mParticleShadingCapturePS[0],      &mParticleShadingCapturePSReflector[0]}, 
        {"CameraHairCapture8PS.fxo",       0, 1, &mCameraHairCapturePS[0],              &mCameraHairCapturePSReflector[0]}, 
        {"StandardCameraHairRender",       "PS.fxo", 4, mStandardCameraHairRenderPS,    mStandardCameraHairRenderPSReflector}, 

        { "FullScreenTrianglePS.fxo", 0, 1, &mFullScreenTrianglePS, &mFullScreenTrianglePSReflector },
    };

    for (int i = 0; i < sizeof(psArray) / sizeof(psArray[0]); i++) {
        psArray[i].Create(d3dDevice, CreatePixelShaderFromCompiledObj, mLoadedPS);
    }

    ////////////////////////////////////////////
    // Create compute shaders
    ////////////////////////////////////////////

    CreateShaderFromObjArgs<ID3D11ComputeShader> csArray[] =
    {   // pFile, pFileAppend, shaderCount, shader, reflector
        { "ComputeFragmentStatsCS.fxo",     0, 1, &mComputeFragmentStatsCS,       &mComputeFragmentStatsCSReflector}, 
        { "ClearFragmentStatsCS.fxo",       0, 1, &mClearFragmentStatsCS,         &mClearFragmentStatsCSReflector}, 
        { "GatherDebugDataCS.fxo",          0, 1, &mGatherDebugDataCS,            &mGatherDebugDataCSReflector}, 
        { "GatherDebugAOIT8DataCS.fxo",     0, 1, &mGatherDebugAOITDataCS[0],     &mGatherDebugAOITDataCSReflector[0]}, 
        { "GatherDebugAOIT16DataCS.fxo",    0, 1, &mGatherDebugAOITDataCS[1],     &mGatherDebugAOITDataCSReflector[1]}, 
        { "GatherDebugAOIT24DataCS.fxo",    0, 1, &mGatherDebugAOITDataCS[2],     &mGatherDebugAOITDataCSReflector[2]}, 
        { "GatherDebugAOIT32DataCS.fxo",    0, 1, &mGatherDebugAOITDataCS[3],     &mGatherDebugAOITDataCSReflector[3]}, 
    };

    for (int i = 0; i < sizeof(csArray) / sizeof(csArray[0]); i++) {
        csArray[i].Create(d3dDevice, CreateComputeShaderFromCompiledObj, mLoadedCS);
    }
   
    // Create standard rasterizer state
    {
        CD3D11_RASTERIZER_DESC desc(D3D11_DEFAULT);
        d3dDevice->CreateRasterizerState(&desc, &mRasterizerState);
    }

    // Create double-sized standard rasterizer state
    {
        CD3D11_RASTERIZER_DESC desc(D3D11_DEFAULT);
        desc.CullMode = D3D11_CULL_NONE;
        desc.ScissorEnable = true;
        d3dDevice->CreateRasterizerState(&desc, &mDoubleSidedRasterizerState);
    }

    // Shadow rasterizer state has no back-face culling and multisampling enabled
    // TODO: This no back-face culling for shadows is generally a bad idea... revisit
    // once we have reasonable scenes in there.
    {
        CD3D11_RASTERIZER_DESC desc(D3D11_DEFAULT);
        desc.CullMode = D3D11_CULL_NONE;
        desc.MultisampleEnable = true;
        desc.DepthClipEnable = false;
        d3dDevice->CreateRasterizerState(&desc, &mShadowRasterizerState);
    }

    // Create particle rasterizer state
    {
        CD3D11_RASTERIZER_DESC desc(D3D11_DEFAULT);
        desc.CullMode = D3D11_CULL_NONE;
        desc.DepthClipEnable = false;
        d3dDevice->CreateRasterizerState(&desc, &mParticleRasterizerState);
    }

    // Create hair rasterizer state
    {
        CD3D11_RASTERIZER_DESC desc(D3D11_DEFAULT);
        desc.CullMode = D3D11_CULL_NONE;
        desc.DepthClipEnable = false;
        d3dDevice->CreateRasterizerState(&desc, &mHairRasterizerState);
    }

    // Create default depth-stencil state
    {
        CD3D11_DEPTH_STENCIL_DESC desc(D3D11_DEFAULT);
        // We need LESS_EQUAL for the skybox phase, so we just set it on everything
        // as it doesn't hurt.
        desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        d3dDevice->CreateDepthStencilState(&desc, &mDefaultDepthStencilState);

        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        d3dDevice->CreateDepthStencilState(&desc, &mDepthWritesDisabledDepthStencilState);

        desc.DepthEnable    = false;
        d3dDevice->CreateDepthStencilState(&desc, &mDepthDisabledDepthStencilState);
    }

    // Create particle depth-stencil state
    {
        CD3D11_DEPTH_STENCIL_DESC desc(D3D11_DEFAULT);
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        d3dDevice->CreateDepthStencilState(&desc, &mParticleDepthStencilState);
    }

    // Create particle depth-stencil state
    {
        CD3D11_DEPTH_STENCIL_DESC desc(D3D11_DEFAULT);
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        desc.DepthEnable    = false;
        d3dDevice->CreateDepthStencilState(&desc, &mAVSMCaptureDepthStencilState);
    }
    

    // Create hair capture depth-stencil state
    {
        CD3D11_DEPTH_STENCIL_DESC desc(D3D11_DEFAULT);
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        d3dDevice->CreateDepthStencilState(&desc, &mHairCaptureDepthStencilState);
    }

    // Create hair render depth-stencil state
    {
        CD3D11_DEPTH_STENCIL_DESC desc(D3D11_DEFAULT);
        desc.DepthEnable = true;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        d3dDevice->CreateDepthStencilState(&desc, &mHairRenderDepthStencilState);
    }

    // Create geometry phase blend state
    {
        CD3D11_BLEND_DESC desc(D3D11_DEFAULT);
        d3dDevice->CreateBlendState(&desc, &mGeometryOpaqueBlendState);
    }

    {
        CD3D11_BLEND_DESC desc(D3D11_DEFAULT);
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        d3dDevice->CreateBlendState(&desc, &mGeometryTransparentBlendState);
    }
    {
        CD3D11_BLEND_DESC desc(D3D11_DEFAULT);
        desc.RenderTarget[0].BlendEnable     = true;
        desc.RenderTarget[0].SrcBlend        = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].DestBlend       = D3D11_BLEND_SRC_COLOR;
        desc.RenderTarget[0].BlendOp         = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha   = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].DestBlendAlpha  = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].BlendOpAlpha    = D3D11_BLEND_OP_ADD;
        d3dDevice->CreateBlendState(&desc, &mMultiplicativeBlendState);
    }
    {
        CD3D11_BLEND_DESC desc(D3D11_DEFAULT);
        desc.RenderTarget[0].BlendEnable     = true;
        desc.RenderTarget[0].SrcBlend        = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlend       = D3D11_BLEND_ONE;
        desc.RenderTarget[0].BlendOp         = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha   = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha  = D3D11_BLEND_ONE;
        desc.RenderTarget[0].BlendOpAlpha    = D3D11_BLEND_OP_ADD;
        d3dDevice->CreateBlendState(&desc, &mAdditiveBlendState);
    }

    // Create lighting phase blend state
    {
        CD3D11_BLEND_DESC desc(D3D11_DEFAULT);
        // Additive blending
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        d3dDevice->CreateBlendState(&desc, &mLightingBlendState);
    }

    // Create alpha phase blend state
    {
        CD3D11_BLEND_DESC desc(D3D11_DEFAULT);
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        d3dDevice->CreateBlendState(&desc, &mParticleBlendState);
    }

    // Create OIT blend state
    {
        CD3D11_BLEND_DESC desc(D3D11_DEFAULT);
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        d3dDevice->CreateBlendState(&desc, &mOITBlendState);
    }

    // Create hair render blend state
    {
        CD3D11_BLEND_DESC desc(D3D11_DEFAULT);
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        d3dDevice->CreateBlendState(&desc, &mHairRenderBlendState);
    }


    // Create constant buffers
    {
        CD3D11_BUFFER_DESC desc(
            sizeof(PerFrameConstants),
            D3D11_BIND_CONSTANT_BUFFER,
            D3D11_USAGE_DYNAMIC,
            D3D11_CPU_ACCESS_WRITE);

        d3dDevice->CreateBuffer(&desc, 0, &mPerFrameConstants);
    }
    {
        CD3D11_BUFFER_DESC desc(
            sizeof(ParticlePerFrameConstants),
            D3D11_BIND_CONSTANT_BUFFER,
            D3D11_USAGE_DYNAMIC,
            D3D11_CPU_ACCESS_WRITE);

        d3dDevice->CreateBuffer(&desc, 0, &mParticlePerFrameConstants);
    }
    {
        CD3D11_BUFFER_DESC desc(
            sizeof(ParticlePerPassConstants),
            D3D11_BIND_CONSTANT_BUFFER,
            D3D11_USAGE_DYNAMIC,
            D3D11_CPU_ACCESS_WRITE);

        d3dDevice->CreateBuffer(&desc, 0, &mParticlePerPassConstants);
    }
    {
        CD3D11_BUFFER_DESC desc(
            sizeof(LT_Constants),
            D3D11_BIND_CONSTANT_BUFFER,
            D3D11_USAGE_DYNAMIC,
            D3D11_CPU_ACCESS_WRITE);

        d3dDevice->CreateBuffer(&desc, 0, &mListTextureConstants);
    }
    {
        CD3D11_BUFFER_DESC desc(
            sizeof(FL_Constants),
            D3D11_BIND_CONSTANT_BUFFER,
            D3D11_USAGE_DYNAMIC,
            D3D11_CPU_ACCESS_WRITE);

        d3dDevice->CreateBuffer(&desc, 0, &mFragmentListConstants);
    }
    {
        CD3D11_BUFFER_DESC desc(
            sizeof(AVSMConstants),
            D3D11_BIND_CONSTANT_BUFFER,
            D3D11_USAGE_DYNAMIC,
            D3D11_CPU_ACCESS_WRITE);

        d3dDevice->CreateBuffer(&desc, 0, &mAVSMConstants);
    }

    {
        CD3D11_BUFFER_DESC desc(
            sizeof(VolumeShadowConstants),
            D3D11_BIND_CONSTANT_BUFFER,
            D3D11_USAGE_DYNAMIC,
            D3D11_CPU_ACCESS_WRITE);

        d3dDevice->CreateBuffer(&desc, 0, &mVolumeShadowConstants);
    }

    {
        CD3D11_BUFFER_DESC desc(
            sizeof(HairConstants),
            D3D11_BIND_CONSTANT_BUFFER,
            D3D11_USAGE_DYNAMIC,
            D3D11_CPU_ACCESS_WRITE);

        d3dDevice->CreateBuffer(&desc, 0, &mHairConstants);
    }

    {
        CD3D11_BUFFER_DESC desc(
            sizeof(HairLTConstants),
            D3D11_BIND_CONSTANT_BUFFER,
            D3D11_USAGE_DYNAMIC,
            D3D11_CPU_ACCESS_WRITE);

        d3dDevice->CreateBuffer(&desc, 0, &mHairLTConstants);
    }

    // Create sampler state
    {
        CD3D11_SAMPLER_DESC desc(D3D11_DEFAULT);
        desc.Filter = D3D11_FILTER_ANISOTROPIC;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.MaxAnisotropy = 16;
        d3dDevice->CreateSamplerState(&desc, &mDiffuseSampler);
    }
    {
        CD3D11_SAMPLER_DESC desc(D3D11_DEFAULT);
        desc.Filter = D3D11_FILTER_ANISOTROPIC;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.MaxAnisotropy = 16;
        d3dDevice->CreateSamplerState(&desc, &mIBLSampler);
    }
    {
        CD3D11_SAMPLER_DESC desc(D3D11_DEFAULT);
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        d3dDevice->CreateSamplerState(&desc, &mAVSMSampler);
    }

    // Create AVSM textures and viewport
    {
        DXGI_SAMPLE_DESC sampleDesc;
        sampleDesc.Count = 1;
        sampleDesc.Quality = 0;
        mAVSMTextures = shared_ptr<Texture2D>(new Texture2D(
            d3dDevice, mAVSMShadowTextureDim, mAVSMShadowTextureDim, DXGI_FORMAT_R32G32B32A32_FLOAT,
            D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, MAX_AVSM_RT_COUNT, sampleDesc,
            D3D11_RTV_DIMENSION_TEXTURE2DARRAY, D3D11_UAV_DIMENSION_TEXTURE2DARRAY, D3D11_SRV_DIMENSION_TEXTURE2DARRAY));

        mAVSMShadowViewport.Width    = static_cast<float>(mAVSMShadowTextureDim);
        mAVSMShadowViewport.Height   = static_cast<float>(mAVSMShadowTextureDim);
        mAVSMShadowViewport.MinDepth = 0.0f;
        mAVSMShadowViewport.MaxDepth = 1.0f;
        mAVSMShadowViewport.TopLeftX = 0.0f;
        mAVSMShadowViewport.TopLeftY = 0.0f;
    }

    // Create some debug (visualization) resources
    {
        HRESULT hr;
        UINT structSize = sizeof(DebugDrawIndirectArgs);

        CD3D11_BUFFER_DESC desc(
            structSize,
            D3D11_BIND_UNORDERED_ACCESS,
            D3D11_USAGE_DEFAULT,
            0,
            D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS,
            0);
        V(d3dDevice->CreateBuffer(&desc, 0, &mDebugDrawIndirectArgs));

        CD3D11_UNORDERED_ACCESS_VIEW_DESC unorderedAccessResourceDesc(
            D3D11_UAV_DIMENSION_BUFFER,
            DXGI_FORMAT_R32_UINT,
            0, 4, 1, 0);
        V(d3dDevice->CreateUnorderedAccessView(mDebugDrawIndirectArgs, 
                                               &unorderedAccessResourceDesc, 
                                               &mDebugDrawIndirectArgsUAV));
        CD3D11_BUFFER_DESC descs(
            structSize,
            0,
            D3D11_USAGE_STAGING,
            D3D10_CPU_ACCESS_READ,
            0,
            0);
        V(d3dDevice->CreateBuffer(&descs, 0, &mDebugDrawIndirectArgsStag));
    }
    {
        HRESULT hr;
        const UINT structSize = sizeof(DebugNodeData);
        const UINT elemCount = 1<<17;

        CD3D11_BUFFER_DESC desc(
            structSize * elemCount,
            D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS,
            D3D11_USAGE_DEFAULT,
            0,
            D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
            structSize);
        V(d3dDevice->CreateBuffer(&desc, 0, &mDebugDrawNodeData));

        CD3D11_BUFFER_DESC descs(
            structSize * elemCount,
            0,
            D3D11_USAGE_STAGING,
            D3D10_CPU_ACCESS_READ,
            0,
            0);
        V(d3dDevice->CreateBuffer(&descs, 0, &mDebugDrawNodeDataStag));

        CD3D11_UNORDERED_ACCESS_VIEW_DESC unorderedAccessResourceDesc(
            D3D11_UAV_DIMENSION_BUFFER,
            DXGI_FORMAT_UNKNOWN,
            0, DEBUG_MAX_NODE_COUNT, 1, 0);
        V(d3dDevice->CreateUnorderedAccessView(mDebugDrawNodeData, 
                                               &unorderedAccessResourceDesc, 
                                               &mDebugDrawNodeDataUAV));

       CD3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceDesc(
            D3D11_SRV_DIMENSION_BUFFER,
            DXGI_FORMAT_UNKNOWN,
            0, DEBUG_MAX_NODE_COUNT, 1);
       V(d3dDevice->CreateShaderResourceView(mDebugDrawNodeData, 
                                             &shaderResourceDesc, 
                                             &mDebugDrawNodeDataSRV));
    }
    {
        CD3D11_BUFFER_DESC desc(
            sizeof(DebugDrawShaderConstants),
            D3D11_BIND_CONSTANT_BUFFER,
            D3D11_USAGE_DYNAMIC,
            D3D11_CPU_ACCESS_WRITE);

        d3dDevice->CreateBuffer(&desc, 0, &mDebugDrawShaderConstants);
    }

    // Create fragment stats resources
    {
        HRESULT hr;
        UINT structSize = sizeof(FragmentStats);

        CD3D11_BUFFER_DESC desc(
            structSize * 1,
            D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS,
            D3D11_USAGE_DEFAULT,
            0,
            D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
            structSize);
        V(d3dDevice->CreateBuffer(&desc, 0, &mFragmentStats));

        CD3D11_UNORDERED_ACCESS_VIEW_DESC unorderedAccessResourceDesc(
            D3D11_UAV_DIMENSION_BUFFER,
            DXGI_FORMAT_UNKNOWN,
            0, 1, 1, 0);
        V(d3dDevice->CreateUnorderedAccessView(mFragmentStats, &unorderedAccessResourceDesc, &mFragmentStatsUAV));

        CD3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceDesc(
            D3D11_SRV_DIMENSION_BUFFER,
            DXGI_FORMAT_UNKNOWN,
            0, 1, 1);
        V(d3dDevice->CreateShaderResourceView(mFragmentStats, &shaderResourceDesc, &mFragmentStatsSRV));

        CD3D11_BUFFER_DESC descStaging(
            structSize * 1,
            0,
            D3D11_USAGE_STAGING,
            D3D10_CPU_ACCESS_READ,
            D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
            structSize);
        V(d3dDevice->CreateBuffer(&descStaging, 0, &mFragmentStatsStaging));
    }

    // Create List Texture first segment node offset texture
    {
        DXGI_SAMPLE_DESC sampleDesc;
        sampleDesc.Count = 1;
        sampleDesc.Quality = 0;
        mListTexFirstSegmentNodeOffset = shared_ptr<Texture2D>(new Texture2D(
            d3dDevice, mAVSMShadowTextureDim, mAVSMShadowTextureDim, DXGI_FORMAT_R32_UINT,
            D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE, 1, sampleDesc,
            D3D11_RTV_DIMENSION_UNKNOWN, D3D11_UAV_DIMENSION_TEXTURE2D, D3D11_SRV_DIMENSION_TEXTURE2D));
    }

    // Create List Texture segment nodes buffer for AVSM data capture
    {
        HRESULT hr;
        UINT structSize = sizeof(SegmentNode);

        CD3D11_BUFFER_DESC desc(
            structSize * mLisTexNodeCount,            
            D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS,
            D3D11_USAGE_DEFAULT,
            0,
            D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
            structSize);
        V(d3dDevice->CreateBuffer(&desc, 0, &mListTexSegmentNodes));

        CD3D11_UNORDERED_ACCESS_VIEW_DESC unorderedAccessResourceDesc(
            D3D11_UAV_DIMENSION_BUFFER,
            DXGI_FORMAT_UNKNOWN,
            0, mLisTexNodeCount, 1, D3D11_BUFFER_UAV_FLAG_COUNTER);
        V(d3dDevice->CreateUnorderedAccessView(mListTexSegmentNodes, &unorderedAccessResourceDesc, &mListTexSegmentNodesUAV));

        CD3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceDesc(
            D3D11_SRV_DIMENSION_BUFFER,
            DXGI_FORMAT_UNKNOWN,
            0, mLisTexNodeCount, 1);
        V(d3dDevice->CreateShaderResourceView(mListTexSegmentNodes, &shaderResourceDesc, &mListTexSegmentNodesSRV));
    }

    // Create fragment list buffers for OIT methods
    {
        HRESULT hr;
        UINT structSize = sizeof(FragmentNode);

        const unsigned int nodeCount = mLisTexNodeCount * 2;

        CD3D11_BUFFER_DESC desc(
            structSize * nodeCount,            
            D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS,
            D3D11_USAGE_DEFAULT,
            0,
            D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
            structSize);
        V(d3dDevice->CreateBuffer(&desc, 0, &mFragmentListNodes));

        CD3D11_UNORDERED_ACCESS_VIEW_DESC unorderedAccessResourceDesc(
            D3D11_UAV_DIMENSION_BUFFER,
            DXGI_FORMAT_UNKNOWN,
            0, nodeCount, 1, D3D11_BUFFER_UAV_FLAG_COUNTER);
        V(d3dDevice->CreateUnorderedAccessView(mFragmentListNodes, &unorderedAccessResourceDesc, &mFragmentListNodesUAV));

        CD3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceDesc(
            D3D11_SRV_DIMENSION_BUFFER,
            DXGI_FORMAT_UNKNOWN,
            0, nodeCount, 1);
        V(d3dDevice->CreateShaderResourceView(mFragmentListNodes, &shaderResourceDesc, &mFragmentListNodesSRV));
    }
   
    // Create skybox mesh and texture
    {
        mSkyboxMesh.Create(d3dDevice, L"media\\Skybox\\Skybox.sdkmesh");
        HRESULT hr;
        hr = D3DX11CreateTextureFromFile(
            d3dDevice, 
            L"media\\Skybox\\Clouds.dds", 
            0, 0, (ID3D11Resource**)&mSkyboxTexture, 0);
        assert(SUCCEEDED(hr));

        d3dDevice->CreateShaderResourceView(mSkyboxTexture, 0, &mSkyboxSRV);
    }

    // Create IBL texture
    {
        HRESULT hr;
        hr = D3DX11CreateTextureFromFile(
            d3dDevice, 
            L"media\\SkyBox\\BlurredLightProbe.dds", 
            0, 0, (ID3D11Resource**)&mIBLTexture, 0);
        assert(SUCCEEDED(hr));

        d3dDevice->CreateShaderResourceView(mIBLTexture, 0, &mIBLSRV);
    }

    // Create GPU queries
    {
        D3D11_QUERY_DESC desc0 = {D3D11_QUERY_TIMESTAMP, 0};
        D3D11_QUERY_DESC desc1 = {D3D11_QUERY_TIMESTAMP_DISJOINT, 0};
        for (UINT i = 0; i < GPUQ_QUERY_COUNT; ++i) {
            d3dDevice->CreateQuery(&desc0, &mGPUQuery[i][0]);
            d3dDevice->CreateQuery(&desc0, &mGPUQuery[i][1]);
            d3dDevice->CreateQuery(&desc1, &mGPUQuery[i][2]);
        }
    }

    mHair = new Hair(d3dDevice, d3dDeviceContext, "Media/hair/dark.hair", shaderDefines);
}

App::~App() 
{
    // Releae queries
    for (UINT i = 0; i < GPUQ_QUERY_COUNT; ++i) {
        SAFE_RELEASE(mGPUQuery[i][0]);
        SAFE_RELEASE(mGPUQuery[i][1]);
        SAFE_RELEASE(mGPUQuery[i][2]);
    }

    SAFE_RELEASE(mSkyboxSRV);
    SAFE_RELEASE(mSkyboxTexture);
    mSkyboxMesh.Destroy();

    SAFE_RELEASE(mIBLSRV);
    SAFE_RELEASE(mIBLTexture);

    SAFE_RELEASE(mParticleDepthStencilState);
    SAFE_RELEASE(mDefaultDepthStencilState);
    SAFE_RELEASE(mDepthDisabledDepthStencilState);
    SAFE_RELEASE(mDepthWritesDisabledDepthStencilState);
    SAFE_RELEASE(mAVSMCaptureDepthStencilState);
    SAFE_RELEASE(mAVSMSampler);
    SAFE_RELEASE(mDiffuseSampler);
    SAFE_RELEASE(mIBLSampler);
    SAFE_RELEASE(mDebugDrawShaderConstants);
    SAFE_RELEASE(mPerFrameConstants);
    SAFE_RELEASE(mParticlePerFrameConstants);
    SAFE_RELEASE(mParticlePerPassConstants);
    SAFE_RELEASE(mListTextureConstants);
    SAFE_RELEASE(mFragmentListConstants);
    SAFE_RELEASE(mAVSMConstants);
    SAFE_RELEASE(mVolumeShadowConstants);
    SAFE_RELEASE(mHairConstants);
    SAFE_RELEASE(mHairLTConstants);
    SAFE_RELEASE(mLightingBlendState);
    SAFE_RELEASE(mMultiplicativeBlendState);
    SAFE_RELEASE(mAdditiveBlendState);
    SAFE_RELEASE(mGeometryOpaqueBlendState);
    SAFE_RELEASE(mGeometryTransparentBlendState);
    SAFE_RELEASE(mParticleBlendState);
    SAFE_RELEASE(mOITBlendState);
    SAFE_RELEASE(mShadowRasterizerState);
    SAFE_RELEASE(mRasterizerState);
    SAFE_RELEASE(mDoubleSidedRasterizerState);
    SAFE_RELEASE(mParticleRasterizerState);
    SAFE_RELEASE(mHairRasterizerState);
    SAFE_RELEASE(mMeshVertexLayout);
    SAFE_RELEASE(mGeometryVS);
    SAFE_RELEASE(mGeometryVSReflector);
    SAFE_RELEASE(mListTexSegmentNodes);
    SAFE_RELEASE(mListTexSegmentNodesUAV);
    SAFE_RELEASE(mListTexSegmentNodesSRV);
    SAFE_RELEASE(mFragmentListNodes);
    SAFE_RELEASE(mFragmentListNodesUAV);
    SAFE_RELEASE(mFragmentListNodesSRV);
    SAFE_RELEASE(mDebugDrawIndirectArgs);
    SAFE_RELEASE(mDebugDrawIndirectArgsStag);
    SAFE_RELEASE(mDebugDrawIndirectArgsUAV);
    SAFE_RELEASE(mDebugDrawNodeData);
    SAFE_RELEASE(mDebugDrawNodeDataStag);
    SAFE_RELEASE(mDebugDrawNodeDataUAV);
    SAFE_RELEASE(mDebugDrawNodeDataSRV);
    SAFE_RELEASE(mFragmentStats);
    SAFE_RELEASE(mFragmentStatsUAV);
    SAFE_RELEASE(mFragmentStatsSRV);
    SAFE_RELEASE(mFragmentStatsStaging);

    // Hair
    SAFE_RELEASE(mHairCaptureDepthStencilState);
    SAFE_RELEASE(mHairRenderDepthStencilState);
    SAFE_RELEASE(mHairRenderBlendState);

    delete mHair;
}

void App::OnD3D11ResizedSwapChain(ID3D11Device* d3dDevice,
                                  const DXGI_SURFACE_DESC* backBufferDesc)
{
    DXGI_SAMPLE_DESC sampleDesc;
    sampleDesc.Count = mMSAASampleCount;
    sampleDesc.Quality = 0;

    // standard depth buffer
    mDepthBuffer = shared_ptr<Depth2D>(new Depth2D(
        d3dDevice, backBufferDesc->Width, backBufferDesc->Height, D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE, sampleDesc));

    // Create List Texture first node offset texture for OIT
    {
        DXGI_SAMPLE_DESC sampleDesc;
        sampleDesc.Count = 1;
        sampleDesc.Quality = 0;
        mFragmentListFirstNodeOffset = 
            shared_ptr<Texture2D>(
                new Texture2D(d3dDevice, 
                              backBufferDesc->Width, 
                              backBufferDesc->Height, 
                              DXGI_FORMAT_R32_UINT,
                              D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE, 
                              1, 
                              sampleDesc,
                              D3D11_RTV_DIMENSION_UNKNOWN, 
                              D3D11_UAV_DIMENSION_TEXTURE2D, 
                              D3D11_SRV_DIMENSION_TEXTURE2D));
    }
  
    // Create Composite buffer texture for OIT
    {
        DXGI_SAMPLE_DESC sampleDesc;
        sampleDesc.Count = mMSAASampleCount;
        sampleDesc.Quality = 0;
        mCompositeBuffer = 
            shared_ptr<Texture2D>(
                new Texture2D(d3dDevice, 
                              backBufferDesc->Width, 
                              backBufferDesc->Height, 
                              DXGI_FORMAT_R32G32B32A32_FLOAT,
                              D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, 
                              1, 
                              sampleDesc,
                              D3D11_RTV_DIMENSION_TEXTURE2DMS, 
                              D3D11_UAV_DIMENSION_UNKNOWN, 
                              D3D11_SRV_DIMENSION_TEXTURE2DMS));
    }     
}


// Cleanup (aka make the runtime happy)
void Cleanup(ID3D11DeviceContext* d3dDeviceContext)
{
    d3dDeviceContext->GSSetShader(NULL, 0, 0);

    d3dDeviceContext->OMSetRenderTargets(0, 0, 0);       
    d3dDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(0, 0, 0, 0, 0, 0, 0);

    ID3D11ShaderResourceView* nullViews[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                               0, 0, 0, 0, 0, 0, 0, 0};
    d3dDeviceContext->VSSetShaderResources(0, 16, nullViews);
    d3dDeviceContext->GSSetShaderResources(0, 16, nullViews);
    d3dDeviceContext->PSSetShaderResources(0, 16, nullViews);
    d3dDeviceContext->CSSetShaderResources(0, 16, nullViews);

    ID3D11UnorderedAccessView* nullUAViews[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    d3dDeviceContext->CSSetUnorderedAccessViews(0, 8, nullUAViews, 0);
                                               
}

void App::UpdateHairMesh()
{
    mHairMeshDirty = true;
}

void App::SetShadowNodeCount(int nodeCount)
{
    mAVSMShadowNodeCount = nodeCount;
    mShadowShaderIdx = nodeCount / 4 - 1;
}

void App::SetAOITNodeCount(int nodeCount)
{
    mAOITShaderIdx = nodeCount / 8 - 1;
}

void App::SetMSAASampleCount(unsigned int msaaSampleCount)
{
    mMSAASampleCount = msaaSampleCount;
}

extern unsigned int g_framecount;
 
void App::Render(Options &options,
                 ID3D11DeviceContext* d3dDeviceContext, 
                 ID3D11RenderTargetView* backBuffer,
                 CDXUTSDKMesh* mesh,
                 CDXUTSDKMesh* meshAlpha,
                 ParticleSystem *particleSystem,
                 const D3DXMATRIXA16& worldMatrix,
                 CFirstPersonAndOrbitCamera* viewerCamera,
                 CFirstPersonAndOrbitCamera* lightCamera,
                 D3D11_VIEWPORT* viewport,
                 UIConstants* ui,
                 FrameStats* frameStats)
{
    assert(!options.enableParticles || particleSystem);

    FrameMatrices frameMatx;
    frameMatx.worldMatrix = worldMatrix;
    frameMatx.cameraProj = *viewerCamera->GetProjMatrix();
    frameMatx.cameraView = *viewerCamera->GetViewMatrix();
    D3DXMatrixInverse(&frameMatx.cameraViewInv, 0, &frameMatx.cameraView);

    // We only use the view direction from the camera object
    // We then center the directional light on the camera frustum and set the
    // extents to completely cover it.
    frameMatx.lightView = *lightCamera->GetViewMatrix();
    {
        // NOTE: We don't include the projection matrix here, since we want to just get back the
        // raw view-space extents and use that to *build* the bounding projection matrix
        D3DXVECTOR3 min, max;
        ComputeFrustumExtents(frameMatx.cameraViewInv, frameMatx.cameraProj,
                              viewerCamera->GetNearClip(), viewerCamera->GetFarClip(),
                              frameMatx.lightView, &min, &max);

        // First adjust the light matrix to be centered on the extents in x/y and behind everything in z
        // TODO: Use scene AABB to determine where to translate the light to. For now just assume that
        // it's already far enough away.
        D3DXVECTOR3 center = 0.5f * (min + max);
        D3DXMATRIXA16 centerTransform;
        D3DXMatrixTranslation(&centerTransform, -center.x, -center.y, 0.0f);
        frameMatx.lightView *= centerTransform;

        // Now create a projection matrix that covers the extents when centered
        // TODO: Again use scene AABB to decide on light far range - this one can actually clip out
        // any objects further away than the frustum can see if desired.
        D3DXVECTOR3 dimensions = max - min;
        D3DXMatrixOrthoLH(&frameMatx.lightProj, dimensions.x, dimensions.y, 0.0f, 1000.0f);
    }

    // Compute composite matrices
    frameMatx.cameraViewProj = frameMatx.cameraView * frameMatx.cameraProj;
    frameMatx.cameraWorldViewProj = frameMatx.worldMatrix * frameMatx.cameraViewProj;
    frameMatx.cameraWorldView = frameMatx.worldMatrix * frameMatx.cameraView;
    frameMatx.lightViewProj = frameMatx.lightView * frameMatx.lightProj;
    frameMatx.lightWorldViewProj = frameMatx.worldMatrix * frameMatx.lightViewProj;
    frameMatx.cameraViewToLightProj = frameMatx.cameraViewInv * frameMatx.lightViewProj;
    frameMatx.cameraViewToLightView = frameMatx.cameraViewInv * frameMatx.lightView;   

    frameMatx.avsmLightProj = *lightCamera->GetProjMatrix();
    frameMatx.avsmLightView = *lightCamera->GetViewMatrix();

    if (mHairMeshDirty) {
        mHair->UpdateHairMesh(d3dDeviceContext, 
                              options.debugX, options.debugY, options.debugZ);
        mHairMeshDirty = false;
    }

    float hairDepthBounds[2];
    if (options.enableAutoBoundsAVSM) {
        // Get bounding boxes from transparent geometry
        const float bigNumf = 1e10f;
        D3DXVECTOR3 maxBB(-bigNumf, -bigNumf, -bigNumf);
        D3DXVECTOR3 minBB(+bigNumf, +bigNumf, +bigNumf);

        if (options.enableHair ||
            options.enableParticles) {
            // Initialize minBB, maxBB
            for (size_t p = 0; p < 3; ++p) {
                minBB[p] = std::numeric_limits<float>::max();
                maxBB[p] = std::numeric_limits<float>::min();
            }

            if (options.enableHair) {
                D3DXVECTOR3 hairMin, hairMax;
                mHair->GetBBox(&hairMin, &hairMax);

                for (size_t p = 0; p < 3; ++p) {
                    minBB[p] = std::min(minBB[p], hairMin[p]);
                    maxBB[p] = std::max(maxBB[p], hairMax[p]);
                }
            } 

            if (options.enableParticles) {
                D3DXVECTOR3 particleMin, particleMax;
                particleSystem->GetBBox(&particleMin, &particleMax);

                for (size_t p = 0; p < 3; ++p) {
                    minBB[p] = std::min(minBB[p], particleMin[p]);
                    maxBB[p] = std::max(maxBB[p], particleMax[p]);
                }
            }

            TransformBBox(&minBB, &maxBB, frameMatx.avsmLightView);
        }

        // First adjust the light matrix to be centered on the extents in x/y and behind everything in z
        // TODO: Use scene AABB to determine where to translate the light to. For now just assume that
        // it's already far enough away.
        D3DXVECTOR3 center = 0.5f * (minBB + maxBB);
        D3DXMATRIXA16 centerTransform;
        D3DXMatrixTranslation(&centerTransform, -center.x, -center.y, -minBB.z);
        frameMatx.avsmLightView *= centerTransform;

        // Compute depth bounds that wil be used to slice this volume (OSM)
        hairDepthBounds[0] = 0.0f;
        hairDepthBounds[1] = maxBB.z - minBB.z;

        // Now create a projection matrix that covers the extents when centered
        // TODO: Again use scene AABB to decide on light far range - this one can actually clip out
        // any objects further away than the frustum can see if desired.
        D3DXVECTOR3 dimensions = maxBB - minBB;
        D3DXMatrixOrthoLH(&frameMatx.avsmLightProj, dimensions.x, dimensions.y, 0, dimensions.z);
    }
    
    // Compute composite matrices;
    frameMatx.avsmLightViewProj = frameMatx.avsmLightView * frameMatx.avsmLightProj;
    frameMatx.avmsLightWorldViewProj = frameMatx.worldMatrix * frameMatx.avsmLightViewProj;
    frameMatx.cameraViewToAvsmLightProj = frameMatx.cameraViewInv * frameMatx.avsmLightViewProj;
    frameMatx.cameraViewToAvsmLightView = frameMatx.cameraViewInv * frameMatx.avsmLightView;

    // Animate particles
    float particleDepthBounds[2];
    if (options.enableParticles) {
        if (ui->pauseParticleAnimaton == false) {
            // Update particles 
            const float currTime = static_cast<float>(DXUTGetGlobalTimer()->GetAbsoluteTime());
            float deltaTime = 0.009; //currTime - mLastTime;
            mLastTime = currTime;
            particleSystem->UpdateParticles(viewerCamera, lightCamera, deltaTime);
        }
                
        particleSystem->SortParticles(particleDepthBounds, &frameMatx.avsmLightView, false, 16);
        particleSystem->PopulateVertexBuffers(d3dDeviceContext);

        // Fill in particle emitter (per frame) constants
        {
            D3D11_MAPPED_SUBRESOURCE mappedResource;
            d3dDeviceContext->Map(mParticlePerFrameConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
            ParticlePerFrameConstants* constants = static_cast<ParticlePerFrameConstants *>(mappedResource.pData);

            constants->mScale                        = 1.0f;
            constants->mParticleSize                 = ((float)ui->particleSize / 3.0f);
            constants->mParticleAlpha                = 1.0f;
            constants->mbSoftParticles               = 1.0f;
            constants->mParticleOpacity              = 0.8f * ((float)ui->particleOpacity / 33.0f);
            constants->mSoftParticlesSaturationDepth = 1.0f;

            d3dDeviceContext->Unmap(mParticlePerFrameConstants, 0);
        }
    }

    // Consolodate depth bounds
    if (options.enableHair && options.enableParticles) {
        float minDepth = std::min(particleDepthBounds[0], hairDepthBounds[0]);
        float maxDepth = std::max(particleDepthBounds[1], hairDepthBounds[1]);
        particleDepthBounds[0] = hairDepthBounds[0] = minDepth;
        particleDepthBounds[1] = hairDepthBounds[1] = maxDepth;
    }

    D3DXVECTOR4 cameraPos  = D3DXVECTOR4(*viewerCamera->GetEyePt(), 1.0f);
    FillInFrameConstants(d3dDeviceContext, frameMatx, cameraPos, lightCamera,  ui, options);

    mesh->SetInFrustumFlags(true);
    meshAlpha->SetInFrustumFlags(true);

	// Render shadow maps
    if (ui->volumeShadowMethod && options.enableVolumeShadowCreation) 
    {
        // This phase computes a visibility representation for our shadow volume
        // Setup constants buffers
        FillListTextureConstants(d3dDeviceContext);
        FillAVSMConstants(d3dDeviceContext);

        ClearShadowBuffers(d3dDeviceContext, ui);

        // First pass, capture all fragments
        if (options.enableHair) {
            FillHairConstants(d3dDeviceContext,
                              frameMatx.avsmLightProj,
                              frameMatx.avsmLightView,
                              frameMatx.avsmLightViewProj);
            CaptureHair(d3dDeviceContext, ui, true);
        } 
        
        if (options.enableParticles) {
            FillParticleRendererConstants(d3dDeviceContext, 
                                          lightCamera, 
                                          frameMatx.avsmLightView, 
                                          frameMatx.avsmLightViewProj);
            CaptureParticles(d3dDeviceContext, particleSystem, ui, !options.enableHair);
        }

        // Second pass, generate shadow visibility curve (AVSM)
        GenerateVisibilityCurve(d3dDeviceContext, ui);            
    }

    mesh->SetInFrustumFlags(true);
    meshAlpha->SetInFrustumFlags(true);
    if (options.enableParticles) {
        FillParticleRendererConstants(d3dDeviceContext,
                                      viewerCamera,
                                      frameMatx.cameraView,
                                      frameMatx.cameraViewProj);
    }
    if (options.enableHair) {
        FillHairConstants(d3dDeviceContext,
                          frameMatx.cameraProj,
                          frameMatx.cameraView,
                          frameMatx.cameraViewProj);
    }

    // Start building the final frame
    ID3D11RenderTargetView* finalRTV = backBuffer;

    float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	d3dDeviceContext->ClearRenderTargetView(finalRTV, bgColor);
    d3dDeviceContext->ClearDepthStencilView(mDepthBuffer->GetDepthStencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0x0);

    

    // Render Skybox and Opaque objects
    RenderSkybox(d3dDeviceContext, finalRTV, viewport);

    if (!options.enableTransparency) {
        RenderStandardGeometry(options, d3dDeviceContext, finalRTV, mesh, viewport);   
        mesh->SetInFrustumFlags(true);
    }

    if (meshAlpha && meshAlpha->IsLoaded() && options.enableAlphaCutoff) {
        Options tempOptions = options;
        tempOptions.enableTransparency = false;
        RenderStandardGeometry(tempOptions, d3dDeviceContext, finalRTV, meshAlpha, viewport, true);   
        meshAlpha->SetInFrustumFlags(true);
    }      


    // Render transparent geometry with AOIT, A-buffer or plain alpha-blending
    bool vrblMemOITEnabled = RenderTransparentGeometry(options, frameMatx, d3dDeviceContext, 
                                                       finalRTV, mesh, meshAlpha, 
                                                       particleSystem, viewport, ui);
 
    if (options.enableStats) {
        d3dDeviceContext->End(mGPUQuery[GPUQ_RESOLVE_PASS][2]);
        d3dDeviceContext->End(mGPUQuery[GPUQ_RESOLVE_PASS][1]);
        GatherFragmentStats(options, ui, d3dDeviceContext, finalRTV, viewport, frameStats, vrblMemOITEnabled);
    }

    bool visualizeVisibilityFn = options.visualizeAOIT || options.visualizeABuffer;
    if (vrblMemOITEnabled && visualizeVisibilityFn) {       
        VisualizeTransmittance(d3dDeviceContext, frameMatx, viewport, options, ui, backBuffer);
    }
}

bool App::RenderTransparentGeometry(Options &options,
                                    FrameMatrices &frameMatx,
                                    ID3D11DeviceContext* d3dDeviceContext,
                                    ID3D11RenderTargetView* finalRTV,
                                    CDXUTSDKMesh* mesh,
                                    CDXUTSDKMesh* meshAlpha,
                                    ParticleSystem *particleSystem,
                                    D3D11_VIEWPORT* viewport,
                                    UIConstants* ui)
{  
    bool const vrblMemOITEnabled =  ((options.transparencyMethod == TM_ABUFFER_SORTED)     ||
                                     (options.transparencyMethod == TM_ABUFFER_ITERATIVE)  ||
                                     (options.transparencyMethod == TM_AOIT));

    bool const renderMeshAlphaTransparent = meshAlpha && meshAlpha->IsLoaded() && !options.enableAlphaCutoff;

    // Clear OIT buffers if we are rendering transparent geometry with a method that requires to capture data in a list    
    if (vrblMemOITEnabled) {
        ClearOITBuffers(d3dDeviceContext, options.transparencyMethod);
        FillFragmentListConstants(d3dDeviceContext, mLisTexNodeCount * 2);
    }    
    
    if (options.enableStats) {
        d3dDeviceContext->Begin(mGPUQuery[GPUQ_CAPTURE_PASS][2]);
        d3dDeviceContext->End(mGPUQuery[GPUQ_CAPTURE_PASS][0]);
    }

    int tileCount = options.enableTiling ? 4 : 0;
    int tile = vrblMemOITEnabled ? tileCount - 1 : -1;
    // Render transparent objects
    do {
	    RenderStandardGeometry(options, d3dDeviceContext, finalRTV, mesh, viewport, false, true, tile);   
        mesh->SetInFrustumFlags(true);

        if (renderMeshAlphaTransparent) {
            Options tempOptions = options;
            tempOptions.enableTransparency = true;
            RenderStandardGeometry(tempOptions, d3dDeviceContext, finalRTV, meshAlpha, viewport, true, !options.enableTransparency, tile);   
            meshAlpha->SetInFrustumFlags(true);
        }

        if (options.enableHair) {
            Options tempOptions = options;
            if (meshAlpha->IsLoaded()) {
                tempOptions.enableTransparency = true;
            }
            RenderHair(d3dDeviceContext, finalRTV, viewport, 
                        frameMatx.cameraWorldView, ui, tempOptions);
        }

        if (options.enableParticles) {
            // Fill particle renderer constants and shade particles
            Options tempOptions = options;
            if (meshAlpha->IsLoaded()) {
                tempOptions.enableTransparency = true;
            }
            ShadeParticles(d3dDeviceContext, particleSystem, finalRTV, viewport, tempOptions, ui);            
        }
    
        if (options.enableStats) {
            d3dDeviceContext->End(mGPUQuery[GPUQ_CAPTURE_PASS][2]);
            d3dDeviceContext->End(mGPUQuery[GPUQ_CAPTURE_PASS][1]);
        }

        // Transparency Resolves
        // Read fragments from a list for each pixel and composite them
        if (vrblMemOITEnabled) {
            if (options.enableStats) {
                d3dDeviceContext->Begin(mGPUQuery[GPUQ_RESOLVE_PASS][2]);
                d3dDeviceContext->End(mGPUQuery[GPUQ_RESOLVE_PASS][0]);
            }

            // Setup a full-screen pass
            d3dDeviceContext->IASetInputLayout(0);
            d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            d3dDeviceContext->IASetVertexBuffers(0, 0, 0, 0, 0);

            d3dDeviceContext->VSSetShader(mFullScreenTriangleVS, 0, 0);

            d3dDeviceContext->RSSetState(mDoubleSidedRasterizerState);
            d3dDeviceContext->RSSetViewports(1, viewport);   

            D3D10_RECT rects[1];
            LONG dx = (LONG)viewport->Width / 2;
            LONG dy = (LONG)viewport->Height / 2;

            switch (tile) {        
                case 0:
                    rects[0].left   = 0;
                    rects[0].right  = dx;
                    rects[0].top    = 0;
                    rects[0].bottom = dy;
                    break;
                case 1:
                    rects[0].left   = dx;
                    rects[0].right  = 2 * dx;
                    rects[0].top    = 0;
                    rects[0].bottom = dy;
                    break;
                case 2:
                    rects[0].left   = dx;
                    rects[0].right  = 2 * dx;
                    rects[0].top    = dy;
                    rects[0].bottom = 2 * dy;
                    break;
                case 3:
                    rects[0].left   = 0;
                    rects[0].right  = dx;
                    rects[0].top    = dy;
                    rects[0].bottom = 2 * dy;
                    break;
                default:
                    rects[0].left   = 0;
                    rects[0].right  = 2 * dx;
                    rects[0].top    = 0;
                    rects[0].bottom = 2 * dy;
            }

            d3dDeviceContext->RSSetScissorRects( 1, rects );

            ID3D11ShaderResourceView* fragmentListFirstNodeOffsetSRV = mFragmentListFirstNodeOffset->GetShaderResource();

            ID3D11PixelShader* shader = NULL;
            ID3D11ShaderReflection* shaderReflector = NULL;
            UINT msaaIdx = options.msaaSampleCount > 1 ? 1 : 0;
            switch (options.transparencyMethod) {
                case TM_ABUFFER_SORTED: 
                {
                    shader = mABufferSortedResolvePS[msaaIdx];
                    shaderReflector = mABufferSortedResolvePSReflector[msaaIdx];
                    break;
                }
                case TM_ABUFFER_ITERATIVE: 
                {
                    shader = mABufferResolvePS[msaaIdx];
                    shaderReflector = mABufferResolvePSReflector[msaaIdx];
                    break;
                }
                case TM_AOIT:
                {
                    shader = mAOITResolvePS[4 * msaaIdx + mAOITShaderIdx];
                    shaderReflector = mAOITResolvePSReflector[4 * msaaIdx + mAOITShaderIdx];
                    break;
                }
                default:
                    break;
            }

            d3dDeviceContext->PSSetShader(shader, 0, 0);

            if (options.transparencyMethod != TM_ABUFFER_SORTED &&
                options.transparencyMethod != TM_ABUFFER_ITERATIVE &&
                options.transparencyMethod != TM_AOIT) {
                PSSetConstantBuffers(d3dDeviceContext, shaderReflector, "FL_Constants", 1, &mFragmentListConstants);
            }
            PSSetShaderResources(d3dDeviceContext, shaderReflector, "gFragmentListFirstNodeAddressSRV", 1, &fragmentListFirstNodeOffsetSRV);
            PSSetShaderResources(d3dDeviceContext, shaderReflector, "gFragmentListNodesSRV", 1,&mFragmentListNodesSRV);          

            d3dDeviceContext->OMSetDepthStencilState(mDefaultDepthStencilState, 0x0);
       
            d3dDeviceContext->OMSetBlendState(mOITBlendState, 0, 0xffffffff);
            d3dDeviceContext->OMSetRenderTargets(1, &finalRTV, mDepthBuffer->GetDepthStencil());

            // Kick full-screen resolve pass that will resolve all transparent fragments
            d3dDeviceContext->Draw(3, 0);           
        
            Cleanup(d3dDeviceContext);
        }

        tile--;
    } while (tile >= 0);

    // Restore full screen viewport
    D3D10_RECT rects[1];
    rects[0].left   = 0;
    rects[0].right  = (LONG)viewport->Width;
    rects[0].top    = 0;
    rects[0].bottom = (LONG)viewport->Height;
    d3dDeviceContext->RSSetScissorRects( 1, rects );    

    return vrblMemOITEnabled;
}

void App::VisualizeTransmittance(ID3D11DeviceContext* d3dDeviceContext,
                                 const FrameMatrices &frameMatx,
                                 const D3D11_VIEWPORT* viewport,
                                 Options &options,
                                 const UIConstants* ui,
                                 ID3D11RenderTargetView* backBuffer)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    d3dDeviceContext->Map(mDebugDrawShaderConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    DebugDrawShaderConstants* constants = static_cast<DebugDrawShaderConstants*>(mappedResource.pData);
    constants->pixelCoordX = (float) options.pickedX;
    constants->pixelCoordY = (float) options.pickedY;
    constants->cameraProj = frameMatx.cameraProj;

    const float lineAlpha = 0.5f;

    // red (A-Buffer / Yang et al.)
    constants->lineColor[4 * DEBUG_DRAW_ABUFFER_COLOR_IDX + 0] = 1.0f;
    constants->lineColor[4 * DEBUG_DRAW_ABUFFER_COLOR_IDX + 1] = 0.0f;
    constants->lineColor[4 * DEBUG_DRAW_ABUFFER_COLOR_IDX + 2] = 0.0f;
    constants->lineColor[4 * DEBUG_DRAW_ABUFFER_COLOR_IDX + 3] = lineAlpha;

    // blue (AOIT)
    constants->lineColor[4 * DEBUG_DRAW_AOIT_COLOR_IDX + 0] = 0.0f;
    constants->lineColor[4 * DEBUG_DRAW_AOIT_COLOR_IDX + 1] = 0.0f;
    constants->lineColor[4 * DEBUG_DRAW_AOIT_COLOR_IDX + 2] = 1.0f;
    constants->lineColor[4 * DEBUG_DRAW_AOIT_COLOR_IDX + 3] = lineAlpha;

    constants->enableABuffer = (unsigned int)options.visualizeABuffer;
    constants->enableAOIT    = (unsigned int)options.visualizeAOIT;

    d3dDeviceContext->Unmap(mDebugDrawShaderConstants, 0);
        
    ID3D11ComputeShader* shaderArray[] = {
        mGatherDebugDataCS,
        mGatherDebugAOITDataCS[mAOITShaderIdx],
    };

    ID3D11ShaderReflection* reflArray[] = {
        mGatherDebugDataCSReflector,
        mGatherDebugAOITDataCSReflector[mAOITShaderIdx],
    }; 

    for (int i = 0; i < sizeof(shaderArray) / sizeof(shaderArray[0]); i++) {
        ID3D11ShaderResourceView* fragmentListFirstNodeOffsetSRV = mFragmentListFirstNodeOffset->GetShaderResource();
        CSSetShaderResources(d3dDeviceContext, reflArray[i], 
                                "gFragmentListFirstNodeAddressSRV", 1, &fragmentListFirstNodeOffsetSRV);
        CSSetShaderResources(d3dDeviceContext, reflArray[i],
                                "gFragmentListNodesSRV", 1,&mFragmentListNodesSRV);
        if (i != 1) {
        CSSetUnorderedAccessViews(d3dDeviceContext,  reflArray[i], 
                                    "gFragmentStatsUAV", 1, &mFragmentStatsUAV, NULL);
        }
        CSSetUnorderedAccessViews(d3dDeviceContext,  reflArray[i], 
                                    "gDebugDrawNodeDataUAV", 1, &mDebugDrawNodeDataUAV, NULL);
        CSSetUnorderedAccessViews(d3dDeviceContext,  reflArray[i], 
                                    "gDebugDrawIndirectArgsUAV", 1, &mDebugDrawIndirectArgsUAV, NULL);
        CSSetConstantBuffers(d3dDeviceContext, reflArray[i], 
                                "DebugDrawShaderConstants", 1, &mDebugDrawShaderConstants);
            
        d3dDeviceContext->CSSetShader(shaderArray[i], NULL, 0);
        d3dDeviceContext->Dispatch(1, 1, 1);        
        d3dDeviceContext->Flush();      
        Cleanup(d3dDeviceContext);
    }        

    D3D11_VIEWPORT localViewPort;
    localViewPort.Width     = 0.75f * viewport->Width;
    localViewPort.Height    = 0.50f * viewport->Height;
    localViewPort.TopLeftX  = 4.f;
    localViewPort.TopLeftY  = viewport->Height - localViewPort.Height - localViewPort.TopLeftX;
    localViewPort.MinDepth  = 0.0f;
    localViewPort.MaxDepth  = 1.0f;

    d3dDeviceContext->IASetInputLayout(0);
    d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    d3dDeviceContext->IASetVertexBuffers(0, 0, 0, 0, 0);

    d3dDeviceContext->VSSetShader(mDebugDataVS, 0, 0);
    VSSetShaderResources(d3dDeviceContext, mDebugDataVSReflector,
                            "gFragmentStatsSRV", 1,&mFragmentStatsSRV);
    VSSetShaderResources(d3dDeviceContext, mDebugDataVSReflector,
                            "gDebugNodeDataSRV", 1,&mDebugDrawNodeDataSRV);

    d3dDeviceContext->GSSetShader(0, 0, 0);

    d3dDeviceContext->PSSetShader(mDebugDataPS, 0, 0);

    d3dDeviceContext->RSSetState(mRasterizerState);
    d3dDeviceContext->RSSetViewports(1, &localViewPort);

    d3dDeviceContext->OMSetBlendState(mGeometryOpaqueBlendState, 0, 0xffffffff);
    d3dDeviceContext->OMSetRenderTargets(1, &backBuffer, NULL);
    d3dDeviceContext->DrawInstancedIndirect(mDebugDrawIndirectArgs, 0);

    // Debug code
    static bool DebugWatchBuffer = false;
    if (DebugWatchBuffer) {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        d3dDeviceContext->CopyResource(mDebugDrawIndirectArgsStag, mDebugDrawIndirectArgs);
        d3dDeviceContext->Map(mDebugDrawIndirectArgsStag, 0, D3D11_MAP_READ, 0, &mappedResource);
        unsigned int* args = static_cast<unsigned int*>(mappedResource.pData);

        d3dDeviceContext->CopyResource(mDebugDrawNodeDataStag, mDebugDrawNodeData);      
        d3dDeviceContext->Map(mDebugDrawNodeDataStag, 0, D3D11_MAP_READ, 0, &mappedResource);
        DebugNodeData* data = static_cast<DebugNodeData*>(mappedResource.pData);

        d3dDeviceContext->CopyResource(mFragmentStatsStaging, mFragmentStats);
        d3dDeviceContext->Map(mFragmentStatsStaging, 0, D3D11_MAP_READ, 0, &mappedResource);
        unsigned int* stats = static_cast<unsigned int*>(mappedResource.pData);

        (void) args;
        (void) data;
        (void) stats;

        d3dDeviceContext->Unmap(mFragmentStatsStaging, 0);
        d3dDeviceContext->Unmap(mDebugDrawIndirectArgsStag, 0);
        d3dDeviceContext->Unmap(mDebugDrawNodeDataStag, 0);
    }
}

void App::SetupStandardGeometryFrontEnd(ID3D11DeviceContext* d3dDeviceContext,
                                        D3D11_VIEWPORT* viewport,
                                        const Options &options,
                                        int tile)
{
    d3dDeviceContext->IASetInputLayout(mMeshVertexLayout);
    VSSetConstantBuffers(d3dDeviceContext, mGeometryVSReflector, "PerFrameConstants", 1, &mPerFrameConstants);
    d3dDeviceContext->VSSetShader(mGeometryVS, 0, 0);
    d3dDeviceContext->GSSetShader(NULL, 0, 0);

    // Uses double-sided polygons (no culling) if requested
    d3dDeviceContext->RSSetState(options.enableDoubleSidedPolygons ? mDoubleSidedRasterizerState : mRasterizerState);
    d3dDeviceContext->RSSetViewports(1, viewport);

    D3D10_RECT rects[1];
    LONG dx = (LONG)viewport->Width / 2;
    LONG dy = (LONG)viewport->Height / 2;

    switch (tile) {        
        case 0:
            rects[0].left   = 0;
            rects[0].right  = dx;
            rects[0].top    = 0;
            rects[0].bottom = dy;
            break;
        case 1:
            rects[0].left   = dx;
            rects[0].right  = 2 * dx;
            rects[0].top    = 0;
            rects[0].bottom = dy;
            break;
        case 2:
            rects[0].left   = dx;
            rects[0].right  = 2 * dx;
            rects[0].top    = dy;
            rects[0].bottom = 2 * dy;
            break;
        case 3:
            rects[0].left   = 0;
            rects[0].right  = dx;
            rects[0].top    = dy;
            rects[0].bottom = 2 * dy;
            break;
        default:
            rects[0].left   = 0;
            rects[0].right  = 2 * dx;
            rects[0].top    = 0;
            rects[0].bottom = 2 * dy;
    }

    d3dDeviceContext->RSSetScissorRects( 1, rects );

}

UINT App::SetupStandardGeometryBackEnd(ID3D11DeviceContext* d3dDeviceContext,
                                       ID3D11PixelShader *geomPS,
                                       ID3D11ShaderReflection *geomPSReflector,
                                       bool alphaOnly/*=false*/,
                                       bool alphaFromTexture/*=false*/)
{
    UINT diffuseTextureIndex = INVALID_SAMPLER_SLOT;
    if (geomPSReflector) {
        if (!alphaOnly) {
            ID3D11ShaderResourceView* asvmTextureSRV = mAVSMTextures->GetShaderResource();
            PSSetSamplers(d3dDeviceContext, geomPSReflector, "gAVSMSampler", 1, &mAVSMSampler);
            PSSetShaderResources(d3dDeviceContext, geomPSReflector, "gAVSMTexture", 1, &asvmTextureSRV);
            PSSetConstantBuffers(d3dDeviceContext, geomPSReflector, "AVSMConstants", 1,&mAVSMConstants);
        }
        if (!alphaOnly || alphaFromTexture) {
            //PSSetSamplers(d3dDeviceContext, geomPSReflector, "gIBLSampler", 1, &mIBLSampler);
            //PSSetShaderResources(d3dDeviceContext, geomPSReflector, "gIBLTexture", 1, &mIBLSRV);
            PSSetSamplers(d3dDeviceContext, geomPSReflector, "gDiffuseSampler", 1, &mDiffuseSampler);
            GetBindIndex(geomPSReflector, "gDiffuseTexture", &diffuseTextureIndex);
        }
        PSSetConstantBuffers(d3dDeviceContext, geomPSReflector, "PerFrameConstants", 1, &mPerFrameConstants);    
    }

    d3dDeviceContext->PSSetShader(geomPS, 0, 0);
    return diffuseTextureIndex;
}

void App::SetupParticleFrontEnd(ID3D11DeviceContext* d3dDeviceContext,
                                D3D11_VIEWPORT* viewport)
{
    VSSetConstantBuffers(d3dDeviceContext, mParticleShadingVSReflector, "ParticlePerFrameConstants", 1, &mParticlePerFrameConstants);
    VSSetConstantBuffers(d3dDeviceContext, mParticleShadingVSReflector, "ParticlePerPassConstants", 1, &mParticlePerPassConstants);
    d3dDeviceContext->VSSetShader(mParticleShadingVS, 0, 0);
    d3dDeviceContext->GSSetShader(NULL, 0, 0);

    d3dDeviceContext->RSSetState(mParticleRasterizerState);
    d3dDeviceContext->RSSetViewports(1, viewport);
}

UINT App::SetupParticleBackEnd(ID3D11DeviceContext* d3dDeviceContext,
                               ID3D11PixelShader *ps,
                               ID3D11ShaderReflection *psReflector,
                               bool alphaOnly/*=false*/)
{
    if (!alphaOnly) {
        ID3D11ShaderResourceView* asvmTextureSRV = mAVSMTextures->GetShaderResource();
        PSSetConstantBuffers(d3dDeviceContext, psReflector, "PerFrameConstants", 1, &mPerFrameConstants);
        PSSetShaderResources(d3dDeviceContext, psReflector, "gIBLTexture", 1, &mIBLSRV);
        PSSetSamplers(d3dDeviceContext, psReflector, "gIBLSampler", 1, &mIBLSampler);
        PSSetShaderResources(d3dDeviceContext, psReflector, "gAVSMTexture", 1, &asvmTextureSRV);
        PSSetSamplers(d3dDeviceContext, psReflector, "gAVSMSampler", 1, &mAVSMSampler);
        PSSetConstantBuffers(d3dDeviceContext, psReflector, "AVSMConstants", 1, &mAVSMConstants); 
    }
    PSSetConstantBuffers(d3dDeviceContext, psReflector, "ParticlePerFrameConstants", 1, &mParticlePerFrameConstants);

    d3dDeviceContext->PSSetShader(ps, 0, 0);

    return INVALID_SAMPLER_SLOT;
}

float saturate(float value)
{
    if (value < 0.0f)
        return 0;
    else if (value > 1.0f)
        return 1;
    else
        return value;
}

float linstep(float min, float max, float v)
{
    return saturate((v - min) / (max - min));
}

float LT_Interp(float d0, float d1, float t0, float t1, float r)
{
    float depth = linstep(d0, d1, r);
    return t0 + (t1 - t0) * depth;
}

std::string App::GetSceneName(SCENE_SELECTION scene)
{
    std::string sceneName("unknown");
    switch (scene) {
    case POWER_PLANT_SCENE: sceneName = "powerplant"; break;
    case GROUND_PLANE_SCENE: sceneName = "groundplane"; break;
    case TEAPOT_SCENE: sceneName = "teapot"; break;  
    case TEAPOTS_SCENE: sceneName = "teapots"; break;  
    case DRAGON_SCENE: sceneName = "dragon"; break;  
    case HAPPYS_SCENE: sceneName = "happys"; break;  
    }
    return sceneName;
}

SCENE_SELECTION App::GetSceneEnum(const std::string &name)
{
    for (int i = 0; i < NUM_SCENES; ++i) {
        if (name == GetSceneName((SCENE_SELECTION) i)) {
            return (SCENE_SELECTION) i;
        }
    }
    assert(0);
    return POWER_PLANT_SCENE;
}

std::string App::GetTransparencyMethodName(TRANSPARENCY_METHOD method)
{
    std::string     transparencyMethod("unknown");
    switch (method) {
    case TM_ALPHA_BLENDING: transparencyMethod = "TM_ALPHA_BLENDING"; break;
    case TM_ABUFFER_SORTED: transparencyMethod = "TM_ABUFFER_SORTED"; break;
    case TM_ABUFFER_ITERATIVE: transparencyMethod = "TM_ABUFFER_ITERATIVE"; break;
    case TM_AOIT: transparencyMethod = "TM_AOIT"; break;
    }
    return transparencyMethod;
}

TRANSPARENCY_METHOD App::GetTransparencyMethodEnum(const std::string &name)
{

    if (name == "TM_ALPHA_BLENDING") {
        return TM_ALPHA_BLENDING;
    } else if (name == "TM_ABUFFER_SORTED") {
        return TM_ABUFFER_SORTED;
    } else if (name == "TM_ABUFFER_ITERATIVE") {
        return TM_ABUFFER_ITERATIVE;
    } else if (name == "TM_AOIT") {
        return TM_AOIT;
    } else {
        assert(0);
        return TM_ALPHA_BLENDING;
    }
}

std::string App::GetShadowMethod(const UIConstants &ui)
{
    char buffer[1024];
    std::string shadowMethod("unknown");
    switch (ui.volumeShadowMethod) {
        case VOL_SHADOW_AVSM: {            
            sprintf_s(buffer, sizeof(buffer), "avsm%d", mAVSMShadowNodeCount);
            shadowMethod = buffer;
            break;
        }
    }
    return shadowMethod;
}

float GetGPUCounterSeconds(ID3D11DeviceContext* d3dDeviceContext, ID3D11Query** query)
{
    // Get GPU counters
    UINT64 queryTimeA, queryTimeB;
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT queryTimeData;

    while( S_OK != d3dDeviceContext->GetData(query[0],  &queryTimeA,    sizeof(UINT64), 0) ) {}
    while( S_OK != d3dDeviceContext->GetData(query[1],  &queryTimeB,    sizeof(UINT64), 0) ) {}
    while( S_OK != d3dDeviceContext->GetData(query[2],  &queryTimeData, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0) ) {}

    if (0 == queryTimeData.Disjoint) {
        UINT64 deltaTimeTicks = queryTimeB - queryTimeA;
        return (float)deltaTimeTicks / (float)queryTimeData.Frequency;  
    } else {
        return 0.0f;
    }
}

void App::GatherFragmentStats(Options &options,
                              UIConstants* ui,
                              ID3D11DeviceContext* d3dDeviceContext, 
                              ID3D11RenderTargetView* backBuffer,
                              D3D11_VIEWPORT* viewport,
                              FrameStats* frameStats,
                              bool variableMemOIT)
{
    if (variableMemOIT) {
        // Init stats
        memset(frameStats, 0x0, sizeof(*frameStats));

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        d3dDeviceContext->Map(mFragmentStatsStaging, 0, D3D11_MAP_READ, 0, &mappedResource);
        FragmentStats* stats = reinterpret_cast<FragmentStats*>(mappedResource.pData);

        float deltaTime;
        float avgFragPerPixel = (float)stats->fragmentCount / (viewport->Width * viewport->Height);
        float avgFragPerNonEmptyPixel = (float)stats->fragmentCount / (float)stats->pixelCount; 
        float stdDevPerPixel = sqrtf(((float)stats->fragmentSquareCount / (viewport->Width * viewport->Height)) - (avgFragPerPixel * avgFragPerPixel));
        float stdDevPerNonEmptyPixel = sqrtf(((float)stats->fragmentSquareCount / (float)stats->pixelCount) - (avgFragPerNonEmptyPixel * avgFragPerNonEmptyPixel));

        frameStats->capturePassTime       = 1000.0f * GetGPUCounterSeconds(d3dDeviceContext, &mGPUQuery[GPUQ_CAPTURE_PASS][0]);
        deltaTime = GetGPUCounterSeconds(d3dDeviceContext, &mGPUQuery[GPUQ_RESOLVE_PASS][0]);
        if (deltaTime > 0.0f) {
            frameStats->fragmentPerSecond = stats->fragmentCount / deltaTime;
            frameStats->resolvePassTime = deltaTime * 1000.0f;
        }
 
        // Fill stats to display on screen
        frameStats->minDepth = *(float*)(&stats->minDepth);
        frameStats->maxDepth = *(float*)(&stats->maxDepth);
        frameStats->avgFragmentPerPixel = avgFragPerPixel;
        frameStats->stdDevFragmentPerPixel = stdDevPerPixel;
        frameStats->avgFragmentPerNonEmptyPixel = avgFragPerNonEmptyPixel;
        frameStats->stdDevFragmentNonEmptyPerPixel = stdDevPerNonEmptyPixel;
        frameStats->fragmentCount = (float)stats->fragmentCount;
        frameStats->maxFragmentPerPixelCount = stats->maxFragmentPerPixelCount;
        frameStats->transparentFragmentScreenCoverage = (float)stats->pixelCount / (viewport->Width * viewport->Height);

        d3dDeviceContext->Unmap(mFragmentStatsStaging, 0);

        // Clear stats
        d3dDeviceContext->CSSetUnorderedAccessViews(0, 1, &mFragmentStatsUAV, NULL);
        d3dDeviceContext->CSSetShader(mClearFragmentStatsCS, NULL, 0);
        d3dDeviceContext->Dispatch(1, 1, 1);
        Cleanup(d3dDeviceContext);
    
        // Compute Stats
        ID3D11ShaderResourceView* fragmentListFirstNodeOffsetSRV = mFragmentListFirstNodeOffset->GetShaderResource();
        d3dDeviceContext->CSSetShaderResources(0, 1, &fragmentListFirstNodeOffsetSRV);
        d3dDeviceContext->CSSetShaderResources(1, 1, &mFragmentListNodesSRV);
        d3dDeviceContext->CSSetUnorderedAccessViews(0, 1, &mFragmentStatsUAV, NULL);
        d3dDeviceContext->CSSetShader(mComputeFragmentStatsCS, NULL, 0);

        // Compute thread group count
        const unsigned int dimX = (unsigned int)viewport->Width;
        const unsigned int dimY = (unsigned int)viewport->Height;
        unsigned int threadGroupCountX = dimX / STATS_TILEDIM_X;
        unsigned int threadGroupCountY = dimY / STATS_TILEDIM_Y;
        threadGroupCountX += threadGroupCountX * STATS_TILEDIM_X == dimX ? 0 : 1;    
        threadGroupCountY += threadGroupCountY * STATS_TILEDIM_Y == dimY ? 0 : 1;

        d3dDeviceContext->Dispatch(threadGroupCountX , threadGroupCountY, 1);
        Cleanup(d3dDeviceContext);

        // Copy stats for the next frame
        d3dDeviceContext->CopyResource(mFragmentStatsStaging, mFragmentStats);
    }
}

void App::FillHairConstants(ID3D11DeviceContext* d3dDeviceContext,
                            D3DXMATRIX const& hairProj,   
                            D3DXMATRIX const& hairWorldView,
                            D3DXMATRIX const& hairWorldViewProj)
{
    // Update Constant Buffers
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        d3dDeviceContext->Map(mHairConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, 
                              &mappedResource);
        HairConstants* constants = 
            static_cast<HairConstants*>(mappedResource.pData);
        constants->mHairProj = hairProj;
        constants->mHairWorldView = hairWorldView;
        constants->mHairWorldViewProj = hairWorldViewProj;
        d3dDeviceContext->Unmap(mHairConstants, 0);
    }
}

void App::RenderHair(ID3D11DeviceContext* d3dDeviceContext,
                     ID3D11RenderTargetView* backBuffer,
                     D3D11_VIEWPORT* viewport,
                     const D3DXMATRIXA16 &cameraWorldView,
                     const UIConstants* ui,
                     const Options &options)
{
    SetupHairFrontEnd(d3dDeviceContext, viewport, mRasterizerState);

    bool requirePerPixelList = (options.transparencyMethod == TM_ABUFFER_SORTED) ||
                               (options.transparencyMethod == TM_ABUFFER_ITERATIVE) ||
                               (options.transparencyMethod == TM_AOIT);

    ID3D11PixelShader* ps = 0;
    ID3D11ShaderReflection* psReflector = 0;
    if (requirePerPixelList) {
        ps          = mCameraHairCapturePS[0];
        psReflector = mCameraHairCapturePSReflector[0];
    } else {
        ps          = mStandardCameraHairRenderPS[mShadowShaderIdx];
        psReflector = mStandardCameraHairRenderPSReflector[mShadowShaderIdx];
    }

    SetupHairBackEnd(d3dDeviceContext, ps, psReflector);

    if (requirePerPixelList) {
        // Update Constant Buffers
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        d3dDeviceContext->Map(mHairLTConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, 
                                &mappedResource);
        HairLTConstants* constants = 
            static_cast<HairLTConstants*>(mappedResource.pData);
        constants->mMaxNodeCount = mHairLTMaxNodeCount;
        d3dDeviceContext->Unmap(mHairLTConstants, 0);

        PSSetConstantBuffers(d3dDeviceContext,
                                psReflector,
                                "FL_Constants",
                                1, 
                                &mFragmentListConstants);

        ID3D11UnorderedAccessView* fragmentListFirstNodeOffsetUAV = mFragmentListFirstNodeOffset->GetUnorderedAccess();

        // Set List Texture UAVs (we don't need any RT!)
        static const char *paramUAVs[] = {
            "gFragmentListFirstNodeAddressUAV",
            "gFragmentListNodesUAV",
        };
        const UINT numUAVs = 2;
        const UINT firstUAVIndex =
            GetStartBindIndex(psReflector, paramUAVs, numUAVs);           
        UINT pUAVInitialCounts[2] = {1, 1};
        ID3D11UnorderedAccessView* pUAVs[2] = {
            fragmentListFirstNodeOffsetUAV, 
            mFragmentListNodesUAV
        };

        d3dDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(
            0, NULL, // render targets
            mDepthBuffer->GetDepthStencil(),    // depth-stencil
            firstUAVIndex, numUAVs, 
            pUAVs, 
            options.enableTransparency ? NULL : pUAVInitialCounts);

        d3dDeviceContext->OMSetDepthStencilState(mHairCaptureDepthStencilState, 0x0);
    } else {
        mHair->ResetSort(d3dDeviceContext);

        d3dDeviceContext->OMSetRenderTargets(1, &backBuffer, mDepthBuffer->GetDepthStencil());
        d3dDeviceContext->OMSetDepthStencilState(mHairRenderDepthStencilState, 0x0);
        d3dDeviceContext->OMSetBlendState(mHairRenderBlendState, 0, 0xffffffff);
    }

    mHair->Draw(d3dDeviceContext);

    Cleanup(d3dDeviceContext);
}

void App::SetupHairFrontEnd(ID3D11DeviceContext* d3dDeviceContext,
                            D3D11_VIEWPORT* viewport,
                            ID3D11RasterizerState* rs)
{
    VSSetConstantBuffers(d3dDeviceContext, mHairVSReflector, "HairConstants", 1, &mHairConstants);
    d3dDeviceContext->VSSetShader(mHairVS, 0, 0);

    GSSetConstantBuffers(d3dDeviceContext, mHairGSReflector, "PerFrameConstants", 1,&mPerFrameConstants);
    GSSetConstantBuffers(d3dDeviceContext, mHairGSReflector, "HairConstants", 1, &mHairConstants);
    d3dDeviceContext->GSSetShader(mHairGS, 0, 0);

    d3dDeviceContext->RSSetState(rs);
    d3dDeviceContext->RSSetViewports(1, viewport);     
}

void App::SetupHairBackEnd(ID3D11DeviceContext* d3dDeviceContext,
                           ID3D11PixelShader* ps,
                           ID3D11ShaderReflection* psReflector,
                           bool alphaOnly/*=false*/)
{
    if (!alphaOnly) {
        ID3D11ShaderResourceView* asvmTextureSRV =  mAVSMTextures->GetShaderResource();
        PSSetConstantBuffers(d3dDeviceContext, psReflector, "AVSMConstants", 1, &mAVSMConstants);
        PSSetShaderResources(d3dDeviceContext, psReflector, "gAVSMTexture", 1, &asvmTextureSRV);
        PSSetSamplers(d3dDeviceContext, psReflector, "gAVSMSampler", 1, &mAVSMSampler); 
    }
    PSSetConstantBuffers(d3dDeviceContext, psReflector, "PerFrameConstants", 1, &mPerFrameConstants);
    d3dDeviceContext->PSSetShader(ps, 0, 0);
}


void App::RenderStandardGeometry(const Options &options,
                                 ID3D11DeviceContext* d3dDeviceContext,
                                 ID3D11RenderTargetView* backBuffer,
                                 CDXUTSDKMesh* mesh,
                                 D3D11_VIEWPORT* viewport,
                                 bool alphaFromTexture,
                                 bool resetUAVCounter,
                                 int tile)
{
    SetupStandardGeometryFrontEnd(d3dDeviceContext, viewport, options);

    ID3D11PixelShader *geomPS;
    ID3D11ShaderReflection *geomPSReflector;

    bool requirePerPixelList = ((options.transparencyMethod == TM_ABUFFER_SORTED) ||
                                (options.transparencyMethod == TM_ABUFFER_ITERATIVE) ||
                                (options.transparencyMethod == TM_AOIT)) && options.enableTransparency;

    if (requirePerPixelList) {
        const int geoShaderIdx = 0;
        const int alphaFromTextIdx = alphaFromTexture ? 2 : 0;
        geomPS = mGeometryCapturePS[alphaFromTextIdx + geoShaderIdx];
        geomPSReflector = mGeometryCapturePSReflector[alphaFromTextIdx + geoShaderIdx];
    } else {
        const int alphaFromTextIdx = alphaFromTexture ? 1 : 0;
        geomPS = mGeometryPS[alphaFromTextIdx];
        geomPSReflector = mGeometryPSReflector[alphaFromTextIdx];
    } 

    UINT diffuseTextureIndex = SetupStandardGeometryBackEnd(d3dDeviceContext, geomPS, geomPSReflector);

    if (requirePerPixelList) {
        SetupStandardGeometryFrontEnd(d3dDeviceContext, viewport, options, tile);

        PSSetConstantBuffers(d3dDeviceContext, geomPSReflector, "FL_Constants", 1, &mFragmentListConstants);

		ID3D11UnorderedAccessView* fragmentListFirstNodeOffsetUAV = 
		mFragmentListFirstNodeOffset->GetUnorderedAccess();

		// Set List Texture UAVs (we don't need any RT!)
		static const char *paramUAVs[] = {
			"gFragmentListFirstNodeAddressUAV",
			"gFragmentListNodesUAV",            
		};
        UINT numUAVs = 2;
		const UINT firstUAVIndex =
			GetStartBindIndex(geomPSReflector, paramUAVs, numUAVs);           
		UINT pUAVInitialCounts[2] = {1, 1};
		ID3D11UnorderedAccessView* pUAVs[2] = {
			fragmentListFirstNodeOffsetUAV, 
			mFragmentListNodesUAV,
		};

		d3dDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(
			0, 
            NULL, // render targets
            mDepthBuffer->GetDepthStencil(),    // depth-stencil
			firstUAVIndex, numUAVs, 
            pUAVs, 
            resetUAVCounter ? pUAVInitialCounts : NULL);

		d3dDeviceContext->OMSetBlendState(mGeometryOpaqueBlendState, 0, 0xFFFFFFFF);
		d3dDeviceContext->OMSetDepthStencilState(mParticleDepthStencilState, 0x0);        
	} else {
		d3dDeviceContext->OMSetRenderTargets(1, &backBuffer, mDepthBuffer->GetDepthStencil());
        if ((options.transparencyMethod == TM_ALPHA_BLENDING) && options.enableTransparency) {
		    d3dDeviceContext->OMSetBlendState(mGeometryTransparentBlendState, 0, 0xFFFFFFFF);
		    d3dDeviceContext->OMSetDepthStencilState(mParticleDepthStencilState, 0x0);
        } else {
		    d3dDeviceContext->OMSetBlendState(mGeometryOpaqueBlendState, 0, 0xFFFFFFFF);
		    d3dDeviceContext->OMSetDepthStencilState(mDefaultDepthStencilState, 0x0);
        }        
	}
    
    mesh->Render(d3dDeviceContext, diffuseTextureIndex);

    // Cleanup (aka make the runtime happy)
    Cleanup(d3dDeviceContext);
}

void App::RenderSkybox(ID3D11DeviceContext* d3dDeviceContext,
                       ID3D11RenderTargetView* backBuffer,
                       const D3D11_VIEWPORT* viewport)
{
    D3D11_VIEWPORT skyboxViewport(*viewport);
    skyboxViewport.MinDepth = 1.0f;
    skyboxViewport.MaxDepth = 1.0f;

    d3dDeviceContext->IASetInputLayout(mMeshVertexLayout);

    VSSetConstantBuffers(d3dDeviceContext,
                         mSkyboxVSReflector,
                         "PerFrameConstants",
                         1, 
                         &mPerFrameConstants);
    
    d3dDeviceContext->VSSetShader(mSkyboxVS, 0, 0);

    d3dDeviceContext->RSSetState(mDoubleSidedRasterizerState);
    d3dDeviceContext->RSSetViewports(1, &skyboxViewport);

    PSSetSamplers(d3dDeviceContext,
                  mSkyboxPSReflector,
                  "gDiffuseSampler",
                  1, 
                  &mDiffuseSampler);

    d3dDeviceContext->PSSetShader(mSkyboxPS, 0, 0);

    // Set skybox texture
    PSSetShaderResources(d3dDeviceContext,
                         mSkyboxPSReflector,
                         "gSkyboxTexture",
                         1,
                         &mSkyboxSRV);

    d3dDeviceContext->OMSetRenderTargets(1, &backBuffer, mDepthBuffer->GetDepthStencil());
    d3dDeviceContext->OMSetBlendState(mGeometryOpaqueBlendState, 0, 0xFFFFFFFF);
    d3dDeviceContext->OMSetDepthStencilState(mDefaultDepthStencilState, 0x0);
    
    mSkyboxMesh.Render(d3dDeviceContext);

    // Cleanup (aka make the runtime happy)
    Cleanup(d3dDeviceContext);
}


void App::CaptureParticles(ID3D11DeviceContext* d3dDeviceContext, 
                           ParticleSystem *particleSystem,
                           const UIConstants* ui,
                           bool initCounter)
{
    switch(ui->volumeShadowMethod) {
        case VOL_SHADOW_AVSM:
        case VOL_SHADOW_AVSM_BOX_4:
        case VOL_SHADOW_AVSM_GAUSS_7:
        {
            VSSetConstantBuffers(d3dDeviceContext,
                                 mParticleShadingVSReflector,
                                 "ParticlePerFrameConstants",
                                 1, 
                                 &mParticlePerFrameConstants);
            VSSetConstantBuffers(d3dDeviceContext,
                                 mParticleShadingVSReflector,
                                 "ParticlePerPassConstants",
                                 1, 
                                 &mParticlePerPassConstants);
            d3dDeviceContext->VSSetShader(mParticleShadingVS, 0, 0);

            d3dDeviceContext->RSSetState(mParticleRasterizerState);
            d3dDeviceContext->RSSetViewports(1, &mAVSMShadowViewport);

            PSSetConstantBuffers(d3dDeviceContext,
                                 mParticleAVSMCapturePSReflector,
                                 "ParticlePerFrameConstants",
                                 1, 
                                 &mParticlePerFrameConstants);
            PSSetConstantBuffers(d3dDeviceContext,
                                 mParticleAVSMCapturePSReflector,
                                 "LT_Constants",
                                 1, 
                                 &mListTextureConstants);
            d3dDeviceContext->PSSetShader(mParticleAVSMCapturePS, 0, 0);

            ID3D11UnorderedAccessView* listTexFirstSegmentNodeOffsetUAV = 
                mListTexFirstSegmentNodeOffset->GetUnorderedAccess();

            // Set List Texture UAVs (we don't need any RT!)
            static const char *paramUAVs[] = {
                "gListTexFirstSegmentNodeAddressUAV",
                "gListTexSegmentNodesUAV",
            };
            const UINT numUAVs = sizeof(paramUAVs) / sizeof(paramUAVs[0]);
            const UINT firstUAVIndex =
                GetStartBindIndex(mParticleAVSMCapturePSReflector, 
                                  paramUAVs, numUAVs);           
            UINT pUAVInitialCounts[numUAVs] = {0, 0};
            ID3D11UnorderedAccessView* pUAVs[numUAVs] = {
                listTexFirstSegmentNodeOffsetUAV, 
                mListTexSegmentNodesUAV
            };

            d3dDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(
                0, NULL, // render targets
                NULL,    // depth-stencil
                firstUAVIndex, numUAVs, pUAVs, initCounter ? pUAVInitialCounts : NULL);

            d3dDeviceContext->OMSetBlendState(mGeometryOpaqueBlendState, 0, 0xFFFFFFFF);
            d3dDeviceContext->OMSetDepthStencilState(mAVSMCaptureDepthStencilState, 0x0);

            particleSystem->Draw(d3dDeviceContext, NULL, 0, particleSystem->GetParticleCount());

            // Cleanup (aka make the runtime happy)
            Cleanup(d3dDeviceContext);

            break;
        }

    }
}

void App::ClearShadowBuffers(ID3D11DeviceContext* d3dDeviceContext, const UIConstants* ui)
{
    switch(ui->volumeShadowMethod) {
        case VOL_SHADOW_NO_SHADOW:
        case VOL_SHADOW_AVSM:
        case VOL_SHADOW_AVSM_BOX_4:
        case VOL_SHADOW_AVSM_GAUSS_7:
        {
            ID3D11UnorderedAccessView* listTexFirstSegmentNodeOffsetUAV = 
                mListTexFirstSegmentNodeOffset->GetUnorderedAccess();

            // Initialize the first node offset RW UAV with a NULL offset (end of the list)
            UINT clearValues[4] = {
                0xFFFFFFFFUL, 
                0xFFFFFFFFUL, 
                0xFFFFFFFFUL, 
                0xFFFFFFFFUL
            };

            d3dDeviceContext->ClearUnorderedAccessViewUint(
                listTexFirstSegmentNodeOffsetUAV, clearValues);
            break;
        }
    }
}


void App::ClearOITBuffers(ID3D11DeviceContext* d3dDeviceContext, TRANSPARENCY_METHOD transparencyMethod)
{
    switch(transparencyMethod) {
        case TM_ABUFFER_SORTED:
        case TM_ABUFFER_ITERATIVE:
        case TM_AOIT:
        {
            ID3D11UnorderedAccessView* fragmentListFirstNodeOffsetUAV = 
                mFragmentListFirstNodeOffset->GetUnorderedAccess();


            // Initialize the first node offset RW UAV with a NULL offset (end of the list)
            UINT clearValuesFirstNode[4] = {
                0x0UL, 
                0x0UL, 
                0x0UL, 
                0x0UL
            };

            d3dDeviceContext->ClearUnorderedAccessViewUint(
                fragmentListFirstNodeOffsetUAV, clearValuesFirstNode);             
            break;
        }
        default:
            break;
    }
}

void App::CaptureHair(ID3D11DeviceContext* d3dDeviceContext, 
                      const UIConstants* ui,
                      bool initCounter)
{
    SetupHairFrontEnd(d3dDeviceContext, &mAVSMShadowViewport, mHairRasterizerState);

    PSSetConstantBuffers(d3dDeviceContext,
                            mShadowHairCapturePSReflector,
                            "PerFrameConstants",
                            1, 
                            &mPerFrameConstants);
    PSSetConstantBuffers(d3dDeviceContext,
                            mShadowHairCapturePSReflector,
                            "LT_Constants",
                            1, 
                            &mListTextureConstants);

    d3dDeviceContext->PSSetShader(mShadowHairCapturePS, 0, 0);
    
    ID3D11UnorderedAccessView* listTexFirstSegmentNodeOffsetUAV = 
        mListTexFirstSegmentNodeOffset->GetUnorderedAccess();

    // Set List Texture UAVs (we don't need any RT!)
    static const char *paramUAVs[] = {
        "gListTexFirstSegmentNodeAddressUAV",
        "gListTexSegmentNodesUAV",
    };
    const UINT numUAVs = sizeof(paramUAVs) / sizeof(paramUAVs[0]);
    const UINT firstUAVIndex =
        GetStartBindIndex(mShadowHairCapturePSReflector, 
                            paramUAVs, numUAVs);
    UINT pUAVInitialCounts[numUAVs] = {0, 0};
    ID3D11UnorderedAccessView* pUAVs[numUAVs] = {
        listTexFirstSegmentNodeOffsetUAV, 
        mListTexSegmentNodesUAV
    };

    d3dDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(
        0, NULL, // render targets
        NULL,    // depth-stencil
        firstUAVIndex, numUAVs, pUAVs, initCounter ? pUAVInitialCounts : NULL);

    d3dDeviceContext->OMSetBlendState(mGeometryOpaqueBlendState, 0, 0xFFFFFFFF);
    d3dDeviceContext->OMSetDepthStencilState(mAVSMCaptureDepthStencilState, 0x0);


    mHair->Draw(d3dDeviceContext);

    // Cleanup (aka make the runtime happy)
    Cleanup(d3dDeviceContext);
}

void App::GenerateVisibilityCurve(ID3D11DeviceContext* d3dDeviceContext,
                                 const UIConstants* ui)
{
    ID3D11ShaderResourceView*  listTexFirstSegmentNodeOffsetSRV = mListTexFirstSegmentNodeOffset->GetShaderResource();

    if (VOL_SHADOW_AVSM == ui->volumeShadowMethod ||
        VOL_SHADOW_AVSM_BOX_4 == ui->volumeShadowMethod ||
        VOL_SHADOW_AVSM_GAUSS_7 == ui->volumeShadowMethod) {

        // Second (full screen) pass, sort fragments and insert them in our AVSM texture(s)
        d3dDeviceContext->IASetInputLayout(0);
        d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        d3dDeviceContext->IASetVertexBuffers(0, 0, 0, 0, 0);

        d3dDeviceContext->VSSetShader(mFullScreenTriangleVS, 0, 0);

        d3dDeviceContext->RSSetState(mRasterizerState);
        d3dDeviceContext->RSSetViewports(1, &mAVSMShadowViewport);     
        
        ID3D11ShaderReflection *shaderReflector = NULL;
        switch (ui->avsmSortingMethod) {
            case 0:
                shaderReflector = mAVSMUnsortedResolvePSReflector[mShadowShaderIdx];
                d3dDeviceContext->PSSetShader(mAVSMUnsortedResolvePS[mShadowShaderIdx], 0, 0); 
                break;
            case 1:
                shaderReflector = mAVSMInsertionSortResolvePSReflector[mShadowShaderIdx];
                d3dDeviceContext->PSSetShader(mAVSMInsertionSortResolvePS[mShadowShaderIdx], 0, 0); 
                break;
            default: 
                break;
        }

        PSSetConstantBuffers(d3dDeviceContext,
                             shaderReflector,
                             "AVSMConstants",
                             1, 
                             &mAVSMConstants);

        PSSetShaderResources(d3dDeviceContext,
                             shaderReflector,
                             "gListTexFirstSegmentNodeAddressSRV",
                             1, 
                             &listTexFirstSegmentNodeOffsetSRV);
        PSSetShaderResources(d3dDeviceContext,
                             shaderReflector,
                             "gListTexSegmentNodesSRV",
                             1, 
                             &mListTexSegmentNodesSRV);
        
        ID3D11RenderTargetView* pRTs[16];
        const int avsmRTCount = mAVSMShadowNodeCount / 2;
        for (int i = 0; i < avsmRTCount; ++i) {
            pRTs[i] = mAVSMTextures->GetRenderTarget(i);
        }
        d3dDeviceContext->OMSetRenderTargets(avsmRTCount, pRTs, 0);
                                                                    
        d3dDeviceContext->OMSetBlendState(mGeometryOpaqueBlendState, 0, 0xFFFFFFFF);
        d3dDeviceContext->OMSetDepthStencilState(mDefaultDepthStencilState, 0x0);

        // Full-screen triangle
        d3dDeviceContext->Draw(3, 0);

        // Cleanup (aka make the runtime happy)
        Cleanup(d3dDeviceContext);
    } 
}


void App::ShadeParticles(ID3D11DeviceContext* d3dDeviceContext,
                         ParticleSystem *particleSystem,
                         ID3D11RenderTargetView* backBuffer,
                         D3D11_VIEWPORT* viewport,
                         const Options &options,
                         const UIConstants* ui)
{
    SetupParticleFrontEnd(d3dDeviceContext, viewport);

    bool requirePerPixelList = (options.transparencyMethod == TM_ABUFFER_SORTED) ||
                               (options.transparencyMethod == TM_ABUFFER_ITERATIVE) ||
                               (options.transparencyMethod == TM_AOIT);

    ID3D11PixelShader* ps = 0;
    ID3D11ShaderReflection* psReflector = 0;
    if (requirePerPixelList) {
        ps          = mParticleShadingCapturePS[0];
        psReflector = mParticleShadingCapturePSReflector[0];
    } else {
        ps          = mParticleShadingPS[mShadowShaderIdx];
        psReflector = mParticleShadingPSReflector[mShadowShaderIdx];
    }

    SetupParticleBackEnd(d3dDeviceContext, ps, psReflector);

    if (requirePerPixelList) {
        PSSetConstantBuffers(d3dDeviceContext, psReflector,	"FL_Constants", 1, &mFragmentListConstants);       
        ID3D11UnorderedAccessView* fragmentListFirstNodeOffsetUAV = mFragmentListFirstNodeOffset->GetUnorderedAccess();

        // Set List Texture UAVs (we don't need any RT!)
        static const char *paramUAVs[] = {
            "gFragmentListFirstNodeAddressUAV",
            "gFragmentListNodesUAV",
        };
        const UINT numUAVs = 2;
        const UINT firstUAVIndex =
            GetStartBindIndex(psReflector, paramUAVs, numUAVs);           
        UINT pUAVInitialCounts[2] = {1, 1};
        ID3D11UnorderedAccessView* pUAVs[2] = {
            fragmentListFirstNodeOffsetUAV, 
            mFragmentListNodesUAV
        };

        d3dDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(
            0, NULL, // render targets
            mDepthBuffer->GetDepthStencil(),    // depth-stencil
            firstUAVIndex, numUAVs, 
            pUAVs,
            options.enableTransparency | options.enableHair ? NULL : pUAVInitialCounts);

        d3dDeviceContext->OMSetBlendState(mGeometryOpaqueBlendState, 0, 0xFFFFFFFF);
        d3dDeviceContext->OMSetDepthStencilState(mParticleDepthStencilState, 0x0);
    } else {
        // Additively blend into back buffer
        d3dDeviceContext->OMSetRenderTargets(1, &backBuffer, mDepthBuffer->GetDepthStencil());
        d3dDeviceContext->OMSetBlendState(mParticleBlendState, 0, 0xFFFFFFFF);
        d3dDeviceContext->OMSetDepthStencilState(mParticleDepthStencilState, 0x0);
    }

    particleSystem->Draw(d3dDeviceContext, NULL, 0, particleSystem->GetParticleCount());

    // Cleanup (aka make the runtime happy)
    Cleanup(d3dDeviceContext);
}

////////////////////////////////
// Constant buffers functions
////////////////////////////////

void App::FillInFrameConstants(ID3D11DeviceContext* d3dDeviceContext,
                               const FrameMatrices &m,
                               D3DXVECTOR4 cameraPos, 
                               CFirstPersonAndOrbitCamera* lightCamera,
                               const UIConstants* ui,
                               const Options &options)
{
    // Compute light direction in view space
    D3DXVECTOR3 lightPosWorld    = *lightCamera->GetEyePt();
    D3DXVECTOR3 lightTargetWorld = *lightCamera->GetLookAtPt();
    D3DXVECTOR3 lightPosView;
    D3DXVec3TransformCoord(&lightPosView, &lightPosWorld, &m.cameraView);
    D3DXVECTOR3 lightTargetView;
    D3DXVec3TransformCoord(&lightTargetView, &lightTargetWorld, &m.cameraView);
    D3DXVECTOR3 lightDirView = lightTargetView - lightPosView;
    D3DXVec3Normalize(&lightDirView, &lightDirView);

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    d3dDeviceContext->Map(mPerFrameConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    PerFrameConstants* constants = static_cast<PerFrameConstants *>(mappedResource.pData);

    // No world matrix for now...
    constants->mCameraWorldViewProj = m.cameraWorldViewProj;
    constants->mCameraWorldView = m.worldMatrix * m.cameraView;
    constants->mCameraViewProj = m.cameraViewProj;
    constants->mCameraProj = m.cameraProj;
    constants->mCameraPos = cameraPos;
    constants->mLightWorldViewProj = m.lightWorldViewProj;
    constants->mAvsmLightWorldViewProj = m.avmsLightWorldViewProj;
    constants->mCameraViewToWorld = m.cameraViewInv;
    constants->mCameraViewToLightProj = m.cameraViewToLightProj;
    constants->mCameraViewToLightView = m.cameraViewToLightView;
    constants->mCameraViewToAvsmLightProj = m.cameraViewToAvsmLightProj;
    constants->mCameraViewToAvsmLightView = m.cameraViewToAvsmLightView;
    constants->mLightDir = D3DXVECTOR4(lightDirView, 0.0f);

    const float msaaCovNrm = 1.0f / (float)mMSAASampleCount;
    constants->mMSAACoverageNorm = D3DXVECTOR4(msaaCovNrm, msaaCovNrm, msaaCovNrm, msaaCovNrm);
    constants->mGeometryAlpha = D3DXVECTOR4(options.geometryAlpha, options.geometryAlpha, options.geometryAlpha, options.geometryAlpha);
    const float alphaCutoff = options.enableAlphaCutoff ? 0.5f : 0.0f;
    constants->mAlphaThreshold = D3DXVECTOR4(alphaCutoff, alphaCutoff, alphaCutoff, alphaCutoff);

    constants->mUI = *ui;
    
    d3dDeviceContext->Unmap(mPerFrameConstants, 0);
}

void App::FillParticleRendererConstants(ID3D11DeviceContext* d3dDeviceContext,
                                        CFirstPersonAndOrbitCamera* camera,
                                        const D3DXMATRIXA16 &cameraView,
                                        const D3DXMATRIXA16 &cameraViewProj)
{
    // Particle renderer constants
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    d3dDeviceContext->Map(mParticlePerPassConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    ParticlePerPassConstants* constants = static_cast<ParticlePerPassConstants *>(mappedResource.pData);

    D3DXMATRIXA16 lightProj = *camera->GetProjMatrix();
    D3DXMATRIXA16 lightView = *camera->GetViewMatrix();

    constants->mParticleWorldViewProj = cameraViewProj;
    constants->mParticleWorldView     = cameraView;
    constants->mEyeRight              = *camera->GetWorldRight();
    constants->mEyeUp                 = *camera->GetWorldUp();
    
    d3dDeviceContext->Unmap(mParticlePerPassConstants, 0);
}

void App::FillListTextureConstants(ID3D11DeviceContext* d3dDeviceContext)
{
    // List texture related constants  
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    d3dDeviceContext->Map(mListTextureConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    LT_Constants* constants = static_cast<LT_Constants*>(mappedResource.pData);

    constants->mMaxNodes = mLisTexNodeCount;
    constants->mFirstNodeMapSize = (float)mAVSMShadowTextureDim;
    
    d3dDeviceContext->Unmap(mListTextureConstants, 0);
}

void App::FillFragmentListConstants(ID3D11DeviceContext* d3dDeviceContext, unsigned int listNodeCount)
{
    // List texture related constants  
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    d3dDeviceContext->Map(mFragmentListConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    FL_Constants* constants = static_cast<FL_Constants*>(mappedResource.pData);

    constants->mMaxListNodes = listNodeCount;
    
    d3dDeviceContext->Unmap(mFragmentListConstants, 0);
}

void App::FillAVSMConstants(ID3D11DeviceContext* d3dDeviceContext)
{
    // AVSM related constants  
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    d3dDeviceContext->Map(mAVSMConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    AVSMConstants* constants = static_cast<AVSMConstants*>(mappedResource.pData);

    constants->mMask0 = D3DXVECTOR4( 0.0f,  1.0f,  2.0f,  3.0f);
    constants->mMask1 = D3DXVECTOR4( 4.0f,  5.0f,  6.0f,  7.0f);
    constants->mMask2 = D3DXVECTOR4( 8.0f,  9.0f, 10.0f, 11.0f);
    constants->mMask3 = D3DXVECTOR4(12.0f, 13.0f, 14.0f, 15.0f);
    constants->mMask4 = D3DXVECTOR4(16.0f, 17.0f, 18.0f, 19.0f);
    constants->mEmptyNode = EMPTY_NODE;
    constants->mOpaqueNodeTrans = 1E-4f;
    constants->mShadowMapSize = (float)mAVSMShadowTextureDim;
    
    d3dDeviceContext->Unmap(mAVSMConstants, 0);
}

