#ifndef ABUFFERGL4_BASIC_H
#define ABUFFERGL4_BASIC_H

#include "Vector.h"
#include "Matrix.h"

typedef float AbufferType;
typedef unsigned int uint;

void initShaders_Basic(void);
void initABuffer_Basic();
void displayClearABuffer_Basic();
void displayRenderABuffer_Basic(NemoGraphics::Mat4f &modelViewMatrix, NemoGraphics::Mat4f &projectionMatrix);
void displayResolveABuffer_Basic();
#endif