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

/* ---------------------------------------------------------------------------- */
void get_bounding_box_for_node	(	const struct aiNode* nd,
									aiVector3D* min,
									aiVector3D* max,
									aiMatrix4x4* trafo
								){
	aiMatrix4x4 prev;
	unsigned int n = 0, t;

	prev = *trafo;
	aiMultiplyMatrix4(trafo,&nd->mTransformation);

	for (; n < nd->mNumMeshes; ++n) {
		const aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];
		for (t = 0; t < mesh->mNumVertices; ++t) {

			aiVector3D tmp = mesh->mVertices[t];
			aiTransformVecByMatrix4(&tmp,trafo);

			min->x = MIN(min->x,tmp.x);
			min->y = MIN(min->y,tmp.y);
			min->z = MIN(min->z,tmp.z);

			max->x = MAX(max->x,tmp.x);
			max->y = MAX(max->y,tmp.y);
			max->z = MAX(max->z,tmp.z);
		}
	}

	for (n = 0; n < nd->mNumChildren; ++n) {
		get_bounding_box_for_node(nd->mChildren[n],min,max,trafo);
	}
	*trafo = prev;
}

/* ---------------------------------------------------------------------------- */
void color4_to_float4(const aiColor4D *c, float f[4])
{
	f[0] = c->r;
	f[1] = c->g;
	f[2] = c->b;
	f[3] = c->a;
}

/* ---------------------------------------------------------------------------- */
void set_float4(float f[4], float a, float b, float c, float d)
{
	f[0] = a;
	f[1] = b;
	f[2] = c;
	f[3] = d;
}

/// ***********************************************************************
/// **
/// ***********************************************************************

int traverseScene(	const aiScene *sc, const aiNode* nd) {

	int totVertices = 0;

	/* draw all meshes assigned to this node */
	for (unsigned int n = 0; n < nd->mNumMeshes; ++n) {
		const aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];

		for (unsigned int t = 0; t < mesh->mNumFaces; ++t) {
			const aiFace* face = &mesh->mFaces[t];

			for(unsigned int i = 0; i < face->mNumIndices; i++) {
				int index = face->mIndices[i];
//				if(mesh->mColors[0] != NULL) {
					vboColors.push_back(0.5);
					vboColors.push_back(0.5);
					vboColors.push_back(1.0);
					vboColors.push_back(1.0);
//					}
				if(mesh->mNormals != NULL) {
					vboNormals.push_back(mesh->mNormals[index].x);
					vboNormals.push_back(mesh->mNormals[index].y);
					vboNormals.push_back(mesh->mNormals[index].z);
					}
				vboVertices.push_back(mesh->mVertices[index].x);
				vboVertices.push_back(mesh->mVertices[index].y);
				vboVertices.push_back(mesh->mVertices[index].z);
				totVertices++;
				}
			}
		}

	for (unsigned int n = 0; n < nd->mNumChildren; ++n) {
		totVertices += traverseScene(sc, nd->mChildren[n]);
		}
	return totVertices;
}

/// ***********************************************************************
void criarObjetos() {
	cout << "Criando objetos " << endl;	
    
	Esfera *esfera1 = new Esfera(0.5, vec3(0.0, 0.0, -5.0), vec3(1.0, 0.0, 0.0));
	Esfera *esfera2 = new Esfera(0.5, vec3(3.0, 0.0, -5.0), vec3(0.0, 1.0, 0.0));
	Esfera *esfera3 = new Esfera(0.5, vec3(-3.0, 0.0, -5.0), vec3(0.0, 0.0, 1.0));
	Esfera *esfera4 = new Esfera(0.5, vec3(1.5, 0.0, -5.0), vec3(1.0, 1.0, 0.0));
	Esfera *esfera5 = new Esfera(0.8, vec3(-1.5, 0.0, -5.0), vec3(0.0, 1.0, 1.0));

	objetos.push_back(esfera1);
	objetos.push_back(esfera2);
	objetos.push_back(esfera5);
	objetos.push_back(esfera3);
	objetos.push_back(esfera4);

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

void createPoints() {
	GLfloat vertices[]  = 	{ 	
		1.0, -2.0, 1.0,
		1.0, 1.0, 1.0,
		-1.0, -3.0, 2.0,
		-1.0, -2.0, -1.0
	}; 	

	GLfloat colors[]  = { 	
		1.0, 1.0, 1.0, 1.0,
		1.0, 0.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		0.0, 0.0, 1.0, 1.0
	}; 

	GLfloat verticesAux[]  = 	{ 	
		1.0, -2.0, 1.0,
		1.0, 1.0, 1.0,
		-1.0, -3.0, 2.0,
		-1.0, -2.0, -1.0
	}; 	

	glGenBuffers(3, pointsVBO);
	
	glBindBuffer(	GL_ARRAY_BUFFER, pointsVBO[0]);

	glBufferData(	GL_ARRAY_BUFFER, 4*3*sizeof(float), 
					vertices, GL_STATIC_DRAW);

	glBindBuffer(	GL_ARRAY_BUFFER, pointsVBO[1]);

	glBufferData(	GL_ARRAY_BUFFER, 4*4*sizeof(float), 
					colors, GL_STATIC_DRAW);

	glBindBuffer(	GL_ELEMENT_ARRAY_BUFFER, pointsVBO[2]);

	glBufferData(	GL_ELEMENT_ARRAY_BUFFER, 4*3*sizeof(float), 
					verticesAux, GL_STATIC_DRAW);

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

void drawPoints() {
	glBindBuffer(GL_ARRAY_BUFFER, pointsVBO[2]);
	glDrawArrays(GL_POINTS, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0); 
	
	
   // glBindVertexArray(pointsVBO[2]);
   // glDrawArrays(GL_POINTS, 0, 4);
   // glFlush(); 
	/*
	int attrV, attrC; 
	
	glBindBuffer(GL_ARRAY_BUFFER, pointsVBO[0]); 		
	attrV = glGetAttribLocation(shader, "aPosition");
	glVertexAttribPointer(attrV, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attrV);

	glBindBuffer(GL_ARRAY_BUFFER, pointsVBO[1]); 		
	attrC = glGetAttribLocation(shader, "aColor");
	glVertexAttribPointer(attrC, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attrC);

	glPointSize(10.0f);
	glColor3f(1.0f, 0.0f, 0.0f);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pointsVBO[2]);
	glDrawElements(GL_POINTS, 4, GL_UNSIGNED_INT, 0);
	

	//glBindBuffer(GL_ARRAY_BUFFER, pointsVBO[0]);
	//glDrawElements(GL_POINTS, 4, GL_UNSIGNED_INT, BUFFER_OFFSET(0));

	glDisableVertexAttribArray(attrV);
	glDisableVertexAttribArray(attrC);

	glBindBuffer(GL_ARRAY_BUFFER, 0); 
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); 	
	*/
}
		
/// ***********************************************************************
/// **
/// ***********************************************************************

void drawMesh() {

int attrV, attrC, attrN; 
	
	glBindBuffer(GL_ARRAY_BUFFER, meshVBO[0]); 		
	attrV = glGetAttribLocation(shader, "aPosition");
	glVertexAttribPointer(attrV, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attrV);

	glBindBuffer(GL_ARRAY_BUFFER, meshVBO[1]); 		
	attrC = glGetAttribLocation(shader, "aColor");
	glVertexAttribPointer(attrC, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attrC);

	glBindBuffer(GL_ARRAY_BUFFER, meshVBO[2]); 		
	attrN = glGetAttribLocation(shader, "aNormal");
	glVertexAttribPointer(attrN, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attrN);

	glDrawArrays(GL_TRIANGLES, 0, meshSize); 

	glDisableVertexAttribArray(attrV);
	glDisableVertexAttribArray(attrC);
	glDisableVertexAttribArray(attrN);

	glBindBuffer(GL_ARRAY_BUFFER, 0); 
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); 
}

void criarVBOs() {
	cout << "Criando VBOs" << endl;
	glGenBuffers(3, meshVBO);
	
	glBindBuffer(	GL_ARRAY_BUFFER, meshVBO[0]);

	glBufferData(	GL_ARRAY_BUFFER, vboVertices.size()*sizeof(float), 
					vboVertices.data(), GL_STATIC_DRAW);

	glBindBuffer(	GL_ARRAY_BUFFER, meshVBO[1]);

	glBufferData(	GL_ARRAY_BUFFER, vboColors.size()*sizeof(float), 
					vboColors.data(), GL_STATIC_DRAW);

	if (vboNormals.size() > 0) {
		glBindBuffer(	GL_ARRAY_BUFFER, meshVBO[2]);

		glBufferData(	GL_ARRAY_BUFFER, vboNormals.size()*sizeof(float), 
						vboNormals.data(), GL_STATIC_DRAW);
	}	

	meshSize = vboVertices.size() / 3;
	cout << "			#Quantidade de vertices= " << meshSize << endl;	
}

void alocarBuffer(vector<GLfloat> vertices, vector<GLfloat> pixels) {
	cout << "Gerando buffers" << endl;
	glGenBuffers(3, vbo);
	cout << "Selecionando shader" << endl;
	glUseProgram(shader);
	
	cout << "Alocando buffer para vertices" << endl;
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

	cout << "Alocando buffer para cores" << endl;
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, pixels.size() * sizeof(GLfloat), pixels.data(), GL_STATIC_DRAW);

	//cout << "Alocando buffer para elementos" << endl;
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[2]);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, raw.size() / 2, &raw, GL_STATIC_DRAW);

	//TODO Terceiro buffer para armazenar as normais
}


void desenharPixels(vector<GLfloat> &vertices, vector<GLfloat> &pixels) {
	
	int attrV, attrC, attrN;
	cout << "Input atributo aPosition" << endl;
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); 		
	attrV = glGetAttribLocation(shader, "aPosition");
	glVertexAttribPointer(attrV, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attrV);
	
	cout << "Input atributo aColor" << endl;
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]); 		
	attrC = glGetAttribLocation(shader, "aColor");
	glVertexAttribPointer(attrC, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attrC);

	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[0]);
	cout << "Desenhando " << pixels.size() / 4 << " pontos" << endl;
	glDrawArrays(GL_POINT, 0, pixels.size() / 4);
	

	//glDrawElements(GL_POINT, WIDTH * HEIGHT, GL_UNSIGNED_INT, BUFFER_OFFSET(0));


	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[0]);
	//glDrawPixels(WIDTH , HEIGHT, GL_RGBA, GL_UNSIGNED_INT, raw.data());

	//glBindVertexArray(vbo[0]);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[2]);
    //glDrawPixels( WIDTH, HEIGHT, GL_RGBA, GL_FLOAT, pixels );
	
	//glDrawElements(GL_POINT, WIDTH * HEIGHT, GL_UNSIGNED_INT, BUFFER_OFFSET(0));

	glDisableVertexAttribArray(attrV);
	glDisableVertexAttribArray(attrC);
		
	glBindBuffer(GL_ARRAY_BUFFER, 0); 
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); 
}


void display(void) { 
	
		angleY += 0.01;
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
		
		mat4 P 	= perspective( 70.0, 1.0, 0.01, 20.0);
		mat4 V 	= lookAt(	vec3(6.0, 6.0, 6.0),
										vec3(0.0, 0.0, 0.0), 
										vec3(0.0, 1.0, 0.0) );
		mat4 M 	= mat4(1.0);
	
		M = rotate( M, angleY, vec3(0.0, 1.0, 0.0));
	
		mat4 MVP 	=  P * V * M;
	
		glUseProgram(shader);
		int loc = glGetUniformLocation( shader, "uMVP" );
		glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(MVP));
		
		//if (drawRef) 
			drawAxis();
			drawPoints();
	/*		glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(-50, 50, -50, 50, -1, 1);
		
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
		
			// draw
			glColor3ub( 255, 255, 255 );
			glEnableClientState( GL_VERTEX_ARRAY );
			glEnableClientState( GL_COLOR_ARRAY );
			glVertexPointer( 2, GL_FLOAT, sizeof(Point), &points[0].x );
			glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof(Point), &points[0].r );
			glPointSize( 3.0 );
			glDrawArrays( GL_POINTS, 0, points.size() );
			glDisableClientState( GL_VERTEX_ARRAY );
			glDisableClientState( GL_COLOR_ARRAY );
		
			glFlush();
	*/
		//if (drawObj) 
			//drawCube();
		   
		   glUseProgram(0);	
	}

	/*
void display2(unsigned int *raw, GLfloat *pixels) {
	glColor3ub( 255, 255, 255 );
    glEnableClientState( GL_VERTEX_ARRAY );
    glEnableClientState( GL_COLOR_ARRAY );
    glVertexPointer( 2, GL_UNSIGNED_INT, 2 * WIDTH * HEIGHT * sizeof(unsigned int), raw );
    glColorPointer( 4, GL_FLOAT, sizeof(GLfloat), pixels );
    glPointSize( 3.0 );
    //glDrawArrays( GL_POINTS, 0, WIDTH * HEIGHT );
    glDisableClientState( GL_VERTEX_ARRAY );
    glDisableClientState( GL_COLOR_ARRAY );

    glFlush();
}
*/
/*void display(void)
{
    glClear (GL_COLOR_BUFFER_BIT);
    GLuint abuffer;
    glGenVertexArrays(1, &abuffer);
    glBindVertexArray(abuffer);
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    Vertices = {{0.3f, 0.3f, 0.5f}, {0.6f, 0.6f, 0.5f}, {0.6f, 0.3f, 0.5f}, {0.3f, 0.6f, 0.5f}};
    glBufferData(GL_ARRAY_BUFFER, Vertices.size() * sizeof(Vertices[0]), &Vertices[0], GL_STATIC_DRAW);
    glVertexPointer(3, GL_FLOAT, 0, &Vertices[0]);
    glDrawArrays(GL_POINTS, 0, Vertices.size());
    glFlush();
    glBegin(GL_POINTS);
    glVertex3f(0.25f, 0.25f, 0.5f);
    glVertex3f(0.25f, 0.75f, 0.5f);
    glVertex3f(0.75f, 0.75f, 0.5f);
    glVertex3f(0.75f, 0.25f, 0.5f);
    glEnd();    
    glFlush();
} */
		
/// ***********************************************************************
/// **
/// ***********************************************************************

void display(GLFWwindow* window) {

	//angleY += 0.02;

	float Max = 5.0; //max(scene_max.x, max(scene_max.y, scene_max.z));

	vec3 lightPos	= vec3(Max, Max, 0.0);
	//vec3 camPos		= vec3(distanciaCamera * Max,  distanciaCamera * Max, distanciaCamera * Max);
	//vec3 lookAt		= vec3(scene_center.x, scene_center.y, scene_center.z);
	//vec3 up			= vec3(0.0, 1.0, 0.0);
	//mat4 ViewMat	= glm::lookAt(camPos, lookAt, up);
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
    float fov = 60, aspectratio = winWidth / float(winHeight);
    float angulo = tan(M_PI * 0.5 * fov / 180.);

	cout << "Percorrendo viewport de " << winWidth * winHeight << " pixels" << endl;	
	double inicio = glfwGetTime(); 
	origemRaio = vec3(0.0f, 0.0f, 0.0f);

	vector<vec3> cores;
	vec3 cor;
	
	cout << "Objetos carregados: " << objetos.size() << endl;	
	
	for (unsigned int y = 0; y < winHeight; y++) {
		for (unsigned int x = 0; x < winWidth; x++) {

			float xMundo = (2 * ((x + 0.5) * invWidth) - 1) * angulo * aspectratio;
			float yMundo = (1 - 2 * ((y + 0.5) * invHeight)) * angulo;
			
			direcaoRaio = normalize(vec3(xMundo, yMundo, -1));
			cor = tracarRaio(origemRaio, direcaoRaio, objetos, vboVertices, vboColors, vboNormals, lightPos);
			cores.push_back(cor);
		}
	}

	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
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

	//drawAxis();
	//drawPoints();

	//alocarBuffer(vboVertices, cores);
	//desenharPixels(vboVertices, cores);

	salvarImagem(window, cores, winWidth, winHeight);
	
	double fim = glfwGetTime();
	double duracao = fim - inicio ;
	int ms = nanoToMili(duracao);
	
	vboColors.clear();
	vboNormals.clear();
	vboVertices.clear();
	cores.clear();	
	
	cout << "Duracao aproximada:  " << ms / 1000 << " segundos " << endl;	
  
	glfwSetWindowShouldClose(window, true);
	
	glUseProgram(0);	
	
}


void shade(vec3 lightPos, vec3 camPos, mat4 MVP, mat4 normalMat, mat4 ModelMat) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader);
		
	int loc = glGetUniformLocation( shader, "uMVP" );
	glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(MVP));

	if ( (shader == shaderGouraud) || ( shader == shaderPhong) ) {
		loc = glGetUniformLocation( shader, "uN" );
		glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(normalMat));
		loc = glGetUniformLocation( shader, "uM" );
		glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(ModelMat));
		loc = glGetUniformLocation( shader, "uLPos" );
		glUniform3fv(loc, 1, value_ptr(lightPos));
		loc = glGetUniformLocation( shader, "uCamPos" );
		glUniform3fv(loc, 1, value_ptr(camPos));
		}
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
    shaderAmbient 	= InitShader( "./shaders/basicShader.vert", 	"./shaders/basicShader.frag" );
 //   shaderGouraud 	= InitShader( "shaders/Gouraud.vert", 		"shaders/Gouraud.frag" );
 //   shaderPhong 	= InitShader( "shaders/Phong.vert", 		"shaders/Phong.frag" );

    shader 			= shaderAmbient;
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
	initShaders();

	createAxis();
	criarObjetos();
	
	GLFW_MainLoop(window);
	
    glfwDestroyWindow(window);
    glfwTerminate();

    exit(EXIT_SUCCESS);
}


