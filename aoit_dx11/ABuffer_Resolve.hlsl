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

#ifndef H_ABUFFER_RESOLVE
#define H_ABUFFER_RESOLVE

#include "Common.hlsl"
#include "FragmentList.hlsl"

#define MAX_ABUFFER_NODES 300

#ifdef MSAA
float4 ABufferSortedResolvePS(FullScreenTriangleVSOut input, in uint sampleIdx : SV_SampleIndex) : SV_Target
#else
float4 ABufferSortedResolvePS(FullScreenTriangleVSOut input) : SV_Target
#endif
{
    int2 screenAddress = int2(input.positionViewport.xy);   

    // Get offset to the first node
    uint nodeOffset = FL_GetFirstNodeOffset(screenAddress);   
    
    uint nodeCount = 0;
    FragmentListNode nodes[MAX_ABUFFER_NODES];            

    // Fetch and sort nodes
    [loop] while ((nodeOffset != FL_NODE_LIST_NULL) && (nodeCount < MAX_ABUFFER_NODES))
    {
        // Get node..
        FragmentListNode node = FL_GetNode(nodeOffset);

        float depth;
        uint  coverageMask;
        FL_UnpackDepthAndCoverage(node.depth, depth, coverageMask);
#ifdef MSAA
        if (coverageMask & (1 << sampleIdx)) {
#endif
            // Insertion Sort
            int i = (int) nodeCount;
			
            while (i > 0) {
                if (nodes[i-1].depth < depth) {
                    nodes[i] = nodes[i-1];
                    i--;                            
                } else break;
            }

            nodes[i] = node;

            // Increase node count
            nodeCount++;   
#ifdef MSAA             
        }
#endif
        // Move to next node
        nodeOffset = node.next;                    
    }

    if (nodeCount == 0) {
        discard;
    }

    float4 blendColor = float4(0, 0, 0, 1);
    for (uint i = 0; i < nodeCount; ++i) {
        float4 nodeColor = FL_UnpackColor(nodes[i].color);
        blendColor.xyz = nodeColor.xyz * nodeColor.www  + (1.0f.xxx - nodeColor.www) * blendColor.xyz;
        blendColor.w   = blendColor.w * (1.0f - nodeColor.w);
    }

    return blendColor;
}


#ifdef MSAA
float4 ABufferResolvePS(FullScreenTriangleVSOut input, in uint sampleIdx : SV_SampleIndex) : SV_Target
#else
float4 ABufferResolvePS(FullScreenTriangleVSOut input) : SV_Target
#endif
{
    float3 blendColor = 0;
    float  totalTransmittance = 1;
    int2   screenAddress = int2(input.positionViewport.xy);   
    uint   firstNodeOffset = FL_GetFirstNodeOffset(screenAddress);   

    // Get offset to the first node
    uint outerNodeOffset = firstNodeOffset;
    
    // Fetch and sort nodes
    [loop] while (outerNodeOffset != FL_NODE_LIST_NULL)
    {
        // Get node..
        FragmentListNode outerNode = FL_GetNode(outerNodeOffset);

        float outerDepth;
        uint  outerCoverageMask;
        FL_UnpackDepthAndCoverage(outerNode.depth, outerDepth, outerCoverageMask);

#ifdef MSAA
        if (outerCoverageMask & (1 << sampleIdx)) {
#endif
            float visibility = 1;
            
            uint innerNodeOffset = firstNodeOffset;
            [loop] while (innerNodeOffset != FL_NODE_LIST_NULL) {
                float innerDepth;
                uint  innerCoverageMask;

                FragmentListNode innerNode = FL_GetNode(innerNodeOffset);
                FL_UnpackDepthAndCoverage(innerNode.depth, innerDepth, innerCoverageMask);
#ifdef MSAA
                if (innerCoverageMask & (1 << sampleIdx)) {
#endif
                    float4 innerNodeColor = FL_UnpackColor(innerNode.color);
                    visibility *= outerDepth <= innerDepth ? 1 : 1 - innerNodeColor.w;
#ifdef MSAA             
                 }
#endif
                innerNodeOffset = innerNode.next;  
            }
            // Composite this fragment
            float4 nodeColor = FL_UnpackColor(outerNode.color);
            blendColor += nodeColor.xyz * nodeColor.www * visibility.xxx;

            // Update total transmittance
            totalTransmittance *= 1 - nodeColor.w;
#ifdef MSAA             
        }
#endif
        // Move to next node
        outerNodeOffset = outerNode.next;                    
    }

    return float4(blendColor, totalTransmittance);
}


#endif // H_ABUFFER_RESOLVE
