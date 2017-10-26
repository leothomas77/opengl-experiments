#include <iostream>
#include <cfloat>
#include <vector>
#include <limits>
#include <cstdlib>
#include <cstdio>
#include <cmath>

#include <fstream>
#include <vector>
#include <cassert>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp> 
#include <glm/mat4x4.hpp> 
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp> 

/* assimp include files. These three are usually needed. */
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include "initShaders.h"
#include "objetos.h"
#include "globals.h"
#include "raycasting.h"

using namespace std;
using namespace glm;

/// ***********************************************************************
void criarObjetos() {
	cout << "Criando objetos " << endl;	
    
	Esfera *esfera1 = new Esfera(2.0, vec3(0.0, 0.0, -10.0), vec3(1.0, 1.0, 1.0));
	Esfera *esfera2 = new Esfera(2.0, vec3(5.0, 0.0,-20.0), vec3(0.0, 1.0, 0.0));
	Esfera *esfera3 = new Esfera(2.0, vec3(-5.0, 0.0, -20.0), vec3(0.0, 0.0, 1.0));
	Esfera *esfera4 = new Esfera(2.0, vec3(0.0, 0.0, -15.0), vec3(1.0, 1.0, 0.0));
	Esfera *esfera5 = new Esfera(2.0, vec3(0.0, 0.0, -5.0), vec3(0.0, 1.0, 1.0));

	esfera1->superficie.espelhamento = true;

	esfera2->superficie.espelhamento = false;

	esfera3->superficie.espelhamento = false;

	esfera4->superficie.espelhamento = false;

	esfera5->superficie.espelhamento = false;

	objetos.push_back(esfera1);
	objetos.push_back(esfera2);
	objetos.push_back(esfera3);
	objetos.push_back(esfera4);
	objetos.push_back(esfera5);
	
}

void createAxis() {

	GLfloat vertices[]  = 	{ 	
		0.0, 0.0, 0.0,
		5.0, 0.0, 0.0,
		0.0, 5.0, 0.0,
		0.0, 0.0, 5.0
	}; 

	GLuint lines[]  = 	{ 	
		0, 1,
		0, 2,
		0, 3
	}; 

	GLfloat colors[]  = { 	
		1.0, 1.0, 1.0, 1.0,
		1.0, 0.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		0.0, 0.0, 1.0, 1.0
	}; 
	
	glGenBuffers(3, axisVBO);

	glBindBuffer(	GL_ARRAY_BUFFER, axisVBO[0]);

	glBufferData(	GL_ARRAY_BUFFER, 4*3*sizeof(float), 
					vertices, GL_STATIC_DRAW);

	glBindBuffer(	GL_ARRAY_BUFFER, axisVBO[1]);

	glBufferData(	GL_ARRAY_BUFFER, 4*4*sizeof(float), 
					colors, GL_STATIC_DRAW);

	glBindBuffer(	GL_ELEMENT_ARRAY_BUFFER, axisVBO[2]);

	glBufferData(	GL_ELEMENT_ARRAY_BUFFER, 3*2*sizeof(unsigned int), 
					lines, GL_STATIC_DRAW);
	
}
		
/// ***********************************************************************
/// **
/// ***********************************************************************

void drawAxis() {
	
	int attrV, attrC; 
		
		glBindBuffer(GL_ARRAY_BUFFER, axisVBO[0]); 		
		attrV = glGetAttribLocation(shader, "aPosition");
		glVertexAttribPointer(attrV, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(attrV);
	
		glBindBuffer(GL_ARRAY_BUFFER, axisVBO[1]); 		
		attrC = glGetAttribLocation(shader, "aColor");
		glVertexAttribPointer(attrC, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(attrC);
	
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, axisVBO[2]);
		glDrawElements(GL_LINES, 6, GL_UNSIGNED_INT, BUFFER_OFFSET(0));
	
		glDisableVertexAttribArray(attrV);
		glDisableVertexAttribArray(attrC);
	
		glBindBuffer(GL_ARRAY_BUFFER, 0); 
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); 
}



void alocarBuffer(vector<vec3> pixels) {
	//cout << "Gerando buffers" << endl;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, vbo);
	//cout << "Alocando buffer para pixels" << endl;
	glBufferData(GL_PIXEL_UNPACK_BUFFER, pixels.size() * sizeof(vec3), pixels.data(), GL_DYNAMIC_DRAW);
}

void desenharPixels(GLint vbo) {
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, vbo);
	glDrawPixels(winWidth, winHeight, GL_RGB, GL_FLOAT, 0);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void display(GLFWwindow* window) {

	//angleY += 0.02;

	float Max = 30.0; //max(scene_max.x, max(scene_max.y, scene_max.z));
	float distanciaCamera = 1.0;
	vec3 lightPos	= vec3(Max, Max, 0.0);
	vec3 camPos	= vec3(0.0f,  0.0f, distanciaCamera);
	vec3 lookAt		= vec3(scene_center.x, scene_center.y, scene_center.z);
	vec3 up			= vec3(0.0, 1.0, 0.0);
	mat4 ViewMat	= glm::lookAt(camPos, lookAt, up);
	//mat4 ProjMat 	= perspective( 70.0, 1.0, 0.01, 100.0);
	//mat4 ModelMat 	= mat4(1.0);

//Rotacao do modelo (movimentacao da cena)
	// ModelMat = rotate( ModelMat, angleX, vec3(1.0, 0.0, 0.0));
	// ModelMat = rotate( ModelMat, angleY, vec3(0.0, 1.0, 0.0));
	// ModelMat = rotate( ModelMat, angleZ, vec3(0.0, 0.0, 1.0));
//Unifica as 3 matrizes em uma 
	//mat4 MVP 			= ProjMat * ViewMat * ModelMat;
	//mat4 normalMat		= transpose(inverse(ModelMat));

//Inicia o tracado de raios de cada ponto da tela ate o objeto
	
	vec3 origemRaio, direcaoRaio;
	
	float invWidth = 1 / float(winWidth), invHeight = 1 / float(winHeight);
    float fov = 70, aspectratio = winWidth / float(winHeight);
    float angulo = tan(M_PI * 0.5 * fov / 180.);

	//cout << "Percorrendo viewport de " << winWidth * winHeight << " pixels" << endl;	
	double inicio = glfwGetTime(); 
	origemRaio = vec3(0.0f, 5.0f, 10.0f);

	vector<vec3> cores;
	vec3 cor;
	
	//cout << "Objetos carregados: " << objetos.size() << endl;	
	
	for (unsigned int y = winHeight; y > 0 ; y--) {
		for (unsigned int x = 0; x < winWidth; x++) {

			float xMundo = (2 * ((x + 0.5) * invWidth) - 1) * angulo * aspectratio;
			float yMundo = (1 - 2 * ((y + 0.5) * invHeight)) * angulo;
			
			direcaoRaio = normalize(vec3(xMundo, yMundo, -1));
			cor = tracarRaio(origemRaio, direcaoRaio, objetos, vboVertices, vboColors, vboNormals, lightPos, 0);
			cores.push_back(cor);
		}
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	//glUseProgram(shader);

	//mat4 P 	= perspective( 70.0, 1.0, 0.01, 20.0);
	//mat4 V 	= glm::lookAt(	vec3(6.0, 6.0, 6.0),
	//								vec3(0.0, 0.0, 0.0), 
	//								vec3(0.0, 1.0, 0.0) );
	//mat4 M 	= mat4(1.0);

	//M = rotate( M, angleY, vec3(0.0, 1.0, 0.0));

	//mat4 MVP 	=  P * V * M;

	//int loc = glGetUniformLocation( shader, "uMVP" );
	//glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(MVP));

	drawAxis();

	alocarBuffer(cores);
	desenharPixels(vbo);

	//salvarImagem(window, cores, winWidth, winHeight);
	
	//double fim = glfwGetTime();
	//double duracao = fim - inicio ;
	//int ms = nanoToMili(duracao);
	
	//vboColors.clear();
	//vboNormals.clear();
	//vboVertices.clear();
	cores.clear();	
	
	//cout << "Duracao aproximada:  " << ms / 1000 << " segundos " << endl;	
  
	//glfwSetWindowShouldClose(window, true);
	
	//glUseProgram(0);	
	
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

void initGL(GLFWwindow* window) {

	glClearColor(0.0, 0.0, 0.0, 0.0);	
	
	if (glewInit()) {
		cout << "Unable to initialize GLEW ... exiting" << endl;
		exit(EXIT_FAILURE);
	}
		
	cout << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << endl;	
	
	cout << "Opengl Version: " << glGetString(GL_VERSION) << endl;
	cout << "Opengl Vendor : " << glGetString(GL_VENDOR) << endl;
	cout << "Opengl Render : " << glGetString(GL_RENDERER) << endl;
	cout << "Opengl Shading Language Version : " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

	int w, h;
	
	glfwGetFramebufferSize(window, &w, &h);

	glViewport(0, 0, w, h);
	glPointSize(3.0);
	glEnable(GL_DEPTH_TEST);
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

static void error_callback(int error, const char* description) {
	cout << "Error: " << description << endl;
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

static void window_size_callback(GLFWwindow* window, int width, int height) {
    winWidth  = width;
	winHeight = height;    
	glViewport(0, 0, width, height);
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	if (action == GLFW_PRESS)										
		switch (key) {	
			case GLFW_KEY_ESCAPE  	: 	glfwSetWindowShouldClose(window, true);
										break;

			case '.'				: 	glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);								
										break;

			case '-'				: 	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);								
										break;

			case 'F'				:
			case 'f'				: 	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);								
										break;
			case 'a'				:
			case 'A'				: 	shader = shaderAmbient;
										break;
			case 'G'				:
			case 'g'				: 	shader = shaderGouraud;
										break;
			case 'P'				:
			case 'p'				: 	shader = shaderPhong;
										break;
	}

}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

static GLFWwindow* initGLFW(char* nameWin, int w, int h) {

	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
	    exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	GLFWwindow* window = glfwCreateWindow(w, h, nameWin, NULL, NULL);
	if (!window) {
	    glfwTerminate();
	    exit(EXIT_FAILURE);
		}

	glfwSetWindowSizeCallback(window, window_size_callback);

	glfwSetKeyCallback(window, key_callback);

	glfwMakeContextCurrent(window);

	glfwSwapInterval(1);

	last = glfwGetTime();	

	return (window);
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

static void GLFW_MainLoop(GLFWwindow* window) {

   while (!glfwWindowShouldClose(window)) {

   		double now = glfwGetTime(); 
   		double ellapsed = now - last;

   		if (ellapsed > 1.0f / 30.0f) {
	   		last = now;
	        display(window);
	        glfwSwapBuffers(window);
	    	}

        glfwPollEvents();
    	}
}

/* ************************************************************************* */
/* ************************************************************************* */
/* *****                                                               ***** */
/* ************************************************************************* */
/* ************************************************************************* */

int main(int argc, char *argv[]) { 

    GLFWwindow* window;

    window = initGLFW(argv[0], winWidth, winHeight);

    initGL(window);

	createAxis();
	criarObjetos();
	
	GLFW_MainLoop(window);
	
    glfwDestroyWindow(window);
    glfwTerminate();

    exit(EXIT_SUCCESS);
}