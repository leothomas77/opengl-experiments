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
#ifndef MIN
#define MIN(x,y) (x<y?x:y)
#endif
#ifndef MAX
#define MAX(x,y) (y>x?y:x)
#endif

#define MAX_INTERSECOES 10

#ifndef RT_WIDTH
#define RT_WIDTH 100
#endif

#ifndef RT_HEIGHT
#define RT_HEIGHT 100
#endif

using namespace std;
using namespace glm;

GLuint 	axisVBO[3];

GLuint 	vbo;

//Movimento da camera
vec3 deslocamento(0);
#define PASSO_CAMERA 0.02f

float 	velocidade = 2.0f;
unsigned moveu = RT_STOP;
float 	anguloX = 0.0f, anguloY	= 0.0f, anguloZ	= 0.0f;
//Movimento do objeto
unsigned indiceObjeto = 0;

double  last;
double  ultima;
vector<ObjetoImplicito*> objetos;
vector<PontoDeLuz> pontosDeLuz;
unsigned estadoLuz = 2;//LUZ_12
unsigned int winWidth 	= RT_WIDTH, winHeight 	= RT_HEIGHT;

float 	angleX 	= 	0.0f,
		angleY	= 	0.0f,
		distanciaCamera = 5.0f;
//C[alculo do FPS
long lastFrame;
long delta;
unsigned fps;
long lastFPS;

#endif //__RTGLOBALS__	