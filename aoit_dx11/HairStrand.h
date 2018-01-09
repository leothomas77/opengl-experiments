#if 0
/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or 
     nondisclosure agreement with Intel Corporation and may not be copied 
     or disclosed except in accordance with the terms of that agreement. 
          Copyright (C) 2009 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

#pragma once

#include "dxut.h"
#include "DXUTCamera.h"
#include "SDKmisc.h"

class CHairStrand
{
    
public:
    struct STRAND_FOLLICLE
    {
        D3DXVECTOR4 position;
        D3DXVECTOR4 props;
        // x :: color index
        // y :: initial distance from the guide strand
        // z :: the corresponding guide strand (1D texcoord)
        // w :: strand length
    };
    
public:
    CHairStrand();
    ~CHairStrand();

public:
    // configuration
    WCHAR m_fHeadModel[MAX_PATH];
    WCHAR m_fBlendMask[MAX_PATH];
	WCHAR m_fGuideMap[MAX_PATH];
	WCHAR m_fStrandList[MAX_PATH];
	WCHAR m_fCollisionMap[MAX_PATH];
    float m_collisionSet[5];

    // Direct3D11 resources
#if 0
    ID3D11EffectShaderResourceVariable* m_ptxBlendMask;
    ID3D11EffectShaderResourceVariable* m_ptxGuide;
    ID3D10EffectShaderResourceVariable* m_ptxCollision;

    ID3D11EffectTechnique* m_techRenderHead;
    ID3D11EffectTechnique* m_techRenderHair;
#endif

    ID3D11ShaderResourceView* m_tBlendMask;
	ID3D11ShaderResourceView* m_tGuideMap;
	ID3D11ShaderResourceView* m_tCollisionMap;

#if 0
    ID3D11EffectScalarVariable* m_pfCollisionDist;
    ID3D11EffectMatrixVariable* m_pmCollisionBone;
    ID3D11EffectMatrixVariable* m_pmInvCollisionBone;

    ID3D11EffectScalarVariable* m_pfGrow;
    ID3D11EffectVectorVariable* m_pvForce;
    ID3D11EffectVectorVariable* m_pvAmbientColor;
    ID3D11EffectVectorVariable* m_pvDiffuseColor;

    ID3D11EffectScalarVariable* m_pDynamicMode;
#endif

    ID3D11InputLayout*  m_pVertexLayout;
    ID3D11InputLayout*  m_pStrandLayout;

    ID3D11Buffer* m_vbStrandLineBuffer;

    CDXUTSDKMesh m_headMesh; // head's geometry

    int m_sqrtHairStrength;
    int m_numStrands;
    int m_segments;
    int m_numGuideStrandVertices;
    int m_numGuideStrands;
    int m_collisionMapRes;

    bool m_drawBody;
    bool m_drawHair;
    bool m_drawGuideStrand;
	int m_hairDynamicMode;

public:
    void LoadScene(ID3D11Device* pd3dDevice);
    void Create(ID3D11Device* pd3dDevice);
    void Destroy( void );
    void OnResizedSwapChain( ID3D11Device* pd3dDevice );
    void Render( ID3D11Device* pd3dDevice, float fElapsedTime );
    void ApplyForce( float* force );
};
#endif
