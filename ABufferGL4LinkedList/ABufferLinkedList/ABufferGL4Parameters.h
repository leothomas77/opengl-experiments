#ifndef ABUFFERGL4PARAMETERS_H
#define ABUFFERGL4PARAMETERS_H
#include "InfoWidget.h"


//Number of layers in the A-Buffer
#define ABUFFER_SIZE			16
//Mesh is loaded from a raw file instead of a formated file->requires assimp
#define LOAD_RAW_MESH			1

#define ABUFFER_PAGE_SIZE		4

//Parameters
enum ABufferAlgorithm{ABuffer_Basic, ABuffer_LinkedList};
extern ABufferAlgorithm pABufferAlgorithm;
extern int pABufferUseTextures;
extern int pSharedPoolUseTextures;

extern NemoGraphics::Vector2i windowSize;


extern InfoWidget infoWidget;

//Because current glew does not define it
#ifndef GL_SHADER_GLOBAL_ACCESS_BARRIER_BIT_NV
#define GL_SHADER_GLOBAL_ACCESS_BARRIER_BIT_NV             0x00000010
#endif

#endif