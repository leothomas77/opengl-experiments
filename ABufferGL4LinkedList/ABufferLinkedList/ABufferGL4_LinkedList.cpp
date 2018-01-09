/**
 * Fast Single-pass A-Buffer using OpenGL 4.0 V2.0 : Linked Lists
 * Copyright Cyril Crassin, July 2010
 *
 * Linked Lists version (V2.0)
 * Fragments are stored in small pages in a shared pool.
 * Each pixel stores a pointer to the current page in the pool
 * as well as a counter of written fragments.
 * The shared pool is composed of a fragment buffer and a link buffer.
 * The link buffer stores, for each page, the index of the chained previous page.
**/

#include "GL/glew.h"
#include "ABufferGL4_LinkedList.h"
#include "ShadersManagment.h"

#include "ABufferGL4Parameters.h"

///Linked Lists///
uint sharedPoolSize=16;

GLuint		curSharedPageBuffID=0;	//VBO
GLuint64EXT curSharedPageAddress=0;

GLuint abufferPageIdxBuffID=0;
GLuint64EXT abufferPageIdxAddress=0;
GLuint abufferPageIdxTexID=0;

GLuint abufferFragCountBuffID=0;
GLuint64EXT abufferFragCountAddress=0;
GLuint abufferFragCountTexID=0;

GLuint sharedPageListBuffID=0;
GLuint64EXT sharedPageListAddress=0;
GLuint sharedPageListTexID=0;

GLuint sharedLinkListBuffID=0;
GLuint64EXT sharedLinkListAddress=0;
GLuint sharedLinkListTexID=0;

GLuint semaphoreBuffID=0;
GLuint64EXT semaphoreAddress=0;
GLuint semaphoreTexID=0;

GLuint renderABuffLinkedListProg=0;
GLuint clearABuffLinkedListProg=0;
GLuint dispABuffLinkedListProg=0;

//Occlusion queries
GLuint totalFragmentQuery=0;
uint lastFrameNumFrags=0;
bool queryRequested=false;

void drawQuad(GLuint prog);
void drawModel(GLuint prog);

void initShaders_LinkedList(void) {

	renderABuffLinkedListProg=createShaderProgram("Shaders/renderABufferLinkedListVert.glsl", NULL, "Shaders/renderABufferLinkedListFrag.glsl", renderABuffLinkedListProg );
	dispABuffLinkedListProg=createShaderProgram("Shaders/passThroughVert.glsl", NULL, "Shaders/dispABufferLinkedListFrag.glsl", dispABuffLinkedListProg );
	clearABuffLinkedListProg=createShaderProgram("Shaders/passThroughVert.glsl", NULL, "Shaders/clearABufferLinkedListFrag.glsl", clearABuffLinkedListProg );

	linkShaderProgram(renderABuffLinkedListProg);
    linkShaderProgram(dispABuffLinkedListProg);
    linkShaderProgram(clearABuffLinkedListProg);

	    
	checkGLError ("initShader");
}


void linkedList_InitSharedPool(){

	if( pSharedPoolUseTextures ){
		///Shared page list///
		if(!sharedPageListBuffID)
			glGenBuffers(1, &sharedPageListBuffID);
		glBindBuffer(GL_TEXTURE_BUFFER, sharedPageListBuffID);
		glBufferData(GL_TEXTURE_BUFFER, sharedPoolSize*sizeof(AbufferType)*4, NULL, GL_STATIC_DRAW);
		glMakeBufferResidentNV(GL_TEXTURE_BUFFER, GL_READ_WRITE);
		glGetBufferParameterui64vNV(GL_TEXTURE_BUFFER, GL_BUFFER_GPU_ADDRESS_NV, &sharedPageListAddress); 
		if(!sharedPageListTexID)
			glGenTextures(1, &sharedPageListTexID);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_BUFFER, sharedPageListTexID);
		//Associate BO storage with the texture
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, sharedPageListBuffID);
		glBindImageTextureEXT(3, sharedPageListTexID, 0, false, 0,  GL_READ_WRITE, GL_RGBA32F);
		checkGLError ("sharedPageListTexID");


		///ABuffer shared link pointer list///
		if(!sharedLinkListBuffID)
			glGenBuffers(1, &sharedLinkListBuffID);
		glBindBuffer(GL_TEXTURE_BUFFER, sharedLinkListBuffID);
		glBufferData(GL_TEXTURE_BUFFER, sharedPoolSize/ABUFFER_PAGE_SIZE*sizeof(uint), NULL, GL_STATIC_DRAW);
		glMakeBufferResidentNV(GL_TEXTURE_BUFFER, GL_READ_WRITE);
		glGetBufferParameterui64vNV(GL_TEXTURE_BUFFER, GL_BUFFER_GPU_ADDRESS_NV, &sharedLinkListAddress); 
		if(!sharedLinkListTexID)
			glGenTextures(1, &sharedLinkListTexID);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_BUFFER, sharedLinkListTexID);
		//Associate BO storage with the texture
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, sharedLinkListBuffID);
		glBindImageTextureEXT(4, sharedLinkListTexID, 0, false, 0,  GL_READ_WRITE, GL_R32UI);
		checkGLError ("sharedLinkListTexID");

	}else{
		///Shared page list///
		if(!sharedPageListBuffID)
			glGenBuffers(1, &sharedPageListBuffID);
		glBindBuffer(GL_TEXTURE_BUFFER, sharedPageListBuffID);
		glBufferData(GL_TEXTURE_BUFFER, sharedPoolSize*sizeof(AbufferType)*4, NULL, GL_STATIC_DRAW);
		glMakeBufferResidentNV(GL_TEXTURE_BUFFER, GL_READ_WRITE);
		glGetBufferParameterui64vNV(GL_TEXTURE_BUFFER, GL_BUFFER_GPU_ADDRESS_NV, &sharedPageListAddress); 

		///Shared link pointer list///
		if(!sharedLinkListBuffID)
			glGenBuffers(1, &sharedLinkListBuffID);
		glBindBuffer(GL_TEXTURE_BUFFER, sharedLinkListBuffID);
		glBufferData(GL_TEXTURE_BUFFER, sharedPoolSize/ABUFFER_PAGE_SIZE*sizeof(uint), NULL, GL_STATIC_DRAW);
		glMakeBufferResidentNV(GL_TEXTURE_BUFFER, GL_READ_WRITE);
		glGetBufferParameterui64vNV(GL_TEXTURE_BUFFER, GL_BUFFER_GPU_ADDRESS_NV, &sharedLinkListAddress); 

	}
	
}

void initABuffer_LinkedList(){

	//Init occlusion query
	if(!totalFragmentQuery)
		glGenQueries(1, &totalFragmentQuery);

	if( pABufferUseTextures ){
		///Page idx storage///
		if(!abufferPageIdxTexID)
			glGenTextures(1, &abufferPageIdxTexID);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, abufferPageIdxTexID);
		// Set filter
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//Texture creation
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, windowSize.x, windowSize.y, 0,  GL_RED, GL_FLOAT, 0);
		glBindImageTextureEXT(0, abufferPageIdxTexID, 0, false, 0,  GL_READ_WRITE, GL_R32UI);
		checkGLError ("abufferPageIdxID");

		///per-pixel page counter///
		if(!abufferFragCountTexID)
			glGenTextures(1, &abufferFragCountTexID);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, abufferFragCountTexID);
		// Set filter
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//Texture creation
		//Uses GL_R32F instead of GL_R32I that is not working in R257.15
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, windowSize.x, windowSize.y, 0,  GL_RED, GL_FLOAT, 0);
		glBindImageTextureEXT(1, abufferFragCountTexID, 0, false, 0,  GL_READ_WRITE, GL_R32UI);
		checkGLError ("abufferPageCountID");

		///Semaphore///
		if(!semaphoreTexID)
			glGenTextures(1, &semaphoreTexID);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, semaphoreTexID);
		// Set filter
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//Texture creation
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, windowSize.x, windowSize.y, 0,  GL_RED, GL_FLOAT, 0);
		glBindImageTextureEXT(2, semaphoreTexID, 0, false, 0,  GL_READ_WRITE, GL_R32UI);
		checkGLError ("semaphoreTexID");
		
	}else{

		///Page idx storage///
		if(!abufferPageIdxBuffID)
			glGenBuffers(1, &abufferPageIdxBuffID);
		glBindBuffer(GL_TEXTURE_BUFFER, abufferPageIdxBuffID);
		glBufferData(GL_TEXTURE_BUFFER, windowSize.x*windowSize.y*sizeof(uint), NULL, GL_STATIC_DRAW);
		glMakeBufferResidentNV(GL_TEXTURE_BUFFER, GL_READ_WRITE);
		glGetBufferParameterui64vNV(GL_TEXTURE_BUFFER, GL_BUFFER_GPU_ADDRESS_NV, &abufferPageIdxAddress);
		checkGLError ("abufferPageIdxID");

		///per-pixel page counter///
		if(!abufferFragCountBuffID)
			glGenBuffers(1, &abufferFragCountBuffID);
		glBindBuffer(GL_TEXTURE_BUFFER, abufferFragCountBuffID);
		glBufferData(GL_TEXTURE_BUFFER, windowSize.x*windowSize.y*sizeof(uint), NULL, GL_STATIC_DRAW);
		glMakeBufferResidentNV(GL_TEXTURE_BUFFER, GL_READ_WRITE);
		glGetBufferParameterui64vNV(GL_TEXTURE_BUFFER, GL_BUFFER_GPU_ADDRESS_NV, &abufferFragCountAddress);
		checkGLError ("abufferFragCountID");

		///Semaphore///
		if(!semaphoreBuffID)
			glGenBuffers(1, &semaphoreBuffID);
		glBindBuffer(GL_TEXTURE_BUFFER, semaphoreBuffID);
		glBufferData(GL_TEXTURE_BUFFER, windowSize.x*windowSize.y*sizeof(uint), NULL, GL_STATIC_DRAW);
		glMakeBufferResidentNV(GL_TEXTURE_BUFFER, GL_READ_WRITE);
		glGetBufferParameterui64vNV(GL_TEXTURE_BUFFER, GL_BUFFER_GPU_ADDRESS_NV, &semaphoreAddress);
		checkGLError ("semaphoreTexID");
	}
	
	
	linkedList_InitSharedPool();

	///Shared page counter///
	if(!curSharedPageBuffID)
		glGenBuffers(1, &curSharedPageBuffID);
	glBindBuffer(GL_ARRAY_BUFFER_ARB, curSharedPageBuffID);
	glBufferData(GL_ARRAY_BUFFER_ARB, sizeof(uint), NULL, GL_STATIC_DRAW);
	glMakeBufferResidentNV(GL_ARRAY_BUFFER_ARB, GL_READ_WRITE);
	glGetBufferParameterui64vNV(GL_ARRAY_BUFFER_ARB, GL_BUFFER_GPU_ADDRESS_NV, &curSharedPageAddress); 
	checkGLError ("curSharedPageBuffID");


	char buff[256];
	sprintf(buff, "[ABuffer Linked Lists] Memory usage: %.2fMB", (sharedPoolSize*4*sizeof(float)/1024)/1024.0f);
	infoWidget.setStateMessage(buff);
}


void linkedListAssignUniforms(GLuint prog){
	if( pABufferUseTextures ){
		glProgramUniform1iEXT(prog, glGetUniformLocation(prog, "abufferPageIdxImg"), 0);
		glProgramUniform1iEXT(prog, glGetUniformLocation(prog, "abufferFragCountImg"), 1);
		glProgramUniform1iEXT(prog, glGetUniformLocation(prog, "semaphoreImg"), 2);	
	}else{
		glProgramUniformui64NV(prog, glGetUniformLocation(prog, "d_abufferPageIdx"), abufferPageIdxAddress);
		glProgramUniformui64NV(prog, glGetUniformLocation(prog, "d_abufferFragCount"), abufferFragCountAddress);
		glProgramUniformui64NV(prog, glGetUniformLocation(prog, "d_semaphore"), semaphoreAddress);
	}

	if( pSharedPoolUseTextures ){
		glProgramUniform1iEXT(prog, glGetUniformLocation(prog, "sharedPageListImg"), 3);
		glProgramUniform1iEXT(prog, glGetUniformLocation(prog, "sharedLinkListImg"), 4);
	}else{
		glProgramUniformui64NV(prog, glGetUniformLocation(prog, "d_sharedPageList"), sharedPageListAddress);
		glProgramUniformui64NV(prog, glGetUniformLocation(prog, "d_sharedLinkList"), sharedLinkListAddress);
	}

	glProgramUniformui64NV(prog, glGetUniformLocation(prog, "d_curSharedPage"), curSharedPageAddress);

	glProgramUniform1iEXT(prog, glGetUniformLocation(prog, "abufferSharedPoolSize"), sharedPoolSize);
	
}

void displayClearABuffer_LinkedList(){

	//Assign uniform parameters
	linkedListAssignUniforms(clearABuffLinkedListProg);

	//Render the full screen quad
	drawQuad(clearABuffLinkedListProg);

	//Ensure that all global memory write are done before starting to render
	glMemoryBarrierEXT(GL_SHADER_GLOBAL_ACCESS_BARRIER_BIT_NV);
}

void displayRenderABuffer_LinkedList(NemoGraphics::Mat4f &modelViewMatrix, NemoGraphics::Mat4f &projectionMatrix){
	NemoGraphics::Mat4f	modelViewMatrixIT=modelViewMatrix.inverse().transpose();

	//Assign uniform parameters
	linkedListAssignUniforms(renderABuffLinkedListProg);

	//Pass matrices to the shader
	glProgramUniformMatrix4fvEXT(renderABuffLinkedListProg, glGetUniformLocation(renderABuffLinkedListProg, "projectionMat"), 1, GL_FALSE, projectionMatrix.mat);
	glProgramUniformMatrix4fvEXT(renderABuffLinkedListProg, glGetUniformLocation(renderABuffLinkedListProg, "modelViewMat"), 1, GL_FALSE, modelViewMatrix.mat);
	glProgramUniformMatrix4fvEXT(renderABuffLinkedListProg, glGetUniformLocation(renderABuffLinkedListProg, "modelViewMatIT"), 1, GL_FALSE, modelViewMatrixIT.mat);


	//glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	glBeginQuery(GL_SAMPLES_PASSED, totalFragmentQuery);

	//Render the model
	drawModel(renderABuffLinkedListProg);

	glEndQuery(GL_SAMPLES_PASSED);
	queryRequested=true;

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	//Ensure that all global memory write are done before starting to render
	glMemoryBarrierEXT(GL_SHADER_GLOBAL_ACCESS_BARRIER_BIT_NV);
}


void displayResolveABuffer_LinkedList(){
	//Ensure that all global memory write are done before resolving
	glMemoryBarrierEXT(GL_SHADER_GLOBAL_ACCESS_BARRIER_BIT_NV);

	//Assign uniform parameters
	linkedListAssignUniforms(dispABuffLinkedListProg);

	drawQuad(dispABuffLinkedListProg);
}

bool display_LinkedList_ManageSharedPool(){
	//Resize shared pool dynamically
	
	lastFrameNumFrags=0;
	if(queryRequested){
		glGetQueryObjectuiv(totalFragmentQuery, GL_QUERY_RESULT, &lastFrameNumFrags);
	
		//A fragments is not discarded each time a page fails to be allocated
		if(lastFrameNumFrags>0){
			sharedPoolSize=sharedPoolSize+(lastFrameNumFrags/ABUFFER_PAGE_SIZE+1)*ABUFFER_PAGE_SIZE*2;
			linkedList_InitSharedPool();

			std::cout<<"Shared buffer size increased\n";

			char buff[256];
			sprintf(buff, "[ABuffer Linked Lists] Memory usage: %.2fMB", (sharedPoolSize*4*sizeof(float)/1024)/1024.0f);
			infoWidget.setStateMessage(buff);

			
		}

		queryRequested=false;
	}

	//checkGLError ("display_LinkedList_ManageSharedPool");

	if(lastFrameNumFrags)
		return true;
	else
		return false;
}