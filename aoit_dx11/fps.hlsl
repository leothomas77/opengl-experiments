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

struct FpsVSOut {
    float4 position : SV_Position;
    float2 texCoord : texCoord;
};

cbuffer FpsCB : register(b0) {
    float3 gFontColor;
    uint2 gPxOffset;
    uint2 gViewportPxDim;
};

FpsVSOut FpsVS(uint2 viewPos : viewPos, float2 texCoord : texCoord) {
    float2 position = viewPos + gPxOffset;
    position = position / gViewportPxDim;
    position = 2 * position - 1;
    position.y = -position.y;

    FpsVSOut Out;
    Out.position = float4(position, 0, 1);
    Out.texCoord = texCoord;
    return Out;
}

Texture2D<float4> gFont : register(t0);
SamplerState gFontSampler : register(s0);

float4 FpsPS(FpsVSOut In) : SV_Target {
    float4 c = gFont.Sample(gFontSampler, In.texCoord);
    c.rgb = gFontColor;
#ifdef DEBUG_FPS_FONT
    c.rgb *= c.a;
    c.a = 1;
#endif
    return c;
}
      
