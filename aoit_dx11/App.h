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

#ifndef H_APP
#define H_APP

//--------------------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------------------

#include "DXUT.h"
#include "DXUTcamera.h"
#include "SDKMesh.h"
#include "AVSM_def.h"
#include "AppShaderConstants.h"
#include "Hair.h"
#include "Texture2D.h"
#include "ParticleSystem.h"
#include "Stats.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <list>

//--------------------------------------------------------------------------------------
// Defines
//--------------------------------------------------------------------------------------

#define MAX_AVSM_RT_COUNT   (8)

#define MAX_SHADER_VARIATIONS (MAX_AVSM_RT_COUNT / 2)


//--------------------------------------------------------------------------------------
// Enums
//--------------------------------------------------------------------------------------
enum SCENE_SELECTION {
    POWER_PLANT_SCENE,
    GROUND_PLANE_SCENE,
    HAIR_AND_PARTICLES_SCENE,
    TEAPOT_SCENE,
    TEAPOTS_SCENE,
    DRAGON_SCENE,
    HAPPYS_SCENE,
    //
    NUM_SCENES
};

enum TRANSPARENCY_METHOD {
    TM_ALPHA_BLENDING       = 0,
    TM_ABUFFER_SORTED       = 1,
    TM_ABUFFER_ITERATIVE    = 2,
    TM_AOIT                 = 3,
    TM_UNKNOWN,
};

enum GEOMETRY_TYPE {
    GEOMETRYTYPE_MESH,
    GEOMETRYTYPE_MESHTEXTURE,
    GEOMETRYTYPE_PARTICLES,
    GEOMETRYTYPE_HAIR,
    NUM_GEOMETRY_TYPES,
};

//--------------------------------------------------------------------------------------
// Classes
//--------------------------------------------------------------------------------------

template<typename ShaderType>
class ShaderList {
private:
    std::vector<std::pair<ShaderType*,ID3D11ShaderReflection*> > mVec;
public:
    ~ShaderList() {
        for (; !mVec.empty(); mVec.pop_back()) {
            SAFE_RELEASE(mVec.back().first);
            SAFE_RELEASE(mVec.back().second);
        }
    }
    void push_back(ShaderType* s, ID3D11ShaderReflection* r) { mVec.push_back(std::make_pair(s, r)); }
};

class App
{
public:
    struct Options {
        Options() : 
            scene(POWER_PLANT_SCENE),
            transparencyMethod(TM_ALPHA_BLENDING),
            enableHair(false),
            enableParticles(false),
            enableDoubleSidedPolygons(true),
            enableTransparency(false),
            enableStats(false),
            enableAlphaCutoff(false),
            enableAutoBoundsAVSM(true),
            ShadowNodeCount(8),
            AOITNodeCount(16),
            enableVolumeShadowCreation(true),
            pickedX(0),
            pickedY(0),
            debugX(50),
            debugY(50),
            debugZ(50),
            visualizeABuffer(false),
            visualizeAOIT(false),
            msaaSampleCount(1),
            geometryAlpha(0.2f),
            enableTiling(false)
        {
        }

        template <class T>
        void SerializeValue(std::ofstream &stream,
                            const std::string &name,
                            const T &value)
        {
            stream << name << " " << value << std::endl;
        }

        void Serialize(std::ofstream &stream) {
            SerializeValue(stream, "scene", App::GetSceneName(scene));
            SerializeValue(stream, "transparencyMethod", App::GetTransparencyMethodName(transparencyMethod));
            SerializeValue(stream, "enableHair", enableHair);
            SerializeValue(stream, "enableParticles", enableParticles);
            SerializeValue(stream, "enableDoubleSidedPolygons", enableDoubleSidedPolygons);
            SerializeValue(stream, "enableAlphaCutoff", enableAlphaCutoff);
            SerializeValue(stream, "enableAutoBoundsAVSM", enableAutoBoundsAVSM);
            SerializeValue(stream, "numAvsmShadowNodes", ShadowNodeCount);
            SerializeValue(stream, "numAOITNodes", AOITNodeCount);
            SerializeValue(stream, "enableVolumeShadowCreation", enableVolumeShadowCreation);
            SerializeValue(stream, "pickedX", pickedX);
            SerializeValue(stream, "pickedY", pickedY);
            SerializeValue(stream, "debugX", debugX);
            SerializeValue(stream, "debugY", debugY);
            SerializeValue(stream, "debugZ", debugZ);
            SerializeValue(stream, "visualizeAOIT", visualizeAOIT);
        }

        void Deserialize(std::ifstream &stream) {
            std::string token;
            while (stream.good()) {
                stream >> token;
                if (token == "{") {
                    continue; // ignore
                } else if (token == "}") {
                    return;   // end of this block
                } else if (token == "scene") {
                    stream >> token;
                    scene = App::GetSceneEnum(token);
                } else if (token == "transparencyMethod") {
                    stream >> token;
                    transparencyMethod = App::GetTransparencyMethodEnum(token);
                } else if (token == "enableHair") {
                    stream >> enableHair;
                } else if (token == "enableParticles") {
                    stream >> enableParticles;
                } else if (token == "enableDoubleSidedPolygons") {
                    stream >> enableDoubleSidedPolygons;
                } else if (token == "enableAlphaCutoff") {
                    stream >> enableAlphaCutoff;
                } else if (token == "enableAutoBoundsAVSM") {
                    stream >> enableAutoBoundsAVSM;
                } else if (token == "numAvsmShadowNodes") {
                    stream >> ShadowNodeCount;
                } else if (token == "numAOITNodes") {
                    stream >> AOITNodeCount;
                } else if (token == "enableVolumeShadowCreation") {
                    stream >> enableVolumeShadowCreation;
                } else if (token == "pickedX") {
                    stream >> pickedX;
                } else if (token == "pickedY") {
                    stream >> pickedY;
                } else if (token == "debugX") {
                    stream >> debugX;
                } else if (token == "debugY") {
                    stream >> debugY;
                } else if (token == "debugZ") {
                    stream >> debugZ;
                } else if (token == "visualizeAOIT") {
                    stream >> visualizeAOIT;
                } else {
                    std::string msg = 
                        "Ignoring unknown serialized value \"" + token + "\"\n";
                    OutputDebugStringA(msg.c_str());
                }
            }
        }

        SCENE_SELECTION scene;
        TRANSPARENCY_METHOD transparencyMethod;
        bool enableHair;
        bool enableParticles;
        bool enableDoubleSidedPolygons;
        bool enableTransparency;
        bool enableStats;
        bool enableAlphaCutoff;
        bool enableAutoBoundsAVSM;
        unsigned int ShadowNodeCount;
        unsigned int AOITNodeCount;
        bool enableVolumeShadowCreation;
        int pickedX;
        int pickedY;
        float debugX;
        float debugY;
        float debugZ;
        bool visualizeABuffer;
        bool visualizeAOIT;
        bool enableTiling;
        unsigned int msaaSampleCount;
        float geometryAlpha;
    };

private:
    struct PointVertex {
        float x, y, z;
    };

    struct FrameMatrices
    {    
        D3DXMATRIXA16 worldMatrix;

        D3DXMATRIXA16 cameraProj;
        D3DXMATRIXA16 cameraView;
        D3DXMATRIXA16 cameraViewInv;

        D3DXMATRIXA16 cameraWorldViewProj;
        D3DXMATRIXA16 cameraWorldView;

        D3DXMATRIXA16 cameraViewProj;
        D3DXMATRIXA16 cameraViewToLightProj;
        D3DXMATRIXA16 cameraViewToLightView;
        D3DXMATRIXA16 cameraViewToAvsmLightProj;
        D3DXMATRIXA16 cameraViewToAvsmLightView;
        D3DXMATRIXA16 cameraViewToWorld;

        D3DXMATRIXA16 lightProj;
        D3DXMATRIXA16 lightView;
        D3DXMATRIXA16 lightViewProj;
        D3DXMATRIXA16 lightWorldViewProj;

        D3DXMATRIXA16 avsmLightProj;
        D3DXMATRIXA16 avsmLightView;
        D3DXMATRIXA16 avsmLightViewProj;
        D3DXMATRIXA16 avmsLightWorldViewProj;
    };

public:

    App(ID3D11Device* d3dDevice,
        ID3D11DeviceContext* d3dDeviceContext,
        unsigned int msaaSampleCount,
        unsigned int shadowNodeCount,       unsigned int aoitNodeCount,
        unsigned int avsmShadowTextureDim);

    ~App();
    
    void OnD3D11ResizedSwapChain(ID3D11Device* d3dDevice,
                                 const DXGI_SURFACE_DESC* backBufferDesc);

    void Render(Options &options,
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
                FrameStats* frameStats);

    bool RenderTransparentGeometry(Options &options,
                                   FrameMatrices &frameMatx,
                                   ID3D11DeviceContext* d3dDeviceContext,
                                   ID3D11RenderTargetView* finalRTV,
                                   CDXUTSDKMesh* mesh,
                                   CDXUTSDKMesh* meshAlpha,
                                   ParticleSystem *particleSystem,
                                   D3D11_VIEWPORT* viewport,
                                   UIConstants* ui);

    void UpdateHairMesh();

    void SetShadowNodeCount(int nodeCount);
    void SetAOITNodeCount(int nodeCount);
    void SetMSAASampleCount(unsigned int msaaSampleCount);

    void GatherFragmentStats(Options &options,
                             UIConstants* ui,
                             ID3D11DeviceContext* d3dDeviceContext, 
                             ID3D11RenderTargetView* backBuffer,
                             D3D11_VIEWPORT* viewport,
                             FrameStats* frameStats,
                             bool variableMemOIT);

    void DumpOrDrawTransmittance(const Options &options,
                                 const UIConstants &ui,
                                 ID3D11DeviceContext* d3dDeviceContext, 
                                 ID3D11RenderTargetView* backBuffer,
                                 D3D11_VIEWPORT* viewport,
                                 float depthBounds[2],
                                 int x, int y);

    void RenderStandardGeometry(const Options &options,
                                ID3D11DeviceContext* d3dDeviceContext,
                                ID3D11RenderTargetView* backBuffer,
                                CDXUTSDKMesh* mesh,
                                D3D11_VIEWPORT* viewport,
                                bool alphaFromTexture = false,
                                bool resetUAVCounter = true,
                                int tile = -1);

    void VisualizeTransmittance(ID3D11DeviceContext* d3dDeviceContext,
                                const FrameMatrices &m,
                                const D3D11_VIEWPORT* viewport,
                                Options &options,
                                const UIConstants* ui,
                                ID3D11RenderTargetView* backBuffer);

    void SetupStandardGeometryFrontEnd(ID3D11DeviceContext* d3dDeviceContext,
                                       D3D11_VIEWPORT* viewport,
                                       const Options &options,
                                       int tile = -1);

    UINT SetupStandardGeometryBackEnd(ID3D11DeviceContext* d3dDeviceContext,
                                      ID3D11PixelShader *geomPS,
                                      ID3D11ShaderReflection *geomPSReflector,
                                      bool alphaOnly=false,
                                      bool alphaFromTexture=false);

    void SetupParticleFrontEnd(ID3D11DeviceContext* d3dDeviceContext,
                               D3D11_VIEWPORT* viewport);

    UINT SetupParticleBackEnd(ID3D11DeviceContext* d3dDeviceContext,
                              ID3D11PixelShader *geomPS,
                              ID3D11ShaderReflection *geomPSReflector,
                              bool alphaOnly=false);

    void SetupHairFrontEnd(ID3D11DeviceContext* d3dDeviceContext,
                           D3D11_VIEWPORT* viewport,
                           ID3D11RasterizerState* rs);

    void SetupHairBackEnd(ID3D11DeviceContext* d3dDeviceContext,
                          ID3D11PixelShader* ps,
                          ID3D11ShaderReflection* psReflector,
                          bool alphaOnly=false);

    void RenderSkybox(ID3D11DeviceContext* d3dDeviceContext,
                      ID3D11RenderTargetView* backBuffer,
                      const D3D11_VIEWPORT* viewport);
 
    void CaptureParticles(ID3D11DeviceContext* d3dDeviceContext,
                          ParticleSystem *particleSystem,
                          const UIConstants* ui,
                          bool initCounter);

    void CaptureHair(ID3D11DeviceContext* d3dDeviceContext, 
                     const UIConstants* ui,
                     bool initCounter);

    void GenerateVisibilityCurve(ID3D11DeviceContext* d3dDeviceContext,
                                 const UIConstants* ui);

    void ShadeParticles(ID3D11DeviceContext* d3dDeviceContext,
                        ParticleSystem *particleSystem,
                        ID3D11RenderTargetView* backBuffer,
                        D3D11_VIEWPORT* viewport,
                        const Options &options,
                        const UIConstants* ui);

    void FillInFrameConstants(ID3D11DeviceContext* d3dDeviceContext,
                              const FrameMatrices &m,
                              D3DXVECTOR4 cameraPos,
                              CFirstPersonAndOrbitCamera* lightCamera,
                              const UIConstants* ui,
                              const Options &options);

    void FillParticleRendererConstants(ID3D11DeviceContext* d3dDeviceContext,
                                       CFirstPersonAndOrbitCamera* lightCamera,
                                       const D3DXMATRIXA16 &cameraView,
                                       const D3DXMATRIXA16 &cameraViewProj);
    void FillListTextureConstants(ID3D11DeviceContext* d3dDeviceContext);
    void FillFragmentListConstants(ID3D11DeviceContext* d3dDeviceContext, unsigned int listNodeCount);
    void FillAVSMConstants(ID3D11DeviceContext* d3dDeviceContext);
    void FillHairConstants(ID3D11DeviceContext* d3dDeviceContext,
                           D3DXMATRIX const& hairProj,   
                           D3DXMATRIX const& hairWorldView,
                           D3DXMATRIX const& hairWorldViewProj);

    void VisualizeAVSM(ID3D11DeviceContext* d3dDeviceContext,
                       ID3D11RenderTargetView* backBuffer,
                       D3D11_VIEWPORT* viewport);

    void RenderHair(ID3D11DeviceContext* d3dDeviceContext,
                    ID3D11RenderTargetView* backBuffer,
                    D3D11_VIEWPORT* viewport,
                    const D3DXMATRIXA16 &cameraWorldView,
                    const UIConstants* ui,
                    const Options &options);

    void ClearShadowBuffers(ID3D11DeviceContext* d3dDeviceContext, const UIConstants* ui);

    void ClearOITBuffers(ID3D11DeviceContext* d3dDeviceContext, TRANSPARENCY_METHOD transparencyMethod);

    static std::string GetSceneName(SCENE_SELECTION scene);
    static SCENE_SELECTION GetSceneEnum(const std::string &name);
    static std::string GetTransparencyMethodName(TRANSPARENCY_METHOD method);
    static TRANSPARENCY_METHOD GetTransparencyMethodEnum(const std::string &name);
    std::string GetShadowMethod(const UIConstants &ui);

    unsigned int mAVSMShadowTextureDim;
    unsigned int mShadowAASamples;
    unsigned int mLisTexNodeCount;

    ID3D11InputLayout*  mMeshVertexLayout;

    ID3D11VertexShader* mGeometryVS;
    ID3D11ShaderReflection* mGeometryVSReflector;
    ID3D11PixelShader*  mGeometryPS[2];
    ID3D11ShaderReflection* mGeometryPSReflector[2];
    ID3D11PixelShader*  mGeometryCapturePS[4];
    ID3D11ShaderReflection* mGeometryCapturePSReflector[4];

    ID3D11PixelShader*  mMSAAResolvePassPS;
    ID3D11ShaderReflection* mMSAAResolvePassPSReflector;

    CDXUTSDKMesh mSkyboxMesh;
    ID3D11Texture2D* mSkyboxTexture;
    ID3D11ShaderResourceView* mSkyboxSRV;
    ID3D11VertexShader* mSkyboxVS;
    ID3D11ShaderReflection* mSkyboxVSReflector;
    ID3D11PixelShader* mSkyboxPS;
    ID3D11ShaderReflection* mSkyboxPSReflector;

    ID3D11Texture2D* mIBLTexture;
    ID3D11ShaderResourceView* mIBLSRV;

    ID3D11VertexShader* mFullScreenTriangleVS;
    ID3D11ShaderReflection* mFullScreenTriangleVSReflector;
    ID3D11PixelShader* mFullScreenTrianglePS;
    ID3D11ShaderReflection* mFullScreenTrianglePSReflector;

    // Particle and AVSM shaders
    ID3D11VertexShader*  mParticleShadingVS;
    ID3D11ShaderReflection* mParticleShadingVSReflector;
    ID3D11PixelShader*   mParticleShadingPS[MAX_SHADER_VARIATIONS];
    ID3D11ShaderReflection* mParticleShadingPSReflector[MAX_SHADER_VARIATIONS];
    ID3D11PixelShader*   mParticleShadingCapturePS[2];
    ID3D11ShaderReflection* mParticleShadingCapturePSReflector[2];
    ID3D11PixelShader*   mParticleAVSMCapturePS; // Capture-all-fragments shaders
    ID3D11ShaderReflection* mParticleAVSMCapturePSReflector;
    ID3D11PixelShader*   mAVSMUnsortedResolvePS[MAX_SHADER_VARIATIONS];
    ID3D11ShaderReflection* mAVSMUnsortedResolvePSReflector[MAX_SHADER_VARIATIONS];
    ID3D11PixelShader*   mAVSMInsertionSortResolvePS[MAX_SHADER_VARIATIONS];
    ID3D11ShaderReflection* mAVSMInsertionSortResolvePSReflector[MAX_SHADER_VARIATIONS]; 
    ID3D11PixelShader*   mABufferSortedResolvePS[2];
    ID3D11ShaderReflection* mABufferSortedResolvePSReflector[2];
    ID3D11PixelShader*   mABufferResolvePS[2];
    ID3D11ShaderReflection* mABufferResolvePSReflector[2];
    ID3D11PixelShader*   mAOITResolvePS[8];
    ID3D11ShaderReflection* mAOITResolvePSReflector[8];
    ID3D11PixelShader*   mAOITCompositePS;
    ID3D11ShaderReflection* mAOITCompositePSReflector;


    // Debug (visualization) shaders
    ID3D11ComputeShader*   mGatherDebugDataCS;
    ID3D11ShaderReflection* mGatherDebugDataCSReflector;
    ID3D11ComputeShader*   mGatherDebugAOITDataCS[4];
    ID3D11ShaderReflection* mGatherDebugAOITDataCSReflector[4];
    ID3D11VertexShader*    mDebugDataVS;
    ID3D11ShaderReflection* mDebugDataVSReflector;
    ID3D11PixelShader*     mDebugDataPS;
    ID3D11ShaderReflection* mDebugDataPSReflector;

    // Stats shaders
    ID3D11ComputeShader*   mComputeFragmentStatsCS;
    ID3D11ShaderReflection* mComputeFragmentStatsCSReflector;
    ID3D11ComputeShader*   mClearFragmentStatsCS;
    ID3D11ShaderReflection* mClearFragmentStatsCSReflector;

    ID3D11Buffer* mDebugDrawShaderConstants;
    ID3D11Buffer* mPerFrameConstants;
    ID3D11Buffer* mParticlePerFrameConstants;
    ID3D11Buffer* mParticlePerPassConstants;
    ID3D11Buffer* mListTextureConstants;
    ID3D11Buffer* mFragmentListConstants;
    ID3D11Buffer* mAVSMConstants;

    ID3D11Buffer* mVolumeShadowConstants;

    ID3D11RasterizerState* mRasterizerState;
    ID3D11RasterizerState* mDoubleSidedRasterizerState;
    ID3D11RasterizerState* mShadowRasterizerState;
    ID3D11RasterizerState* mParticleRasterizerState;
    ID3D11RasterizerState* mHairRasterizerState;

    ID3D11DepthStencilState* mDefaultDepthStencilState;
    ID3D11DepthStencilState* mDepthDisabledDepthStencilState;
    ID3D11DepthStencilState* mDepthWritesDisabledDepthStencilState;
    ID3D11DepthStencilState* mParticleDepthStencilState;
    ID3D11DepthStencilState* mAVSMCaptureDepthStencilState;
    ID3D11DepthStencilState* mHairCaptureDepthStencilState;
    ID3D11DepthStencilState* mHairRenderDepthStencilState;

    ID3D11BlendState* mMultiplicativeBlendState;
    ID3D11BlendState* mAdditiveBlendState;
    ID3D11BlendState* mGeometryOpaqueBlendState;
    ID3D11BlendState* mGeometryTransparentBlendState;
    ID3D11BlendState* mLightingBlendState;
    ID3D11BlendState* mParticleBlendState;
    ID3D11BlendState* mOITBlendState;
    ID3D11BlendState* mHairRenderBlendState;
   

    ID3D11SamplerState* mDiffuseSampler;
    ID3D11SamplerState* mIBLSampler;
    ID3D11SamplerState* mAVSMSampler;

    std::tr1::shared_ptr<Depth2D> mDepthBuffer;

    // Debug (transmittance plot)
    ID3D11Buffer*                   mDebugDrawIndirectArgs;
    ID3D11Buffer*                   mDebugDrawIndirectArgsStag;
    ID3D11UnorderedAccessView*      mDebugDrawIndirectArgsUAV;
    ID3D11Buffer*                   mDebugDrawNodeData;
    ID3D11UnorderedAccessView*      mDebugDrawNodeDataUAV;
    ID3D11ShaderResourceView*       mDebugDrawNodeDataSRV;
    ID3D11Buffer*                   mDebugDrawNodeDataStag;

    // Statistics
    ID3D11Buffer*                   mFragmentStats;
    ID3D11UnorderedAccessView*      mFragmentStatsUAV;
    ID3D11ShaderResourceView*       mFragmentStatsSRV;
    ID3D11Buffer*                   mFragmentStatsStaging;


    int                             mAOITShaderIdx;

    unsigned int                    mMSAASampleCount;

    // AVSM 
    D3D11_VIEWPORT                  mAVSMShadowViewport;
    int                             mAVSMShadowNodeCount;
    int                             mShadowShaderIdx;
    std::tr1::shared_ptr<Texture2D> mAVSMTextures;
    ID3D11Texture2D*                mAVSMTexturesDebug;
    ID3D11Buffer*                   mAVSMStructBuf;
    ID3D11UnorderedAccessView*      mAVSMStructBufUAV;
    ID3D11ShaderResourceView*       mAVSMStructBufSRV;

    std::tr1::shared_ptr<Texture2D> mCompositeBuffer;

    // List texture 
    std::tr1::shared_ptr<Texture2D> mListTexFirstSegmentNodeOffset;
    std::tr1::shared_ptr<Texture2D> mListTexFirstVisibilityNodeOffset;
    std::tr1::shared_ptr<Texture2D> mFragmentListFirstNodeOffset;
    std::tr1::shared_ptr<Texture2D> mMSAAResolveBuffer;
    ID3D11Buffer*                   mListTexSegmentNodes;
    ID3D11Buffer*                   mListTexSegmentNodesDebug;
    ID3D11Buffer*                   mFragmentListNodes;
    ID3D11Buffer*                   mListTexVisibilityNodes;
    ID3D11Buffer*                   mListTexVisibilityNodesDebug;
    ID3D11UnorderedAccessView*      mListTexSegmentNodesUAV;
    ID3D11ShaderResourceView*       mListTexSegmentNodesSRV;
    ID3D11UnorderedAccessView*      mFragmentListNodesUAV;
    ID3D11ShaderResourceView*       mFragmentListNodesSRV;
    ID3D11UnorderedAccessView*      mListTexVisibilityNodesUAV;
    ID3D11ShaderResourceView*       mListTexVisibilityNodesSRV;
    ID3D11Texture2D*                mListTexFirstOffsetDebug;

    // Hair constants
    unsigned int                    mHairLTMaxNodeCount;

    // Hair constant buffers
    ID3D11Buffer*                   mHairConstants;
    ID3D11Buffer*                   mHairLTConstants;

    // Hair shaders
    ID3D11VertexShader*             mHairVS;
    ID3D11GeometryShader*           mHairGS;
    ID3D11PixelShader*              mCameraHairCapturePS[2];
    ID3D11PixelShader*              mStandardCameraHairRenderPS[MAX_SHADER_VARIATIONS];
    ID3D11PixelShader*              mShadowHairCapturePS;
    ID3D11ShaderReflection*         mHairVSReflector;
    ID3D11ShaderReflection*         mHairGSReflector;
    ID3D11ShaderReflection*         mCameraHairCapturePSReflector[MAX_SHADER_VARIATIONS];
    ID3D11ShaderReflection*         mStandardCameraHairRenderPSReflector[MAX_SHADER_VARIATIONS];
    ID3D11ShaderReflection*         mShadowHairCapturePSReflector;

    // Hair renderer
    Hair*                           mHair;
    bool                            mHairMeshDirty;

    float                           mLastTime;

    // Queries
    enum GPUQueries {
        GPUQ_CAPTURE_PASS           = 0,
        GPUQ_RESOLVE_PASS           = 1,
        // NUM
        GPUQ_QUERY_COUNT,
    };

    ID3D11Query*                    mGPUQuery[GPUQ_QUERY_COUNT][3];

    ShaderList<ID3D11VertexShader>   mLoadedVS;
    ShaderList<ID3D11GeometryShader> mLoadedGS;
    ShaderList<ID3D11PixelShader>    mLoadedPS;
    ShaderList<ID3D11ComputeShader>  mLoadedCS;
};

#endif // H_APP
