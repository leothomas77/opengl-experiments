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

#include "HairCommon.hlsl"
#include "FragmentList.hlsl"

[earlydepthstencil]
void CameraHairCapturePS(HairPixelInput input, uint coverageMask : SV_Coverage)
{
    const float hairOpacity = ComputeHairOpacity(input.distanceFromCenter);

	float4 hairColor = LightHair(input.position, input.tangent, float4(input.color, hairOpacity));

    // Get fragment viewport coordinates
    int2 screenAddress = int2(input.hposition.xy);

#ifdef LIST_EARLY_DEPTH_CULL
    uint packedCullData = gFragmentListDepthCullUAV[screenAddress];
    float cullDepth = asfloat(packedCullData & 0xFFFFFF00UL);
    uint  cullTrans = packedCullData & 0xFFUL;
      	
    if ((cullTrans > EARLY_DEPTH_CULL_ALPHA) || (cullDepth > input.position.z)) 
    {	
#endif
        uint newNodeAddress;
        if (FL_AllocNode(newNodeAddress)) {
            // Fill node
            FragmentListNode node;            

            node.color    = FL_PackColor(saturate(hairColor));
            node.depth    = FL_PackDepthAndCoverage(input.position.z, coverageMask); input.position.z;                 
            
            // Insert node!
            FL_InsertNode(screenAddress, newNodeAddress, node);   

#ifdef LIST_EARLY_DEPTH_CULL
            // Update culling data
            float trans = (1.0f - hairColor.w) * float(cullTrans);
            packedCullData = asuint(max(input.position.z, cullDepth)) & 0xFFFFFF00UL;
            packedCullData = packedCullData | (uint(ceil(trans)) & 0xFFUL);
            gFragmentListDepthCullUAV[screenAddress] = packedCullData;
        }  
#endif                  
    }  
}

float4
CameraHairCapturePSDebug(HairPixelInput input) : SV_Target
{
    return float4(0, 0, 1, 1);
}
