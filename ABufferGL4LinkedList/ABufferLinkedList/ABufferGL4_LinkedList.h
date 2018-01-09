#ifndef ABUFFERGL4_LINKEDLIST_H
#define ABUFFERGL4_LINKEDLIST_H

#include "Vector.h"
#include "Matrix.h"

typedef float AbufferType;
typedef unsigned int uint;

void initShaders_LinkedList(void);
void initABuffer_LinkedList();

void displayClearABuffer_LinkedList();
void displayRenderABuffer_LinkedList(NemoGraphics::Mat4f &modelViewMatrix, NemoGraphics::Mat4f &projectionMatrix);
void displayResolveABuffer_LinkedList();

bool display_LinkedList_ManageSharedPool();
#endif