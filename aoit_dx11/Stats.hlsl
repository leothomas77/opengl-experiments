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

#ifndef STATS_SHADER_CODE
#define STATS_SHADER_CODE

#include "Stats.h"
#include "FragmentList.hlsl"

StructuredBuffer<FragmentStats>     gFragmentStatsSRV;
RWStructuredBuffer<FragmentStats>   gFragmentStatsUAV;

/////////////////////////////////////////////////////////////


void ClearFragmentStats(inout FragmentStats stats)
{
    stats.pixelCount                = 0;
    stats.fragmentCount             = 0;
	stats.fragmentSquareCount       = 0;
    stats.maxFragmentPerPixelCount  = 0;
    stats.minDepth                  = 0xFFFFFFFFU;
    stats.maxDepth                  = 0;
    stats.localMinDepth             = 0xFFFFFFFFU;
    stats.localMaxDepth             = 0;
}

/////////////////////////////////////////////////////////////


[numthreads(1, 1, 1)]
void ClearFragmentStatsCS()
{
	FragmentStats stats;
	ClearFragmentStats(stats);

	gFragmentStatsUAV[0] = stats;
}


/////////////////////////////////////////////////////////////

groupshared FragmentStats localStats;

[numthreads(STATS_TILEDIM_X, STATS_TILEDIM_X, 1)]
void ComputeFragmentStatsCS(uint3 groupID   : SV_GroupID,
                            uint3 threadID  : SV_GroupThreadID,
							uint  threadIDFlattened : SV_GroupIndex)
{
	
	// Clear local stats
	if (threadIDFlattened == 0U) {
		FragmentStats stats;
		ClearFragmentStats(stats);

		localStats = stats;
	}

	GroupMemoryBarrierWithGroupSync();

    // Get offset to the first node
    int2 screenAddress = int2(groupID.xy * uint2(STATS_TILEDIM_X, STATS_TILEDIM_Y) + threadID.xy);
	
	uint fragmentCount = 0;
	int2 viewDim = FL_GetDimensions();

	if (all(screenAddress < viewDim)) {
		uint nodeOffset = FL_GetFirstNodeOffset(screenAddress);   
   
		[loop] while (nodeOffset != FL_NODE_LIST_NULL) {
			// Get fragment node..  
			FragmentListNode node = FL_GetNode(nodeOffset);

			float4 fragmentColor = FL_UnpackColor(node.color);

			// Update fragment counter
			fragmentCount++;
    
            InterlockedMin(localStats.minDepth, asuint(node.depth));
            InterlockedMax(localStats.maxDepth, asuint(node.depth));

			// Move to next node
			nodeOffset = node.next;        
		}

		// Count pixels that map to non-empty lists
		// Also find the smallest number of nodes per list (except empty nodes)
		if (0 != fragmentCount) {
			InterlockedAdd(localStats.pixelCount, 1U);
		}

		// Accumulate fragment count
		InterlockedAdd(localStats.fragmentCount, fragmentCount);
		InterlockedAdd(localStats.fragmentSquareCount, fragmentCount * fragmentCount);
    
		// Accumulate max number of fragment per pixel
		InterlockedMax(localStats.maxFragmentPerPixelCount, fragmentCount);
	}

	GroupMemoryBarrierWithGroupSync();

	if (threadIDFlattened == 0U) {
		// local stats --> global stats
		if (0 != fragmentCount) {
			InterlockedAdd(gFragmentStatsUAV[0].pixelCount, localStats.pixelCount);
		}

		InterlockedAdd(gFragmentStatsUAV[0].fragmentCount, localStats.fragmentCount);
		InterlockedAdd(gFragmentStatsUAV[0].fragmentSquareCount, localStats.fragmentSquareCount);
		InterlockedMax(gFragmentStatsUAV[0].maxFragmentPerPixelCount, localStats.maxFragmentPerPixelCount);
		InterlockedMin(gFragmentStatsUAV[0].minDepth, localStats.minDepth);
		InterlockedMax(gFragmentStatsUAV[0].maxDepth, localStats.maxDepth);
	}
}

#endif // STATS_SHADER_CODE