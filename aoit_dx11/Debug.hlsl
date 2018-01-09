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

#ifndef DEBUG_SHADER_CODE
#define DEBUG_SHADER_CODE
#endif

#include "Debug.h"
#include "Common.hlsl"
#include "Stats.hlsl"
#include "FragmentList.hlsl"
#include "AOIT.hlsl"

////////////////////////////////////
// Defines
////////////////////////////////////


////////////////////////////////////
// Resources
////////////////////////////////////

globallycoherent RWStructuredBuffer<DebugNodeData>           gDebugDrawNodeDataUAV;
globallycoherent RWBuffer<uint>                              gDebugDrawIndirectArgsUAV;

StructuredBuffer<DebugNodeData>             gDebugNodeDataSRV;
Texture2DMS<float>                          gSTDepthMap0;
Texture2DMS<float>                          gSTDepthMap1;
Texture2DMS<float>                          gSTDepthMap2;
Texture2DMS<float>                          gSTDepthMap3;
Texture2DMS<float>                          gTotalAlpha;

////////////////////////////////////
// Structs
////////////////////////////////////

struct DebugDataVSOut
{
    float4 position : SV_Position;
    float4 color    : COLOR;
};

////////////////////////////////////
// Debug code
////////////////////////////////////

[numthreads(1, 1, 1)]
void GatherDebugDataCS(uint3 groupID   : SV_GroupID,
                        uint3 threadID  : SV_GroupThreadID,
	        			uint  threadIDFlattened : SV_GroupIndex)
{
    uint i, j;

    // Clear draw indirect arguments
    uint lineCount = 0;

    int2 screenAddress = int2(pixelCoordX, pixelCoordY);
    // Get offset to the first node
    uint nodeOffset = FL_GetFirstNodeOffset(screenAddress);   

    // Init min/max depth
    float minDepth = 1E30;
    float maxDepth = -minDepth;

    // Compute min/max depth
    uint nodeCount = 0;
    [loop] while ((nodeOffset != FL_NODE_LIST_NULL) && (nodeCount < DEBUG_MAX_NODE_COUNT - 1))
    {
        // Get node..
        FragmentListNode node = FL_GetNode(nodeOffset);

        // Update min/max depth
        minDepth = min(minDepth, node.depth);
        maxDepth = max(maxDepth, node.depth);

        // Move to next node
        nodeCount++; 
        nodeOffset = node.next;    
    }

    // Store min/max in the stats structure
    gFragmentStatsUAV[0].localMinDepth = asuint(minDepth);
    gFragmentStatsUAV[0].localMaxDepth = asuint(maxDepth);

    if (nodeCount > 0) {
        if (0 != enableABuffer) {
            nodeCount = 0;
            // Insert first node (default values)
            DebugNodeData nodes[DEBUG_MAX_NODE_COUNT];  
            nodes[nodeCount].depth = 0.0f;
            nodes[nodeCount].trans = 1.0f;
            nodes[nodeCount].color = lineColor[DEBUG_DRAW_ABUFFER_COLOR_IDX];
            nodeCount++;

            // Get offset to the first node
            uint nodeOffset = FL_GetFirstNodeOffset(screenAddress);
            // Fetch and sort nodes
            [loop] while ((nodeOffset != FL_NODE_LIST_NULL) && (nodeCount < DEBUG_MAX_NODE_COUNT - 1))
            {
                // Get node..
                FragmentListNode node = FL_GetNode(nodeOffset);

                // Insertion Sort
                int i = (int) nodeCount;			
                while (i > 0) {
                    if (nodes[i-1].depth >= node.depth) {
                        nodes[i] = nodes[i-1];
                        i--;                            
                    } else break;
                }

                // Store current node in a temporary array
                nodes[i].depth = node.depth;
                nodes[i].trans = 1.0f - FL_UnpackColor(node.color).w;
                nodes[i].color = lineColor[DEBUG_DRAW_ABUFFER_COLOR_IDX];

                // Move to next node
                nodeCount++; 
                nodeOffset = node.next;                    
            }     
    
            // Composite transmittance
            for (i = 1; i < nodeCount; i++) {
                nodes[i].trans *= nodes[i-1].trans;
            }

            // Add a last node to beautify the plot (only if we have space for it..)
            if (nodeCount < DEBUG_MAX_NODE_COUNT - 1) {
                nodes[nodeCount].depth = 1E30;
                nodes[nodeCount].trans = nodes[nodeCount - 1].trans;
                nodes[nodeCount].color = lineColor[DEBUG_DRAW_ABUFFER_COLOR_IDX];
                nodeCount++;
            }

            // Add vertical steps
            for (i = 1; i < nodeCount - 1; i++) {
                uint index = 2 * lineCount;
                DebugNodeData vertex0 = {nodes[i].depth, nodes[i-1].trans,  nodes[i].color};
                DebugNodeData vertex1 = {nodes[i].depth, nodes[i].trans,    nodes[i].color};
                gDebugDrawNodeDataUAV[index]     = vertex0;
                gDebugDrawNodeDataUAV[1 + index] = vertex1;
                lineCount++;
            }
    
            // Add horizontal steps
            for (i = 0; i < nodeCount - 1; i++) {
                uint index = 2 * lineCount;
                DebugNodeData vertex0 = {nodes[i].depth,   nodes[i].trans,  nodes[i].color};
                DebugNodeData vertex1 = {nodes[i+1].depth, nodes[i].trans,  nodes[i].color};
                gDebugDrawNodeDataUAV[index]     = vertex0;
                gDebugDrawNodeDataUAV[1 + index] = vertex1;
                lineCount++;
            }
        }
    }

    // Prepare arguments for DrawIndirect()    
    gDebugDrawIndirectArgsUAV[0] = 2;                
    gDebugDrawIndirectArgsUAV[1] = lineCount;  // number of line instances
    gDebugDrawIndirectArgsUAV[2] = 0;               
    gDebugDrawIndirectArgsUAV[3] = 0;        
    
    DeviceMemoryBarrierWithGroupSync();
}

[numthreads(1, 1, 1)]
void GatherDebugAOITDataCS(uint3 groupID   : SV_GroupID,
                           uint3 threadID  : SV_GroupThreadID,
	        			   uint  threadIDFlattened : SV_GroupIndex)
{
    DeviceMemoryBarrierWithGroupSync();
    
    if (enableAOIT) {
        uint i, j;
        uint lineCount = gDebugDrawIndirectArgsUAV[1];

        int2 screenAddress = int2(pixelCoordX, pixelCoordY);
        // Get offset to the first node
        uint nodeOffset = FL_GetFirstNodeOffset(screenAddress);   

        AOITData data;
        // Initialize AVSM data    
        [unroll]for (i = 0; i < AOIT_RT_COUNT; ++i) {
            data.depth[i] = AIOT_EMPTY_NODE_DEPTH.xxxx;
            data.trans[i] = AOIT_FIRT_NODE_TRANS.xxxx;
        }

        // Fetch all nodes and add them to our visibiity function
        nodeOffset = FL_GetFirstNodeOffset(screenAddress);
        [loop] while (nodeOffset != FL_NODE_LIST_NULL) 
        {
            // Get node..
            FragmentListNode node = FL_GetNode(nodeOffset);
            // Unpack color
            float4 nodeColor = FL_UnpackColor(node.color);

            float depth[2] = {node.depth, node.depth + 0.001};
            AOITInsertFragment(node.depth,  saturate(1.0 - nodeColor.w), data);      
        
            // Move to next node
            nodeOffset = node.next;                    
        }

        // Unpack AVSM data
        float depth[AOIT_NODE_COUNT];	
        float trans[AOIT_NODE_COUNT];	                
        [unroll] for (i = 0; i < AOIT_RT_COUNT; ++i) {
	        [unroll] for (j = 0; j < 4; ++j) {
		        depth[4 * i + j] = data.depth[i][j];
		        trans[4 * i + j] = data.trans[i][j];			        
	        }
        }	

        // Add vertical steps (except the first one)
        [loop]for (i = 1; i < AOIT_NODE_COUNT; i++) {
            uint index = 2 * lineCount;
            DebugNodeData vertex0 = {depth[i], trans[i-1], lineColor[DEBUG_DRAW_AOIT_COLOR_IDX]};
            DebugNodeData vertex1 = {depth[i], trans[i],   lineColor[DEBUG_DRAW_AOIT_COLOR_IDX]};
            gDebugDrawNodeDataUAV[index]     = vertex0;
            gDebugDrawNodeDataUAV[1 + index] = vertex1;
            lineCount++;
        }

        // Add horizontal steps (except the first and the last one)
        [loop]for (i = 0; i < AOIT_NODE_COUNT - 1; i++) {
            uint index = 2 * lineCount;
            DebugNodeData vertex0 = {depth[i],   trans[i], lineColor[DEBUG_DRAW_AOIT_COLOR_IDX]};
            DebugNodeData vertex1 = {depth[i+1], trans[i], lineColor[DEBUG_DRAW_AOIT_COLOR_IDX]};
            gDebugDrawNodeDataUAV[index]     = vertex0;
            gDebugDrawNodeDataUAV[1 + index] = vertex1;
            lineCount++;
        }

        // Add first step (hor + vert lines)
        {
            uint index = 2 * lineCount;
            DebugNodeData vertex0 = {0.0f,     1.0f,     lineColor[DEBUG_DRAW_AOIT_COLOR_IDX]};
            DebugNodeData vertex1 = {depth[0], 1.0f,     lineColor[DEBUG_DRAW_AOIT_COLOR_IDX]};
            DebugNodeData vertex2 = {depth[0], trans[0], lineColor[DEBUG_DRAW_AOIT_COLOR_IDX]};
            gDebugDrawNodeDataUAV[index]     = vertex0;
            gDebugDrawNodeDataUAV[1 + index] = vertex1;
            gDebugDrawNodeDataUAV[2 + index] = vertex1;
            gDebugDrawNodeDataUAV[3 + index] = vertex2;
            lineCount += 2;
        }

        // Add last horizontal line
        {
            uint index = 2 * lineCount;
            DebugNodeData vertex0 = {depth[AOIT_NODE_COUNT - 1],  trans[AOIT_NODE_COUNT - 1], lineColor[DEBUG_DRAW_AOIT_COLOR_IDX]};
            DebugNodeData vertex1 = {1E30,                        trans[AOIT_NODE_COUNT - 1], lineColor[DEBUG_DRAW_AOIT_COLOR_IDX]};
            gDebugDrawNodeDataUAV[index]     = vertex0;
            gDebugDrawNodeDataUAV[1 + index] = vertex1;
            lineCount++;
        }

        // Prepare arguments for DrawIndirect()
        gDebugDrawIndirectArgsUAV[0] = 2;                
        gDebugDrawIndirectArgsUAV[1] = lineCount;  // number of line instances
        gDebugDrawIndirectArgsUAV[2] = 0;               
        gDebugDrawIndirectArgsUAV[3] = 0;      
    }
}

DebugDataVSOut DebugDataVS(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    DebugNodeData node;
    DebugDataVSOut output;

    // Read fragment min/max depth values and compute scale and offset parameters..
    // ..to nicely fit data in the current viewport
    const float depthOffset = asfloat(gFragmentStatsSRV[0].localMinDepth);
          float depthScale  = asfloat(gFragmentStatsSRV[0].localMaxDepth) - asfloat(gFragmentStatsSRV[0].localMinDepth);
    if (depthScale == 0.0) depthScale = 1.0;

    node = gDebugNodeDataSRV[2 * instanceID + (vertexID & 0x1U)];

    // Fit node in the viewport
    float shrinkFactor  = 0.9;
    output.position.x   = shrinkFactor * (2 * ((node.depth - depthOffset) / depthScale) - 1);
    output.position.y   = shrinkFactor * (2 * node.trans - 1);
    output.position.zw  = float2(0.5, 1);
    output.color        = node.color;
    return output;
}

float4 DebugDataPS(DebugDataVSOut input) : SV_Target0
{
    return input.color;
}

Texture2DMS<float4> gFinalRender;
float4 FullScreenTrianglePS(float4 position  : SV_Position,
                            uint sampleIndex : SV_SampleIndex) : SV_Target0
{
    return gFinalRender.Load(position.xy, sampleIndex);
}

float4 MagnificationPIPPS(float4 position  : SV_Position) : SV_Target0
{
    uint2 pos = uint2(position.xy - viewportTopLeft);
    if (pos.x == 0 || pos.x == (uint) viewportDim.x-1 || pos.y == 0 || pos.y == (uint) viewportDim.y-1) return 1.0.xxxx;

    uint dummy;
    uint numSamples;
    gFinalRender.GetDimensions(dummy, dummy, numSamples);

    int MagnificationPixelWidth = 24;

    float2 coord = float2(pos - uint2(1,1)) / (viewportDim - uint2(2,2));
    coord *= MagnificationPixelWidth;

    pos = (pickedTarget - uint(MagnificationPixelWidth/2).xx) + uint2(coord);

    float4 c = 0.0.xxxx;
    for (uint i = 0; i < numSamples; ++i) {
        c += gFinalRender.Load(pos, i);
    }
    return c / numSamples;
}

