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


#ifndef H_STATS_CPP
#define H_STATS_CPP

#define STATS_TILEDIM_X (16)
#define STATS_TILEDIM_Y (16)

// We define a different data type for the stats struct members if we
// are including this header from a shader

#ifdef STATS_SHADER_CODE
typedef uint stats_data;
#else
typedef unsigned int stats_data;
#endif // STATS_SHADER_CODE

struct FragmentStats
{
    stats_data  pixelCount;
    stats_data  fragmentCount;
    stats_data  fragmentSquareCount;
    stats_data  maxFragmentPerPixelCount;
    stats_data  minDepth;
    stats_data  maxDepth;
    stats_data  localMinDepth;
    stats_data  localMaxDepth;
};

#ifndef STATS_SHADER_CODE

struct FrameStats
{
    float minDepth;
    float maxDepth;
    float fragmentCount;
    float capturePassTime;
    float resolvePassTime;
    float fragmentPerSecond;
    float avgFragmentPerPixel;
    float stdDevFragmentPerPixel;
    float avgFragmentPerNonEmptyPixel;
    float stdDevFragmentNonEmptyPerPixel;
    float transparentFragmentScreenCoverage;
    unsigned int maxFragmentPerPixelCount;
};

#endif

#endif // H_STATS_CPP