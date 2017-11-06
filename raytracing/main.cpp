#include <iostream>
#include <vector>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>


#include <glm/vec3.hpp>
#include <glm/vec4.hpp> 
#include <glm/mat4x4.hpp> 
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp> 

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "objetos.h"
#include "globals.h"
#include "raycasting.h"

using namespace std;
using namespace glm;

/// ***********************************************************************
void criarObjetos() {
	cout << "Criando objetos da cena" << endl;
	
	PontoDeLuz luz1;
	luz1.posicao = vec3(8.0f, 10.0f, 10.0f);
	luz1.corRGB = vec3(1.0f, 1.0f, 1.0f);
	luz1.estado = DESLIGADA;

	PontoDeLuz luz2;
	luz2.posicao = vec3(-8.0f, 10.0f, 10.0f);
	luz2.corRGB = vec3(1.0f, 1.0f, 1.0f);
	luz2.estado = LIGADA;	
	
	pontosDeLuz.push_back(luz1);
	pontosDeLuz.push_back(luz2);

	Esfera *esfera1 = new Esfera(3.0, vec3(0.0, 0.0, -10.0), vec3(0.8, 0.8, 0.8));//centro
	Esfera *esfera2 = new Esfera(3.0, vec3(10.0, 0.0,-20.0), vec3(0.0, 1.0, 0.0));//leste
	Esfera *esfera3 = new Esfera(3.0, vec3(-10.0, 0.0, -20.0), vec3(0.0, 1.0, 1.0));//oeste
	Esfera *esfera4 = new Esfera(3.0, vec3(0.0, 10.0, -20.0), vec3(1.0, 1.0, 0.0));//norte
	Esfera *esfera5 = new Esfera(3.0, vec3(5.0, 0.0, 10.0), vec3(0.8, 0.8, 0.8));//sudoeste	
	Esfera *esfera6 = new Esfera(3.0, vec3(0.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0));//sul	

	Plano 	*plano1 = new Plano(vec3(0.0, -13.0, 0.0), vec3(0.0, 1.0, 0.0), 13.0);//chao
	
	plano1->superficie.difusaRGB = vec3(1.0, 1.0, 1.0);//chao
		
	esfera1->superficie.tipoSuperficie = reflexiva;
	esfera5->superficie.tipoSuperficie = refrataria;
	
	objetos.push_back(esfera1);
	objetos.push_back(esfera2);
	objetos.push_back(esfera3);
	objetos.push_back(esfera4);
	objetos.push_back(esfera5);
	objetos.push_back(esfera6);

	objetos.push_back(plano1);
	
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


void exibirFPS(double now, double &lastFPS, double &intervaloFPS, unsigned &contFPS, GLFWwindow* window) {
	if (intervaloFPS > 1.0f) {// Passou 1 segundo
		lastFPS = now;
		char titulo [50];
		titulo [49] = '\0';
		snprintf (titulo, 49, "%s - [FPS: %3.0u]", RT_APP, contFPS );

		glfwSetWindowTitle(window, titulo);
			  
		contFPS = 0;
		intervaloFPS = 0;
	}
}

/*	Rotacao eixo Y
| cos θ    0   sin θ| |x|   | x cos θ + z sin θ|   |x'|
|   0      1       0| |y| = |         y        | = |y'|
|−sin θ    0   cos θ| |z|   |−x sin θ + z cos θ|   |z'|
*/
vec3 rotacaoY(vec3 ponto, float anguloY) {
	return vec3(ponto.x * cos(anguloY) + ponto.z * sin(anguloY), 
				ponto.y, 
	 			-ponto.x * sin(anguloY) + ponto.z * cos(anguloY));;		
}

void display(GLFWwindow* window) {
	anguloY += PASSO_CAMERA;

	mat4 model = mat4(1.0f);
	model = translate(mat4(1.0f), vec3(0.0f, 0.0f, 1.0f));

	//Inicializa a piramide de projecao e o viewport
	mat4 projection = frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);
    vec4 viewport(0.0f, 0.0f, float(winHeight), float(winWidth));
	vector<vec3> cores;
	vec3 origemRaio = vec3(0.0f, 0.0f, 20.0f);	//posiciona o ponto da camera
	//Rotaciona a camera em Y
	vec3 origemRaioR = rotacaoY(origemRaio, anguloY);
	/*
	vec3 lookAt 	= vec3(0.0f, 0.0f, 0.0f); //ponto da camera
	vec3 up			= vec3(0.0f, 1.0f, 0.0f); //up da camera
	mat4 view  		= glm::lookAt(origemRaioR, lookAt, up); //matriz do view
	mat4 viewRotated	= rotate(view, anguloY, vec3(0.0f, 1.0f, 0.0f));
	*/
	for (unsigned int y = 0; y < winHeight ; y++) {
		for (unsigned int x = 0; x < winWidth; x++) {
			vec3 posicaoTela = vec3(x, y, 0);
			//vec3 posicaoTela = vec3(0.0, 0.5, 0);
			
			vec3 posicaoMundo = glm::unProject(posicaoTela, model, projection, viewport);
			//vec3 posicaoMundo = vec3(0.0f, 0.0f, 0.0f);
			vec3 direcaoRaio = normalize(vec3(posicaoMundo.x, posicaoMundo.y, -1) - vec3(0));
			
			vec3 direcaoRaioR = rotacaoY(direcaoRaio, anguloY); //rotaciona a direcao do raio

			vec3 cor = tracarRaio(origemRaioR, direcaoRaioR, objetos, pontosDeLuz, 0);
			cores.push_back(cor);
		}
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	alocarBuffer(cores);
	desenharPixels(vbo);
	cores.clear();
	// if (DEBUG) {
	// 	glfwSetWindowShouldClose(window, true); //habilitar caso queira forcar o encerramento do programa
	// } 
	
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
	aspect = winWidth / winHeight;
	
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
			case GLFW_KEY_LEFT		: 	moveu = RT_LEFT;
										break;
			case GLFW_KEY_RIGHT		: 	moveu = RT_RIGHT;
										break;
			case GLFW_KEY_UP		: 	moveu = RT_UP;
										break;
			case GLFW_KEY_DOWN		: 	moveu = RT_DOWN;
										break;
			case 'a'				: 	
			case 'A'				: 	moveu = RT_Y_HORARIO;
										break;
			case 'D'				: 	
			case 'd'				: 	moveu = RT_Y_ANTI_HORARIO;
										break;
			case 'w'				: 	
			case 'W'				: 	moveu = RT_X_HORARIO;
										break;
			case 'X'				: 	
			case 'x'				: 	moveu = RT_X_ANTI_HORARIO;
										break;
			case 'l'				: 	
			case 'L'				: 	estadoLuz = obterEstadoLuz(pontosDeLuz);
										mudarEstadoLuz(estadoLuz, pontosDeLuz);
			break;
			case '0'				: 	indiceObjeto = 0;
			break;
			case '1'				: 	indiceObjeto = 1;
			break;
			case '2'				: 	indiceObjeto = 2;
			break;
			case '3'				: 	indiceObjeto = 3;
			break;
			case '4'				: 	indiceObjeto = 4;
			break;
			case '5'				: 	indiceObjeto = 5;
			break;
			case '6'				: 	indiceObjeto = 6;
			break;
			case '7'				: 	indiceObjeto = 7;
			break;
			case '8'				: 	indiceObjeto = 8;
			break;
			case '9'				: 	indiceObjeto = 9;
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
		double intervaloFPS = now - lastFPS;

		ObjetoImplicito* objetoSelecionado = NULL;
		if (indiceObjeto <= objetos.size()) {
			objetoSelecionado = objetos.at(indiceObjeto);
		}
	

   		if (ellapsed > 1.0f / 60.0f) {//intervalo de atualizacao da tela
			last = now;
			
			display(window);
			glfwSwapBuffers(window);
			if (moveu != RT_STOP && objetoSelecionado != NULL) {
				objetoSelecionado->mover(moveu, ellapsed);
			}
			moveu = RT_STOP;
			contFPS++;
			
		}

		exibirFPS(now, lastFPS, intervaloFPS, contFPS, window);
		
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

    window = initGLFW(RT_APP, winWidth, winHeight);

    initGL(window);

	criarObjetos();
	
	GLFW_MainLoop(window);
		
	glfwDestroyWindow(window);
    glfwTerminate();

    exit(EXIT_SUCCESS);
}