#include <iostream>
#include <cstdio>
#include <cstdlib>

#include <glm/vec3.hpp> 
#include <glm/vec4.hpp> 
#include <glm/mat4x4.hpp> 
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp> 

#include <vector>

#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include "initShaders.h"

#ifndef BUFFER_OFFSET 
	#define BUFFER_OFFSET(i) ((char *)NULL + (i))
#endif
					
using namespace std;

GLuint 	shader,
		shaderBase,
		shaderElavation;

GLuint 	axisVBO[3];
GLuint 	terrainVBO[2];

int		winWidth 	= 800,
		winHeight	= 600;
			
bool	drawRef		= true,
		drawObj		= true;

float 	angleY 		= 0.0f,
		deltaRot	= 0.0f,
		deltaT 		= 0.0f,
		dT 			= 0.0f;

double 	last;

int numPtos=0, numTri=0;

/// ***********************************************************************
/// **
/// ***********************************************************************

void createTerrain(int res) {
		
vector<GLfloat> vertices;

vector<GLuint> faces;

numPtos = 0;
numTri = 0;

float delta = 20.0 / (float)res;

	for (GLfloat j = -10.0 ; j <= 10.0 ; j+=delta) 
		for (GLfloat i = -10.0 ; i <= 10.0 ; i+=delta) {
			vertices.push_back(i);
			vertices.push_back(j);

			numPtos++;
			}

	for (GLuint i = 0 ; i < res ; i++)  
		for (GLuint j = 0 ; j < res ; j++) {
			faces.push_back( i*(res+1) 		+ j);		// V0
			faces.push_back( i*(res+1) 		+ (j+1));	// V1
			faces.push_back( (i+1)*(res+1) 	+ j);		// V2

			faces.push_back( i*(res+1) 		+ (j+1));	// V1
			faces.push_back( (i+1)*(res+1) 	+ (j+1));	// V3
			faces.push_back( (i+1)*(res+1) 	+ j);		// V2
			numTri+=2;
		}

	glGenBuffers(	2, terrainVBO);

	glBindBuffer(	GL_ARRAY_BUFFER, terrainVBO[0]);

	glBufferData(	GL_ARRAY_BUFFER, vertices.size()*sizeof(GLfloat), 
					vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(	GL_ELEMENT_ARRAY_BUFFER, terrainVBO[1]);

	glBufferData(	GL_ELEMENT_ARRAY_BUFFER, faces.size()*sizeof(GLuint), 
					faces.data(), GL_STATIC_DRAW);

}


/// ***********************************************************************
/// **
/// ***********************************************************************

void createAxis() {
		
GLfloat vertices[]  = 	{ 	0.0, 0.0, 0.0,
							5.0, 0.0, 0.0,
							0.0, 5.0, 0.0,
							0.0, 0.0, 5.0
						}; 

GLuint lines[]  = 	{ 	0, 1,
						0, 2,
						0, 3
					}; 

GLfloat colors[]  = { 	1.0, 1.0, 1.0, 1.0,
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
		
/// ***********************************************************************
/// **
/// ***********************************************************************

void drawTerrain() {

int attrV, attrC; 
	
	glBindBuffer(GL_ARRAY_BUFFER, terrainVBO[0]); 		
	attrV = glGetAttribLocation(shader, "aPosition");
	glVertexAttribPointer(attrV, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attrV);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainVBO[1]);

	glDrawElements(GL_TRIANGLES, numTri*3, GL_UNSIGNED_INT, BUFFER_OFFSET(0)); 

	glDisableVertexAttribArray(attrV);

	glBindBuffer(GL_ARRAY_BUFFER, 0); 
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); 
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

void display(void) { 

	int loc;

	angleY += deltaRot;
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

	glm::mat4 P 	= glm::perspective( 70.0, 1.0, 0.01, 70.0);
	glm::mat4 V 	= glm::lookAt(	glm::vec3(15.0, 15.0, 0.0),
									glm::vec3(0.0, 0.0, 0.0), 
									glm::vec3(0.0, 1.0, 0.0) );
	glm::mat4 M 	= glm::mat4(1.0);

	M = glm::rotate( M, angleY, glm::vec3(0.0, 1.0, 0.0));

	glm::mat4 MVP 	=  P * V * M; 

   	glUseProgram(shader);

	loc = glGetUniformLocation( shader, "uMVP" );
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(MVP));

	loc = glGetUniformLocation( shader, "uM" );
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(M));

	loc = glGetUniformLocation( shader, "uV" );
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(V));

	loc = glGetUniformLocation( shader, "uP" );
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(P));

	loc = glGetUniformLocation( shader, "T" );

	if (loc != -1)
		glUniform1f(loc, 0.0);
	
    if (drawRef) 
    	drawAxis();

	if (loc != -1)
		glUniform1f(loc, dT);

    if (drawObj) 
    	drawTerrain();
   	
   	glUseProgram(0);	
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

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);								
										
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

void initShaders(void) {

    // Load shaders and use the resulting shader program
    shaderBase 		= InitShader( "planeShader.vert", "planeShader.frag" );
    shaderElavation = InitShader( "terrainShader.vert", "terrainShader.frag" );
    shader = shaderBase;
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
        
	glViewport(0, 0, width, height);

	cout << "Resized window: " << width << " , " << height << endl;
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	if (action == GLFW_PRESS)										
		switch (key) {	
			case GLFW_KEY_ESCAPE  	: 	glfwSetWindowShouldClose(window, true);
										break;
			case 'A'				: 	
			case 'a'				: 	drawRef = !drawRef;
										break;
			case 'O'				: 	
			case 'o'				: 	drawObj = !drawObj;
										break;
			case 'E'				: 	
			case 'e'				: 	shader = shaderElavation;
										break;
			case 'b'				: 	
			case 'B'				: 	shader = shaderBase;
										break;
			case 'r'				: 	
			case 'R'				: 	if (deltaRot == 0.0) 
											deltaRot = 0.01;
										else
											deltaRot = 0.0;
										break;

			case 't'				: 	
			case 'T'				: 	if (deltaT == 0.0) 
											deltaT = 0.01;
										else
											deltaT = 0.0;
										break;

			case '.'				: 	glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);								
										break;

			case '-'				: 	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);								
										break;

			case 'F'				:
			case 'f'				: 	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);								
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
	   		dT += deltaT;
	        display();
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

	int res=10;

	if (argc == 2) 
		res = atoi(argv[1]);

    GLFWwindow* window;

    window = initGLFW(argv[0], winWidth, winHeight);

    initGL(window);
	initShaders();

	createAxis();
	createTerrain(res);

    GLFW_MainLoop(window);

    glfwDestroyWindow(window);
    glfwTerminate();

    exit(EXIT_SUCCESS);
	}
    
   
