#ifndef __RTGLOBALS__
#define __RTGLOBALS__

#include <glm/vec3.hpp>
#include <glm/vec4.hpp> 
#include <glm/mat4x4.hpp> 
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp> 

#include "objetos.h"

#ifndef BUFFER_OFFSET 
#define BUFFER_OFFSET(i) ((char *)NULL + (i))
#endif
#define RT_APP "Raycasting v1.0 - Leonardo Thomas"
#define RT_WIDTH 100
#define RT_HEIGHT 100

using namespace std;
using namespace glm;

GLuint 	vbo;

//Movimento da camera
#define PASSO_CAMERA 0.3f

float DEBUG = false;

float 	velocidade = 2.0f;
unsigned moveu = RT_STOP;
float 	anguloX = 0.0f, anguloY	= 0.0f, anguloZ	= 0.0f;
//Movimento do objeto
unsigned indiceObjeto = 0; //objeto selecionado na cena

double  last;
vector<ObjetoImplicito*> objetos;
vector<PontoDeLuz> pontosDeLuz;
unsigned estadoLuz = 2;//LUZ_12
unsigned int winWidth 	= RT_WIDTH, winHeight 	= RT_HEIGHT;
//Calculo do FPS
unsigned contFPS = 0;
double lastFPS = 0.0, intervaloFPS = 0.0;
char  titulo[50] = RT_APP;
#endif //__RTGLOBALS__	