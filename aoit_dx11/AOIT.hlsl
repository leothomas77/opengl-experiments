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

#ifndef H_AOIT
#define H_AOIT
#include "Common.hlsl"

//////////////////////////////////////////////
// Defines
//////////////////////////////////////////////

#ifndef AOIT_NODE_COUNT 
#define AOIT_NODE_COUNT			(16)
#endif

#define AOIT_FIRT_NODE_TRANS	(1)
#define AOIT_RT_COUNT			(AOIT_NODE_COUNT / 4)
#define AIOT_EMPTY_NODE_DEPTH	(1E30)

// Forces compression to only work on the second half of the nodes (cheaper and better IQ in most cases)
#define AOIT_DONT_COMPRESS_FIRST_HALF 

//////////////////////////////////////////////
// Structs
//////////////////////////////////////////////

struct AOITData 
{
    float4 depth[AOIT_RT_COUNT];
    float4 trans[AOIT_RT_COUNT];
};

struct AOITFragment
{
    int   index;
    float depthA;
    float transA;
};

//////////////////////////////////////////////////
// Two-level search for AT visibility functions
//////////////////////////////////////////////////
 
AOITFragment AOITFindFragment(in AOITData data, in float fragmentDepth)
{
    int    index;
    float4 depth, trans;
    float  leftDepth;
    float  leftTrans;
    
    AOITFragment Output;      

#if AOIT_RT_COUNT > 7    
    [flatten]if (fragmentDepth > data.depth[6][3])
    {
        depth        = data.depth[7];
        trans        = data.trans[7];
        leftDepth    = data.depth[6][3];
        leftTrans    = data.trans[6][3];
        Output.index = 28;        
    }
    else
#endif  
#if AOIT_RT_COUNT > 6    
    [flatten]if (fragmentDepth > data.depth[5][3])
    {
        depth        = data.depth[6];
        trans        = data.trans[6];
        leftDepth    = data.depth[5][3];
        leftTrans    = data.trans[5][3];
        Output.index = 24;        
    }
    else
#endif  
#if AOIT_RT_COUNT > 5    
    [flatten]if (fragmentDepth > data.depth[4][3])
    {
        depth        = data.depth[5];
        trans        = data.trans[5];
        leftDepth    = data.depth[4][3];
        leftTrans    = data.trans[4][3];
        Output.index = 20;        
    }
    else
#endif  
#if AOIT_RT_COUNT > 4    
    [flatten]if (fragmentDepth > data.depth[3][3])
    {
        depth        = data.depth[4];
        trans        = data.trans[4];
        leftDepth    = data.depth[3][3];
        leftTrans    = data.trans[3][3];    
        Output.index = 16;        
    }
    else
#endif    
#if AOIT_RT_COUNT > 3    
    [flatten]if (fragmentDepth > data.depth[2][3])
    {
        depth        = data.depth[3];
        trans        = data.trans[3];
        leftDepth    = data.depth[2][3];
        leftTrans    = data.trans[2][3];    
        Output.index = 12;        
    }
    else
#endif    
#if AOIT_RT_COUNT > 2    
    [flatten]if (fragmentDepth > data.depth[1][3])
    {
        depth        = data.depth[2];
        trans        = data.trans[2];
        leftDepth    = data.depth[1][3];
        leftTrans    = data.trans[1][3];          
        Output.index = 8;        
    }
    else
#endif    
#if AOIT_RT_COUNT > 1    
    [flatten]if (fragmentDepth > data.depth[0][3])
    {
        depth        = data.depth[1];
        trans        = data.trans[1];
        leftDepth    = data.depth[0][3];
        leftTrans    = data.trans[0][3];       
        Output.index = 4;        
    }
    else
#endif
    {    
        depth        = data.depth[0];
        trans        = data.trans[0];
        leftDepth    = data.depth[0][0];
        leftTrans    = data.trans[0][0];      
        Output.index = 0;        
    } 
      
    [flatten]if (fragmentDepth <= depth[0]) {
        Output.depthA = leftDepth;
        Output.transA = leftTrans;
    } else if (fragmentDepth <= depth[1]) {
        Output.index += 1;
        Output.depthA = depth[0]; 
        Output.transA = trans[0];            
    } else if (fragmentDepth <= depth[2]) {
        Output.index += 2;
        Output.depthA = depth[1];
        Output.transA = trans[1];            
    } else if (fragmentDepth <= depth[3]) {
        Output.index += 3;    
        Output.depthA = depth[2];
        Output.transA = trans[2];            
    } else {
        Output.index += 4;       
        Output.depthA = depth[3];
        Output.transA = trans[3];         
    }
    
    return Output;
}	

////////////////////////////////////////////////////
// Insert a new fragment in the visibility function
////////////////////////////////////////////////////

void AOITInsertFragment(in float fragmentDepth,
                        in float fragmentTrans,
                        inout AOITData AOITData)
{	
    int i, j;
  
    // Unpack AOIT data    
    float depth[AOIT_NODE_COUNT + 1];	
    float trans[AOIT_NODE_COUNT + 1];	                
    [unroll] for (i = 0; i < AOIT_RT_COUNT; ++i) {
	    [unroll] for (j = 0; j < 4; ++j) {
		    depth[4 * i + j] = AOITData.depth[i][j];
		    trans[4 * i + j] = AOITData.trans[i][j];			        
	    }
    }	

    // Find insertion index 
    AOITFragment tempFragment = AOITFindFragment(AOITData, fragmentDepth);
    const int   index = tempFragment.index;
    // If we are inserting in the first node then use 1.0 as previous transmittance value
    // (we don't store it, but it's implicitly set to 1. This allows us to store one more node)
    const float prevTrans = index != 0 ? tempFragment.transA : 1.0f;

    // Make space for the new fragment. Also composite new fragment with the current curve 
    // (except for the node that represents the new fragment)
    [unroll]for (i = AOIT_NODE_COUNT - 1; i >= 0; --i) {
        [flatten]if (index <= i) {
            depth[i + 1] = depth[i];
            trans[i + 1] = trans[i] * fragmentTrans;
        }
    }
    
    // Insert new fragment
    [unroll]for (i = 0; i <= AOIT_NODE_COUNT; ++i) {
        [flatten]if (index == i) {
            depth[i] = fragmentDepth;
            trans[i] = fragmentTrans * prevTrans;
        }
    } 
    
    // pack representation if we have too many nodes
    [flatten]if (depth[AOIT_NODE_COUNT] != AIOT_EMPTY_NODE_DEPTH) {	                
        
        // That's total number of nodes that can be possibly removed
        const int removalCandidateCount = (AOIT_NODE_COUNT + 1) - 1;

#ifdef AOIT_DONT_COMPRESS_FIRST_HALF
        // Although to bias our compression scheme in order to favor..
        // .. the closest nodes to the eye we skip the first 50%
		const int startRemovalIdx = removalCandidateCount / 2;
#else
		const int startRemovalIdx = 1;
#endif

        float nodeUnderError[removalCandidateCount];
        [unroll]for (i = startRemovalIdx; i < removalCandidateCount; ++i) {
            nodeUnderError[i] = (depth[i] - depth[i - 1]) * (trans[i - 1] - trans[i]);
        }

        // Find the node the generates the smallest removal error
        int smallestErrorIdx;
        float smallestError;

        smallestErrorIdx = startRemovalIdx;
        smallestError    = nodeUnderError[smallestErrorIdx];
        i = startRemovalIdx + 1;

        [unroll]for ( ; i < removalCandidateCount; ++i) {
            [flatten]if (nodeUnderError[i] < smallestError) {
                smallestError = nodeUnderError[i];
                smallestErrorIdx = i;
            } 
        }

        // Remove that node..
        [unroll]for (i = startRemovalIdx; i < AOIT_NODE_COUNT; ++i) {
            [flatten]if (smallestErrorIdx <= i) {
                depth[i] = depth[i + 1];
            }
        }
        [unroll]for (i = startRemovalIdx - 1; i < AOIT_NODE_COUNT; ++i) {
            [flatten]if (smallestErrorIdx - 1 <= i) {
                trans[i] = trans[i + 1];
            }
        }
    }
    
    // Pack AOIT data
    [unroll] for (i = 0; i < AOIT_RT_COUNT; ++i) {
	    [unroll] for (j = 0; j < 4; ++j) {
		    AOITData.depth[i][j] = depth[4 * i + j];
		    AOITData.trans[i][j] = trans[4 * i + j];			        
	    }
    }	
}

#endif // H_AOIT

