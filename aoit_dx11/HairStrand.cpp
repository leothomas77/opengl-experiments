#if 0
/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or 
     nondisclosure agreement with Intel Corporation and may not be copied 
     or disclosed except in accordance with the terms of that agreement. 
          Copyright (C) 2009 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTsettingsdlg.h"
#include "SDKmesh.h"

#include "HairStrand.h"

#if 0
extern ID3D10Effect* g_pEffect;
#endif

CHairStrand::CHairStrand()
{
    m_tGuideMap = NULL;
    m_tBlendMask = NULL;
    m_vbStrandLineBuffer = NULL;
    m_tCollisionMap = NULL;
    m_numGuideStrandVertices = 0;
	m_numGuideStrands = 0;
	m_sqrtHairStrength = 0;
	m_segments = 50;
    m_numStrands = 0;
    m_collisionMapRes = 1024;

#if 0
    m_techRenderHead = NULL;
    m_techRenderHair = NULL;
#endif

    m_drawBody = true;
    m_drawHair = true;
    m_drawGuideStrand = false;
	m_hairDynamicMode = 0;  // static mode

#if 0
    m_ptxBlendMask = NULL;
    m_ptxGuide = NULL;
    m_ptxCollision = NULL;
    m_pfCollisionDist = NULL;
    m_pmCollisionBone = NULL;
    m_pmInvCollisionBone = NULL;
    m_pDynamicMode = NULL;
#endif

#if 0
    m_pfGrow = NULL;
    m_pvForce = NULL;
    m_pvAmbientColor;
    m_pvDiffuseColor;
#endif

    m_pVertexLayout = NULL;
    m_pStrandLayout = NULL;
}

CHairStrand::~CHairStrand()
{
    Destroy();
}

void CHairStrand::Destroy(void)
{
    SAFE_RELEASE(m_tBlendMask);
    SAFE_RELEASE(m_tGuideMap);
    SAFE_RELEASE(m_tCollisionMap);

	SAFE_RELEASE(m_vbStrandLineBuffer);
    SAFE_RELEASE(m_pStrandLayout);
    SAFE_RELEASE(m_pVertexLayout);

    m_headMesh.Destroy();
}

void CHairStrand::LoadScene(ID3D11Device* pd3dDevice)
{
	HRESULT hr;
    // load textures
    V( D3DX11CreateShaderResourceViewFromFile(pd3dDevice, m_fBlendMask, NULL, NULL, &m_tBlendMask, NULL) );
    
    // load maps
    D3DX11_IMAGE_INFO infoImage;
    D3DX11_IMAGE_LOAD_INFO infoLoad;
    ZeroMemory( &infoLoad, sizeof(D3DX11_IMAGE_LOAD_INFO) );
    infoLoad.Width = D3DX11_DEFAULT;
    infoLoad.Height = D3DX11_DEFAULT;
    infoLoad.Depth = D3DX11_DEFAULT;
    infoLoad.MipLevels = 1;
    infoLoad.Usage = D3D11_USAGE_DEFAULT;
    infoLoad.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    infoLoad.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    infoLoad.Filter = D3DX10_FILTER_NONE;
    infoLoad.MipFilter = D3DX10_FILTER_NONE;
    infoLoad.pSrcInfo = &infoImage;
    V( D3DX10GetImageInfoFromFile(m_fGuideMap, NULL, &infoImage, NULL) );
	m_numGuideStrandVertices = infoImage.Width;    
	m_numGuideStrands = infoImage.Height;    
    V( D3DX10CreateShaderResourceViewFromFile(pd3dDevice, m_fGuideMap, &infoLoad, NULL, &m_tGuideMap, NULL) );
    V( D3DX10GetImageInfoFromFile(m_fCollisionMap, NULL, &infoImage, NULL) );
    m_collisionMapRes = infoImage.Width;
    V( D3DX10CreateShaderResourceViewFromFile(pd3dDevice, m_fCollisionMap, &infoLoad, NULL, &m_tCollisionMap, NULL) );

    // set shader resources
    V( m_ptxBlendMask->SetResource( m_tBlendMask ) );
    V( m_ptxGuide->SetResource( m_tGuideMap ) );
    V( m_ptxCollision->SetResource( m_tCollisionMap ) );

    // apply parms for the collision test
    D3DXMATRIX mCollisionBone;
    D3DXMATRIX mTran, mRot;
    D3DXVECTOR3 vRot = D3DXVECTOR3(1.0f, 0.0f, 0.0f);

    D3DXMatrixRotationAxis(&mRot, &vRot, m_collisionSet[0]);
    D3DXMatrixTranslation(&mTran, m_collisionSet[1], m_collisionSet[2], m_collisionSet[3]);
    V( m_pfCollisionDist->SetFloat( m_collisionSet[4]) );

    D3DXMatrixMultiply(&mCollisionBone, &mRot, &mTran);
    V( m_pmCollisionBone->SetMatrix((float*)&mCollisionBone) );

    D3DXMatrixInverse(&mCollisionBone, NULL, &mCollisionBone);
    V( m_pmInvCollisionBone->SetMatrix((float*)&mCollisionBone) );

	SAFE_RELEASE(m_vbStrandLineBuffer);

    // load the Strand List
    STRAND_FOLLICLE* pVrtxBuf = new STRAND_FOLLICLE[m_numStrands];
    FILE* strandFile = NULL;
    _wfopen_s(&strandFile, m_fStrandList, L"rb");
    fread(pVrtxBuf, sizeof(STRAND_FOLLICLE), m_numStrands, strandFile);
    fclose(strandFile);
	
	D3D11_BUFFER_DESC vbdesc =
	{
		m_numStrands*sizeof(STRAND_FOLLICLE),
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_VERTEX_BUFFER,
		0,
		0
	};
	D3D11_SUBRESOURCE_DATA vbInitData;
	ZeroMemory( &vbInitData, sizeof(D3D11_SUBRESOURCE_DATA) );

	vbInitData.pSysMem = pVrtxBuf;
	vbInitData.SysMemPitch = 0;

	V( pd3dDevice->CreateBuffer( &vbdesc, &vbInitData, &m_vbStrandLineBuffer ) );

    delete[] pVrtxBuf;

}

void CHairStrand::Create(ID3D11Device* pd3dDevice)
{
    HRESULT hr;
    WCHAR str[MAX_PATH];

    // Obtain variables
#if 0
    m_ptxBlendMask = g_pEffect->GetVariableByName( "g_blendMaskMap" )->AsShaderResource();
    m_ptxGuide = g_pEffect->GetVariableByName( "g_GuideMap" )->AsShaderResource();
    m_ptxCollision = g_pEffect->GetVariableByName( "g_CollisionMap" )->AsShaderResource();
    m_pfCollisionDist = g_pEffect->GetVariableByName( "g_collisionDist" )->AsScalar();
    m_pmCollisionBone = g_pEffect->GetVariableByName( "g_mCollisionBone" )->AsMatrix();
    m_pmInvCollisionBone = g_pEffect->GetVariableByName( "g_mInverseCollisionBone" )->AsMatrix();
    m_pfGrow = g_pEffect->GetVariableByName( "g_grow" )->AsScalar();
    m_pvForce = g_pEffect->GetVariableByName( "g_force" )->AsVector();
    m_pDynamicMode = g_pEffect->GetVariableByName( "g_dynamicMode" )->AsScalar();
    m_pvAmbientColor = g_pEffect->GetVariableByName( "g_ambientColor" )->AsVector();
    m_pvDiffuseColor = g_pEffect->GetVariableByName( "g_diffuseColor" )->AsVector();
    m_techRenderHead = g_pEffect->GetTechniqueByName( "RenderScene" );
    m_techRenderHair = g_pEffect->GetTechniqueByName( "RenderHair" );
#endif

    LoadScene(pd3dDevice);

    // Initialize the head mesh
    const D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    D3D11_PASS_DESC PassDesc;
    V( m_techRenderHead->GetPassByIndex( 0 )->GetDesc( &PassDesc ) );
    V( pd3dDevice->CreateInputLayout( layout, 3, PassDesc.pIAInputSignature,
                                             PassDesc.IAInputSignatureSize, &m_pVertexLayout ) );
    V( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, m_fHeadModel ) );
    V( m_headMesh.Create( pd3dDevice, str ) );

	// create RV for base route
    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    ZeroMemory( &SRVDesc, sizeof(SRVDesc) );
    SRVDesc.Format			= DXGI_FORMAT_R32G32B32A32_FLOAT;
	SRVDesc.ViewDimension	= D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.MipLevels = 1;

    V( m_pfGrow->SetFloat(1.f/m_segments) );
#if 0
    V( g_pEffect->GetVariableByName( "g_collisionPixelSize" )->AsScalar()->SetFloat(1.f/m_collisionMapRes) );
#endif

    const D3D11_INPUT_ELEMENT_DESC strand_layout[] = 
    {
        {"POSITION",	0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"PROPERTY",	0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    V( m_techRenderHair->GetPassByIndex( 0 )->GetDesc( &PassDesc ) );
    V( pd3dDevice->CreateInputLayout( strand_layout, 2, PassDesc.pIAInputSignature,
                                             PassDesc.IAInputSignatureSize, &m_pStrandLayout ) );

    m_pDynamicMode->SetInt(0);
}

void CHairStrand::OnResizedSwapChain( ID3D11Device* pd3dDevice )
{
    Create(pd3dDevice);
}


void CHairStrand::Render(ID3D11Device* pd3dDevice, float fElapsedTime )
{
    D3D11_TECHNIQUE_DESC techDesc;
	ID3D11Buffer* pVB[1];
	UINT strides[1];
	UINT offsets[1] = {0};

    // render head
    if (m_drawBody)
    {
        // apply buffers
	    pVB[0] = m_headMesh.GetVB10( 0,0 );
	    strides[0] = ( UINT )m_headMesh.GetVertexStride( 0,0 );
	    pd3dDevice->IASetVertexBuffers( 0, 1, pVB, strides, offsets );
	    pd3dDevice->IASetIndexBuffer( m_headMesh.GetIB10( 0 ), m_headMesh.GetIBFormat10( 0 ),0 );
	    // IA setup
        pd3dDevice->IASetInputLayout( m_pVertexLayout );
    	
	    m_techRenderHead->GetDesc( &techDesc );
	    SDKMESH_SUBSET* pSubset = NULL;
	    for( UINT p = 0; p<techDesc.Passes; ++p ) {
		    for( UINT subset = 0; subset < m_headMesh.GetNumSubsets( 0 ); ++subset ) {
			    // get subset
			    pSubset = m_headMesh.GetSubset( 0, subset );
			    pd3dDevice->IASetPrimitiveTopology( CDXUTSDKMesh::GetPrimitiveType10( ( SDKMESH_PRIMITIVE_TYPE ) pSubset->PrimitiveType ) );
                // set material
                m_pvDiffuseColor->SetFloatVector(&m_headMesh.GetMaterial( pSubset->MaterialID )->Diffuse.x);
                m_pvAmbientColor->SetFloatVector(&m_headMesh.GetMaterial( pSubset->MaterialID )->Ambient.x);
		        m_techRenderHead->GetPassByIndex( p )->Apply( 0 );
			    pd3dDevice->DrawIndexed( (UINT)pSubset->IndexCount, (UINT)pSubset->IndexStart, (UINT)pSubset->VertexStart );
		    }
	    }
    }

    // render hair strands
    if (m_drawHair)
    {
        ID3D11ShaderResourceView *const pSRV[3] = {NULL, NULL, NULL};
        ID3D11Buffer* pVB[1];
        UINT strides[1];
        UINT offsets[1] = {0};

        // apply the vertex buffer for a segment rendering
        int numHairs = m_drawGuideStrand ? m_numGuideStrands : m_numStrands;
        pd3dDevice->IASetInputLayout( m_pStrandLayout );
        pVB[0] = m_vbStrandLineBuffer;
        strides[0] = sizeof(STRAND_FOLLICLE);
        offsets[0] = 0;
        pd3dDevice->IASetVertexBuffers( 0, 1, pVB, strides, offsets );

        // render hair strand
        pd3dDevice->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );
        m_techRenderHair->GetPassByIndex(0)->Apply( 0 );
        pd3dDevice->Draw( numHairs, 0 );

        // clear D3D11 render resources
        pd3dDevice->PSSetShaderResources(0, 3, pSRV);
        pd3dDevice->VSSetShaderResources(0, 3, pSRV);
    }
}

void CHairStrand::ApplyForce( float* force )
{
    HRESULT hr;
    V( m_pvForce->SetFloatVector(force) );
}
#endif
