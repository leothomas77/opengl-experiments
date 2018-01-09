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

#ifndef H_AIOT_RESOLVE
#define H_AIOT_RESOLVE

#include "AOIT.hlsl"
#include "FragmentList.hlsl"


#ifdef MSAA
float4 AOITResolvePS(FullScreenTriangleVSOut input, in uint sampleIdx : SV_SampleIndex) : SV_Target
#else
float4 AOITResolvePS(FullScreenTriangleVSOut input) : SV_Target
#endif
{
    uint i;
    uint nodeOffset;  
    int2 screenAddress = int2(input.positionViewport.xy);  

    // Get offset to the first node
    uint firstNodeOffset = FL_GetFirstNodeOffset(screenAddress); 

    AOITData data;
    // Initialize AVSM data    
    [unroll]for (i = 0; i < AOIT_RT_COUNT; ++i) {
        data.depth[i] = AIOT_EMPTY_NODE_DEPTH.xxxx;
        data.trans[i] = AOIT_FIRT_NODE_TRANS.xxxx;
    }
    
    // Fetch all nodes and add them to our visibiity function
    nodeOffset = firstNodeOffset;
    [loop] while (nodeOffset != FL_NODE_LIST_NULL) 
    {
        // Get node..
        FragmentListNode node = FL_GetNode(nodeOffset);

        float depth;
        uint  coverageMask;
        FL_UnpackDepthAndCoverage(node.depth, depth, coverageMask);
#ifdef MSAA
        if (coverageMask & (1 << sampleIdx)) {
#endif      
            // Unpack color
            float4 nodeColor = FL_UnpackColor(node.color);        
            AOITInsertFragment(depth,  saturate(1.0 - nodeColor.w), data);      
#ifdef MSAA
        }
#endif
        // Move to next node
        nodeOffset = node.next;                    
    }

    float3 color = float3(0, 0, 0);
    // Fetch all nodes again and composite them
    nodeOffset = firstNodeOffset;
    [loop]  while (nodeOffset != FL_NODE_LIST_NULL) {
        // Get node..
        FragmentListNode node = FL_GetNode(nodeOffset);

        float depth;
        uint  coverageMask;
        FL_UnpackDepthAndCoverage(node.depth, depth, coverageMask);
#ifdef MSAA
        if (coverageMask & (1 << sampleIdx)) {
#endif              
            // Unpack color
            float4 nodeColor = FL_UnpackColor(node.color);
            AOITFragment frag = AOITFindFragment(data, depth);
            float vis = frag.index == 0 ? 1.0f : frag.transA;
            color += nodeColor.xyz * nodeColor.www * vis.xxx;
#ifdef MSAA           
        }
#endif
        // Move to next node
        nodeOffset = node.next;                    
    }

    float4 blendColor = float4(color, data.trans[AOIT_RT_COUNT - 1][3]);
    return blendColor;
}

#endif // H_AIOT_RESOLVE
