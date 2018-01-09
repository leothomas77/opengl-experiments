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

#include "fps.h"

#include <d3dx11.h>
#include <d3dcompiler.h>

//#define DEBUG_FPS_FONT

template<typename T>
void SafeRelease(T*& t) {
    if (t) {
        t->Release();
        t = 0;
    }
}

__declspec(align(16))
struct FontCB {
    float Color[3];
    float padding;
    unsigned int PxOffset[2];
    unsigned int ViewportPxWidth;
    unsigned int ViewportPxHeight;
};

FPSWriter::FPSWriter()
    : mInputLayout(0)
    , mIndexBuffer(0)
    , mVertexBuffer(0)
    , mVertices()
    , mVertexShader(0)
    , mPixelShader(0)
    , mBlendState(0)
    , mFontSRV(0)
    , mFontSampler(0)
    , mFontCB(0)
    , mFontTextureWidth(0)
    , mFontTextureHeight(0)
{
}

FPSWriter::~FPSWriter() {
    Release();
}

bool FPSWriter::Create(ID3D11Device* d3dDevice, size_t maxChars) {
    Release();

    mVertices.resize(6 * maxChars);
    {
        CD3D11_BUFFER_DESC desc((UINT) (mVertices.size() * sizeof(Vertex)),
                                D3D11_BIND_VERTEX_BUFFER,
                                D3D11_USAGE_DYNAMIC,
                                D3D11_CPU_ACCESS_WRITE);
        d3dDevice->CreateBuffer(&desc, 0, &mVertexBuffer);
    }
    {
        // Vertices: 0 1
        //           2 3
        std::vector<unsigned int> indices(6 * maxChars);
        for (unsigned int i = 0; i < maxChars; ++i) {
            indices[6 * i + 0] = 4 * i + 0;
            indices[6 * i + 1] = 4 * i + 3;
            indices[6 * i + 2] = 4 * i + 2;
            indices[6 * i + 3] = 4 * i + 0;
            indices[6 * i + 4] = 4 * i + 1;
            indices[6 * i + 5] = 4 * i + 3;
        }

        CD3D11_BUFFER_DESC desc((UINT) indices.size() * sizeof(unsigned int), D3D11_BIND_INDEX_BUFFER);
        D3D11_SUBRESOURCE_DATA data = { &indices[0], 0, 0 };
        d3dDevice->CreateBuffer(&desc, &data, &mIndexBuffer);
    }

    {
        ID3D10Blob* blob = 0;
        if (FAILED(D3DX11CompileFromFile(L"fps.hlsl", 0, 0, "FpsVS", "vs_5_0", 0, 0, 0, &blob, 0, 0))) {
            return false;
        }
        
        d3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), 0, &mVertexShader);

        const D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            {"viewPos",  0, DXGI_FORMAT_R32G32_UINT,  0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"texCoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        
        d3dDevice->CreateInputLayout(layout, ARRAYSIZE(layout), blob->GetBufferPointer(), blob->GetBufferSize(), &mInputLayout);

        blob->Release();      
    }

    {
        D3D10_SHADER_MACRO defines[] = {
#ifdef DEBUG_FPS_FONT
            {"DEBUG_FPS_FONT", "1"},
#endif
            {0, 0}
        };

        ID3D10Blob* blob = 0;
        if (FAILED(D3DX11CompileFromFile(L"fps.hlsl", defines, 0, "FpsPS", "ps_5_0", 0, 0, 0, &blob, 0, 0))) {
            return false;
        }
        
        d3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), 0, &mPixelShader);

        blob->Release();      
    }

    {
        ID3D11Texture2D* fontTexture = 0;
        D3DX11CreateTextureFromFile(d3dDevice, L"media\\fps_font.png", 0, 0, (ID3D11Resource**) &fontTexture, 0);

        D3D11_TEXTURE2D_DESC tdesc;
        fontTexture->GetDesc(&tdesc);
        mFontTextureWidth = tdesc.Width;
        mFontTextureHeight = tdesc.Height;

        CD3D11_SHADER_RESOURCE_VIEW_DESC srvdesc(fontTexture, D3D11_SRV_DIMENSION_TEXTURE2D, tdesc.Format, 0, tdesc.MipLevels);
        d3dDevice->CreateShaderResourceView(fontTexture, &srvdesc, &mFontSRV);

        fontTexture->Release();
    }
    {
        CD3D11_SAMPLER_DESC desc(D3D11_DEFAULT);
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        d3dDevice->CreateSamplerState(&desc, &mFontSampler);
    }

    {
        CD3D11_BLEND_DESC desc(D3D11_DEFAULT);
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend    = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend   = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp     = D3D11_BLEND_OP_ADD;
        d3dDevice->CreateBlendState(&desc, &mBlendState);
    }

    {
        CD3D11_BUFFER_DESC desc(sizeof(FontCB), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
        d3dDevice->CreateBuffer(&desc, 0, &mFontCB);
    }

    return true;
}

void FPSWriter::Release() {
    SafeRelease(mInputLayout);
    SafeRelease(mIndexBuffer);
    SafeRelease(mVertexBuffer);
    mVertices.clear();
    SafeRelease(mVertexShader);
    SafeRelease(mPixelShader);
    SafeRelease(mFontSRV);
    SafeRelease(mFontSampler);
    SafeRelease(mBlendState);
    SafeRelease(mFontCB);
}

void FPSWriter::AddCharacter(char ch, size_t& vbCharIndex,
                             unsigned int& viewportPxX, unsigned int viewportPxY) {

    if (4 * vbCharIndex >= mVertices.size()) return;

                                        // 0   1   2   3   4   5   6    7    8    9    .    m    f    
    unsigned int const CharPxIndex[14] = { 0, 16, 30, 46, 62, 78, 93, 109, 125, 141, 155, 162, 292, 349 };
    unsigned int const CharPxHeight = 26;

    unsigned int i = 0;
         if (ch >= '0' && ch <= '9') { i = ch - '0'; }
    else if (ch == '.')              { i = 10; }
    else if (ch == 'm')              { i = 11; }
    else if (ch == 'f')              { i = 12; }

    unsigned int const CharPxWidth = CharPxIndex[i+1] - CharPxIndex[i];

    unsigned int nextViewportPxX = viewportPxX + CharPxWidth;
    unsigned int nextViewportPxY = viewportPxY + CharPxHeight;

    float u0 = (float) CharPxIndex[i] / mFontTextureWidth;
    float u1 = u0 + (float) CharPxWidth / mFontTextureWidth;
    float v0 = 0.f;
    float v1 = (float) CharPxHeight / mFontTextureHeight;

    // Vertices: 0 1
    //           2 3
    mVertices[4 * vbCharIndex + 0].viewPos[0] = viewportPxX;
    mVertices[4 * vbCharIndex + 0].viewPos[1] = viewportPxY;
    mVertices[4 * vbCharIndex + 0].texCoord[0] = u0;
    mVertices[4 * vbCharIndex + 0].texCoord[1] = v0;

    mVertices[4 * vbCharIndex + 1].viewPos[0] = nextViewportPxX;
    mVertices[4 * vbCharIndex + 1].viewPos[1] = viewportPxY;
    mVertices[4 * vbCharIndex + 1].texCoord[0] = u1;
    mVertices[4 * vbCharIndex + 1].texCoord[1] = v0;

    mVertices[4 * vbCharIndex + 2].viewPos[0] = viewportPxX;
    mVertices[4 * vbCharIndex + 2].viewPos[1] = nextViewportPxY;
    mVertices[4 * vbCharIndex + 2].texCoord[0] = u0;
    mVertices[4 * vbCharIndex + 2].texCoord[1] = v1;

    mVertices[4 * vbCharIndex + 3].viewPos[0] = nextViewportPxX;
    mVertices[4 * vbCharIndex + 3].viewPos[1] = nextViewportPxY;
    mVertices[4 * vbCharIndex + 3].texCoord[0] = u1;
    mVertices[4 * vbCharIndex + 3].texCoord[1] = v1;

    viewportPxX = nextViewportPxX;

#ifdef DEBUG_FPS_FONT
    viewportPxX += 2.f / viewportPxWidth;
#endif

    ++vbCharIndex;
}

void FPSWriter::AddFloat(char const* format, float f, size_t& vbCharIndex,
                         unsigned int& viewportPxX, unsigned int viewportPxY) {
    std::vector<char> buffer((mVertices.size() / 6) - vbCharIndex);
    size_t numChars = sprintf_s(&buffer[0], buffer.size(), format, f);
    for (size_t i = 0; i < numChars; ++i) {
        AddCharacter(buffer[i], vbCharIndex, viewportPxX, viewportPxY);
    }
}

void FPSWriter::Draw(ID3D11DeviceContext* d3dDeviceContext,
                     unsigned int viewportPxX, unsigned int viewportPxY,
                     unsigned int viewportPxWidth, unsigned int viewportPxHeight,
                     float frameSeconds) {
    size_t vbCharIndex = 0;
#ifdef DEBUG_FPS_FONT
    AddCharacter('0', vbCharIndex, viewportPxX, viewportPxY);
    AddCharacter('1', vbCharIndex, viewportPxX, viewportPxY);
    AddCharacter('2', vbCharIndex, viewportPxX, viewportPxY);
    AddCharacter('3', vbCharIndex, viewportPxX, viewportPxY);
    AddCharacter('4', vbCharIndex, viewportPxX, viewportPxY);
    AddCharacter('5', vbCharIndex, viewportPxX, viewportPxY);
    AddCharacter('6', vbCharIndex, viewportPxX, viewportPxY);
    AddCharacter('7', vbCharIndex, viewportPxX, viewportPxY);
    AddCharacter('8', vbCharIndex, viewportPxX, viewportPxY);
    AddCharacter('9', vbCharIndex, viewportPxX, viewportPxY);
    AddCharacter('.', vbCharIndex, viewportPxX, viewportPxY);
    AddCharacter('m', vbCharIndex, viewportPxX, viewportPxY);
    AddCharacter('f', vbCharIndex, viewportPxX, viewportPxY);
#else
    AddFloat("%.1f", 1000.f * frameSeconds, vbCharIndex, viewportPxX, viewportPxY);
    AddCharacter('m', vbCharIndex, viewportPxX, viewportPxY);
    AddFloat("%.1f", 1.f / frameSeconds, vbCharIndex, viewportPxX, viewportPxY);
    AddCharacter('f', vbCharIndex, viewportPxX, viewportPxY);
#endif

    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        d3dDeviceContext->Map(mVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        CopyMemory(mapped.pData, (void*) &mVertices[0], vbCharIndex * 4 * sizeof(Vertex));
        d3dDeviceContext->Unmap(mVertexBuffer, 0);
    }

    UINT Stride = sizeof(Vertex);
    UINT Offset = 0;
    d3dDeviceContext->IASetInputLayout(mInputLayout);
    d3dDeviceContext->IASetIndexBuffer(mIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    d3dDeviceContext->IASetVertexBuffers(0, 1, &mVertexBuffer, &Stride, &Offset);
    d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    d3dDeviceContext->VSSetShader(mVertexShader, 0, 0);

    d3dDeviceContext->PSSetShaderResources(0, 1, &mFontSRV);
    d3dDeviceContext->PSSetSamplers(0, 1, &mFontSampler);
    d3dDeviceContext->PSSetShader(mPixelShader, 0, 0);

    d3dDeviceContext->OMSetBlendState(mBlendState, 0, 0xffffffffu);

    FontCB const cb[] = {
        { 0.f, 0.f, 0.f, 1.f, 2, 2, viewportPxWidth, viewportPxHeight }, // shadow
        { 1.f, 1.f, 1.f, 1.f, 0, 0, viewportPxWidth, viewportPxHeight }, // text
    };

    for (size_t i = 0; i < sizeof(cb) / sizeof(FontCB); ++i) {
        {
            D3D11_MAPPED_SUBRESOURCE mappedResource;
            d3dDeviceContext->Map(mFontCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
            memcpy(mappedResource.pData, &cb[i], sizeof(cb[i]));
            d3dDeviceContext->Unmap(mFontCB, 0);
        }
        d3dDeviceContext->VSSetConstantBuffers(0, 1, &mFontCB);
        d3dDeviceContext->PSSetConstantBuffers(0, 1, &mFontCB);

        d3dDeviceContext->DrawIndexed((UINT) (vbCharIndex * 6), 0, 0);
    }

    ID3D11ShaderResourceView* nullSRV = 0;
    d3dDeviceContext->PSSetShaderResources(0, 1, &nullSRV);
    ID3D11SamplerState* nullSampler = 0;
    d3dDeviceContext->PSSetSamplers(0, 1, &nullSampler);
    ID3D11Buffer* nullCB = 0;
    d3dDeviceContext->VSSetConstantBuffers(0, 1, &nullCB);
    d3dDeviceContext->PSSetConstantBuffers(0, 1, &nullCB);
}

