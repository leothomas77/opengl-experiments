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

#ifndef H_FRAGMENT_LIST
#define H_FRAGMENT_LIST

//////////////////////////////////////////////
// Defines
//////////////////////////////////////////////

#define FL_NODE_LIST_NULL (0x0U)

//////////////////////////////////////////////
// Structs
//////////////////////////////////////////////

struct FragmentListNode
{
    uint    next;  
    float   depth;    
    uint    color;
};

//////////////////////////////////////////////
// Constant Buffers
//////////////////////////////////////////////

cbuffer FL_Constants
{
    uint  mMaxListNodes;
};

//////////////////////////////////////////////
// Resources
//////////////////////////////////////////////

RWTexture2D<uint>                          gFragmentListDepthCullUAV;

RWTexture2D<uint>                          gFragmentListFirstNodeAddressUAV;
Texture2D<uint>                            gFragmentListFirstNodeAddressSRV;

RWStructuredBuffer<FragmentListNode>       gFragmentListNodesUAV;
StructuredBuffer<FragmentListNode>         gFragmentListNodesSRV;


//////////////////////////////////////////////
// Helper Functions
//////////////////////////////////////////////

int2 FL_GetDimensions()
{
	int2 dim;
	gFragmentListFirstNodeAddressSRV.GetDimensions(dim.x, dim.y);

	return dim;
}

// Max 4X AA
float FL_PackDepthAndCoverage(in float depth, in uint coverage)
{
    return asfloat((asuint(depth) & 0xFFFFFFF0UL) | coverage);
}

void FL_UnpackDepthAndCoverage(in float packedDepthCovg, out float depth, out uint coverage)
{
    uint uiPackedDepthCovg = asuint(packedDepthCovg);
    depth    = asfloat(uiPackedDepthCovg & 0xFFFFFFF0UL);
    coverage = uiPackedDepthCovg & 0xFUL;
}

uint FL_PackColor(float4 fColor)
{
    uint color;
    color = uint(fColor[0] * 255.0f) & 0xFFUL;
    color |= (uint(fColor[1] * 255.0f) & 0xFFUL) << 8;
    color |= (uint(fColor[2] * 255.0f) & 0xFFUL) << 16;
    color |= (uint(fColor[3] * 255.0f) & 0xFFUL) << 24;

    return color;
}

float4 FL_UnpackColor(uint uiColor)
{    
    float4 color;
    const float norm = 1.0f / 255.0f;
    color[0] = (uiColor & 0xFFUL) * norm;
    color[1] = ((uiColor >> 8)  & 0xFFUL) * norm;
    color[2] = ((uiColor >> 16) & 0xFFUL) * norm;
    color[3] = ((uiColor >> 24) & 0xFFUL) * norm;
    
    return color;    
}

uint FL_GetFirstNodeOffset(int2 screenAddress)
{
    return gFragmentListFirstNodeAddressSRV[screenAddress];
}

bool FL_AllocNode(out uint newNodeAddress1D)
{
    // alloc a new node
    newNodeAddress1D = gFragmentListNodesUAV.IncrementCounter();

    // running out of memory?
    return newNodeAddress1D <= mMaxListNodes;    
}

// Insert a new node at the head of the list
void FL_InsertNode(in int2 screenAddress, in uint newNodeAddress, in FragmentListNode newNode)
{
    uint oldNodeAddress;
    InterlockedExchange(gFragmentListFirstNodeAddressUAV[screenAddress], newNodeAddress, oldNodeAddress); 
      
    newNode.next = oldNodeAddress;    
    gFragmentListNodesUAV[newNodeAddress] =  newNode;
}

FragmentListNode FL_GetNode(uint nodeAddress)
{
    return gFragmentListNodesSRV[nodeAddress];
}     

#endif // H_FRAGMENT_LIST