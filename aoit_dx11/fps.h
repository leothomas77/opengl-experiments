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

#ifndef FPS_H
#define FPS_H 

#include <d3d11.h>
#include <vector>

class FPSWriter
{
private:
    struct Vertex {
        unsigned int viewPos[2];
        float texCoord[2];
    };

    ID3D11InputLayout*        mInputLayout;
    ID3D11Buffer*             mIndexBuffer;
    ID3D11Buffer*             mVertexBuffer;
    std::vector<Vertex>       mVertices;
    ID3D11VertexShader*       mVertexShader;
    ID3D11PixelShader*        mPixelShader;
    ID3D11BlendState*         mBlendState;

    ID3D11ShaderResourceView* mFontSRV;
    ID3D11SamplerState*       mFontSampler;
    ID3D11Buffer*             mFontCB;
    unsigned int              mFontTextureWidth;
    unsigned int              mFontTextureHeight;

    void AddCharacter(char ch, size_t& vbCharIndex,
                      unsigned int& viewportPxX, unsigned int viewportPxY);

    void AddFloat(char const* format, float f, size_t& vbCharIndex,
                  unsigned int& viewportPxX, unsigned int viewportPxY);

public:
    FPSWriter();
    ~FPSWriter();
    bool Create(ID3D11Device* d3dDevice, size_t maxChars);
    void Release();
    void Draw(ID3D11DeviceContext* d3dDeviceContext,
              unsigned int viewportPxX, unsigned int viewportPxY,
              unsigned int viewportPxWidth, unsigned int viewportPxHeight,
              float frameSeconds);
};

#endif // ifndef FPS_H

