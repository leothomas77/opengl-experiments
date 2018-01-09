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

#include "MathUtils.h"

#include <algorithm>
#include <limits>

namespace MathUtils {

void
TransformBBox(D3DXVECTOR3 *min,
              D3DXVECTOR3 *max,
              const D3DXMATRIXA16 &m)
{
    const D3DXVECTOR3 &minCorner = *min;
    const D3DXVECTOR3 &maxCorner = *max;
    D3DXVECTOR3 corners[8];
    // Bottom corners
    corners[0] = D3DXVECTOR3(minCorner.x, minCorner.y, minCorner.z);
    corners[1] = D3DXVECTOR3(maxCorner.x, minCorner.y, minCorner.z);
    corners[2] = D3DXVECTOR3(maxCorner.x, minCorner.y, maxCorner.z);
    corners[3] = D3DXVECTOR3(minCorner.x, minCorner.y, maxCorner.z);
    // Top corners
    corners[4] = D3DXVECTOR3(minCorner.x, maxCorner.y, minCorner.z);
    corners[5] = D3DXVECTOR3(maxCorner.x, maxCorner.y, minCorner.z);
    corners[6] = D3DXVECTOR3(maxCorner.x, maxCorner.y, maxCorner.z);
    corners[7] = D3DXVECTOR3(minCorner.x, maxCorner.y, maxCorner.z);

    D3DXVECTOR3 newCorners[8];
    for (int i = 0; i < 8; i++) {
        D3DXVec3TransformCoord(&newCorners[i], &corners[i], &m);
    }

    D3DXVECTOR3 newMin, newMax;

    // Initialize
    for (int i = 0; i < 3; ++i) {
        newMin[i] = std::numeric_limits<float>::max();
        newMax[i] = std::numeric_limits<float>::min();
    }

    // Find new min and max corners
    for (int i = 0; i < 8; i++) {
        D3DXVec3Minimize(&newMin, &newMin, &newCorners[i]);
        D3DXVec3Maximize(&newMax, &newMax, &newCorners[i]);
    }

    *min = newMin;
    *max = newMax;
}

}
