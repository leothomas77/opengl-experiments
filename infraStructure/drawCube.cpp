#include <iostream>
#include <cstdio>
#include <cstdlib>

#include <glm/vec3.hpp> 
#include <glm/vec4.hpp> 
#include <glm/mat4x4.hpp> 
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp> 

#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include "initShaders.h"

#ifndef BUFFER_OFFSET 
	#define BUFFER_OFFSET(i) ((char *)NULL + (i))
#endif
					
using namespace std;

GLuint 	shader;

GLuint 	axisVBO[3];
GLuint 	cubeVBO[2];

int		winWidth 	= 800,
		winHeight	= 600;
			
bool	drawRef		= true,
		drawObj		= true;

float 	angleX 		= 	0.0f,
		angleY		= 	0.0f,
		angleZ		=	0.0f;

double 	last;
	
/// ***********************************************************************
/// **
/// ***********************************************************************

void createCube() {
		
// cube ///////////////////////////////////////////////////////////////////////
//    v6----- v5
//   /|      /|
//  v1------v0|
//  | |     | |
//  | |v7---|-|v4
//  |/      |/
//  v2------v3

// vertex coords array for glDrawArrays() =====================================
// A cube has 6 sides and each side has 2 triangles, therefore, a cube consists
// of 36 vertices (6 sides * 2 tris * 3 vertices = 36 vertices). And, each
// vertex is 3 components (x,y,z) of floats, therefore, the size of vertex
// array is 108 floats (36 * 3 = 108).
GLfloat vertices[]  = { 1, 1, 1,  -1, 1, 1,  -1,-1, 1,      // v0-v1-v2 (front)
                       -1,-1, 1,   1,-1, 1,   1, 1, 1,      // v2-v3-v0

                        1, 1, 1,   1,-1, 1,   1,-1,-1,      // v0-v3-v4 (right)
                        1,-1,-1,   1, 1,-1,   1, 1, 1,      // v4-v5-v0

                        1, 1, 1,   1, 1,-1,  -1, 1,-1,      // v0-v5-v6 (top)
                       -1, 1,-1,  -1, 1, 1,   1, 1, 1,      // v6-v1-v0

                       -1, 1, 1,  -1, 1,-1,  -1,-1,-1,      // v1-v6-v7 (left)
                       -1,-1,-1,  -1,-1, 1,  -1, 1, 1,      // v7-v2-v1

                       -1,-1,-1,   1,-1,-1,   1,-1, 1,      // v7-v4-v3 (bottom)
                        1,-1, 1,  -1,-1, 1,  -1,-1,-1,      // v3-v2-v7

                        1,-1,-1,  -1,-1,-1,  -1, 1,-1,      // v4-v7-v6 (back)
                       -1, 1,-1,   1, 1,-1,   1,-1,-1 };    // v6-v5-v4


// color array
GLfloat colors[]    = { 1, 1, 1,   1, 1, 0,   1, 0, 0,      // v0-v1-v2 (front)
                        1, 0, 0,   1, 0, 1,   1, 1, 1,      // v2-v3-v0

                        1, 1, 1,   1, 0, 1,   0, 0, 1,      // v0-v3-v4 (right)
                        0, 0, 1,   0, 1, 1,   1, 1, 1,      // v4-v5-v0

                        1, 1, 1,   0, 1, 1,   0, 1, 0,      // v0-v5-v6 (top)
                        0, 1, 0,   1, 1, 0,   1, 1, 1,      // v6-v1-v0

                        1, 1, 0,   0, 1, 0,   0, 0, 0,      // v1-v6-v7 (left)
                        0, 0, 0,   1, 0, 0,   1, 1, 0,      // v7-v2-v1

                        0, 0, 0,   0, 0, 1,   1, 0, 1,      // v7-v4-v3 (bottom)
                        1, 0, 1,   1, 0, 0,   0, 0, 0,      // v3-v2-v7

                        0, 0, 1,   0, 0, 0,   0, 1, 0,      // v4-v7-v6 (back)
                        0, 1, 0,   0, 1, 1,   0, 0, 1 };    // v6-v5-v4


	glGenBuffers(2, cubeVBO);

	glBindBuffer(	GL_ARRAY_BUFFER, cubeVBO[0]);

	glBufferData(	GL_ARRAY_BUFFER, sizeof(vertices), 
					vertices, GL_STATIC_DRAW);

	glBindBuffer(	GL_ARRAY_BUFFER, cubeVBO[1]);

	glBufferData(	GL_ARRAY_BUFFER, sizeof(colors), 
					colors, GL_STATIC_DRAW);
	
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

void drawCube() {

int attrV, attrC; 
	
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO[0]); 		
	attrV = glGetAttribLocation(shader, "aPosition");
	glVertexAttribPointer(attrV, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attrV);

	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO[1]); 		
	attrC = glGetAttribLocation(shader, "aColor");
	glVertexAttribPointer(attrC, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attrC);

	glDrawArrays(GL_TRIANGLES, 0, 36); 

	glDisableVertexAttribArray(attrV);
	glDisableVertexAttribArray(attrC);

	glBindBuffer(GL_ARRAY_BUFFER, 0); 
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

void display(void) { 

	angleY += 0.01;
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

	glm::mat4 P 	= glm::perspective( 70.0, 1.0, 0.01, 20.0);
	glm::mat4 V 	= glm::lookAt(	glm::vec3(6.0, 6.0, 6.0),
									glm::vec3(0.0, 0.0, 0.0), 
									glm::vec3(0.0, 1.0, 0.0) );
	glm::mat4 M 	= glm::mat4(1.0);

	M = glm::rotate( M, angleY, glm::vec3(0.0, 1.0, 0.0));

	glm::mat4 MVP 	=  P * V * M;

   	glUseProgram(shader);
	int loc = glGetUniformLocation( shader, "uMVP" );
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(MVP));
	
    if (drawRef) 
    	drawAxis();

    if (drawObj) 
    	drawCube();
   	
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
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

void initShaders(void) {

    // Load shaders and use the resulting shader program
    shader = InitShader( "basicShader.vert", "basicShader.frag" );
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

    GLFWwindow* window;

    window = initGLFW(argv[0], winWidth, winHeight);

    initGL(window);
	initShaders();

	createAxis();
	createCube();

    GLFW_MainLoop(window);

    glfwDestroyWindow(window);
    glfwTerminate();

    exit(EXIT_SUCCESS);
	}
    
   
