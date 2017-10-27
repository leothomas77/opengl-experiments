#ifndef __RTGLOBALS__
#define __RTGLOBALS__


#include <glm/vec3.hpp>
#include <glm/vec4.hpp> 
#include <glm/mat4x4.hpp> 
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp> 

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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
#define RT_WIDTH 600
#endif

#ifndef RT_HEIGHT
#define RT_HEIGHT 600
#endif

using namespace std;
using namespace glm;

GLuint 	shaderAmbient,
		shaderGouraud,
		shaderPhong,
		shader;
GLuint 	axisVBO[3];

GLuint 	vbo;
GLuint 	meshSize;

//Movimento da camera
vec3 deslocamento(0);
#define PASSO_CAMERA 3.0f
float 	velocidade = 3.0f;
bool 	moveu = false;


double  last;

vector<ObjetoImplicito*> objetos;

unsigned int winWidth 	= RT_WIDTH, winHeight 	= RT_HEIGHT;

float 	angleX 	= 	0.0f,
		angleY	= 	0.0f,
		angleZ	=	0.0f,
		distanciaCamera = 5.0f;

/* the global Assimp scene object */
const aiScene* scene = NULL;
GLuint scene_list = 0;
aiVector3D scene_min, scene_max, scene_center;


#endif //__RTGLOBALS__	