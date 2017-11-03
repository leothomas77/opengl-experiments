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

#define RT_WIDTH 400
#define RT_HEIGHT 400

using namespace std;
using namespace glm;

GLuint 	vbo;

//Movimento da camera
#define PASSO_CAMERA 0.3f


float 	velocidade = 2.0f;
unsigned moveu = RT_STOP;
float 	anguloX = 0.0f, anguloY	= 0.0f, anguloZ	= 0.0f;
//Movimento do objeto
unsigned indiceObjeto = 0; //objeto selecionado na cena

double  last;
double  ultima;
vector<ObjetoImplicito*> objetos;
vector<PontoDeLuz> pontosDeLuz;
unsigned estadoLuz = 2;//LUZ_12
unsigned int winWidth 	= RT_WIDTH, winHeight 	= RT_HEIGHT;
double aspect = RT_WIDTH / double(RT_HEIGHT);

//C[alculo do FPS
double inicioPrograma = 0.0, fimPrograma = 0.0;
unsigned contFPS = 0;

#endif //__RTGLOBALS__	