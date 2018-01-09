/**
 * Fast Single-pass A-Buffer using OpenGL 4.0
 * Copyright Cyril Crassin, June 2010
 *
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

#include <GL/freeglut.h>

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <sys/stat.h>

#include "ShadersManagment.h"
#include "Matrix.h"
#include "InfoWidget.h"

//Number of layers in the A-Buffer
#define ABUFFER_SIZE			16
//Mesh is loaded from a raw file instead of a formated file->requires assimp
#define LOAD_RAW_MESH			1


//Because current glew does not define it
#ifndef GL_SHADER_GLOBAL_ACCESS_BARRIER_BIT_NV
#define GL_SHADER_GLOBAL_ACCESS_BARRIER_BIT_NV             0x00000010
#endif

//Filename of the mesh to use
const char *modelNames[]={
	"../models/dragon_vrip.ply", 
	"../models/happy_vrip.ply",
	"../models/Armadillo.ply"	//"../models0/bun_zipper.ply" //, "../models0/xyzrgb_dragon.ply"
};


//Program parameters
int pUseABuffer=1;
int pABufferUseSorting=1;
int pResolveAlphaCorrection=1;
int pResolveGelly=0;
int pABufferUseTextures=1;
NemoGraphics::Vector4f pBackgroundColor=NemoGraphics::Vector4f(1.0f, 1.0f, 1.0f, 1.0f);	//0.15f

//Types
typedef float AbufferType;
typedef unsigned int uint;


//View properties & manip
NemoGraphics::Vector2i windowSize=NemoGraphics::Vector2i(512, 512);
NemoGraphics::Vector2i moldpos;
NemoGraphics::Vector3f viewPos(0.0, 0.0, -1.0);
NemoGraphics::Vector3f viewOrient;
bool mclicked;
int mbutton;

//MVP matrices
GLfloat projMatrix[16];
NemoGraphics::Mat4f projectionMatrix; //Projection matrix
NemoGraphics::Mat4f modelViewMatrix;

InfoWidget infoWidget;

//Full screen quad vertices definition
const GLfloat quadVArray[] = {
   -1.0f, -1.0f, 0.0f, 1.0f,
   1.0f, -1.0f, 0.0f, 1.0f,
   -1.0f, 1.0f, 0.0f, 1.0f,    
   1.0f, -1.0f, 0.0f, 1.0f,
   1.0f, 1.0f, 0.0f, 1.0f,
   -1.0f, 1.0f, 0.0f, 1.0f  
};


//ABuffer textures (pABufferUseTextures)
GLuint abufferTexID=0;
GLuint abufferCounterTexID=0;

//ABuffer VBOs (pABufferUseTextures==0)
GLuint abufferID=0;
GLuint abufferIdxID=0;
//ABuffer global memory addresses
GLuint64EXT abufferGPUAddress;
GLuint64EXT abufferCounterGPUAddress;

//Shaders
GLuint renderABuffProg=0;
GLuint dispABuffProg=0;
GLuint clearABuffProg=0;

//Timing
const int timingNumFrames=2000;	//Use 0 to disable
uint frameCounter=0;
int timeCounter;

//Mesh attributes storage
std::vector<float>		meshVertexPositionList;
std::vector<float>		meshVertexNormalList;
std::vector<float>		meshVertexColorList;
std::vector<uint>		meshTriangleList;

//Mesh VBOs
GLuint vertexBufferName=0;
GLuint vertexBufferModelPosID=0;
GLuint vertexBufferModelColorID=0;
GLuint vertexBufferModelNormalID=0;
GLuint vertexBufferModelIndexID=0;

//Mesh loading from a formated file: not compiled by default
void loadMeshFile(const std::string &meshFileName);

//Vectors read/write helpers: for mesh loading
template<class T> void writeStdVector(const std::string &fileName, std::vector<T> &vec);
template<class T> void readStdVector(const std::string &fileName, std::vector<T> &vec);


void initShaders(void) {

	//Shader dynamic macro setting
	resetShadersGlobalMacros();

	setShadersGlobalMacro("ABUFFER_SIZE", ABUFFER_SIZE);
	setShadersGlobalMacro("SCREEN_WIDTH", windowSize.x);
	setShadersGlobalMacro("SCREEN_HEIGHT", windowSize.y);

	setShadersGlobalMacro("BACKGROUND_COLOR_R", pBackgroundColor.x);
	setShadersGlobalMacro("BACKGROUND_COLOR_G", pBackgroundColor.y);
	setShadersGlobalMacro("BACKGROUND_COLOR_B", pBackgroundColor.z);
	
	setShadersGlobalMacro("USE_ABUFFER", pUseABuffer);
	setShadersGlobalMacro("ABUFFER_USE_TEXTURES", pABufferUseTextures);
	setShadersGlobalMacro("ABUFFER_RESOLVE_USE_SORTING", pABufferUseSorting);

	setShadersGlobalMacro("ABUFFER_RESOLVE_ALPHA_CORRECTION", pResolveAlphaCorrection);
	setShadersGlobalMacro("ABUFFER_RESOLVE_GELLY", pResolveGelly);
	

	//Shaders loading
	renderABuffProg=createShaderProgram("Shaders/renderABufferVert.glsl", "Shaders/renderABufferFrag.glsl", renderABuffProg );
	dispABuffProg=createShaderProgram("Shaders/passThroughVert.glsl", "Shaders/dispABufferFrag.glsl", dispABuffProg );
	clearABuffProg=createShaderProgram("Shaders/passThroughVert.glsl", "Shaders/clearABufferFrag.glsl", clearABuffProg );

	checkGLError ("initShader");
}


//Mesh file loading
void loadMesh(std::string fileName){
#if LOAD_RAW_MESH==0
	/////////Mesh
	loadMeshFile(fileName);
	writeStdVector(fileName+"_pos.raw", meshVertexPositionList);
	writeStdVector(fileName+"_normal.raw", meshVertexNormalList);
	//writeStdVector(fileName+"_color.raw", meshVertexColorList);
	writeStdVector(fileName+"_triangles.raw", meshTriangleList);
#else
	readStdVector(fileName+"_pos.raw", meshVertexPositionList);
	readStdVector(fileName+"_normal.raw", meshVertexNormalList);
	//readStdVector(fileName+"_color.raw", meshVertexColorList);
	readStdVector(fileName+"_triangles.raw", meshTriangleList);
#endif

	std::cout<<"Mesh num triangles: "<<meshTriangleList.size()/3<<"\n";
}

//Initialize A-Buffer storage. It is composed of a fragment buffer with 
//ABUFFER_SIZE layers and a "counter" buffer used to maintain the number 
//of fragments stored per pixel.
void initABuffer(){

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
}


void initMesh(int modelNum=0){
	//Mesh loading
	loadMesh(modelNames[modelNum]);

	//Mesh VBOs
	if(!vertexBufferModelPosID)
		glGenBuffers (1, &vertexBufferModelPosID);
	glBindBuffer (GL_ARRAY_BUFFER, vertexBufferModelPosID);
	glBufferData (GL_ARRAY_BUFFER, sizeof(float)*meshVertexPositionList.size(), &(meshVertexPositionList[0]), GL_STATIC_DRAW);

	/*glGenBuffers (1, &vertexBufferModelColorID);
	glBindBuffer (GL_ARRAY_BUFFER, vertexBufferModelColorID);
	glBufferData (GL_ARRAY_BUFFER, sizeof(float)*meshVertexColorList.size(), &(meshVertexColorList[0]), GL_STATIC_DRAW);*/

	if(!vertexBufferModelNormalID)
		glGenBuffers (1, &vertexBufferModelNormalID);
	glBindBuffer (GL_ARRAY_BUFFER, vertexBufferModelNormalID);
	glBufferData (GL_ARRAY_BUFFER, sizeof(float)*meshVertexNormalList.size(), &(meshVertexNormalList[0]), GL_STATIC_DRAW);

	if(!vertexBufferModelIndexID)
		glGenBuffers (1, &vertexBufferModelIndexID);
	glBindBuffer (GL_ARRAY_BUFFER, vertexBufferModelIndexID);
	glBufferData (GL_ARRAY_BUFFER, sizeof(uint)*meshTriangleList.size(), &(meshTriangleList[0]), GL_STATIC_DRAW);
	checkGLError ("init mesh VBOs");
}

//Global init function
void init(void) {
   
	initShaders ();

	initABuffer();

	//Full screen quad initialization
	glGenBuffers (1, &vertexBufferName);
	glBindBuffer (GL_ARRAY_BUFFER, vertexBufferName);
	glBufferData (GL_ARRAY_BUFFER, sizeof(quadVArray), quadVArray, GL_STATIC_DRAW);
	checkGLError ("initBuffer");

	initMesh();

	//Disable backface culling to keep all fragments
	glDisable(GL_CULL_FACE);
	//Disable depth test
	glDisable(GL_DEPTH_TEST);
	//Disable stencil test
	glDisable(GL_STENCIL_TEST);
	//Disable blending
	glDisable(GL_BLEND);
}

//Dump GL infos
void dumpInfo(void) {
   printf ("Vendor: %s\n", glGetString (GL_VENDOR));
   printf ("Renderer: %s\n", glGetString (GL_RENDERER));
   printf ("Version: %s\n", glGetString (GL_VERSION));
   printf ("GLSL: %s\n", glGetString (GL_SHADING_LANGUAGE_VERSION));
   checkGLError ("dumpInfo");
}


void drawQuad(GLuint prog) {

	glUseProgram (prog);

	glEnableVertexAttribArray (glGetAttribLocation(prog, "vertexPos"));

	glBindBuffer (GL_ARRAY_BUFFER, vertexBufferName);

	glVertexAttribPointer (glGetAttribLocation(prog, "vertexPos"), 4, GL_FLOAT, GL_FALSE,
						   sizeof(GLfloat)*4, 0);

	glDrawArrays(GL_TRIANGLES, 0, 24);

	//checkGLError ("triangle");
}


void drawModel(GLuint prog) {
	
	glUseProgram (prog);

	glEnableVertexAttribArray (glGetAttribLocation(prog, "vertexPos"));
	glEnableVertexAttribArray (glGetAttribLocation(prog, "vertexNormal"));

	glBindBuffer (GL_ARRAY_BUFFER, vertexBufferModelPosID);
	glVertexAttribPointer (glGetAttribLocation(renderABuffProg, "vertexPos"), 3, GL_FLOAT, GL_FALSE,0, NULL);

	glBindBuffer (GL_ARRAY_BUFFER, vertexBufferModelNormalID);
	glVertexAttribPointer (glGetAttribLocation(renderABuffProg, "vertexNormal"), 3, GL_FLOAT, GL_FALSE,0, NULL);

	glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, vertexBufferModelIndexID);
	glDrawElements(	GL_TRIANGLES, meshTriangleList.size(), GL_UNSIGNED_INT, NULL);

	//checkGLError ("model");
}

//Global display function
void display(void) {

	//Clear color buffer
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClearColor (pBackgroundColor.x, pBackgroundColor.y, pBackgroundColor.z, pBackgroundColor.w);
	glClear (GL_COLOR_BUFFER_BIT);
	

	//Build view transformation matrix
	modelViewMatrix = NemoGraphics::Mat4f::identity();
	modelViewMatrix *= NemoGraphics::Mat4f::translation(viewPos);
	modelViewMatrix *= NemoGraphics::Mat4f::rotation(NemoGraphics::Vector3f(1.0, 0.0, 0.0), viewOrient.x);
	modelViewMatrix *= NemoGraphics::Mat4f::rotation(NemoGraphics::Vector3f(0.0, 1.0, 0.0), viewOrient.y);
	modelViewMatrix *= NemoGraphics::Mat4f::translation(NemoGraphics::Vector3f(-0.5f, -0.3f, -0.2f));
	
	
	//Clear A-Buffer
	if(pUseABuffer){ 
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
	

	//Renderg the model into the A-Buffer
	{
		//Assign uniform parameters
		if(pABufferUseTextures){
			glProgramUniform1iEXT(renderABuffProg, glGetUniformLocation(renderABuffProg, "abufferImg"), 0);
			glProgramUniform1iEXT(renderABuffProg, glGetUniformLocation(renderABuffProg, "abufferCounterImg"), 1);
		}else{
			glProgramUniformui64NV(renderABuffProg, glGetUniformLocation(renderABuffProg, "d_abuffer"), abufferGPUAddress);
			glProgramUniformui64NV(renderABuffProg, glGetUniformLocation(renderABuffProg, "d_abufferIdx"), abufferCounterGPUAddress);
		}

		//Pass matrices to teh shader
		glProgramUniformMatrix4fvEXT(renderABuffProg, glGetUniformLocation(renderABuffProg, "projectionMat"), 1, GL_FALSE, projectionMatrix.mat);
		glProgramUniformMatrix4fvEXT(renderABuffProg, glGetUniformLocation(renderABuffProg, "modelViewMat"), 1, GL_FALSE, modelViewMatrix.mat);
		NemoGraphics::Mat4f	modelViewMatrixIT=modelViewMatrix.inverse().transpose();
		glProgramUniformMatrix4fvEXT(renderABuffProg, glGetUniformLocation(renderABuffProg, "modelViewMatIT"), 1, GL_FALSE, modelViewMatrixIT.mat);

		//Render the model
		drawModel(renderABuffProg);

	}
	//"Resolve" A-Buffer
	if( pUseABuffer ){
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

	glUseProgram (0);

	frameCounter++;
	if(frameCounter==timingNumFrames){
		glFinish();
		int timeInterval=glutGet(GLUT_ELAPSED_TIME)-timeCounter;

		std::cout<<"FPS: "<<float(timingNumFrames)*1000.0f/float(timeInterval)<<"\n";

		timeCounter=glutGet(GLUT_ELAPSED_TIME);
		frameCounter=0;
	}


	infoWidget.draw();
	

	glutSwapBuffers();

	
	int time;
	time = glutGet(GLUT_ELAPSED_TIME);


	checkGLError ("display");
   
}



void reshape (int w, int h) {
	windowSize.x=w;
	windowSize.y=h;

	glViewport (0, 0, (GLsizei) w, (GLsizei) h);

	initShaders(); initABuffer();

	float ratio = float(w) / float(h);
	projectionMatrix.perspective(60.0f, ratio, 0.010f, 20.0f);

}

void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 27:
		exit(0);
		break;
	case 't':
		pABufferUseTextures=!pABufferUseTextures;
		if(pABufferUseTextures)
			infoWidget.print("Texture storage mode");
		else
			infoWidget.print("Global memory storage mode");
		break;
	case 'a':
		pUseABuffer=!pUseABuffer;
		if(pUseABuffer)
			infoWidget.print("ABuffer ON");
		else
			infoWidget.print("ABuffer OFF");
		break;
	case 's':
		pABufferUseSorting=!pABufferUseSorting;
		if(pABufferUseSorting)
			infoWidget.print("Resolve: fragment Sorting mode");
		else
			infoWidget.print("Resolve: closest fragment mode");
		break;
	case 'c':
		pResolveAlphaCorrection=!pResolveAlphaCorrection;
		if(pResolveAlphaCorrection)
			infoWidget.print("Resolve: blending WITH alpha correction");
		else
			infoWidget.print("Resolve: blending WITHOUT alpha correction");
		break;
	case 'g':
		pResolveGelly=!pResolveGelly;
		if(pResolveGelly)
			infoWidget.print("Resolve: GELLY");
		else
			infoWidget.print("Resolve: ALPHA BLENDING");
		break;
	case '1':
		initMesh(0);
		break;
	case '2':
		initMesh(1);
		break;
	case '3':
		initMesh(2);
		break;
	case '4':
		initMesh(3);
		break;
   }

	initShaders();
	initABuffer();
}


//Called when a mouse button is pressed or released
void mouse_click(int button, int state, int x, int y){
	mclicked=state;
	moldpos=NemoGraphics::Vector2i(x, y);
	mbutton=button;
}

//Called when a mouse move and a button is pressed
void mouse_motion(int x, int y){
	NemoGraphics::Vector2i mmotion=NemoGraphics::Vector2i(x, y)-moldpos;
	NemoGraphics::Vector2f mmotionf=NemoGraphics::Vector2f(mmotion)/NemoGraphics::Vector2f(windowSize);

	if(mbutton==0){
		viewOrient.x=viewOrient.x+mmotionf.y*100.0f;
		viewOrient.y=viewOrient.y+mmotionf.x*100.0f;
	}else if(mbutton==1){
		viewPos.x=viewPos.x+mmotionf.x*1.0f;
		viewPos.y=viewPos.y-mmotionf.y*1.0f;
	}else if(mbutton==2){
		viewPos.z=viewPos.z+mmotionf.y*1.0f;
	}
	moldpos=NemoGraphics::Vector2i(x, y);
}

int main(int argc, char** argv) {

	std::cout<<"\n"	<<"Fast Single-Pass A-Buffer using OpenGL 4.0\n"
					<<"Cyril Crassin, June 2010\n\n";

	glutInit(&argc, argv);
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL);

	//Init OpenGL 4.0 context
	glutInitContextVersion (4, 0);
	glutInitContextProfile(GLUT_CORE_PROFILE ); 
	//glutInitContextProfile( GLUT_COMPATIBILITY_PROFILE); //needed for glutBitmapCharacter
	glutInitContextFlags (GLUT_FORWARD_COMPATIBLE /*| GLUT_DEBUG */); //Can be uses for compatibility with openGL 2.x

	glutInitWindowSize (windowSize.x, windowSize.y); 
	glutInitWindowPosition (100, 100);
	glutCreateWindow ("OpenGL 4.0 ABuffer Sample by Cyril Crassin 2010");
	checkGLError ("glutCreateWindow");
	
	//Init glew
	glewExperimental=GL_TRUE;
	glewInit();
  	checkGLError ("glewInit");

	//Display GL info
	dumpInfo ();

	//Init everything
	init ();

	std::cout<<"\n"	<<"Keys: \n\t'a' Enable/Disable A-Buffer\n"
					<<"\t's' Enable/Disable fragments sorting. Disable= closest fragment kept during resolve.\n"
					<<"\t'g' Swith between Alpha-Blending and Gelly resolve modes.\n"
					<<"\t'c' Enable/Disable alpha correction when in Alpha-Blending mode.\n"
					<<"\t't' Swith between using textures or global memory for A-Buffer storage.\n"
					<<"\t'1'-'4' Change mesh.\n\n";
					

	glutDisplayFunc(display); 
	glutReshapeFunc(reshape);
	glutKeyboardFunc (keyboard);
	glutMotionFunc	 ( mouse_motion );
	glutMouseFunc	 ( mouse_click );
	glutIdleFunc	 ( display );

	checkGLError ("main");
	glutMainLoop();

	return 0;
}


//Writing a vector to a raw file
template<class T>
void writeStdVector(const std::string &fileName, std::vector<T> &vec){
    std::ofstream dataOut( fileName.c_str(), std::ios::out | std::ios::binary);
    dataOut.write((const char *)&(vec[0]), vec.size()*sizeof(T));
    dataOut.close();
}
//Reading a vector from a raw file
template<class T>
void readStdVector(const std::string &fileName, std::vector<T> &vec){
    struct stat results;
    if (stat(fileName.c_str(), &results) == 0){
        uint numElem=results.st_size/sizeof(T);

        vec.resize(numElem);

        std::ifstream dataIn( fileName.c_str(), std::ios::in | std::ios::binary);
        dataIn.read((char *)&(vec[0]), vec.size()*sizeof(T));
        dataIn.close();
    }else{
        std::cout<<"Unable to open file: "<<fileName<<"\n";
    }
}

//Mesh loading from a formated file: not used by default to remove the dependency with assimp
//->Link with assimp (http://assimp.sourceforge.net/) to use it.
#if LOAD_RAW_MESH==0

//Assimp
#include <assimp.hpp>      // C++ importer interface
#include <aiScene.h>       // Outptu data structure
#include <aiPostProcess.h> // Post processing flags
#pragma comment(lib, "assimp.lib")

 void loadMeshFile(const std::string &meshFileName){
	std::cout<<"Loading "<<meshFileName<<" ... \n";

    const aiScene* aiscene;
    aiscene=NULL;

    // Create an instance of the Importer class
    Assimp::Importer importer;

    // And have it read the given file with some example postprocessing
    // Usually - if speed is not the most important aspect for you - you'll
    // propably to request more postprocessing than we do in this example.
    /*const aiScene* scene = importer.ReadFile( meshFileName,
        aiProcess_CalcTangentSpace       |
        aiProcess_Triangulate            |
        aiProcess_JoinIdenticalVertices  |
        aiProcess_SortByPType);*/

    aiscene = importer.ReadFile( meshFileName, aiProcess_GenSmoothNormals );

    // If the import failed, report it
    if( !aiscene)    {
        std::cout<< importer.GetErrorString();
        return ;
    }

    // Now we can access the file's contents.
    //DoTheSceneProcessing( scene);

    //std::cout<<aiscene->mRootNode->mNumMeshes<<" !\n";

    uint previousNumVertex=meshVertexPositionList.size();

    aiMesh* aimesh  = aiscene->mMeshes[0];

	bool hasColor=aimesh->GetNumColorChannels();

	//Compu BBox
	float bboxMinX=1000000.0f;float bboxMinY=1000000.0f;float bboxMinZ=1000000.0f;
	float bboxMaxX=-1000000.0f;float bboxMaxY=-1000000.0f;float bboxMaxZ=-1000000.0f;
	for(uint v=0; v < aimesh->mNumVertices; ++v){
		float valX=aimesh->mVertices[v].x;
		float valY=aimesh->mVertices[v].y;
		float valZ=aimesh->mVertices[v].z;
		if(valX<bboxMinX)
			bboxMinX=valX;
		if(valX>bboxMaxX)
			bboxMaxX=valX;

		if(valY<bboxMinY)
			bboxMinY=valY;
		if(valY>bboxMaxY)
			bboxMaxY=valY;

		if(valZ<bboxMinZ)
			bboxMinZ=valZ;
		if(valZ>bboxMaxZ)
			bboxMaxZ=valZ;
	}

	float sizeX=(bboxMaxX-bboxMinX);
	float sizeY=(bboxMaxY-bboxMinY);
	float sizeZ=(bboxMaxZ-bboxMinZ);
	float maxSize=max(sizeX, max(sizeY, sizeZ));

    for(uint v=0; v < aimesh->mNumVertices; ++v){

        meshVertexPositionList.push_back( (aimesh->mVertices[v].x-bboxMinX)/(maxSize) );
		meshVertexPositionList.push_back( (aimesh->mVertices[v].y-bboxMinY)/(maxSize) );
		meshVertexPositionList.push_back( (aimesh->mVertices[v].z-bboxMinZ)/(maxSize) );
		//meshVertexPositionList.push_back( 1.0f );

		if(hasColor){
			meshVertexColorList.push_back( aimesh->mColors[0][v].r );
			meshVertexColorList.push_back( aimesh->mColors[0][v].g );
			meshVertexColorList.push_back( aimesh->mColors[0][v].b );
			meshVertexColorList.push_back( aimesh->mColors[0][v].a );
		}else{
			meshVertexColorList.push_back( 0.9f );
			meshVertexColorList.push_back( 0.9f );
			meshVertexColorList.push_back( 0.9f );
			meshVertexColorList.push_back( 1.0f );
		}
       

		meshVertexNormalList.push_back( aimesh->mNormals[v].x );
		meshVertexNormalList.push_back( aimesh->mNormals[v].y );
		meshVertexNormalList.push_back( aimesh->mNormals[v].z );
		//meshVertexNormalList.push_back( 1.0f );

	}
    //

    for(uint t=0; t < aimesh->mNumFaces; ++t){
        meshTriangleList.push_back( aimesh->mFaces[t].mIndices[0] );
		meshTriangleList.push_back( aimesh->mFaces[t].mIndices[1] );
		meshTriangleList.push_back( aimesh->mFaces[t].mIndices[2] );
    }

	std::cout<<meshFileName<<" Loaded \n";
}
#endif