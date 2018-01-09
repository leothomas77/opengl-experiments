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

#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "SDKMesh.h"
#include "App.h"
#include "ParticleSystem.h"
#include "fps.h"

#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>

//--------------------------------------------------------------------------------------
// Defines
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
enum {
    UI_TEXT,
    UI_CONTROLMODE,
    UI_TOGGLEFULLSCREEN,
    UI_CHANGEDEVICE,
    UI_ANIMATELIGHT,
    UI_DOUBLESIDEDPOLYGONS,
    UI_ALPHATEST,
    UI_FACENORMALS,
    UI_ENABLETRANSPARENCY,
    UI_ENABLESTATS,
    UI_VOLUMETRICMODEL,
    UI_AVSMAUTOBOUNDS,
    UI_SELECTEDSCENE,
    UI_TRANSPARENCYMETHOD,
    UI_AVSMSORTINGMETHOD,
    UI_AOITNODECOUNT,
    UI_VOLUMESHADOWINGMETHOD,
    UI_PARTICLESIZE,
    UI_PARTICLEOPACITY,
    UI_HAIR_SHADOW_THICKNESS, 
    UI_HAIR_THICKNESS, // world space thickness
    UI_HAIR_OPACITY,
    UI_X,
    UI_Y,
    UI_Z,
    UI_PAUSEPARTICLEANIMATION,
    UI_LIGHTINGONLY,
    UI_VOLUMESHADOWCREATION,
    UI_VOLUMESHADOWLOOKUP,
    UI_SAVESTATE,
    UI_LOADSTATE,
    UI_VISUALIZEABUFFER,
    UI_VISUALIZEAOIT,
    UI_GEOMETRYALPHA,
    UI_GEOMETRYALPHATEXT,
    UI_ENABLETILING,
};

// List these top to bottom, since it is also the reverse draw order
enum {
    HUD_GENERIC = 0,
    HUD_FILTERING,
    HUD_AVSM,
    HUD_HAIR,
    HUD_EXPERT,
    HUD_NUM,
};

enum AVSMSortingMethod {
    AVSM_UNSORTED,
    AVSM_INSERTION_SORT,
};

enum ControlObject
{
    CONTROL_CAMERA = 0,
    CONTROL_LIGHT,
    CONTROL_NUM
};

enum VolumetricModel
{
    VOLUME_NONE      = 0x0,
    VOLUME_PARTICLES = 0x1,
    VOLUME_HAIR      = 0x2,
};

// State
App::Options gNewAppOptions;
UIConstants gNewUIConstants;
D3DXVECTOR3 gNewViewEye, gNewViewLookAt;
D3DXVECTOR3 gNewLightEye, gNewLightLookAt;
bool gRestoreAppOptions = false;
bool gRestoreUIConstants = false;
bool gRestoreViewState = false;

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------

struct CameraKey {
    float time;
    D3DXVECTOR3 eye;
    D3DXVECTOR3 look;
};

enum {
    CAMERAMODE_DEFAULT,
    CAMERAMODE_RECORD,
    CAMERAMODE_PLAYBACK,
} gCameraMode = CAMERAMODE_DEFAULT;

unsigned int const MaxCameraKeys = 10*1024;
CameraKey gCameraPath[MaxCameraKeys];
unsigned int gCameraPathLength = 0;
unsigned int gCameraPathCurrentIndex = 0;
float gCameraPlaybackTime = 0.f;

App* gApp = 0;
App::Options gAppOptions;

extern bool gDXUTResetFPS;

std::ofstream & SerializeVec3(std::ofstream &stream, const D3DXVECTOR3 &vec)
{
    stream << vec.x << " " << vec.y << " " << vec.z;
    return stream;
}

std::ifstream & DeserializeVec3(std::ifstream &stream, D3DXVECTOR3 &vec)
{
    stream >> vec.x;
    stream >> vec.y;
    stream >> vec.z;
    return stream;
}

void SerializeCamera(std::ofstream &stream,
                     const std::string &cameraName,
                     const D3DXVECTOR3 &eye, 
                     const D3DXVECTOR3 &lookAt)
{
    stream << cameraName << " ";
    SerializeVec3(stream, eye);
    stream << " ";
    SerializeVec3(stream, lookAt);
    stream << std::endl;
}

void DeserializeCamera(std::ifstream &stream,
                       D3DXVECTOR3 *eye, 
                       D3DXVECTOR3 *lookAt)
{
    DeserializeVec3(stream, *eye);
    DeserializeVec3(stream, *lookAt);
}

class MainOptions {
  public:
    MainOptions() :
        gControlObject(CONTROL_CAMERA) {}

    void Serialize(std::ofstream &stream) {
        SerializeCamera(stream, "viewerCamera",
                        *gViewerCamera.GetEyePt(), 
                        *gViewerCamera.GetLookAtPt());
        SerializeCamera(stream, "lightCamera",
                        *gLightCamera.GetEyePt(), 
                        *gLightCamera.GetLookAtPt());
        stream << "controlObject " << gControlObject << std::endl;
    }

    void Deserialize(std::ifstream &stream) {
        while (stream.good()) {
            std::string token;
            stream >> token;
            if (token == "{") {
                continue; // ignore
            } else if (token == "}")
                return; // end of this block
            else if (token == "viewerCamera") {
                DeserializeCamera(stream, &gNewViewEye, &gNewViewLookAt);
                gViewerCamera.SetViewParams(&gNewViewEye, &gNewViewLookAt);
            } else if (token == "lightCamera") {
                DeserializeCamera(stream, &gNewLightEye, &gNewLightLookAt);
                gLightCamera.SetViewParams(&gNewLightEye, &gNewLightLookAt);
            } else if (token == "controlObject") {
                stream >> gControlObject;
            }
        }
    }

    CFirstPersonAndOrbitCamera gViewerCamera;
    CFirstPersonAndOrbitCamera gLightCamera;
    int gControlObject;
};

MainOptions gMainOptions;

CDXUTSDKMesh gMesh;
CDXUTSDKMesh gMeshAlpha;
ParticleSystem *gParticleSystem = NULL;
D3DXMATRIXA16 gWorldMatrix;
UINT gFrameWidth = 0;
UINT gFrameHeight = 0;

// DXUT GUI stuff
CDXUTDialogResourceManager gDialogResourceManager;
CD3DSettingsDlg gD3DSettingsDlg;
CDXUTDialog gHUD[HUD_NUM];
CDXUTCheckBox* gAnimateLightCheck = 0;
CDXUTCheckBox* gAnimateViewCheck = 0;
CDXUTComboBox* gSceneSelectCombo = 0;
CDXUTComboBox* gTransparencyMethodCombo = 0;
CDXUTComboBox* gVolumeShadowingMethodCombo = 0;
CDXUTComboBox* gAVSMSortingMethodCombo = 0;
CDXUTComboBox* gAOITNodeCountCombo = 0;
CDXUTTextHelper* gTextHelper = 0;
FPSWriter gFPSWriter;
bool gShowFPS = true;

float gAspectRatio;
bool gDisplayUI = true;

// Any UI state passed directly to rendering shaders
UIConstants gUIConstants;

// Constants
static const float kLightRotationSpeed = 0.15f; // radians/sec
static const float kViewRotationSpeed = 0.15f; // radians/sec
static const float kSliderFactorResolution = 10000.0f;
static const float kParticleSizeFactorUI = 20.0f; // Divisor on [0,100] range of particle size UI to particle size
static const float kDsmErrorFactorUI = 1000.0f;

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* deviceSettings, void* userContext);
void CALLBACK OnFrameMove(double time, FLOAT elapsedTime, void* userContext);
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* noFurtherProcessing,
                         void* userContext);
void CALLBACK OnKeyboard(UINT character, bool keyDown, bool altDown, void* userContext);
void CALLBACK OnGUIEvent(UINT eventID, INT controlID, CDXUTControl* control, void* userContext);
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* d3dDevice, const DXGI_SURFACE_DESC* backBufferSurfaceDesc,
                                     void* userContext);
HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* d3dDevice, IDXGISwapChain* swapChain,
                                         const DXGI_SURFACE_DESC* backBufferSurfaceDesc, void* userContext);
void CALLBACK OnD3D11ReleasingSwapChain(void* userContext);
void CALLBACK OnD3D11DestroyDevice(void* userContext);
void CALLBACK OnD3D11FrameRender(ID3D11Device* d3dDevice, ID3D11DeviceContext* d3dDeviceContext, double time,
                                 FLOAT elapsedTime, void* userContext);

void InitApp(ID3D11Device* d3dDevice, ID3D11DeviceContext* d3dDeviceContext);
void DeinitApp();

void InitUI();
void UpdateViewerCameraNearFar();
CFirstPersonAndOrbitCamera * GetCurrentUserCamera();

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nCmdShow)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    // XXX: 
    // Insert proper alloc number reported by CRT at app exit time to
    // track down the allocation that is leaking.
    // XXX: 
    // _CrtSetBreakAlloc(933);
#endif

    // Set DXUT callbacks
    DXUTSetCallbackDeviceChanging(ModifyDeviceSettings);
    DXUTSetCallbackMsgProc(MsgProc);
    DXUTSetCallbackKeyboard(OnKeyboard);
    DXUTSetCallbackFrameMove(OnFrameMove);

    DXUTSetCallbackD3D11DeviceCreated(OnD3D11CreateDevice);
    DXUTSetCallbackD3D11SwapChainResized(OnD3D11ResizedSwapChain);
    DXUTSetCallbackD3D11FrameRender(OnD3D11FrameRender);
    DXUTSetCallbackD3D11SwapChainReleasing(OnD3D11ReleasingSwapChain);
    DXUTSetCallbackD3D11DeviceDestroyed(OnD3D11DestroyDevice);
    
    DXUTInit(true, true, 0);
    InitUI();

    DXUTSetCursorSettings(true, true);
    DXUTSetHotkeyHandling(true, true, false);
    DXUTCreateWindow(L"ADAPTIVE ORDER INDEPENDENT TRANSPARENCY");

    
    DXUTCreateDevice(D3D_FEATURE_LEVEL_11_0, true, 1280, 720, gAppOptions.msaaSampleCount);
    DXUTMainLoop();

    return DXUTGetExitCode();
}


void InitUI()
{
    // Setup default UI state
    // NOTE: All of these are made directly available in the shader constant buffer
    // This is convenient for development purposes.
    if (gRestoreUIConstants) {
        gUIConstants = gNewUIConstants;
        gRestoreUIConstants = false;
    } else {
        gUIConstants.lightingOnly = 0;  
        gUIConstants.faceNormals = 0;
        gUIConstants.avsmSortingMethod = 0;
        gUIConstants.volumeShadowMethod = VOL_SHADOW_AVSM;
        gUIConstants.enableVolumeShadowLookup = 1;
        gUIConstants.pauseParticleAnimaton = 0;
        gUIConstants.particleSize = 1.0f;
        gUIConstants.particleOpacity = 33;
        gUIConstants.hairThickness = 34;       // Actually, 3.40
        gUIConstants.hairShadowThickness = 10; // Actually, 0.10
        gUIConstants.hairOpacity = 18;         // Actually, 0.18
    }
    
    gD3DSettingsDlg.Init(&gDialogResourceManager);

    for (int i = 0; i < HUD_NUM; ++i) {
        gHUD[i].RemoveAllControls();
        gHUD[i].Init(&gDialogResourceManager);
        gHUD[i].SetCallback(OnGUIEvent);
    }

    int width = 150;
    const int yDelta = 24;
    const int yyDelta = 40;

    // Generic HUD
    {
        CDXUTDialog* HUD = &gHUD[HUD_GENERIC];
        int y = 8;

        HUD->AddButton(UI_TOGGLEFULLSCREEN, L"Toggle full screen", 0, y, width, 23);
        y += yDelta;

        HUD->AddButton(UI_CHANGEDEVICE, L"Change device (F2)", 0, y, width, 23, VK_F2);
        y += yyDelta;

        HUD->AddStatic(UI_TEXT, L"Scene",  -45, 2 + y, 140, 18);
        HUD->AddComboBox(UI_SELECTEDSCENE, 0, y, width, 23, 0, false, &gSceneSelectCombo);
        const int numScenes = 5;
        gSceneSelectCombo->SetDropHeight(numScenes * 40);
        gSceneSelectCombo->AddItem(L"Power Plant", ULongToPtr(POWER_PLANT_SCENE));
        gSceneSelectCombo->AddItem(L"Ground Plane",
                                   ULongToPtr(GROUND_PLANE_SCENE));
        gSceneSelectCombo->AddItem(L"Hair & Prtcls", ULongToPtr(HAIR_AND_PARTICLES_SCENE));
        gSceneSelectCombo->AddItem(L"Teapot", ULongToPtr(TEAPOT_SCENE));
        gSceneSelectCombo->AddItem(L"Teapots", ULongToPtr(TEAPOTS_SCENE));
        gSceneSelectCombo->AddItem(L"Dragon", ULongToPtr(DRAGON_SCENE));
        gSceneSelectCombo->AddItem(L"Buddhas", ULongToPtr(HAPPYS_SCENE));
        gSceneSelectCombo->SetSelectedByIndex(gAppOptions.scene);
        gSceneSelectCombo->SetDropHeight(gSceneSelectCombo->GetNumItems() * 23);
        y += yDelta;

        HUD->AddStatic(UI_TEXT, L"Camera Mode",  -90, y, 140, 18);
        CDXUTComboBox* ControlMode;
        HUD->AddComboBox(UI_CONTROLMODE, 0, y, width, 23, 0, false, &ControlMode);
        y += yDelta;
        ControlMode->AddItem(L"Camera", IntToPtr(CONTROL_CAMERA));
        ControlMode->AddItem(L"Light", IntToPtr(CONTROL_LIGHT));
        ControlMode->SetSelectedByData(IntToPtr(gMainOptions.gControlObject));

        HUD->AddStatic(UI_TEXT, L"Models",  -52, y, 140, 18);
        CDXUTComboBox* VolumeModel;
        HUD->AddComboBox(UI_VOLUMETRICMODEL, 0, y, width, 23, 0, false, &VolumeModel);
        VolumeModel->AddItem(L"None", IntToPtr(VOLUME_NONE));
        VolumeModel->AddItem(L"Particles", IntToPtr(VOLUME_PARTICLES));
        VolumeModel->AddItem(L"Hair", IntToPtr(VOLUME_HAIR));
        VolumeModel->AddItem(L"Hair & Prtcls", IntToPtr(VOLUME_PARTICLES | VOLUME_HAIR));
        VolumeModel->SetSelectedByData(IntToPtr((gAppOptions.enableParticles ? VOLUME_PARTICLES : 0) |
                                                (gAppOptions.enableHair      ? VOLUME_HAIR : 0)));
        y += yDelta;

        HUD->AddStatic(UI_TEXT, L"Transparency Method",  -150, y, 140, 18);
        HUD->AddComboBox(UI_TRANSPARENCYMETHOD, 0, y, width, 23, 0, false, &gTransparencyMethodCombo);
        gTransparencyMethodCombo->AddItem(L"Alpha Blend", IntToPtr(TM_ALPHA_BLENDING));
        gTransparencyMethodCombo->AddItem(L"A-Buffer", IntToPtr(TM_ABUFFER_SORTED));        
        gTransparencyMethodCombo->AddItem(L"A-Buffer Iter.", IntToPtr(TM_ABUFFER_ITERATIVE));        
        gTransparencyMethodCombo->AddItem(L"AT", IntToPtr(TM_AOIT));
        gTransparencyMethodCombo->SetSelectedByData(IntToPtr(gAppOptions.transparencyMethod));
        gTransparencyMethodCombo->SetDropHeight(gTransparencyMethodCombo->GetNumItems() * 23);
        y += yyDelta;

        HUD->AddStatic(UI_TEXT, L"AT Node Count",  -117, y, 140, 18);
        HUD->AddComboBox(UI_AOITNODECOUNT, 0, y, width, 23, 0, false, &gAOITNodeCountCombo);
        gAOITNodeCountCombo->AddItem(L"8", ULongToPtr(8));
        gAOITNodeCountCombo->AddItem(L"16", ULongToPtr(16));
        gAOITNodeCountCombo->AddItem(L"24", ULongToPtr(24));
        gAOITNodeCountCombo->AddItem(L"32", ULongToPtr(32));
        gAOITNodeCountCombo->SetSelectedByData(ULongToPtr(gAppOptions.AOITNodeCount));
        y += yyDelta;

        HUD->AddCheckBox(UI_ENABLETRANSPARENCY, L"Enable Transp", 0, y, width, 23, gAppOptions.enableTransparency != 0);
        y += yyDelta;

        HUD->AddCheckBox(UI_PAUSEPARTICLEANIMATION, L"Stop Part Anim", 0, y, width, 23, gUIConstants.pauseParticleAnimaton != 0);
        y += yDelta;

        HUD->AddCheckBox(UI_AVSMAUTOBOUNDS, L"Shadow Bounds", 0, y, width, 23, 
                         gAppOptions.enableAutoBoundsAVSM != 0);
        y += yDelta;

        HUD->AddCheckBox(UI_DOUBLESIDEDPOLYGONS, L"No BFC", 0, y, width, 23, 
                         gAppOptions.enableDoubleSidedPolygons != 0);
        y += yDelta;
        
        HUD->AddCheckBox(UI_ALPHATEST, L"Alpha Cutoff", 0, y, width, 23, 
                         gAppOptions.enableAlphaCutoff != 0);
        y += yDelta;

        HUD->AddCheckBox(UI_ANIMATELIGHT, L"Animate Light", 0, y, width, 23, false, VK_SPACE, false, &gAnimateLightCheck);
        y += yDelta;

        HUD->AddCheckBox(UI_ANIMATELIGHT, L"Animate View", 0, y, width, 23, false, VK_SPACE, false, &gAnimateViewCheck);
        y += yyDelta;

        //HUD->AddCheckBox(UI_ENABLETILING, L"Enable Tiling", 0, y, width, 23, gAppOptions.enableTiling != 0);
        //y += yDelta;

        HUD->AddCheckBox(UI_ENABLESTATS, L"Enable Stats", 0, y, width, 23, gAppOptions.enableStats != 0);
        y += yyDelta;

        HUD->AddStatic(UI_TEXT, L"View Visibility Fn:", 0, y, width, 18);
        y += yDelta;
        HUD->AddCheckBox(UI_VISUALIZEABUFFER, L"A-Buffer (red)", 0, y, width, 23, gAppOptions.visualizeABuffer != 0);
        y += yDelta;
        HUD->AddCheckBox(UI_VISUALIZEAOIT, L"AT (blue)", 0, y, width, 23, gAppOptions.visualizeAOIT != 0);
        y += yyDelta;

        HUD->AddStatic(UI_GEOMETRYALPHATEXT, L"Geometry Alpha=8.88", -150, y, 140, 18);
        HUD->AddSlider(UI_GEOMETRYALPHA, 5, y, width - 10, 22, 0, 100, (int)(100.0f * gAppOptions.geometryAlpha));
        OnGUIEvent(0, UI_GEOMETRYALPHA, HUD->GetSlider(UI_GEOMETRYALPHA), 0);
        y += yDelta;

        HUD->AddStatic(UI_TEXT, L"Particle Size",  -102, y, 140, 18);
        HUD->AddSlider(UI_PARTICLESIZE, 5, y, width - 10, 22, 0, 100, 
                       static_cast<UINT>(gUIConstants.particleSize * kParticleSizeFactorUI));
        y += yDelta;

        HUD->AddStatic(UI_TEXT, L"Particle Opacity",  -125, y, 140, 18);
        HUD->AddSlider(UI_PARTICLEOPACITY, 5, y, width - 10, 22, 0, 100, gUIConstants.particleOpacity);

        HUD->SetSize(width, y);
    }

    // Hair HUD
    {
        CDXUTDialog* HUD = &gHUD[HUD_HAIR];
        int y = -yDelta;

        HUD->AddStatic(UI_TEXT, L"Hair thickness:",  5, y, 140, 18);
        y += yDelta;
        HUD->AddSlider(UI_HAIR_THICKNESS, 5, y, 170, 22, 0, 100,
                       gUIConstants.hairThickness);
        y += yDelta;

        HUD->AddStatic(UI_TEXT, L"Hair (shadow) thickness:",  5, y, 140, 18);
        y += yDelta;
        HUD->AddSlider(UI_HAIR_SHADOW_THICKNESS, 5, y, 170, 22, 0, 100,
                       gUIConstants.hairShadowThickness);
        y += yDelta;

        HUD->AddStatic(UI_TEXT, L"Hair opacity:",  5, y, 140, 18);
        y += yDelta;
        HUD->AddSlider(UI_HAIR_OPACITY, 5, y, 170, 22, 0, 100,
                       gUIConstants.hairOpacity);
        y += yDelta;

        HUD->SetSize(width, y);

        // Initially hidden
        HUD->SetVisible(false);
    }

    // Expert HUD
    {
        CDXUTDialog* HUD = &gHUD[HUD_EXPERT];
        int y = 0;
    
        HUD->AddCheckBox(UI_VOLUMESHADOWCREATION, L"Do Volume Shadow Creation", 0,
                         y, width, 23, gAppOptions.enableVolumeShadowCreation != 0);
        y += 26;

        HUD->AddCheckBox(UI_VOLUMESHADOWLOOKUP, L"Do Volume Shadow Lookup", 0,
                         y, width, 23, gUIConstants.enableVolumeShadowLookup != 0);
        y += 26;

        HUD->AddButton(UI_SAVESTATE, L"Save State (F11)", 0, y, width, 23, VK_F11);
        y += 26;

        HUD->AddButton(UI_LOADSTATE, L"Load State (F12)", 0, y, width, 23, VK_F12);
        y += 26;

        HUD->SetSize(width, y);

        // Initially hidden
        HUD->SetVisible(false);
    }
}

void InitApp(ID3D11Device* d3dDevice, ID3D11DeviceContext* d3dDeviceContext)
{
    DeinitApp();

    unsigned int avsmShadowTextureDim = 256;

    if (gRestoreAppOptions) {
        gAppOptions = gNewAppOptions;
        gRestoreAppOptions = false;
    }

    gApp = new App(d3dDevice, 
                   d3dDeviceContext, 
                   gAppOptions.msaaSampleCount, 
                   gAppOptions.ShadowNodeCount, 
                   gAppOptions.AOITNodeCount,
                   avsmShadowTextureDim);

    // Initialize with the current surface description
    gApp->OnD3D11ResizedSwapChain(d3dDevice, DXUTGetDXGIBackBufferSurfaceDesc());

    // TODO: Zero out the elapsed time for the next frame...
}

void DeinitApp()
{
    SAFE_DELETE(gApp);
}


bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* deviceSettings, void* userContext)
{
    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool firstTime = true;
    if (firstTime) {
        firstTime = false;
        if (deviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE) {
            DXUTDisplaySwitchingToREFWarning(deviceSettings->ver);
        }
    }

    gAppOptions.msaaSampleCount = deviceSettings->d3d11.sd.SampleDesc.Count;

    // Also don't need a depth/stencil buffer... we'll manage that ourselves
    deviceSettings->d3d11.AutoCreateDepthStencil = false;

    return true;
}


void CALLBACK OnFrameMove(double time, FLOAT elapsedTime, void* userContext)
{
    // If requested, orbit the light
    if (gAnimateLightCheck && gAnimateLightCheck->GetChecked()) {
        D3DXVECTOR3 eyePt = *gMainOptions.gLightCamera.GetEyePt();
        D3DXVECTOR3 lookAtPt = *gMainOptions.gLightCamera.GetLookAtPt();

        // Note: Could be replaced by gMesh.GetMeshBBoxCenter()
        D3DXVECTOR3 sceneCenter = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        
        // rotate around center of the scene
        float angle = kLightRotationSpeed * elapsedTime;
        eyePt -= sceneCenter;
        lookAtPt -= sceneCenter;
        D3DXMATRIX rotate;
        D3DXMatrixRotationY(&rotate, angle);
        D3DXVECTOR4 newEyePt;
        D3DXVECTOR4 newLookAtPt;
        D3DXVec3Transform(&newEyePt, &eyePt, &rotate);
        D3DXVec3Transform(&newLookAtPt, &lookAtPt, &rotate);
        eyePt = D3DXVECTOR3(newEyePt);
        lookAtPt = D3DXVECTOR3(newLookAtPt);
        eyePt += sceneCenter;
        lookAtPt += sceneCenter;

        gMainOptions.gLightCamera.SetViewParams(&eyePt, &lookAtPt);
    }
    
    if (gCameraMode == CAMERAMODE_PLAYBACK) {
        gCameraPlaybackTime += elapsedTime;
        for (;; ++gCameraPathCurrentIndex) {
            if (gCameraPathCurrentIndex+1 == gCameraPathLength) {
                OutputDebugStringA("Camera path playback complete.\n");
                gCameraMode = CAMERAMODE_DEFAULT;
                break;
            }
            if (gCameraPath[gCameraPathCurrentIndex+1].time > gCameraPlaybackTime) {
                // TODO: only really ned to set if gCameraPathCurrentIndex changed...
                gMainOptions.gViewerCamera.SetViewParams(&gCameraPath[gCameraPathCurrentIndex].eye,
                                                         &gCameraPath[gCameraPathCurrentIndex].look);
                break;
            }
        }
    } else {
        // Update the camera's position based on animation and user input 
        if (gAnimateViewCheck && gAnimateViewCheck->GetChecked()) {
            D3DXVECTOR3 eyePt = *gMainOptions.gViewerCamera.GetEyePt();
            D3DXVECTOR3 lookAtPt = *gMainOptions.gViewerCamera.GetLookAtPt();
            D3DXVECTOR3 at2eye = eyePt - lookAtPt;

            float angle = kViewRotationSpeed * elapsedTime;
            D3DXMATRIX rotate;
            D3DXMatrixRotationY(&rotate, angle);
            D3DXVec3TransformCoord(&at2eye, &at2eye, &rotate);

            eyePt = lookAtPt + at2eye;

            gMainOptions.gViewerCamera.SetViewParams(&eyePt, &lookAtPt);
        }
        gMainOptions.gLightCamera.FrameMove(elapsedTime);
        gMainOptions.gViewerCamera.FrameMove(elapsedTime);

        if (gCameraMode == CAMERAMODE_RECORD) {
            if (gCameraPathLength < MaxCameraKeys) {
                gCameraPath[gCameraPathLength].time = gCameraPathLength ? (gCameraPath[gCameraPathLength-1].time + elapsedTime) : 0.f;
                gCameraPath[gCameraPathLength].eye  = *gMainOptions.gViewerCamera.GetEyePt();
                gCameraPath[gCameraPathLength].look = *gMainOptions.gViewerCamera.GetLookAtPt();

                ++gCameraPathLength;
                if (gCameraPathLength == MaxCameraKeys) {
                    OutputDebugStringA("Allocated camera path is full!\n");
                    OutputDebugStringA("Stopped recording camera.\n");
                    gCameraMode = CAMERAMODE_DEFAULT;
                }
            }
        }
    }
}


LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* noFurtherProcessing,
                          void* userContext)
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *noFurtherProcessing = gDialogResourceManager.MsgProc(hWnd, uMsg, wParam, lParam );
    if (*noFurtherProcessing) {
        return 0;
    }

    // Pass messages to settings dialog if its active
    if (gD3DSettingsDlg.IsActive()) {
        gD3DSettingsDlg.MsgProc(hWnd, uMsg, wParam, lParam);
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    for (int i = 0; i < HUD_NUM; ++i) {
        *noFurtherProcessing = gHUD[i].MsgProc(hWnd, uMsg, wParam, lParam);
        if(*noFurtherProcessing) {
            return 0;
        }
    }

    switch(uMsg) {
        case WM_LBUTTONUP: {
            POINT ptCursor = {(short)LOWORD(lParam), (short)HIWORD(lParam)};
            int avsmShadowTextureDim = 256;
                
            if (ptCursor.x < avsmShadowTextureDim &&
                    ptCursor.y < avsmShadowTextureDim) {
                gAppOptions.pickedX = ptCursor.x;
                gAppOptions.pickedY = ptCursor.y;
            }
            break;
        }
    }

    if (uMsg == WM_MBUTTONUP) {
        gAppOptions.pickedX = LOWORD(lParam);
        gAppOptions.pickedY = HIWORD(lParam);
    }

    // Pass all remaining windows messages to camera so it can respond to user input
    GetCurrentUserCamera()->HandleMessages(hWnd, uMsg, wParam, lParam);

    return 0;
}

static void
DestroyScene()
{
    gMesh.Destroy();
    gMeshAlpha.Destroy();
    SAFE_DELETE(gParticleSystem);
}
static void 
CreateParticles(ID3D11Device *d3dDevice, SCENE_SELECTION scene)
{
    SAFE_DELETE(gParticleSystem);

    // Set up shader define macros
    std::string shadowAAStr; {
        std::ostringstream oss;
        unsigned int avsmShadowTextureDim = 256;

        oss << avsmShadowTextureDim;
        shadowAAStr = oss.str();
    }
    std::string avsmShadowNodeCountStr; {
        std::ostringstream oss;
        unsigned int avsmNodeCount = 8;
        oss << avsmNodeCount;
        avsmShadowNodeCountStr = oss.str();
    }

    D3D10_SHADER_MACRO shaderDefines[] = {
        {"SHADOWAA_SAMPLES", shadowAAStr.c_str()},
        {"AVSM_NODE_COUNT", avsmShadowNodeCountStr.c_str()},
        {0, 0}
    };

    switch (scene) {
        case HAIR_AND_PARTICLES_SCENE: {
            ParticleEmitter emitter0;
            {
                emitter0.mDrag           = 0.2f;
                emitter0.mGravity        = 0.0f;
                emitter0.mRandScaleX     = 2.5f;
                emitter0.mRandScaleY     = 2.0f;
                emitter0.mRandScaleZ     = 2.5f;
                emitter0.mLifetime       = 8.0f;
                emitter0.mStartSize      = 13.0f;//3.0f;//
                emitter0.mSizeRate       = 0.07f;
                emitter0.mpPos[0]        = -40.0f;
                emitter0.mpPos[1]        = -2.0f; 
                emitter0.mpPos[2]        = -55.0;
                emitter0.mpVelocity[0]   = 0.3f;
                emitter0.mpVelocity[1]   = 10.0f;
                emitter0.mpVelocity[2]   = 0.5f;
            }

            
            ParticleEmitter largeCoolingTower;
            {
                largeCoolingTower.mDrag           = 0.2f;
                largeCoolingTower.mGravity        = 0.0f;
                largeCoolingTower.mRandScaleX     = 5.5f;
                largeCoolingTower.mRandScaleY     = 4.0f;
                largeCoolingTower.mRandScaleZ     = 5.5f;
                largeCoolingTower.mLifetime       = 17.0f;
                largeCoolingTower.mStartSize      = 10.0f;//5.0f;//
                largeCoolingTower.mSizeRate       = 0.05f;
                largeCoolingTower.mpPos[0]        = -40.0f;
                largeCoolingTower.mpPos[1]        = -2.0f; 
                largeCoolingTower.mpPos[2]        = -40.0;
                largeCoolingTower.mpVelocity[0]   = 0.6f;
                largeCoolingTower.mpVelocity[1]   = 15.0f;
                largeCoolingTower.mpVelocity[2]   = 0.6f;
            }           

            const ParticleEmitter* emitters[3] = {&largeCoolingTower}; //{&emitter0, &largeCoolingTower};
            const UINT maxNumPartices = 5000;
            gParticleSystem = new ParticleSystem(maxNumPartices);
            gParticleSystem->InitializeParticles(emitters, 1, d3dDevice, 8, 8, shaderDefines);

            break;
        }
        case POWER_PLANT_SCENE: {
            ParticleEmitter emitter0;
            {
                emitter0.mDrag           = 0.2f;
                emitter0.mGravity        = 0.0f;
                emitter0.mRandScaleX     = 2.5f;
                emitter0.mRandScaleY     = 2.0f;
                emitter0.mRandScaleZ     = 2.5f;
                emitter0.mLifetime       = 7.0f;
                emitter0.mStartSize      = 6.0f;//3.0f;//
                emitter0.mSizeRate       = 0.05f;
                emitter0.mpPos[0]        = 40.0f;
                emitter0.mpPos[1]        = -2.0f; 
                emitter0.mpPos[2]        = 0.0;
                emitter0.mpVelocity[0]   = 0.3f;
                emitter0.mpVelocity[1]   = 10.0f;
                emitter0.mpVelocity[2]   = 0.5f;
            }

            ParticleEmitter emitter1;
            {
                emitter1.mDrag           = 0.2f;
                emitter1.mGravity        = 0.0f;
                emitter1.mRandScaleX     = 3.0f;
                emitter1.mRandScaleY     = 2.0f;
                emitter1.mRandScaleZ     = 3.0f;
                emitter1.mLifetime       = 5.0f;
                emitter1.mStartSize      = 6.0f;//3.0f;//
                emitter1.mSizeRate       = 0.005f;
                emitter1.mpPos[0]        = 60.0f;
                emitter1.mpPos[1]        = -2.0f; 
                emitter1.mpPos[2]        = 14.0;
                emitter1.mpVelocity[0]   = 0.0f;
                emitter1.mpVelocity[1]   = 6.0f;
                emitter1.mpVelocity[2]   = 0.0f;
            }
            
            ParticleEmitter largeCoolingTower;
            {
                largeCoolingTower.mDrag           = 0.2f;
                largeCoolingTower.mGravity        = 0.0f;
                largeCoolingTower.mRandScaleX     = 5.5f;
                largeCoolingTower.mRandScaleY     = 4.0f;
                largeCoolingTower.mRandScaleZ     = 5.5f;
                largeCoolingTower.mLifetime       = 7.0f;
                largeCoolingTower.mStartSize      = 10.0f;//5.0f;//
                largeCoolingTower.mSizeRate       = 0.05f;
                largeCoolingTower.mpPos[0]        = 0.0f;
                largeCoolingTower.mpPos[1]        = -2.0f; 
                largeCoolingTower.mpPos[2]        = 0.0;
                largeCoolingTower.mpVelocity[0]   = 0.6f;
                largeCoolingTower.mpVelocity[1]   = 15.0f;
                largeCoolingTower.mpVelocity[2]   = 0.6f;
            }           

            const ParticleEmitter* emitters[3] = {&emitter0, &emitter1, &largeCoolingTower};
            const UINT maxNumPartices = 5000;
            gParticleSystem = new ParticleSystem(maxNumPartices);
            gParticleSystem->InitializeParticles(emitters, 3, d3dDevice, 8, 8, shaderDefines);

            break;
        }

        case GROUND_PLANE_SCENE:
        case TEAPOT_SCENE:
        case TEAPOTS_SCENE:
        case DRAGON_SCENE:
        case HAPPYS_SCENE:
        {
            ParticleEmitter emitter1;
            {
                emitter1.mDrag           = 0.2f;
                emitter1.mGravity        = 0.0f;
                emitter1.mRandScaleX     = 3.0f;
                emitter1.mRandScaleY     = 2.0f;
                emitter1.mRandScaleZ     = 3.0f;
                emitter1.mLifetime       = 5.0f;
                emitter1.mStartSize      = 6.0f;//3.0f;//
                emitter1.mSizeRate       = 0.005f;
                emitter1.mpPos[0]        = 0.0f;
                emitter1.mpPos[1]        = -2.0f; 
                emitter1.mpPos[2]        = -40.0;
                emitter1.mpVelocity[0]   = 0.0f;
                emitter1.mpVelocity[1]   = 6.0f;
                emitter1.mpVelocity[2]   = 0.0f;
            }
            
            ParticleEmitter emitter0;
            {
                emitter0.mDrag           = 0.2f;
                emitter0.mGravity        = 0.0f;
                emitter0.mRandScaleX     = 2.5f;
                emitter0.mRandScaleY     = 2.0f;
                emitter0.mRandScaleZ     = 2.5f;
                emitter0.mLifetime       = 7.0f;
                emitter0.mStartSize      = 6.0f;//3.0f;//
                emitter0.mSizeRate       = 0.05f;
                emitter0.mpPos[0]        = -20.0f;
                emitter0.mpPos[1]        = -2.0f; 
                emitter0.mpPos[2]        = -50.0;
                emitter0.mpVelocity[0]   = 0.3f;
                emitter0.mpVelocity[1]   = 10.0f;
                emitter0.mpVelocity[2]   = 0.5f;
            }


            ParticleEmitter largeCoolingTower;
            {
                largeCoolingTower.mDrag           = 0.2f;
                largeCoolingTower.mGravity        = 0.0f;
                largeCoolingTower.mRandScaleX     = 5.5f;
                largeCoolingTower.mRandScaleY     = 4.0f;
                largeCoolingTower.mRandScaleZ     = 5.5f;
                largeCoolingTower.mLifetime       = 7.0f;
                largeCoolingTower.mStartSize      = 10.0f;//5.0f;//
                largeCoolingTower.mSizeRate       = 0.05f;
                largeCoolingTower.mpPos[0]        = -60.0f;
                largeCoolingTower.mpPos[1]        = -2.0f; 
                largeCoolingTower.mpPos[2]        = -70.0;
                largeCoolingTower.mpVelocity[0]   = 0.6f;
                largeCoolingTower.mpVelocity[1]   = 15.0f;
                largeCoolingTower.mpVelocity[2]   = 0.6f;
            }           

            const ParticleEmitter* emitters[3] = {&emitter0, &emitter1, &largeCoolingTower};
            const UINT maxNumPartices = 5000;
            gParticleSystem = new ParticleSystem(maxNumPartices);
            gParticleSystem->InitializeParticles(emitters, 3, d3dDevice, 8, 8, shaderDefines);

            break;
        }
        default: {
            assert(false);
        }
    }
}



static void
InitScene(ID3D11Device *d3dDevice)
{
    DestroyScene();

    D3DXVECTOR3 cameraEye(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 cameraAt(0.0f, 0.0f, 0.0f);
    float sceneScaling = 1.0f;
    float rotationScaling = 0.01f;
    float moveScaling = 10.0f;
    bool zAxisUp = false;

    switch (gAppOptions.scene) {
        case HAIR_AND_PARTICLES_SCENE:
        case GROUND_PLANE_SCENE: {
            gMesh.Create(d3dDevice, L"media\\GroundPlane\\GroundPlane.sdkmesh");
            sceneScaling = 5.0f;
            moveScaling = 100.0f;
            cameraEye = D3DXVECTOR3(-0.473993, 140.063812, 238.968491);
            cameraAt = D3DXVECTOR3(1.8163376, 19.331497, 10.104355);

            if (gAppOptions.scene == HAIR_AND_PARTICLES_SCENE) {
                gAppOptions.enableHair = true;
                gAppOptions.debugY = 65.0f;
                gAppOptions.debugX = 85.0f;
                gApp->UpdateHairMesh();
            }
            break;
        }
        case POWER_PLANT_SCENE: {
            gMesh.Create(d3dDevice, L"media\\powerplant\\powerplant.sdkmesh");
            sceneScaling = 1.0f;
            cameraEye = sceneScaling * D3DXVECTOR3(100.0f, 5.0f, 5.0f);
            cameraAt = sceneScaling * D3DXVECTOR3(0.0f, 0.0f, 0.0f);
            break;
        }
        case TEAPOT_SCENE: {
            gMesh.Create(d3dDevice, L"media\\teapot\\teapot.sdkmesh");
            sceneScaling = 1.0f;
            cameraEye = sceneScaling * D3DXVECTOR3(10.0f, 5.0f, 5.0f);
            cameraAt = sceneScaling * D3DXVECTOR3(0.0f, 0.0f, 0.0f);
            break;
        }
        case TEAPOTS_SCENE: {
            gMesh.Create(d3dDevice, L"media\\teapot\\teapots.sdkmesh");
            sceneScaling = 1.0f;
            cameraEye = sceneScaling * D3DXVECTOR3(-.57672393, .78871685, 19.330332);
            cameraAt = sceneScaling * D3DXVECTOR3(-.43721151, .58952415, 18.360353);
            break;
        }
        case DRAGON_SCENE: {
            gMesh.Create(d3dDevice, L"media\\dragon\\dragon.sdkmesh");
            sceneScaling = 1.0f;
            cameraEye = sceneScaling * D3DXVECTOR3(10.0f, 5.0f, 5.0f);
            cameraAt = sceneScaling * D3DXVECTOR3(0.0f, 0.0f, 0.0f);
            break;
        }
        case HAPPYS_SCENE: {
            gMesh.Create(d3dDevice, L"media\\happy\\happys.sdkmesh");
            sceneScaling = 1.0f;
            cameraEye = sceneScaling * D3DXVECTOR3(1.9277762f, 3.7334092f, -9.2676630f);
            cameraAt = sceneScaling * D3DXVECTOR3(1.9932435f, 3.6729131f, -8.2716436f);
            break;
        }
        default: {
            assert(false);
        }
    }

    
    D3DXMatrixScaling(&gWorldMatrix, sceneScaling, sceneScaling, sceneScaling);
    if (zAxisUp) {
        D3DXMATRIXA16 m;
        D3DXMatrixRotationX(&m, -D3DX_PI / 2.0f);
        gWorldMatrix *= m;
    }

    gMainOptions.gViewerCamera.SetViewParams(&cameraEye, &cameraAt);
    gMainOptions.gViewerCamera.SetScalers(rotationScaling, moveScaling);
    gMainOptions.gViewerCamera.FrameMove(0.0f);

    // Create a particle system for this specific scene
    CreateParticles(d3dDevice, gAppOptions.scene);

    if (gRestoreViewState) {
        gMainOptions.gViewerCamera.SetViewParams(&gNewViewEye, &gNewViewLookAt);
        gMainOptions.gLightCamera.SetViewParams(&gNewLightEye, &gNewLightLookAt);
        gRestoreViewState = false;
    }
}

void SaveState()
{
    std::ofstream stream;
    stream.open("avsm.settings2",  std::ios::out);
    if (stream.bad()) {
        return;
    }
    stream << std::setprecision(10);

    stream << "uiOptions {" << std::endl;
    gUIConstants.Serialize(stream);
    stream << "}" << std::endl;

    stream << "appOptions {" << std::endl;
    gAppOptions.Serialize(stream);
    stream << "}" << std::endl;

    stream << "mainOptions {" << std::endl;
    gMainOptions.Serialize(stream);
    stream << "}" << std::endl;

    stream.close();

    OutputDebugStringA("Wrote state in avsm.settings2");
}

void LoadState()
{
    SCENE_SELECTION oldScene = gAppOptions.scene;
    unsigned int oldShadowNodeCount = gAppOptions.ShadowNodeCount;

    std::ifstream stream;

    stream.open("avsm.settings2", std::ios::in);
    if (stream.bad()) {
        return;
    }
    std::string token;
    while (stream.good()) {
        stream >> token;
        if (token == "uiOptions") {
            gNewUIConstants.Deserialize(stream);
        } else if (token == "appOptions") {
            gNewAppOptions.Deserialize(stream);
        } else if (token == "mainOptions") {
            gMainOptions.Deserialize(stream);
        }
    }
    gUIConstants = gNewUIConstants;
    gAppOptions = gNewAppOptions;

    stream.close();

    gRestoreUIConstants = true;
    InitUI();

    if (gAppOptions.ShadowNodeCount != oldShadowNodeCount) {
        gApp->SetShadowNodeCount(gAppOptions.ShadowNodeCount);
        gRestoreAppOptions = true;
    }

    if (oldScene != gAppOptions.scene) {
        DestroyScene();
        gRestoreViewState = true;
    }

    srand(1234);

    gApp->UpdateHairMesh();

    OutputDebugStringA("Read state from avsm.settings");
}

void CALLBACK OnKeyboard(UINT character, bool keyDown, bool altDown, void* userContext)
{
    if(keyDown) {
        switch (character) {
        case 'O':
            if (gCameraMode == CAMERAMODE_RECORD) {
                gCameraMode = CAMERAMODE_DEFAULT;
                OutputDebugStringA("Stopped recording camera.\n");
            } else {
                gCameraMode = CAMERAMODE_RECORD;
                gCameraPathLength = 0;
                OutputDebugStringA("Started recording camera.\n");
            }
            break;
        case 'P':
            if (gCameraMode == CAMERAMODE_PLAYBACK) {
                gCameraMode = CAMERAMODE_DEFAULT;
                OutputDebugStringA("Stopped camera playback.\n");
            } else if (gCameraPathLength) {
                gCameraMode = CAMERAMODE_PLAYBACK;
                gCameraPlaybackTime = 0.f;
                gCameraPathCurrentIndex = 0;
                gMainOptions.gViewerCamera.SetViewParams(&gCameraPath[gCameraPathCurrentIndex].eye,
                                                         &gCameraPath[gCameraPathCurrentIndex].look);
                OutputDebugStringA("Started camera playback.\n");
            }
            break;
        case 'C':
            if (gCameraPathLength) {
                FILE* fp = 0;
                if (!fopen_s(&fp, "savedcamera.bin", "wb")) {
                    fwrite(gCameraPath, gCameraPathLength * sizeof(CameraKey), 1, fp);
                    fclose(fp);
                    OutputDebugStringA("Saved camera keys to savedcamera.bin.\n");
                }
            } else {
                FILE* fp = 0;
                if (!fopen_s(&fp, "savedcamera.bin", "rb")) {
                    fseek(fp, 0L, SEEK_END);
                    long size = ftell(fp);
                    fseek(fp, 0L, SEEK_SET);
                    gCameraMode = CAMERAMODE_DEFAULT;
                    gCameraPathLength = size / sizeof(CameraKey);
                    fread(gCameraPath, gCameraPathLength * sizeof(CameraKey), 1, fp);
                    fclose(fp);
                    OutputDebugStringA("Loaded camera keys from savedcamera.bin.\n");
                }
            }
            break;
        case VK_F6:
            gShowFPS = !gShowFPS;
            break;
        case VK_F7:
            // Toggle visibility of expert HUD
            gHUD[HUD_HAIR].SetVisible(!gHUD[HUD_HAIR].GetVisible());
	        break;
        case VK_F8:
            // Toggle display of UI on/off
            gDisplayUI = !gDisplayUI;
            break;
        }
    }
}

void CALLBACK OnGUIEvent(UINT eventID, INT controlID, CDXUTControl* control, void* userContext)
{
    CDXUTSlider* Slider;
    switch (controlID)
    {
        case UI_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen(); break;
        case UI_CONTROLMODE: {
                CDXUTComboBox* Combo = dynamic_cast<CDXUTComboBox*>(control);
                gMainOptions.gControlObject = PtrToInt(Combo->GetSelectedData());
                break;
        }
        case UI_CHANGEDEVICE:
            gD3DSettingsDlg.SetActive(!gD3DSettingsDlg.IsActive()); 
            break;
        case UI_VOLUMETRICMODEL: {
            CDXUTComboBox* Combo = dynamic_cast<CDXUTComboBox*>(control);
            INT vol = PtrToInt(Combo->GetSelectedData());
            gAppOptions.enableParticles = (vol & VOLUME_PARTICLES) != 0;
            gAppOptions.enableHair      = (vol & VOLUME_HAIR) != 0;
            gDXUTResetFPS = true;
            break;
        }
        case UI_AVSMAUTOBOUNDS:
            gAppOptions.enableAutoBoundsAVSM =
                dynamic_cast<CDXUTCheckBox*>(control)->GetChecked();
            break;
        case UI_DOUBLESIDEDPOLYGONS:
            gAppOptions.enableDoubleSidedPolygons = 
                dynamic_cast<CDXUTCheckBox*>(control)->GetChecked();
            break;
        case UI_ALPHATEST:
            gAppOptions.enableAlphaCutoff =
                dynamic_cast<CDXUTCheckBox*>(control)->GetChecked();
            break;
        case UI_FACENORMALS:
            gUIConstants.faceNormals = dynamic_cast<CDXUTCheckBox*>(control)->GetChecked(); 
            break;
        case UI_ENABLETRANSPARENCY:
            gAppOptions.enableTransparency = dynamic_cast<CDXUTCheckBox*>(control)->GetChecked(); 
            gDXUTResetFPS = true;
            break;
        case UI_ENABLESTATS:
            gAppOptions.enableStats = dynamic_cast<CDXUTCheckBox*>(control)->GetChecked(); 
            break;
        case UI_SELECTEDSCENE: {
            gAppOptions.scene = 
                (SCENE_SELECTION) PtrToUlong(
                    gSceneSelectCombo->GetSelectedData());
            DestroyScene();
        }
        case UI_TRANSPARENCYMETHOD: 
            gAppOptions.transparencyMethod = (TRANSPARENCY_METHOD) PtrToUlong(gTransparencyMethodCombo->GetSelectedData());
            gDXUTResetFPS = true;
            break;

        // These controls all imply changing parameters to the App constructor
        // (i.e. recreating resources and such), so we'll just clean up the app here and let it be
        // lazily recreated next render.
        case UI_AVSMSORTINGMETHOD:
            gUIConstants.avsmSortingMethod = static_cast<unsigned int>(PtrToUlong(gAVSMSortingMethodCombo->GetSelectedData())); 
            break;
        case UI_AOITNODECOUNT: {
            unsigned int newNodeCount =
                static_cast<unsigned int>(PtrToUlong(gAOITNodeCountCombo->GetSelectedData()));

            if (newNodeCount != gAppOptions.AOITNodeCount) {
                gAppOptions.AOITNodeCount = newNodeCount;
                gApp->SetAOITNodeCount(newNodeCount);
            }
            break;
        }
        case UI_PARTICLESIZE: {
            // Map the slider's [0,100] integer range to floating point [0, 5.0f]
            Slider = gHUD[HUD_GENERIC].GetSlider(UI_PARTICLESIZE);
            gUIConstants.particleSize = static_cast<float>(Slider->GetValue()) / kParticleSizeFactorUI;
            break;
        }
        case UI_PARTICLEOPACITY: {
            Slider = gHUD[HUD_GENERIC].GetSlider(UI_PARTICLEOPACITY);
            gUIConstants.particleOpacity = Slider->GetValue();
            break;
        }
        case UI_GEOMETRYALPHA: {
            Slider = gHUD[HUD_GENERIC].GetSlider(UI_GEOMETRYALPHA);
            gAppOptions.geometryAlpha = Slider->GetValue() / 100.0f;
            wchar_t text[256];
            swprintf_s(text, L"Geometry Alpha=%3.2f", gAppOptions.geometryAlpha);
            gHUD[HUD_GENERIC].GetStatic(UI_GEOMETRYALPHATEXT)->SetText(text);
            break;
        }
        case UI_HAIR_THICKNESS: {
            Slider = gHUD[HUD_HAIR].GetSlider(UI_HAIR_THICKNESS);
            gUIConstants.hairThickness = Slider->GetValue();
            break;
        }
        case UI_HAIR_SHADOW_THICKNESS: {
            Slider = gHUD[HUD_HAIR].GetSlider(UI_HAIR_SHADOW_THICKNESS);
            gUIConstants.hairShadowThickness = Slider->GetValue();
            break;
        }
        case UI_HAIR_OPACITY: {
            Slider = gHUD[HUD_HAIR].GetSlider(UI_HAIR_OPACITY);
            gUIConstants.hairOpacity = Slider->GetValue();
            break;
        }
        case UI_X: {
            Slider = gHUD[HUD_HAIR].GetSlider(UI_X);
            gAppOptions.debugX = static_cast<float>(Slider->GetValue());
            gApp->UpdateHairMesh();
            break;
        }
        case UI_Y: {
            Slider = gHUD[HUD_HAIR].GetSlider(UI_Y);
            gAppOptions.debugY = static_cast<float>(Slider->GetValue());
            gApp->UpdateHairMesh();
            break;
        }
        case UI_Z: {
            Slider = gHUD[HUD_HAIR].GetSlider(UI_Z);
            gAppOptions.debugZ = static_cast<float>(Slider->GetValue());
            gApp->UpdateHairMesh();
            break;
        }
        case UI_VOLUMESHADOWINGMETHOD:
            gUIConstants.volumeShadowMethod = (unsigned int)PtrToUlong(gVolumeShadowingMethodCombo->GetSelectedData()); 
            break;
        case UI_PAUSEPARTICLEANIMATION:
            gUIConstants.pauseParticleAnimaton = dynamic_cast<CDXUTCheckBox*>(control)->GetChecked(); 
            break;
        case UI_LIGHTINGONLY:
            gUIConstants.lightingOnly = dynamic_cast<CDXUTCheckBox*>(control)->GetChecked(); 
            break;
        case UI_VOLUMESHADOWCREATION:
            gAppOptions.enableVolumeShadowCreation = !gAppOptions.enableVolumeShadowCreation;
            break;
        case UI_VOLUMESHADOWLOOKUP:
            gUIConstants.enableVolumeShadowLookup = !gUIConstants.enableVolumeShadowLookup;
            break;
        case UI_SAVESTATE:
            SaveState();
            break;
        case UI_LOADSTATE:
            LoadState();
            break;
        case UI_VISUALIZEABUFFER:
            gAppOptions.visualizeABuffer = static_cast<CDXUTCheckBox*>(control)->GetChecked();
            break;
        case UI_VISUALIZEAOIT:
            gAppOptions.visualizeAOIT = static_cast<CDXUTCheckBox*>(control)->GetChecked();
            break;
        case UI_ENABLETILING:
            gAppOptions.enableTiling = static_cast<CDXUTCheckBox*>(control)->GetChecked();
            break;
        default:
            return;
    }
}


void CALLBACK OnD3D11DestroyDevice(void* userContext)
{
    DeinitApp();
    DestroyScene();
    
    gDialogResourceManager.OnD3D11DestroyDevice();
    gD3DSettingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE(gTextHelper);
    gFPSWriter.Release();
}


HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* d3dDevice, const DXGI_SURFACE_DESC* backBufferSurfaceDesc,
                                     void* userContext)
{
    ID3D11DeviceContext* d3dDeviceContext = DXUTGetD3D11DeviceContext();
    gDialogResourceManager.OnD3D11CreateDevice(d3dDevice, d3dDeviceContext);
    gD3DSettingsDlg.OnD3D11CreateDevice(d3dDevice);
    gTextHelper = new CDXUTTextHelper(d3dDevice, d3dDeviceContext, &gDialogResourceManager, 15);
    gFPSWriter.Create(d3dDevice, 256);
    
    D3DXVECTOR3 vecEye(120.0f, 80.0f, -5.0f);
    D3DXVECTOR3 vecAt(50.0f,0.0f,30.0f);

    gMainOptions.gViewerCamera.SetViewParams(&vecEye, &vecAt);
    gMainOptions.gViewerCamera.SetScalers(0.01f, 50.0f);
    gMainOptions.gViewerCamera.SetDrag(true);
    gMainOptions.gViewerCamera.SetEnableYAxisMovement(true);
    gMainOptions.gViewerCamera.FrameMove(0);

    vecEye = D3DXVECTOR3(-320.0f, 300.0f, -220.3);
    vecAt =  D3DXVECTOR3(0.0f, 0.0f, 0.0f);

    // TODO: Set near/far based on scene...?

    gMainOptions.gLightCamera.SetViewParams(&vecEye, &vecAt);
    gMainOptions.gLightCamera.SetScalers(0.01f, 50.0f);
    gMainOptions.gLightCamera.SetDrag(true);
    gMainOptions.gLightCamera.SetEnableYAxisMovement(true);
    gMainOptions.gLightCamera.SetProjParams(D3DX_PI / 4, 1.0f, 0.1f , 1000.0f);
    gMainOptions.gLightCamera.FrameMove(0);

    gAppOptions.pickedX = backBufferSurfaceDesc->Width / 2;
    gAppOptions.pickedY = backBufferSurfaceDesc->Height / 2;
    gFrameWidth  = backBufferSurfaceDesc->Width;
    gFrameHeight = backBufferSurfaceDesc->Height;

    return S_OK;
}


void UpdateViewerCameraNearFar()
{
    // TODO: Set near/far based on scene...?
    gMainOptions.gViewerCamera.SetProjParams(D3DX_PI / 4, gAspectRatio, 0.05f, 500.0f);
}


HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* d3dDevice, IDXGISwapChain* swapChain,
                                          const DXGI_SURFACE_DESC* backBufferSurfaceDesc, void* userContext)
{
    HRESULT hr;

    V_RETURN(gDialogResourceManager.OnD3D11ResizedSwapChain(d3dDevice, backBufferSurfaceDesc));
    V_RETURN(gD3DSettingsDlg.OnD3D11ResizedSwapChain(d3dDevice, backBufferSurfaceDesc));

    gAspectRatio = backBufferSurfaceDesc->Width / (FLOAT)backBufferSurfaceDesc->Height;

    gAppOptions.pickedX = gAppOptions.pickedX * backBufferSurfaceDesc->Width  / gFrameWidth;
    gAppOptions.pickedY = gAppOptions.pickedY * backBufferSurfaceDesc->Height / gFrameHeight;
    gFrameWidth = backBufferSurfaceDesc->Width;
    gFrameHeight = backBufferSurfaceDesc->Height;

    UpdateViewerCameraNearFar();
    
    // Standard HUDs
    const int border = 20;
    int y = border;
    for (int i = 0; i < HUD_HAIR; ++i) {
        gHUD[i].SetLocation(backBufferSurfaceDesc->Width - gHUD[i].GetWidth() - border, y);
        y += gHUD[i].GetHeight() + border;
    }

    y = backBufferSurfaceDesc->Height - border;

    // Expert HUD
    y -= gHUD[HUD_EXPERT].GetHeight();
    gHUD[HUD_EXPERT].SetLocation(border, y);

    // Hair HUD
    y -= gHUD[HUD_HAIR].GetHeight();
    gHUD[HUD_HAIR].SetLocation(border, y);

    // If there's no app, it'll pick this up when it gets lazily created so just ignore it
    if (gApp) {
        gApp->OnD3D11ResizedSwapChain(d3dDevice, backBufferSurfaceDesc);
    }

    return S_OK;
}


void CALLBACK OnD3D11ReleasingSwapChain(void* userContext)
{
    gDialogResourceManager.OnD3D11ReleasingSwapChain();
}


void CALLBACK OnD3D11FrameRender(ID3D11Device* d3dDevice, ID3D11DeviceContext* d3dDeviceContext, double time,
                                 FLOAT elapsedTime, void* userContext)
{
    if (gD3DSettingsDlg.IsActive()) {
        gD3DSettingsDlg.OnRender(elapsedTime);
        return;
    }

    // Lazily create the application if need be
    if (!gApp) {
        InitApp(d3dDevice, d3dDeviceContext);
    }

    // Lazily load scene
    if (!gMesh.IsLoaded()) {
        InitScene(d3dDevice);
        gDXUTResetFPS = true;
    }

    ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
    
    D3D11_VIEWPORT viewport;
    viewport.Width    = static_cast<float>(DXUTGetDXGIBackBufferSurfaceDesc()->Width);
    viewport.Height   = static_cast<float>(DXUTGetDXGIBackBufferSurfaceDesc()->Height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;

    FrameStats frameStats;
    gApp->Render(gAppOptions, 
                 d3dDeviceContext, pRTV, &gMesh, &gMeshAlpha, gParticleSystem, 
                 gWorldMatrix, GetCurrentUserCamera(), &gMainOptions.gLightCamera, &viewport, 
                 &gUIConstants, &frameStats);

    d3dDeviceContext->RSSetViewports(1, &viewport);
    d3dDeviceContext->OMSetRenderTargets(1, &pRTV, 0);

    if (gDisplayUI) {
        // Render HUDs in reverse order
        for (int i = HUD_NUM - 1; i >= 0; --i) {
            gHUD[i].OnRender(elapsedTime);
        }
    }

    int numLines = 0;
    gTextHelper->Begin();

    gTextHelper->SetInsertionPos(2, 0 );
    gTextHelper->SetForegroundColor(D3DXCOLOR(0.0f, 0.0f, 1.0f, 1.0f));
    if (gDisplayUI) {
        gTextHelper->DrawTextLine(L"F6:toggle FPS. F7:hair HUD. F8:toggle all HUDs. ");
        ++numLines;
        gTextHelper->DrawFormattedTextLine(L"(%d,%d)", gAppOptions.pickedX, gAppOptions.pickedY);
        numLines += 2;
    }

    if (gAppOptions.enableStats && (0.0f != frameStats.fragmentCount)) {
        gTextHelper->DrawFormattedTextLine(L"");
        gTextHelper->DrawFormattedTextLine(L"");
        gTextHelper->DrawFormattedTextLine(L"");
        gTextHelper->DrawFormattedTextLine(L"");
        gTextHelper->DrawFormattedTextLine(L"Fragment Capture Pass: %.2f ms", frameStats.capturePassTime);
        gTextHelper->DrawFormattedTextLine(L"Resolve Pass: %.2f ms", frameStats.resolvePassTime);
        gTextHelper->DrawFormattedTextLine(L"Compositing Rate: %.2f MFrag/s", frameStats.fragmentPerSecond / 1E6F);
        gTextHelper->DrawFormattedTextLine(L"Transparent Fragment Count: %.2f MFrag", frameStats.fragmentCount / 1E6F);
        gTextHelper->DrawFormattedTextLine(L"Max Depth Complexity: %i", frameStats.maxFragmentPerPixelCount);
        gTextHelper->DrawFormattedTextLine(L"Average Depth Complexity: %.2f", frameStats.avgFragmentPerPixel);
        gTextHelper->DrawFormattedTextLine(L"Std Deviation: %.2f", frameStats.stdDevFragmentPerPixel);
        //gTextHelper->DrawFormattedTextLine(L"Average Depth Complexity (NEP): %.2f", frameStats.avgFragmentPerNonEmptyPixel);
        //gTextHelper->DrawFormattedTextLine(L"Std Deviation (NEP): %.2f", frameStats.stdDevFragmentNonEmptyPerPixel);
        gTextHelper->DrawFormattedTextLine(L"Transp. Fragment Screen Coverage: %.2f%%", 100.0f * frameStats.transparentFragmentScreenCoverage);
        gTextHelper->DrawFormattedTextLine(L"Min/Max Depth: %.2f/%.2f", frameStats.minDepth, frameStats.maxDepth);
    }

    gTextHelper->End();

    // Output frame time
    if (gShowFPS) gFPSWriter.Draw(DXUTGetD3D11DeviceContext(), 2, numLines * 15 + 5, gFrameWidth, gFrameHeight, 1.f / DXUTGetFPS());
}

CFirstPersonAndOrbitCamera * GetCurrentUserCamera()
{
    switch (gMainOptions.gControlObject) {
        case CONTROL_CAMERA: return &gMainOptions.gViewerCamera;
        case CONTROL_LIGHT:  return &gMainOptions.gLightCamera;
        default:             throw std::runtime_error("Unknown user control object!");
    }
}
