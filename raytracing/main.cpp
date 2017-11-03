#include <iostream>
#include <vector>
#include <cstdio>
#include <fstream>
#include <iomanip>


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
	luz1.posicao = vec3(8.0f, 40.0f, 30.0f);
	luz1.estado = DESLIGADA;

	PontoDeLuz luz2;
	luz2.posicao = vec3(-8.0f, 40.0f, 30.0f);
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
	Plano 	*plano2 = new Plano(vec3(0.0, 0.0, -25.0), vec3(0.0, 0.0, 1.0), 25.0);//fundo
	Plano 	*plano3 = new Plano(vec3(0.0, 13.0, 0.0), vec3(0.0, -1.0, 0.0), 13.0);//ceu
	Plano 	*plano4 = new Plano(vec3(-13.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), 13.0);//esq
	Plano 	*plano5 = new Plano(vec3(13.0, 0.0, 0.0), vec3(-1.0, 0.0, 0.0), 13.0);//dir
	

	plano1->superficie.corRGB = vec3(1.0, 1.0, 1.0);//chao
	plano2->superficie.corRGB = vec3(1.0, 1.0, 1.0);//fundo
	plano3->superficie.corRGB = vec3(0.8, 0.5, 0.2);//ceu
	plano4->superficie.corRGB = vec3(1.0, 0.0, 0.0);//esq
	plano5->superficie.corRGB = vec3(1.0, 1.0, 0.0);//dir
		
	esfera1->superficie.tipoSuperficie = reflexiva;
	esfera5->superficie.tipoSuperficie = refrataria;

	//plano1->superficie.tipoSuperficie = solida;
	
	objetos.push_back(esfera1);
//	objetos.push_back(esfera2);
//	objetos.push_back(esfera3);
//	objetos.push_back(esfera4);
	objetos.push_back(esfera5);
	objetos.push_back(esfera6);

	objetos.push_back(plano1);
//	objetos.push_back(plano2);
//	objetos.push_back(plano3);
//	objetos.push_back(plano4);
//	objetos.push_back(plano5);

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

float calcularFPS() {
	//cout << "inicio " << inicioPrograma << endl; 
	//cout << "fim " << fimPrograma << endl; 
	//cout << "fps " << contFPS << endl; 
	return (contFPS / (fimPrograma - inicioPrograma));
}

void exibirFPS() {
	cout << fixed << setprecision(3) << "FPS calculado: " << calcularFPS() << endl;
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
	/*	Rotacao eixo Y
	| cos θ    0   sin θ| |x|   | x cos θ + z sin θ|   |x'|
    |   0      1       0| |y| = |         y        | = |y'|
    |−sin θ    0   cos θ| |z|   |−x sin θ + z cos θ|   |z'|
	*/
	//Rotaciona a camera em Y
	vec3 origemRaioR = vec3(origemRaio.x * cos(anguloY) + origemRaio.z * sin(anguloY), 
							origemRaio.y, 
						    -origemRaio.x * sin(anguloY) + origemRaio.z * cos(anguloY));  	
	/*
	vec3 lookAt 	= vec3(0.0f, 0.0f, 0.0f); //ponto da camera
	vec3 up			= vec3(0.0f, 1.0f, 0.0f); //up da camera
	mat4 view  		= glm::lookAt(origemRaioR, lookAt, up); //matriz do view
	mat4 viewRotated	= rotate(view, anguloY, vec3(0.0f, 1.0f, 0.0f));
	*/
	for (unsigned int y = 0; y < winHeight ; y++) {
		for (unsigned int x = 0; x < winWidth; x++) {
			vec3 posicaoTela = vec3(x, y, 0);
			
			vec3 posicaoMundo = glm::unProject(posicaoTela, model, projection, viewport);
			vec3 direcaoRaio = normalize(vec3(posicaoMundo.x, posicaoMundo.y, -1) - vec3(0));
			
			vec3 direcaoRaioR = vec3(direcaoRaio.x * cos(anguloY) + direcaoRaio.z * sin(anguloY), 
									direcaoRaio.y, 
									-direcaoRaio.x * sin(anguloY) + direcaoRaio.z * cos(anguloY));
			vec3 cor = tracarRaio(origemRaioR, direcaoRaioR, objetos, pontosDeLuz, 0);
			cores.push_back(cor);
		}
	}

	//glfwSetWindowShouldClose(window, true); //habilitar caso queira forcar o encerramento do programa
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	alocarBuffer(cores);
	desenharPixels(vbo);
	cores.clear();
	
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
										exibirFPS();
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

	double lastFPS = 0.0, nowFPS;	
	
	while (!glfwWindowShouldClose(window)) {
		double now = glfwGetTime();
		//double inicioFPS = glfwGetTime();
		
		//double duracaoFPS = inicioFPS - lastFPS;
		double ellapsed = now - last;

		ObjetoImplicito* objetoSelecionado = NULL;
		if (indiceObjeto <= objetos.size()) {
			objetoSelecionado = objetos.at(indiceObjeto);
		}
	

   		if (ellapsed > 1.0f / 30.0f) {//intervalo de atualizacao da tela
		//if (ellapsed > 1e-4) {//intervalo de atualizacao da tela
			
	        display(window);
			glfwSwapBuffers(window);
			if (moveu != RT_STOP && objetoSelecionado != NULL) {
				objetoSelecionado->mover(moveu, ellapsed);
			}
			moveu = RT_STOP;
	
		}
		
		/*
		if (duracaoFPS > 1 / 1000) {//calculo de fps
			lastFPS = inicioFPS;			
			cout << "FPS" << contFPS << endl;
			contFPS++;
		}
		*/
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

	criarObjetos();
	
	inicioPrograma = glfwGetTime();

	GLFW_MainLoop(window);
	
	fimPrograma = glfwGetTime();
	
	glfwDestroyWindow(window);
    glfwTerminate();

    exit(EXIT_SUCCESS);
}