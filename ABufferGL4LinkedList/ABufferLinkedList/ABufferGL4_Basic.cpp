/**
 * Fast Single-pass A-Buffer using OpenGL 4.0 V2.0 : Linked Lists
 * Copyright Cyril Crassin, June/July 2010
 *
 * Basic version (V1.0)
 * It uses either EXT_shader_image_load_store or 
 * GL_NV_shader_buffer_load+GL_NV_shader_buffer_store
 * to fill a list of fragments per pixels during the 
 * rasterisation. 
 * Then in a "resolve" pass, this list is sorted by z 
 * and fragments are blended in order.
 * Fragments are 4x 32bits float components containing 
 * RGB + Z.
 * 
 * There are two paths, one using textures 
 * (EXT_shader_image_load_store) and one using global memory
 * (GL_NV_shader_buffer_load+GL_NV_shader_buffer_store) to 
 * store the A-Buffer. 
 * On GF100, using textures is a little bit faster.
 *
 * This example also uses Direct State Access to remove 
 * multiple bindings. (EXT_direct_state_access)
**/


#include "GL/glew.h"
#include "ABufferGL4_Basic.h"
#include "ShadersManagment.h"

#include "ABufferGL4Parameters.h"

//ABuffer textures (pABufferUseTextures)
GLuint abufferTexID=0;
GLuint abufferCounterTexID=0;

//ABuffer VBOs (pABufferUseTextures==0)
GLuint abufferID=0;
GLuint abufferIdxID=0;
//ABuffer global memory addresses
GLuint64EXT abufferGPUAddress=0;
GLuint64EXT abufferCounterGPUAddress=0;

///Shaders///
GLuint renderABuffProg=0;
GLuint dispABuffProg=0;
GLuint clearABuffProg=0;

void drawQuad(GLuint prog);
void drawModel(GLuint prog);


void initShaders_Basic(void) {


	renderABuffProg=createShaderProgram("Shaders/renderABufferVert.glsl", NULL, "Shaders/renderABufferFrag.glsl", renderABuffProg );
	dispABuffProg=createShaderProgram("Shaders/passThroughVert.glsl", NULL, "Shaders/dispABufferFrag.glsl", dispABuffProg );
	clearABuffProg=createShaderProgram("Shaders/passThroughVert.glsl", NULL, "Shaders/clearABufferFrag.glsl", clearABuffProg );

	linkShaderProgram(renderABuffProg);
    linkShaderProgram(dispABuffProg);
    linkShaderProgram(clearABuffProg);

	    
	checkGLError ("initShader");
}

//Initialize A-Buffer storage. 
//It is composed of a fragment buffer with 
//ABUFFER_SIZE layers and a "counter" buffer used to maintain the number 
//of fragments stored per pixel.
void initABuffer_Basic(){

	//Texture storage path
	if( pABufferUseTextures ){
		
		///ABuffer storage///
		if(!abufferTexID)
			glGenTextures(1, &abufferTexID);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, abufferTexID);

		// Set filter
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		//Texture creation
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowSize.x, windowSize.y*ABUFFER_SIZE, 0,  GL_RGBA, GL_FLOAT, 0);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, windowSize.x, windowSize.y, ABUFFER_SIZE, 0,  GL_RGBA, GL_FLOAT, 0);
		glBindImageTextureEXT(0, abufferTexID, 0, true, 0,  GL_READ_WRITE, GL_RGBA32F);

		checkGLError ("AbufferTex");

		///ABuffer per-pixel counter///
		if(!abufferCounterTexID)
			glGenTextures(1, &abufferCounterTexID);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, abufferCounterTexID);

		// Set filter
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		//Texture creation
		//Uses GL_R32F instead of GL_R32I that is not working in R257.15
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, windowSize.x, windowSize.y, 0,  GL_RED, GL_FLOAT, 0);
		glBindImageTextureEXT(1, abufferCounterTexID, 0, false, 0,  GL_READ_WRITE, GL_R32UI);

		checkGLError ("AbufferIdxTex");

	}else{	//Global memory storage path

		//Abuffer
		if(!abufferID)
			glGenBuffers(1, &abufferID);
		glBindBuffer(GL_ARRAY_BUFFER_ARB, abufferID);

		glBufferData(GL_ARRAY_BUFFER_ARB, windowSize.x*windowSize.y*sizeof(AbufferType)*4*ABUFFER_SIZE, NULL, GL_STATIC_DRAW);
		glMakeBufferResidentNV(GL_ARRAY_BUFFER_ARB, GL_READ_WRITE);
		glGetBufferParameterui64vNV(GL_ARRAY_BUFFER_ARB, GL_BUFFER_GPU_ADDRESS_NV, &abufferGPUAddress); 
		

		//AbufferIdx
		if(!abufferIdxID)
			glGenBuffers(1, &abufferIdxID);
		glBindBuffer(GL_ARRAY_BUFFER_ARB, abufferIdxID);
		
		glBufferData(GL_ARRAY_BUFFER_ARB, windowSize.x*windowSize.y*sizeof(uint), NULL, GL_STATIC_DRAW);
		glMakeBufferResidentNV(GL_ARRAY_BUFFER_ARB, GL_READ_WRITE);
		glGetBufferParameterui64vNV(GL_ARRAY_BUFFER_ARB, GL_BUFFER_GPU_ADDRESS_NV, &abufferCounterGPUAddress); 
		
		checkGLError ("Abuffer");
	}

	char buff[256];
	sprintf(buff, "[ABuffer Basic] Memory usage: %.2fMB", (windowSize.x*windowSize.y*ABUFFER_SIZE*4*sizeof(float)/1024)/1024.0f);
	infoWidget.setStateMessage(buff);

}


void displayClearABuffer_Basic(){
	//Assign uniform parameters
	if(pABufferUseTextures){
		glProgramUniform1iEXT(clearABuffProg, glGetUniformLocation(clearABuffProg, "abufferImg"), 0);
		glProgramUniform1iEXT(clearABuffProg, glGetUniformLocation(clearABuffProg, "abufferCounterImg"), 1);
	}else{
		glProgramUniformui64NV(clearABuffProg, glGetUniformLocation(clearABuffProg, "d_abuffer"), abufferGPUAddress);
		glProgramUniformui64NV(clearABuffProg, glGetUniformLocation(clearABuffProg, "d_abufferIdx"), abufferCounterGPUAddress);
	}

	//Render the full screen quad
	drawQuad(clearABuffProg);

	//Ensure that all global memory write are done before starting to render
	glMemoryBarrierEXT(GL_SHADER_GLOBAL_ACCESS_BARRIER_BIT_NV);
}

void displayRenderABuffer_Basic(NemoGraphics::Mat4f &modelViewMatrix, NemoGraphics::Mat4f &projectionMatrix){
	//Assign uniform parameters
	if(pABufferUseTextures){
		glProgramUniform1iEXT(renderABuffProg, glGetUniformLocation(renderABuffProg, "abufferImg"), 0);
		glProgramUniform1iEXT(renderABuffProg, glGetUniformLocation(renderABuffProg, "abufferCounterImg"), 1);

	}else{
		glProgramUniformui64NV(renderABuffProg, glGetUniformLocation(renderABuffProg, "d_abuffer"), abufferGPUAddress);
		glProgramUniformui64NV(renderABuffProg, glGetUniformLocation(renderABuffProg, "d_abufferIdx"), abufferCounterGPUAddress);
	}

	//Pass matrices to the shader
	glProgramUniformMatrix4fvEXT(renderABuffProg, glGetUniformLocation(renderABuffProg, "projectionMat"), 1, GL_FALSE, projectionMatrix.mat);
	glProgramUniformMatrix4fvEXT(renderABuffProg, glGetUniformLocation(renderABuffProg, "modelViewMat"), 1, GL_FALSE, modelViewMatrix.mat);
	NemoGraphics::Mat4f	modelViewMatrixIT=modelViewMatrix.inverse().transpose();
	glProgramUniformMatrix4fvEXT(renderABuffProg, glGetUniformLocation(renderABuffProg, "modelViewMatIT"), 1, GL_FALSE, modelViewMatrixIT.mat);

	//Render the model
	drawModel(renderABuffProg);
}

void displayResolveABuffer_Basic(){
	//Ensure that all global memory write are done before resolving
	glMemoryBarrierEXT(GL_SHADER_GLOBAL_ACCESS_BARRIER_BIT_NV);

	//Assign uniform parameters
	if(pABufferUseTextures){
		glProgramUniform1iEXT(dispABuffProg, glGetUniformLocation(dispABuffProg, "abufferImg"), 0);
		glProgramUniform1iEXT(dispABuffProg, glGetUniformLocation(dispABuffProg, "abufferCounterImg"), 1);
	}else{
		glProgramUniformui64NV(dispABuffProg, glGetUniformLocation(dispABuffProg, "d_abuffer"), abufferGPUAddress);
		glProgramUniformui64NV(dispABuffProg, glGetUniformLocation(dispABuffProg, "d_abufferIdx"), abufferCounterGPUAddress);
	}

	drawQuad(dispABuffProg);
}